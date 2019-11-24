#include "Etterna/Globals/global.h"
#include "ScreenGameplayReplay.h"
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
#include "Etterna/Models/ScoreKeepers/ScoreKeeper.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Models/Misc/RadarValues.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/GameSoundManager.h"

#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/LuaManager.h"

REGISTER_SCREEN_CLASS(ScreenGameplayReplay);

void
ScreenGameplayReplay::FillPlayerInfo(PlayerInfo* playerInfoOut)
{
	playerInfoOut->Load(PLAYER_1,
						MultiPlayer_Invalid,
						true,
						Difficulty_Invalid,
						GameplayMode_Replay);
}

ScreenGameplayReplay::ScreenGameplayReplay()
{
	ASSERT_M(PlayerAI::pScoreData != nullptr,
			 "Replay Highscore Info was empty.");

	// Set up rate
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = PlayerAI::replayRate;

	if (PREFSMAN->m_bReplaysUseScoreMods) {
		// Set up mods
		GAMESTATE->m_pPlayerState->m_PlayerOptions.Init();
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred().FromString(
		  PlayerAI::replayModifiers);
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().FromString(
		  PlayerAI::replayModifiers);
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong().FromString(
		  PlayerAI::replayModifiers);
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage().FromString(
		  PlayerAI::replayModifiers);

		// Undo noteskin change
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred().m_sNoteSkin =
		  PlayerAI::oldNoteskin;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_sNoteSkin =
		  PlayerAI::oldNoteskin;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong().m_sNoteSkin =
		  PlayerAI::oldNoteskin;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage().m_sNoteSkin =
		  PlayerAI::oldNoteskin;
	}
}

void
ScreenGameplayReplay::Init()
{
	ScreenGameplay::Init();

	m_fReplayBookmarkSeconds = 0.f;
}

ScreenGameplayReplay::~ScreenGameplayReplay()
{
	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("ScreenGameplayReplay::~ScreenGameplayReplay()");

	if (!GAMESTATE->m_bRestartedGameplay) {
		if (PREFSMAN->m_bReplaysUseScoreMods) {
			GAMESTATE->m_pPlayerState->m_PlayerOptions.Init();
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred()
			  .FromString(PlayerAI::oldModifiers);
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().FromString(
			  PlayerAI::oldModifiers);
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong().FromString(
			  PlayerAI::oldModifiers);
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage().FromString(
			  PlayerAI::oldModifiers);
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred()
			  .m_sNoteSkin = PlayerAI::oldNoteskin;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
			  .m_sNoteSkin = PlayerAI::oldNoteskin;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong().m_sNoteSkin =
			  PlayerAI::oldNoteskin;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage().m_sNoteSkin =
			  PlayerAI::oldNoteskin;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred()
			  .m_FailType = PlayerAI::oldFailType;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_FailType =
			  PlayerAI::oldFailType;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong().m_FailType =
			  PlayerAI::oldFailType;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage().m_FailType =
			  PlayerAI::oldFailType;
		}
		GAMESTATE->m_SongOptions.Init();
		GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate =
		  PlayerAI::oldRate;
		GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = PlayerAI::oldRate;
		GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = PlayerAI::oldRate;
	} else
		PlayerAI::SetScoreData();
}

void
ScreenGameplayReplay::Update(float fDeltaTime)
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
			PlayerNumber pn = m_vPlayerInfo.GetStepsAndTrailIndex();

			// Update living players' alive time
			// HACK: Don't scale alive time when using tab/tilde.  Instead of
			// accumulating time from a timer, this time should instead be tied
			// to the music position.
			float fUnscaledDeltaTime = m_timerGameplaySeconds.GetDeltaTime();
			if (!m_vPlayerInfo.GetPlayerStageStats()->m_bFailed)
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
			{
				float fSecondsToStartFadingOutMusic,
				  fSecondsToStartTransitioningOut;
				GetMusicEndTiming(fSecondsToStartFadingOutMusic,
								  fSecondsToStartTransitioningOut);

				bool bAllReallyFailed = STATSMAN->m_CurStageStats.AllFailed();
				if (bAllReallyFailed)
					fSecondsToStartTransitioningOut += BEGIN_FAILED_DELAY;

				if (GAMESTATE->m_Position.m_fMusicSeconds >=
					  fSecondsToStartTransitioningOut &&
					!m_NextSong.IsTransitioning() && !GAMESTATE->GetPaused())
					this->PostScreenMessage(SM_NotesEnded, 0);
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

bool
ScreenGameplayReplay::Input(const InputEventPlus& input)
{
	// LOG->Trace( "ScreenGameplayReplay::Input()" );

	Message msg("");
	if (m_Codes.InputMessage(input, msg))
		this->HandleMessage(msg);

	if (m_DancingState != STATE_OUTRO && GAMESTATE->IsHumanPlayer(input.pn) &&
		!m_Cancel.IsTransitioning()) {

		// Exiting gameplay by pressing Back (Immediate Exit)
		bool bHoldingBack = false;
		if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(
			  input.GameI) == Column_Invalid) {
			bHoldingBack |= input.MenuI == GAME_BUTTON_BACK;
		}

		if (bHoldingBack) {
			if (((!PREFSMAN->m_bDelayedBack && input.type == IET_FIRST_PRESS) ||
				 (input.DeviceI.device == DEVICE_KEYBOARD &&
				  input.type == IET_REPEAT) ||
				 (input.DeviceI.device != DEVICE_KEYBOARD &&
				  INPUTFILTER->GetSecsHeld(input.DeviceI) >= 1.0f))) {
				if (PREFSMAN->m_verbose_log > 1)
					LOG->Trace("Player %i went back", input.pn + 1);
				BeginBackingOutFromGameplay();
			} else if (PREFSMAN->m_bDelayedBack &&
					   input.type == IET_FIRST_PRESS) {
				m_textDebug.SetText(GIVE_UP_BACK_TEXT);
				m_textDebug.PlayCommand("BackOn");
			} else if (PREFSMAN->m_bDelayedBack && input.type == IET_RELEASE) {
				m_textDebug.PlayCommand("TweenOff");
			}

			return true;
		}
	}

	if (!input.GameI.IsValid())
		return false;

	/* Restart gameplay button moved from theme to allow for rebinding for
	 * people who dont want to edit lua files :)
	 */
	bool bHoldingRestart = false;
	if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(input.GameI) ==
		Column_Invalid) {
		bHoldingRestart |= input.MenuI == GAME_BUTTON_RESTART;
	}
	if (bHoldingRestart) {
		RestartGameplay();
	}

	return false;
}

void
ScreenGameplayReplay::SaveStats()
{
	// Reload the notedata after finishing in case we truncated it
	SetupNoteDataFromRow(GAMESTATE->m_pCurSteps, -1);
	// Reload the replay data to make sure it is clean for calculations
	PlayerAI::SetScoreData();
	PlayerAI::SetUpExactTapMap(PlayerAI::pReplayTiming);

	ScreenGameplay::SaveStats();
}

void
ScreenGameplayReplay::StageFinished(bool bBackedOut)
{
	if (bBackedOut) {
		GAMESTATE->CancelStage();
		return;
	}

	auto pss = m_vPlayerInfo.GetPlayerStageStats();
	// Makes sure all PlayerStageStats discrepancies are corrected forcibly.
	PlayerAI::SetPlayerStageStatsForReplay(pss);

	STATSMAN->m_CurStageStats.FinalizeScores(false);

	STATSMAN->m_vPlayedStageStats.push_back(STATSMAN->m_CurStageStats);

	STATSMAN->CalcAccumPlayedStageStats();
	GAMESTATE->FinishStage();
}

float
ScreenGameplayReplay::SetRate(float newRate)
{
	float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

	// Rates outside of this range may crash
	if (newRate < 0.3f || newRate > 5.f)
		return rate;

	bool paused = GAMESTATE->GetPaused();

	// Stop the music and generate a new "music"
	m_pSoundMusic->Stop();

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);

	float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
	GetMusicEndTiming(fSecondsToStartFadingOutMusic,
					  fSecondsToStartTransitioningOut);

	RageSoundParams p;
	p.m_StartSecond = fSeconds;
	// Turns out the music rate doesn't affect anything by itself, so we have to
	// set every rate
	p.m_fSpeed = newRate;
	GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = newRate;
	GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = newRate;
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = newRate;
	// Prevent music from making noise when doing things in pause mode
	// Volume gets reset when leaving pause mode or doing almost anything else
	if (paused)
		p.m_Volume = 0.f;
	// Set up the music so we don't wait for an Etternaty when messing around
	// near the end of the song.
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

	// misc info update
	GAMESTATE->m_Position.m_fMusicSeconds = fSeconds;
	UpdateSongPosition(0);
	MESSAGEMAN->Broadcast(
	  "CurrentRateChanged"); // Tell the theme we changed the rate

	return newRate;
}

void
ScreenGameplayReplay::SetSongPosition(float newPositionSeconds)
{
	// If you go too far negative, bad things may happen
	// But remember some files have notes at 0.0 seconds
	if (newPositionSeconds <= 0)
		newPositionSeconds = 0.f;
	SOUND->SetSoundPosition(m_pSoundMusic, newPositionSeconds);

	bool paused = GAMESTATE->GetPaused();
	m_pSoundMusic->Pause(paused);

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);

	m_vPlayerInfo.m_pPlayer->RenderAllNotesIgnoreScores();

	// Lightly reset some stats in case someone wants them
	// Precalculated values are put in their place.
	if (!paused) {
		// Current StageStats is going to get overwritten so let's clear it here
		STATSMAN->m_CurStageStats.m_player.InternalInit();
	}

	const float fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const int rowNow = BeatToNoteRow(fSongBeat);
	// This breaks some oop standard in some book
	PlayerStageStats* pss = m_vPlayerInfo.GetPlayerStageStats();
	auto rs = PlayerAI::GetReplaySnapshotForNoterow(rowNow);
	FOREACH_ENUM(TapNoteScore, tns)
	{
		pss->m_iTapNoteScores[tns] = rs->judgments[tns];
	}
	FOREACH_ENUM(HoldNoteScore, hns)
	{
		pss->m_iHoldNoteScores[hns] = rs->hns[hns];
	}
}

void
ScreenGameplayReplay::TogglePause()
{
	// True if we were paused before now
	bool oldPause = GAMESTATE->GetPaused();
	// True if we are becoming paused
	bool newPause = !GAMESTATE->GetPaused();
	RageTimer tm;

	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);
	float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	Steps* pSteps = GAMESTATE->m_pCurSteps;
	const float fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const int rowNow = BeatToNoteRow(fSongBeat);

	// We are leaving pause mode
	if (oldPause) {
		m_pSoundMusic->Stop();

		// Basically reinitialize all stage data from precalculated things
		// Restarts the basic replay data in case something went weird
		SetupNoteDataFromRow(pSteps, rowNow);
		STATSMAN->m_CurStageStats.m_player.InternalInit();
		PlayerAI::SetScoreData(PlayerAI::pScoreData, rowNow);
		PlayerAI::SetUpExactTapMap(PlayerAI::pReplayTiming);

		PlayerStageStats* pss = m_vPlayerInfo.GetPlayerStageStats();
		auto rs = PlayerAI::GetReplaySnapshotForNoterow(rowNow);
		FOREACH_ENUM(TapNoteScore, tns)
		{
			pss->m_iTapNoteScores[tns] = rs->judgments[tns];
		}
		FOREACH_ENUM(HoldNoteScore, hns)
		{
			pss->m_iHoldNoteScores[hns] = rs->hns[hns];
		}
		PlayerState* ps = m_vPlayerInfo.GetPlayerState();
		m_vPlayerInfo.m_pPlayer->curwifescore = rs->curwifescore;
		m_vPlayerInfo.m_pPlayer->maxwifescore = rs->maxwifescore;

		// Reset the wife/judge counter related visible stuff
		FOREACH_ENUM(TapNoteScore, tns)
		{
			Message msg = Message("Judgment");
			msg.SetParam("FromReplay", true);
			msg.SetParam("Judgment", tns);
			msg.SetParam("WifePercent",
						 100 * rs->curwifescore / rs->maxwifescore);
			msg.SetParam("Player", 0);
			msg.SetParam("TapNoteScore", tns);
			msg.SetParam("FirstTrack", 0);
			msg.SetParam("CurWifeScore", rs->curwifescore);
			msg.SetParam("MaxWifeScore", rs->maxwifescore);
			msg.SetParam("WifeDifferential",
						 rs->curwifescore -
						   rs->maxwifescore * ps->playertargetgoal);
			msg.SetParam("TotalPercent",
						 100 * rs->curwifescore /
						   m_vPlayerInfo.m_pPlayer->totalwifescore);
			msg.SetParam("Type", RString("Tap"));
			msg.SetParam("Val", pss->m_iTapNoteScores[tns]);
			MESSAGEMAN->Broadcast(msg);
		}
		// We have to hackily only allow LetGo and Held through
		// because til death decided that it should be this way
		for (HoldNoteScore hns = HNS_LetGo; hns <= HNS_Held;
			 hns = static_cast<HoldNoteScore>(hns + 1)) {
			Message msg = Message("Judgment");
			msg.SetParam("FromReplay", true);
			msg.SetParam("Player", 0);
			msg.SetParam("MultiPlayer", 0);
			msg.SetParam("WifePercent",
						 100 * rs->curwifescore / rs->maxwifescore);
			msg.SetParam("FirstTrack", 0);
			msg.SetParam("CurWifeScore", rs->curwifescore);
			msg.SetParam("MaxWifeScore", rs->maxwifescore);
			msg.SetParam("WifeDifferential",
						 rs->curwifescore -
						   rs->maxwifescore * ps->playertargetgoal);
			msg.SetParam("TotalPercent",
						 100 * rs->curwifescore /
						   m_vPlayerInfo.m_pPlayer->totalwifescore);
			msg.SetParam("FirstTrack", 0);
			msg.SetParam(
			  "NumTracks",
			  static_cast<int>(
				m_vPlayerInfo.m_pPlayer->GetNoteData().GetNumTracks()));
			msg.SetParam("TapNoteScore", TapNoteScore_Invalid);
			msg.SetParam("HoldNoteScore", HoldNoteScore_Invalid);
			msg.SetParam("Judgment", hns);
			msg.SetParam("Type", std::string("Hold"));
			msg.SetParam("Val", pss->m_iHoldNoteScores[hns]);
			MESSAGEMAN->Broadcast(msg);
		}

		// Set up the stage music to current params, simply
		float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
		GetMusicEndTiming(fSecondsToStartFadingOutMusic,
						  fSecondsToStartTransitioningOut);

		RageSoundParams p;
		p.m_StartSecond = fSeconds - 0.25f;
		p.m_fSpeed = rate;
		if (fSecondsToStartFadingOutMusic <
			GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
								MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		}
		p.StopMode = RageSoundParams::M_CONTINUE;
		p.m_bAccurateSync = true;

		// Unpause
		m_pSoundMusic->Play(false, &p);
	} else {
		// Almost all of gameplay is based on the music moving.
		// If the music is paused, nothing works.
		// This is all we have to do.

		// Set up the stage music to current params, simply
		float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
		GetMusicEndTiming(fSecondsToStartFadingOutMusic,
						  fSecondsToStartTransitioningOut);
		RageSoundParams p;
		p.m_fSpeed = rate;
		if (fSecondsToStartFadingOutMusic <
			GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
								MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		}
		p.StopMode = RageSoundParams::M_CONTINUE;
		// Turn accuratesync off for going into pause mode
		// to allow for smooth seeking in any direction
		// AccurateSync is turned back on later when unpausing.
		p.m_bAccurateSync = false;
		m_pSoundMusic->SetParams(p);
	}
	m_pSoundMusic->Pause(newPause);
	GAMESTATE->SetPaused(newPause);
}

// lua
class LunaScreenGameplayReplay : public Luna<ScreenGameplayReplay>
{
  public:
	static int SetSongPosition(T* p, lua_State* L)
	{
		float newpos = FArg(1);
		if (GAMESTATE->GetPaused())
			p->SetSongPosition(newpos);
		return 0;
	}
	static int SetRate(T* p, lua_State* L)
	{
		float newrate = FArg(1);
		if (!GAMESTATE->GetPaused()) {
			lua_pushnumber(L, -1.f);
			return 1;
		}
		lua_pushnumber(L, p->SetRate(newrate));
		return 1;
	}
	static int TogglePause(T* p, lua_State* L)
	{
		p->TogglePause();
		return 0;
	}
	static int SetBookmark(T* p, lua_State* L)
	{
		float position = FArg(1);
		p->m_fReplayBookmarkSeconds = position;
		return 0;
	}
	static int JumpToBookmark(T* p, lua_State* L)
	{
		if (GAMESTATE->GetPaused()) {
			p->SetSongPosition(p->m_fReplayBookmarkSeconds);
			return 0;
		}
		return 0;
	}

	LunaScreenGameplayReplay()
	{
		ADD_METHOD(SetSongPosition);
		ADD_METHOD(SetRate);
		ADD_METHOD(TogglePause);
		ADD_METHOD(SetBookmark);
		ADD_METHOD(JumpToBookmark);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenGameplayReplay, ScreenGameplay)
