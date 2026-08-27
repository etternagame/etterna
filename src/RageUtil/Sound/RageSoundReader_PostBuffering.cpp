#include "Etterna/Globals/global.h"
#include "RageSoundReader_PostBuffering.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageSoundUtil.h"

/*
 * This filter is normally inserted after extended buffering, implementing
 * properties that do not seek the sound, allowing these properties to be
 * changed with low latency.
 */

RageMutex g_Mutex("PostBuffering");
static float g_fMasterVolume = 1.F;
static float g_fActionVolume = 1.F;
static float g_fBGMVolume = 1.F;

RageSoundReader_PostBuffering::RageSoundReader_PostBuffering(
  RageSoundReader* pSource)
  : RageSoundReader_Filter(pSource)
{
	m_fVolume = 1.0f;
}

void
RageSoundReader_PostBuffering::SetMasterVolume(float fVolume) {
	LockMut(g_Mutex);
	g_fMasterVolume = fVolume;
}

void
RageSoundReader_PostBuffering::SetActionVolume(float fVolume)
{
	LockMut(g_Mutex);
	g_fActionVolume = fVolume;
}

void
RageSoundReader_PostBuffering::SetBGMVolume(float fVolume)
{
	LockMut(g_Mutex);
	g_fBGMVolume = fVolume;
}

float
RageSoundReader_PostBuffering::GetMasterVolume() {
	return g_fMasterVolume;
}

float
RageSoundReader_PostBuffering::GetBGMVolume() const
{
	if (m_bIsBGM) {
		return g_fBGMVolume;
	}
	return 1.F;
}

float
RageSoundReader_PostBuffering::GetActionVolume() const
{
	if (m_bIsAction) {
		return g_fActionVolume;
	}
	return 1.F;
}

int
RageSoundReader_PostBuffering::Read(float* pBuf, int iFrames)
{
	iFrames = m_pSource->Read(pBuf, iFrames);
	if (iFrames < 0)
		return iFrames;

	g_Mutex.Lock();
	float fVolume = m_fVolume * g_fMasterVolume * g_fMasterVolume *
					GetBGMVolume() * GetActionVolume();
	CLAMP(fVolume, 0, 1);
	g_Mutex.Unlock();

	if(fVolume != 1.0F)
		RageSoundUtil::Attenuate(pBuf, iFrames * this->GetNumChannels(), fVolume);

	return iFrames;
}

bool
RageSoundReader_PostBuffering::SetProperty(const std::string& sProperty,
										   float fValue)
{
	if (sProperty == "Volume") {
		m_fVolume = fValue;
		return true;
	} else if (sProperty == "Action") {
		m_bIsAction = fValue != 0.F;
		return true;
	} else if (sProperty == "BGM") {
		m_bIsBGM = fValue != 0.F;
		return true;
	}

	return RageSoundReader_Filter::SetProperty(sProperty, fValue);
}
