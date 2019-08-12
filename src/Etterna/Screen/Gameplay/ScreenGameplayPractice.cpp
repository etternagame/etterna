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
#include "Etterna/Models/Misc/RadarValues.h"
#include "Etterna/Singletons/DownloadManager.h"

#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/LuaManager.h"

REGISTER_SCREEN_CLASS(ScreenGameplayPractice);

void
ScreenGameplayPractice::FillPlayerInfo(PlayerInfo* playerInfoOut)
{
	playerInfoOut->Load(
	  PLAYER_1, MultiPlayer_Invalid, true, Difficulty_Invalid);
}

ScreenGameplayPractice::ScreenGameplayPractice()
{
	m_pSongBackground = NULL;
	m_pSongForeground = NULL;
	m_delaying_ready_announce = false;
	GAMESTATE->m_AdjustTokensBySongCostForFinalStageCheck = false;
	DLMAN->UpdateDLSpeed(true);
}

void
ScreenGameplayPractice::Init()
{
	ScreenGameplay::Init();
}

ScreenGameplayPractice::~ScreenGameplayPractice()
{
	GAMESTATE->m_AdjustTokensBySongCostForFinalStageCheck = true;
	if (this->IsFirstUpdate()) {
		/* We never received any updates. That means we were deleted without
		 * being used, and never actually played. (This can happen when backing
		 * out of ScreenStage.) Cancel the stage. */
		GAMESTATE->CancelStage();
	}

	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("ScreenGameplayReplay::~ScreenGameplayReplay()");

	SAFE_DELETE(m_pSongBackground);
	SAFE_DELETE(m_pSongForeground);

	if (m_pSoundMusic != nullptr)
		m_pSoundMusic->StopPlaying();

	m_GameplayAssist.StopPlaying();

	DLMAN->UpdateDLSpeed(false);
}

void
ScreenGameplayPractice::TogglePracticePause()
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

		RageSoundParams p;
		p.m_StartSecond = fSeconds;
		p.m_fSpeed = rate;
		if (fSecondsToStartFadingOutMusic <
			GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
								MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		}
		p.StopMode = RageSoundParams::M_CONTINUE;
		// Go
		m_pSoundMusic->Play(false, &p);
	} else {
		m_pSoundMusic->Pause(newPause);
	}
	GAMESTATE->SetPaused(newPause);
}

void
ScreenGameplayPractice::SetPracticeSongPosition(float newPositionSeconds)
{
	m_pSoundMusic->SetPositionSeconds(newPositionSeconds);

	bool isPaused = GAMESTATE->GetPaused();
	m_pSoundMusic->Pause(isPaused);

	m_vPlayerInfo.m_pPlayer->RenderAllNotesIgnoreScores();

	// curwifescore sent via judgment msgs is stored in player
	auto pl = m_vPlayerInfo.m_pPlayer;
	// but the tns counts are stored here
	auto stats = m_vPlayerInfo.GetPlayerStageStats();

	// Reset the wife/judge counter related visible stuff
	// we should _probably_ reset the replaydata vectors but i don't feel like
	// it if we are refactoring gameplay soon
	pl->curwifescore = 0;
	FOREACH_ENUM(TapNoteScore, tns)
	stats->m_iTapNoteScores[tns] = 0;
	FOREACH_ENUM(TapNoteScore, hns)
	stats->m_iHoldNoteScores[hns] = 0;

	// just having a message we can respond to directly is probably the best way
	// to reset lua elemenmts rather than emulating a judgment message like
	// replays
	MESSAGEMAN->Broadcast("PracticeModeReset");
}

float
ScreenGameplayPractice::AddToPracticeRate(float amountAdded)
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
	static int SetPreviewNoteFieldMusicPosition(T* p, lua_State* L)
	{
		float given = FArg(1);
		p->SetPracticeSongPosition(given);
		return 0;
	}

	static int AddToPracticeRate(T* p, lua_State* L)
	{
		float rate = FArg(1);
		lua_pushnumber(L, p->AddToPracticeRate(rate));
		return 1;
	}

	static int TogglePracticePause(T* p, lua_State* L)
	{
		p->TogglePracticePause();
		return 0;
	}

	LunaScreenGameplayPractice()
	{
		ADD_METHOD(SetPreviewNoteFieldMusicPosition);
		ADD_METHOD(AddToPracticeRate);
		ADD_METHOD(TogglePracticePause);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenGameplayPractice, ScreenGameplay)
