#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/AnnouncerManager.h"
#include "Etterna/Models/Misc/CodeDetector.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
#include "ScreenEvaluation.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Globals/StepMania.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Singletons/ReplayManager.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Globals/rngthing.h"

#define CHEER_DELAY_SECONDS THEME->GetMetricF(m_sName, "CheerDelaySeconds")
#define BAR_ACTUAL_MAX_COMMAND THEME->GetMetricA(m_sName, "BarActualMaxCommand")

static ThemeMetric<TapNoteScore> g_MinScoreToMaintainCombo(
  "Gameplay",
  "MinScoreToMaintainCombo");

AutoScreenMessage(SM_PlayCheer);

REGISTER_SCREEN_CLASS(ScreenEvaluation);

ScreenEvaluation::ScreenEvaluation()
{
	m_pStageStats = nullptr;
	m_bSavedScreenshot = false;
}

ScreenEvaluation::~ScreenEvaluation() = default;

void
ScreenEvaluation::Init()
{
	Locator::getLogger()->debug("ScreenEvaluation::Init()");

	if (STATSMAN->m_vPlayedStageStats.empty()) {
		LuaHelpers::ReportScriptError("PlayerStageStats is empty!  Do not use "
									  "SM_GoToNextScreen on ScreenGameplay, "
									  "use SM_DoNextScreen instead so that "
									  "ScreenGameplay can clean up properly.");
		STATSMAN->m_vPlayedStageStats.push_back(STATSMAN->m_CurStageStats);
	}
	m_pStageStats = &STATSMAN->m_vPlayedStageStats.back();

	m_bSavedScreenshot = false;

	// update persistent statistics
	if (GamePreferences::m_AutoPlay == PC_REPLAY) {
		m_pStageStats->m_player.m_HighScore.SetRadarValues(
		  m_pStageStats->m_player.m_radarActual);
	}

	// Run this here, so STATSMAN->m_CurStageStats is available to overlays.
	ScreenWithMenuElements::Init();

	// load sounds
	m_soundStart.Load(THEME->GetPathS(m_sName, "start"));

	// init records area
	bool bOneHasNewTopRecord = false;
	bool bOneHasFullW1Combo = false;
	bool bOneHasFullW2Combo = false;
	bool bOneHasFullW3Combo = false;
	bool bOneHasFullW4Combo = false;
	if (GAMESTATE->IsPlayerEnabled(PLAYER_1)) {
		if ((m_pStageStats->m_player.m_iMachineHighScoreIndex == 0 ||
			 m_pStageStats->m_player.m_iPersonalHighScoreIndex == 0)) {
			bOneHasNewTopRecord = true;
		}

		if (m_pStageStats->m_player.FullComboOfScore(TNS_W4))
			bOneHasFullW4Combo = true;

		if (m_pStageStats->m_player.FullComboOfScore(TNS_W3))
			bOneHasFullW3Combo = true;

		if (m_pStageStats->m_player.FullComboOfScore(TNS_W2))
			bOneHasFullW2Combo = true;

		if (m_pStageStats->m_player.FullComboOfScore(TNS_W1))
			bOneHasFullW1Combo = true;
	}

	if (bOneHasNewTopRecord &&
		ANNOUNCER->HasSoundsFor("evaluation new record")) {
		SOUND->PlayOnceFromDir(ANNOUNCER->GetPathTo("evaluation new record"));
	} else if (bOneHasFullW4Combo && g_MinScoreToMaintainCombo == TNS_W4) {
		SOUND->PlayOnceFromDir(
		  ANNOUNCER->GetPathTo("evaluation full combo W4"));
	} else if ((bOneHasFullW1Combo || bOneHasFullW2Combo ||
				bOneHasFullW3Combo)) {
		std::string sComboType =
		  bOneHasFullW1Combo ? "W1" : (bOneHasFullW2Combo ? "W2" : "W3");
		SOUND->PlayOnceFromDir(
		  ANNOUNCER->GetPathTo("evaluation full combo " + sComboType));
	}
}

bool
ScreenEvaluation::Input(const InputEventPlus& input)
{
	if (IsTransitioning())
		return false;

	// restart gameplay should work
	// but only if not online and not in a replay
	if (input.DeviceI.device == DEVICE_KEYBOARD &&
		input.MenuI == GAME_BUTTON_RESTART) {
		if (input.type != IET_FIRST_PRESS)
			return false;

		// this was probably a replay, dont allow
		if (!m_pStageStats->m_bLivePlay)
			return false;

		// not in netplay
		if (m_sName.find("Net") != std::string::npos)
			return false;

		// technically this is correct
		GAMESTATE->m_bRestartedGameplay = false;

		// go
		SetPrevScreenName("ScreenStageInformation");
		HandleScreenMessage(SM_GoToPrevScreen);
	}
			
	if (input.GameI.IsValid()) {
		if (CodeDetector::EnteredCode(input.GameI.controller,
									  CODE_SAVE_SCREENSHOT1) ||
			CodeDetector::EnteredCode(input.GameI.controller,
									  CODE_SAVE_SCREENSHOT2)) {
			PlayerNumber pn = input.pn;
			bool bHoldingShift = (INPUTFILTER->IsBeingPressed(
									DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)) ||
								  INPUTFILTER->IsBeingPressed(
									DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT)));
			std::string sDir;
			std::string sFileName;
			// To save a screenshot to your own profile you must hold shift
			// and press the button it saves compressed so you don't end up
			// with an inflated profile size
			// Otherwise, you can tap away at the screenshot button without
			// holding shift.
			if (bHoldingShift) {
				if (!m_bSavedScreenshot) {
					Profile* pProfile = PROFILEMAN->GetProfile(pn);
					sDir = PROFILEMAN->GetProfileDir((ProfileSlot)pn) +
						   "Screenshots/";
					sFileName =
					  StepMania::SaveScreenshot(sDir, bHoldingShift, "", "");
					if (!sFileName.empty()) {
						std::string sPath = sDir + sFileName;

						const HighScore& hs =
						  m_pStageStats->m_player.m_HighScore;
						Screenshot screenshot;
						screenshot.sFileName = sFileName;
						screenshot.sMD5 =
						  BinaryToHex(CRYPTMAN->GetMD5ForFile(sPath));
						screenshot.highScore = hs;
						pProfile->AddScreenshot(screenshot);
					}
					m_bSavedScreenshot = true;
				}
			} else {
				sDir = "Screenshots/";
				sFileName =
				  StepMania::SaveScreenshot(sDir, bHoldingShift, "", "");
			}
			return true; // handled
		}
	}

	return ScreenWithMenuElements::Input(input);
}

void
ScreenEvaluation::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_PlayCheer) {
		SOUND->PlayOnceFromDir(ANNOUNCER->GetPathTo("evaluation cheer"));
	}

	ScreenWithMenuElements::HandleScreenMessage(SM);
}

bool
ScreenEvaluation::MenuBack(const InputEventPlus& input)
{
	return MenuStart(input);
}

bool
ScreenEvaluation::MenuStart(const InputEventPlus& input)
{
	if (IsTransitioning())
		return false;

	m_soundStart.Play(true);

	HandleMenuStart();
	return true;
}

void
ScreenEvaluation::HandleMenuStart()
{
	StepsID stepsid;
	stepsid.FromSteps(GAMESTATE->m_pCurSteps);
	SongID songid;
	songid.FromSong(GAMESTATE->m_pCurSong);

	// Reset mods
	if (GAMEMAN->m_bResetModifiers) {
		float oldRate = GAMEMAN->m_fPreviousRate;
		const std::string mods = GAMEMAN->m_sModsToReset;
		GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = oldRate;
		GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = oldRate;
		GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = oldRate;
		GAMEMAN->m_bResetModifiers = false;

		const std::vector<std::string> oldturns = GAMEMAN->m_vTurnsToReset;
		if (GAMEMAN->m_bResetTurns) {
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong()
			  .ResetModsToStringVector(oldturns);
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
			  .ResetModsToStringVector(oldturns);
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred()
			  .ResetModsToStringVector(oldturns);
			GAMEMAN->m_bResetTurns = false;
			GAMEMAN->m_vTurnsToReset.clear();
		}
		GAMEMAN->m_sModsToReset = "";
		MESSAGEMAN->Broadcast("RateChanged");
	}
	StartTransitioningScreen(SM_GoToNextScreen);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ScreenEvaluation. */
class LunaScreenEvaluation : public Luna<ScreenEvaluation>
{
  public:
	static int GetStageStats(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->GetStageStats());
		return 1;
	}
	static int RescoreReplay(T* p, lua_State* L)
	{
		Locator::getLogger()->info("Rescoring ReplayData via Lua");
		PlayerStageStats* pPSS = Luna<PlayerStageStats>::check(L, 1);

		// allow either a highscore or nothing, which defaults to most recent
		HighScore* hs;
		if (lua_isnoneornil(L, 3))
			hs = SCOREMAN->GetMostRecentScore();
		else
			hs = Luna<HighScore>::check(L, 3);
		
		float ts = FArg(2);
		auto replay = REPLAYS->GetReplay(hs);

		PlayerOptions potmp;
		potmp.FromString(hs->GetModifiers());
		if (hs->GetChordCohesion() || !replay->CanSafelyTransformNoteData()) {
			Locator::getLogger()->info(
			  "The replay could not be rescored because of RNG mods or CC on");
			lua_pushboolean(L, false);
			return 1;
		}

		bool useReprioritizedNoterows = false;
		if (!lua_isnoneornil(L, 4)) {
			useReprioritizedNoterows = BArg(4);
		}

		replay->SetUseReprioritizedNoteRows(useReprioritizedNoterows);
		REPLAYS->EnableCustomScoringFunctions();
		REPLAYS->InitReplayPlaybackForScore(hs, ts);
		REPLAYS->RescoreReplay(*replay, pPSS, ts);
		REPLAYS->ReleaseReplay(replay); // remove extra reference
		REPLAYS->DisableCustomScoringFunctions();
		lua_pushboolean(L, true);
		return 1;
	}
	static int GetReplayRate(T* p, lua_State* L)
	{
		Locator::getLogger()->debug("Getting replay rate");
		// if we have a replay, give the data
		if (REPLAYS->GetActiveReplayScore() != nullptr) {
			lua_pushnumber(L, REPLAYS->GetActiveReplayScore()->GetMusicRate());
			return 1;
		} else {
			// otherwise give nothing
			lua_pushnil(L);
			return 1;
		}
	}
	static int GetReplayJudge(T* p, lua_State* L)
	{
		Locator::getLogger()->debug("Getting replay judge");
		if (REPLAYS->GetActiveReplayScore() != nullptr) {
			lua_pushnumber(L, REPLAYS->GetActiveReplayScore()->GetJudgeScale());
		} else {
			lua_pushnumber(L, Player::GetTimingWindowScale());
		}
		return 1;
	}
	static int GetReplayModifiers(T* p, lua_State* L)
	{
		Locator::getLogger()->debug("Getting replay modifiers");
		if (REPLAYS->GetActiveReplayScore() != nullptr) {
			LuaHelpers::Push(L,
							 REPLAYS->GetActiveReplayScore()->GetModifiers());
		} else {
			lua_pushnil(L);
		}
		return 1;
	}
	static int ScoreUsedInvalidModifier(T* p, lua_State* L)
	{
		Locator::getLogger()->debug("Checking for invalid modifiers on Highscore via Lua");
		HighScore* hs = SCOREMAN->GetMostRecentScore();
		if (hs == nullptr) {
			Locator::getLogger()->warn("MOST RECENT SCORE WAS EMPTY.");
			lua_pushboolean(L, true);
			return 1;
		}
		Locator::getLogger()->debug("Getting Player Options from HighScore...");
		PlayerOptions potmp;
		potmp.FromString(hs->GetModifiers());
		Locator::getLogger()->debug("Checking modifiers...");
		lua_pushboolean(L, potmp.ContainsTransformOrTurn());
		Locator::getLogger()->debug("Done checking.");
		return 1;
	}

	LunaScreenEvaluation()
	{
		ADD_METHOD(GetStageStats);
		ADD_METHOD(RescoreReplay);
		ADD_METHOD(GetReplayRate);
		ADD_METHOD(GetReplayJudge);
		ADD_METHOD(ScoreUsedInvalidModifier);
		ADD_METHOD(GetReplayModifiers);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenEvaluation, ScreenWithMenuElements)

// lua end
