/* Handle loading and decoding of sounds.
 *
 * For small files, pre-decode the entire file into a regular buffer.  We
 * might want to play many samples at once, and we don't want to have to decode
 * 5-10 mp3s simultaneously during play.
 *
 * For larger files, decode them on the fly.  These are usually music, and
 * there's usually only one of those playing at a time.  When we get updates,
 * decode data at the same rate we're playing it.  If we don't do this, and
 * we're being read in large chunks, we're forced to decode in larger chunks as
 * well, which can cause framerate problems.
 *
 * Error handling:
 * Decoding errors (eg. CRC failures) will be recovered from when possible.
 *
 * When they can't be recovered, the sound will stop (unless loop or !autostop)
 * and the error will be available in GetError().
 *
 * Seeking past the end of the file will throw a warning and rewind.
 */

#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Core/Services/Locator.hpp"
#include "RageSound.h"
#include "RageSoundManager.h"
#include "Etterna/Screen/Others/Screen.h"
#include "RageSoundUtil.h"
#include "Etterna/Models/Lua/LuaReference.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageSoundReader_Extend.h"
#include "RageSoundReader_FileReader.h"
#include "RageSoundReader_Pan.h"
#include "RageSoundReader_PitchChange.h"
#include "RageSoundReader_PostBuffering.h"
#include "RageSoundReader_Preload.h"
#include "RageSoundReader_Resample_Good.h"
#include "RageSoundReader_ThreadedBuffer.h"
#include "fft.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#define samplerate() m_pSource->GetSampleRate()

RageSoundParams::RageSoundParams()
  : m_StartTime(RageZeroTimer)
{
}

RageSoundLoadParams::RageSoundLoadParams() = default;

RageSound::RageSound()
  : m_Mutex("RageSound")
  , m_pSource(nullptr)
  , m_sFilePath("")
  , m_Param()
  , m_iStreamFrame(0)
  , m_sError("")
{
	ASSERT(SOUNDMAN != NULL);
}

RageSound::~RageSound()
{
	Unload();
	// Make sure we free fftPlan after Unload()ing
	// Otherwise, the decode thread could lock recentSamplesMutex in 
	// RageSound::GetDataToPlay and if we lock it first, we free the 
	// fftPlan and then it tries to use it.
	// Unload() removes this RageSound from the arrays the decode thread uses
	std::lock_guard<std::mutex> guard(recentSamplesMutex);
	if (fftPlan)
		mufft_free_plan_1d(fftPlan);
}

RageSound::RageSound(const RageSound& cpy)
  : RageSoundBase(cpy)
  , m_Mutex("RageSound")
{
	ASSERT(SOUNDMAN != NULL);

	m_pSource = nullptr;

	*this = cpy;
}

RageSound&
RageSound::operator=(const RageSound& cpy)
{
	LockMut(cpy.m_Mutex);

	/* If m_bDeleteWhenFinished, then nobody that has a reference to the sound
	 * should be making copies. */
	ASSERT(!cpy.m_bDeleteWhenFinished);

	m_Param = cpy.m_Param;
	m_iStreamFrame = cpy.m_iStreamFrame;
	m_iStoppedSourceFrame = cpy.m_iStoppedSourceFrame;
	m_bPlaying = false;
	m_bPaused = cpy.m_bPaused;
	m_bDeleteWhenFinished = false;

	if (m_pSource != nullptr) {
		delete m_pSource;
	}
	if (cpy.m_pSource != nullptr)
		m_pSource = cpy.m_pSource->Copy();
	else
		m_pSource = nullptr;

	m_sFilePath = cpy.m_sFilePath;

	return *this;
}

void
RageSound::Unload()
{
	if (IsPlaying())
		StopPlaying();

	LockMut(m_Mutex);

	if (m_pSource != nullptr) {
		delete m_pSource;
	}
	m_pSource = nullptr;

	m_sFilePath = "";
}

/* The sound will self-delete itself when it stops playing. If the sound is not
 * playing, the sound will be deleted immediately. The caller loses ownership
 * of the sound. */
void
RageSound::DeleteSelfWhenFinishedPlaying()
{
	m_Mutex.Lock();

	if (!m_bPlaying) {
		m_Mutex.Unlock();
		delete this;
		return;
	}

	m_bDeleteWhenFinished = true;
	m_Mutex.Unlock();
}

bool
RageSound::IsLoaded() const
{
	return m_pSource != nullptr;
}

class RageSoundReader_Silence : public RageSoundReader
{
  public:
	int GetLength() const override { return 0; }
	int GetLength_Fast() const override { return 0; }
	int SetPosition(int iFrame) override { return 1; }
	int Read(float* pBuf, int iFrames) override
	{
		return RageSoundReader::END_OF_FILE;
	}
	RageSoundReader* Copy() const override
	{
		return new RageSoundReader_Silence;
	}
	int GetSampleRate() const override { return 44100; }
	unsigned GetNumChannels() const override { return 1; }
	int GetNextSourceFrame() const override { return 0; }
	float GetStreamToSourceRatio() const override { return 1.0f; }
	std::string GetError() const override { return ""; }
};

bool
RageSound::Load(const std::string& sSoundFilePath)
{
	/* Automatically determine whether to precache */
	/* TODO: Hook this up to a pref? */
	return Load(sSoundFilePath, false);
}

bool
RageSound::Load(const std::string& sSoundFilePath,
				bool bPrecache,
				const RageSoundLoadParams* pParams)
{
	Locator::getLogger()->debug("RageSound: Load \"{}\" (precache: {})",
		sSoundFilePath.c_str(),
		bPrecache);

	if (pParams == nullptr) {
		static const RageSoundLoadParams Defaults;
		pParams = &Defaults;
	}

	/* If this sound is already preloaded and held by SOUNDMAN, just make a copy
	 * of that.  Since RageSoundReader_Preload is refcounted, this is cheap. */
	RageSoundReader* pSound = SOUNDMAN->GetLoadedSound(sSoundFilePath);
	auto bNeedBuffer = true;
	if (pSound == nullptr) {
		std::string error;
		bool bPrebuffer;
		pSound = RageSoundReader_FileReader::OpenFile(
		  sSoundFilePath, error, &bPrebuffer);
		if (pSound == nullptr) {
			Locator::getLogger()->warn("RageSound::Load: error opening sound \"{}\": {}",
					  sSoundFilePath.c_str(), error.c_str());

			pSound = new RageSoundReader_Silence;
		}

		/* If the sound is prebuffered into memory, we don't need to buffer
		 * reads. */
		if (bPrebuffer)
			bNeedBuffer = false;
	} else {
		/* The sound we were given from SOUNDMAN is already preloaded. */
		bPrecache = false;
		bNeedBuffer = false;
	}

	LoadSoundReader(pSound);

	/* Try to precache.  Do this after calling LoadSoundReader() to put the
	 * sound in this->m_pSource, so we preload after resampling. */
	if (bPrecache) {
		if (RageSoundReader_Preload::PreloadSound(m_pSource)) {
			/* We've preloaded the sound.  Pass it to SOUNDMAN, for reuse. */
			SOUNDMAN->AddLoadedSound(
			  sSoundFilePath,
			  reinterpret_cast<RageSoundReader_Preload*>(m_pSource));
		}
		bNeedBuffer = false;
	}

	m_pSource = new RageSoundReader_Extend(m_pSource);
	if (bNeedBuffer)
		m_pSource = new RageSoundReader_ThreadedBuffer(m_pSource);
	m_pSource = new RageSoundReader_PostBuffering(m_pSource);

	if (pParams->m_bSupportRateChanging) {
		auto* pRate = new RageSoundReader_PitchChange(m_pSource);
		m_pSource = pRate;
	}

	if (pParams->m_bSupportPan)
		m_pSource = new RageSoundReader_Pan(m_pSource);

	m_sFilePath = sSoundFilePath;

	m_Mutex.SetName(
	  ssprintf("RageSound (%s)", Basename(sSoundFilePath).c_str()));

	return true;
}

void
RageSound::LoadSoundReader(RageSoundReader* pSound)
{
	Unload();

	m_iStreamFrame = m_iStoppedSourceFrame = 0;

	const auto iNeededRate = SOUNDMAN->GetDriverSampleRate();
	auto bSupportRateChange = false;
	if (iNeededRate != pSound->GetSampleRate() || bSupportRateChange) {
		auto* Resample = new RageSoundReader_Resample_Good(pSound, iNeededRate);
		pSound = Resample;
	}

	m_pSource = pSound;
}

/*
 * Retrieve audio data, for mixing.  At the time of this call, the frameno at
 * which the sound will be played doesn't have to be known.  Once committed, and
 * the frameno is known, call CommitPCMData.
 *
 * RageSound::GetDataToPlay and RageSound::FillBuf are the main threaded API.
 * These need to execute without blocking other threads from calling eg.
 * GetPositionSeconds, since they may take some time to run.
 *
 * On underrun, if no data was read, returns WOULD_BLOCK.  On end of file, if no
 * data was read, returns END_OF_FILE.  If any data is read, it is returned;
 * these conditions are masked and will be seen on the next call.  Otherwise,
 * the requested number of frames will always be returned.
 */
int
RageSound::GetDataToPlay(float* pBuffer,
						 int iFrames,
						 int64_t& iStreamFrame,
						 int& iFramesStored)
{
	/* We only update m_iStreamFrame; only take a shared lock, so we don't block
	 * the main thread. */
	//	LockMut(m_Mutex);

	ASSERT_M(m_bPlaying, ssprintf("%p", this));
	ASSERT(m_pSource != nullptr);

	iFramesStored = 0;
	iStreamFrame = m_iStreamFrame;

	while (iFrames > 0) {
		auto fRate = 1.0f;
		int iSourceFrame;

		/* Read data from our source. */
		auto iGotFrames = m_pSource->RetriedRead(
		  pBuffer + (iFramesStored * m_pSource->GetNumChannels()),
		  iFrames,
		  &iSourceFrame,
		  &fRate);

		if (iGotFrames == RageSoundReader::RSRERROR) {
			m_sError = m_pSource->GetError();
			Locator::getLogger()->warn("Decoding {} failed: {}",
					  GetLoadedFilePath().c_str(),
					  m_sError.c_str());
		}

		if (iGotFrames < 0) {
			if (iFramesStored == 0)
				return iGotFrames;

			break;
		}

		m_Mutex.Lock();
		m_StreamToSourceMap.Insert(
		  m_iStreamFrame, iGotFrames, iSourceFrame, fRate);
		m_Mutex.Unlock();

		m_iStreamFrame += iGotFrames;

		iFramesStored += iGotFrames;
		iFrames -= iGotFrames;
	}
	if (m_pSource->GetNumChannels() == 1)
		RageSoundUtil::ConvertMonoToStereoInPlace(pBuffer, iFramesStored);
	if (soundPlayCallback != nullptr) {
		std::lock_guard<std::mutex> guard(recentSamplesMutex);
		// checking to see that the lua function exists
		// also check to see that the source exists
		// if the source exists, the sound is still valid
		// why check to see that the source exists?
		// the above lock has a chance to race:
		//  lock is grabbed at ~RageSound, and source is nulled
		//  lock is released, and then this code is executed
		//  so if the source disappears before the lock is released, bad.
		if (!soundPlayCallback->IsNil() && soundPlayCallback->IsSet() &&
			  m_pSource != nullptr) {
			unsigned int currentSamples = recentPCMSamples.size();
			auto samplesToCopy =
			  std::min(iFramesStored * m_pSource->GetNumChannels(),
					   recentPCMSamplesBufferSize - currentSamples);
			auto until = pBuffer + samplesToCopy;
			copy(pBuffer, until, back_inserter(recentPCMSamples));
			if (recentPCMSamples.size() >= recentPCMSamplesBufferSize) {
				if (fftPlan != nullptr) {
					mufft_execute_plan_1d(
					  fftPlan, fftBuffer.data(), recentPCMSamples.data());
					recentPCMSamples.clear();
					pendingPlayBackCall = true;
				}
			}
		}
	}
	return iFramesStored;
}

void
RageSound::ExecutePlayBackCallback(Lua* L)
{
	if (soundPlayCallback == nullptr || !pendingPlayBackCall)
		return;
	{
		// Make sure we're not holding this mutex when we run the lua callback/script
		// Otherwise it can deadlock if it calls ClearPlayBackCallback which locks g_mutex
		// and GameSoundManager tries to delete the RageSound which will lock this one after
		// g_mutex (In the opposite order)
		// This *should* be fine since even if soundPlayCallback is set to nil by another thread while
		// we are running it, since the `soundPlayCallback->PushSelf` call which we do while holding
		// the mutex does a lua_rawgeti which pushes a reference to the callback to the lua stack
		// which is owned by the lua VM and garbage collected, so it should *not* get freed.
		std::lock_guard<std::mutex> guard(recentSamplesMutex);
		// Recheck in case it changed inbetween the earlier check and the mutex lock
		if (soundPlayCallback == nullptr)
			return;
		soundPlayCallback->PushSelf(L);
		lua_newtable(L);
		for (size_t i = 0; i < fftBuffer.size(); ++i) {
			auto r = fftBuffer[i].real;
			auto im = fftBuffer[i].imag;
			lua_pushnumber(
			  L,
			  (r * r + im * im) /
				(0.01f + RageSoundReader_PostBuffering::GetMasterVolume()) /
				(0.01f + RageSoundReader_PostBuffering::GetMasterVolume()) /
				15.f);
			lua_rawseti(L, -2, i + 1);
		}
		pendingPlayBackCall = false;
	}
	// TODO: Change this parameter from RageSound to the sampling rate
	//       by removing this PushSelf.
	//       We're not holding the g_Mutex from GameSoundManager when we call
	//       the callback for it's global g_playing, which I think might cause
	//       issues depending on what the lua callback does with it.
	PushSelf(L);
	std::string error;
	LuaHelpers::RunScriptOnStack(L, error, 2, 0, false); // 2 args, 0 returns
	if (error != "") // hack for now because we're bad and didn't deal with
					 // clearing this -mina
		soundPlayCallback->Unset();
}

/* Indicate that a block of audio data has been written to the device. */
void
RageSound::CommitPlayingPosition(int64_t iHardwareFrame,
								 int64_t iStreamFrame,
								 int iGotFrames)
{
	m_Mutex.Lock();
	m_HardwareToStreamMap.Insert(iHardwareFrame, iGotFrames, iStreamFrame);
	m_Mutex.Unlock();
}

/* Start playing from the current position. */
void
RageSound::StartPlaying(float fGiven, bool forcedTime)
{
	ASSERT(!m_bPlaying);

	// Move to the start position or the given time.
	if (forcedTime)
		SetPositionFrames(lround(fGiven * samplerate()));
	else
		SetPositionFrames(lround(m_Param.m_StartSecond * samplerate()));

	/* If m_StartTime is in the past, then we probably set a start time but took
	 * too long loading.  We don't want that; log it, since it can be unobvious.
	 */
	if (!m_Param.m_StartTime.IsZero() && m_Param.m_StartTime.Ago() > 0)
		Locator::getLogger()->debug("Sound \"{}\" has a start time {} seconds in the past",
				   GetLoadedFilePath().c_str(),
				   m_Param.m_StartTime.Ago());

	/* Tell the sound manager to start mixing us. */
	//	LOG->Trace("set playing true for %p (StartPlaying) (%s)", this,
	// this->GetLoadedFilePath().c_str());

	m_bPlaying = true;

	/* Save the attract volume, so changes don't affect previously played
	 * sounds. */
	m_Param.m_fAttractVolume = SOUNDMAN->GetVolumeOfNonCriticalSounds();
	ApplyParams();

	/* Don't lock while calling SOUNDMAN driver calls. */
	ASSERT(!m_Mutex.IsLockedByThisThread());

	SOUNDMAN->StartMixing(this);
	Pause(m_bPaused);

	//	LOG->Trace("StartPlaying %p finished (%s)", this,
	// this->GetLoadedFilePath().c_str());
}

void
RageSound::SetPositionSeconds(float fGiven)
{
	if (m_bPlaying) {
		Stop();
		// Sometimes the state of this bool becomes incorrect if running this
		// function very very often, so force it here in the hopes that
		// StartPlaying will fix everything. (the result in these cases is
		// instead a lot of perceived lag, but that's better than a crash to me)
		m_bPlaying = false;
		// SetPositionFrames(lround(fGiven * samplerate()));

		// and funnily enough in conjunction with the above hack if we dont put
		// this into a while loop the song sync breaks in those situations
		while (!m_bPlaying)
			StartPlaying(fGiven, true);
	}
}

void
RageSound::StopPlaying()
{
	/* Don't lock while calling SOUNDMAN driver calls. */
	ASSERT(!m_Mutex.IsLockedByThisThread());

	/* Tell the sound driver to stop mixing this sound. */
	SOUNDMAN->StopMixing(this);
}

/* This is called by sound drivers when we're done playing. */
void
RageSound::SoundIsFinishedPlaying()
{
	if (!m_bPlaying)
		return;

	/* Get our current hardware position. */
	auto iCurrentHardwareFrame = SOUNDMAN->GetPosition(nullptr);

	m_Mutex.Lock();

	if (m_bDeleteWhenFinished) {
		m_bDeleteWhenFinished = false;
		m_Mutex.Unlock();
		delete this;
		return;
	}

	/* Lock the mutex after calling SOUNDMAN->GetPosition().  We must not make
	 * driver calls with our mutex locked (driver mutex < sound mutex). */
	if (!m_HardwareToStreamMap.IsEmpty() && !m_StreamToSourceMap.IsEmpty())
		m_iStoppedSourceFrame = static_cast<int>(
		  GetSourceFrameFromHardwareFrame(iCurrentHardwareFrame));

	//	LOG->Trace("set playing false for %p (SoundIsFinishedPlaying) (%s)",
	// this, this->GetLoadedFilePath().c_str());
	m_bPlaying = false;

	m_HardwareToStreamMap.Clear();
	m_StreamToSourceMap.Clear();

	//	LOG->Trace("SoundIsFinishedPlaying %p finished (%s)", this,
	// this->GetLoadedFilePath().c_str());

	m_Mutex.Unlock();
}

void
RageSound::Play(bool is_action, const RageSoundParams* pParams)
{
	if (m_pSource == nullptr) {
		Locator::getLogger()->warn("RageSound::Play: sound not loaded");
		return;
	}
	if (is_action && PREFSMAN->m_MuteActions) {
		return;
	}

	if (IsPlaying()) {
		PlayCopy(is_action, pParams);
		return;
	}

	if (pParams != nullptr)
		SetParams(*pParams);

	StartPlaying();
}

void
RageSound::PlayCopy(bool is_action, const RageSoundParams* pParams) const
{
	if (is_action && PREFSMAN->m_MuteActions) {
		return;
	}
	auto* pSound = new RageSound(*this);

	if (pParams != nullptr)
		pSound->SetParams(*pParams);

	pSound->StartPlaying();
	pSound->DeleteSelfWhenFinishedPlaying();
}

void
RageSound::Stop()
{
	StopPlaying();
}

bool
RageSound::Pause(bool bPause)
{
	if (m_pSource == nullptr) {
		Locator::getLogger()->warn("RageSound::Pause: sound not loaded");
		return false;
	}
	m_bPaused = bPause;

	return SOUNDMAN->Pause(this, bPause);
}

float
RageSound::GetLengthSeconds()
{
	if (m_pSource == nullptr) {
		Locator::getLogger()->warn("RageSound::GetLengthSeconds: sound not loaded");
		return -1;
	}

	auto iLength = m_pSource->GetLength();

	if (iLength < 0) {
		Locator::getLogger()->warn("GetLengthSeconds failed on {}: {}",
				  GetLoadedFilePath().c_str(),
				  m_pSource->GetError().c_str());
		return -1;
	}

	return iLength / 1000.f; // ms -> secs
}

int
RageSound::GetSourceFrameFromHardwareFrame(int64_t iHardwareFrame,
										   bool* bApproximate) const
{
	if (m_HardwareToStreamMap.IsEmpty() || m_StreamToSourceMap.IsEmpty())
		return 0;

	bool bApprox;
	auto iStreamFrame = m_HardwareToStreamMap.Search(iHardwareFrame, &bApprox);
	if ((bApproximate != nullptr) && bApprox)
		*bApproximate = true;
	auto iSourceFrame = m_StreamToSourceMap.Search(iStreamFrame, &bApprox);
	if ((bApproximate != nullptr) && bApprox)
		*bApproximate = true;
	return static_cast<int>(iSourceFrame);
}

/* If non-NULL, approximate is set to true if the returned time is approximated
 * because of underrun, the sound not having started (after Play()) or finished
 * (after EOF) yet.
 *
 * If non-NULL, Timestamp is set to the real clock time associated with the
 * returned sound position.  We might take a variable amount of time before
 * grabbing the timestamp (to lock SOUNDMAN); we might lose the scheduler after
 * grabbing it, when releasing SOUNDMAN.
 */
float
RageSound::GetPositionSeconds(bool* bApproximate, RageTimer* pTimestamp)
{
	/* Get our current hardware position. */
	auto iCurrentHardwareFrame = SOUNDMAN->GetPosition(pTimestamp);

	/* Lock the mutex after calling SOUNDMAN->GetPosition().  We must not make
	 * driver calls with our mutex locked (driver mutex < sound mutex). */
	LockMut(m_Mutex);

	if (bApproximate != nullptr)
		*bApproximate = false;

	/* If we're not playing, just report the static position. */
	if (!IsPlaying())
		return m_iStoppedSourceFrame / static_cast<float>(samplerate());

	/* If we don't yet have any position data, CommitPlayingPosition hasn't yet
	 * been called at all, so guess what we think the real time is. */
	if (m_HardwareToStreamMap.IsEmpty() || m_StreamToSourceMap.IsEmpty()) {
		if (bApproximate != nullptr)
			*bApproximate = true;
		return m_iStoppedSourceFrame / static_cast<float>(samplerate());
	}

	const auto iSourceFrame =
		GetSourceFrameFromHardwareFrame(iCurrentHardwareFrame, bApproximate);

	const auto fSeconds = iSourceFrame / static_cast<float>(samplerate());

	if (m_bPaused || pTimestamp == nullptr) {
		return fSeconds;
	}

	// GetPosition is untrustworthy, so use a high resolution clock to
	// clean it up. Mostly, jitter. This can be bad enough to cause visible
	// jittering of the rendered notes, and, even when not visible, can add
	// milliseconds of jitter to a players' hit judgments.

	// The last time we called into GetPosition.
	auto tm = m_Pasteurizer.tm;

	// Used to do linear extrapolation in time when GetPosition returns the same
	// value multiple times.
	auto hwTime = m_Pasteurizer.hwTime;
	auto hwPosition = m_Pasteurizer.hwPosition;

	// An abitrary point in the past where both the wall-time and music-time are
	// known. We can't use hwTime as it updates too often.
	auto syncTime = m_Pasteurizer.syncTime;
	auto syncPosition = m_Pasteurizer.syncPosition;

	// The accumulated error from what we think the sync actually is, as opposed
	// to what GetPosition + extrapolation is telling us. To follow clock drift,
	// it is exponentially decayed towards the time according to GetPosition. We
	// expect drift to be linear (<<< exponential) so if the time from
	// GetPosition is good then we follow it very closely. If GetPosition is
	// noisy then this acts like a low-pass filter. If acc exceeds 50ms, we
	// reset.
	//
	// At 1.0x rate and high fps, it should never exceed 1-2 ms even when
	// GetPosition is noisy, unless it skips, in which case it will gradually
	// correct the sync over ~3 seconds. The assumption here is that, if there
	// is a skip, we are already out of sync, and it is better to not cause a
	// visual skip or instantaneously move the note out from under the player.
	// Plus, skips tend to be matched by a skip in the opposite direction
	// shortly after (with wall-time as a reference frame, not absolutely). We
	// will never correct more 50ms out, which is 3 frames at 60fps.
	//
	// Between frames acc is stored as (acc - last_frame_err) and you get the
	// true acc from (acc + this_frame_err).
	auto acc = m_Pasteurizer.acc;

	RageTimer z1 = tm;
	tm = *pTimestamp;

	if (hwPosition != fSeconds) {
		hwTime = tm;
		hwPosition = fSeconds;
	}

	const auto rate = m_Param.m_fSpeed;
	const auto extrapolatedPosition = fSeconds + (tm - hwTime) * rate;
	const auto syncTimeDelta = (tm - syncTime) * rate;
	const auto syncPositionDelta = extrapolatedPosition - syncPosition;
	const auto err = syncTimeDelta - syncPositionDelta;

	// For values we expect, exp(-(tm - z1)) ≈ 1 - (tm - z1)
	acc += err;
	acc *= expf(-(tm - z1));
	auto correction = acc;
	acc -= err;

	if (fabsf(correction) > 0.05f) {
		acc = 0;
		correction = 0;
		syncTime = hwTime;
		syncPosition = hwPosition;
	}

	const auto correctedPosition = extrapolatedPosition + correction;
	const auto vsyncAdjust = DISPLAY->GetFrameTimingAdjustment(tm.tm);

#if defined(TRACY_ENABLE) && defined(TRACE_BUTTER)
	TracyPlot("acc ms", 1000.0f * (correction));
	if (syncTime.tm == tm.tm) TracyMessageL("sync reset");
	TracyPlot("extrapolatedPosition error from rdtsc ms", 1000.0f * err);
	TracyPlot("correctedPosition error from rdtsc ms", 1000.0f *
		((tm - syncTime) * rate - (correctedPosition - syncPosition)));
	TracyPlot("vsyncAdjust ms", 1000.0f * vsyncAdjust);
#endif

	m_Pasteurizer.tm = tm;
	m_Pasteurizer.hwTime = hwTime;
	m_Pasteurizer.hwPosition = hwPosition;
	m_Pasteurizer.syncTime = syncTime;
	m_Pasteurizer.syncPosition = syncPosition;
	m_Pasteurizer.acc = acc;

	*pTimestamp += vsyncAdjust;
	return correctedPosition + vsyncAdjust * rate;
}

bool
RageSound::SetPositionFrames(int iFrames)
{
	LockMut(m_Mutex);

	if (m_pSource == nullptr) {
		Locator::getLogger()->warn("RageSound::SetPositionFrames({}): sound not loaded", iFrames);
		return false;
	}

	auto iRet = m_pSource->SetPosition(iFrames);
	if (iRet == -1) {
		m_sError = m_pSource->GetError();
		Locator::getLogger()->warn("SetPositionFrames: seek {} failed: {}",
				  GetLoadedFilePath().c_str(),
				  m_sError.c_str());
	} else if (iRet == 0) {
		/* Seeked past EOF. */
		Locator::getLogger()->warn("SetPositionFrames: {} samples is beyond EOF in {}",
				  iFrames,
				  GetLoadedFilePath().c_str());
	} else {
		m_iStoppedSourceFrame = iFrames;
	}

	return iRet == 1;
}

float
RageSound::GetPlaybackRate() const
{
	return m_Param.m_fSpeed;
}

RageTimer
RageSound::GetStartTime() const
{
	return m_Param.m_StartTime;
}

void
RageSound::SetParams(const RageSoundParams& p)
{
	m_Param = p;
	ApplyParams();
}

void
RageSound::ApplyParams()
{
	if (m_pSource == nullptr)
		return;

	m_pSource->SetProperty("Pitch", m_Param.m_fPitch);
	m_pSource->SetProperty("Speed", m_Param.m_fSpeed);
	m_pSource->SetProperty("StartSecond", m_Param.m_StartSecond);
	m_pSource->SetProperty("LengthSeconds", m_Param.m_LengthSeconds);
	m_pSource->SetProperty("FadeInSeconds", m_Param.m_fFadeInSeconds);
	m_pSource->SetProperty("FadeSeconds", m_Param.m_fFadeOutSeconds);
	m_pSource->SetProperty("AccurateSync", m_Param.m_bAccurateSync);

	auto fVolume = m_Param.m_Volume;
	if (!m_Param.m_bIsCriticalSound)
		fVolume *= m_Param.m_fAttractVolume;
	m_pSource->SetProperty("Volume", fVolume);

	switch (GetStopMode()) {
		case RageSoundParams::M_LOOP:
			m_pSource->SetProperty("Loop", 1.0f);
			break;
		case RageSoundParams::M_STOP:
			m_pSource->SetProperty("Stop", 1.0f);
			break;
		case RageSoundParams::M_CONTINUE:
			m_pSource->SetProperty("Continue", 1.0f);
			break;
		default:
			break;
	}
}

bool
RageSound::SetProperty(const std::string& sProperty, float fValue)
{
	return m_pSource->SetProperty(sProperty, fValue);
}

RageSoundParams::StopMode_t
RageSound::GetStopMode() const
{
	if (m_Param.StopMode != RageSoundParams::M_AUTO)
		return m_Param.StopMode;

	if (m_sFilePath.find("loop") != std::string::npos)
		return RageSoundParams::M_LOOP;

	return RageSoundParams::M_STOP;
}

void
RageSound::SetStopModeFromString(const std::string& sStopMode)
{
	if (sStopMode.find("stop") != std::string::npos) {
		m_Param.StopMode = RageSoundParams::M_STOP;
	} else if (sStopMode.find("loop") != std::string::npos) {
		m_Param.StopMode = RageSoundParams::M_LOOP;
	} else if (sStopMode.find("continue") != std::string::npos) {
		m_Param.StopMode = RageSoundParams::M_CONTINUE;
	} else if (sStopMode.find("auto") != std::string::npos) {
		m_Param.StopMode = RageSoundParams::M_AUTO;
	} else {
		// error
	}
}

void
RageSound::SetPlayBackCallback(const std::shared_ptr<LuaReference>& f,
							   unsigned int bufSize)
{
	std::lock_guard<std::mutex> guard(recentSamplesMutex);

	soundPlayCallback = f;
	recentPCMSamplesBufferSize = std::max(bufSize, 1024u);
	recentPCMSamples.reserve(recentPCMSamplesBufferSize + 2);
	fftBuffer.resize(recentPCMSamplesBufferSize / 2 + 1, {});
	if (fftPlan)
		mufft_free_plan_1d(fftPlan);
	fftPlan =
	  mufft_create_plan_1d_r2c(recentPCMSamplesBufferSize, MUFFT_FLAG_CPU_ANY);
	if (!fftPlan)
		Locator::getLogger()->warn(
		  "Failed to set playback callback in mufft...");
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the RageSound. */
class LunaRageSound : public Luna<RageSound>
{
  public:
	static int get_length(T* p, lua_State* L)
	{
		auto reader = p->GetSoundReader();
		if (reader == nullptr) {
			lua_pushnumber(L, -1.0f);
		} else {
			lua_pushnumber(L, reader->GetLength() / 1000.0f);
		}
		return 1;
	}
	static int pitch(T* p, lua_State* L)
	{
		auto params(p->GetParams());
		params.m_fPitch = FArg(1);
		p->SetParams(params);
		COMMON_RETURN_SELF;
	}

	static int speed(T* p, lua_State* L)
	{
		auto params(p->GetParams());
		params.m_fSpeed = FArg(1);
		p->SetParams(params);
		COMMON_RETURN_SELF;
	}

	static int volume(T* p, lua_State* L)
	{
		auto params(p->GetParams());
		params.m_Volume = FArg(1);
		p->SetParams(params);
		COMMON_RETURN_SELF;
	}

	static int SetProperty(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->SetProperty(SArg(1), FArg(2)));
		return 1;
	}

	// Rename me and deprecate the above one? -DaisuMaster
	static int SetParam(T* p, lua_State* L)
	{
		auto params(p->GetParams());

		std::string val = SArg(1);
		if (val == "StartSecond")
			params.m_StartSecond = FArg(2);
		else if (val == "LengthSeconds")
			params.m_LengthSeconds = FArg(2);
		else if (val == "FadeInSeconds")
			params.m_fFadeInSeconds = FArg(2);
		else if (val == "FadeSeconds")
			params.m_fFadeOutSeconds = FArg(2);
		else if (val == "Pitch")
			params.m_fPitch = FArg(2);
		else if (val == "Speed")
			params.m_fSpeed = FArg(2);
		else if (val == "Volume")
			params.m_Volume = FArg(2);

		p->SetParams(params);
		COMMON_RETURN_SELF;
	}

	static int SetPlayBackCallback(T* p, lua_State* L)
	{
		if (lua_isnumber(L, 2))
			p->SetPlayBackCallback(
			  std::make_shared<LuaReference>(GetFuncArg(1, L)), IArg(2));
		else
			p->SetPlayBackCallback(
			  std::make_shared<LuaReference>(GetFuncArg(1, L)));
		COMMON_RETURN_SELF;
	}

	static int ClearPlayBackCallback(T* p, lua_State* L)
	{
		p->SetPlayBackCallback(std::make_shared<LuaReference>(LuaReference()));
		COMMON_RETURN_SELF;
	}

	static int GetSampleRate(T* p, lua_State* L)
	{
		auto s = p->GetSoundReader();
		lua_pushnumber(L, s != nullptr ? s->GetSampleRate() : 44100);
		return 1;
	}

	/*
	static int SetStopMode( T* p, lua_State *L )
	{
		LuaHelpers::Push( L, p->SetStopModeFromString(SArg(1)) );
		return 1;
	}
	*/

	LunaRageSound()
	{
		ADD_METHOD(GetSampleRate);
		ADD_METHOD(SetPlayBackCallback);
		ADD_METHOD(get_length);
		ADD_METHOD(pitch);
		ADD_METHOD(speed);
		ADD_METHOD(volume);
		ADD_METHOD(SetProperty);
		ADD_METHOD(SetParam);
		// ADD_METHOD( SetStopMode );
	}
};

LUA_REGISTER_CLASS(RageSound)
// lua end
