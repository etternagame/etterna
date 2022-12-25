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
#include "Etterna/Models/Misc/PlayerInfo.h"
#include "Etterna/Models/Misc/PlayerStageStats.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/ReplayManager.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"

#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Models/Songs/SongOptions.h"

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
	ASSERT_M(REPLAYS->GetActiveReplayScore() != nullptr,
			 "Replay Highscore Info was empty.");

	m_fReplayBookmarkSeconds = 0.F;

	auto settings = REPLAYS->GetActiveReplaySettings();

	// Set up rate
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = settings.replayRate;
	GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = settings.replayRate;
	GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = settings.replayRate;
	GAMESTATE->m_SongOptions.GetStage().m_fMusicRate = settings.replayRate;

	PlayerOptions po;
	po.Init();
	po.SetForReplay(true);
	po.FromString(settings.replayModifiers);

	// For all non transforming mods
	if (PREFSMAN->m_bReplaysUseScoreMods) {
		// Set up mods
		GAMESTATE->m_pPlayerState->m_PlayerOptions.Init();
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred() = po;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent() = po;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong() = po;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage() = po;

		// Undo noteskin change
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred().m_sNoteSkin =
		  settings.oldNoteskin;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_sNoteSkin =
		  settings.oldNoteskin;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong().m_sNoteSkin =
		  settings.oldNoteskin;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage().m_sNoteSkin =
		  settings.oldNoteskin;
	}

	// Set up transforming mods
	{
		auto f = [&po, &settings](PlayerOptions& playerOptions) {
			std::copy(std::begin(po.m_bTurns),
					  std::end(po.m_bTurns),
					  std::begin(playerOptions.m_bTurns));
			if (settings.replayRngSeed == 0) {
				po.m_bTurns[PlayerOptions::TURN_SHUFFLE] = false;
				po.m_bTurns[PlayerOptions::TURN_SOFT_SHUFFLE] = false;
				po.m_bTurns[PlayerOptions::TURN_SUPER_SHUFFLE] = false;
				po.m_bTurns[PlayerOptions::TURN_HRAN_SHUFFLE] = false;
			}
		};
		f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred());
		f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent());
		f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong());
		f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage());
	}

	// Set up mirror (this is redundant)
	FOREACH_ENUM(ModsLevel, lvl)
	{
		PO_GROUP_ASSIGN_N(GAMESTATE->m_pPlayerState->m_PlayerOptions,
						  lvl,
						  m_bTurns,
						  PlayerOptions::TURN_MIRROR,
						  settings.replayUsedMirror);
	}
	// We don't have to ever turn off mirror actually, due to the
	// reinitialization of PlayerOptions in the deconstructor.
}

void
ScreenGameplayReplay::Init()
{
	ScreenGameplay::Init();

	m_fReplayBookmarkSeconds = 0.F;
}

void
ScreenGameplayReplay::LoadPlayer()
{
	auto settings = REPLAYS->GetActiveReplaySettings();
	if (settings.replayRngSeed != 0) {
		GAMESTATE->m_iStageSeed = settings.replayRngSeed;
	}
	m_vPlayerInfo.m_pPlayer->Load();
}

void
ScreenGameplayReplay::ReloadPlayer()
{
	auto settings = REPLAYS->GetActiveReplaySettings();
	if (settings.replayRngSeed != 0) {
		GAMESTATE->m_iStageSeed = settings.replayRngSeed;
	}
	m_vPlayerInfo.m_pPlayer->Reload();
}

void
ScreenGameplayReplay::LoadScoreKeeper()
{
	auto settings = REPLAYS->GetActiveReplaySettings();
	if (settings.replayRngSeed != 0) {
		GAMESTATE->m_iStageSeed = settings.replayRngSeed;
	}
	if (m_vPlayerInfo.m_pPrimaryScoreKeeper != nullptr) {
		m_vPlayerInfo.m_pPrimaryScoreKeeper->Load(m_apSongsQueue,
												  m_vPlayerInfo.m_vpStepsQueue);
	}
}

ScreenGameplayReplay::~ScreenGameplayReplay()
{
	Locator::getLogger()->debug("ScreenGameplayReplay::~ScreenGameplayReplay()");

	if (!GAMESTATE->m_bRestartedGameplay) {
		auto settings = REPLAYS->GetActiveReplaySettings();

		if (settings.replayRngSeed != 0) {
			GAMESTATE->m_iStageSeed = settings.oldRngSeed;
		}

		PlayerOptions po;
		po.Init();
		po.SetForReplay(true);
		po.FromString(settings.oldModifiers);

		// For all non transforming mods
		if (PREFSMAN->m_bReplaysUseScoreMods) {
			GAMESTATE->m_pPlayerState->m_PlayerOptions.Init();
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred() = po;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent() = po;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong() = po;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage() = po;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred()
			  .m_sNoteSkin = settings.oldNoteskin;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
			  .m_sNoteSkin = settings.oldNoteskin;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong().m_sNoteSkin =
			  settings.oldNoteskin;
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage().m_sNoteSkin =
			  settings.oldNoteskin;
		}

		// Fix transforming mods
		{
			auto f = [&po, &settings](PlayerOptions& playerOptions) {
				std::copy(std::begin(po.m_bTurns),
						  std::end(po.m_bTurns),
						  std::begin(playerOptions.m_bTurns));
			};
			f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred());
			f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent());
			f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong());
			f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage());
		}

		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred().m_FailType =
		  settings.oldFailType;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_FailType =
		  settings.oldFailType;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong().m_FailType =
		  settings.oldFailType;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage().m_FailType =
		  settings.oldFailType;
		GAMESTATE->m_SongOptions.Init();
		GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = settings.oldRate;
		GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = settings.oldRate;
		GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = settings.oldRate;
		GAMESTATE->m_SongOptions.GetStage().m_fMusicRate = settings.oldRate;

		REPLAYS->ResetActiveReplaySettings();
	}
}

void
ScreenGameplayReplay::Update(const float fDeltaTime)
{
	if (GAMESTATE->m_pCurSong == nullptr) {
		ScreenWithMenuElements::Update(fDeltaTime); // NOLINT(bugprone-parent-virtual-call)
		return;
	}

	UpdateSongPosition();

	if (m_bZeroDeltaOnNextUpdate) {
		ScreenWithMenuElements::Update(0); // NOLINT(bugprone-parent-virtual-call)
		m_bZeroDeltaOnNextUpdate = false;
	} else {
		ScreenWithMenuElements::Update(fDeltaTime); // NOLINT(bugprone-parent-virtual-call)
	}

	if (SCREENMAN->GetTopScreen() != this) {
		return;
	}

	m_AutoKeysounds.Update(fDeltaTime);

	m_vPlayerInfo.m_SoundEffectControl.Update(fDeltaTime);

	{
		const auto fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		auto p = m_pSoundMusic->GetParams();
		if (std::fabs(p.m_fSpeed - fSpeed) > 0.01F && fSpeed >= 0.0F) {
			p.m_fSpeed = fSpeed;
			m_pSoundMusic->SetParams(p);
		}
	}

	switch (m_DancingState) {
		case STATE_DANCING: {
			{
				float fSecondsToStartFadingOutMusic;
				float fSecondsToStartTransitioningOut;
				GetMusicEndTiming(fSecondsToStartFadingOutMusic,
								  fSecondsToStartTransitioningOut);

				const auto bAllReallyFailed =
				  STATSMAN->m_CurStageStats.Failed();
				if (bAllReallyFailed) {
					fSecondsToStartTransitioningOut += BEGIN_FAILED_DELAY;
				}

				if (GAMESTATE->m_Position.m_fMusicSeconds >=
					  fSecondsToStartTransitioningOut &&
					!m_NextSong.IsTransitioning() && !GAMESTATE->GetPaused()) {
					this->PostScreenMessage(SM_NotesEnded, 0);
				}
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

auto
ScreenGameplayReplay::Input(const InputEventPlus& input) -> bool
{
	// LOG->Trace( "ScreenGameplayReplay::Input()" );

	Message msg("");
	if (m_Codes.InputMessage(input, msg)) {
		this->HandleMessage(msg);
	}

	if (m_DancingState != STATE_OUTRO && GAMESTATE->IsHumanPlayer(input.pn) &&
		!m_Cancel.IsTransitioning()) {

		// Exiting gameplay by pressing Back (Immediate Exit)
		auto bHoldingBack = false;
		if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(
			  input.GameI) == Column_Invalid) {
			bHoldingBack |= input.MenuI == GAME_BUTTON_BACK;
		}

		if (bHoldingBack) {
			if (((!PREFSMAN->m_bDelayedBack && input.type == IET_FIRST_PRESS) ||
				 (input.DeviceI.device == DEVICE_KEYBOARD &&
				  input.type == IET_REPEAT) ||
				 (input.DeviceI.device != DEVICE_KEYBOARD &&
				  INPUTFILTER->GetSecsHeld(input.DeviceI) >= 1.0F))) {
				Locator::getLogger()->info("Player {} went back", input.pn + 1);
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

	if (!input.GameI.IsValid()) {
		return false;
	}

	/* Restart gameplay button moved from theme to allow for rebinding for
	 * people who dont want to edit lua files :)
	 */
	auto bHoldingRestart = false;
	if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(input.GameI) ==
		Column_Invalid) {
		bHoldingRestart |= input.MenuI == GAME_BUTTON_RESTART;
	}
	if (bHoldingRestart) {
		// delayedback pref will work, or if it's off just go immediately
		// but also just let it be instant if you failed
		if ((PREFSMAN->m_bDelayedBack &&
			 INPUTFILTER->GetSecsHeld(input.DeviceI) >= 1.0F) ||
			!PREFSMAN->m_bDelayedBack || AllAreFailing())
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
	REPLAYS->InitReplayPlaybackForScore(REPLAYS->GetActiveReplayScore(),
										Player::GetTimingWindowScale());

	ScreenGameplay::SaveStats();
}

void
ScreenGameplayReplay::StageFinished(bool bBackedOut)
{
	Locator::getLogger()->info("Finishing Stage");
	if (bBackedOut) {
		GAMESTATE->CancelStage();
		return;
	}

	auto* const pss = m_vPlayerInfo.GetPlayerStageStats();
	// Makes sure all PlayerStageStats discrepancies are corrected forcibly.
	REPLAYS->RescoreReplay(*REPLAYS->GetActiveReplay(), pss);

	STATSMAN->m_CurStageStats.FinalizeScores();

	STATSMAN->m_vPlayedStageStats.push_back(STATSMAN->m_CurStageStats);

	STATSMAN->CalcAccumPlayedStageStats();
	GAMESTATE->FinishStage();
	Locator::getLogger()->info("Done Finishing Stage");
}

auto
ScreenGameplayReplay::SetRate(const float newRate) -> float
{
	const auto rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

	// Rates outside of this range may crash
	if (newRate < 0.3F || newRate > 5.F) {
		return rate;
	}

	const auto paused = GAMESTATE->GetPaused();

	// Stop the music and generate a new "music"
	m_pSoundMusic->Stop();

	RageTimer tm;
	const auto fSeconds = m_pSoundMusic->GetPositionSeconds(nullptr, &tm);

	float fSecondsToStartFadingOutMusic;
	float fSecondsToStartTransitioningOut;
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
	if (paused) {
		p.m_Volume = 0.F;
	}
	// Set up the music so we don't wait for an Etternaty when messing around
	// near the end of the song.
	if (fSecondsToStartFadingOutMusic < GAMESTATE->m_pCurSteps->lastsecond) {
		p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
		p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
							MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
	}
	p.StopMode = RageSoundParams::M_CONTINUE;

	// Go
	m_pSoundMusic->Play(false, &p);
	// But only for like 1 frame if we are paused
	if (paused) {
		m_pSoundMusic->Pause(true);
	}

	// misc info update
	GAMESTATE->m_Position.m_fMusicSeconds = fSeconds;
	UpdateSongPosition();
	MESSAGEMAN->Broadcast(
	  "CurrentRateChanged"); // Tell the theme we changed the rate

	return newRate;
}

void
ScreenGameplayReplay::SetSongPosition(float newPositionSeconds)
{
	// If you go too far negative, bad things may happen
	// But remember some files have notes at 0.0 seconds
	if (newPositionSeconds <= 0) {
		newPositionSeconds = 0.F;
	}
	SOUND->SetSoundPosition(m_pSoundMusic, newPositionSeconds);

	const auto paused = GAMESTATE->GetPaused();
	m_pSoundMusic->Pause(paused);

	m_vPlayerInfo.m_pPlayer->RenderAllNotesIgnoreScores();

	// Lightly reset some stats in case someone wants them
	// Precalculated values are put in their place.
	if (!paused) {
		// Current StageStats is going to get overwritten so let's clear it here
		STATSMAN->m_CurStageStats.m_player.InternalInit();
	}

	const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const auto rowNow = BeatToNoteRow(fSongBeat);
	// This breaks some oop standard in some book
	auto* pss = m_vPlayerInfo.GetPlayerStageStats();
	auto rs = REPLAYS->GetActiveReplay()->GetReplaySnapshotForNoterow(rowNow);
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
	const auto oldPause = GAMESTATE->GetPaused();
	// True if we are becoming paused
	const auto newPause = !GAMESTATE->GetPaused();
	RageTimer tm;

	const auto fSeconds = m_pSoundMusic->GetPositionSeconds(nullptr, &tm);
	const auto rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	Steps* pSteps = GAMESTATE->m_pCurSteps;
	const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const auto rowNow = BeatToNoteRow(fSongBeat);

	// We are leaving pause mode
	if (oldPause) {
		m_pSoundMusic->Stop();

		// Basically reinitialize all stage data from precalculated things
		// Restarts the basic replay data in case something went weird
		REPLAYS->InitReplayPlaybackForScore(
		  REPLAYS->GetActiveReplayScore(), Player::GetTimingWindowScale(), rowNow);
		SetupNoteDataFromRow(pSteps, rowNow);
		STATSMAN->m_CurStageStats.m_player.InternalInit();

		auto* pss = m_vPlayerInfo.GetPlayerStageStats();
		auto rs = REPLAYS->GetActiveReplay()->GetReplaySnapshotForNoterow(rowNow);
		FOREACH_ENUM(TapNoteScore, tns)
		{
			pss->m_iTapNoteScores[tns] = rs->judgments[tns];
		}
		FOREACH_ENUM(HoldNoteScore, hns)
		{
			pss->m_iHoldNoteScores[hns] = rs->hns[hns];
		}
		auto* ps = m_vPlayerInfo.GetPlayerState();
		m_vPlayerInfo.m_pPlayer->curwifescore = rs->curwifescore;
		m_vPlayerInfo.m_pPlayer->maxwifescore = rs->maxwifescore;

		// Reset the wife/judge counter related visible stuff
		FOREACH_ENUM(TapNoteScore, tns)
		{
			auto msg = Message("Judgment");
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
			msg.SetParam("Type", std::string("Tap"));
			msg.SetParam("Val", pss->m_iTapNoteScores[tns]);
			MESSAGEMAN->Broadcast(msg);
		}
		// We have to hackily only allow LetGo and Held through
		// because til death decided that it should be this way
		for (auto hns = HNS_LetGo; hns <= HNS_Held;
			 hns = static_cast<HoldNoteScore>(hns + 1)) {
			auto msg = Message("Judgment");
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
		float fSecondsToStartFadingOutMusic;
		float fSecondsToStartTransitioningOut;
		GetMusicEndTiming(fSecondsToStartFadingOutMusic,
						  fSecondsToStartTransitioningOut);

		RageSoundParams p;
		p.m_StartSecond = fSeconds - 0.25F;
		p.m_fSpeed = rate;
		if (fSecondsToStartFadingOutMusic <
			GAMESTATE->m_pCurSteps->lastsecond) {
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
		float fSecondsToStartFadingOutMusic;
		float fSecondsToStartTransitioningOut;
		GetMusicEndTiming(fSecondsToStartFadingOutMusic,
						  fSecondsToStartTransitioningOut);
		RageSoundParams p;
		p.m_fSpeed = rate;
		if (fSecondsToStartFadingOutMusic <
			GAMESTATE->m_pCurSteps->lastsecond) {
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
	static auto SetSongPosition(T* p, lua_State* L) -> int
	{
		const auto newpos = FArg(1);
		if (GAMESTATE->GetPaused()) {
			p->SetSongPosition(newpos);
		}
		return 0;
	}
	static auto SetRate(T* p, lua_State* L) -> int
	{
		const auto newrate = FArg(1);
		if (!GAMESTATE->GetPaused()) {
			lua_pushnumber(L, -1.F);
			return 1;
		}
		lua_pushnumber(L, p->SetRate(newrate));
		return 1;
	}
	static auto TogglePause(T* p, lua_State * /*L*/) -> int
	{
		p->TogglePause();
		return 0;
	}
	static auto SetBookmark(T* p, lua_State* L) -> int
	{
		const auto position = FArg(1);
		p->m_fReplayBookmarkSeconds = position;
		return 0;
	}
	static auto JumpToBookmark(T* p, lua_State * /*L*/) -> int
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
