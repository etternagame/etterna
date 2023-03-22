#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/AdjustSync.h"
#include "ArrowEffects.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/Misc/GameCommand.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/InputMapper.h"
#include "LifeMeter.h"
#include "Etterna/Singletons/MessageManager.h"
#include "Etterna/Singletons/NetworkSyncManager.h" //used for sending timing offset
#include "Etterna/Models/NoteData/NoteDataUtil.h"
#include "Etterna/Models/NoteData/NoteDataWithScoring.h"
#include "NoteField.h"
#include "Player.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Models/Misc/Profile.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Misc/StageStats.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "HoldJudgment.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Globals/rngthing.h"
#include "Etterna/Globals/GameLoop.h"

#include <algorithm>
using std::max;
using std::min;

void
TimingWindowSecondsInit(size_t /*TimingWindow*/ i,
						std::string& sNameOut,
						float& defaultValueOut);

void
TimingWindowSecondsInit(size_t /*TimingWindow*/ i,
						std::string& sNameOut,
						float& defaultValueOut)
{
	sNameOut = "TimingWindowSeconds" +
			   TimingWindowToString(static_cast<TimingWindow>(i));
	switch (i) {
		case TW_W1:
			defaultValueOut = 0.0225F;
			break;
		case TW_W2:
			defaultValueOut = 0.045F;
			break;
		case TW_W3:
			defaultValueOut = 0.090F;
			break;
		case TW_W4:
			defaultValueOut = 0.135F;
			break;
		case TW_W5:
			defaultValueOut = 0.180F;
			break;
		case TW_Mine:
			// ~same as j5 great, the explanation for this is quite long but
			// the general idea is that mines are more punishing now so we
			// can give a little back
			defaultValueOut = MINE_WINDOW_SEC;
			break;
		case TW_Hold:
			// allow enough time to take foot off and put back on
			defaultValueOut = 0.250F;
			break;
		case TW_Roll:
			defaultValueOut = 0.500F;
			break;
		case TW_Checkpoint:
		 	// similar to TW_Hold, but a little more strict
			defaultValueOut = 0.1664F;
			break;
		default:
			FAIL_M(ssprintf("Invalid timing window: %i", static_cast<int>(i)));
	}
}

static Preference<float> m_fTimingWindowScale("TimingWindowScale", 1.0F);
static Preference1D<float> m_fTimingWindowSeconds(TimingWindowSecondsInit,
												  NUM_TimingWindow);
static Preference<bool> g_bEnableMineSoundPlayback("EnableMineHitSound", true);

// moved out of being members of player.h
static ThemeMetric<float> GRAY_ARROWS_Y_STANDARD;
static ThemeMetric<float> GRAY_ARROWS_Y_REVERSE;
static ThemeMetric<float> HOLD_JUDGMENT_Y_STANDARD;
static ThemeMetric<float> HOLD_JUDGMENT_Y_REVERSE;
static ThemeMetric<int> BRIGHT_GHOST_COMBO_THRESHOLD;
static ThemeMetric<bool> TAP_JUDGMENTS_UNDER_FIELD;
static ThemeMetric<bool> HOLD_JUDGMENTS_UNDER_FIELD;
static ThemeMetric<bool> COMBO_UNDER_FIELD;
static ThemeMetric<int> DRAW_DISTANCE_AFTER_TARGET_PIXELS;
static ThemeMetric<int> DRAW_DISTANCE_BEFORE_TARGET_PIXELS;
static ThemeMetric<bool> ROLL_BODY_INCREMENTS_COMBO;
static ThemeMetric<bool> COMBO_BREAK_ON_IMMEDIATE_HOLD_LET_GO;

ThemeMetric<bool> PENALIZE_TAP_SCORE_NONE("Player", "PenalizeTapScoreNone");
ThemeMetric<bool> CHECKPOINTS_FLASH_ON_HOLD(
  "Player",
  "CheckpointsFlashOnHold"); // sm-ssc addition
ThemeMetric<bool> IMMEDIATE_HOLD_LET_GO("Player", "ImmediateHoldLetGo");
/**
 * @brief Must a Player step on a hold head for a hold to activate?
 *
 * If set to true, the Player must step on a hold head in order for the hold to
 * activate. If set to false, merely holding your foot down as the hold head
 * approaches will suffice. */
ThemeMetric<bool> REQUIRE_STEP_ON_HOLD_HEADS("Player",
											 "RequireStepOnHoldHeads");

/**
 * @brief Does not stepping on a mine increase the combo?
 *
 * If set to true, every mine missed will increment the combo.
 * If set to false, missing a mine will not affect the combo. */
ThemeMetric<bool> AVOID_MINE_INCREMENTS_COMBO("Gameplay",
											  "AvoidMineIncrementsCombo");
/**
 * @brief Does stepping on a mine increment the miss combo?
 *
 * If set to true, every mine stepped on will break the combo and increment the
 * miss combo. If set to false, stepping on a mine will not affect the combo. */
ThemeMetric<bool> MINE_HIT_INCREMENTS_MISS_COMBO("Gameplay",
												 "MineHitIncrementsMissCombo");
/**
 * @brief Are checkpoints and taps considered separate judgments?
 *
 * If set to true, they are considered separate.
 * If set to false, they are considered the same. */
ThemeMetric<bool> CHECKPOINTS_TAPS_SEPARATE_JUDGMENT(
  "Player",
  "CheckpointsTapsSeparateJudgment");
/**
 * @brief Do we score missed holds and rolls with HoldNoteScores?
 *
 * If set to true, missed holds and rolls are given LetGo judgments.
 * If set to false, missed holds and rolls are given no judgment on the hold
 * side of things. */
ThemeMetric<bool> SCORE_MISSED_HOLDS_AND_ROLLS("Player",
											   "ScoreMissedHoldsAndRolls");
/** @brief How much of the song must have gone by before a Player's combo is
 * colored? */
ThemeMetric<float> PERCENT_UNTIL_COLOR_COMBO("Player",
											 "PercentUntilColorCombo");
/** @brief How much combo must be earned before the announcer says "Combo
 * Stopped"? */
ThemeMetric<int> COMBO_STOPPED_AT("Player", "ComboStoppedAt");

/**
 * @brief What is our highest cap for mMods?
 *
 * If set to 0 or less, assume the song takes over. */
ThemeMetric<float> M_MOD_HIGH_CAP("Player", "MModHighCap");

inline void
JudgedRows::Resize(size_t iMin)
{
	const auto iNewSize = std::max(2 * m_vRows.size(), iMin);
	std::vector<bool> vNewRows(m_vRows.begin() + m_iOffset, m_vRows.end());
	vNewRows.reserve(iNewSize);
	vNewRows.insert(
	  vNewRows.end(), m_vRows.begin(), m_vRows.begin() + m_iOffset);
	vNewRows.resize(iNewSize, false);
	m_vRows.swap(vNewRows);
	m_iOffset = 0;
}

auto
Player::GetWindowSeconds(TimingWindow tw) -> float
{
	switch (tw) {
		// mines should have a static hit window across all judges to be
		// logically consistent with the idea that increasing judge should
		// not make any elementof the game easier, so now they do
		case TW_Mine:
			return MINE_WINDOW_SEC;
		case TW_Hold:
			return 0.25F * GetTimingWindowScale();
		case TW_Roll:
			return 0.5F * GetTimingWindowScale();
		default:
			break;
	}

	float fSecs = m_fTimingWindowSeconds[tw];
	fSecs *= GetTimingWindowScale();
	fSecs = std::clamp(fSecs, 0.F, MISS_WINDOW_BEGIN_SEC);
	return fSecs;
}

auto
Player::GetWindowSecondsCustomScale(TimingWindow tw, float timingScale) -> float
{
	switch (tw) {
		// mines should have a static hit window across all judges to be
		// logically consistent with the idea that increasing judge should
		// not make any elementof the game easier, so now they do
		case TW_Mine:
			return MINE_WINDOW_SEC;
		case TW_Hold:
			return 0.25F * timingScale;
		case TW_Roll:
			return 0.5F * timingScale;
		default:
			break;
	}

	float fSecs = m_fTimingWindowSeconds[tw];
	fSecs *= timingScale;
	fSecs = std::clamp(fSecs, 0.F, MISS_WINDOW_BEGIN_SEC);
	return fSecs;
}

auto
Player::GetTimingWindowScale() -> float
{
	return std::clamp(m_fTimingWindowScale.Get(), 0.001F, 1.F);
}

Player::Player(NoteData& nd, bool bVisibleParts)
  : m_NoteData(nd)
{
	m_drawing_notefield_board = false;
	m_bLoaded = false;
	m_inside_lua_set_life = false;

	m_pPlayerState = nullptr;
	m_pPlayerStageStats = nullptr;
	m_fNoteFieldHeight = 0;

	m_pLifeMeter = nullptr;
	m_pPrimaryScoreKeeper = nullptr;
	m_pIterNeedsTapJudging = nullptr;
	m_pIterNeedsHoldJudging = nullptr;
	m_pIterUncrossedRows = nullptr;
	m_pIterUnjudgedRows = nullptr;
	m_pIterUnjudgedMineRows = nullptr;

	totalwifescore = 0;
	m_Timing = nullptr;
	m_LastTapNoteScore = TNS_None;
	m_iFirstUncrossedRow = -1;
	m_iLastSeenCombo = 0;
	m_bSeenComboYet = false;
	m_bTickHolds = false;

	m_bPaused = false;
	m_bDelay = false;

	m_pNoteField = nullptr;
	if (bVisibleParts) {
		m_pNoteField = new NoteField;
		m_pNoteField->SetName("NoteField");
	}
	m_pJudgedRows = new JudgedRows;

	m_bSendJudgmentAndComboMessages = true;
}

Player::~Player()
{
	SAFE_DELETE(m_pNoteField);
	for (unsigned i = 0; i < m_vpHoldJudgment.size(); ++i) {
		SAFE_DELETE(m_vpHoldJudgment[i]);
	}
	SAFE_DELETE(m_pJudgedRows);
	SAFE_DELETE(m_pIterNeedsTapJudging);
	SAFE_DELETE(m_pIterNeedsHoldJudging);
	SAFE_DELETE(m_pIterUncrossedRows);
	SAFE_DELETE(m_pIterUnjudgedRows);
	SAFE_DELETE(m_pIterUnjudgedMineRows);
}

/* Init() does the expensive stuff: load sounds and noteskins.  Load() just
 * loads a NoteData. */
void
Player::Init(const std::string& sType,
			 PlayerState* pPlayerState,
			 PlayerStageStats* pPlayerStageStats,
			 LifeMeter* pLM,
			 ScoreKeeper* pPrimaryScoreKeeper)
{

	GRAY_ARROWS_Y_STANDARD.Load(sType, "ReceptorArrowsYStandard");
	GRAY_ARROWS_Y_REVERSE.Load(sType, "ReceptorArrowsYReverse");
	HOLD_JUDGMENT_Y_STANDARD.Load(sType, "HoldJudgmentYStandard");
	HOLD_JUDGMENT_Y_REVERSE.Load(sType, "HoldJudgmentYReverse");
	BRIGHT_GHOST_COMBO_THRESHOLD.Load(sType, "BrightGhostComboThreshold");

	TAP_JUDGMENTS_UNDER_FIELD.Load(sType, "TapJudgmentsUnderField");
	HOLD_JUDGMENTS_UNDER_FIELD.Load(sType, "HoldJudgmentsUnderField");
	COMBO_UNDER_FIELD.Load(sType, "ComboUnderField");
	DRAW_DISTANCE_AFTER_TARGET_PIXELS.Load(sType,
										   "DrawDistanceAfterTargetsPixels");
	DRAW_DISTANCE_BEFORE_TARGET_PIXELS.Load(sType,
											"DrawDistanceBeforeTargetsPixels");
	ROLL_BODY_INCREMENTS_COMBO.Load("Player", "RollBodyIncrementsCombo");
	COMBO_BREAK_ON_IMMEDIATE_HOLD_LET_GO.Load("Player",
											  "ComboBreakOnImmediateHoldLetGo");

	this->SortByDrawOrder();

	m_pPlayerState = pPlayerState;
	m_pPlayerStageStats = pPlayerStageStats;
	m_pLifeMeter = pLM;
	m_pPrimaryScoreKeeper = pPrimaryScoreKeeper;

	m_iLastSeenCombo = 0;
	m_bSeenComboYet = false;

	// set initial life
	if ((m_pLifeMeter != nullptr) && (m_pPlayerStageStats != nullptr)) {
		const auto fLife = m_pLifeMeter->GetLife();
		m_pPlayerStageStats->SetLifeRecordAt(
		  fLife,
		  GAMESTATE->m_Position.m_fMusicSeconds /
			GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate);
		// m_pPlayerStageStats->SetWifeRecordAt( 1.f,
		// STATSMAN->m_CurStageStats.m_fStepsSeconds);
	}

	// TODO(Sam): Remove use of PlayerNumber.
	const auto pn = m_pPlayerState->m_PlayerNumber;

	m_pPlayerState->SetNumCols(
	  GAMESTATE->GetCurrentStyle(pn)->m_iColsPerPlayer);

	RageSoundLoadParams SoundParams;
	SoundParams.m_bSupportPan = true;
	m_soundMine.Load(THEME->GetPathS(sType, "mine"), true, &SoundParams);

	// calculate M-mod speed here, so we can adjust properly on a per-song
	// basis.
	// XXX: can we find a better location for this?
	// Always calculate the reading bpm, to allow switching to an mmod mid-song.
	{
		DisplayBpms bpms;

		ASSERT(GAMESTATE->m_pCurSong != nullptr);
		GAMESTATE->m_pCurSong->GetDisplayBpms(bpms);

		float fMaxBPM = 0;

		/* TODO: Find a way to not go above a certain BPM range
		 * for getting the max BPM. Otherwise, you get songs
		 * like Tsuhsuixamush, M550, 0.18x speed. Even slow
		 * speed readers would not generally find this fun.
		 * -Wolfman2000
		 */

		// all BPMs are listed and available, so try them first.
		// get the maximum listed value for the song
		// if the BPMs are < 0, reset and get the actual values.
		if (!bpms.IsSecret()) {
			fMaxBPM = (M_MOD_HIGH_CAP > 0 ? bpms.GetMaxWithin(M_MOD_HIGH_CAP)
										  : bpms.GetMax());
			fMaxBPM = max(0.F, fMaxBPM);
		}

		// we can't rely on the displayed BPMs, so manually calculate.
		if (fMaxBPM == 0) {
			float fThrowAway = 0;

			if (M_MOD_HIGH_CAP > 0) {
				GAMESTATE->m_pCurSong->m_SongTiming.GetActualBPM(
				  fThrowAway, fMaxBPM, M_MOD_HIGH_CAP);
			} else {
				GAMESTATE->m_pCurSong->m_SongTiming.GetActualBPM(fThrowAway,
																 fMaxBPM);
			}
		}

		ASSERT(fMaxBPM > 0);
		m_pPlayerState->m_fReadBPM = fMaxBPM;
	}

	const auto fBalance = GameSoundManager::GetPlayerBalance(pn);
	m_soundMine.SetProperty("Pan", fBalance);

	if (HasVisibleParts()) {
		LuaThreadVariable var(
		  "Player", LuaReference::Create(m_pPlayerState->m_PlayerNumber));
		LuaThreadVariable var2("MultiPlayer",
							   LuaReference::Create(m_pPlayerState->m_mp));

		m_sprCombo.Load(THEME->GetPathG(sType, "combo"));
		m_sprCombo->SetName("Combo");
		this->AddChild(m_sprCombo);

		// todo: allow for judgments to be loaded per-column a la pop'n?
		// see how HoldJudgments are handled below for an example, though
		// it would need more work. -aj
		m_sprJudgment.Load(THEME->GetPathG(sType, "judgment"));
		m_sprJudgment->SetName("Judgment");
		this->AddChild(m_sprJudgment);
	}

	// Load HoldJudgments
	m_vpHoldJudgment.resize(
	  GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
		->m_iColsPerPlayer);
	lastHoldHeadsSeconds.resize(
	  GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
		->m_iColsPerPlayer);
	for (auto i = 0;
		 i < GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
			   ->m_iColsPerPlayer;
		 ++i) {
		m_vpHoldJudgment[i] = nullptr;
		// set this reasonably negative because if we don't, the first row of
		// the song doesn't get judged
		// and also it gets changed back to a realistic number after a hold is
		// hit -poco
		lastHoldHeadsSeconds[i] = -1000.F;
	}

	if (HasVisibleParts()) {
		for (auto i = 0;
			 i < GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
				   ->m_iColsPerPlayer;
			 ++i) {
			auto* pJudgment = new HoldJudgment;
			// xxx: assumes sprite; todo: don't force 1x2 -aj
			pJudgment->Load(THEME->GetPathG("HoldJudgment", "label 1x2"));
			m_vpHoldJudgment[i] = pJudgment;
			this->AddChild(m_vpHoldJudgment[i]);
		}
	}

	m_fNoteFieldHeight = GRAY_ARROWS_Y_REVERSE - GRAY_ARROWS_Y_STANDARD;
	if (m_pNoteField != nullptr) {
		m_pNoteField->Init(m_pPlayerState, m_fNoteFieldHeight);
		ActorUtil::LoadAllCommands(*m_pNoteField, sType);
		this->AddChild(m_pNoteField);
	}
}
/**
 * @brief Determine if a TapNote needs a tap note style judgment.
 * @param tn the TapNote in question.
 * @return true if it does, false otherwise. */
auto
Player::NeedsTapJudging(const TapNote& tn) -> bool
{
	switch (tn.type) {
		DEFAULT_FAIL(tn.type);
		case TapNoteType_Tap:
		case TapNoteType_HoldHead:
		case TapNoteType_Mine:
		case TapNoteType_Lift:
			return tn.result.tns == TNS_None;
		case TapNoteType_HoldTail:
		case TapNoteType_AutoKeysound:
		case TapNoteType_Fake:
		case TapNoteType_Empty:
			return false;
	}
}

/**
 * @brief Determine if a TapNote needs a hold note style judgment.
 * @param tn the TapNote in question.
 * @return true if it does, false otherwise. */
auto
Player::NeedsHoldJudging(const TapNote& tn) -> bool
{
	switch (tn.type) {
		DEFAULT_FAIL(tn.type);
		case TapNoteType_HoldHead:
			return tn.HoldResult.hns == HNS_None;
		case TapNoteType_Tap:
		case TapNoteType_HoldTail:
		case TapNoteType_Mine:
		case TapNoteType_Lift:
		case TapNoteType_AutoKeysound:
		case TapNoteType_Fake:
		case TapNoteType_Empty:
			return false;
	}
}

static TapNoteScore
GetAutoplayTapNoteScore(const PlayerState* pPlayerState)
{
	if (pPlayerState->m_PlayerController == PC_REPLAY)
		return TNS_Miss;
	if (pPlayerState->m_PlayerController == PC_AUTOPLAY ||
		pPlayerState->m_PlayerController == PC_CPU)
		return TNS_W1;

	return TNS_Miss;
}

void
Player::Load()
{
	m_bLoaded = true;

	// Figured this is probably a little expensive so let's cache it
	m_bTickHolds = GAMESTATE->GetCurrentGame()->m_bTickHolds;

	m_LastTapNoteScore = TNS_None;
	// The editor can start playing in the middle of the song.
	const auto iNoteRow = BeatToNoteRow(GAMESTATE->m_Position.m_fSongBeat);
	m_iFirstUncrossedRow = iNoteRow - 1;
	m_pJudgedRows->Reset(iNoteRow);

	// Make sure c++ bound actor's tweens are reset if they exist
	if (m_sprJudgment != nullptr) {
		m_sprJudgment->PlayCommand("Reset");
	}
	if (m_pPlayerStageStats != nullptr) {
		SetCombo(
		  m_pPlayerStageStats->m_iCurCombo,
		  m_pPlayerStageStats
			->m_iCurMissCombo); // combo can persist between songs and games
	}

	// Mina garbage - Mina
	m_Timing = GAMESTATE->m_pCurSteps->GetTimingData();
	m_Timing->NegStopAndBPMCheck();
	const auto lastRow = m_NoteData.GetLastRow();
	m_Timing->BuildAndGetEtar(lastRow);

	totalwifescore = m_NoteData.WifeTotalScoreCalc(m_Timing, 0, 1073741824);
	curwifescore = 0.F;
	maxwifescore = 0.F;

	m_NoteData.LogNonEmptyRows(m_Timing);
	nerv = m_NoteData.GetNonEmptyRowVector();
	const auto& etaner = m_Timing->BuildAndGetEtaner(nerv);
	if (m_pPlayerStageStats != nullptr) {
		m_pPlayerStageStats->serializednd =
		  m_NoteData.SerializeNoteData(etaner);
	}
	m_NoteData.UnsetSerializedNoteData();

	if (m_pPlayerStageStats != nullptr) {
		// if we can ensure that files that have fakes or warps no longer
		// inflate file rating, we can actually lift this restriction, look into
		// it for 0.70 calc release, related: we can look at solo upload stuff
		// as well
		if (m_Timing->HasWarps() || m_Timing->HasFakes()) {
			m_pPlayerStageStats->filehadnegbpms = true;
		}

		// check before nomines transform
		if (GAMESTATE->m_pCurSteps->GetRadarValues()[RadarCategory_Mines] > 0) {
			m_pPlayerStageStats->filegotmines = true;
		}

		if (GAMESTATE->m_pCurSteps->GetRadarValues()[RadarCategory_Holds] > 0 ||
			GAMESTATE->m_pCurSteps->GetRadarValues()[RadarCategory_Rolls] > 0) {
			m_pPlayerStageStats->filegotholds = true;
		}

		// check for lua script load (technically this is redundant a little
		// with negbpm but whatever) -mina
		if (!m_Timing->ValidSequentialAssumption) {
			m_pPlayerStageStats->luascriptwasloaded = true;
		}
	}

	const HighScore* pb = SCOREMAN->GetChartPBAt(
	  GAMESTATE->m_pCurSteps->GetChartKey(),
	  GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate);
	
	// the latter condition checks for Grade_Failed, NUM_Grade, Grade_Invalid
	if (pb == nullptr || pb->GetGrade() >= Grade_Failed) {
		wifescorepersonalbest = m_pPlayerState->playertargetgoal;
	} else {
		wifescorepersonalbest = pb->GetWifeScore();
	}

	if (m_pPlayerStageStats != nullptr) {
		m_pPlayerStageStats->m_fTimingScale = GetTimingWindowScale();
	}

	/* Apply transforms. */
	NoteDataUtil::TransformNoteData(
	  m_NoteData,
	  *m_Timing,
	  m_pPlayerState->m_PlayerOptions.GetStage(),
	  GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
		->m_StepsType);

	// Generate some cache data structure.
	m_pPlayerState->ResetCacheInfo(/*m_NoteData*/);

	const int iDrawDistanceAfterTargetsPixels =
	  DRAW_DISTANCE_AFTER_TARGET_PIXELS;
	const int iDrawDistanceBeforeTargetsPixels =
	  DRAW_DISTANCE_BEFORE_TARGET_PIXELS;

	const auto fNoteFieldMiddle =
	  (GRAY_ARROWS_Y_STANDARD + GRAY_ARROWS_Y_REVERSE) / 2;

	if (m_pNoteField != nullptr) {
		m_pNoteField->SetY(fNoteFieldMiddle);
		m_pNoteField->Load(&m_NoteData,
						   iDrawDistanceAfterTargetsPixels,
						   iDrawDistanceBeforeTargetsPixels);
	}

	// Load keysounds.  If sounds are already loaded (as in the editor), don't
	// reload them.
	// XXX: the editor will load several duplicate copies (in each NoteField),
	// and each player will load duplicate sounds.  Does this belong somewhere
	// else (perhaps in a separate object, used alongside
	// ScreenGameplay::m_pSoundMusic and ScreenEdit::m_pSoundMusic?) We don't
	// have to load separate copies to set player fade: always make a copy, and
	// set the fade on the copy.
	const Song* pSong = GAMESTATE->m_pCurSong;
	const auto sSongDir = pSong->GetSongDir();
	m_vKeysounds.resize(pSong->m_vsKeysoundFile.size());

	// parameters are invalid somehow... -aj
	RageSoundLoadParams SoundParams;
	SoundParams.m_bSupportPan = true;

	const auto fBalance = GameSoundManager::GetPlayerBalance(PLAYER_1);
	for (unsigned i = 0; i < m_vKeysounds.size(); i++) {
		auto sKeysoundFilePath = sSongDir + pSong->m_vsKeysoundFile[i];
		auto& sound = m_vKeysounds[i];
		if (sound.GetLoadedFilePath() != sKeysoundFilePath) {
			sound.Load(sKeysoundFilePath, true, &SoundParams);
		}
		sound.SetProperty("Pan", fBalance);
		sound.SetStopModeFromString("stop");
	}

	if (m_pPlayerStageStats != nullptr) {
		SendComboMessages(m_pPlayerStageStats->m_iCurCombo,
						  m_pPlayerStageStats->m_iCurMissCombo);
	}

	SAFE_DELETE(m_pIterNeedsTapJudging);
	m_pIterNeedsTapJudging = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterNeedsHoldJudging);
	m_pIterNeedsHoldJudging = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterUncrossedRows);
	m_pIterUncrossedRows = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterUnjudgedRows);
	m_pIterUnjudgedRows = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterUnjudgedMineRows);
	m_pIterUnjudgedMineRows = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW));
}

void
Player::Reload()
{
	// This basically does most of Player::Load
	// but not all of it

	m_LastTapNoteScore = TNS_None;

	const auto iNoteRow = BeatToNoteRow(GAMESTATE->m_Position.m_fSongBeat);
	m_iFirstUncrossedRow = iNoteRow - 1;
	m_pJudgedRows->Reset(iNoteRow);

	// Make sure c++ bound actor's tweens are reset if they exist
	if (m_sprJudgment != nullptr) {
		m_sprJudgment->PlayCommand("Reset");
	}
	if (m_pPlayerStageStats != nullptr) {
		SetCombo(
		  m_pPlayerStageStats->m_iCurCombo,
		  m_pPlayerStageStats
			->m_iCurMissCombo); // combo can persist between songs and games
	}

	curwifescore = 0.F;
	maxwifescore = 0.F;

	if (m_pPlayerStageStats != nullptr) {
		SendComboMessages(m_pPlayerStageStats->m_iCurCombo,
						  m_pPlayerStageStats->m_iCurMissCombo);
	}

	/* Apply transforms. */
	NoteDataUtil::TransformNoteData(
	  m_NoteData,
	  *m_Timing,
	  m_pPlayerState->m_PlayerOptions.GetStage(),
	  GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
		->m_StepsType);

	SAFE_DELETE(m_pIterNeedsTapJudging);
	m_pIterNeedsTapJudging = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterNeedsHoldJudging);
	m_pIterNeedsHoldJudging = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterUncrossedRows);
	m_pIterUncrossedRows = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterUnjudgedRows);
	m_pIterUnjudgedRows = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterUnjudgedMineRows);
	m_pIterUnjudgedMineRows = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW));
}

void
Player::SendComboMessages(unsigned int iOldCombo,
						  unsigned int iOldMissCombo) const
{
	const auto iCurCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	if (iOldCombo > static_cast<unsigned int>(COMBO_STOPPED_AT) &&
		iCurCombo < static_cast<unsigned int>(COMBO_STOPPED_AT)) {
		SCREENMAN->PostMessageToTopScreen(SM_ComboStopped, 0);
	}

	if (m_bSendJudgmentAndComboMessages) {
		Message msg("ComboChanged");
		msg.SetParam("OldCombo", iOldCombo);
		msg.SetParam("OldMissCombo", iOldMissCombo);
		if (m_pPlayerState != nullptr) {
			msg.SetParam("Player", m_pPlayerState->m_PlayerNumber);
			msg.SetParam("PlayerState",
						 LuaReference::CreateFromPush(*m_pPlayerState));
		}
		if (m_pPlayerStageStats != nullptr) {
			msg.SetParam("PlayerStageStats",
						 LuaReference::CreateFromPush(*m_pPlayerStageStats));
		}
		MESSAGEMAN->Broadcast(msg);
	}
}

void
Player::UpdateVisibleParts()
{
	// Optimization: Don't spend time processing the things below that won't
	// show if the Player doesn't show anything on the screen.
	if (!HasVisibleParts()) {
		return;
	}

	const auto fMiniPercent = m_pPlayerState->m_PlayerOptions.GetCurrent()
								.m_fEffects[PlayerOptions::EFFECT_MINI];
	const auto fTinyPercent = m_pPlayerState->m_PlayerOptions.GetCurrent()
								.m_fEffects[PlayerOptions::EFFECT_TINY];
	const auto fJudgmentZoom =
	  min(powf(0.5F, fMiniPercent + fTinyPercent), 1.0F);

	// Update Y positions
	{
		for (auto c = 0;
			 c < GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
				   ->m_iColsPerPlayer;
			 c++) {
			const auto fPercentReverse =
			  m_pPlayerState->m_PlayerOptions.GetCurrent()
				.GetReversePercentForColumn(c);
			const auto fHoldJudgeYPos = SCALE(fPercentReverse,
											  0.F,
											  1.F,
											  HOLD_JUDGMENT_Y_STANDARD,
											  HOLD_JUDGMENT_Y_REVERSE);
			// float fGrayYPos = SCALE( fPercentReverse, 0.f, 1.f,
			// GRAY_ARROWS_Y_STANDARD, GRAY_ARROWS_Y_REVERSE );

			auto fX = ArrowEffects::GetXPos(m_pPlayerState, c, 0);
			const auto fZ = ArrowEffects::GetZPos(c, 0);
			fX *= (1 - fMiniPercent * 0.5F);

			m_vpHoldJudgment[c]->SetX(fX);
			m_vpHoldJudgment[c]->SetY(fHoldJudgeYPos);
			m_vpHoldJudgment[c]->SetZ(fZ);
			m_vpHoldJudgment[c]->SetZoom(fJudgmentZoom);
		}
	}

	// NoteField accounts for reverse on its own now.
	// if( m_pNoteField )
	//	m_pNoteField->SetY( fGrayYPos );

	const auto fNoteFieldZoom = 1 - fMiniPercent * 0.5F;
	if (m_pNoteField != nullptr) {
		m_pNoteField->SetZoom(fNoteFieldZoom);
	}
}

void
Player::UpdatePressedFlags()
{
	const auto iNumCols =
	  GAMESTATE->GetCurrentStyle(PLAYER_1)->m_iColsPerPlayer;
	ASSERT_M(iNumCols <= MAX_COLS_PER_PLAYER,
			 ssprintf("%i > %i", iNumCols, MAX_COLS_PER_PLAYER));
	for (auto col = 0; col < iNumCols; ++col) {
		ASSERT(m_pPlayerState != nullptr);

		// TODO(Sam): Remove use of PlayerNumber.
		std::vector<GameInput> GameI;
		GAMESTATE->GetCurrentStyle(PLAYER_1)->StyleInputToGameInput(col, GameI);

		const auto bIsHoldingButton = INPUTMAPPER->IsBeingPressed(GameI);

		// TODO(Sam): Make this work for non-human-controlled players
		if (bIsHoldingButton &&
			m_pPlayerState->m_PlayerController == PC_HUMAN) {
			if (m_pNoteField != nullptr) {
				m_pNoteField->SetPressed(col);
			}
		}
	}
}

void
Player::UpdateHoldsAndRolls(float fDeltaTime,
							const std::chrono::steady_clock::time_point& now)
{
	const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const auto iSongRow = BeatToNoteRow(fSongBeat);
	// handle Autoplay for rolls
	if (m_pPlayerState->m_PlayerController != PC_HUMAN) {
		for (auto iTrack = 0; iTrack < m_NoteData.GetNumTracks(); ++iTrack) {
			int iHeadRow;
			if (!m_NoteData.IsHoldNoteAtRow(iTrack, iSongRow, &iHeadRow)) {
				iHeadRow = iSongRow;
			}

			const auto& tn = m_NoteData.GetTapNote(iTrack, iHeadRow);
			if (tn.type != TapNoteType_HoldHead ||
				tn.subType != TapNoteSubType_Roll) {
				continue;
			}
			if (tn.HoldResult.hns != HNS_None) {
				continue;
			}
			if (tn.HoldResult.fLife >= 0.5F) {
				continue;
			}

			Step(iTrack, iHeadRow, now, true, false); // bHeld really doesnt
													  // make a difference for
													  // autoplay and replay
			if (m_pPlayerState->m_PlayerController == PC_AUTOPLAY) {
				STATSMAN->m_CurStageStats.m_bUsedAutoplay = true;
				if (m_pPlayerStageStats != nullptr) {
					m_pPlayerStageStats->m_bDisqualified = true;
					m_pPlayerStageStats->everusedautoplay = true;
				}
			}
		}
	}

	// update HoldNotes logic
	{

		// Fast forward to the first that needs hold judging.
		{
			auto& iter = *m_pIterNeedsHoldJudging;
			while (!iter.IsAtEnd() && iter.Row() <= iSongRow &&
				   !NeedsHoldJudging(*iter)) {
				++iter;
			}
		}

		std::vector<TrackRowTapNote> vHoldNotesToGradeTogether;
		auto iter = *m_pIterNeedsHoldJudging; // copy
		for (; !iter.IsAtEnd() && iter.Row() <= iSongRow; ++iter) {
			auto& tn = *iter;
			if (tn.type != TapNoteType_HoldHead) {
				continue;
			}

			const auto iTrack = iter.Track();
			const auto iRow = iter.Row();
			TrackRowTapNote trtn = { iTrack, iRow, &tn };

			// Set last hold head seconds stuff
			// This is used to avoid accidentally tapping taps from within
			// holds.
			lastHoldHeadsSeconds[iTrack] =
			  max(lastHoldHeadsSeconds[iTrack],
				  m_Timing->WhereUAtBro(NoteRowToBeat(iRow + tn.iDuration)));

			/* All holds must be of the same subType because fLife is handled
			 * in different ways depending on the SubType. Handle Rolls one at
			 * a time and don't mix with holds. */
			switch (tn.subType) {
				DEFAULT_FAIL(tn.subType);
				case TapNoteSubType_Hold:
					break;
				case TapNoteSubType_Roll: {
					std::vector<TrackRowTapNote> v;
					v.push_back(trtn);
					UpdateHoldNotes(iSongRow, fDeltaTime, v);
				}
					continue; // don't process this below
			}

			if (!vHoldNotesToGradeTogether.empty()) {
				UpdateHoldNotes(
				  iSongRow, fDeltaTime, vHoldNotesToGradeTogether);
				vHoldNotesToGradeTogether.clear();
			}
			vHoldNotesToGradeTogether.push_back(trtn);
		}

		if (!vHoldNotesToGradeTogether.empty()) {
			UpdateHoldNotes(iSongRow, fDeltaTime, vHoldNotesToGradeTogether);
			vHoldNotesToGradeTogether.clear();
		}
	}
}

void
Player::UpdateCrossedRows(const std::chrono::steady_clock::time_point& now)
{
	const auto iRowNow = BeatToNoteRow(GAMESTATE->m_Position.m_fSongBeat);
	if (iRowNow >= 0) {
		if (GAMESTATE->IsPlayerEnabled(m_pPlayerState)) {
			if (GAMESTATE->m_Position.m_bDelay) {
				if (!m_bDelay) {
					m_bDelay = true;
				}
			} else {
				if (m_bDelay) {
					if (m_pPlayerState->m_PlayerController != PC_HUMAN) {
						CrossedRows(iRowNow - 1, now);
					}
					m_bDelay = false;
				}
				CrossedRows(iRowNow, now);
			}
		}
	}
}

void
Player::Update(float fDeltaTime)
{
	// const RageTimer now;
	const auto now = std::chrono::steady_clock::now();
	// Don't update if we haven't been loaded yet.
	if (!m_bLoaded) {
		return;
	}

	// LOG->Trace( "Player::Update(%f)", fDeltaTime );

	if (GAMESTATE->m_pCurSong == nullptr) {
		return;
	}

	ActorFrame::Update(fDeltaTime);

	if (m_pPlayerState->m_mp != MultiPlayer_Invalid) {
		/* In multiplayer, it takes too long to run player updates for every
		 * player each frame; with 32 players and three difficulties, we have 96
		 * Players to update.  Stagger these updates, by only updating a few
		 * players each update; since we don't have screen elements tightly tied
		 * to user actions in this mode, this doesn't degrade gameplay.  Run 4
		 * players per update, which means 12 Players in 3-difficulty mode.
		 */
		static auto iCycle = 0;
		iCycle = (iCycle + 1) % 8;

		if ((m_pPlayerState->m_mp % 8) != iCycle) {
			return;
		}
	}

	ArrowEffects::SetCurrentOptions(
	  &m_pPlayerState->m_PlayerOptions.GetCurrent());

	// Tell the NoteField and other visible C++ Actors to update
	UpdateVisibleParts();

	// If we're paused, don't update tap or hold note logic, so hold notes can
	// be released during pause.
	if (m_bPaused) {
		return;
	}

	// Tell the NoteField we pressed (or didnt press) certain columns
	UpdatePressedFlags();

	// Tell Rolls to update (if in Autoplay)
	// Tell Holds to update (lose life)
	UpdateHoldsAndRolls(fDeltaTime, now);

	// A lot of logic involving rows that have been passed
	UpdateCrossedRows(now);

	// Check for completely judged rows.
	UpdateJudgedRows(fDeltaTime);
	UpdateTapNotesMissedOlderThan(GetMaxStepDistanceSeconds());
}

// Update a group of holds with shared scoring/life. All of these holds will
// have the same start row.
void
Player::UpdateHoldNotes(int iSongRow,
						float fDeltaTime,
						std::vector<TrackRowTapNote>& vTN)
{
	ASSERT(!vTN.empty());

	// LOG->Trace("--------------------------------");
	/*
	LOG->Trace("[Player::UpdateHoldNotes] begins");
	LOG->Trace( ssprintf("song row %i, deltaTime = %f",iSongRow,fDeltaTime) );
	*/

	// dont let gameloop updaterate changes break hold note life.
	// in fact, make it harsh to halt abusers:
	// instant kill holds if the update rate is 0
	auto rate = GameLoop::GetUpdateRate();
	if (rate > 0)
		fDeltaTime /= rate;
	else
		fDeltaTime = 9999;

	const auto iStartRow = vTN[0].iRow;
	auto iMaxEndRow = INT_MIN;
	auto iFirstTrackWithMaxEndRow = -1;

	auto subType = TapNoteSubType_Invalid;
	for (auto& trtn : vTN) {
		const auto iTrack = trtn.iTrack;
		ASSERT(iStartRow == trtn.iRow);
		const auto iEndRow = iStartRow + trtn.pTN->iDuration;
		if (subType == TapNoteSubType_Invalid) {
			subType = trtn.pTN->subType;
		}

		if (iEndRow > iMaxEndRow) {
			iMaxEndRow = iEndRow;
			iFirstTrackWithMaxEndRow = iTrack;
		}
	}

	ASSERT(iFirstTrackWithMaxEndRow != -1);

	for (auto& trtn : vTN) {
		// set hold flags so NoteField can do intelligent drawing
		trtn.pTN->HoldResult.bHeld = false;
		trtn.pTN->HoldResult.bActive = false;

		const auto iRow = trtn.iRow;

		// If the song beat is in the range of this hold:
		if (iRow <= iSongRow && iRow <= iMaxEndRow) {
			trtn.pTN->HoldResult.fOverlappedTime += fDeltaTime;
		} else {
			trtn.pTN->HoldResult.fOverlappedTime = 0;
		}
	}

	auto hns = vTN[0].pTN->HoldResult.hns;
	auto fLife = vTN[0].pTN->HoldResult.fLife;

	if (hns != HNS_None) // if this HoldNote already has a result
	{
		return; // we don't need to update the logic for this group
	}

	auto bSteppedOnHead = true;
	auto bHeadJudged = true;
	for (auto& trtn : vTN) {
		const auto& tns = trtn.pTN->result.tns;

		// TODO(Sam): When using JUDGE_HOLD_NOTES_ON_SAME_ROW_TOGETHER, require
		// that the whole row of taps was hit before activating this group
		// of holds.
		/* Something about the logic in this section is causing 192nd steps
		 * to fail for some odd reason. -aj */
		// Nah, lets just forget about judging all holds/taps at once like
		// that because we are in the cc off era now :)
		bSteppedOnHead &=
		  (tns != TNS_Miss &&
		   tns != TNS_None); // did they step on the start of this hold?
		bHeadJudged &=
		  (tns != TNS_None); // has this hold really even started yet?
	}

	bool bInitiatedNote;
	if (REQUIRE_STEP_ON_HOLD_HEADS) {
		// XXX HACK: Miniholds (a 64th or 192nd length hold) will not always
		// register as Held, even if you hit the note. This is considered a
		// major roadblock to adoption, so until a proper fix is found,
		// DON'T REMOVE THIS HACK! -aj
		/*if( iMaxEndRow-iStartRow <= 4 )
			bInitiatedNote = true;
		else*/
		bInitiatedNote = bSteppedOnHead;
	} else {
		bInitiatedNote = true;
		bHeadJudged = true;
	}

	auto bIsHoldingButton = true;
	for (auto& trtn : vTN) {
		/*if this hold is already done, pretend it's always being pressed.
		fixes/masks the phantom hold issue. -FSX*/
		// That interacts badly with !IMMEDIATE_HOLD_LET_GO,
		// causing ALL holds to be judged HNS_Held whether they were or not.
		if (!IMMEDIATE_HOLD_LET_GO ||
			(iStartRow + trtn.pTN->iDuration) > iSongRow) {
			const auto iTrack = trtn.iTrack;

			if (m_pPlayerState->m_PlayerController != PC_HUMAN) {
				// TODO(Sam): Make the CPU miss sometimes.
				if (m_pPlayerState->m_PlayerController == PC_AUTOPLAY) {
					STATSMAN->m_CurStageStats.m_bUsedAutoplay = true;
					if (m_pPlayerStageStats != nullptr) {
						m_pPlayerStageStats->m_bDisqualified = true;
						m_pPlayerStageStats->everusedautoplay = true;
					}
				}
			} else {
				std::vector<GameInput> GameI;
				GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
				  ->StyleInputToGameInput(iTrack, GameI);

				bIsHoldingButton &=
				  INPUTMAPPER->IsBeingPressed(GameI, m_pPlayerState->m_mp);
			}
		}
	}

	if (bInitiatedNote && fLife != 0 && bHeadJudged) {
		/* This hold note is not judged and we stepped on its head.
		 * Update iLastHeldRow. Do this even if we're a little beyond the
		 * end of the hold note, to make sure iLastHeldRow is clamped to
		 * iEndRow if the hold note is held all the way. */
		for (auto& trtn : vTN) {
			auto iEndRow = iStartRow + trtn.pTN->iDuration;

			trtn.pTN->HoldResult.iLastHeldRow = min(iSongRow, iEndRow);
		}
	}

	// If the song beat is in the range of this hold:
	if (iStartRow <= iSongRow && iStartRow <= iMaxEndRow && bHeadJudged) {
		switch (subType) {
			case TapNoteSubType_Hold:
				for (auto& trtn : vTN) {
					// set hold flag so NoteField can do intelligent drawing
					trtn.pTN->HoldResult.bHeld =
					  bIsHoldingButton && bInitiatedNote;
					trtn.pTN->HoldResult.bActive = bInitiatedNote;
				}

				if (bInitiatedNote && bIsHoldingButton) {
					fLife = 1;
				} else {
					const auto window = m_bTickHolds ? TW_Checkpoint : TW_Hold;
					fLife -= fDeltaTime / GetWindowSeconds(window);
					fLife = max(0.F, fLife);
				}
				break;
			case TapNoteSubType_Roll:
				for (auto& trtn : vTN) {

					trtn.pTN->HoldResult.bHeld = true;
					trtn.pTN->HoldResult.bActive = bInitiatedNote;
				}

				// Decrease life
				// Also clamp the roll decay window to the accepted "Judge
				// 7" value for it. -poco
				fLife -= fDeltaTime / max(GetWindowSeconds(TW_Roll), 0.25F);
				fLife = max(fLife, 0.F); // clamp life
				break;
			/*
			case TapNoteSubType_Mine:
				break;
			*/
			default:
				FAIL_M(ssprintf("Invalid tap note subtype: %i", subType));
		}
	}

	// TODO(Sam): Cap the active time passed to the score keeper to the actual
	// start time and end time of the hold.
	if (vTN[0].pTN->HoldResult.bActive) {
		const auto fSecondsActiveSinceLastUpdate =
		  fDeltaTime * GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		if (m_pPrimaryScoreKeeper != nullptr) {
			m_pPrimaryScoreKeeper->HandleHoldActiveSeconds(
			  fSecondsActiveSinceLastUpdate);
		}
	}

	// check for LetGo. If the head was missed completely, don't count an
	// LetGo.
	/* Why? If you never step on the head, then it will be left as HNS_None,
	 * which doesn't seem correct. */
	if (IMMEDIATE_HOLD_LET_GO) {
		if (bInitiatedNote && fLife == 0 && bHeadJudged) // the player has not
														 // pressed the button
														 // for a long time!
		{
			hns = HNS_LetGo;
		}
	}

	// score hold notes that have passed
	if (iSongRow >= iMaxEndRow && bHeadJudged) {
		auto bLetGoOfHoldNote = false;

		/* Score rolls that end with fLife == 0 as LetGo, even if
		 * m_bTickHolds is on. Rolls don't have iCheckpointsMissed set, so,
		 * unless we check Life == 0, rolls would always be scored as Held.
		 */
		bool bAllowHoldCheckpoints;
		switch (subType) {
			DEFAULT_FAIL(subType);
			case TapNoteSubType_Hold:
				bAllowHoldCheckpoints = true;
				break;
			case TapNoteSubType_Roll:
				bAllowHoldCheckpoints = false;
				break;
		}

		if (m_bTickHolds && bAllowHoldCheckpoints) {
			// LOG->Trace("(hold checkpoints are allowed and enabled.)");
			auto iCheckpointsHit = 0;
			auto iCheckpointsMissed = 0;
			for (auto& v : vTN) {
				iCheckpointsHit += v.pTN->HoldResult.iCheckpointsHit;
				iCheckpointsMissed += v.pTN->HoldResult.iCheckpointsMissed;
			}
			bLetGoOfHoldNote = iCheckpointsMissed > 0 || iCheckpointsHit == 0;

			// TRICKY: If the hold is so short that it has no checkpoints,
			// then mark it as Held if the head was stepped on.
			if (iCheckpointsHit == 0 && iCheckpointsMissed == 0) {
				bLetGoOfHoldNote = !bSteppedOnHead;
			}
		} else {
			bLetGoOfHoldNote = fLife == 0;
		}

		if (bInitiatedNote) {
			if (!bLetGoOfHoldNote) {
				fLife = 1;
				hns = HNS_Held;
				const auto bBright =
				  (m_pPlayerStageStats != nullptr) &&
				  m_pPlayerStageStats->m_iCurCombo >
					static_cast<unsigned int>(BRIGHT_GHOST_COMBO_THRESHOLD);
				if (m_pNoteField != nullptr) {
					for (auto& trtn : vTN) {
						const auto iTrack = trtn.iTrack;
						m_pNoteField->DidHoldNote(
						  iTrack, HNS_Held, bBright); // bright ghost flash
					}
				}
			}

			else {
				// LOG->Trace("initiated note and let go :(");
			}
		} else if (SCORE_MISSED_HOLDS_AND_ROLLS) {
			hns = HNS_LetGo;
		} else {
			hns = HNS_Missed;
		}
	}

	const auto fLifeFraction = fLife / 1; // haha im just gonna leave this here

	// aren't keysounds broken anyway?
	for (auto& trtn : vTN) {
		trtn.pTN->HoldResult.fLife = fLife;
		trtn.pTN->HoldResult.hns = hns;
		// Stop the playing keysound for the hold note.
		// I think this causes crashes too. -aj
		// This can still crash. I think it expects a full game and quit
		// before the preference works: otherwise, it causes problems on
		// holds. At least, that hapened on my Mac. -wolfman2000

		static auto* pVolume =
		  Preference<float>::GetPreferenceByName("SoundVolume");
		if (pVolume != nullptr) {
			static auto fVol = pVolume->Get();

			if (trtn.pTN->iKeysoundIndex >= 0 &&
				trtn.pTN->iKeysoundIndex <
				  static_cast<int>(m_vKeysounds.size())) {
				auto factor = (trtn.pTN->subType == TapNoteSubType_Roll
								 ? 2.0F * fLifeFraction
								 : 10.0F * fLifeFraction - 8.5F);
				m_vKeysounds[trtn.pTN->iKeysoundIndex].SetProperty(
				  "Volume", max(0.0F, min(1.0F, factor)) * fVol);
			}
		}
	}

	if ((hns == HNS_LetGo) && COMBO_BREAK_ON_IMMEDIATE_HOLD_LET_GO) {
		IncrementMissCombo();
	}

	if (hns != HNS_None) {
		auto& tn = *vTN[0].pTN;
		SetHoldJudgment(tn, iFirstTrackWithMaxEndRow, iSongRow);
		HandleHoldScore(tn);
	}
}

void
Player::DrawPrimitives()
{
	if (m_pPlayerState == nullptr) {
		return;
	}

	const auto draw_notefield = (m_pNoteField != nullptr);

	const auto& curr_options = m_pPlayerState->m_PlayerOptions.GetCurrent();
	const auto tilt = curr_options.m_fPerspectiveTilt;
	const auto skew = curr_options.m_fSkew;
	const auto mini = curr_options.m_fEffects[PlayerOptions::EFFECT_MINI];
	const auto center_y =
	  GetY() + (GRAY_ARROWS_Y_STANDARD + GRAY_ARROWS_Y_REVERSE) / 2;
	const auto reverse = curr_options.GetReversePercentForColumn(0) > .5;

	if (m_drawing_notefield_board) {
		// Ask the Notefield to draw its board primitive before everything
		// else so that things drawn under the field aren't behind the
		// opaque board. -Kyz
		if (draw_notefield) {
			PlayerNoteFieldPositioner poser(
			  this, GetX(), tilt, skew, mini, center_y, reverse);
			m_pNoteField->DrawBoardPrimitive();
		}
		return;
	}

	// Draw these below everything else.
	if (COMBO_UNDER_FIELD && curr_options.m_fBlind == 0) {
		if (m_sprCombo != nullptr) {
			m_sprCombo->Draw();
		}
	}

	if (TAP_JUDGMENTS_UNDER_FIELD) {
		DrawTapJudgments();
	}

	if (HOLD_JUDGMENTS_UNDER_FIELD) {
		DrawHoldJudgments();
	}

	if (draw_notefield) {
		PlayerNoteFieldPositioner poser(
		  this, GetX(), tilt, skew, mini, center_y, reverse);
		m_pNoteField->Draw();
	}

	// m_pNoteField->m_sprBoard->GetVisible()
	if (!COMBO_UNDER_FIELD && curr_options.m_fBlind == 0) {
		if (m_sprCombo != nullptr) {
			m_sprCombo->Draw();
		}
	}

	if (!static_cast<bool>(TAP_JUDGMENTS_UNDER_FIELD)) {
		DrawTapJudgments();
	}

	if (!static_cast<bool>(HOLD_JUDGMENTS_UNDER_FIELD)) {
		DrawHoldJudgments();
	}
}

void
Player::PushPlayerMatrix(float x, float skew, float center_y)
{
	DISPLAY->CameraPushMatrix();
	DISPLAY->PushMatrix();
	DISPLAY->LoadMenuPerspective(45,
								 SCREEN_WIDTH,
								 SCREEN_HEIGHT,
								 SCALE(skew, 0.1F, 1.0F, x, SCREEN_CENTER_X),
								 center_y);
}

void
Player::PopPlayerMatrix()
{
	DISPLAY->CameraPopMatrix();
	DISPLAY->PopMatrix();
}

void
Player::DrawNoteFieldBoard()
{
	m_drawing_notefield_board = true;
	Draw();
	m_drawing_notefield_board = false;
}

Player::PlayerNoteFieldPositioner::PlayerNoteFieldPositioner(Player* p,
															 float x,
															 float tilt,
															 float skew,
															 float mini,
															 float center_y,
															 bool reverse)
  : player(p)
{
	player->PushPlayerMatrix(x, skew, center_y);
	const auto reverse_mult = (reverse ? -1 : 1);
	original_y = player->m_pNoteField->GetY();
	const auto tilt_degrees = SCALE(tilt, -1.F, +1.F, +30, -30) * reverse_mult;
	auto zoom = SCALE(mini, 0.F, 1.F, 1.F, .5F);
	// Something strange going on here.  Notice that the range for tilt's
	// effect on y_offset goes to -45 when positive, but -20 when negative.
	// I don't know why it's done this why, simply preserving old behavior.
	// -Kyz
	if (tilt > 0) {
		zoom *= SCALE(tilt, 0.F, 1.F, 1.F, 0.9F);
		y_offset = SCALE(tilt, 0.F, 1.F, 0.F, -45.F) * reverse_mult;
	} else {
		zoom *= SCALE(tilt, 0.F, -1.F, 1.F, 0.9F);
		y_offset = SCALE(tilt, 0.F, -1.F, 0.F, -20.F) * reverse_mult;
	}
	player->m_pNoteField->SetY(original_y + y_offset);
	player->m_pNoteField->SetZoom(zoom);
	player->m_pNoteField->SetRotationX(tilt_degrees);
}

Player::PlayerNoteFieldPositioner::~PlayerNoteFieldPositioner()
{
	player->m_pNoteField->SetY(original_y);
	player->PopPlayerMatrix();
}

void
Player::DrawTapJudgments()
{
	if (m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind > 0) {
		return;
	}

	if (m_sprJudgment != nullptr) {
		m_sprJudgment->Draw();
	}
}

void
Player::DrawHoldJudgments()
{
	if (m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind > 0) {
		return;
	}

	for (auto c = 0; c < m_NoteData.GetNumTracks(); c++) {
		if (m_vpHoldJudgment[c] != nullptr) {
			m_vpHoldJudgment[c]->Draw();
		}
	}
}

void
Player::ChangeLife(TapNoteScore tns) const
{
	if (m_pLifeMeter != nullptr) {
		m_pLifeMeter->ChangeLife(tns);
	}

	ChangeLifeRecord();
}

void
Player::ChangeLife(HoldNoteScore hns, TapNoteScore tns) const
{
	if (m_pLifeMeter != nullptr) {
		m_pLifeMeter->ChangeLife(hns, tns);
	}

	ChangeLifeRecord();
}

void
Player::ChangeLife(float delta) const
{
	// If ChangeLifeRecord is not called before the change, then the life
	// graph will show a gradual change from the time of the previous step
	// (or change) to the time of this change, instead of the sharp change
	// that actually occurred. -Kyz
	ChangeLifeRecord();
	if (m_pLifeMeter != nullptr) {
		m_pLifeMeter->ChangeLife(delta);
	}
	ChangeLifeRecord();
}

void
Player::SetLife(float value) const
{
	// If ChangeLifeRecord is not called before the change, then the life
	// graph will show a gradual change from the time of the previous step
	// (or change) to the time of this change, instead of the sharp change
	// that actually occurred. -Kyz
	ChangeLifeRecord();
	if (m_pLifeMeter != nullptr) {
		m_pLifeMeter->SetLife(value);
	}
	ChangeLifeRecord();
}

void
Player::ChangeLifeRecord() const
{
	float fLife = -1;
	if (m_pLifeMeter != nullptr) {
		fLife = m_pLifeMeter->GetLife();
	}
	if (fLife != -1) {
		if (m_pPlayerStageStats != nullptr) {
			m_pPlayerStageStats->SetLifeRecordAt(
			  fLife,
			  GAMESTATE->m_Position.m_fMusicSeconds /
				GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate);
		}
	}
}

void
Player::ChangeWifeRecord() const
{
	// Sets the life ... to the wife....
	// That's not right.
	if (m_pPlayerStageStats != nullptr) {
		m_pPlayerStageStats->SetLifeRecordAt(
		  curwifescore / maxwifescore,
		  GAMESTATE->m_Position.m_fMusicSeconds /
			GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate);
	}
}

auto
Player::GetClosestNoteDirectional(int col,
								  int iStartRow,
								  int iEndRow,
								  bool bAllowGraded,
								  bool bForward) const -> int
{
	NoteData::const_iterator begin;
	NoteData::const_iterator end;
	m_NoteData.GetTapNoteRange(col, iStartRow, iEndRow, begin, end);

	if (!bForward) {
		swap(begin, end);
	}

	while (begin != end) {
		if (!bForward) {
			--begin;
		}

		// Is this the row we want?
		do {
			const auto& tn = begin->second;
			if (!m_Timing->IsJudgableAtRow(begin->first)) {
				break;
			}
			// unsure if autoKeysounds should be excluded. -Wolfman2000
			if (tn.type == TapNoteType_Empty ||
				  tn.type == TapNoteType_AutoKeysound ||
				  tn.type == TapNoteType_Fake) {
				break;
			}
			if (!bAllowGraded && tn.result.tns != TNS_None) {
				break;
			}

			return begin->first;
		} while (0);

		if (bForward) {
			++begin;
		}
	}

	return -1;
}

// Find the closest note to a row or the song position.
auto
Player::GetClosestNote(int col,
					   int iNoteRow,
					   int iMaxRowsAhead,
					   int iMaxRowsBehind,
					   bool bAllowGraded,
					   bool bUseSongTiming,
					   bool bAllowOldMines) const -> int
{
	// Start at iIndexStartLookingAt and search outward.
	const auto iNextIndex = GetClosestNoteDirectional(
	  col, iNoteRow, iNoteRow + iMaxRowsAhead, bAllowGraded, true);
	const auto iPrevIndex = GetClosestNoteDirectional(
	  col, iNoteRow - iMaxRowsBehind, iNoteRow, bAllowGraded, false);

	if (iNextIndex == -1 && iPrevIndex == -1) {
		return -1;
	}
	if (iNextIndex == -1) {
		return iPrevIndex;
	}
	if (iPrevIndex == -1) {
		return iNextIndex;
	}

	// Get the current time, previous time, and next time.
	const auto fNoteTime = bUseSongTiming
							 ? GAMESTATE->m_Position.m_fMusicSeconds
							 : m_Timing->WhereUAtBro(iNoteRow);
	const auto fNextTime = m_Timing->WhereUAtBro(iNextIndex);
	const auto fPrevTime = m_Timing->WhereUAtBro(iPrevIndex);

	// If we passed a mine, we can't hit it anymore. Literally.
	// So forget about them.
	// RIP Minebug 20xx - 2019
	if (!bAllowOldMines) {
		auto iter = m_NoteData.FindTapNote(col, iPrevIndex);
		if (iter != m_NoteData.end(col)) {
			if ((&iter->second)->type == TapNoteType_Mine) {
				return iNextIndex;
			}
		}
	}

	/* Figure out which row is closer. */
	if (fabsf(fNoteTime - fNextTime) > fabsf(fNoteTime - fPrevTime)) {
		return iPrevIndex;
	}

	return iNextIndex;
}

auto
Player::GetClosestNonEmptyRowDirectional(int iStartRow,
										 int iEndRow,
										 bool /* bAllowGraded */,
										 bool bForward) const -> int
{
	if (bForward) {
		auto iter = m_NoteData.GetTapNoteRangeAllTracks(iStartRow, iEndRow);

		while (!iter.IsAtEnd()) {
			if (NoteDataWithScoring::IsRowCompletelyJudged(m_NoteData,
														   iter.Row())) {
				++iter;
				continue;
			}
			if (!m_Timing->IsJudgableAtRow(iter.Row())) {
				++iter;
				continue;
			}
			return iter.Row();
		}
	} else {
		auto iter =
		  m_NoteData.GetTapNoteRangeAllTracksReverse(iStartRow, iEndRow);

		while (!iter.IsAtEnd()) {
			if (NoteDataWithScoring::IsRowCompletelyJudged(m_NoteData,
														   iter.Row())) {
				++iter;
				continue;
			}
			return iter.Row();
		}
	}

	return -1;
}

// Find the closest note to fBeat.
auto
Player::GetClosestNonEmptyRow(int iNoteRow,
							  int iMaxRowsAhead,
							  int iMaxRowsBehind,
							  bool bAllowGraded) const -> int
{
	// Start at iIndexStartLookingAt and search outward.
	const auto iNextRow = GetClosestNonEmptyRowDirectional(
	  iNoteRow, iNoteRow + iMaxRowsAhead, bAllowGraded, true);
	const auto iPrevRow = GetClosestNonEmptyRowDirectional(
	  iNoteRow - iMaxRowsBehind, iNoteRow, bAllowGraded, false);

	if (iNextRow == -1 && iPrevRow == -1) {
		return -1;
	}
	if (iNextRow == -1) {
		return iPrevRow;
	}
	if (iPrevRow == -1) {
		return iNextRow;
	}

	// Get the current time, previous time, and next time.
	const auto fNoteTime = GAMESTATE->m_Position.m_fMusicSeconds;
	const auto fNextTime = m_Timing->WhereUAtBro(iNextRow);
	const auto fPrevTime = m_Timing->WhereUAtBro(iPrevRow);

	/* Figure out which row is closer. */
	if (fabsf(fNoteTime - fNextTime) > fabsf(fNoteTime - fPrevTime)) {
		return iPrevRow;
	}

	return iNextRow;
}

void
Player::DoTapScoreNone()
{
	Message msg("ScoreNone");
	MESSAGEMAN->Broadcast(msg);

	const auto iOldCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const auto iOldMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	/* The only real way to tell if a mine has been scored is if it has
	 * disappeared but this only works for hit mines so update the scores
	 * for avoided mines here. */
	if (m_pPrimaryScoreKeeper != nullptr) {
		m_pPrimaryScoreKeeper->HandleTapScoreNone();
	}

	SendComboMessages(iOldCombo, iOldMissCombo);

	if (m_pLifeMeter != nullptr) {
		m_pLifeMeter->HandleTapScoreNone();
	}

	if (PENALIZE_TAP_SCORE_NONE && m_pPlayerState != nullptr) {
		SetJudgment(BeatToNoteRow(GAMESTATE->m_Position.m_fSongBeat),
					-1,
					TAP_EMPTY,
					TNS_Miss,
					0);
		// the ScoreKeeper will subtract points later.
	}
}

void
Player::ScoreAllActiveHoldsLetGo()
{
	if (PENALIZE_TAP_SCORE_NONE) {
		const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
		const auto iSongRow = BeatToNoteRow(fSongBeat);

		// Score all active holds to NotHeld
		for (auto iTrack = 0; iTrack < m_NoteData.GetNumTracks(); ++iTrack) {
			// Since this is being called every frame, let's not check the
			// whole array every time. Instead, only check 1 beat back. Even
			// 1 is overkill.
			const auto iStartCheckingAt = max(0, iSongRow - BeatToNoteRow(1));
			NoteData::TrackMap::iterator begin;
			NoteData::TrackMap::iterator end;
			m_NoteData.GetTapNoteRangeInclusive(
			  iTrack, iStartCheckingAt, iSongRow + 1, begin, end);
			for (; begin != end; ++begin) {
				auto& tn = begin->second;
				if (tn.HoldResult.bActive) {
					tn.HoldResult.hns = HNS_LetGo;
					tn.HoldResult.fLife = 0;

					SetHoldJudgment(tn, iTrack, iSongRow);
					HandleHoldScore(tn);
				}
			}
		}
	}
}

void
Player::PlayKeysound(const TapNote& tn, TapNoteScore score)
{
	// tap note must have keysound
	if (tn.iKeysoundIndex >= 0 &&
		tn.iKeysoundIndex < static_cast<int>(m_vKeysounds.size())) {
		// handle a case for hold notes
		if (tn.type == TapNoteType_HoldHead) {
			// if the hold is not already held
			if (tn.HoldResult.hns == HNS_None) {
				// if the hold is already activated
				const auto tns = tn.result.tns;
				if (tns != TNS_None && tns != TNS_Miss && score == TNS_None) {
					// the sound must also be already playing
					if (m_vKeysounds[tn.iKeysoundIndex].IsPlaying()) {
						// if all of these conditions are met, don't play
						// the sound.
						return;
					}
				}
			}
		}
		m_vKeysounds[tn.iKeysoundIndex].Play(false);
		static auto* pVolume =
		  Preference<float>::GetPreferenceByName("SoundVolume");
		static auto fVol = pVolume->Get();
		m_vKeysounds[tn.iKeysoundIndex].SetProperty("Volume", fVol);
	}
}

void
Player::AddNoteToReplayData(int col,
							const TapNote* pTN,
							int RowOfOverlappingNoteOrRow) const
{
	m_pPlayerStageStats->m_vOffsetVector.emplace_back(
	  pTN->result.fTapNoteOffset);
	m_pPlayerStageStats->m_vTrackVector.emplace_back(col);
	m_pPlayerStageStats->m_vNoteRowVector.emplace_back(
	  RowOfOverlappingNoteOrRow);
	m_pPlayerStageStats->m_vTapNoteTypeVector.emplace_back(pTN->type);
}

void
Player::AddHoldToReplayData(int col,
							const TapNote* pTN,
							int RowOfOverlappingNoteOrRow) const
{
	if (pTN->HoldResult.hns == HNS_Held) {
		return;
	}
	HoldReplayResult hrr;
	hrr.row = RowOfOverlappingNoteOrRow;
	hrr.track = col;
	hrr.subType = pTN->subType;
	m_pPlayerStageStats->m_vHoldReplayData.emplace_back(hrr);
}

void
Player::AddMineToReplayData(int col, int row) const
{
	MineReplayResult mrr;
	mrr.row = row;
	mrr.track = col;
	m_pPlayerStageStats->m_vMineReplayData.emplace_back(mrr);
}

void
Player::Step(int col,
			 int row,
			 const std::chrono::steady_clock::time_point& tm,
			 bool /*bHeld*/,
			 bool bRelease,
			 float padStickSeconds)
{
	const auto fMusicRate =
		GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	const auto fHitUpdateDelta =
		(GAMESTATE->m_Position.m_LastBeatUpdate - RageTimer(tm)) - padStickSeconds;
	const auto fMusicSeconds =
		GAMESTATE->m_Position.m_fMusicSeconds - (fHitUpdateDelta * fMusicRate);
	const auto fSongBeat =
		GAMESTATE->m_pCurSteps ? m_Timing->GetBeatFromElapsedTime(fMusicSeconds)
							   : GAMESTATE->m_Position.m_fSongBeat;

	const auto iSongRow = row == -1 ? BeatToNoteRow(fSongBeat) : row;

	if (col != -1 && !bRelease) {
		// Update roll life
		// Let's not check the whole array every time.
		// Instead, only check 1 beat back.  Even 1 is overkill.
		// Just update the life here and let Update judge the roll.
		const auto iStartCheckingAt = max(0, iSongRow - BeatToNoteRow(1));
		NoteData::TrackMap::iterator begin;
		NoteData::TrackMap::iterator end;
		m_NoteData.GetTapNoteRangeInclusive(
		  col, iStartCheckingAt, iSongRow + 1, begin, end);
		for (; begin != end; ++begin) {
			auto& tn = begin->second;
			if (tn.type != TapNoteType_HoldHead) {
				continue;
			}

			switch (tn.subType) {
				DEFAULT_FAIL(tn.subType);
				case TapNoteSubType_Hold:
					continue;
				case TapNoteSubType_Roll:
					break;
			}

			const auto iRow = begin->first;

			auto hns = tn.HoldResult.hns;
			if (hns != HNS_None) { // if this HoldNote already has a result
				continue; // we don't need to update the logic for this one
			}

			// if they got a bad score or haven't stepped on the
			// corresponding tap yet
			const auto tns = tn.result.tns;
			auto bInitiatedNote = true;
			if (REQUIRE_STEP_ON_HOLD_HEADS) {
				bInitiatedNote = tns != TNS_None &&
								 tns != TNS_Miss; // did they step on the start?
			}
			const auto iEndRow = iRow + tn.iDuration;

			if (bInitiatedNote && tn.HoldResult.fLife != 0) {
				/* This hold note is not judged and we stepped on its head.
				 * Update iLastHeldRow. Do this even if we're a little
				 * beyond the end of the hold note, to make sure
				 * iLastHeldRow is clamped to iEndRow if the hold note is
				 * held all the way. */
				// LOG->Trace("setting iLastHeldRow to min of iSongRow (%i)
				// and iEndRow (%i)",iSongRow,iEndRow);
				tn.HoldResult.iLastHeldRow = min(iSongRow, iEndRow);
			}

			// If the song beat is in the range of this hold:
			if (iRow <= iSongRow && iRow <= iEndRow) {
				if (bInitiatedNote) {
					// Increase life
					tn.HoldResult.fLife = 1;

					if (ROLL_BODY_INCREMENTS_COMBO &&
						m_pPlayerState->m_PlayerController != PC_AUTOPLAY) {
						IncrementCombo();

						auto bBright = (m_pPlayerStageStats != nullptr) &&
									   m_pPlayerStageStats->m_iCurCombo >
										 static_cast<unsigned int>(
										   BRIGHT_GHOST_COMBO_THRESHOLD);
						if (m_pNoteField != nullptr) {
							m_pNoteField->DidHoldNote(col, HNS_Held, bBright);
						}
					}
				}
				break;
			}
		}
	}

	// Check for step on a TapNote
	/* XXX: This seems wrong. If a player steps twice quickly and two notes
	 * are close together in the same column then it is possible for the two
	 * notes to be graded out of order. Two possible fixes:
	 * 1. Adjust the fSongBeat (or the resulting note row) backward by
	 * iStepSearchRows and search forward two iStepSearchRows lengths,
	 * disallowing graded. This doesn't seem right because if a second note
	 * has passed, an earlier one should not be graded.
	 * 2. Clamp the distance searched backward to the previous row graded.
	 * Either option would fundamentally change the grading of two quick
	 * notes "jack hammers." Hmm.
	 */

	int iStepSearchRows;
	static const auto StepSearchDistance = GetMaxStepDistanceSeconds();

	// if the nerv is too small, dont optimize
	auto skipstart = nerv.size() > 10 ? nerv[10] : iSongRow + 1;

	if (iSongRow < skipstart || iSongRow > static_cast<int>(nerv.size()) - 10) {
		iStepSearchRows =
		  max(BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(
				GAMESTATE->m_Position.m_fMusicSeconds + StepSearchDistance)) -
				iSongRow,
			  iSongRow - BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(
						   GAMESTATE->m_Position.m_fMusicSeconds -
						   StepSearchDistance))) +
		  ROWS_PER_BEAT;
	} else {
		/* Buncha bullshit that speeds up searching for the rows that we're
		concerned about judging taps within by avoiding the invocation of
		the incredibly slow getbeatfromelapsedtime. Needs to be cleaned up a
		lot, whole system does. Only in use if sequential assumption remains
		valid. - Mina */

		if (nerv[nervpos] < iSongRow && nervpos < nerv.size()) {
			nervpos += 1;
		}

		auto SearchIndexBehind = nervpos;
		auto SearchIndexAhead = nervpos;
		auto SearchBeginTime = m_Timing->WhereUAtBro(nerv[nervpos]);

		while (SearchIndexBehind > 1 &&
			   SearchBeginTime -
				   m_Timing->WhereUAtBro(nerv[SearchIndexBehind - 1]) <
				 StepSearchDistance) {
			SearchIndexBehind -= 1;
		}

		while (SearchIndexAhead > 1 && SearchIndexAhead + 1 > nerv.size() &&
			   m_Timing->WhereUAtBro(nerv[SearchIndexAhead + 1]) -
				   SearchBeginTime <
				 StepSearchDistance) {
			SearchIndexAhead += 1;
		}

		auto MaxLookBehind = nerv[nervpos] - nerv[SearchIndexBehind];
		auto MaxLookAhead = nerv[SearchIndexAhead] - nerv[nervpos];

		if (nervpos > 0) {
			iStepSearchRows =
			  (max(MaxLookBehind, MaxLookAhead) + ROWS_PER_BEAT);
		}
	}

	// calculate TapNoteScore
	auto score = TNS_None;

	auto iRowOfOverlappingNoteOrRow = row;
	if (row == -1 && col != -1) {
		iRowOfOverlappingNoteOrRow = GetClosestNote(
		  col, iSongRow, iStepSearchRows, iStepSearchRows, false, true, false);
	}

	if (iRowOfOverlappingNoteOrRow != -1 && col != -1) {
		// compute the score for this hit
		auto fNoteOffset = 0.F;
		// we need this later if we are autosyncing
		const auto fStepBeat = NoteRowToBeat(iRowOfOverlappingNoteOrRow);
		const auto fStepSeconds = m_Timing->WhereUAtBro(fStepBeat);

		TapNote* pTN = nullptr;
		auto iter = m_NoteData.FindTapNote(col, iRowOfOverlappingNoteOrRow);
		DEBUG_ASSERT(iter != m_NoteData.end(col));
		pTN = &iter->second;

		if (row == -1) {
			fNoteOffset = (fStepSeconds - fMusicSeconds) / fMusicRate;
			// input data (a real tap mapped to a note any distance away)
			// this also skips things like mines hit by CrossedRows (holding)
			m_pPlayerStageStats->InputData.emplace_back(
			  !bRelease,
			  col,
			  fMusicSeconds,
			  iRowOfOverlappingNoteOrRow,
			  -fNoteOffset,
			  pTN->type,
			  pTN->subType);
		}

		NOTESKIN->SetLastSeenColor(
		  NoteTypeToString(GetNoteType(iRowOfOverlappingNoteOrRow)));

		const auto fSecondsFromExact = fabsf(fNoteOffset);

		// We don't really have to care if we are releasing on a non-lift,
		// right? This fixes a weird noteskin bug with tap explosions.
		if (PREFSMAN->m_bFullTapExplosions && bRelease &&
			pTN->type != TapNoteType_Lift) {
			return;
		}

		// Fakes.
		if (pTN->type == TapNoteType_Fake) {
			return;
		}

		switch (m_pPlayerState->m_PlayerController) {
			case PC_HUMAN:
				switch (pTN->type) {
					case TapNoteType_Mine:
						// Stepped too close to mine?
						if (!bRelease &&
							fSecondsFromExact <= GetWindowSeconds(TW_Mine) &&
							m_Timing->IsJudgableAtRow(iSongRow)) {
							score = TNS_HitMine;
						}
						break;
					case TapNoteType_HoldHead:
						// oh wow, this was causing the trigger before the
						// hold heads bug. (It was fNoteOffset > 0.f before)
						// -DaisuMaster
						if (!REQUIRE_STEP_ON_HOLD_HEADS &&
							(fNoteOffset <= GetWindowSeconds(TW_W5) &&
							 GetWindowSeconds(TW_W5) != 0)) {
							score = TNS_W1;
							break;
						}
						// Fall through to default.
					default:
						if (pTN->type != TapNoteType_HoldHead &&
							lastHoldHeadsSeconds[col] > fMusicSeconds) {
							if (fSecondsFromExact > TAP_IN_HOLD_REQ_SEC) {
								break;
							}
						}
						if ((pTN->type == TapNoteType_Lift) == bRelease) {
							if (fSecondsFromExact <= GetWindowSeconds(TW_W1)) {
								score = TNS_W1;
							} else if (fSecondsFromExact <=
									   GetWindowSeconds(TW_W2)) {
								score = TNS_W2;
							} else if (fSecondsFromExact <=
									   GetWindowSeconds(TW_W3)) {
								score = TNS_W3;
							} else if (fSecondsFromExact <=
									   GetWindowSeconds(TW_W4)) {
								score = TNS_W4;
							} else if (fSecondsFromExact <=
									   max(GetWindowSeconds(TW_W5),
										   MISS_WINDOW_BEGIN_SEC)) {
								score = TNS_W5;
							}
						}
						break;
				}
				break;

			case PC_CPU:
			case PC_AUTOPLAY:
				score = GetAutoplayTapNoteScore(m_pPlayerState);

				/* XXX: This doesn't make sense.
				 * Step should only be called in autoplay for hit notes. */
#if 0
			// GetTapNoteScore always returns TNS_W1 in autoplay.
			// If the step is far away, don't judge it.
			if( m_pPlayerState->m_PlayerController == PC_AUTOPLAY &&
				fSecondsFromExact > GetWindowSeconds(TW_W5) )
			{
				score = TNS_None;
				break;
			}
#endif

				// TRICKY:  We're asking the AI to judge mines. Consider
				// TNS_W4 and below as "mine was hit" and everything else as
				// "mine was avoided"
				if (pTN->type == TapNoteType_Mine) {
					// The CPU hits a lot of mines. Only consider hitting
					// the first mine for a row. We know we're the first
					// mine if there are are no mines to the left of us.
					for (auto t = 0; t < col; t++) {
						if (m_NoteData.GetTapNote(t, iRowOfOverlappingNoteOrRow)
							  .type == TapNoteType_Mine) { // there's a mine to
														   // the left of us
							return;						   // avoid
						}
					}

					// The CPU hits a lot of mines. Make it less likely to
					// hit mines that don't have a tap note on the same row.
					auto bTapsOnRow = m_NoteData.IsThereATapOrHoldHeadAtRow(
					  iRowOfOverlappingNoteOrRow);
					auto get_to_avoid = bTapsOnRow ? TNS_W3 : TNS_W4;

					if (score >= get_to_avoid) {
						return; // avoided
					}

					score = TNS_HitMine;
				}

				if (score > TNS_W4) {
					score = TNS_W2; // sentinel
				}

				/* AI will generate misses here. Don't handle a miss like a
				 * regular note because we want the judgment animation to
				 * appear delayed. Instead, return early if AI generated a
				 * miss, and let UpdateTapNotesMissedOlderThan() detect and
				 * handle the misses. */
				if (score == TNS_Miss) {
					return;
				}

				// Put some small, random amount in fNoteOffset so that
				// demonstration show a mix of late and early. - Chris
				// (StepMania r15628)
				// fNoteOffset = randomf( -0.1f, 0.1f );
				// Since themes may use the offset in a visual graph, the
				// above behavior is not the best thing to do. Instead,
				// random numbers should be generated based on the
				// TapNoteScore, so that they can logically match up with
				// the current timing windows. -aj
				{
					// auto fWindowW1 = GetWindowSeconds(TW_W1);
					// auto fWindowW2 = GetWindowSeconds(TW_W2);
					auto fWindowW3 = GetWindowSeconds(TW_W3);
					auto fWindowW4 = GetWindowSeconds(TW_W4);
					auto fWindowW5 = GetWindowSeconds(TW_W5);

					// figure out overlap.
					auto fLowerBound = 0.0F;	// negative upper limit
					auto fUpperBound = 0.0F;	// positive lower limit
					auto fCompareWindow = 0.0F; // filled in here:
					if (score == TNS_W4) {
						fLowerBound = -fWindowW3;
						fUpperBound = fWindowW3;
						fCompareWindow = fWindowW4;
					} else if (score == TNS_W5) {
						fLowerBound = -fWindowW4;
						fUpperBound = fWindowW4;
						fCompareWindow = fWindowW5;
					}
					auto f1 = randomf(-fCompareWindow, fLowerBound);
					auto f2 = randomf(fUpperBound, fCompareWindow);

					if (randomf() * 100 >= 50) {
						fNoteOffset = f1;
					} else {
						fNoteOffset = f2;
					}
				}

				break;
			default:
				FAIL_M(ssprintf("Invalid player controller type: %i",
								m_pPlayerState->m_PlayerController));
		}

		if (m_pPlayerState->m_PlayerController == PC_HUMAN && score >= TNS_W3) {
			AdjustSync::HandleAutosync(fNoteOffset, fStepSeconds);
		}

		// Do game-specific and mode-specific score mapping.
		score = GAMESTATE->GetCurrentGame()->MapTapNoteScore(score);

		if (score != TNS_None) {
			pTN->result.tns = score;
			pTN->result.fTapNoteOffset = -fNoteOffset;
		}

		m_LastTapNoteScore = score;
		if (pTN->result.tns != TNS_None) {
			AddNoteToReplayData(GAMESTATE->CountNotesSeparately() ? col : -1,
								pTN,
								iRowOfOverlappingNoteOrRow);
		}
		if (GAMESTATE->CountNotesSeparately()) {
			if (pTN->type != TapNoteType_Mine) {
				const auto bBlind =
				  (m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind != 0);
				const auto bBright =
				  ((m_pPlayerStageStats != nullptr) &&
				   m_pPlayerStageStats->m_iCurCombo >
					 static_cast<unsigned int>(BRIGHT_GHOST_COMBO_THRESHOLD));
				if (m_pNoteField != nullptr) {
					m_pNoteField->DidTapNote(
					  col, bBlind ? TNS_W1 : score, bBright);
				}
				if (score >= m_pPlayerState->m_PlayerOptions.GetCurrent()
							   .m_MinTNSToHideNotes) {
					HideNote(col, iRowOfOverlappingNoteOrRow);
				}

				if (pTN->result.tns != TNS_None) {
					SetJudgment(iRowOfOverlappingNoteOrRow, col, *pTN);
					HandleTapRowScore(iRowOfOverlappingNoteOrRow);
				}
			}
		} else if (NoteDataWithScoring::IsRowCompletelyJudged(
					 m_NoteData, iRowOfOverlappingNoteOrRow)) {
			FlashGhostRow(iRowOfOverlappingNoteOrRow);
		}
	} else {
		// input data
		// (autoplay, forced step, or step REALLY far away)

		TapNoteType tnt = TapNoteType_Invalid;
		TapNoteSubType tnst = TapNoteSubType_Invalid;

		if (col != -1 && iRowOfOverlappingNoteOrRow != -1) {
			const auto& tn =
			  m_NoteData.GetTapNote(col, iRowOfOverlappingNoteOrRow);
			tnt = tn.type;
			tnst = tn.subType;
		}

		m_pPlayerStageStats->InputData.emplace_back(!bRelease,
													col,
													fMusicSeconds,
													iRowOfOverlappingNoteOrRow,
													0.F,
													tnt,
													tnst);
	}

	if (score == TNS_None) {
		DoTapScoreNone();
	}

	if (!bRelease && col != -1) {
		/* Search for keyed sounds separately.  Play the nearest note. */
		/* XXX: This isn't quite right. As per the above XXX for
		 * iRowOfOverlappingNote, if iRowOfOverlappingNote is set to a
		 * previous note, the keysound could have changed and this would
		 * cause the wrong one to play, in essence playing two sounds in the
		 * opposite order. Maybe this should always perform the search.
		 * Still, even that doesn't seem quite right since it would then
		 * play the same (new) keysound twice which would sound wrong even
		 * though the notes were judged as being correct, above. Fixing the
		 * above problem would fix this one as well. */
		int iHeadRow;
		if (iRowOfOverlappingNoteOrRow != -1 && score != TNS_None) {
			// just pressing a note, use that row.
			// in other words, iRowOfOverlappingNoteOrRow =
			// iRowOfOverlappingNoteOrRow
		} else if (m_NoteData.IsHoldNoteAtRow(col, iSongRow, &iHeadRow)) {
			// stepping on a hold, use it!
			iRowOfOverlappingNoteOrRow = iHeadRow;
		} else {
			// or else find the closest note.
			iRowOfOverlappingNoteOrRow =
			  GetClosestNote(col, iSongRow, MAX_NOTE_ROW, MAX_NOTE_ROW, true);
		}
		if (iRowOfOverlappingNoteOrRow != -1) {
			const auto& tn =
			  m_NoteData.GetTapNote(col, iRowOfOverlappingNoteOrRow);
			PlayKeysound(tn, score);
		}
	}
	// XXX:

	if (!bRelease) {
		if (m_pNoteField != nullptr && col != -1) {
			m_pNoteField->Step(col, score);
		}
		Message msg("Step");
		msg.SetParam("PlayerNumber", m_pPlayerState->m_PlayerNumber);
		msg.SetParam("MultiPlayer", m_pPlayerState->m_mp);
		msg.SetParam("Column", col);
		MESSAGEMAN->Broadcast(msg);
		// Backwards compatibility
		Message msg2(ssprintf("StepP%d", m_pPlayerState->m_PlayerNumber + 1));
		MESSAGEMAN->Broadcast(msg2);
	}
}

void
Player::FlashGhostRow(int iRow)
{
	const auto lastTNS =
	  NoteDataWithScoring::LastTapNoteWithResult(m_NoteData, iRow).result.tns;
	const auto bBright =
	  ((m_pPlayerStageStats != nullptr) &&
	   m_pPlayerStageStats->m_iCurCombo >
		 static_cast<unsigned int>(BRIGHT_GHOST_COMBO_THRESHOLD));

	for (auto iTrack = 0; iTrack < m_NoteData.GetNumTracks(); ++iTrack) {
		const auto& tn = m_NoteData.GetTapNote(iTrack, iRow);

		if (tn.type == TapNoteType_Empty || tn.type == TapNoteType_Mine ||
			tn.type == TapNoteType_Fake || tn.result.bHidden) {
			continue;
		}
		if (m_pNoteField != nullptr) {
			m_pNoteField->DidTapNote(iTrack, lastTNS, bBright);
		}
		if (lastTNS >=
			m_pPlayerState->m_PlayerOptions.GetCurrent().m_MinTNSToHideNotes) {
			HideNote(iTrack, iRow);
		}
	}
}

void
Player::CrossedRows(int iLastRowCrossed,
					const std::chrono::steady_clock::time_point& now)
{
	// LOG->Trace( "Player::CrossedRows   %d    %d", iFirstRowCrossed,
	// iLastRowCrossed );

	auto& iter = *m_pIterUncrossedRows;
	auto iLastSeenRow = -1;
	for (; !iter.IsAtEnd() && iter.Row() <= iLastRowCrossed; ++iter) {
		// Apply InitialHoldLife.
		auto& tn = *iter;
		auto iRow = iter.Row();
		const auto iTrack = iter.Track();
		switch (tn.type) {
			case TapNoteType_HoldHead: {
				tn.HoldResult.fLife = initialHoldLife;
				if (!REQUIRE_STEP_ON_HOLD_HEADS) {
					std::vector<GameInput> GameI;
					GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
					  ->StyleInputToGameInput(iTrack, GameI);
					if (PREFSMAN->m_fPadStickSeconds > 0.F) {
						for (auto& i : GameI) {
							const auto fSecsHeld =
							  INPUTMAPPER->GetSecsHeld(i, m_pPlayerState->m_mp);
							if (fSecsHeld >= PREFSMAN->m_fPadStickSeconds) {
								Step(iTrack,
									 -1,
									 now,
									 true,
									 false,
									 PREFSMAN->m_fPadStickSeconds);
							}
						}
					} else {
						if (INPUTMAPPER->IsBeingPressed(GameI,
														m_pPlayerState->m_mp)) {
							Step(iTrack, -1, now, true, false);
						}
					}
				}
				break;
			}
			case TapNoteType_Mine: {
				// Holding the button crossing a mine will cause the mine
				// to explode
				// This action also adds an input event for the mine hit
				std::vector<GameInput> GameI;
				GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
				  ->StyleInputToGameInput(iTrack, GameI);
				if (PREFSMAN->m_fPadStickSeconds > 0.0F) {
					for (auto& i : GameI) {
						const auto fSecsHeld =
						  INPUTMAPPER->GetSecsHeld(i, m_pPlayerState->m_mp);
						if (fSecsHeld >= PREFSMAN->m_fPadStickSeconds) {
							Step(iTrack,
								 -1,
								 now,
								 true,
								 false,
								 PREFSMAN->m_fPadStickSeconds);
						}
					}
				} else if (INPUTMAPPER->IsBeingPressed(GameI,
													   m_pPlayerState->m_mp)) {
					Step(iTrack, iRow, now, true, false);
				}
				break;
			}
			default:
				break;
		}

		// check to see if there's a note at the crossed row
		if (m_pPlayerState->m_PlayerController != PC_HUMAN) {
			if (tn.type != TapNoteType_Empty && tn.type != TapNoteType_Fake &&
				tn.type != TapNoteType_AutoKeysound &&
				tn.result.tns == TNS_None &&
				this->m_Timing->IsJudgableAtRow(iRow)) {
				if (m_pPlayerState->m_PlayerController == PC_AUTOPLAY ||
					m_pPlayerState->m_PlayerController == PC_CPU) {
					Step(iTrack, iRow, now, false, false);
					if (m_pPlayerState->m_PlayerController == PC_AUTOPLAY ||
						m_pPlayerState->m_PlayerController == PC_CPU) {
						if (m_pPlayerStageStats != nullptr) {
							m_pPlayerStageStats->m_bDisqualified = true;
						}
					}
				}
			}
		}

		// TODO(Sam): Can we remove the iLastSeenRow logic and the
		// autokeysound for loop, since the iterator in this loop will
		// already be iterating over all of the tracks?
		if (iRow != iLastSeenRow) {
			// crossed a new not-empty row
			iLastSeenRow = iRow;

			for (auto t = 0; t < m_NoteData.GetNumTracks(); ++t) {
				const auto& tap = m_NoteData.GetTapNote(t, iRow);
				if (tap.type == TapNoteType_AutoKeysound) {
					PlayKeysound(tap, TNS_None);
				}
			}
		}
	}

	/* Update hold checkpoints
	 *
	 * TODO: Move this to a separate function. */
	if (m_bTickHolds && m_pPlayerState->m_PlayerController == PC_HUMAN) {
		// Few rows typically cross per update. Easier to check all crossed
		// rows than to calculate from timing segments.
		for (auto r = m_iFirstUncrossedRow; r <= iLastRowCrossed; ++r) {
			const auto tickCurrent = m_Timing->GetTickcountAtRow(r);

			// There is a tick count at this row
			if (tickCurrent > 0 && r % (ROWS_PER_BEAT / tickCurrent) == 0) {

				std::vector<int> viColsWithHold;
				auto iNumHoldsHeldThisRow = 0;
				auto iNumHoldsMissedThisRow = 0;

				// start at r-1 so that we consider holds whose end rows are
				// equal to the checkpoint row
				auto nIter =
				  m_NoteData.GetTapNoteRangeAllTracks(r - 1, r, true);
				for (; !nIter.IsAtEnd(); ++nIter) {
					auto& tn = *nIter;
					if (tn.type != TapNoteType_HoldHead) {
						continue;
					}

					auto iTrack = nIter.Track();
					viColsWithHold.push_back(iTrack);

					if (tn.HoldResult.fLife > 0) {
						++iNumHoldsHeldThisRow;
						++tn.HoldResult.iCheckpointsHit;
					} else {
						++iNumHoldsMissedThisRow;
						++tn.HoldResult.iCheckpointsMissed;
					}
				}
				GAMESTATE->SetProcessedTimingData(this->m_Timing);

				// TODO(Sam): Find a better way of handling hold checkpoints
				// with other taps.
				if (!viColsWithHold.empty() &&
					(CHECKPOINTS_TAPS_SEPARATE_JUDGMENT ||
					 m_NoteData.GetNumTapNotesInRow(r) == 0)) {
					HandleHoldCheckpoint(r,
										 iNumHoldsHeldThisRow,
										 iNumHoldsMissedThisRow,
										 viColsWithHold);
				}
			}
		}
	}

	m_iFirstUncrossedRow = iLastRowCrossed + 1;
}

void
Player::UpdateTapNotesMissedOlderThan(float fMissIfOlderThanSeconds)
{
	// LOG->Trace( "Steps::UpdateTapNotesMissedOlderThan(%f)",
	// fMissIfOlderThanThisBeat );
	int iMissIfOlderThanThisRow;
	const auto fEarliestTime =
	  GAMESTATE->m_Position.m_fMusicSeconds - fMissIfOlderThanSeconds;
	{
		TimingData::GetBeatArgs beat_info;
		beat_info.elapsed_time = fEarliestTime;
		m_Timing->GetBeatAndBPSFromElapsedTime(beat_info);

		iMissIfOlderThanThisRow = BeatToNoteRow(beat_info.beat);
		if (beat_info.freeze_out || beat_info.delay_out) {
			/* If there is a freeze on iMissIfOlderThanThisIndex, include
			 * this index too. Otherwise we won't show misses for tap notes
			 * on freezes until the freeze finishes. */
			if (!beat_info.delay_out) {
				iMissIfOlderThanThisRow++;
			}
		}
	}

	auto& iter = *m_pIterNeedsTapJudging;

	for (; !iter.IsAtEnd() && iter.Row() < iMissIfOlderThanThisRow; ++iter) {
		auto& tn = *iter;

		if (!NeedsTapJudging(tn)) {
			continue;
		}

		// Ignore all notes in WarpSegments or FakeSegments.
		if (!m_Timing->IsJudgableAtRow(iter.Row())) {
			continue;
		}

		if (tn.type == TapNoteType_Mine) {
			tn.result.tns = TNS_AvoidMine;
			/* The only real way to tell if a mine has been scored is if it
			 * has disappeared but this only works for hit mines so update
			 * the scores for avoided mines here. */
			if (m_pPrimaryScoreKeeper != nullptr) {
				m_pPrimaryScoreKeeper->HandleTapScore(tn);
			}
		} else {
			tn.result.tns = TNS_Miss;

			// avoid scoring notes that get passed when seeking in pm
			// not sure how many rows grace time is needed (if any?)
			if (GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
				  .m_bPractice &&
				iMissIfOlderThanThisRow - iter.Row() > 8) {
				tn.result.tns = TNS_None;
			}
			if (GAMESTATE->CountNotesSeparately()) {
				SetJudgment(iter.Row(), iter.Track(), tn);
				HandleTapRowScore(iter.Row());
			}
		}
	}
}

void
Player::UpdateJudgedRows(float /*fDeltaTime*/)
{
	// Look into the future only as far as we need to
	const auto iEndRow = BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(
	  GAMESTATE->m_Position.m_fMusicSeconds + GetMaxStepDistanceSeconds()));
	auto bAllJudged = true;

	if (!GAMESTATE->CountNotesSeparately()) {
		auto iter = *m_pIterUnjudgedRows;
		auto iLastSeenRow = -1;
		for (; !iter.IsAtEnd() && iter.Row() <= iEndRow; ++iter) {
			const auto iRow = iter.Row();

			// Do not judge arrows in WarpSegments or FakeSegments
			if (!m_Timing->IsJudgableAtRow(iRow)) {
				continue;
			}

			if (iLastSeenRow != iRow) {
				iLastSeenRow = iRow;

				// crossed a nonempty row
				if (!NoteDataWithScoring::IsRowCompletelyJudged(m_NoteData,
																iRow)) {
					bAllJudged = false;
					continue;
				}
				if (bAllJudged) {
					*m_pIterUnjudgedRows = iter;
				}
				if (m_pJudgedRows->JudgeRow(iRow)) {
					continue;
				}

				const auto& lastTN =
				  NoteDataWithScoring::LastTapNoteWithResult(m_NoteData, iRow);

				if (lastTN.result.tns < TNS_Miss) {
					continue;
				}

				SetJudgment(
				  iRow,
				  m_NoteData.GetFirstTrackWithTapOrHoldHead(iter.Row()),
				  lastTN);
				HandleTapRowScore(iRow);
			}
		}
	}

	// handle mines.
	{
		bAllJudged = true;
		std::set<RageSound*> setSounds;
		auto iter = *m_pIterUnjudgedMineRows; // copy
		auto iLastSeenRow = -1;
		for (; !iter.IsAtEnd() && iter.Row() <= iEndRow; ++iter) {
			const auto iRow = iter.Row();

			// Do not worry about mines in WarpSegments or FakeSegments
			if (!m_Timing->IsJudgableAtRow(iRow)) {
				continue;
			}

			auto& tn = *iter;

			if (iRow != iLastSeenRow) {
				iLastSeenRow = iRow;
				if (bAllJudged) {
					*m_pIterUnjudgedMineRows = iter;
				}
			}

			const auto bMineNotHidden =
			  tn.type == TapNoteType_Mine && !tn.result.bHidden;
			if (!bMineNotHidden) {
				continue;
			}

			switch (tn.result.tns) {
				DEFAULT_FAIL(tn.result.tns);
				case TNS_None:
					bAllJudged = false;
					continue;
				case TNS_AvoidMine:
					SetMineJudgment(tn.result.tns, iter.Track(), iRow);
					tn.result.bHidden = true;
					continue;
				case TNS_HitMine:
					SetMineJudgment(tn.result.tns, iter.Track(), iRow);
					break;
			}
			if (m_pNoteField != nullptr) {
				m_pNoteField->DidTapNote(iter.Track(), tn.result.tns, false);
			}

			if (tn.iKeysoundIndex >= 0 &&
				tn.iKeysoundIndex < static_cast<int>(m_vKeysounds.size())) {
				setSounds.insert(&m_vKeysounds[tn.iKeysoundIndex]);
			} else if (g_bEnableMineSoundPlayback) {
				setSounds.insert(&m_soundMine);
			}

			ChangeLife(tn.result.tns);
			// Make sure hit mines affect the dance points.
			if (m_pPrimaryScoreKeeper != nullptr) {
				m_pPrimaryScoreKeeper->HandleTapScore(tn);
			}
			tn.result.bHidden = true;
		}
		// If we hit the end of the loop, m_pIterUnjudgedMineRows needs to
		// be updated. -Kyz
		if ((iter.IsAtEnd() || iLastSeenRow == iEndRow) && bAllJudged) {
			*m_pIterUnjudgedMineRows = iter;
		}

		for (const auto& s : setSounds) {
			// Only play one copy of each mine sound at a time per player.
			s->Stop();
			s->Play(false);
		}
	}
}

void
Player::HandleTapRowScore(unsigned row)
{
	const auto bNoCheating = true;
#ifdef DEBUG
	bNoCheating = false;
#endif

	// don't accumulate points if AutoPlay is on.
	if (bNoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY) {
		return;
	}

	const auto scoreOfLastTap =
	  NoteDataWithScoring::LastTapNoteWithResult(m_NoteData, row).result.tns;
	const auto iOldCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const auto iOldMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	if (scoreOfLastTap == TNS_Miss) {
		m_LastTapNoteScore = TNS_Miss;
	}

	for (auto track = 0; track < m_NoteData.GetNumTracks(); ++track) {
		const auto& tn = m_NoteData.GetTapNote(track, row);
		// Mines cannot be handled here.
		if (tn.type == TapNoteType_Empty || tn.type == TapNoteType_Fake ||
			tn.type == TapNoteType_Mine ||
			tn.type == TapNoteType_AutoKeysound) {
			continue;
		}
		if (m_pPrimaryScoreKeeper != nullptr) {
			m_pPrimaryScoreKeeper->HandleTapScore(tn);
		}
	}

	if (m_pPrimaryScoreKeeper != nullptr) {
		m_pPrimaryScoreKeeper->HandleTapRowScore(m_NoteData, row);
	}

	const auto iCurCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const auto iCurMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	SendComboMessages(iOldCombo, iOldMissCombo);

	if (m_pPlayerStageStats != nullptr) {
		SetCombo(iCurCombo, iCurMissCombo);
	}

#define CROSSED(x) (iOldCombo < (x) && iCurCombo >= (x))
	if (CROSSED(100)) {
		SCREENMAN->PostMessageToTopScreen(SM_100Combo, 0);
	} else if (CROSSED(200)) {
		SCREENMAN->PostMessageToTopScreen(SM_200Combo, 0);
	} else if (CROSSED(300)) {
		SCREENMAN->PostMessageToTopScreen(SM_300Combo, 0);
	} else if (CROSSED(400)) {
		SCREENMAN->PostMessageToTopScreen(SM_400Combo, 0);
	} else if (CROSSED(500)) {
		SCREENMAN->PostMessageToTopScreen(SM_500Combo, 0);
	} else if (CROSSED(600)) {
		SCREENMAN->PostMessageToTopScreen(SM_600Combo, 0);
	} else if (CROSSED(700)) {
		SCREENMAN->PostMessageToTopScreen(SM_700Combo, 0);
	} else if (CROSSED(800)) {
		SCREENMAN->PostMessageToTopScreen(SM_800Combo, 0);
	} else if (CROSSED(900)) {
		SCREENMAN->PostMessageToTopScreen(SM_900Combo, 0);
	} else if (CROSSED(1000)) {
		SCREENMAN->PostMessageToTopScreen(SM_1000Combo, 0);
	} else if ((iOldCombo / 100) < (iCurCombo / 100) && iCurCombo > 1000) {
		SCREENMAN->PostMessageToTopScreen(SM_ComboContinuing, 0);
	}
#undef CROSSED

	// new max combo
	if (m_pPlayerStageStats != nullptr) {
		m_pPlayerStageStats->m_iMaxCombo =
		  max(m_pPlayerStageStats->m_iMaxCombo, iCurCombo);
	}

	/* Use the real current beat, not the beat we've been passed. That's
	 * because we want to record the current life/combo to the current time;
	 * eg. if it's a MISS, the beat we're registering is in the past, but
	 * the life is changing now. We need to include time from previous songs
	 * in a course, so we can't use GAMESTATE->m_fMusicSeconds. Use
	 * fStepsSeconds instead. */
	if (m_pPlayerStageStats != nullptr) {
		m_pPlayerStageStats->UpdateComboList(
		  GAMESTATE->m_Position.m_fMusicSeconds /
			GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate,
		  false);
	}

	ChangeLife(scoreOfLastTap);
}

void
Player::HandleHoldCheckpoint(int iRow,
							 int iNumHoldsHeldThisRow,
							 int iNumHoldsMissedThisRow,
							 const std::vector<int>& viColsWithHold)
{
	const auto bNoCheating = true;
#ifdef DEBUG
	bNoCheating = false;
#endif

	// WarpSegments and FakeSegments aren't judged in any way.
	if (!m_Timing->IsJudgableAtRow(iRow)) {
		return;
	}

	// don't accumulate combo if AutoPlay is on.
	if (bNoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY) {
		return;
	}

	const auto iOldCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const auto iOldMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	if (m_pPrimaryScoreKeeper != nullptr) {
		m_pPrimaryScoreKeeper->HandleHoldCheckpointScore(
		  m_NoteData, iRow, iNumHoldsHeldThisRow, iNumHoldsMissedThisRow);
	}

	if (iNumHoldsMissedThisRow == 0) {
		// added for http://ssc.ajworld.net/sm-ssc/bugtracker/view.php?id=16
		// -aj
		if (CHECKPOINTS_FLASH_ON_HOLD) {
			for (const auto& i : viColsWithHold) {
				const auto bBright =
				  (m_pPlayerStageStats != nullptr) &&
				  m_pPlayerStageStats->m_iCurCombo >
					static_cast<unsigned int>(BRIGHT_GHOST_COMBO_THRESHOLD);
				if (m_pNoteField != nullptr) {
					m_pNoteField->DidHoldNote(i, HNS_Held, bBright);
				}
			}
		}
	}

	SendComboMessages(iOldCombo, iOldMissCombo);

	if (m_pPlayerStageStats != nullptr) {
		SetCombo(m_pPlayerStageStats->m_iCurCombo,
				 m_pPlayerStageStats->m_iCurMissCombo);
		m_pPlayerStageStats->UpdateComboList(
		  GAMESTATE->m_Position.m_fMusicSeconds /
			GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate,
		  false);
	}

	ChangeLife(iNumHoldsMissedThisRow == 0 ? TNS_CheckpointHit
										   : TNS_CheckpointMiss);

	SetJudgment(iRow,
				viColsWithHold[0],
				TAP_EMPTY,
				iNumHoldsMissedThisRow == 0 ? TNS_CheckpointHit
											: TNS_CheckpointMiss,
				0);
}

void
Player::HandleHoldScore(const TapNote& tn) const
{
	const auto holdScore = tn.HoldResult.hns;
	const auto tapScore = tn.result.tns;
	const auto bNoCheating = true;
#ifdef DEBUG
	bNoCheating = false;
#endif

	// don't accumulate points if AutoPlay is on.
	if (bNoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY) {
		return;
	}

	if (m_pPrimaryScoreKeeper != nullptr) {
		m_pPrimaryScoreKeeper->HandleHoldScore(tn);
	}
	ChangeLife(holdScore, tapScore);
}

auto
Player::GetMaxStepDistanceSeconds() -> float
{
	// The former behavior of this did not follow capped 180ms window logic.
	// The result is that on higher judges, the late window was too small.
	// Setting this hard to .18 x rate brings it back into line.
	// (On that note, this should only be used in the context of music
	// timing, because at a 3x rate this would expand by 3x correctly)
	float fMax = MISS_WINDOW_BEGIN_SEC;
	/*
	float fMax = 0;
	fMax = max(fMax, GetWindowSeconds(TW_W5));
	fMax = max(fMax, GetWindowSeconds(TW_W4));
	fMax = max(fMax, GetWindowSeconds(TW_W3));
	fMax = max(fMax, GetWindowSeconds(TW_W2));
	fMax = max(fMax, GetWindowSeconds(TW_W1));
	*/
	const auto f = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate * fMax;
	return f;
}

void
Player::FadeToFail()
{
	if (m_pNoteField != nullptr) {
		m_pNoteField->FadeToFail();
	}

	// clear miss combo
	SetCombo(0, 0);
}

void
Player::CacheAllUsedNoteSkins() const
{
	if (m_pNoteField != nullptr) {
		m_pNoteField->CacheAllUsedNoteSkins();
	}
}

/* Reworked the judgment messages. Header file states that -1 should be sent
as the offset for misses. This was not the case and 0s were being sent. Now
it just sends nothing so params.Judgment
~= nil can be used to filter messages with and without offsets. Also now
there's a params.Judgment that just gives the judgment for taps holds and
mines in aggregate for things that need to be done with any judgment.
Params.Type is used to diffrentiate between those attributes for things that
are done differently between the types. Current values for taps/holds are
sent in params.Val. Like it all should have been to begin with. Not sure
where checkpoints are but I also don't care, so.

Update: both message types are being sent out currently for compatability.
-Mina*/
//#define autoplayISHUMAN
void
Player::SetMineJudgment(TapNoteScore tns, int iTrack, int iRow)
{
	if (tns == TNS_HitMine) {
		AddMineToReplayData(iTrack, iRow);
	}

	if (m_bSendJudgmentAndComboMessages) {
		Message msg("Judgment");
		msg.SetParam("Player", m_pPlayerState->m_PlayerNumber);
		msg.SetParam("TapNoteScore", tns);
		msg.SetParam("FirstTrack", iTrack);
		msg.SetParam("Judgment", tns);
		msg.SetParam("Type", std::string("Mine"));

		// Ms scoring implemenation - Mina
		if (tns == TNS_HitMine) {
			curwifescore += wife3_mine_hit_weight;
		}

		if (m_pPlayerStageStats != nullptr) {
			if (maxwifescore == 0.F) {
				msg.SetParam("WifePercent", 0);
			} else {
				msg.SetParam("WifePercent", 100 * curwifescore / maxwifescore);
			}

			msg.SetParam("CurWifeScore", curwifescore);
			msg.SetParam("MaxWifeScore", maxwifescore);
			msg.SetParam("WifeDifferential",
						 curwifescore -
						   maxwifescore * m_pPlayerState->playertargetgoal);
			msg.SetParam("TotalPercent", 100 * curwifescore / totalwifescore);
			if (wifescorepersonalbest != m_pPlayerState->playertargetgoal) {
				msg.SetParam("WifePBDifferential",
							 curwifescore -
							   maxwifescore * wifescorepersonalbest);
				msg.SetParam("WifePBGoal", wifescorepersonalbest);
			}
#ifdef autoplayISHUMAN
			ChangeWifeRecord();
			m_pPlayerStageStats->m_fWifeScore = curwifescore / totalwifescore;

#else
			if (m_pPlayerState->m_PlayerController == PC_HUMAN ||
				m_pPlayerState->m_PlayerController == PC_REPLAY) {
				m_pPlayerStageStats->m_fWifeScore =
				  curwifescore / totalwifescore;
				m_pPlayerStageStats->CurWifeScore = curwifescore;
				m_pPlayerStageStats->MaxWifeScore = maxwifescore;
			} else {
				curwifescore -= 6666666.F; // sail hatan
			}
#endif
		}

		MESSAGEMAN->Broadcast(msg);
		if ((m_pPlayerStageStats != nullptr) &&
			((tns == TNS_AvoidMine && AVOID_MINE_INCREMENTS_COMBO) ||
			 (tns == TNS_HitMine && MINE_HIT_INCREMENTS_MISS_COMBO))) {
			SetCombo(m_pPlayerStageStats->m_iCurCombo,
					 m_pPlayerStageStats->m_iCurMissCombo);
		}
	}
}

void
Player::SetJudgment(int iRow,
					int iTrack,
					const TapNote& tn,
					TapNoteScore tns,
					float fTapNoteOffset)
{
	if (tns == TNS_Miss && m_pPlayerStageStats != nullptr) {
		AddNoteToReplayData(
		  GAMESTATE->CountNotesSeparately() ? iTrack : -1, &tn, iRow);

		// add miss to input data
		// the fmusicseconds is when the judgment occurs
		// but the row is the row of the actual note
		m_pPlayerStageStats->m_vNoteMissVector.emplace_back(
		  GAMESTATE->CountNotesSeparately() ? iTrack : -1,
		  iRow,
		  tn.type,
		  tn.subType);
	}

	if (m_bSendJudgmentAndComboMessages) {
		Message msg("Judgment");
		msg.SetParam("Player", m_pPlayerState->m_PlayerNumber);
		msg.SetParam("MultiPlayer", m_pPlayerState->m_mp);
		msg.SetParam("FirstTrack", iTrack);
		msg.SetParam("TapNoteScore", tns);
		msg.SetParam("Early", fTapNoteOffset < 0.0F);
		msg.SetParam("Judgment", tns);
		msg.SetParam("NoteRow", iRow);
		msg.SetParam("Type", std::string("Tap"));
		msg.SetParam("TapNoteOffset", tn.result.fTapNoteOffset);
		if (m_pPlayerStageStats != nullptr) {
			if (tns >= NUM_TapNoteScore || tns < 0) {
				Locator::getLogger()->fatal(
				  "Invalid TNS {} sent to SetJudgment. Ignored 'Val' param in "
				  "JudgmentMessage",
				  tns);
			} else {
				msg.SetParam("Val",
							 m_pPlayerStageStats->m_iTapNoteScores[tns] + 1);
			}
		}

		if (tns != TNS_Miss) {
			msg.SetParam("Offset",
						 tn.result.fTapNoteOffset * 1000); // don't send out ms
		}
		// offsets for
		// misses, multiply
		// by 1000 for
		// convenience - Mina

		if (m_pPlayerStageStats != nullptr) {
			if (tns == TNS_Miss) {
				curwifescore += wife3_miss_weight;
			} else {
				curwifescore +=
				  wife3(tn.result.fTapNoteOffset, GetTimingWindowScale());
			}
			maxwifescore += 2;

			msg.SetParam("WifePercent", 100 * curwifescore / maxwifescore);
			msg.SetParam("CurWifeScore", curwifescore);
			msg.SetParam("MaxWifeScore", maxwifescore);
			msg.SetParam("WifeDifferential",
						 curwifescore -
						   maxwifescore * m_pPlayerState->playertargetgoal);
			msg.SetParam("TotalPercent", 100 * curwifescore / totalwifescore);
			if (wifescorepersonalbest != m_pPlayerState->playertargetgoal) {
				msg.SetParam("WifePBDifferential",
							 curwifescore -
							   maxwifescore * wifescorepersonalbest);
				msg.SetParam("WifePBGoal", wifescorepersonalbest);
			}
#ifdef autoplayISHUMAN
			m_pPlayerStageStats->m_fWifeScore = curwifescore / totalwifescore;
			m_pPlayerStageStats->m_vOffsetVector.emplace_back(
			  tn.result.fTapNoteOffset);
			m_pPlayerStageStats->m_vNoteRowVector.emplace_back(iRow);
			ChangeWifeRecord();
#else
			if (m_pPlayerState->m_PlayerController == PC_HUMAN ||
				m_pPlayerState->m_PlayerController == PC_REPLAY) {
				m_pPlayerStageStats->m_fWifeScore =
				  curwifescore / totalwifescore;
				m_pPlayerStageStats->CurWifeScore = curwifescore;
				m_pPlayerStageStats->MaxWifeScore = maxwifescore;
			} else {
				curwifescore -= 666.F; // hail satan
			}

#endif
		}

		auto* L = LUA->Get();
		lua_createtable(L, 0, m_NoteData.GetNumTracks()); // TapNotes this row
		lua_createtable(
		  L,
		  0,
		  m_NoteData.GetNumTracks()); // HoldHeads of tracks held at this row.
		if (GAMESTATE->CountNotesSeparately()) {
			for (auto jTrack = 0; jTrack < m_NoteData.GetNumTracks();
				 ++jTrack) {
				auto tn = m_NoteData.FindTapNote(jTrack, iRow);
				if (tn != m_NoteData.end(jTrack) && jTrack == iTrack) {
					tn->second.PushSelf(L);
					lua_rawseti(L, -3, jTrack + 1);
				} else {
					int iHeadRow;
					if (m_NoteData.IsHoldNoteAtRow(jTrack, iRow, &iHeadRow)) {
						auto hold = m_NoteData.FindTapNote(jTrack, iHeadRow);
						hold->second.PushSelf(L);
						lua_rawseti(L, -2, jTrack + 1);
					}
				}
			}
		} else {
			for (auto jTrack = 0; jTrack < m_NoteData.GetNumTracks();
				 ++jTrack) {
				auto tn = m_NoteData.FindTapNote(jTrack, iRow);
				if (tn != m_NoteData.end(jTrack)) {
					tn->second.PushSelf(L);
					lua_rawseti(L, -3, jTrack + 1);
				} else {
					int iHeadRow;
					if (m_NoteData.IsHoldNoteAtRow(jTrack, iRow, &iHeadRow)) {
						auto hold = m_NoteData.FindTapNote(jTrack, iHeadRow);
						hold->second.PushSelf(L);
						lua_rawseti(L, -2, jTrack + 1);
					}
				}
			}
		}
		msg.SetParamFromStack(L, "Holds");
		msg.SetParamFromStack(L, "Notes");

		LUA->Release(L);
		MESSAGEMAN->Broadcast(msg);
	}
}

void
Player::SetHoldJudgment(TapNote& tn, int iTrack, int iRow)
{
	ASSERT(iTrack < static_cast<int>(m_vpHoldJudgment.size()));
	if (m_vpHoldJudgment[iTrack] != nullptr) {
		m_vpHoldJudgment[iTrack]->SetHoldJudgment(tn.HoldResult.hns);
	}

	// IMPORTANT:
	// for drops: this row is the row which the drop occurs
	// for misses: this row is the row at the end of the hold
	// for anything on miniholds: this row is the row of the song position.
	// that means for miniholds, row can be later than the actual hold end
	AddHoldToReplayData(iTrack, &tn, iRow);

	if (m_bSendJudgmentAndComboMessages) {
		Message msg("Judgment");
		msg.SetParam("Player", m_pPlayerState->m_PlayerNumber);
		msg.SetParam("MultiPlayer", m_pPlayerState->m_mp);
		msg.SetParam("FirstTrack", iTrack);
		msg.SetParam("NumTracks", static_cast<int>(m_vpHoldJudgment.size()));
		msg.SetParam("TapNoteScore", tn.result.tns);
		msg.SetParam("HoldNoteScore", tn.HoldResult.hns);
		msg.SetParam("Judgment", tn.HoldResult.hns);
		msg.SetParam("Type", std::string("Hold"));
		if (m_pPlayerStageStats != nullptr) {
			msg.SetParam(
			  "Val",
			  m_pPlayerStageStats->m_iHoldNoteScores[tn.HoldResult.hns] + 1);

			// Ms scoring implemenation - Mina
			if (tn.HoldResult.hns == HNS_LetGo ||
				tn.HoldResult.hns == HNS_Missed) {
				curwifescore += wife3_hold_drop_weight;
			}

			msg.SetParam("WifePercent", 100 * curwifescore / maxwifescore);
			msg.SetParam("CurWifeScore", curwifescore);
			msg.SetParam("MaxWifeScore", maxwifescore);
			msg.SetParam("WifeDifferential",
						 curwifescore -
						   maxwifescore * m_pPlayerState->playertargetgoal);
			msg.SetParam("TotalPercent", 100 * curwifescore / totalwifescore);
			if (wifescorepersonalbest != m_pPlayerState->playertargetgoal) {
				msg.SetParam("WifePBDifferential",
							 curwifescore -
							   maxwifescore * wifescorepersonalbest);
				msg.SetParam("WifePBGoal", wifescorepersonalbest);
			}
#ifdef autoplayISHUMAN
			m_pPlayerStageStats->m_fWifeScore = curwifescore / totalwifescore;
			ChangeWifeRecord();
#else
			if (m_pPlayerState->m_PlayerController == PC_HUMAN ||
				m_pPlayerState->m_PlayerController == PC_REPLAY) {
				m_pPlayerStageStats->m_fWifeScore =
				  curwifescore / totalwifescore;
				m_pPlayerStageStats->CurWifeScore = curwifescore;
				m_pPlayerStageStats->MaxWifeScore = maxwifescore;
			}

#endif
		}

		auto* L = LUA->Get();
		tn.PushSelf(L);
		msg.SetParamFromStack(L, "TapNote");
		LUA->Release(L);

		MESSAGEMAN->Broadcast(msg);
	}
}

void
Player::SetCombo(unsigned int iCombo, unsigned int iMisses)
{
	if (!m_bSeenComboYet) // first update, don't set bIsMilestone=true
	{
		m_bSeenComboYet = true;
		m_iLastSeenCombo = iCombo;
	}

	auto b25Milestone = false;
	auto b50Milestone = false;
	auto b100Milestone = false;
	auto b250Milestone = false;
	auto b1000Milestone = false;

#define MILESTONE_CHECK(amount)                                                \
	((iCombo / (amount)) > (m_iLastSeenCombo / (amount)))
	if (m_iLastSeenCombo < 600) {
		b25Milestone = MILESTONE_CHECK(25);
		b50Milestone = MILESTONE_CHECK(50);
		b100Milestone = MILESTONE_CHECK(100);
		b250Milestone = MILESTONE_CHECK(250);
		b1000Milestone = MILESTONE_CHECK(1000);
	} else {
		b1000Milestone = MILESTONE_CHECK(1000);
	}
#undef MILESTONE_CHECK

	m_iLastSeenCombo = iCombo;

	if (b25Milestone) {
		this->PlayCommand("TwentyFiveMilestone");
	}
	if (b50Milestone) {
		this->PlayCommand("FiftyMilestone");
	}
	if (b100Milestone) {
		this->PlayCommand("HundredMilestone");
	}
	if (b250Milestone) {
		this->PlayCommand("TwoHundredFiftyMilestone");
	}
	if (b1000Milestone) {
		this->PlayCommand("ThousandMilestone");
	}

	/* Colored combo logic differs between Songs and Courses.
	 *	Songs:
	 *	The theme decides how far into the song the combo color should
	 *appear. (PERCENT_UNTIL_COLOR_COMBO)
	 *
	 *	Courses:
	 *	PERCENT_UNTIL_COLOR_COMBO refers to how long through the course the
	 *	combo color should appear (scaling to the number of songs). This may
	 *	not be desired behavior, however. -aj
	 *
	 *	TODO: Add a metric that determines Course combo colors logic?
	 *	Or possibly move the logic to a Lua function? -aj */
	auto bPastBeginning = false;

	bPastBeginning =
	  GAMESTATE->m_Position.m_fMusicSeconds >
	  GAMESTATE->m_pCurSteps->GetLengthSeconds() * PERCENT_UNTIL_COLOR_COMBO;

	if (m_bSendJudgmentAndComboMessages) {
		Message msg("Combo");
		if (iCombo != 0U) {
			msg.SetParam("Combo", iCombo);
		}
		if (iMisses != 0U) {
			msg.SetParam("Misses", iMisses);
		}
		if (bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W1)) {
			msg.SetParam("FullComboW1", true);
		}
		if (bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W2)) {
			msg.SetParam("FullComboW2", true);
		}
		if (bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W3)) {
			msg.SetParam("FullComboW3", true);
		}
		if (bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W4)) {
			msg.SetParam("FullComboW4", true);
		}
		this->HandleMessage(msg);
	}
}

void
Player::IncrementComboOrMissCombo(const bool bComboOrMissCombo)
{
	const auto iOldCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const auto iOldMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	if (m_pPlayerStageStats != nullptr) {
		if (bComboOrMissCombo) {
			m_pPlayerStageStats->m_iCurCombo++;
			m_pPlayerStageStats->m_iCurMissCombo = 0;
		} else {
			m_pPlayerStageStats->m_iCurCombo = 0;
			m_pPlayerStageStats->m_iCurMissCombo++;
		}
		SetCombo(m_pPlayerStageStats->m_iCurCombo,
				 m_pPlayerStageStats->m_iCurMissCombo);
	}

	SendComboMessages(iOldCombo, iOldMissCombo);
}

void
Player::RenderAllNotesIgnoreScores()
{
	// Go over every single non empty row and their tracks
	FOREACH_NONEMPTY_ROW_ALL_TRACKS(m_NoteData, row)
	{
		for (auto track = 0; track < m_NoteData.GetNumTracks(); track++) {
			// Find the tapnote we are on
			auto iter = m_NoteData.FindTapNote(track, row);

			// Reset the score so it can be visible
			if (iter != m_NoteData.end(track)) {
				auto* pTN = &iter->second;
				if (pTN->type == TapNoteType_Empty) {
					continue;
				}
				if (pTN->HoldResult.hns != HNS_None) {
					pTN->HoldResult.hns = HNS_None;
					pTN->HoldResult.fLife = 1;
					pTN->HoldResult.iLastHeldRow = -1;
					pTN->HoldResult.bHeld = false;
					pTN->HoldResult.bActive = false;
					pTN->HoldResult.fOverlappedTime = 0;
				}
				if (pTN->result.tns != TNS_None) {
					pTN->result.bHidden = false;
					pTN->result.tns = TNS_None;
				}
			}
		}
	}
	// Draw the results
	m_pNoteField->DrawPrimitives();

	// Now do some magic that makes holds and rolls function properly...
	// Tell every hold/roll that they haven't been touched yet:
	m_iFirstUncrossedRow = -1;
	m_pJudgedRows->Reset(-1);

	for (auto i = 0;
		 i < GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
			   ->m_iColsPerPlayer;
		 ++i) {
		lastHoldHeadsSeconds[i] = -1000.F;
	}

	// Might as well regenerate every single iterator for judging.
	// They get set to the right stuff after the next update.
	SAFE_DELETE(m_pIterNeedsTapJudging);
	m_pIterNeedsTapJudging = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(0, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterNeedsHoldJudging);
	m_pIterNeedsHoldJudging = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(0, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterUncrossedRows);
	m_pIterUncrossedRows = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(0, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterUnjudgedRows);
	m_pIterUnjudgedRows = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(0, MAX_NOTE_ROW));

	SAFE_DELETE(m_pIterUnjudgedMineRows);
	m_pIterUnjudgedMineRows = new NoteData::all_tracks_iterator(
	  m_NoteData.GetTapNoteRangeAllTracks(0, MAX_NOTE_ROW));
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the Player. */
class LunaPlayer : public Luna<Player>
{
  public:
	static auto SetLife(T* p, lua_State* L) -> int
	{
		if (p->m_inside_lua_set_life) {
			luaL_error(L,
					   "Do not call SetLife from inside "
					   "LifeChangedMessageCommand because SetLife causes a "
					   "LifeChangedMessageCommand.");
		}
		p->m_inside_lua_set_life = true;
		p->SetLife(FArg(1));
		p->m_inside_lua_set_life = false;
		COMMON_RETURN_SELF;
	}
	static auto ChangeLife(T* p, lua_State* L) -> int
	{
		if (p->m_inside_lua_set_life) {
			luaL_error(L,
					   "Do not call ChangeLife from inside "
					   "LifeChangedMessageCommand because ChangeLife causes a "
					   "LifeChangedMessageCommand.");
		}
		p->m_inside_lua_set_life = true;
		p->ChangeLife(FArg(1));
		p->m_inside_lua_set_life = false;
		COMMON_RETURN_SELF;
	}
	static auto GetPlayerTimingData(T* p, lua_State* L) -> int
	{
		p->GetPlayerTimingData().PushSelf(L);
		return 1;
	}

	LunaPlayer()
	{
		ADD_METHOD(SetLife);
		ADD_METHOD(ChangeLife);
		ADD_METHOD(GetPlayerTimingData);
	}
};

LUA_REGISTER_DERIVED_CLASS(Player, ActorFrame)
// lua end
