#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Singletons/AnnouncerManager.h"
#include "Etterna/Models/Misc/CodeDetector.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/Grade.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
#include "ScreenEvaluation.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Globals/StepMania.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Models/Misc/PlayerAI.h"
#include "Etterna/Models/NoteData/NoteData.h"

#define CHEER_DELAY_SECONDS THEME->GetMetricF(m_sName, "CheerDelaySeconds")
#define BAR_ACTUAL_MAX_COMMAND THEME->GetMetricA(m_sName, "BarActualMaxCommand")

static ThemeMetric<TapNoteScore> g_MinScoreToMaintainCombo(
  "Gameplay",
  "MinScoreToMaintainCombo");
static const int NUM_SHOWN_RADAR_CATEGORIES = 5;

AutoScreenMessage(SM_PlayCheer);

REGISTER_SCREEN_CLASS(ScreenEvaluation);

ScreenEvaluation::ScreenEvaluation()
{
	m_pStageStats = NULL;
	m_bSavedScreenshot = false;
}

ScreenEvaluation::~ScreenEvaluation() {}

void
ScreenEvaluation::Init()
{
	LOG->Trace("ScreenEvaluation::Init()");

	// debugging
	// Only fill StageStats with fake info if we're the InitialScreen
	// (i.e. StageStats not already filled)
	if (PREFSMAN->m_sTestInitialScreen.Get() == m_sName) {
		PROFILEMAN->LoadFirstAvailableProfile(PLAYER_1);

		STATSMAN->m_vPlayedStageStats.clear();
		STATSMAN->m_vPlayedStageStats.push_back(StageStats());
		StageStats& ss = STATSMAN->m_vPlayedStageStats.back();

		GAMESTATE->m_PlayMode.Set(PLAY_MODE_REGULAR);
		GAMESTATE->SetCurrentStyle(
		  GAMEMAN->GameAndStringToStyle(GAMEMAN->GetDefaultGame(), "versus"),
		  PLAYER_INVALID);
		ss.m_playMode = GAMESTATE->m_PlayMode;
		ss.m_Stage = Stage_1st;
		enum_add(ss.m_Stage, random_up_to(3));
		GAMESTATE->SetMasterPlayerNumber(PLAYER_1);
		GAMESTATE->m_pCurSong.Set(SONGMAN->GetRandomSong());
		ss.m_vpPlayedSongs.push_back(GAMESTATE->m_pCurSong);
		ss.m_vpPossibleSongs.push_back(GAMESTATE->m_pCurSong);
		GAMESTATE->m_iCurrentStageIndex = 0;
		GAMESTATE->m_iPlayerStageTokens = 1;

		ss.m_player.m_pStyle = GAMESTATE->GetCurrentStyle(PLAYER_1);
		if (RandomInt(2))
			PO_GROUP_ASSIGN_N(GAMESTATE->m_pPlayerState->m_PlayerOptions,
							  ModsLevel_Stage,
							  m_bTransforms,
							  PlayerOptions::TRANSFORM_ECHO,
							  true); // show "disqualified"
		SO_GROUP_ASSIGN(
		  GAMESTATE->m_SongOptions, ModsLevel_Stage, m_fMusicRate, 1.1f);

		GAMESTATE->JoinPlayer(PLAYER_1);
		GAMESTATE->m_pCurSteps.Set(GAMESTATE->m_pCurSong->GetAllSteps()[0]);
		ss.m_player.m_vpPossibleSteps.push_back(GAMESTATE->m_pCurSteps);
		ss.m_player.m_iStepsPlayed = 1;

		PO_GROUP_ASSIGN(GAMESTATE->m_pPlayerState->m_PlayerOptions,
						ModsLevel_Stage,
						m_fScrollSpeed,
						2.0f);
		PO_GROUP_CALL(GAMESTATE->m_pPlayerState->m_PlayerOptions,
					  ModsLevel_Stage,
					  ChooseRandomModifiers);

		for (float f = 0; f < 100.0f; f += 1.0f) {
			float fP1 = fmodf(f / 100 * 4 + .3f, 1);
			ss.m_player.SetLifeRecordAt(fP1, f);
		}
		float fSeconds = GAMESTATE->m_pCurSong->GetStepsSeconds();
		ss.m_player.m_iActualDancePoints = RandomInt(3);
		ss.m_player.m_iPossibleDancePoints = 2;
		if (RandomInt(2))
			ss.m_player.m_iCurCombo = RandomInt(15000);
		else
			ss.m_player.m_iCurCombo = 0;
		ss.m_player.UpdateComboList(0, true);

		ss.m_player.m_iCurCombo += 50;
		ss.m_player.UpdateComboList(0.10f * fSeconds, false);

		ss.m_player.m_iCurCombo = 0;
		ss.m_player.UpdateComboList(0.15f * fSeconds, false);
		ss.m_player.m_iCurCombo = 1;
		ss.m_player.UpdateComboList(0.25f * fSeconds, false);
		ss.m_player.m_iCurCombo = 50;
		ss.m_player.UpdateComboList(0.35f * fSeconds, false);
		ss.m_player.m_iCurCombo = 0;
		ss.m_player.UpdateComboList(0.45f * fSeconds, false);
		ss.m_player.m_iCurCombo = 1;
		ss.m_player.UpdateComboList(0.50f * fSeconds, false);
		ss.m_player.m_iCurCombo = 100;
		ss.m_player.UpdateComboList(1.00f * fSeconds, false);
		if (RandomInt(5) == 0) {
			ss.m_player.m_bFailed = true;
		}
		ss.m_player.m_iTapNoteScores[TNS_W1] = RandomInt(3);
		ss.m_player.m_iTapNoteScores[TNS_W2] = RandomInt(3);
		ss.m_player.m_iTapNoteScores[TNS_W3] = RandomInt(3);
		ss.m_player.m_iPossibleGradePoints =
		  4 * ScoreKeeperNormal::TapNoteScoreToGradePoints(TNS_W1, false);
		ss.m_player.m_fLifeRemainingSeconds = randomf(90, 580);
		ss.m_player.m_iScore = random_up_to(900 * 1000 * 1000);
		ss.m_player.m_iPersonalHighScoreIndex = (random_up_to(3)) - 1;
		ss.m_player.m_iMachineHighScoreIndex = (random_up_to(3)) - 1;

		FOREACH_ENUM(RadarCategory, rc)
		{
			switch (rc) {
				case RadarCategory_TapsAndHolds:
				case RadarCategory_Jumps:
				case RadarCategory_Holds:
				case RadarCategory_Mines:
				case RadarCategory_Hands:
				case RadarCategory_Rolls:
				case RadarCategory_Lifts:
				case RadarCategory_Fakes:
					ss.m_player.m_radarPossible[rc] = 1 + (random_up_to(200));
					ss.m_player.m_radarActual[rc] = random_up_to(
					  static_cast<int>(ss.m_player.m_radarPossible[rc]));
					break;
				default:
					break;
			}

			; // filled in by ScreenGameplay on start of notes
		}
	}

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
		RString sComboType =
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
			RString sDir;
			RString sFileName;
			// To save a screenshot to your own profile you must hold shift
			// and press the button it saves compressed so you don't end up
			// with an inflated profile size
			// Otherwise, you can tap away at the screenshot button without
			// holding shift.
			if (bHoldingShift && PROFILEMAN->IsPersistentProfile(pn)) {
				if (!m_bSavedScreenshot) {
					Profile* pProfile = PROFILEMAN->GetProfile(pn);
					sDir = PROFILEMAN->GetProfileDir((ProfileSlot)pn) +
						   "Screenshots/";
					sFileName = StepMania::SaveScreenshot(
					  sDir, bHoldingShift, true, "", "");
					if (!sFileName.empty()) {
						RString sPath = sDir + sFileName;

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
				  StepMania::SaveScreenshot(sDir, bHoldingShift, true, "", "");
			}
			return true; // handled
		}
	}

	return ScreenWithMenuElements::Input(input);
}

void
ScreenEvaluation::HandleScreenMessage(const ScreenMessage SM)
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
		const RString mods = GAMEMAN->m_sModsToReset;
		GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = oldRate;
		GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = oldRate;
		GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = oldRate;
		GAMEMAN->m_bResetModifiers = false;

		const vector<RString> oldturns = GAMEMAN->m_vTurnsToReset;
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
	static int SetPlayerStageStatsFromReplayData(T* p, lua_State* L)
	{
		CHECKPOINT_M("Setting PSS from ReplayData via Lua");
		PlayerStageStats* pPSS = Luna<PlayerStageStats>::check(L, 1);
		NoteData nd = GAMESTATE->m_pCurSteps->GetNoteData();
		HighScore* hs = SCOREMAN->GetMostRecentScore();
		float ts = FArg(2);
		PlayerOptions potmp;
		potmp.FromString(hs->GetModifiers());
		if (hs->GetChordCohesion() || potmp.ContainsTransformOrTurn()) {
			lua_pushboolean(L, false);
			return 1;
		}
		PlayerAI::SetScoreData(hs, 0);
		PlayerAI::SetUpSnapshotMap(&nd, std::set<int>(), ts);
		PlayerAI::SetUpExactTapMap(GAMESTATE->m_pCurSteps->GetTimingData());
		pPSS->m_fLifeRecord.clear();
		pPSS->m_ComboList.clear();
		pPSS->m_fLifeRecord = PlayerAI::GenerateLifeRecordForReplay(ts);
		pPSS->m_ComboList = PlayerAI::GenerateComboListForReplay(ts);
		lua_pushboolean(L, true);
		return 1;
	}
	static int GetReplaySnapshotJudgmentsForNoterow(T* p, lua_State* L)
	{
		int row = IArg(1);
		auto rs = PlayerAI::GetReplaySnapshotForNoterow(row);
		vector<int> toPush;

		FOREACH_ENUM(TapNoteScore, tns)
		toPush.emplace_back(rs->judgments[tns]);

		LuaHelpers::CreateTableFromArray(toPush, L);
		return 1;
	}
	static int GetReplaySnapshotWifePercentForNoterow(T* p, lua_State* L)
	{
		int row = IArg(1);
		auto rs = PlayerAI::GetReplaySnapshotForNoterow(row);

		lua_pushnumber(L, rs->curwifescore / rs->maxwifescore);
		return 1;
	}
	static int GetReplayRate(T* p, lua_State* L)
	{
		CHECKPOINT_M("Getting replay rate");
		// if we have a replay, give the data
		if (PlayerAI::pScoreData != nullptr) {
			lua_pushnumber(L, PlayerAI::pScoreData->GetMusicRate());
			return 1;
		} else {
			// otherwise give nothing
			lua_pushnil(L);
			return 1;
		}
		CHECKPOINT_M("Got replay rate");
	}
	static int GetReplayJudge(T* p, lua_State* L)
	{
		CHECKPOINT_M("Getting replay judge");
		if (PlayerAI::pScoreData != nullptr) {
			lua_pushnumber(L, PlayerAI::pScoreData->GetJudgeScale());
		} else {
			lua_pushnumber(L, Player::GetTimingWindowScale());
		}
		CHECKPOINT_M("Got replay judge");
		return 1;
	}
	static int GetReplayModifiers(T* p, lua_State* L)
	{
		CHECKPOINT_M("Getting replay modifiers");
		if (PlayerAI::pScoreData != nullptr) {
			LuaHelpers::Push(L, PlayerAI::pScoreData->GetModifiers());
		} else {
			lua_pushnil(L);
		}
		CHECKPOINT_M("Got replay modifiers");
		return 1;
	}
	static int ScoreUsedInvalidModifier(T* p, lua_State* L)
	{
		CHECKPOINT_M("Checking for invalid modifiers on Highscore via Lua");
		HighScore* hs = SCOREMAN->GetMostRecentScore();
		CHECKPOINT_M("Getting Player Options from HighScore...");
		PlayerOptions potmp;
		potmp.FromString(hs->GetModifiers());
		CHECKPOINT_M("Checking modifiers...");
		lua_pushboolean(L, potmp.ContainsTransformOrTurn());
		CHECKPOINT_M("Done checking.");
		return 1;
	}

	LunaScreenEvaluation()
	{
		ADD_METHOD(GetStageStats);
		ADD_METHOD(SetPlayerStageStatsFromReplayData);
		ADD_METHOD(GetReplaySnapshotJudgmentsForNoterow);
		ADD_METHOD(GetReplaySnapshotWifePercentForNoterow);
		ADD_METHOD(GetReplayRate);
		ADD_METHOD(GetReplayJudge);
		ADD_METHOD(ScoreUsedInvalidModifier);
		ADD_METHOD(GetReplayModifiers);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenEvaluation, ScreenWithMenuElements)

// lua end
