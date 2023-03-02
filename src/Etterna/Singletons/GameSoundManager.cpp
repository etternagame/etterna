#include "Etterna/Globals/global.h"
#include "AnnouncerManager.h"
#include "GameSoundManager.h"
#include "GameState.h"
#include "LuaManager.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSM.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSSC.h"
#include "PrefsManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Sound/RageSound.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Misc/TimingData.h"
#include "ScreenManager.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Globals/rngthing.h"
class SongOptions;

#include "arch/Sound/RageSoundDriver.h"

#include <algorithm>

GameSoundManager* SOUND = nullptr;

/*
 * When playing music, automatically search for an SM file for timing data.  If
 * one is found, automatically handle GAMESTATE->m_fSongBeat, etc.
 *
 * modf(GAMESTATE->m_fSongBeat) should always be continuously moving from 0
 * to 1.  To do this, wait before starting a sound until the fractional portion
 * of the beat will be the same.
 *
 * If PlayMusic(fLengthSeconds) is set, peek at the beat, and extend the length
 * so we'll be on the same fractional beat when we loop.
 */

/* Lock this before touching g_UpdatingTimer or g_Playing. */
static RageEvent* g_Mutex;
static bool g_UpdatingTimer;
static bool g_Shutdown;
static bool g_bFlushing = false;

enum FadeState
{
	FADE_NONE,
	FADE_OUT,
	FADE_WAIT,
	FADE_IN
};
static FadeState g_FadeState = FADE_NONE;
static float g_fDimVolume = 1.0f;
static float g_fOriginalVolume = 1.0f;
static float g_fDimDurationRemaining = 0.0f;
static bool g_bWasPlayingOnLastUpdate = false;

struct MusicPlaying
{
	bool m_bTimingDelayed;
	bool m_bHasTiming;
	bool m_bApplyMusicRate;
	// The timing data that we're currently using.
	TimingData m_Timing;

	/* If m_bTimingDelayed is true, this will be the timing data for the
	 * song that's starting. We'll copy it to m_Timing once sound is heard. */
	TimingData m_NewTiming;
	RageSound* m_Music;
	explicit MusicPlaying(RageSound* Music)
	{
		m_Timing.AddSegment(BPMSegment(0, 120));
		m_NewTiming.AddSegment(BPMSegment(0, 120));
		m_bHasTiming = false;
		m_bTimingDelayed = false;
		m_bApplyMusicRate = false;
		m_Music = Music;
	}

	~MusicPlaying() { delete m_Music; }
};

static MusicPlaying* g_Playing;

static RageThread MusicThread;

std::vector<std::string> g_SoundsToPlayOnce;
std::vector<std::string> g_SoundsToPlayOnceFromDir;
std::vector<std::string> g_SoundsToPlayOnceFromAnnouncer;

struct MusicToPlay
{
	std::string m_sFile, m_sTimingFile;
	bool HasTiming = false;
	TimingData m_TimingData;
	bool bForceLoop = false;
	float fStartSecond = 0, fLengthSeconds = 0, fFadeInLengthSeconds = 0,
		  fFadeOutLengthSeconds = 0;
	bool bAlignBeat = false, bApplyMusicRate = false, bAccurateSync = false;
	MusicToPlay() { HasTiming = false; }
};
std::vector<MusicToPlay> g_MusicsToPlay;
static GameSoundManager::PlayMusicParams g_FallbackMusicParams;

// A position to be set on a sound
struct SoundPositionSetter
{
	RageSound* m_psound;
	float fSeconds;
};
std::vector<SoundPositionSetter> g_PositionsToSet;

// A param to set on a sound
struct MusicParamSetter
{
	RageSound* m_psound;
	RageSoundParams p;
};
std::vector<MusicParamSetter> g_ParamsToSet;

void
GameSoundManager::StartMusic(MusicToPlay& ToPlay)
{
	LockMutex L(*g_Mutex);
	if (g_Playing->m_Music->IsPlaying() &&
		EqualsNoCase(g_Playing->m_Music->GetLoadedFilePath(), ToPlay.m_sFile) &&
		ToPlay.HasTiming)
		return;

	/* We're changing or stopping the music.  If we were dimming, reset. */
	g_FadeState = FADE_NONE;

	if (ToPlay.m_sFile.empty()) {
		/* StopPlaying() can take a while, so don't hold the lock while we stop
		 * the sound. Be sure to leave the rest of g_Playing in place. */
		RageSound* pOldSound = g_Playing->m_Music;
		g_Playing->m_Music = new RageSound;
		if (!soundPlayCallback->IsNil() && soundPlayCallback->IsSet())
			g_Playing->m_Music->SetPlayBackCallback(soundPlayCallback,
													recentPCMSamplesBufferSize);
		L.Unlock();

		delete pOldSound;
		return;
	}

	/* Unlock, load the sound here, and relock.  Loading may take a while if
	 * we're reading from CD and we have to seek far, which can throw off the
	 * timing below. */
	MusicPlaying* NewMusic;
	{
		g_Mutex->Unlock();
		auto* pSound = new RageSound;
		if (!soundPlayCallback->IsNil() && soundPlayCallback->IsSet()) {
			pSound->SetPlayBackCallback(soundPlayCallback,
										recentPCMSamplesBufferSize);
		}
		RageSoundLoadParams params;
		params.m_bSupportRateChanging = ToPlay.bApplyMusicRate;
		pSound->Load(ToPlay.m_sFile, false, &params);
		g_Mutex->Lock();

		NewMusic = new MusicPlaying(pSound);
	}

	NewMusic->m_Timing = g_Playing->m_Timing;

	/* See if we can find timing data, if it's not already loaded. */
	if (!ToPlay.HasTiming && IsAFile(ToPlay.m_sTimingFile)) {
		Locator::getLogger()->debug("Found '{}'", ToPlay.m_sTimingFile.c_str());
		Song song;
		SSCLoader loaderSSC;
		SMLoader loaderSM;
		if (GetExtension(ToPlay.m_sTimingFile) == ".ssc" &&
			loaderSSC.LoadFromSimfile(ToPlay.m_sTimingFile, song)) {
			ToPlay.HasTiming = true;
			ToPlay.m_TimingData = song.m_SongTiming;
		} else if (GetExtension(ToPlay.m_sTimingFile) == ".sm" &&
				   loaderSM.LoadFromSimfile(ToPlay.m_sTimingFile, song)) {
			ToPlay.HasTiming = true;
			ToPlay.m_TimingData = song.m_SongTiming;
		}
	}

	if (ToPlay.HasTiming) {
		NewMusic->m_NewTiming = ToPlay.m_TimingData;
	}

	if (ToPlay.bAlignBeat && ToPlay.HasTiming && ToPlay.bForceLoop &&
		ToPlay.fLengthSeconds != -1) {
		/* Extend the loop period so it always starts and ends on the same
		 * fractional beat.  That is, if it starts on beat 1.5, and ends on
		 * beat 10.2, extend it to end on beat 10.5.  This way, effects always
		 * loop cleanly. */
		float fStartBeat =
		  NewMusic->m_NewTiming.WhereUAtBroNoOffset(ToPlay.fStartSecond);
		float fEndSec = ToPlay.fStartSecond + ToPlay.fLengthSeconds;
		float fEndBeat = NewMusic->m_NewTiming.WhereUAtBroNoOffset(fEndSec);

		const float fStartBeatFraction = fmodfp(fStartBeat, 1);
		const float fEndBeatFraction = fmodfp(fEndBeat, 1);

		float fBeatDifference = fStartBeatFraction - fEndBeatFraction;
		if (fBeatDifference < 0)
			fBeatDifference += 1.0f; /* unwrap */

		fEndBeat += fBeatDifference;

		const float fRealEndSec =
		  NewMusic->m_NewTiming.WhereUAtBroNoOffset(fEndBeat);
		const float fNewLengthSec = fRealEndSec - ToPlay.fStartSecond;

		/* Extend fFadeOutLengthSeconds, so the added time is faded out. */
		ToPlay.fFadeOutLengthSeconds += fNewLengthSec - ToPlay.fLengthSeconds;
		ToPlay.fLengthSeconds = fNewLengthSec;
	}

	bool StartImmediately = false;
	if (!ToPlay.HasTiming) {
		/* This song has no real timing data.  The offset is arbitrary.  Change
		 * it so the beat will line up to where we are now, so we don't have to
		 * delay. */
		float fDestBeat = fmodfp(GAMESTATE->m_Position.m_fSongBeatNoOffset, 1);
		float fTime =
		  NewMusic->m_NewTiming.GetElapsedTimeFromBeatNoOffset(fDestBeat);

		NewMusic->m_NewTiming.m_fBeat0OffsetInSeconds = fTime;

		StartImmediately = true;
	}

	/* If we have an active timer, try to start on the next update.  Otherwise,
	 * start now. */
	if (!g_Playing->m_bHasTiming && !g_UpdatingTimer)
		StartImmediately = true;
	if (!ToPlay.bAlignBeat)
		StartImmediately = true;

	RageTimer when; /* zero */
	if (!StartImmediately) {
		/* GetPlayLatency returns the minimum time until a sound starts.  That's
		 * common when starting a precached sound, but our sound isn't, so it'll
		 * probably take a little longer.  Nudge the latency up. */
		const float fPresumedLatency = SOUNDMAN->GetPlayLatency() + 0.040f;
		const float fCurSecond =
		  GAMESTATE->m_Position.m_fMusicSeconds + fPresumedLatency;
		const float fCurBeat =
		  g_Playing->m_Timing.GetBeatFromElapsedTimeNoOffset(fCurSecond);

		/* The beat that the new sound will start on. */
		const float fStartBeat =
		  NewMusic->m_NewTiming.GetBeatFromElapsedTimeNoOffset(
			ToPlay.fStartSecond);
		const float fStartBeatFraction = fmodfp(fStartBeat, 1);

		float fCurBeatToStartOn = truncf(fCurBeat) + fStartBeatFraction;
		if (fCurBeatToStartOn < fCurBeat)
			fCurBeatToStartOn += 1.0f;

		const float fSecondToStartOn =
		  g_Playing->m_Timing.WhereUAtBroNoOffset(fCurBeatToStartOn);
		const float fMaximumDistance = 2;
		const float fDistance =
		  std::min(fSecondToStartOn - GAMESTATE->m_Position.m_fMusicSeconds,
				   fMaximumDistance);

		when = GAMESTATE->m_Position.m_LastBeatUpdate + fDistance;
	}

	/* Important: don't hold the mutex while we load and seek the actual sound.
	 */
	L.Unlock();
	{
		NewMusic->m_bHasTiming = ToPlay.HasTiming;
		if (ToPlay.HasTiming)
			NewMusic->m_NewTiming = ToPlay.m_TimingData;
		NewMusic->m_bTimingDelayed = true;
		NewMusic->m_bApplyMusicRate = ToPlay.bApplyMusicRate;
		//		NewMusic->m_Music->Load( ToPlay.m_sFile, false );

		RageSoundParams p;
		p.m_StartSecond = ToPlay.fStartSecond;
		p.m_LengthSeconds = ToPlay.fLengthSeconds;
		p.m_fFadeInSeconds = ToPlay.fFadeInLengthSeconds;
		p.m_fFadeOutSeconds = ToPlay.fFadeOutLengthSeconds;
		p.m_StartTime = when;
		p.m_bAccurateSync = ToPlay.bAccurateSync;
		if (ToPlay.bForceLoop)
			p.StopMode = RageSoundParams::M_LOOP;
		NewMusic->m_Music->SetParams(p);
		NewMusic->m_Music->StartPlaying();
	}

	LockMut(*g_Mutex);
	delete g_Playing;
	g_Playing = NewMusic;
}
void
GameSoundManager::DoPlayOnce(std::string sPath)
{
	/* We want this to start quickly, so don't try to prebuffer it. */
	auto* pSound = new RageSound;
	if (!soundPlayCallback->IsNil() && soundPlayCallback->IsSet())
		pSound->SetPlayBackCallback(soundPlayCallback,
									recentPCMSamplesBufferSize);
	pSound->Load(sPath, false);

	pSound->Play(false);
	pSound->DeleteSelfWhenFinishedPlaying();
}

void
GameSoundManager::DoPlayOnceFromDir(std::string sPath)
{
	if (sPath == "")
		return;

	// make sure there's a slash at the end of this path
	ensure_slash_at_end((sPath));

	std::vector<std::string> arraySoundFiles;
	FILEMAN->GetDirListing(sPath + "*.mp3", arraySoundFiles, ONLY_FILE);
	FILEMAN->GetDirListing(sPath + "*.wav", arraySoundFiles, ONLY_FILE);
	FILEMAN->GetDirListing(sPath + "*.ogg", arraySoundFiles, ONLY_FILE);
	FILEMAN->GetDirListing(sPath + "*.oga", arraySoundFiles, ONLY_FILE);

	if (arraySoundFiles.empty())
		return;

	int index = RandomInt(arraySoundFiles.size());
	DoPlayOnce(sPath + arraySoundFiles[index]);
}

bool
GameSoundManager::SoundWaiting()
{
	return !g_SoundsToPlayOnce.empty() || !g_SoundsToPlayOnceFromDir.empty() ||
		   !g_SoundsToPlayOnceFromAnnouncer.empty() ||
		   !g_MusicsToPlay.empty() || !g_PositionsToSet.empty() ||
		   !g_ParamsToSet.empty();
}

void
GameSoundManager::StartQueuedSounds()
{
	g_Mutex->Lock();
	std::vector<std::string> aSoundsToPlayOnce = g_SoundsToPlayOnce;
	g_SoundsToPlayOnce.clear();
	std::vector<std::string> aSoundsToPlayOnceFromDir = g_SoundsToPlayOnceFromDir;
	g_SoundsToPlayOnceFromDir.clear();
	std::vector<std::string> aSoundsToPlayOnceFromAnnouncer =
	  g_SoundsToPlayOnceFromAnnouncer;
	g_SoundsToPlayOnceFromAnnouncer.clear();
	std::vector<MusicToPlay> aMusicsToPlay = g_MusicsToPlay;
	g_MusicsToPlay.clear();
	g_Mutex->Unlock();

	for (unsigned i = 0; i < aSoundsToPlayOnce.size(); ++i)
		if (aSoundsToPlayOnce[i] != "")
			DoPlayOnce(aSoundsToPlayOnce[i]);

	for (unsigned i = 0; i < aSoundsToPlayOnceFromDir.size(); ++i)
		DoPlayOnceFromDir(aSoundsToPlayOnceFromDir[i]);

	for (unsigned i = 0; i < aSoundsToPlayOnceFromAnnouncer.size(); ++i) {
		std::string sPath = aSoundsToPlayOnceFromAnnouncer[i];
		if (sPath != "") {
			sPath = ANNOUNCER->GetPathTo(sPath);
			DoPlayOnceFromDir(sPath);
		}
	}

	for (unsigned i = 0; i < aMusicsToPlay.size(); ++i) {
		/* Don't bother starting this music if there's another one in the queue
		 * after it. */
		/* Actually, it's a little trickier: the editor gives us a stop and then
		 * a sound in quick succession; if we ignore the stop, we won't rewind
		 * the sound if it was already playing.  We don't want to waste time
		 * loading a sound if it's going to be replaced immediately, though.
		 * So, if we have more music in the queue, then forcibly stop the
		 * current sound. */
		if (i + 1 == aMusicsToPlay.size())
			StartMusic(aMusicsToPlay[i]);
		else {
			Locator::getLogger()->trace("Removing old sound at index {}", i);
			/* StopPlaying() can take a while, so don't hold the lock while we
			 * stop the sound. */
			g_Mutex->Lock();
			RageSound* pOldSound = g_Playing->m_Music;
			g_Playing->m_Music = new RageSound;
			g_Mutex->Unlock();
			if (!soundPlayCallback->IsNil() && soundPlayCallback->IsSet())
				g_Playing->m_Music->SetPlayBackCallback(
				  soundPlayCallback, recentPCMSamplesBufferSize);

			delete pOldSound;
		}
	}
}

void
GameSoundManager::HandleSetPosition()
{
	g_Mutex->Lock();
	std::vector<SoundPositionSetter> vec = g_PositionsToSet;
	g_PositionsToSet.clear();
	g_Mutex->Unlock();
	for (unsigned i = 0; i < vec.size(); i++) {
		// I wonder if this can crash when sounds get deleted
		// only one way to find out - checkpoint and see if someone crashes :)
		Locator::getLogger()->trace("Setting position for sound.");
		vec[i].m_psound->SetPositionSeconds(vec[i].fSeconds);
	}
}

void
GameSoundManager::HandleSetParams()
{
	g_Mutex->Lock();
	std::vector<MusicParamSetter> vec = g_ParamsToSet;
	g_ParamsToSet.clear();
	g_Mutex->Unlock();
	for (unsigned i = 0; i < vec.size(); i++) {
		Locator::getLogger()->trace("Setting params for sound.");
		// vec[i].m_psound->SetParams(vec[i].p);
		g_Playing->m_Music->SetParams(vec[i].p);
	}
}

void
GameSoundManager::Flush()
{
	g_Mutex->Lock();
	g_bFlushing = true;

	g_Mutex->Broadcast();

	while (g_bFlushing)
		g_Mutex->Wait();
	g_Mutex->Unlock();
}

int
MusicThread_start(void* p)
{
	auto soundman = static_cast<GameSoundManager*>(p);
	while (!g_Shutdown) {
		g_Mutex->Lock();
		while (!soundman->SoundWaiting() && !g_Shutdown && !g_bFlushing)
			g_Mutex->Wait();
		g_Mutex->Unlock();

		/* This is a little hack: we want to make sure that we go through
		 * StartQueuedSounds after Flush() is called, to be sure we're flushed,
		 * so check g_bFlushing before calling.  This won't work if more than
		 * one thread might call Flush(), but only the main thread is allowed
		 * to make SOUND calls. */
		bool bFlushing = g_bFlushing;

		soundman->StartQueuedSounds();

		soundman->HandleSetParams();
		soundman->HandleSetPosition();

		if (bFlushing) {
			g_Mutex->Lock();
			g_Mutex->Signal();
			g_bFlushing = false;
			g_Mutex->Unlock();
		}
	}

	return 0;
}

GameSoundManager::GameSoundManager()
{
	/* Init RageSoundMan first: */
	ASSERT(SOUNDMAN != NULL);

	g_Mutex = new RageEvent("GameSoundManager");
	g_Playing = new MusicPlaying(new RageSound);
	soundPlayCallback = std::make_shared<LuaReference>(LuaReference());
	if (!soundPlayCallback->IsNil() && soundPlayCallback->IsSet())
		g_Playing->m_Music->SetPlayBackCallback(soundPlayCallback,
												recentPCMSamplesBufferSize);

	g_UpdatingTimer = true;

	g_Shutdown = false;
	MusicThread.SetName("Music thread");
	MusicThread.Create(MusicThread_start, this);

	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "SOUND");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
	SubscribeToMessage(Message_ScreenChanged);
}

GameSoundManager::~GameSoundManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("SOUND");

	/* Signal the mixing thread to quit. */
	Locator::getLogger()->info("Shutting down music start thread ...");
	g_Mutex->Lock();
	g_Shutdown = true;
	g_Mutex->Broadcast();
	g_Mutex->Unlock();
	MusicThread.Wait();
	Locator::getLogger()->info("Music start thread shut down.");

	SAFE_DELETE(g_Playing);
	SAFE_DELETE(g_Mutex);
}

void
GameSoundManager::Update(float fDeltaTime)
{
	{
		g_Mutex->Lock();
		if (g_Playing->m_bApplyMusicRate) {
			RageSoundParams p = g_Playing->m_Music->GetParams();
			float fRate = GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate;
			if (p.m_fSpeed != fRate) {
				p.m_fSpeed = fRate;
				g_Playing->m_Music->SetParams(p);
			}
		}

		if (g_Playing->m_Music->pendingPlayBackCall) {
			g_Mutex->Unlock();
			auto L = LUA->Get();
			g_Playing->m_Music->ExecutePlayBackCallback(L);
			LUA->Release(L);
			g_Mutex->Lock();
		}

		bool bIsPlaying = g_Playing->m_Music->IsPlaying();
		g_Mutex->Unlock();
		if (!bIsPlaying && g_bWasPlayingOnLastUpdate &&
			!g_FallbackMusicParams.sFile.empty()) {
			PlayMusic(g_FallbackMusicParams);

			g_FallbackMusicParams.sFile = "";
		}
		g_bWasPlayingOnLastUpdate = bIsPlaying;
	}

	LockMut(*g_Mutex);

	{
		/* Duration of the fade-in and fade-out: */
		// const float fFadeInSpeed = 1.5f;
		// const float fFadeOutSpeed = 0.3f;
		float fFadeInSpeed = g_Playing->m_Music->GetParams().m_fFadeInSeconds;
		float fFadeOutSpeed = g_Playing->m_Music->GetParams().m_fFadeOutSeconds;
		float fVolume = g_Playing->m_Music->GetParams().m_Volume;
		switch (g_FadeState) {
			case FADE_NONE:
				break;
			case FADE_OUT:
				fapproach(fVolume, g_fDimVolume, fDeltaTime / fFadeOutSpeed);
				if (fabsf(fVolume - g_fDimVolume) < 0.001f)
					g_FadeState = FADE_WAIT;
				break;
			case FADE_WAIT:
				g_fDimDurationRemaining -= fDeltaTime;
				if (g_fDimDurationRemaining <= 0)
					g_FadeState = FADE_IN;
				break;
			case FADE_IN:
				fapproach(
				  fVolume, g_fOriginalVolume, fDeltaTime / fFadeInSpeed);
				if (fabsf(fVolume - g_fOriginalVolume) < 0.001f)
					g_FadeState = FADE_NONE;
				break;
		}

		RageSoundParams p = g_Playing->m_Music->GetParams();
		if (p.m_Volume != fVolume) {
			p.m_Volume = fVolume;
			g_Playing->m_Music->SetParams(p);
		}
	}

	if (!g_UpdatingTimer)
		return;

	const float fRate = g_Playing->m_Music->GetPlaybackRate();
	if (!g_Playing->m_Music->IsPlaying()) {
		/* There's no song playing.  Fake it. */
		GAMESTATE->UpdateSongPosition(GAMESTATE->m_Position.m_fMusicSeconds
									 + fDeltaTime * fRate,
									  g_Playing->m_Timing);
		return;
	}

	/* There's a delay between us calling Play() and the sound actually playing.
	 * During this time, m_bApproximate will be true.  Keep using the previous
	 * timing data until we get a non-approximate time, indicating that the
	 * sound has actually started playing. */
	bool m_bApproximate;
	RageTimer tm = RageZeroTimer;
	const float fSeconds =
	  g_Playing->m_Music->GetPositionSeconds(&m_bApproximate, &tm);

	// Check for song timing skips.
	if (PREFSMAN->m_bLogSkips && !g_Playing->m_bTimingDelayed) {
		const float fExpectedTimePassed =
		  (tm - GAMESTATE->m_Position.m_LastBeatUpdate) * fRate;
		const float fSoundTimePassed =
		  fSeconds - GAMESTATE->m_Position.m_fMusicSeconds;
		const float fDiff = fExpectedTimePassed - fSoundTimePassed;

		static std::string sLastFile = "";
		const std::string ThisFile = g_Playing->m_Music->GetLoadedFilePath();

		/* If fSoundTimePassed < 0, the sound has probably looped. */
		if (sLastFile == ThisFile && fSoundTimePassed >= 0 &&
			fabsf(fDiff) > 0.003f)
			Locator::getLogger()->trace("Song position skip in {}: expected {:.3f}, got {:.3f} (cur "
					   "{}, prev {}) ({:.3f} difference)",
					   Basename(ThisFile).c_str(), fExpectedTimePassed, fSoundTimePassed,
					   fSeconds, GAMESTATE->m_Position.m_fMusicSeconds, fDiff);
		sLastFile = ThisFile;
	}

	// If g_Playing->m_bTimingDelayed, we're waiting for the new music to
	// actually start playing.
	if (g_Playing->m_bTimingDelayed && !m_bApproximate) {
		/* Load up the new timing data. */
		g_Playing->m_Timing = g_Playing->m_NewTiming;
		g_Playing->m_bTimingDelayed = false;
	}

	if (g_Playing->m_bTimingDelayed) {
		/* We're still waiting for the new sound to start playing, so keep using
		 * the old timing data and fake the time. */
		GAMESTATE->UpdateSongPosition(GAMESTATE->m_Position.m_fMusicSeconds
									 + fDeltaTime * fRate,
									  g_Playing->m_Timing);
	} else {
		GAMESTATE->UpdateSongPosition(fSeconds, g_Playing->m_Timing);
	}

	// Send crossed messages
	if (GAMESTATE->m_pCurSong) {
		static int iBeatLastCrossed = 0;

		float fSongBeat = GAMESTATE->m_Position.m_fSongBeat;

		int iRowNow = BeatToNoteRow(fSongBeat);
		iRowNow = std::max(0, iRowNow);

		int iBeatNow = iRowNow / ROWS_PER_BEAT;

		for (int iBeat = iBeatLastCrossed + 1; iBeat <= iBeatNow; ++iBeat) {
			Message msg("CrossedBeat");
			msg.SetParam("Beat", iBeat);
			MESSAGEMAN->Broadcast(msg);
		}

		iBeatLastCrossed = iBeatNow;
	}
}

std::string
GameSoundManager::GetMusicPath() const
{
	LockMut(*g_Mutex);
	return g_Playing->m_Music->GetLoadedFilePath();
}

void
GameSoundManager::WithRageSoundPlaying(std::function<void(RageSound*)> f)
{
	// LockMut(*g_Mutex); // commented this to hack around something else
	f(g_Playing->m_Music);
}

TimingData
GameSoundManager::GetPlayingMusicTiming()
{
	LockMut(*g_Mutex);
	return g_Playing->m_Timing;
}

void
GameSoundManager::SetSoundPosition(RageSound* s, float fSeconds)
{
	SoundPositionSetter snd;
	snd.fSeconds = fSeconds;
	snd.m_psound = s;
	g_Mutex->Lock();
	g_PositionsToSet.push_back(snd);
	g_Mutex->Broadcast();
	g_Mutex->Unlock();
}

void
GameSoundManager::SetPlayingMusicParams(RageSoundParams p)
{
	// This will replace the params for the music pointer
	// So needs to be first filled out by the existing params
	// (preferably, unless the intention is to overwrite them)
	MusicParamSetter prm;
	// prm.m_psound = g_Playing->m_Music;
	prm.p = p;
	g_Mutex->Lock();
	g_ParamsToSet.push_back(prm);
	g_Mutex->Broadcast();
	g_Mutex->Unlock();
}

const RageSoundParams&
GameSoundManager::GetPlayingMusicParams()
{
	return g_Playing->m_Music->GetParams();
}

void
GameSoundManager::ResyncMusicPlaying()
{
	// This function is primarily just useful for hacking MP3 sync back to normal since moving sound backwards force it to completely resync at the moment
	auto* rs = g_Playing->m_Music;
	auto now = rs->GetPositionSeconds();
	SetSoundPosition(rs, now - 0.01F);
}

void
GameSoundManager::PlayMusic(const std::string& sFile,
							const TimingData* pTiming,
							bool bForceLoop,
							float fStartSecond,
							float fLengthSeconds,
							float fFadeInLengthSeconds,
							float fFadeOutLengthSeconds,
							bool bAlignBeat,
							bool bApplyMusicRate,
							bool bAccurateSync)
{
	PlayMusicParams params;
	params.sFile = sFile;
	params.pTiming = pTiming;
	params.bForceLoop = bForceLoop;
	params.fStartSecond = fStartSecond;
	params.fLengthSeconds = fLengthSeconds;
	params.fFadeInLengthSeconds = fFadeInLengthSeconds;
	params.fFadeOutLengthSeconds = fFadeOutLengthSeconds;
	params.bAlignBeat = bAlignBeat;
	params.bApplyMusicRate = bApplyMusicRate;
	PlayMusic(params);
}

void
GameSoundManager::PlayMusic(PlayMusicParams params,
							PlayMusicParams FallbackMusicParams)
{
	g_FallbackMusicParams = FallbackMusicParams;

	//	LOG->Trace("play '%s' (current '%s')", file.c_str(),
	// g_Playing->m_Music->GetLoadedFilePath().c_str());

	MusicToPlay ToPlay;

	ToPlay.m_sFile = params.sFile;
	if (params.pTiming != nullptr) {
		ToPlay.HasTiming = true;
		ToPlay.m_TimingData = *params.pTiming;
	} else {
		/* If no timing data was provided, look for it in the same place as the
		 * music file. */
		// todo: allow loading .ssc files as well -aj
		ToPlay.m_sTimingFile = SetExtension(params.sFile, "sm");
	}

	ToPlay.bForceLoop = params.bForceLoop;
	ToPlay.fStartSecond = params.fStartSecond;
	ToPlay.fLengthSeconds = params.fLengthSeconds;
	ToPlay.fFadeInLengthSeconds = params.fFadeInLengthSeconds;
	ToPlay.fFadeOutLengthSeconds = params.fFadeOutLengthSeconds;
	ToPlay.bAlignBeat = params.bAlignBeat;
	ToPlay.bApplyMusicRate = params.bApplyMusicRate;
	ToPlay.bAccurateSync = params.bAccurateSync;

	/* Add the MusicToPlay to the g_MusicsToPlay queue. */
	g_Mutex->Lock();
	g_MusicsToPlay.push_back(ToPlay);
	g_Mutex->Broadcast();
	g_Mutex->Unlock();
}

void
GameSoundManager::DimMusic(float fVolume, float fDurationSeconds)
{
	LockMut(*g_Mutex);

	if (g_FadeState == FADE_NONE)
		g_fOriginalVolume = g_Playing->m_Music->GetParams().m_Volume;
	// otherwise, g_fOriginalVolume is already set and m_Volume will be the
	// current state, not the original state

	g_fDimDurationRemaining = fDurationSeconds;
	g_fDimVolume = fVolume;
	g_FadeState = FADE_OUT;
}

void
GameSoundManager::HandleSongTimer(bool on)
{
	LockMut(*g_Mutex);
	g_UpdatingTimer = on;
}

void
GameSoundManager::PlayOnce(const std::string& sPath)
{
	/* Add the sound to the g_SoundsToPlayOnce queue. */
	g_Mutex->Lock();
	g_SoundsToPlayOnce.push_back(sPath);
	g_Mutex->Broadcast();
	g_Mutex->Unlock();
}

void
GameSoundManager::PlayOnceFromDir(const std::string& sPath)
{
	/* Add the path to the g_SoundsToPlayOnceFromDir queue. */
	g_Mutex->Lock();
	g_SoundsToPlayOnceFromDir.push_back(sPath);
	g_Mutex->Broadcast();
	g_Mutex->Unlock();
}

void
GameSoundManager::PlayOnceFromAnnouncer(const std::string& sPath)
{
	/* Add the path to the g_SoundsToPlayOnceFromAnnouncer queue. */
	g_Mutex->Lock();
	g_SoundsToPlayOnceFromAnnouncer.push_back(sPath);
	g_Mutex->Broadcast();
	g_Mutex->Unlock();
}

float
GameSoundManager::GetPlayerBalance(PlayerNumber pn)
{
	/* If two players are active, play sounds on each players' side. */
	if (GAMESTATE->GetNumPlayersEnabled() == 2)
		return (pn == PLAYER_1) ? -1.0f : 1.0f;

	return 0;
}

void
GameSoundManager::HandleMessage(const Message& msg)
{
	if (msg.GetName() == "ScreenChanged" &&
		callbackOwningScreen != SCREENMAN->GetTopScreen()) {
		soundPlayCallback = std::make_shared<LuaReference>(LuaReference());
		g_Mutex->Lock();
		g_Playing->m_Music->SetPlayBackCallback(soundPlayCallback,
												recentPCMSamplesBufferSize);
		g_Mutex->Unlock();
	}
}

#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the GameSoundManager. */
class LunaGameSoundManager : public Luna<GameSoundManager>
{
  public:
	static int DimMusic(T* p, lua_State* L)
	{
		float fVolume = FArg(1);
		float fDurationSeconds = FArg(2);
		p->DimMusic(fVolume, fDurationSeconds);
		COMMON_RETURN_SELF;
	}
	static int SetVolume(T* p, lua_State* L)
	{
		Preference<float>* pRet =
		  Preference<float>::GetPreferenceByName("SoundVolume");
		float fVol = FArg(1);
		CLAMP(fVol, 0.0f, 1.0f);
		pRet->Set(fVol);
		SOUNDMAN->SetMixVolume();
		return 0;
	}
	static int PlayOnce(T* p, lua_State* L)
	{
		std::string sPath = SArg(1);
		if (lua_toboolean(L, 2) && PREFSMAN->m_MuteActions) {
			COMMON_RETURN_SELF;
		}
		p->PlayOnce(sPath);
		COMMON_RETURN_SELF;
	}
	static int PlayAnnouncer(T* p, lua_State* L)
	{
		std::string sPath = SArg(1);
		p->PlayOnceFromAnnouncer(sPath);
		COMMON_RETURN_SELF;
	}
	static int GetPlayerBalance(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;
		lua_pushnumber(L, p->GetPlayerBalance(pn));
		return 1;
	}
	static int PlayMusicPart(T* p, lua_State* L)
	{
		std::string musicPath = SArg(1);
		float musicStart = FArg(2);
		float musicLength = FArg(3);
		float fadeIn = 0;
		float fadeOut = 0;
		bool loop = false;
		bool applyRate = false;
		bool alignBeat = true;
		if (!lua_isnoneornil(L, 4)) {
			fadeIn = FArg(4);
		}
		if (!lua_isnoneornil(L, 5)) {
			fadeOut = FArg(5);
		}
		if (!lua_isnoneornil(L, 6)) {
			loop = BArg(6);
		}
		if (!lua_isnoneornil(L, 7)) {
			applyRate = BArg(7);
		}
		if (!lua_isnoneornil(L, 8)) {
			alignBeat = BArg(8);
		}
		p->PlayMusic(musicPath,
					 nullptr,
					 loop,
					 musicStart,
					 musicLength,
					 fadeIn,
					 fadeOut,
					 alignBeat,
					 applyRate);
		COMMON_RETURN_SELF;
	}

	static int StopMusic(T* p, lua_State* L)
	{
		p->StopMusic();
		COMMON_RETURN_SELF;
	}
	static int IsTimingDelayed(T* p, lua_State* L)
	{
		lua_pushboolean(L, static_cast<int>(g_Playing->m_bTimingDelayed));
		return 1;
	}
	static int SetPlayBackCallback(T* p, lua_State* L)
	{
		p->callbackOwningScreen = SCREENMAN->GetTopScreen();
		p->soundPlayCallback = std::make_shared<LuaReference>(GetFuncArg(1, L));
		if (lua_isnumber(L, 2))
			p->recentPCMSamplesBufferSize =
			  std::max(static_cast<unsigned int>(IArg(2)), 512u);
		g_Mutex->Lock();
		g_Playing->m_Music->SetPlayBackCallback(p->soundPlayCallback,
												p->recentPCMSamplesBufferSize);
		g_Mutex->Unlock();
		COMMON_RETURN_SELF;
	}
	static int ClearPlayBackCallback(T* p, lua_State* L)
	{
		p->callbackOwningScreen = nullptr;
		p->soundPlayCallback->Unset();
		g_Mutex->Lock();
		g_Playing->m_Music->SetPlayBackCallback(p->soundPlayCallback,
												p->recentPCMSamplesBufferSize);
		g_Mutex->Unlock();
		COMMON_RETURN_SELF;
	}
	static int ResyncMusicPlaying(T* p, lua_State* L)
	{
		p->ResyncMusicPlaying();
		COMMON_RETURN_SELF;
	}

	LunaGameSoundManager()
	{
		ADD_METHOD(SetPlayBackCallback);
		ADD_METHOD(ClearPlayBackCallback);
		ADD_METHOD(DimMusic);
		ADD_METHOD(PlayOnce);
		ADD_METHOD(PlayAnnouncer);
		ADD_METHOD(GetPlayerBalance);
		ADD_METHOD(PlayMusicPart);
		ADD_METHOD(StopMusic);
		ADD_METHOD(IsTimingDelayed);
		ADD_METHOD(SetVolume);
		ADD_METHOD(ResyncMusicPlaying);
	}
};

LUA_REGISTER_CLASS(GameSoundManager);

int
LuaFunc_get_sound_driver_list(lua_State* L);
int
LuaFunc_get_sound_driver_list(lua_State* L)
{
	std::vector<std::string> driver_names;
	split(
	  RageSoundDriver::GetDefaultSoundDriverList(), ",", driver_names, true);
	lua_createtable(L, driver_names.size(), 0);
	for (size_t n = 0; n < driver_names.size(); ++n) {
		lua_pushstring(L, driver_names[n].c_str());
		lua_rawseti(L, -2, n + 1);
	}
	return 1;
}
LUAFUNC_REGISTER_COMMON(get_sound_driver_list);
