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

		RageSoundParams p = m_pSoundMusic->GetParams();
		p.m_StartSecond = fSeconds - 0.01f;
		p.m_fSpeed = rate;
		if (fSecondsToStartFadingOutMusic <
			GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
								MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		}
		p.StopMode = RageSoundParams::M_CONTINUE;
		p.m_bAccurateSync = true;
		// Go
		m_pSoundMusic->Play(false, &p);
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

	if (unpause && isPaused)
		m_pSoundMusic->Pause(false);

	Steps* pSteps = GAMESTATE->m_pCurSteps;
	TimingData* pTiming = pSteps->GetTimingData();
	const float fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const float fNotesBeat =
	  pTiming->GetBeatFromElapsedTime(newSongPositionSeconds);
	const int rowNow = BeatToNoteRow(fNotesBeat);
	SetupNoteDataFromRow(pSteps, rowNow);

	// Reset the wife/judge counter related visible stuff
	PlayerPractice* pl = static_cast<PlayerPractice*>(m_vPlayerInfo.m_pPlayer);
	pl->RenderAllNotesIgnoreScores();
	pl->PositionReset();

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

	bool paused = GAMESTATE->GetPaused();

	m_pSoundMusic->Stop();

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);

	float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
	GetMusicEndTiming(fSecondsToStartFadingOutMusic,
					  fSecondsToStartTransitioningOut);

	RageSoundParams p;
	p.m_StartSecond = fSeconds;
	p.m_fSpeed = newRate;
	GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = newRate;
	GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = newRate;
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = newRate;
	if (paused)
		p.m_Volume = 0.f;
	if (fSecondsToStartFadingOutMusic <
		GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
		p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
		p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
							MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
	}
	p.StopMode = RageSoundParams::M_CONTINUE;
	p.m_bAccurateSync = true;
	// Go
	m_pSoundMusic->Play(false, &p);
	// But only for like 1 frame if we are paused
	if (paused)
		m_pSoundMusic->Pause(true);
	GAMESTATE->m_Position.m_fMusicSeconds = fSeconds;

	MESSAGEMAN->Broadcast(
	  "CurrentRateChanged"); // Tell the theme we changed the rate
	return newRate;
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

	LunaScreenGameplayPractice()
	{
		ADD_METHOD(SetSongPosition);
		ADD_METHOD(SetSongPositionAndUnpause);
		ADD_METHOD(AddToRate);
		ADD_METHOD(TogglePause);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenGameplayPractice, ScreenGameplay)
