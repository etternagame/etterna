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
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Models/Misc/RadarValues.h"
#include "Etterna/Singletons/DownloadManager.h"

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

	m_pSongBackground = NULL;
	m_pSongForeground = NULL;
	m_delaying_ready_announce = false;
	GAMESTATE->m_AdjustTokensBySongCostForFinalStageCheck = false;
	DLMAN->UpdateDLSpeed(true);
}

void
ScreenGameplayReplay::Init()
{
	ScreenGameplay::Init();

	m_fReplayBookmarkSeconds = 0.f;
}

ScreenGameplayReplay::~ScreenGameplayReplay()
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
	GAMESTATE->m_gameplayMode.Set(GameplayMode_Normal);

	DLMAN->UpdateDLSpeed(false);
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
					!m_NextSong.IsTransitioning())
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
		SCREENMAN->GetTopScreen()->SetPrevScreenName("ScreenStageInformation");
		BeginBackingOutFromGameplay();
	}

	return false;
}

void
ScreenGameplayReplay::SaveStats()
{
	// We need to replace the newly created replay data with the actual old
	// data Because to keep consistently lazy practices, we can just hack
	// things together instead of fixing the real issue -poco
	// (doing this fixes a lot of issues in the eval screen)
	PlayerStageStats* pss = m_vPlayerInfo.GetPlayerStageStats();
	HighScore* hs = PlayerAI::pScoreData;
	pss->m_vHoldReplayData = hs->GetHoldReplayDataVector();
	pss->m_vNoteRowVector = hs->GetNoteRowVector();
	pss->m_vOffsetVector = hs->GetOffsetVector();
	pss->m_vTapNoteTypeVector = hs->GetTapNoteTypeVector();
	pss->m_vTrackVector = hs->GetTrackVector();

	ScreenGameplay::SaveStats();
}

void
ScreenGameplayReplay::StageFinished(bool bBackedOut)
{
	if (bBackedOut) {
		GAMESTATE->CancelStage();
		return;
	}

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
	bool paused = GAMESTATE->GetPaused();

	// Stop the music and generate a new "music"
	m_pSoundMusic->Stop();

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);
	float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

	float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
	GetMusicEndTiming(fSecondsToStartFadingOutMusic,
					  fSecondsToStartTransitioningOut);

	// Set up current rate and new position to play
	RageSoundParams p;
	p.m_StartSecond = newPositionSeconds;
	p.m_fSpeed = rate;
	GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = rate;
	GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = rate;
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = rate;

	// Prevent endless music or something
	if (fSecondsToStartFadingOutMusic <
		GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
		p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
		p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
							MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
	}
	p.StopMode = RageSoundParams::M_CONTINUE;

	// If we scroll backwards, we need to render those notes again
	if (newPositionSeconds < fSeconds) {
		m_vPlayerInfo.m_pPlayer->RenderAllNotesIgnoreScores();
	}

	// If we are paused, set the volume to 0 so we don't make weird noises
	if (paused) {
		p.m_Volume = 0.f;
	} else {
		// Restart the file to make sure nothing weird is going on
		ReloadCurrentSong();
		STATSMAN->m_CurStageStats.m_player.InternalInit();
	}

	// Go
	m_pSoundMusic->Play(false, &p);
	// But only for like 1 frame if we are paused
	if (paused)
		m_pSoundMusic->Pause(true);

	// misc info update
	GAMESTATE->m_Position.m_fMusicSeconds = newPositionSeconds;
	UpdateSongPosition(0);
}

void
ScreenGameplayReplay::ToggleReplayPause()
{
	// True if we were paused before now
	bool oldPause = GAMESTATE->GetPaused();
	// True if we are becoming paused
	bool newPause = !GAMESTATE->GetPaused();

	// We are leaving pause mode
	if (oldPause) {
		RageTimer tm;

		const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);
		float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

		// Restart the stage, technically (This will cause a lot of lag if there
		// are a lot of notes.)
		ReloadCurrentSong();
		STATSMAN->m_CurStageStats.m_player.InternalInit();
		PlayerAI::SetScoreData(PlayerAI::pScoreData);
		PlayerAI::SetUpExactTapMap(PlayerAI::pReplayTiming);

		// Reset the wife/judge counter related visible stuff
		FOREACH_ENUM(TapNoteScore, tns)
		{
			Message msg = Message("Judgment");
			msg.SetParam("Judgment", tns);
			msg.SetParam("WifePercent", 0);
			msg.SetParam("Player", 0);
			msg.SetParam("TapNoteScore", tns);
			msg.SetParam("FirstTrack", 0);
			msg.SetParam("CurWifeScore", 0);
			msg.SetParam("MaxWifeScore", 0);
			msg.SetParam("WifeDifferential", 0);
			msg.SetParam("TotalPercent", 0);
			msg.SetParam("Type", RString("Tap"));
			msg.SetParam("Val", 0);
			MESSAGEMAN->Broadcast(msg);
		}

		// Set up the stage music to current params, simply
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

		// Unpause
		m_pSoundMusic->Play(false, &p);
		GAMESTATE->m_Position.m_fMusicSeconds = fSeconds;
		UpdateSongPosition(0);
		// SCREENMAN->SystemMessage("Unpaused Replay");
	} else {
		// Almost all of gameplay is based on the music moving.
		// If the music is paused, nothing works.
		// This is all we have to do.
		m_pSoundMusic->Pause(newPause);
		// SCREENMAN->SystemMessage("Paused Replay");
	}
	GAMESTATE->SetPaused(newPause);
}

// lua
class LunaScreenGameplayReplay : public Luna<ScreenGameplayReplay>
{
  public:
	static int SetReplayPosition(T* p, lua_State* L)
	{
		float newpos = FArg(1);
		if (GAMESTATE->GetPaused())
			p->SetSongPosition(newpos);
		return 0;
	}
	static int SetReplayRate(T* p, lua_State* L)
	{
		float newrate = FArg(1);
		if (!GAMESTATE->GetPaused()) {
			lua_pushnumber(L, -1.f);
			return 1;
		}
		lua_pushnumber(L, p->SetRate(newrate));
		return 1;
	}
	static int ToggleReplayPause(T* p, lua_State* L)
	{
		p->ToggleReplayPause();
		return 0;
	}
	static int SetReplayBookmark(T* p, lua_State* L)
	{
		float position = FArg(1);
		p->m_fReplayBookmarkSeconds = position;
		return 0;
	}
	static int JumpToReplayBookmark(T* p, lua_State* L)
	{
		if (GAMESTATE->GetPaused()) {
			p->SetSongPosition(p->m_fReplayBookmarkSeconds);
			return 0;
		}
		return 0;
	}

	LunaScreenGameplayReplay()
	{
		ADD_METHOD(SetReplayPosition);
		ADD_METHOD(SetReplayRate);
		ADD_METHOD(ToggleReplayPause);
		ADD_METHOD(SetReplayBookmark);
		ADD_METHOD(JumpToReplayBookmark);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenGameplayReplay, ScreenGameplay)
