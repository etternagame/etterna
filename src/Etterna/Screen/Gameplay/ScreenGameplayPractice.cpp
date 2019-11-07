#include "Etterna/Globals/global.h"
#include "ScreenGameplayPractice.h"
#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Actor/Gameplay/ArrowEffects.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Models/Misc/HighScore.h"
#include "Etterna/Models/Misc/PlayerAI.h"
#include "Etterna/Models/Misc/PlayerInfo.h"
#include "Etterna/Models/Misc/PlayerStageStats.h"
#include "Etterna/Models/NoteData/NoteDataWithScoring.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Actor/Gameplay/PlayerPractice.h"
#include "Etterna/Models/Misc/RadarValues.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "RageUtil/Misc/RageInput.h"
#include "Etterna/Singletons/SongManager.h"

#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/LuaManager.h"

REGISTER_SCREEN_CLASS(ScreenGameplayPractice);

void
ScreenGameplayPractice::FillPlayerInfo(PlayerInfo* playerInfoOut)
{
	playerInfoOut->Load(PLAYER_1,
						MultiPlayer_Invalid,
						true,
						Difficulty_Invalid,
						GameplayMode_Practice);
}

ScreenGameplayPractice::ScreenGameplayPractice()
{
	// covered by base class constructor
}

void
ScreenGameplayPractice::Init()
{
	ScreenGameplay::Init();
}

ScreenGameplayPractice::~ScreenGameplayPractice()
{
	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("ScreenGameplayReplay::~ScreenGameplayReplay()");
}

bool
ScreenGameplayPractice::Input(const InputEventPlus& input)
{
	// override default input here so we can reload the song
	// ... i wonder if this is doable.
	// haha dont try to break it please
	if (!IsTransitioning()) {
		bool bHoldingCtrl =
		  INPUTFILTER->IsBeingPressed(
			DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL)) ||
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL));
		bool bHoldingShift =
		  INPUTFILTER->IsBeingPressed(
			DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)) ||
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT));

		wchar_t c = INPUTMAN->DeviceInputToChar(input.DeviceI, false);
		MakeUpper(&c, 1);

		// if you press ctrl+shift+R, you reload the song like in Music Select
		// the catch is that this can break a lot of things probably
		// so be careful with it

		// initial implementation below, might just end up restarting the song
		// and reloading there instead
		if (bHoldingCtrl && bHoldingShift && c == 'R') {
			Song* cursong = GAMESTATE->m_pCurSong;
			vector<Steps*> chartsForThisSong = cursong->GetAllSteps();
			vector<std::string> oldKeys;
			for (auto k : chartsForThisSong)
				oldKeys.emplace_back(k->GetChartKey());

			cursong->ReloadFromSongDir();
			SONGMAN->ReconcileChartKeysForReloadedSong(cursong, oldKeys);

			SetupNoteDataFromRow(GAMESTATE->m_pCurSteps);

			float fSecondsToStartFadingOutMusic,
			  fSecondsToStartTransitioningOut;
			GetMusicEndTiming(fSecondsToStartFadingOutMusic,
							  fSecondsToStartTransitioningOut);
			RageSoundParams p = m_pSoundMusic->GetParams();
			float basicSongStart =
			  GAMESTATE->m_pCurSong->GetFirstSecond() * p.m_fSpeed;

			// not using a loop region so just set the new sound params
			if (loopEnd == loopStart) {
				p.m_StartSecond = basicSongStart;
				p.m_LengthSeconds =
				  fSecondsToStartTransitioningOut - p.m_StartSecond;
			} else {
				// using a loop region, check to see if either end is outside
				// the new song length
				if (loopStart < basicSongStart) {
					loopStart = basicSongStart;
					p.m_StartSecond = basicSongStart;
				}
				if (loopEnd > fSecondsToStartTransitioningOut) {
					loopEnd = fSecondsToStartTransitioningOut;
					p.m_LengthSeconds =
					  fSecondsToStartTransitioningOut - p.m_StartSecond;
				}
			}
			RageTimer tm;
			const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);
			if (fSeconds > fSecondsToStartTransitioningOut ||
				fSeconds < p.m_StartSecond) {
				// i want to make sure things are done in a very particular
				// order
				m_pSoundMusic->SetParams(p);
				SOUND->SetSoundPosition(m_pSoundMusic, p.m_StartSecond);
			} else {
				m_pSoundMusic->SetParams(p);
			}

			// let the theme know we just changed so many things internally
			MESSAGEMAN->Broadcast("PracticeModeReload");
			return true;
		}
	}

	return ScreenGameplay::Input(input);
}

void
ScreenGameplayPractice::Update(float fDeltaTime)
{
	if (GAMESTATE->m_pCurSong == NULL) {
		Screen::Update(fDeltaTime);
		return;
	}

	UpdateSongPosition(fDeltaTime);

	if (m_bZeroDeltaOnNextUpdate) {
		Screen::Update(0);
		m_bZeroDeltaOnNextUpdate = false;
	} else {
		Screen::Update(fDeltaTime);
	}

	if (SCREENMAN->GetTopScreen() != this)
		return;

	m_AutoKeysounds.Update(fDeltaTime);

	m_vPlayerInfo.m_SoundEffectControl.Update(fDeltaTime);

	{
		float fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		RageSoundParams p = m_pSoundMusic->GetParams();
		if (std::fabs(p.m_fSpeed - fSpeed) > 0.01f && fSpeed >= 0.0f) {
			p.m_fSpeed = fSpeed;
			m_pSoundMusic->SetParams(p);
		}
	}

	// If we are using a loop region, check if the music looped
	// If it did, reset the notedata.
	if (!m_Out.IsTransitioning() && loopStart != loopEnd &&
		GAMESTATE->m_Position.m_fMusicSeconds + 0.1f < lastReportedSeconds) {
		if (!m_GiveUpTimer.IsZero())
			return;

		auto td = GAMESTATE->m_pCurSteps->GetTimingData();
		const float startBeat = td->GetBeatFromElapsedTime(loopStart);
		const float endBeat = td->GetBeatFromElapsedTime(loopEnd);

		const int rowStart = BeatToNoteRow(startBeat);
		const int rowEnd = BeatToNoteRow(endBeat);

		if (rowStart < rowEnd)
			SetupNoteDataFromRow(GAMESTATE->m_pCurSteps, rowStart, rowEnd);
		if (PREFSMAN->m_bEasterEggs)
			m_Toasty.Reset();

		// Reset the wife/judge counter related visible stuff
		PlayerPractice* pl =
		  static_cast<PlayerPractice*>(m_vPlayerInfo.m_pPlayer);
		pl->PositionReset();
	}
	lastReportedSeconds = GAMESTATE->m_Position.m_fMusicSeconds;

	switch (m_DancingState) {
		case STATE_DANCING: {

			// Update living players' alive time
			// HACK: Don't scale alive time when using tab/tilde.  Instead of
			// accumulating time from a timer, this time should instead be tied
			// to the music position.
			float fUnscaledDeltaTime = m_timerGameplaySeconds.GetDeltaTime();
			m_vPlayerInfo.GetPlayerStageStats()->m_fAliveSeconds +=
			  fUnscaledDeltaTime *
			  GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

			// update fGameplaySeconds
			STATSMAN->m_CurStageStats.m_fGameplaySeconds += fUnscaledDeltaTime;
			float curBeat = GAMESTATE->m_Position.m_fSongBeat;
			Song& s = *GAMESTATE->m_pCurSong;

			if (curBeat >= s.GetFirstBeat() && curBeat < s.GetLastBeat()) {
				STATSMAN->m_CurStageStats.m_fStepsSeconds += fUnscaledDeltaTime;
			}

			// Handle the "give up" timer
			// except hard code it to fire after 1 second
			// instead of checking metrics
			// because we want to exit fast on demand
			bool bGiveUpTimerFired = false;
			bGiveUpTimerFired =
			  !m_GiveUpTimer.IsZero() && m_GiveUpTimer.Ago() > 1.f;
			m_gave_up = bGiveUpTimerFired;

			if (bGiveUpTimerFired) {
				m_vPlayerInfo.GetPlayerStageStats()->gaveuplikeadumbass = true;
				m_vPlayerInfo.GetPlayerStageStats()->m_bDisqualified = true;
				LOG->Trace("Exited Practice Mode to Evaluation");
				this->PostScreenMessage(SM_LeaveGameplay, 0);
			}
		}
		default:
			break;
	}

	PlayTicks();
	SendCrossedMessages();

	// ArrowEffects::Update call moved because having it happen once per
	// NoteField (which means twice in two player) seemed wasteful. -Kyz
	ArrowEffects::Update();
}

void
ScreenGameplayPractice::SetupNoteDataFromRow(Steps* pSteps,
											 int minRow,
											 int maxRow)
{
	NoteData originalNoteData;
	pSteps->GetNoteData(originalNoteData);

	const Style* pStyle = GAMESTATE->GetCurrentStyle(m_vPlayerInfo.m_pn);
	NoteData ndTransformed;
	pStyle->GetTransformedNoteDataForStyle(
	  m_vPlayerInfo.GetStepsAndTrailIndex(), originalNoteData, ndTransformed);

	m_vPlayerInfo.GetPlayerState()->Update(0);

	NoteDataUtil::RemoveAllButRange(ndTransformed, minRow, maxRow);

	// load player
	{
		m_vPlayerInfo.m_NoteData = ndTransformed;
		NoteDataUtil::RemoveAllTapsOfType(m_vPlayerInfo.m_NoteData,
										  TapNoteType_AutoKeysound);
		m_vPlayerInfo.m_pPlayer->Reload();
	}

	// load auto keysounds
	{
		NoteData nd = ndTransformed;
		NoteDataUtil::RemoveAllTapsExceptForType(nd, TapNoteType_AutoKeysound);
		m_AutoKeysounds.Load(m_vPlayerInfo.GetStepsAndTrailIndex(), nd);
	}

	{
		RString sType;
		switch (GAMESTATE->m_SongOptions.GetCurrent().m_SoundEffectType) {
			case SoundEffectType_Off:
				sType = "SoundEffectControl_Off";
				break;
			case SoundEffectType_Speed:
				sType = "SoundEffectControl_Speed";
				break;
			case SoundEffectType_Pitch:
				sType = "SoundEffectControl_Pitch";
				break;
			default:
				break;
		}

		m_vPlayerInfo.m_SoundEffectControl.Load(
		  sType, m_vPlayerInfo.GetPlayerState(), &m_vPlayerInfo.m_NoteData);
	}
}

void
ScreenGameplayPractice::TogglePause()
{
	// True if we were paused before now
	bool oldPause = GAMESTATE->GetPaused();
	// True if we are becoming paused
	bool newPause = !GAMESTATE->GetPaused();

	if (oldPause) {
		float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		m_pSoundMusic->Stop();

		RageTimer tm;
		const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);

		float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
		GetMusicEndTiming(fSecondsToStartFadingOutMusic,
						  fSecondsToStartTransitioningOut);

		// Restart the music with the proper params
		RageSoundParams p = m_pSoundMusic->GetParams();
		p.m_StartSecond = fSeconds - 0.01f;
		p.m_fSpeed = rate;
		// If using a loop region, use the loop params instead
		if (loopStart != loopEnd) {
			p.m_fFadeOutSeconds = 1.f;
			p.m_LengthSeconds = loopEnd + 5.f - loopStart;
			p.StopMode = RageSoundParams::M_LOOP;
		} else {
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
								MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
			p.StopMode = RageSoundParams::M_CONTINUE;
		}
		p.m_bAccurateSync = true;

		// Go
		m_pSoundMusic->Play(false, &p);

		// To force the music to actually loop like it should, need to do this
		// after starting
		if (loopStart != loopEnd) {
			p.m_StartSecond = loopStart - 2.f;
			m_pSoundMusic->SetParams(p);
		}
	}

	m_pSoundMusic->Pause(newPause);
	GAMESTATE->SetPaused(newPause);
}

void
ScreenGameplayPractice::SetSongPosition(float newSongPositionSeconds,
										float noteDelay,
										bool hardSeek,
										bool unpause)
{
	bool isPaused = GAMESTATE->GetPaused();
	RageSoundParams p = m_pSoundMusic->GetParams();

	// Letting this execute will freeze the music most of the time and thats bad
	if (loopStart != loopEnd && newSongPositionSeconds > loopEnd)
		return;

	// If paused, we need to move fast so dont use slow seeking
	// but if we want to hard seek, we dont care about speed
	p.m_bAccurateSync = !isPaused || hardSeek;
	m_pSoundMusic->SetParams(p);

	// realign mp3 files by seeking backwards to force a full reseek, then
	// seeking forward to finish the job
	if (hardSeek &&
		newSongPositionSeconds > GAMESTATE->m_Position.m_fMusicSeconds)
		SOUND->SetSoundPosition(m_pSoundMusic,
								GAMESTATE->m_Position.m_fMusicSeconds - 0.01f);

	// Set the final position
	SOUND->SetSoundPosition(m_pSoundMusic, newSongPositionSeconds - noteDelay);
	UpdateSongPosition(0);

	// Unpause the music if we want it unpaused
	if (unpause && isPaused)
		m_pSoundMusic->Pause(false);

	// Restart the notedata for the row we just moved to until the end of the
	// file
	Steps* pSteps = GAMESTATE->m_pCurSteps;
	TimingData* pTiming = pSteps->GetTimingData();
	const float fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const float fNotesBeat =
	  pTiming->GetBeatFromElapsedTime(newSongPositionSeconds);
	const int rowNow = BeatToNoteRow(fNotesBeat);

	// When using a loop region, dont load notedata too far ahead since we won't
	// see it
	if (loopStart != loopEnd) {
		const float endBeat = pTiming->GetBeatFromElapsedTime(loopEnd);
		const int rowEnd = BeatToNoteRow(endBeat);
		if (rowNow < rowEnd)
			SetupNoteDataFromRow(pSteps, rowNow, rowEnd);
	} else {
		SetupNoteDataFromRow(pSteps, rowNow);
	}

	// Reset the wife/judge counter related visible stuff
	PlayerPractice* pl = static_cast<PlayerPractice*>(m_vPlayerInfo.m_pPlayer);
	pl->RenderAllNotesIgnoreScores();
	pl->PositionReset();

	if (PREFSMAN->m_bEasterEggs)
		m_Toasty.Reset();

	// just having a message we can respond to directly is probably the best way
	// to reset lua elements rather than emulating a judgment message like
	// replays
	MESSAGEMAN->Broadcast("PracticeModeReset");
}

float
ScreenGameplayPractice::AddToRate(float amountAdded)
{
	float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	float newRate = rate + amountAdded;

	// Rates outside of this range may crash
	// Use 0.25 because of floating point errors...
	if (newRate <= 0.25f || newRate > 3.f)
		return rate;

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);

	float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
	GetMusicEndTiming(fSecondsToStartFadingOutMusic,
					  fSecondsToStartTransitioningOut);

	// Set Music params using new rate
	RageSoundParams p;
	p.m_fSpeed = newRate;
	GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = newRate;
	GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = newRate;
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = newRate;

	// If using loop region, also consider the loop params
	if (loopStart != loopEnd) {
		p.m_StartSecond = loopStart - 2.f;
		p.m_fFadeOutSeconds = 1.f;
		p.m_LengthSeconds = loopEnd + 5.f - loopStart;
		p.StopMode = RageSoundParams::M_LOOP;
	} else {
		p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
		p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
							MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		p.StopMode = RageSoundParams::M_CONTINUE;
	}
	p.m_bAccurateSync = true;

	// Apply param (rate) changes to the running Music
	m_pSoundMusic->SetParams(p);
	GAMESTATE->m_Position.m_fMusicSeconds = fSeconds;

	MESSAGEMAN->Broadcast(
	  "CurrentRateChanged"); // Tell the theme we changed the rate
	return newRate;
}

void
ScreenGameplayPractice::SetLoopRegion(float start, float end)
{
	loopStart = start;
	loopEnd = end;

	// Tell the Music that it should loop on a given region instead
	// 2 seconds are removed from the start for "intro"
	// 5 seconds are added to the end for "outro"
	// No notedata will occupy that space.
	RageSoundParams p = m_pSoundMusic->GetParams();
	p.m_StartSecond = start - 2.f;
	p.m_fFadeOutSeconds = 1.f;
	p.m_LengthSeconds = end + 5.f - start;
	p.StopMode = RageSoundParams::M_LOOP;

	// We dont reset notedata here because that could get repetitive and also be
	// annoying to users or cause other slowdowns

	m_pSoundMusic->SetParams(p);
}

void
ScreenGameplayPractice::ResetLoopRegion()
{
	// magic number defaults for loop region bounds
	loopStart = -2000.f;
	loopEnd = -2000.f;

	// Reload notedata for the entire file starting at current row
	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);
	auto td = GAMESTATE->m_pCurSteps->GetTimingData();
	const float startBeat = td->GetBeatFromElapsedTime(fSeconds);
	const int rowNow = BeatToNoteRow(startBeat);
	SetupNoteDataFromRow(GAMESTATE->m_pCurSteps, rowNow);

	float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
	GetMusicEndTiming(fSecondsToStartFadingOutMusic,
					  fSecondsToStartTransitioningOut);

	// Reapply the standard Music parameters
	RageSoundParams p = m_pSoundMusic->GetParams();
	p.m_StartSecond = GAMESTATE->m_pCurSong->GetFirstSecond() *
					  GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
	p.m_LengthSeconds =
	  fSecondsToStartFadingOutMusic + MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
	p.StopMode = RageSoundParams::M_CONTINUE;

	m_pSoundMusic->SetParams(p);
}

class LunaScreenGameplayPractice : public Luna<ScreenGameplayPractice>
{
  public:
	static int SetSongPosition(T* p, lua_State* L)
	{
		float position = FArg(1);
		float delay = FArg(2);
		bool hardseek = BArg(3);
		p->SetSongPosition(position, delay, hardseek);
		return 0;
	}

	static int SetSongPositionAndUnpause(T* p, lua_State* L)
	{
		float position = FArg(1);
		float delay = FArg(2);
		bool hardseek = BArg(3);
		p->SetSongPosition(position, delay, hardseek, true);
		return 0;
	}

	static int AddToRate(T* p, lua_State* L)
	{
		float rate = FArg(1);
		lua_pushnumber(L, p->AddToRate(rate));
		return 1;
	}

	static int TogglePause(T* p, lua_State* L)
	{
		p->TogglePause();
		return 0;
	}

	static int SetLoopRegion(T* p, lua_State* L)
	{
		float begin = FArg(1);
		float end = FArg(2);
		p->SetLoopRegion(begin, end);
		return 0;
	}

	static int ResetLoopRegion(T* p, lua_State* L)
	{
		p->ResetLoopRegion();
		return 0;
	}

	LunaScreenGameplayPractice()
	{
		ADD_METHOD(SetSongPosition);
		ADD_METHOD(SetSongPositionAndUnpause);
		ADD_METHOD(AddToRate);
		ADD_METHOD(TogglePause);
		ADD_METHOD(SetLoopRegion);
		ADD_METHOD(ResetLoopRegion);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenGameplayPractice, ScreenGameplay)
