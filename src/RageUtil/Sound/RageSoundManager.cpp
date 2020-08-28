/*
 * This manager has several distinct purposes:
 *
 * Load the sound driver, and handle most communication between it and
 * RageSound. Factory and reference count RageSoundReader objects for RageSound.
 * User-level:
 *  - global volume management
 *  - sound detaching ("play and delete when done playing")
 */

#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Misc/Preference.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Core/Services/Locator.hpp"
#include "RageSound.h"
#include "RageSoundManager.h"
#include "RageSoundReader_Preload.h"
#include "RageUtil/Misc/RageTimer.h"
#include "RageUtil/Utils/RageUtil.h"
#include "arch/Sound/RageSoundDriver.h"

#include <algorithm>

/*
 * The lock ordering requirements are:
 * RageSound::Lock before g_SoundManMutex
 * RageSound::Lock must not be locked when calling driver calls (since the
 * driver may lock a mutex and then make RageSound calls back)
 *
 * Do not make RageSound calls that might lock while holding g_SoundManMutex.
 */

static RageMutex g_SoundManMutex("SoundMan");
static Preference<std::string> g_sSoundDrivers(
  "SoundDrivers",
  ""); // "" == DEFAULT_SOUND_DRIVER_LIST

RageSoundManager* SOUNDMAN = nullptr;

RageSoundManager::RageSoundManager()
  : m_pDriver(nullptr)
{
}

static LocalizedString COULDNT_FIND_SOUND_DRIVER(
  "RageSoundManager",
  "Couldn't find a sound driver that works");
void
RageSoundManager::Init()
{
	m_pDriver = RageSoundDriver::Create(g_sSoundDrivers);
	if (m_pDriver == nullptr)
		RageException::Throw("%s",
							 COULDNT_FIND_SOUND_DRIVER.GetValue().c_str());
}

RageSoundManager::~RageSoundManager()
{
	/* Don't lock while deleting the driver (the decoder thread might deadlock).
	 */
	delete m_pDriver;
	for (auto& s : m_mapPreloadedSounds) {
		delete s.second;
	}

	m_mapPreloadedSounds.clear();
}

void
RageSoundManager::fix_bogus_sound_driver_pref(std::string const& valid_setting)
{
	g_sSoundDrivers.Set(valid_setting);
}

/*
 * Previously, we went to some lengths to shut down sounds before exiting
 * threads. The only other thread that actually starts sounds is SOUND.  Doing
 * this was ugly; instead, let's shut down the driver early, stopping all
 * sounds.  We don't want to delete SOUNDMAN early, since those threads are
 * still using it; just shut down the driver.
 */
void
RageSoundManager::Shutdown()
{
	SAFE_DELETE(m_pDriver);
}

void
RageSoundManager::StartMixing(RageSoundBase* pSound)
{
	if (m_pDriver != nullptr)
		m_pDriver->StartMixing(pSound);
}

void
RageSoundManager::StopMixing(RageSoundBase* pSound)
{
	if (m_pDriver != nullptr)
		m_pDriver->StopMixing(pSound);
}

bool
RageSoundManager::Pause(RageSoundBase* pSound, bool bPause)
{
	if (m_pDriver == nullptr)
		return false;

	return m_pDriver->PauseMixing(pSound, bPause);
}

int64_t
RageSoundManager::GetPosition(RageTimer* pTimer) const
{
	if (m_pDriver == nullptr)
		return 0;
	return m_pDriver->GetHardwareFrame(pTimer);
}

void
RageSoundManager::Update()
{
	/* Scan m_mapPreloadedSounds for sounds that are no longer loaded, and
	 * delete them. */
	g_SoundManMutex
	  .Lock(); /* lock for access to m_mapPreloadedSounds, owned_sounds */
	{
		std::map<std::string, RageSoundReader_Preload*>::iterator it, next;
		it = m_mapPreloadedSounds.begin();

		while (it != m_mapPreloadedSounds.end()) {
			next = it;
			++next;
			if (it->second->GetReferenceCount() == 1) {
				if (PREFSMAN->m_verbose_log > 1)
					Locator::getLogger()->trace("Deleted old sound \"{}\"", it->first.c_str());
				delete it->second;
				m_mapPreloadedSounds.erase(it);
			}

			it = next;
		}
	}

	g_SoundManMutex.Unlock(); /* finished with m_mapPreloadedSounds */

	if (m_pDriver != nullptr)
		m_pDriver->Update();
}

float
RageSoundManager::GetPlayLatency() const
{
	if (m_pDriver == nullptr)
		return 0;

	return m_pDriver->GetPlayLatency();
}

int
RageSoundManager::GetDriverSampleRate() const
{
	if (m_pDriver == nullptr)
		return 44100;

	return m_pDriver->GetSampleRate();
}

/* If the given path is loaded, return a copy; otherwise return NULL.
 * It's the caller's responsibility to delete the result. */
RageSoundReader*
RageSoundManager::GetLoadedSound(const std::string& sPath_)
{
	LockMut(g_SoundManMutex); /* lock for access to m_mapPreloadedSounds */

	std::string sPath(sPath_);
	sPath = make_lower(sPath);
	std::map<std::string, RageSoundReader_Preload*>::const_iterator it;
	it = m_mapPreloadedSounds.find(sPath);
	if (it == m_mapPreloadedSounds.end())
		return nullptr;

	return it->second->Copy();
}

/* Add the sound to the set of loaded sounds that can be copied for reuse.
 * The sound will be kept in memory as long as there are any other references
 * to it; once we hold the last one, we'll release it. */
void
RageSoundManager::AddLoadedSound(const std::string& sPath_,
								 RageSoundReader_Preload* pSound)
{
	LockMut(g_SoundManMutex); /* lock for access to m_mapPreloadedSounds */

	/* Don't AddLoadedSound a sound that's already registered.  It should have
	 * been used in GetLoadedSound. */
	std::string sPath(sPath_);
	sPath = make_lower(sPath);
	std::map<std::string, RageSoundReader_Preload*>::const_iterator it;
	it = m_mapPreloadedSounds.find(sPath);
	ASSERT_M(it == m_mapPreloadedSounds.end(), sPath);

	m_mapPreloadedSounds[sPath] = pSound->Copy();
}

static Preference<float> g_fSoundVolume("SoundVolume", 1.0f);

void
RageSoundManager::SetMixVolume()
{
	g_SoundManMutex.Lock(); /* lock for access to m_fMixVolume */
	m_fMixVolume = std::clamp(g_fSoundVolume.Get(), 0.0f, 1.0f);
	g_SoundManMutex.Unlock(); /* finished with m_fMixVolume */
}

void
RageSoundManager::SetVolumeOfNonCriticalSounds(float fVolumeOfNonCriticalSounds)
{
	g_SoundManMutex
	  .Lock(); /* lock for access to m_fVolumeOfNonCriticalSounds */
	m_fVolumeOfNonCriticalSounds = fVolumeOfNonCriticalSounds;
	g_SoundManMutex.Unlock(); /* finished with m_fVolumeOfNonCriticalSounds */
}
