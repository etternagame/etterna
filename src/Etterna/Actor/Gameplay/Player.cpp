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
#include "Etterna/Models/Misc/PlayerAI.h"
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
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/Misc/StageStats.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "HoldJudgment.h"
#include "Etterna/Models/Misc/GamePreferences.h"

void
TimingWindowSecondsInit(size_t /*TimingWindow*/ i,
						std::string& sNameOut,
						float& defaultValueOut);

void
TimingWindowSecondsInit(size_t /*TimingWindow*/ i,
						RString& sNameOut,
						float& defaultValueOut)
{
	sNameOut = "TimingWindowSeconds" + TimingWindowToString((TimingWindow)i);
	switch (i) {
		case TW_W1:
			defaultValueOut = 0.0225f;
			break;
		case TW_W2:
			defaultValueOut = 0.045f;
			break;
		case TW_W3:
			defaultValueOut = 0.090f;
			break;
		case TW_W4:
			defaultValueOut = 0.135f;
			break;
		case TW_W5:
			defaultValueOut = 0.180f;
			break;
		case TW_Mine: // ~same as j5 great, the explanation for this is quite
					  // long but the general
			defaultValueOut = 0.075f; // idea is that mines are more punishing
									  // now so we can give a little back
			break;
		case TW_Hold: // allow enough time to take foot off and put back on
			defaultValueOut = 0.250f;
			break;
		case TW_Roll:
			defaultValueOut = 0.500f;
			break;
		case TW_Checkpoint: // similar to TW_Hold, but a little more
							// strict/accurate to Pump play.
			defaultValueOut = 0.1664f;
			break;
		default:
			FAIL_M(ssprintf("Invalid timing window: %i", static_cast<int>(i)));
	}
}

static Preference<float> m_fTimingWindowScale("TimingWindowScale", 1.0f);
static Preference<float> m_fTimingWindowAdd("TimingWindowAdd", 0);
static Preference1D<float> m_fTimingWindowSeconds(TimingWindowSecondsInit,
												  NUM_TimingWindow);
static Preference<float> m_fTimingWindowJump("TimingWindowJump", 0.25);
static Preference<float> m_fMaxInputLatencySeconds("MaxInputLatencySeconds",
												   0.0);
static Preference<bool> g_bEnableMineSoundPlayback("EnableMineHitSound", true);

/** @brief How much life is in a hold note when you start on it? */
ThemeMetric<float> INITIAL_HOLD_LIFE("Player", "InitialHoldLife");
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

float
Player::GetWindowSeconds(TimingWindow tw)
{
	// mines should have a static hit window across all judges to be
	// logically consistent with the idea that increasing judge should
	// not make any elementof the game easier, so now they do
	if (tw == TW_Mine)
		return 0.075f; // explicit return until i remove this stuff from
					   // prefs.ini

	float fSecs = m_fTimingWindowSeconds[tw];
	fSecs *= m_fTimingWindowScale;
	fSecs += m_fTimingWindowAdd;
	return fSecs;
}

float
Player::GetWindowSecondsCustomScale(TimingWindow tw, float timingScale)
{
	if (tw == TW_Mine)
		return 0.075f;

	float fSecs = m_fTimingWindowSeconds[tw];
	fSecs *= timingScale;
	fSecs += m_fTimingWindowAdd;
	return fSecs;
}

float
Player::GetTimingWindowScale()
{
	return m_fTimingWindowScale;
}

Player::Player(NoteData& nd, bool bVisibleParts)
  : m_NoteData(nd)
{
	m_drawing_notefield_board = false;
	m_bLoaded = false;
	m_inside_lua_set_life = false;

	m_pPlayerState = NULL;
	m_pPlayerStageStats = NULL;
	m_fNoteFieldHeight = 0;

	m_pLifeMeter = NULL;
	m_pPrimaryScoreKeeper = NULL;
	m_pIterNeedsTapJudging = NULL;
	m_pIterNeedsHoldJudging = NULL;
	m_pIterUncrossedRows = NULL;
	m_pIterUnjudgedRows = NULL;
	m_pIterUnjudgedMineRows = NULL;

	totalwifescore = 0;
	m_Timing = NULL;
	m_pActorWithJudgmentPosition = NULL;
	m_pActorWithComboPosition = NULL;
	m_LastTapNoteScore = TNS_None;
	m_iFirstUncrossedRow = -1;
	m_iLastSeenCombo = 0;
	m_bSeenComboYet = false;
	m_bTickHolds = false;

	m_bPaused = false;
	m_bDelay = false;

	m_pNoteField = NULL;
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
	for (unsigned i = 0; i < m_vpHoldJudgment.size(); ++i)
		SAFE_DELETE(m_vpHoldJudgment[i]);
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

	{
		// Init judgment positions
		bool bPlayerUsingBothSides =
		  GAMESTATE->GetCurrentStyle(pPlayerState->m_PlayerNumber)
			->GetUsesCenteredArrows();
		Actor TempJudgment;
		TempJudgment.SetName("Judgment");
		ActorUtil::LoadCommand(TempJudgment, sType, "Transform");

		Actor TempCombo;
		TempCombo.SetName("Combo");
		ActorUtil::LoadCommand(TempCombo, sType, "Transform");

		int iEnabledPlayerIndex = 0;
		int iNumEnabledPlayers = 1;

		for (int i = 0; i < NUM_REVERSE; i++) {
			for (int j = 0; j < NUM_CENTERED; j++) {
				Message msg("Transform");
				msg.SetParam("Player", pPlayerState->m_PlayerNumber);
				msg.SetParam("MultiPlayer", pPlayerState->m_mp);
				msg.SetParam("iEnabledPlayerIndex", iEnabledPlayerIndex);
				msg.SetParam("iNumEnabledPlayers", iNumEnabledPlayers);
				msg.SetParam("bPlayerUsingBothSides", bPlayerUsingBothSides);
				msg.SetParam("bReverse", !!i);
				msg.SetParam("bCentered", !!j);

				TempJudgment.HandleMessage(msg);
				m_tsJudgment[i][j] = TempJudgment.DestTweenState();

				TempCombo.HandleMessage(msg);
				m_tsCombo[i][j] = TempCombo.DestTweenState();
			}
		}
	}

	this->SortByDrawOrder();

	m_pPlayerState = pPlayerState;
	m_pPlayerStageStats = pPlayerStageStats;
	m_pLifeMeter = pLM;
	m_pPrimaryScoreKeeper = pPrimaryScoreKeeper;

	m_iLastSeenCombo = 0;
	m_bSeenComboYet = false;

	// set initial life
	if ((m_pLifeMeter != nullptr) && (m_pPlayerStageStats != nullptr)) {
		float fLife = m_pLifeMeter->GetLife();
		m_pPlayerStageStats->SetLifeRecordAt(
		  fLife, STATSMAN->m_CurStageStats.m_fStepsSeconds);
		// m_pPlayerStageStats->SetWifeRecordAt( 1.f,
		// STATSMAN->m_CurStageStats.m_fStepsSeconds);
	}

	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

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

		ASSERT(GAMESTATE->m_pCurSong != NULL);
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
			fMaxBPM = max(0, fMaxBPM);
		}

		// we can't rely on the displayed BPMs, so manually calculate.
		if (fMaxBPM == 0) {
			float fThrowAway = 0;

			if (M_MOD_HIGH_CAP > 0)
				GAMESTATE->m_pCurSong->m_SongTiming.GetActualBPM(
				  fThrowAway, fMaxBPM, M_MOD_HIGH_CAP);
			else
				GAMESTATE->m_pCurSong->m_SongTiming.GetActualBPM(fThrowAway,
																 fMaxBPM);
		}

		ASSERT(fMaxBPM > 0);
		m_pPlayerState->m_fReadBPM = fMaxBPM;
	}

	float fBalance = GameSoundManager::GetPlayerBalance(pn);
	m_soundMine.SetProperty("Pan", fBalance);

	if (HasVisibleParts()) {
		LuaThreadVariable var(
		  "Player", LuaReference::Create(m_pPlayerState->m_PlayerNumber));
		LuaThreadVariable var2("MultiPlayer",
							   LuaReference::Create(m_pPlayerState->m_mp));

		m_sprCombo.Load(THEME->GetPathG(sType, "combo"));
		m_sprCombo->SetName("Combo");
		m_pActorWithComboPosition = &*m_sprCombo;
		this->AddChild(m_sprCombo);

		// todo: allow for judgments to be loaded per-column a la pop'n?
		// see how HoldJudgments are handled below for an example, though
		// it would need more work. -aj
		m_sprJudgment.Load(THEME->GetPathG(sType, "judgment"));
		m_sprJudgment->SetName("Judgment");
		m_pActorWithJudgmentPosition = &*m_sprJudgment;
		this->AddChild(m_sprJudgment);
	} else {
		m_pActorWithComboPosition = NULL;
		m_pActorWithJudgmentPosition = NULL;
	}

	// Load HoldJudgments
	m_vpHoldJudgment.resize(
	  GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
		->m_iColsPerPlayer);
	lastHoldHeadsSeconds.resize(
	  GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
		->m_iColsPerPlayer);
	for (int i = 0;
		 i < GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
			   ->m_iColsPerPlayer;
		 ++i) {
		m_vpHoldJudgment[i] = NULL;
		// set this reasonably negative because if we don't, the first row of
		// the song doesn't get judged
		// and also it gets changed back to a realistic number after a hold is
		// hit -poco
		lastHoldHeadsSeconds[i] = -1000.f;
	}

	if (HasVisibleParts()) {
		for (int i = 0;
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
bool
Player::NeedsTapJudging(const TapNote& tn)
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
bool
Player::NeedsHoldJudging(const TapNote& tn)
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

static void
GenerateCacheDataStructure(PlayerState* pPlayerState, const NoteData& notes)
{

	pPlayerState->m_CacheDisplayedBeat.clear();

	const vector<TimingSegment*> vScrolls =
	  pPlayerState->GetDisplayedTiming().GetTimingSegments(SEGMENT_SCROLL);

	float displayedBeat = 0.0f;
	float lastRealBeat = 0.0f;
	float lastRatio = 1.0f;
	for (unsigned i = 0; i < vScrolls.size(); i++) {
		ScrollSegment* seg = ToScroll(vScrolls[i]);
		displayedBeat += (seg->GetBeat() - lastRealBeat) * lastRatio;
		lastRealBeat = seg->GetBeat();
		lastRatio = seg->GetRatio();
		CacheDisplayedBeat c = { seg->GetBeat(),
								 displayedBeat,
								 seg->GetRatio() };
		pPlayerState->m_CacheDisplayedBeat.push_back(c);
	}

	pPlayerState->m_CacheNoteStat.clear();

	NoteData::all_tracks_const_iterator it =
	  notes.GetTapNoteRangeAllTracks(0, MAX_NOTE_ROW, true);
	int count = 0, lastCount = 0;
	for (; !it.IsAtEnd(); ++it) {
		for (int t = 0; t < notes.GetNumTracks(); t++) {
			if (notes.GetTapNote(t, it.Row()) != TAP_EMPTY)
				count++;
		}
		CacheNoteStat c = { NoteRowToBeat(it.Row()), lastCount, count };
		lastCount = count;
		pPlayerState->m_CacheNoteStat.push_back(c);
	}
}

void
Player::Load()
{
	m_bLoaded = true;

	// Figured this is probably a little expensive so let's cache it
	m_bTickHolds = GAMESTATE->GetCurrentGame()->m_bTickHolds;

	m_LastTapNoteScore = TNS_None;
	// The editor can start playing in the middle of the song.
	const int iNoteRow = BeatToNoteRow(m_pPlayerState->m_Position.m_fSongBeat);
	m_iFirstUncrossedRow = iNoteRow - 1;
	m_pJudgedRows->Reset(iNoteRow);

	// Make sure c++ bound actor's tweens are reset if they exist
	if (m_sprJudgment)
		m_sprJudgment->PlayCommand("Reset");
	if (m_pPlayerStageStats != nullptr) {
		SetCombo(
		  m_pPlayerStageStats->m_iCurCombo,
		  m_pPlayerStageStats
			->m_iCurMissCombo); // combo can persist between songs and games
	}

	// Mina garbage - Mina
	m_Timing = GAMESTATE->m_pCurSteps->GetTimingData();
	m_Timing->NegStopAndBPMCheck();
	int lastRow = m_NoteData.GetLastRow();
	m_Timing->BuildAndGetEtar(lastRow);

	totalwifescore = m_NoteData.WifeTotalScoreCalc(m_Timing, 0, 1073741824);
	curwifescore = 0.f;
	maxwifescore = 0.f;

	m_NoteData.LogNonEmptyRows(m_Timing);
	nerv = m_NoteData.GetNonEmptyRowVector();
	const vector<float>& etaner = m_Timing->BuildAndGetEtaner(nerv);
	if (m_pPlayerStageStats != nullptr)
		m_pPlayerStageStats->serializednd =
		  m_NoteData.SerializeNoteData(etaner);
	m_NoteData.UnsetSerializedNoteData();

	if (m_pPlayerStageStats != nullptr) {
		// if we can ensure that files that have fakes or warps no longer
		// inflate file rating, we can actually lift this restriction, look into
		// it for 0.70 calc release, related: we can look at solo upload stuff
		// as well
		if (m_Timing->HasWarps() || m_Timing->HasFakes())
			m_pPlayerStageStats->filehadnegbpms = true;

		// check before nomines transform
		if (GAMESTATE->m_pCurSteps->GetRadarValues()[RadarCategory_Mines] > 0)
			m_pPlayerStageStats->filegotmines = true;

		if (GAMESTATE->m_pCurSteps->GetRadarValues()[RadarCategory_Holds] > 0 ||
			GAMESTATE->m_pCurSteps->GetRadarValues()[RadarCategory_Rolls] > 0)
			m_pPlayerStageStats->filegotholds = true;

		// check for lua script load (technically this is redundant a little
		// with negbpm but whatever) -mina
		if (!m_Timing->ValidSequentialAssumption)
			m_pPlayerStageStats->luascriptwasloaded = true;
	}

	const HighScore* pb = SCOREMAN->GetChartPBAt(
	  GAMESTATE->m_pCurSteps->GetChartKey(),
	  GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate);
	if (pb != nullptr)
		wifescorepersonalbest = pb->GetWifeScore();
	else
		wifescorepersonalbest = m_pPlayerState->playertargetgoal;

	if (m_pPlayerStageStats)
		m_pPlayerStageStats->m_fTimingScale = m_fTimingWindowScale;

	/* Apply transforms. */
	NoteDataUtil::TransformNoteData(
	  m_NoteData,
	  *m_Timing,
	  m_pPlayerState->m_PlayerOptions.GetStage(),
	  GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
		->m_StepsType);

	// Generate some cache data structure.
	GenerateCacheDataStructure(m_pPlayerState, m_NoteData);

	int iDrawDistanceAfterTargetsPixels = DRAW_DISTANCE_AFTER_TARGET_PIXELS;
	int iDrawDistanceBeforeTargetsPixels = DRAW_DISTANCE_BEFORE_TARGET_PIXELS;

	float fNoteFieldMiddle =
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
	std::string sSongDir = pSong->GetSongDir();
	m_vKeysounds.resize(pSong->m_vsKeysoundFile.size());

	// parameters are invalid somehow... -aj
	RageSoundLoadParams SoundParams;
	SoundParams.m_bSupportPan = true;

	float fBalance = GameSoundManager::GetPlayerBalance(PLAYER_1);
	for (unsigned i = 0; i < m_vKeysounds.size(); i++) {
		std::string sKeysoundFilePath = sSongDir + pSong->m_vsKeysoundFile[i];
		RageSound& sound = m_vKeysounds[i];
		if (sound.GetLoadedFilePath() != sKeysoundFilePath)
			sound.Load(sKeysoundFilePath, true, &SoundParams);
		sound.SetProperty("Pan", fBalance);
		sound.SetStopModeFromString("stop");
	}

	if (m_pPlayerStageStats != nullptr)
		SendComboMessages(m_pPlayerStageStats->m_iCurCombo,
						  m_pPlayerStageStats->m_iCurMissCombo);

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

	const int iNoteRow = BeatToNoteRow(m_pPlayerState->m_Position.m_fSongBeat);
	m_iFirstUncrossedRow = iNoteRow - 1;
	m_pJudgedRows->Reset(iNoteRow);

	// Make sure c++ bound actor's tweens are reset if they exist
	if (m_sprJudgment)
		m_sprJudgment->PlayCommand("Reset");
	if (m_pPlayerStageStats != nullptr) {
		SetCombo(
		  m_pPlayerStageStats->m_iCurCombo,
		  m_pPlayerStageStats
			->m_iCurMissCombo); // combo can persist between songs and games
	}

	curwifescore = 0.f;
	maxwifescore = 0.f;

	if (m_pPlayerStageStats != nullptr)
		SendComboMessages(m_pPlayerStageStats->m_iCurCombo,
						  m_pPlayerStageStats->m_iCurMissCombo);

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
Player::SendComboMessages(unsigned int iOldCombo, unsigned int iOldMissCombo)
{
	const unsigned int iCurCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	if (iOldCombo > (unsigned int)COMBO_STOPPED_AT &&
		iCurCombo < (unsigned int)COMBO_STOPPED_AT) {
		SCREENMAN->PostMessageToTopScreen(SM_ComboStopped, 0);
	}

	if (m_bSendJudgmentAndComboMessages) {
		Message msg("ComboChanged");
		msg.SetParam("OldCombo", iOldCombo);
		msg.SetParam("OldMissCombo", iOldMissCombo);
		if (m_pPlayerState) {
			msg.SetParam("Player", m_pPlayerState->m_PlayerNumber);
			msg.SetParam("PlayerState",
						 LuaReference::CreateFromPush(*m_pPlayerState));
		}
		if (m_pPlayerStageStats)
			msg.SetParam("PlayerStageStats",
						 LuaReference::CreateFromPush(*m_pPlayerStageStats));
		MESSAGEMAN->Broadcast(msg);
	}
}

void
Player::UpdateVisibleParts()
{
	// Optimization: Don't spend time processing the things below that won't
	// show if the Player doesn't show anything on the screen.
	if (!HasVisibleParts())
		return;

	float fMiniPercent = m_pPlayerState->m_PlayerOptions.GetCurrent()
						   .m_fEffects[PlayerOptions::EFFECT_MINI];
	float fTinyPercent = m_pPlayerState->m_PlayerOptions.GetCurrent()
						   .m_fEffects[PlayerOptions::EFFECT_TINY];
	float fJudgmentZoom = min(powf(0.5f, fMiniPercent + fTinyPercent), 1.0f);

	// Update Y positions
	{
		for (int c = 0;
			 c < GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
				   ->m_iColsPerPlayer;
			 c++) {
			float fPercentReverse = m_pPlayerState->m_PlayerOptions.GetCurrent()
									  .GetReversePercentForColumn(c);
			float fHoldJudgeYPos = SCALE(fPercentReverse,
										 0.f,
										 1.f,
										 HOLD_JUDGMENT_Y_STANDARD,
										 HOLD_JUDGMENT_Y_REVERSE);
			// float fGrayYPos = SCALE( fPercentReverse, 0.f, 1.f,
			// GRAY_ARROWS_Y_STANDARD, GRAY_ARROWS_Y_REVERSE );

			float fX = ArrowEffects::GetXPos(m_pPlayerState, c, 0);
			const float fZ = ArrowEffects::GetZPos(c, 0);
			fX *= (1 - fMiniPercent * 0.5f);

			m_vpHoldJudgment[c]->SetX(fX);
			m_vpHoldJudgment[c]->SetY(fHoldJudgeYPos);
			m_vpHoldJudgment[c]->SetZ(fZ);
			m_vpHoldJudgment[c]->SetZoom(fJudgmentZoom);
		}
	}

	// NoteField accounts for reverse on its own now.
	// if( m_pNoteField )
	//	m_pNoteField->SetY( fGrayYPos );

	const bool bReverse =
	  m_pPlayerState->m_PlayerOptions.GetCurrent().GetReversePercentForColumn(
		0) == 1;
	float fPercentCentered = m_pPlayerState->m_PlayerOptions.GetCurrent()
							   .m_fScrolls[PlayerOptions::SCROLL_CENTERED];

	if (m_pActorWithJudgmentPosition != NULL) {
		const Actor::TweenState& ts1 = m_tsJudgment[bReverse ? 1 : 0][0];
		const Actor::TweenState& ts2 = m_tsJudgment[bReverse ? 1 : 0][1];
		Actor::TweenState::MakeWeightedAverage(
		  m_pActorWithJudgmentPosition->DestTweenState(),
		  ts1,
		  ts2,
		  fPercentCentered);
	}

	if (m_pActorWithComboPosition != NULL) {
		const Actor::TweenState& ts1 = m_tsCombo[bReverse ? 1 : 0][0];
		const Actor::TweenState& ts2 = m_tsCombo[bReverse ? 1 : 0][1];
		Actor::TweenState::MakeWeightedAverage(
		  m_pActorWithComboPosition->DestTweenState(),
		  ts1,
		  ts2,
		  fPercentCentered);
	}

	float fNoteFieldZoom = 1 - fMiniPercent * 0.5f;
	if (m_pNoteField)
		m_pNoteField->SetZoom(fNoteFieldZoom);
	if (m_pActorWithJudgmentPosition != NULL)
		m_pActorWithJudgmentPosition->SetZoom(
		  m_pActorWithJudgmentPosition->GetZoom() * fJudgmentZoom);
	if (m_pActorWithComboPosition != NULL)
		m_pActorWithComboPosition->SetZoom(
		  m_pActorWithComboPosition->GetZoom() * fJudgmentZoom);
}

void
Player::UpdatePressedFlags()
{
	const int iNumCols =
	  GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
		->m_iColsPerPlayer;
	ASSERT_M(iNumCols <= MAX_COLS_PER_PLAYER,
			 ssprintf("%i > %i", iNumCols, MAX_COLS_PER_PLAYER));
	for (int col = 0; col < iNumCols; ++col) {
		ASSERT(m_pPlayerState != NULL);

		// TODO: Remove use of PlayerNumber.
		vector<GameInput> GameI;
		GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
		  ->StyleInputToGameInput(col, m_pPlayerState->m_PlayerNumber, GameI);

		bool bIsHoldingButton = INPUTMAPPER->IsBeingPressed(GameI);

		// TODO: Make this work for non-human-controlled players
		if (bIsHoldingButton && m_pPlayerState->m_PlayerController == PC_HUMAN)
			if (m_pNoteField != nullptr)
				m_pNoteField->SetPressed(col);
	}
}

void
Player::UpdateHoldsAndRolls(float fDeltaTime,
							const std::chrono::steady_clock::time_point& now)
{
	const float fSongBeat = m_pPlayerState->m_Position.m_fSongBeat;
	const int iSongRow = BeatToNoteRow(fSongBeat);
	// handle Autoplay for rolls
	if (m_pPlayerState->m_PlayerController != PC_HUMAN) {
		for (int iTrack = 0; iTrack < m_NoteData.GetNumTracks(); ++iTrack) {
			// TODO: Make the CPU miss sometimes.
			int iHeadRow;
			if (!m_NoteData.IsHoldNoteAtRow(iTrack, iSongRow, &iHeadRow))
				iHeadRow = iSongRow;

			const TapNote& tn = m_NoteData.GetTapNote(iTrack, iHeadRow);
			if (tn.type != TapNoteType_HoldHead ||
				tn.subType != TapNoteSubType_Roll)
				continue;
			if (tn.HoldResult.hns != HNS_None)
				continue;
			if (tn.HoldResult.fLife >= 0.5f)
				continue;

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
			NoteData::all_tracks_iterator& iter = *m_pIterNeedsHoldJudging;
			while (!iter.IsAtEnd() && iter.Row() <= iSongRow &&
				   !NeedsHoldJudging(*iter))
				++iter;
		}

		vector<TrackRowTapNote> vHoldNotesToGradeTogether;
		int iRowOfLastHoldNote = -1;
		NoteData::all_tracks_iterator iter = *m_pIterNeedsHoldJudging; // copy
		for (; !iter.IsAtEnd() && iter.Row() <= iSongRow; ++iter) {
			TapNote& tn = *iter;
			if (tn.type != TapNoteType_HoldHead)
				continue;

			int iTrack = iter.Track();
			int iRow = iter.Row();
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
					vector<TrackRowTapNote> v;
					v.push_back(trtn);
					UpdateHoldNotes(iSongRow, fDeltaTime, v);
				}
					continue; // don't process this below
			}

			if (!vHoldNotesToGradeTogether.empty()) {
				// LOG->Trace( ssprintf("UpdateHoldNotes; %i != %i || !judge
				// holds on same row together",iRow,iRowOfLastHoldNote) );
				UpdateHoldNotes(
				  iSongRow, fDeltaTime, vHoldNotesToGradeTogether);
				vHoldNotesToGradeTogether.clear();
			}
			iRowOfLastHoldNote = iRow;
			vHoldNotesToGradeTogether.push_back(trtn);
		}

		if (!vHoldNotesToGradeTogether.empty()) {
			// LOG->Trace("UpdateHoldNotes since
			// !vHoldNotesToGradeTogether.empty()");
			UpdateHoldNotes(iSongRow, fDeltaTime, vHoldNotesToGradeTogether);
			vHoldNotesToGradeTogether.clear();
		}
	}
}

void
Player::UpdateCrossedRows(const std::chrono::steady_clock::time_point& now)
{
	const int iRowNow = BeatToNoteRow(m_pPlayerState->m_Position.m_fSongBeat);
	if (iRowNow >= 0) {
		if (GAMESTATE->IsPlayerEnabled(m_pPlayerState)) {
			if (m_pPlayerState->m_Position.m_bDelay) {
				if (!m_bDelay)
					m_bDelay = true;
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
	if (!m_bLoaded)
		return;

	// LOG->Trace( "Player::Update(%f)", fDeltaTime );

	if (GAMESTATE->m_pCurSong == NULL)
		return;

	ActorFrame::Update(fDeltaTime);

	if (m_pPlayerState->m_mp != MultiPlayer_Invalid) {
		/* In multiplayer, it takes too long to run player updates for every
		 * player each frame; with 32 players and three difficulties, we have 96
		 * Players to update.  Stagger these updates, by only updating a few
		 * players each update; since we don't have screen elements tightly tied
		 * to user actions in this mode, this doesn't degrade gameplay.  Run 4
		 * players per update, which means 12 Players in 3-difficulty mode.
		 */
		static int iCycle = 0;
		iCycle = (iCycle + 1) % 8;

		if ((m_pPlayerState->m_mp % 8) != iCycle)
			return;
	}

	ArrowEffects::SetCurrentOptions(
	  &m_pPlayerState->m_PlayerOptions.GetCurrent());

	// Tell the NoteField and other visible C++ Actors to update
	UpdateVisibleParts();

	// If we're paused, don't update tap or hold note logic, so hold notes can
	// be released during pause.
	if (m_bPaused)
		return;

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
						vector<TrackRowTapNote>& vTN)
{
	ASSERT(!vTN.empty());

	// LOG->Trace("--------------------------------");
	/*
	LOG->Trace("[Player::UpdateHoldNotes] begins");
	LOG->Trace( ssprintf("song row %i, deltaTime = %f",iSongRow,fDeltaTime) );
	*/

	int iStartRow = vTN[0].iRow;
	int iMaxEndRow = INT_MIN;
	int iFirstTrackWithMaxEndRow = -1;

	TapNoteSubType subType = TapNoteSubType_Invalid;
	FOREACH(TrackRowTapNote, vTN, trtn)
	{
		int iTrack = trtn->iTrack;
		ASSERT(iStartRow == trtn->iRow);
		TapNote& tn = *trtn->pTN;
		int iEndRow = iStartRow + tn.iDuration;
		if (subType == TapNoteSubType_Invalid)
			subType = tn.subType;

		/* All holds must be of the same subType because fLife is handled
		 * in different ways depending on the SubType. */
		ASSERT(tn.subType == subType);

		if (iEndRow > iMaxEndRow) {
			iMaxEndRow = iEndRow;
			iFirstTrackWithMaxEndRow = iTrack;
		}
	}

	ASSERT(iFirstTrackWithMaxEndRow != -1);
	// LOG->Trace( ssprintf("start row: %i; max/end row: =
	// %i",iStartRow,iMaxEndRow) );  LOG->Trace( ssprintf("first track with max
	// end row = %i",iFirstTrackWithMaxEndRow) );  LOG->Trace( ssprintf("max end
	// row - start row (in beats) =
	// %f",NoteRowToBeat(iMaxEndRow)-NoteRowToBeat(iStartRow)) );

	FOREACH(TrackRowTapNote, vTN, trtn)
	{
		TapNote& tn = *trtn->pTN;

		// set hold flags so NoteField can do intelligent drawing
		tn.HoldResult.bHeld = false;
		tn.HoldResult.bActive = false;

		int iRow = trtn->iRow;
		// LOG->Trace( ssprintf("this row: %i",iRow) );

		// If the song beat is in the range of this hold:
		if (iRow <= iSongRow && iRow <= iMaxEndRow) {
			// LOG->Trace( ssprintf("overlap time before:
			// %f",tn.HoldResult.fOverlappedTime) );
			tn.HoldResult.fOverlappedTime += fDeltaTime;
			// LOG->Trace( ssprintf("overlap time after:
			// %f",tn.HoldResult.fOverlappedTime) );
		} else {
			// LOG->Trace( "overlap time = 0" );
			tn.HoldResult.fOverlappedTime = 0;
		}
	}

	HoldNoteScore hns = vTN[0].pTN->HoldResult.hns;
	float fLife = vTN[0].pTN->HoldResult.fLife;

	if (hns != HNS_None) // if this HoldNote already has a result
	{
		// LOG->Trace("hold note has a result, skipping.");
		return; // we don't need to update the logic for this group
	}

	// LOG->Trace("hold note doesn't already have result, let's check.");

	// LOG->Trace( ssprintf("[C++] hold note score:
	// %s",HoldNoteScoreToString(hns).c_str()) );
	// LOG->Trace(ssprintf("[Player::UpdateHoldNotes] fLife = %f",fLife));

	bool bSteppedOnHead = true;
	bool bHeadJudged = true;
	FOREACH(TrackRowTapNote, vTN, trtn)
	{
		TapNote& tn = *trtn->pTN;
		TapNoteScore tns = tn.result.tns;
		// LOG->Trace( ssprintf("[C++] tap note score:
		// %s",StringConversion::ToString(tns).c_str()) );

		// TODO: When using JUDGE_HOLD_NOTES_ON_SAME_ROW_TOGETHER, require that
		// the whole row of taps was hit before activating this group of holds.
		/* Something about the logic in this section is causing 192nd steps to
		 * fail for some odd reason. -aj */
		// Nah, lets just forget about judging all holds/taps at once like that
		// because we are in the cc off era now :)
		bSteppedOnHead &=
		  (tns != TNS_Miss &&
		   tns != TNS_None); // did they step on the start of this hold?
		bHeadJudged &=
		  (tns != TNS_None); // has this hold really even started yet?

		/*
		if(bSteppedOnHead)
			LOG->Trace("[Player::UpdateHoldNotes] player stepped on head");
		else
			LOG->Trace("[Player::UpdateHoldNotes] player didn't step on the
		head");
		*/
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

	bool bIsHoldingButton = true;
	FOREACH(TrackRowTapNote, vTN, trtn)
	{
		/*if this hold is already done, pretend it's always being pressed.
		fixes/masks the phantom hold issue. -FSX*/
		// That interacts badly with !IMMEDIATE_HOLD_LET_GO,
		// causing ALL holds to be judged HNS_Held whether they were or not.
		if (!IMMEDIATE_HOLD_LET_GO ||
			(iStartRow + trtn->pTN->iDuration) > iSongRow) {
			int iTrack = trtn->iTrack;

			if (m_pPlayerState->m_PlayerController != PC_HUMAN) {
				// TODO: Make the CPU miss sometimes.
				if (m_pPlayerState->m_PlayerController == PC_AUTOPLAY) {
					STATSMAN->m_CurStageStats.m_bUsedAutoplay = true;
					if (m_pPlayerStageStats != nullptr) {
						m_pPlayerStageStats->m_bDisqualified = true;
						m_pPlayerStageStats->everusedautoplay = true;
					}
				}
			} else {
				vector<GameInput> GameI;
				GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
				  ->StyleInputToGameInput(iTrack, PLAYER_1, GameI);

				bIsHoldingButton &=
				  INPUTMAPPER->IsBeingPressed(GameI, m_pPlayerState->m_mp);
			}
		}
	}

	if (bInitiatedNote && fLife != 0 && bHeadJudged) {
		// LOG->Trace("[Player::UpdateHoldNotes] initiated note, fLife != 0");
		/* This hold note is not judged and we stepped on its head.
		 * Update iLastHeldRow. Do this even if we're a little beyond the end
		 * of the hold note, to make sure iLastHeldRow is clamped to iEndRow
		 * if the hold note is held all the way. */
		FOREACH(TrackRowTapNote, vTN, trtn)
		{
			TapNote& tn = *trtn->pTN;
			int iEndRow = iStartRow + tn.iDuration;

			// LOG->Trace(ssprintf("trying for min between iSongRow (%i) and
			// iEndRow (%i) (duration %i)",iSongRow,iEndRow,tn.iDuration));
			trtn->pTN->HoldResult.iLastHeldRow = min(iSongRow, iEndRow);
		}
	}

	// If the song beat is in the range of this hold:
	if (iStartRow <= iSongRow && iStartRow <= iMaxEndRow && bHeadJudged) {
		switch (subType) {
			case TapNoteSubType_Hold:
				FOREACH(TrackRowTapNote, vTN, trtn)
				{
					TapNote& tn = *trtn->pTN;

					// set hold flag so NoteField can do intelligent drawing
					tn.HoldResult.bHeld = bIsHoldingButton && bInitiatedNote;
					tn.HoldResult.bActive = bInitiatedNote;
				}

				if (bInitiatedNote && bIsHoldingButton) {
					// LOG->Trace("bInitiatedNote && bIsHoldingButton;
					// Increase life
					fLife = 1;
				} else {
					/*
					LOG->Trace("Checklist:");
					if(bInitiatedNote)
						LOG->Trace("[X] Initiated Note");
					else
						LOG->Trace("[ ] Initiated Note");

					if(bIsHoldingButton)
						LOG->Trace("[X] Holding Button");
					else
						LOG->Trace("[ ] Holding Button");
					*/

					TimingWindow window =
					  m_bTickHolds ? TW_Checkpoint : TW_Hold;
					// LOG->Trace("fLife before minus: %f",fLife);
					fLife -= fDeltaTime / GetWindowSeconds(window);
					// LOG->Trace("fLife before clamp: %f",fLife);
					fLife = max(0, fLife);
					// LOG->Trace("fLife after: %f",fLife);
				}
				break;
			case TapNoteSubType_Roll:
				FOREACH(TrackRowTapNote, vTN, trtn)
				{
					TapNote& tn = *trtn->pTN;
					tn.HoldResult.bHeld = true;
					tn.HoldResult.bActive = bInitiatedNote;
				}

				// give positive life in Step(), not here.

				// Decrease life
				// Also clamp the roll decay window to the accepted "Judge 7"
				// value for it. -poco
				fLife -= fDeltaTime / max(GetWindowSeconds(TW_Roll), 0.25f);
				fLife = max(fLife, 0); // clamp life
				break;
			/*
			case TapNoteSubType_Mine:
				break;
			*/
			default:
				FAIL_M(ssprintf("Invalid tap note subtype: %i", subType));
		}
	}

	// TODO: Cap the active time passed to the score keeper to the actual start
	// time and end time of the hold.
	if (vTN[0].pTN->HoldResult.bActive) {
		float fSecondsActiveSinceLastUpdate =
		  fDeltaTime * GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		if (m_pPrimaryScoreKeeper != nullptr)
			m_pPrimaryScoreKeeper->HandleHoldActiveSeconds(
			  fSecondsActiveSinceLastUpdate);
	}

	// check for LetGo. If the head was missed completely, don't count an LetGo.
	/* Why? If you never step on the head, then it will be left as HNS_None,
	 * which doesn't seem correct. */
	if (IMMEDIATE_HOLD_LET_GO) {
		if (bInitiatedNote && fLife == 0 && bHeadJudged) // the player has not
														 // pressed the button
														 // for a long time!
		{
			// LOG->Trace("LetGo from life == 0 (did initiate hold)");
			hns = HNS_LetGo;
		}
	}

	// score hold notes that have passed
	if (iSongRow >= iMaxEndRow && bHeadJudged) {
		bool bLetGoOfHoldNote = false;

		/* Score rolls that end with fLife == 0 as LetGo, even if
		 * m_bTickHolds is on. Rolls don't have iCheckpointsMissed set, so,
		 * unless we check Life == 0, rolls would always be scored as Held. */
		bool bAllowHoldCheckpoints;
		switch (subType) {
			DEFAULT_FAIL(subType);
			case TapNoteSubType_Hold:
				bAllowHoldCheckpoints = true;
				break;
			case TapNoteSubType_Roll:
				bAllowHoldCheckpoints = false;
				break;
				/*
				case TapNoteSubType_Mine:
					bAllowHoldCheckpoints = true;
					break;
				*/
		}

		if (m_bTickHolds && bAllowHoldCheckpoints) {
			// LOG->Trace("(hold checkpoints are allowed and enabled.)");
			int iCheckpointsHit = 0;
			int iCheckpointsMissed = 0;
			FOREACH(TrackRowTapNote, vTN, v)
			{
				iCheckpointsHit += v->pTN->HoldResult.iCheckpointsHit;
				iCheckpointsMissed += v->pTN->HoldResult.iCheckpointsMissed;
			}
			bLetGoOfHoldNote = iCheckpointsMissed > 0 || iCheckpointsHit == 0;

			// TRICKY: If the hold is so short that it has no checkpoints,
			// then mark it as Held if the head was stepped on.
			if (iCheckpointsHit == 0 && iCheckpointsMissed == 0)
				bLetGoOfHoldNote = !bSteppedOnHead;

			/*
			if(bLetGoOfHoldNote)
				LOG->Trace("let go of hold note, life is 0");
			else
				LOG->Trace("did not let go of hold note :D");
			*/
		} else {
			// LOG->Trace("(hold checkpoints disabled.)");
			bLetGoOfHoldNote = fLife == 0;
			/*
			if(bLetGoOfHoldNote)
				LOG->Trace("let go of hold note, life is 0");
			else
				LOG->Trace("did not let go of hold note :D");
			*/
		}

		if (bInitiatedNote) {
			if (!bLetGoOfHoldNote) {
				// LOG->Trace("initiated note and didn't let go");
				fLife = 1;
				hns = HNS_Held;
				bool bBright = m_pPlayerStageStats &&
							   m_pPlayerStageStats->m_iCurCombo >
								 (unsigned int)BRIGHT_GHOST_COMBO_THRESHOLD;
				if (m_pNoteField != nullptr) {
					FOREACH(TrackRowTapNote, vTN, trtn)
					{
						int iTrack = trtn->iTrack;
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

	float fLifeFraction = fLife / 1; // haha im just gonna leave this here

	FOREACH(TrackRowTapNote, vTN, trtn)
	{
		TapNote& tn = *trtn->pTN;
		tn.HoldResult.fLife = fLife;
		tn.HoldResult.hns = hns;
		// Stop the playing keysound for the hold note.
		// I think this causes crashes too. -aj
		// This can still crash. I think it expects a full game and quit before
		// the preference works: otherwise, it causes problems on holds. At
		// least, that hapened on my Mac. -wolfman2000

		static Preference<float>* pVolume =
		  Preference<float>::GetPreferenceByName("SoundVolume");
		if (pVolume != NULL) {
			static float fVol = pVolume->Get();

			if (tn.iKeysoundIndex >= 0 &&
				tn.iKeysoundIndex < static_cast<int>(m_vKeysounds.size())) {
				float factor = (tn.subType == TapNoteSubType_Roll
								  ? 2.0f * fLifeFraction
								  : 10.0f * fLifeFraction - 8.5f);
				m_vKeysounds[tn.iKeysoundIndex].SetProperty(
				  "Volume", max(0.0f, min(1.0f, factor)) * fVol);
			}
		}
	}

	if ((hns == HNS_LetGo) && COMBO_BREAK_ON_IMMEDIATE_HOLD_LET_GO)
		IncrementMissCombo();

	if (hns != HNS_None) {
		// LOG->Trace("tap note scoring time.");
		TapNote& tn = *vTN[0].pTN;
		SetHoldJudgment(tn, iFirstTrackWithMaxEndRow, iSongRow);
		HandleHoldScore(tn);
		// LOG->Trace("hold result =
		// %s",StringConversion::ToString(tn.HoldResult.hns).c_str());
	}
	// LOG->Trace("[Player::UpdateHoldNotes] ends");
}

void
Player::DrawPrimitives()
{
	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	if (pn != PLAYER_1)
		return;

	bool draw_notefield = (m_pNoteField != nullptr);

	const PlayerOptions& curr_options =
	  m_pPlayerState->m_PlayerOptions.GetCurrent();
	float tilt = curr_options.m_fPerspectiveTilt;
	float skew = curr_options.m_fSkew;
	float mini = curr_options.m_fEffects[PlayerOptions::EFFECT_MINI];
	float center_y =
	  GetY() + (GRAY_ARROWS_Y_STANDARD + GRAY_ARROWS_Y_REVERSE) / 2;
	bool reverse = curr_options.GetReversePercentForColumn(0) > .5;

	if (m_drawing_notefield_board) {
		// Ask the Notefield to draw its board primitive before everything else
		// so that things drawn under the field aren't behind the opaque board.
		// -Kyz
		if (draw_notefield) {
			PlayerNoteFieldPositioner poser(
			  this, GetX(), tilt, skew, mini, center_y, reverse);
			m_pNoteField->DrawBoardPrimitive();
		}
		return;
	}

	// Draw these below everything else.
	if (COMBO_UNDER_FIELD && curr_options.m_fBlind == 0) {
		if (m_sprCombo != nullptr)
			m_sprCombo->Draw();
	}

	if (TAP_JUDGMENTS_UNDER_FIELD)
		DrawTapJudgments();

	if (HOLD_JUDGMENTS_UNDER_FIELD)
		DrawHoldJudgments();

	if (draw_notefield) {
		PlayerNoteFieldPositioner poser(
		  this, GetX(), tilt, skew, mini, center_y, reverse);
		m_pNoteField->Draw();
	}

	// m_pNoteField->m_sprBoard->GetVisible()
	if (!COMBO_UNDER_FIELD && curr_options.m_fBlind == 0)
		if (m_sprCombo != nullptr)
			m_sprCombo->Draw();

	if (!(bool)TAP_JUDGMENTS_UNDER_FIELD)
		DrawTapJudgments();

	if (!(bool)HOLD_JUDGMENTS_UNDER_FIELD)
		DrawHoldJudgments();
}

void
Player::PushPlayerMatrix(float x, float skew, float center_y)
{
	DISPLAY->CameraPushMatrix();
	DISPLAY->PushMatrix();
	DISPLAY->LoadMenuPerspective(45,
								 SCREEN_WIDTH,
								 SCREEN_HEIGHT,
								 SCALE(skew, 0.1f, 1.0f, x, SCREEN_CENTER_X),
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
	int reverse_mult = (reverse ? -1 : 1);
	original_y = player->m_pNoteField->GetY();
	float tilt_degrees = SCALE(tilt, -1.f, +1.f, +30, -30) * reverse_mult;
	float zoom = SCALE(mini, 0.f, 1.f, 1.f, .5f);
	// Something strange going on here.  Notice that the range for tilt's
	// effect on y_offset goes to -45 when positive, but -20 when negative.
	// I don't know why it's done this why, simply preserving old behavior.
	// -Kyz
	if (tilt > 0) {
		zoom *= SCALE(tilt, 0.f, 1.f, 1.f, 0.9f);
		y_offset = SCALE(tilt, 0.f, 1.f, 0.f, -45.f) * reverse_mult;
	} else {
		zoom *= SCALE(tilt, 0.f, -1.f, 1.f, 0.9f);
		y_offset = SCALE(tilt, 0.f, -1.f, 0.f, -20.f) * reverse_mult;
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
	if (m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind > 0)
		return;

	if (m_sprJudgment != nullptr)
		m_sprJudgment->Draw();
}

void
Player::DrawHoldJudgments()
{
	if (m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind > 0)
		return;

	for (int c = 0; c < m_NoteData.GetNumTracks(); c++)
		if (m_vpHoldJudgment[c])
			m_vpHoldJudgment[c]->Draw();
}

void
Player::ChangeLife(TapNoteScore tns)
{
	if (m_pLifeMeter != nullptr)
		m_pLifeMeter->ChangeLife(tns);

	ChangeLifeRecord();
}

void
Player::ChangeLife(HoldNoteScore hns, TapNoteScore tns)
{
	if (m_pLifeMeter != nullptr)
		m_pLifeMeter->ChangeLife(hns, tns);

	ChangeLifeRecord();
}

void
Player::ChangeLife(float delta)
{
	// If ChangeLifeRecord is not called before the change, then the life graph
	// will show a gradual change from the time of the previous step (or
	// change) to the time of this change, instead of the sharp change that
	// actually occurred. -Kyz
	ChangeLifeRecord();
	if (m_pLifeMeter != nullptr) {
		m_pLifeMeter->ChangeLife(delta);
	}
	ChangeLifeRecord();
}

void
Player::SetLife(float value)
{
	// If ChangeLifeRecord is not called before the change, then the life graph
	// will show a gradual change from the time of the previous step (or
	// change) to the time of this change, instead of the sharp change that
	// actually occurred. -Kyz
	ChangeLifeRecord();
	if (m_pLifeMeter != nullptr) {
		m_pLifeMeter->SetLife(value);
	}
	ChangeLifeRecord();
}

void
Player::ChangeLifeRecord()
{
	float fLife = -1;
	if (m_pLifeMeter != nullptr) {
		fLife = m_pLifeMeter->GetLife();
	}
	if (fLife != -1)
		if (m_pPlayerStageStats)
			m_pPlayerStageStats->SetLifeRecordAt(
			  fLife, STATSMAN->m_CurStageStats.m_fStepsSeconds);
}

void
Player::ChangeWifeRecord()
{
	// Sets the life ... to the wife....
	// That's not right.
	if (m_pPlayerStageStats)
		m_pPlayerStageStats->SetLifeRecordAt(
		  curwifescore / maxwifescore,
		  STATSMAN->m_CurStageStats.m_fStepsSeconds);
}

int
Player::GetClosestNoteDirectional(int col,
								  int iStartRow,
								  int iEndRow,
								  bool bAllowGraded,
								  bool bForward) const
{
	NoteData::const_iterator begin, end;
	m_NoteData.GetTapNoteRange(col, iStartRow, iEndRow, begin, end);

	if (!bForward)
		swap(begin, end);

	while (begin != end) {
		if (!bForward)
			--begin;

		// Is this the row we want?
		do {
			const TapNote& tn = begin->second;
			if (!m_Timing->IsJudgableAtRow(begin->first))
				break;
			// unsure if autoKeysounds should be excluded. -Wolfman2000
			if (tn.type == TapNoteType_Empty ||
				tn.type == TapNoteType_AutoKeysound)
				break;
			if (!bAllowGraded && tn.result.tns != TNS_None)
				break;

			return begin->first;
		} while (0);

		if (bForward)
			++begin;
	}

	return -1;
}

// Find the closest note to fBeat.
int
Player::GetClosestNote(int col,
					   int iNoteRow,
					   int iMaxRowsAhead,
					   int iMaxRowsBehind,
					   bool bAllowGraded,
					   bool bAllowOldMines) const
{
	// Start at iIndexStartLookingAt and search outward.
	int iNextIndex = GetClosestNoteDirectional(
	  col, iNoteRow, iNoteRow + iMaxRowsAhead, bAllowGraded, true);
	int iPrevIndex = GetClosestNoteDirectional(
	  col, iNoteRow - iMaxRowsBehind, iNoteRow, bAllowGraded, false);

	if (iNextIndex == -1 && iPrevIndex == -1)
		return -1;
	if (iNextIndex == -1)
		return iPrevIndex;
	if (iPrevIndex == -1)
		return iNextIndex;

	// Get the current time, previous time, and next time.
	float fNoteTime = m_pPlayerState->m_Position.m_fMusicSeconds;
	float fNextTime = m_Timing->WhereUAtBro(iNextIndex);
	float fPrevTime = m_Timing->WhereUAtBro(iPrevIndex);

	// If we passed a mine, we can't hit it anymore. Literally.
	// So forget about them.
	// RIP Minebug 20xx - 2019
	if (!bAllowOldMines) {
		TapNote* pTN = NULL;
		NoteData::iterator iter = m_NoteData.FindTapNote(col, iPrevIndex);
		DEBUG_ASSERT(iter != m_NoteData.end(col));
		pTN = &iter->second;
		if (pTN->type == TapNoteType_Mine)
			return iNextIndex;
	}

	/* Figure out which row is closer. */
	if (fabsf(fNoteTime - fNextTime) > fabsf(fNoteTime - fPrevTime))
		return iPrevIndex;

	return iNextIndex;
}

int
Player::GetClosestNonEmptyRowDirectional(int iStartRow,
										 int iEndRow,
										 bool /* bAllowGraded */,
										 bool bForward) const
{
	if (bForward) {
		NoteData::all_tracks_iterator iter =
		  m_NoteData.GetTapNoteRangeAllTracks(iStartRow, iEndRow);

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
		NoteData::all_tracks_reverse_iterator iter =
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
int
Player::GetClosestNonEmptyRow(int iNoteRow,
							  int iMaxRowsAhead,
							  int iMaxRowsBehind,
							  bool bAllowGraded) const
{
	// Start at iIndexStartLookingAt and search outward.
	int iNextRow = GetClosestNonEmptyRowDirectional(
	  iNoteRow, iNoteRow + iMaxRowsAhead, bAllowGraded, true);
	int iPrevRow = GetClosestNonEmptyRowDirectional(
	  iNoteRow - iMaxRowsBehind, iNoteRow, bAllowGraded, false);

	if (iNextRow == -1 && iPrevRow == -1)
		return -1;
	if (iNextRow == -1)
		return iPrevRow;
	if (iPrevRow == -1)
		return iNextRow;

	// Get the current time, previous time, and next time.
	float fNoteTime = m_pPlayerState->m_Position.m_fMusicSeconds;
	float fNextTime = m_Timing->WhereUAtBro(iNextRow);
	float fPrevTime = m_Timing->WhereUAtBro(iPrevRow);

	/* Figure out which row is closer. */
	if (fabsf(fNoteTime - fNextTime) > fabsf(fNoteTime - fPrevTime))
		return iPrevRow;

	return iNextRow;
}

void
Player::DoTapScoreNone()
{
	Message msg("ScoreNone");
	MESSAGEMAN->Broadcast(msg);

	const unsigned int iOldCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const unsigned int iOldMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	/* The only real way to tell if a mine has been scored is if it has
	 * disappeared but this only works for hit mines so update the scores for
	 * avoided mines here. */
	if (m_pPrimaryScoreKeeper != nullptr)
		m_pPrimaryScoreKeeper->HandleTapScoreNone();

	SendComboMessages(iOldCombo, iOldMissCombo);

	if (m_pLifeMeter != nullptr)
		m_pLifeMeter->HandleTapScoreNone();

	if (PENALIZE_TAP_SCORE_NONE && m_pPlayerState != nullptr) {
		SetJudgment(BeatToNoteRow(m_pPlayerState->m_Position.m_fSongBeat),
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
		const float fSongBeat = m_pPlayerState->m_Position.m_fSongBeat;
		const int iSongRow = BeatToNoteRow(fSongBeat);

		// Score all active holds to NotHeld
		for (int iTrack = 0; iTrack < m_NoteData.GetNumTracks(); ++iTrack) {
			// Since this is being called every frame, let's not check the whole
			// array every time. Instead, only check 1 beat back.  Even 1 is
			// overkill.
			const int iStartCheckingAt = max(0, iSongRow - BeatToNoteRow(1));
			NoteData::TrackMap::iterator begin, end;
			m_NoteData.GetTapNoteRangeInclusive(
			  iTrack, iStartCheckingAt, iSongRow + 1, begin, end);
			for (; begin != end; ++begin) {
				TapNote& tn = begin->second;
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
				TapNoteScore tns = tn.result.tns;
				if (tns != TNS_None && tns != TNS_Miss && score == TNS_None) {
					// the sound must also be already playing
					if (m_vKeysounds[tn.iKeysoundIndex].IsPlaying()) {
						// if all of these conditions are met, don't play the
						// sound.
						return;
					}
				}
			}
		}
		m_vKeysounds[tn.iKeysoundIndex].Play(false);
		static Preference<float>* pVolume =
		  Preference<float>::GetPreferenceByName("SoundVolume");
		static float fVol = pVolume->Get();
		m_vKeysounds[tn.iKeysoundIndex].SetProperty("Volume", fVol);
	}
}

void
Player::AddNoteToReplayData(int col,
							const TapNote* pTN,
							int RowOfOverlappingNoteOrRow)
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
							int RowOfOverlappingNoteOrRow)
{
	if (pTN->HoldResult.hns == HNS_Held)
		return;
	HoldReplayResult hrr;
	hrr.row = RowOfOverlappingNoteOrRow;
	hrr.track = col;
	hrr.subType = pTN->subType;
	m_pPlayerStageStats->m_vHoldReplayData.emplace_back(hrr);
}

void
Player::Step(int col,
			 int row,
			 const std::chrono::steady_clock::time_point& tm,
			 bool bHeld,
			 bool bRelease,
			 float padStickSeconds)
{
	// Do everything that depends on a timer here;
	// set your breakpoints somewhere after this block.
	std::chrono::duration<float> stepDelta =
	  std::chrono::steady_clock::now() - tm;
	float stepAgo = stepDelta.count() - padStickSeconds;

	const float fLastBeatUpdate =
	  m_pPlayerState->m_Position.m_LastBeatUpdate.Ago();
	const float fPositionSeconds =
	  m_pPlayerState->m_Position.m_fMusicSeconds - stepAgo;
	const float fTimeSinceStep = stepAgo;

	// idk if this is the correct value for input logs but we'll use them to
	// test -mina ok this is 100% not the place to do this
	// m_pPlayerStageStats->InputData.emplace_back(fPositionSeconds);

	float fSongBeat = m_pPlayerState->m_Position.m_fSongBeat;

	if (GAMESTATE->m_pCurSteps)
		fSongBeat = m_Timing->GetBeatFromElapsedTime(fPositionSeconds);

	const int iSongRow = row == -1 ? BeatToNoteRow(fSongBeat) : row;

	if (col != -1 && !bRelease) {
		// Update roll life
		// Let's not check the whole array every time.
		// Instead, only check 1 beat back.  Even 1 is overkill.
		// Just update the life here and let Update judge the roll.
		const int iStartCheckingAt = max(0, iSongRow - BeatToNoteRow(1));
		NoteData::TrackMap::iterator begin, end;
		m_NoteData.GetTapNoteRangeInclusive(
		  col, iStartCheckingAt, iSongRow + 1, begin, end);
		for (; begin != end; ++begin) {
			TapNote& tn = begin->second;
			if (tn.type != TapNoteType_HoldHead)
				continue;

			switch (tn.subType) {
				DEFAULT_FAIL(tn.subType);
				case TapNoteSubType_Hold:
					continue;
				case TapNoteSubType_Roll:
					break;
			}

			const int iRow = begin->first;

			HoldNoteScore hns = tn.HoldResult.hns;
			if (hns != HNS_None) // if this HoldNote already has a result
				continue; // we don't need to update the logic for this one

			// if they got a bad score or haven't stepped on the corresponding
			// tap yet
			const TapNoteScore tns = tn.result.tns;
			bool bInitiatedNote = true;
			if (REQUIRE_STEP_ON_HOLD_HEADS)
				bInitiatedNote = tns != TNS_None &&
								 tns != TNS_Miss; // did they step on the start?
			const int iEndRow = iRow + tn.iDuration;

			if (bInitiatedNote && tn.HoldResult.fLife != 0) {
				/* This hold note is not judged and we stepped on its head.
				 * Update iLastHeldRow. Do this even if we're a little beyond
				 * the end of the hold note, to make sure iLastHeldRow is
				 * clamped to iEndRow if the hold note is held all the way. */
				// LOG->Trace("setting iLastHeldRow to min of iSongRow (%i) and
				// iEndRow (%i)",iSongRow,iEndRow);
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

						bool bBright =
						  m_pPlayerStageStats &&
						  m_pPlayerStageStats->m_iCurCombo >
							(unsigned int)BRIGHT_GHOST_COMBO_THRESHOLD;
						if (m_pNoteField)
							m_pNoteField->DidHoldNote(col, HNS_Held, bBright);
					}
				}
				break;
			}
		}
	}

	// Check for step on a TapNote
	/* XXX: This seems wrong. If a player steps twice quickly and two notes are
	 * close together in the same column then it is possible for the two notes
	 * to be graded out of order.
	 * Two possible fixes:
	 * 1. Adjust the fSongBeat (or the resulting note row) backward by
	 * iStepSearchRows and search forward two iStepSearchRows lengths,
	 * disallowing graded. This doesn't seem right because if a second note has
	 * passed, an earlier one should not be graded.
	 * 2. Clamp the distance searched backward to the previous row graded.
	 * Either option would fundamentally change the grading of two quick notes
	 * "jack hammers." Hmm.
	 */

	int iStepSearchRows;
	static const float StepSearchDistance = GetMaxStepDistanceSeconds();
	int skipstart = nerv[10]; // this is not robust need to come up with
							  // something better later - Mina

	if (iSongRow < skipstart || iSongRow > static_cast<int>(nerv.size()) - 10) {
		iStepSearchRows =
		  max(BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(
				m_pPlayerState->m_Position.m_fMusicSeconds +
				StepSearchDistance)) -
				iSongRow,
			  iSongRow - BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(
						   m_pPlayerState->m_Position.m_fMusicSeconds -
						   StepSearchDistance))) +
		  ROWS_PER_BEAT;
	} else {
		/* Buncha bullshit that speeds up searching for the rows that we're
		concerned about judging taps within by avoiding the invocation of the
		incredibly slow getbeatfromelapsedtime. Needs to be cleaned up a lot,
		whole system does. Only in use if sequential assumption remains valid. -
		Mina */

		if (nerv[nervpos] < iSongRow && nervpos < nerv.size())
			nervpos += 1;

		size_t SearchIndexBehind = nervpos;
		size_t SearchIndexAhead = nervpos;
		float SearchBeginTime = m_Timing->WhereUAtBro(nerv[nervpos]);

		while (SearchIndexBehind > 1 &&
			   SearchBeginTime -
				   m_Timing->WhereUAtBro(nerv[SearchIndexBehind - 1]) <
				 StepSearchDistance)
			SearchIndexBehind -= 1;

		while (SearchIndexAhead > 1 && SearchIndexAhead + 1 > nerv.size() &&
			   m_Timing->WhereUAtBro(nerv[SearchIndexAhead + 1]) -
				   SearchBeginTime <
				 StepSearchDistance)
			SearchIndexAhead += 1;

		int MaxLookBehind = nerv[nervpos] - nerv[SearchIndexBehind];
		int MaxLookAhead = nerv[SearchIndexAhead] - nerv[nervpos];

		if (nervpos > 0)
			iStepSearchRows =
			  (max(MaxLookBehind, MaxLookAhead) + ROWS_PER_BEAT);
	}

	// calculate TapNoteScore
	TapNoteScore score = TNS_None;

	int iRowOfOverlappingNoteOrRow = row;
	if (row == -1 && col != -1)
		iRowOfOverlappingNoteOrRow = GetClosestNote(
		  col, iSongRow, iStepSearchRows, iStepSearchRows, false, false);

	if (iRowOfOverlappingNoteOrRow != -1 && col != -1) {
		// compute the score for this hit
		float fNoteOffset = 0.f;
		// only valid if
		float fMusicSeconds = 0.f;
		// we need this later if we are autosyncing
		const float fStepBeat = NoteRowToBeat(iRowOfOverlappingNoteOrRow);
		const float fStepSeconds = m_Timing->WhereUAtBro(fStepBeat);

		if (row == -1) {
			// We actually stepped on the note this long ago:
			// fTimeSinceStep

			/* GAMESTATE->m_fMusicSeconds is the music time as of
			 * GAMESTATE->m_LastBeatUpdate. Figure out what the music time is as
			 * of now. */
			const float fCurrentMusicSeconds =
			  m_pPlayerState->m_Position.m_fMusicSeconds +
			  (fLastBeatUpdate *
			   GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate);

			// ... which means it happened at this point in the music:
			fMusicSeconds =
			  fCurrentMusicSeconds -
			  fTimeSinceStep *
				GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

			// The offset from the actual step in seconds:
			fNoteOffset = (fStepSeconds - fMusicSeconds) /
						  GAMESTATE->m_SongOptions.GetCurrent()
							.m_fMusicRate; // account for music rate
										   /*
										   LOG->Trace("step was %.3f ago, music is off by %f: %f vs %f, step
										   was %f off",							    fTimeSinceStep,
										   GAMESTATE->m_LastBeatUpdate.Ago()/GAMESTATE->m_SongOptions.m_fMusicRate,
											   fStepSeconds, fMusicSeconds, fNoteOffset );
										   */
		}

		NOTESKIN->SetLastSeenColor(
		  NoteTypeToString(GetNoteType(iRowOfOverlappingNoteOrRow)));

		const float fSecondsFromExact = fabsf(fNoteOffset);

		TapNote* pTN = NULL;
		NoteData::iterator iter =
		  m_NoteData.FindTapNote(col, iRowOfOverlappingNoteOrRow);
		DEBUG_ASSERT(iter != m_NoteData.end(col));
		pTN = &iter->second;

		// We don't really have to care if we are releasing on a non-lift,
		// right? This fixes a weird noteskin bug with tap explosions.
		if (PREFSMAN->m_bFullTapExplosions && bRelease &&
			pTN->type != TapNoteType_Lift)
			return;

		// Fakes.
		if (pTN->type == TapNoteType_Fake)
			return;

		switch (m_pPlayerState->m_PlayerController) {
			case PC_HUMAN:
				switch (pTN->type) {
					case TapNoteType_Mine:
						// Stepped too close to mine?
						if (!bRelease &&
							fSecondsFromExact <= GetWindowSeconds(TW_Mine) &&
							m_Timing->IsJudgableAtRow(iSongRow))
							score = TNS_HitMine;
						break;
					case TapNoteType_HoldHead:
						// oh wow, this was causing the trigger before the hold
						// heads bug. (It was fNoteOffset > 0.f before)
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
							if (fSecondsFromExact > GetWindowSeconds(TW_W4))
								break;
						}
						if ((pTN->type == TapNoteType_Lift) == bRelease) {
							if (fSecondsFromExact <= GetWindowSeconds(TW_W1))
								score = TNS_W1;
							else if (fSecondsFromExact <=
									 GetWindowSeconds(TW_W2))
								score = TNS_W2;
							else if (fSecondsFromExact <=
									 GetWindowSeconds(TW_W3))
								score = TNS_W3;
							else if (fSecondsFromExact <=
									 GetWindowSeconds(TW_W4))
								score = TNS_W4;
							else if (fSecondsFromExact <=
									 max(GetWindowSeconds(TW_W5), 0.18f))
								score = TNS_W5;
						}
						break;
				}
				break;

			case PC_CPU:
			case PC_AUTOPLAY:
				score = PlayerAI::GetTapNoteScore(m_pPlayerState);

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

				// TRICKY:  We're asking the AI to judge mines. Consider TNS_W4
				// and below as "mine was hit" and everything else as "mine was
				// avoided"
				if (pTN->type == TapNoteType_Mine) {
					// The CPU hits a lot of mines. Only consider hitting the
					// first mine for a row. We know we're the first mine if
					// there are are no mines to the left of us.
					for (int t = 0; t < col; t++) {
						if (m_NoteData.GetTapNote(t, iRowOfOverlappingNoteOrRow)
							  .type == TapNoteType_Mine) // there's a mine to
														 // the left of us
							return;						 // avoid
					}

					// The CPU hits a lot of mines. Make it less likely to hit
					// mines that don't have a tap note on the same row.
					bool bTapsOnRow = m_NoteData.IsThereATapOrHoldHeadAtRow(
					  iRowOfOverlappingNoteOrRow);
					TapNoteScore get_to_avoid = bTapsOnRow ? TNS_W3 : TNS_W4;

					if (score >= get_to_avoid)
						return; // avoided

					score = TNS_HitMine;
				}

				if (score > TNS_W4)
					score = TNS_W2; // sentinel

				/* AI will generate misses here. Don't handle a miss like a
				 * regular note because we want the judgment animation to appear
				 * delayed. Instead, return early if AI generated a miss, and
				 * let UpdateTapNotesMissedOlderThan() detect and handle the
				 * misses. */
				if (score == TNS_Miss)
					return;

				// Put some small, random amount in fNoteOffset so that
				// demonstration show a mix of late and early. - Chris
				// (StepMania r15628)
				// fNoteOffset = randomf( -0.1f, 0.1f );
				// Since themes may use the offset in a visual graph, the above
				// behavior is not the best thing to do. Instead, random numbers
				// should be generated based on the TapNoteScore, so that they
				// can logically match up with the current timing windows. -aj
				{
					float fWindowW1 = GetWindowSeconds(TW_W1);
					float fWindowW2 = GetWindowSeconds(TW_W2);
					float fWindowW3 = GetWindowSeconds(TW_W3);
					float fWindowW4 = GetWindowSeconds(TW_W4);
					float fWindowW5 = GetWindowSeconds(TW_W5);

					// figure out overlap.
					float fLowerBound = 0.0f;	// negative upper limit
					float fUpperBound = 0.0f;	// positive lower limit
					float fCompareWindow = 0.0f; // filled in here:
					if (score == TNS_W4) {
						fLowerBound = -fWindowW3;
						fUpperBound = fWindowW3;
						fCompareWindow = fWindowW4;
					} else if (score == TNS_W5) {
						fLowerBound = -fWindowW4;
						fUpperBound = fWindowW4;
						fCompareWindow = fWindowW5;
					}
					float f1 = randomf(-fCompareWindow, fLowerBound);
					float f2 = randomf(fUpperBound, fCompareWindow);

					if (randomf() * 100 >= 50)
						fNoteOffset = f1;
					else
						fNoteOffset = f2;
				}

				break;
			default:
				FAIL_M(ssprintf("Invalid player controller type: %i",
								m_pPlayerState->m_PlayerController));
		}

		if (m_pPlayerState->m_PlayerController == PC_HUMAN && score >= TNS_W3)
			AdjustSync::HandleAutosync(fNoteOffset, fStepSeconds);

		// Do game-specific and mode-specific score mapping.
		score = GAMESTATE->GetCurrentGame()->MapTapNoteScore(score);
		if (score == TNS_W1 && !GAMESTATE->ShowW1())
			score = TNS_W2;

		if (score != TNS_None) {
			pTN->result.tns = score;
			pTN->result.fTapNoteOffset = -fNoteOffset;
		}

		m_LastTapNoteScore = score;
		if (pTN->result.tns != TNS_None)
			AddNoteToReplayData(GAMESTATE->CountNotesSeparately() ? col : -1,
								pTN,
								iRowOfOverlappingNoteOrRow);
		if (GAMESTATE->CountNotesSeparately()) {
			if (pTN->type != TapNoteType_Mine) {
				const bool bBlind =
				  (m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind != 0);
				const bool bBright =
				  (m_pPlayerStageStats &&
				   m_pPlayerStageStats->m_iCurCombo >
					 (unsigned int)BRIGHT_GHOST_COMBO_THRESHOLD);
				if (m_pNoteField != nullptr)
					m_pNoteField->DidTapNote(
					  col, bBlind ? TNS_W1 : score, bBright);
				if (score >= m_pPlayerState->m_PlayerOptions.GetCurrent()
							   .m_MinTNSToHideNotes)
					HideNote(col, iRowOfOverlappingNoteOrRow);

				if (pTN->result.tns != TNS_None) {
					SetJudgment(iRowOfOverlappingNoteOrRow, col, *pTN);
					HandleTapRowScore(iRowOfOverlappingNoteOrRow);
				}
			}
		} else if (NoteDataWithScoring::IsRowCompletelyJudged(
					 m_NoteData, iRowOfOverlappingNoteOrRow)) {
			FlashGhostRow(iRowOfOverlappingNoteOrRow);
		}
	}

	if (score == TNS_None)
		DoTapScoreNone();

	if (!bRelease && col != -1) {
		/* Search for keyed sounds separately.  Play the nearest note. */
		/* XXX: This isn't quite right. As per the above XXX for
		 * iRowOfOverlappingNote, if iRowOfOverlappingNote is set to a previous
		 * note, the keysound could have changed and this would cause the wrong
		 * one to play, in essence playing two sounds in the opposite order.
		 * Maybe this should always perform the search. Still, even that doesn't
		 * seem quite right since it would then play the same (new) keysound
		 * twice which would sound wrong even though the notes were judged as
		 * being correct, above. Fixing the above problem would fix this one as
		 * well. */
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
			const TapNote& tn =
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
	TapNoteScore lastTNS =
	  NoteDataWithScoring::LastTapNoteWithResult(m_NoteData, iRow).result.tns;
	const bool bBlind =
	  (m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind != 0);
	const bool bBright =
	  (m_pPlayerStageStats && m_pPlayerStageStats->m_iCurCombo >
								(unsigned int)BRIGHT_GHOST_COMBO_THRESHOLD);

	for (int iTrack = 0; iTrack < m_NoteData.GetNumTracks(); ++iTrack) {
		const TapNote& tn = m_NoteData.GetTapNote(iTrack, iRow);

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

	NoteData::all_tracks_iterator& iter = *m_pIterUncrossedRows;
	int iLastSeenRow = -1;
	for (; !iter.IsAtEnd() && iter.Row() <= iLastRowCrossed; ++iter) {
		// Apply InitialHoldLife.
		TapNote& tn = *iter;
		int iRow = iter.Row();
		int iTrack = iter.Track();
		switch (tn.type) {
			case TapNoteType_HoldHead: {
				tn.HoldResult.fLife = INITIAL_HOLD_LIFE;
				if (!REQUIRE_STEP_ON_HOLD_HEADS) {
					PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
					vector<GameInput> GameI;
					GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
					  ->StyleInputToGameInput(iTrack, pn, GameI);
					if (PREFSMAN->m_fPadStickSeconds > 0.f) {
						for (size_t i = 0; i < GameI.size(); ++i) {
							float fSecsHeld = INPUTMAPPER->GetSecsHeld(
							  GameI[i], m_pPlayerState->m_mp);
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
				// Hold the panel while crossing a mine will cause the mine to
				// explode
				// TODO: Remove use of PlayerNumber.
				PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
				vector<GameInput> GameI;
				GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
				  ->StyleInputToGameInput(iTrack, pn, GameI);
				if (PREFSMAN->m_fPadStickSeconds > 0.0f) {
					for (size_t i = 0; i < GameI.size(); ++i) {
						float fSecsHeld = INPUTMAPPER->GetSecsHeld(
						  GameI[i], m_pPlayerState->m_mp);
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
						if (m_pPlayerStageStats)
							m_pPlayerStageStats->m_bDisqualified = true;
					}
				}
			}
		}

		// TODO: Can we remove the iLastSeenRow logic and the
		// autokeysound for loop, since the iterator in this loop will
		// already be iterating over all of the tracks?
		if (iRow != iLastSeenRow) {
			// crossed a new not-empty row
			iLastSeenRow = iRow;

			for (int t = 0; t < m_NoteData.GetNumTracks(); ++t) {
				const TapNote& tap = m_NoteData.GetTapNote(t, iRow);
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
		// Few rows typically cross per update. Easier to check all crossed rows
		// than to calculate from timing segments.
		for (int r = m_iFirstUncrossedRow; r <= iLastRowCrossed; ++r) {
			int tickCurrent = m_Timing->GetTickcountAtRow(r);

			// There is a tick count at this row
			if (tickCurrent > 0 && r % (ROWS_PER_BEAT / tickCurrent) == 0) {

				vector<int> viColsWithHold;
				int iNumHoldsHeldThisRow = 0;
				int iNumHoldsMissedThisRow = 0;

				// start at r-1 so that we consider holds whose end rows are
				// equal to the checkpoint row
				NoteData::all_tracks_iterator nIter =
				  m_NoteData.GetTapNoteRangeAllTracks(r - 1, r, true);
				for (; !nIter.IsAtEnd(); ++nIter) {
					TapNote& tn = *nIter;
					if (tn.type != TapNoteType_HoldHead)
						continue;

					int iTrack = nIter.Track();
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

				// TODO: Find a better way of handling hold checkpoints with
				// other taps.
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
	const float fEarliestTime =
	  m_pPlayerState->m_Position.m_fMusicSeconds - fMissIfOlderThanSeconds;
	{
		TimingData::GetBeatArgs beat_info;
		beat_info.elapsed_time = fEarliestTime;
		m_Timing->GetBeatAndBPSFromElapsedTime(beat_info);

		iMissIfOlderThanThisRow = BeatToNoteRow(beat_info.beat);
		if (beat_info.freeze_out || beat_info.delay_out) {
			/* If there is a freeze on iMissIfOlderThanThisIndex, include this
			 * index too. Otherwise we won't show misses for tap notes on
			 * freezes until the freeze finishes. */
			if (!beat_info.delay_out)
				iMissIfOlderThanThisRow++;
		}
	}

	NoteData::all_tracks_iterator& iter = *m_pIterNeedsTapJudging;

	for (; !iter.IsAtEnd() && iter.Row() < iMissIfOlderThanThisRow; ++iter) {
		TapNote& tn = *iter;

		if (!NeedsTapJudging(tn))
			continue;

		// Ignore all notes in WarpSegments or FakeSegments.
		if (!m_Timing->IsJudgableAtRow(iter.Row()))
			continue;

		if (tn.type == TapNoteType_Mine) {
			tn.result.tns = TNS_AvoidMine;
			/* The only real way to tell if a mine has been scored is if it has
			 * disappeared but this only works for hit mines so update the
			 * scores for avoided mines here. */
			if (m_pPrimaryScoreKeeper)
				m_pPrimaryScoreKeeper->HandleTapScore(tn);
		} else {
			tn.result.tns = TNS_Miss;

			// avoid scoring notes that get passed when seeking in pm
			// not sure how many rows grace time is needed (if any?)
			if (GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
				  .m_bPractice &&
				iMissIfOlderThanThisRow - iter.Row() > 8)
				tn.result.tns = TNS_None;
			if (GAMESTATE->CountNotesSeparately()) {
				SetJudgment(iter.Row(), iter.Track(), tn);
				HandleTapRowScore(iter.Row());
			}
		}
	}
}

void
Player::UpdateJudgedRows(float fDeltaTime)
{
	// Look into the future only as far as we need to
	const int iEndRow = BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(
	  m_pPlayerState->m_Position.m_fMusicSeconds +
	  GetMaxStepDistanceSeconds()));
	bool bAllJudged = true;

	if (!GAMESTATE->CountNotesSeparately()) {
		NoteData::all_tracks_iterator iter = *m_pIterUnjudgedRows;
		int iLastSeenRow = -1;
		for (; !iter.IsAtEnd() && iter.Row() <= iEndRow; ++iter) {
			int iRow = iter.Row();

			// Do not judge arrows in WarpSegments or FakeSegments
			if (!m_Timing->IsJudgableAtRow(iRow))
				continue;

			if (iLastSeenRow != iRow) {
				iLastSeenRow = iRow;

				// crossed a nonempty row
				if (!NoteDataWithScoring::IsRowCompletelyJudged(m_NoteData,
																iRow)) {
					bAllJudged = false;
					continue;
				}
				if (bAllJudged)
					*m_pIterUnjudgedRows = iter;
				if (m_pJudgedRows->JudgeRow(iRow))
					continue;

				const TapNote& lastTN =
				  NoteDataWithScoring::LastTapNoteWithResult(m_NoteData, iRow);

				if (lastTN.result.tns < TNS_Miss)
					continue;

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
		set<RageSound*> setSounds;
		NoteData::all_tracks_iterator iter = *m_pIterUnjudgedMineRows; // copy
		int iLastSeenRow = -1;
		for (; !iter.IsAtEnd() && iter.Row() <= iEndRow; ++iter) {
			int iRow = iter.Row();

			// Do not worry about mines in WarpSegments or FakeSegments
			if (!m_Timing->IsJudgableAtRow(iRow))
				continue;

			TapNote& tn = *iter;

			if (iRow != iLastSeenRow) {
				iLastSeenRow = iRow;
				if (bAllJudged)
					*m_pIterUnjudgedMineRows = iter;
			}

			bool bMineNotHidden =
			  tn.type == TapNoteType_Mine && !tn.result.bHidden;
			if (!bMineNotHidden)
				continue;

			switch (tn.result.tns) {
				DEFAULT_FAIL(tn.result.tns);
				case TNS_None:
					bAllJudged = false;
					continue;
				case TNS_AvoidMine:
					SetMineJudgment(tn.result.tns, iter.Track());
					tn.result.bHidden = true;
					continue;
				case TNS_HitMine:
					SetMineJudgment(tn.result.tns, iter.Track());
					break;
			}
			if (m_pNoteField)
				m_pNoteField->DidTapNote(iter.Track(), tn.result.tns, false);

			if (tn.iKeysoundIndex >= 0 &&
				tn.iKeysoundIndex < static_cast<int>(m_vKeysounds.size()))
				setSounds.insert(&m_vKeysounds[tn.iKeysoundIndex]);
			else if (g_bEnableMineSoundPlayback)
				setSounds.insert(&m_soundMine);

			ChangeLife(tn.result.tns);
			// Make sure hit mines affect the dance points.
			if (m_pPrimaryScoreKeeper)
				m_pPrimaryScoreKeeper->HandleTapScore(tn);
			tn.result.bHidden = true;
		}
		// If we hit the end of the loop, m_pIterUnjudgedMineRows needs to be
		// updated. -Kyz
		if ((iter.IsAtEnd() || iLastSeenRow == iEndRow) && bAllJudged) {
			*m_pIterUnjudgedMineRows = iter;
		}

		FOREACHS(RageSound*, setSounds, s)
		{
			// Only play one copy of each mine sound at a time per player.
			(*s)->Stop();
			(*s)->Play(false);
		}
	}
}

void
Player::HandleTapRowScore(unsigned row)
{
	bool bNoCheating = true;
#ifdef DEBUG
	bNoCheating = false;
#endif

	// don't accumulate points if AutoPlay is on.
	if (bNoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY)
		return;

	TapNoteScore scoreOfLastTap =
	  NoteDataWithScoring::LastTapNoteWithResult(m_NoteData, row).result.tns;
	const unsigned int iOldCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const unsigned int iOldMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	if (scoreOfLastTap == TNS_Miss)
		m_LastTapNoteScore = TNS_Miss;

	for (int track = 0; track < m_NoteData.GetNumTracks(); ++track) {
		const TapNote& tn = m_NoteData.GetTapNote(track, row);
		// Mines cannot be handled here.
		if (tn.type == TapNoteType_Empty || tn.type == TapNoteType_Fake ||
			tn.type == TapNoteType_Mine || tn.type == TapNoteType_AutoKeysound)
			continue;
		if (m_pPrimaryScoreKeeper != nullptr)
			m_pPrimaryScoreKeeper->HandleTapScore(tn);
	}

	if (m_pPrimaryScoreKeeper != NULL)
		m_pPrimaryScoreKeeper->HandleTapRowScore(m_NoteData, row);

	const unsigned int iCurCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const unsigned int iCurMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	SendComboMessages(iOldCombo, iOldMissCombo);

	if (m_pPlayerStageStats != nullptr) {
		SetCombo(iCurCombo, iCurMissCombo);
	}

#define CROSSED(x) (iOldCombo < (x) && iCurCombo >= (x))
	if (CROSSED(100))
		SCREENMAN->PostMessageToTopScreen(SM_100Combo, 0);
	else if (CROSSED(200))
		SCREENMAN->PostMessageToTopScreen(SM_200Combo, 0);
	else if (CROSSED(300))
		SCREENMAN->PostMessageToTopScreen(SM_300Combo, 0);
	else if (CROSSED(400))
		SCREENMAN->PostMessageToTopScreen(SM_400Combo, 0);
	else if (CROSSED(500))
		SCREENMAN->PostMessageToTopScreen(SM_500Combo, 0);
	else if (CROSSED(600))
		SCREENMAN->PostMessageToTopScreen(SM_600Combo, 0);
	else if (CROSSED(700))
		SCREENMAN->PostMessageToTopScreen(SM_700Combo, 0);
	else if (CROSSED(800))
		SCREENMAN->PostMessageToTopScreen(SM_800Combo, 0);
	else if (CROSSED(900))
		SCREENMAN->PostMessageToTopScreen(SM_900Combo, 0);
	else if (CROSSED(1000))
		SCREENMAN->PostMessageToTopScreen(SM_1000Combo, 0);
	else if ((iOldCombo / 100) < (iCurCombo / 100) && iCurCombo > 1000)
		SCREENMAN->PostMessageToTopScreen(SM_ComboContinuing, 0);
#undef CROSSED

	// new max combo
	if (m_pPlayerStageStats)
		m_pPlayerStageStats->m_iMaxCombo =
		  max(m_pPlayerStageStats->m_iMaxCombo, iCurCombo);

	/* Use the real current beat, not the beat we've been passed. That's because
	 * we want to record the current life/combo to the current time; eg. if it's
	 * a MISS, the beat we're registering is in the past, but the life is
	 * changing now. We need to include time from previous songs in a course, so
	 * we can't use GAMESTATE->m_fMusicSeconds. Use fStepsSeconds instead. */
	if (m_pPlayerStageStats)
		m_pPlayerStageStats->UpdateComboList(
		  STATSMAN->m_CurStageStats.m_fStepsSeconds, false);

	ChangeLife(scoreOfLastTap);
}

void
Player::HandleHoldCheckpoint(int iRow,
							 int iNumHoldsHeldThisRow,
							 int iNumHoldsMissedThisRow,
							 const vector<int>& viColsWithHold)
{
	bool bNoCheating = true;
#ifdef DEBUG
	bNoCheating = false;
#endif

	// WarpSegments and FakeSegments aren't judged in any way.
	if (!m_Timing->IsJudgableAtRow(iRow))
		return;

	// don't accumulate combo if AutoPlay is on.
	if (bNoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY)
		return;

	const unsigned int iOldCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const unsigned int iOldMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	if (m_pPrimaryScoreKeeper != nullptr)
		m_pPrimaryScoreKeeper->HandleHoldCheckpointScore(
		  m_NoteData, iRow, iNumHoldsHeldThisRow, iNumHoldsMissedThisRow);

	if (iNumHoldsMissedThisRow == 0) {
		// added for http://ssc.ajworld.net/sm-ssc/bugtracker/view.php?id=16 -aj
		if (CHECKPOINTS_FLASH_ON_HOLD) {
			FOREACH_CONST(int, viColsWithHold, i)
			{
				bool bBright = m_pPlayerStageStats &&
							   m_pPlayerStageStats->m_iCurCombo >
								 (unsigned int)BRIGHT_GHOST_COMBO_THRESHOLD;
				if (m_pNoteField)
					m_pNoteField->DidHoldNote(*i, HNS_Held, bBright);
			}
		}
	}

	SendComboMessages(iOldCombo, iOldMissCombo);

	if (m_pPlayerStageStats != nullptr) {
		SetCombo(m_pPlayerStageStats->m_iCurCombo,
				 m_pPlayerStageStats->m_iCurMissCombo);
		m_pPlayerStageStats->UpdateComboList(
		  STATSMAN->m_CurStageStats.m_fStepsSeconds, false);
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
Player::HandleHoldScore(const TapNote& tn)
{
	HoldNoteScore holdScore = tn.HoldResult.hns;
	TapNoteScore tapScore = tn.result.tns;
	bool bNoCheating = true;
#ifdef DEBUG
	bNoCheating = false;
#endif

	// don't accumulate points if AutoPlay is on.
	if (bNoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY)
		return;

	if (m_pPrimaryScoreKeeper != nullptr)
		m_pPrimaryScoreKeeper->HandleHoldScore(tn);
	ChangeLife(holdScore, tapScore);
}

float
Player::GetMaxStepDistanceSeconds()
{
	float fMax = 0;
	fMax = max(fMax, GetWindowSeconds(TW_W5));
	fMax = max(fMax, GetWindowSeconds(TW_W4));
	fMax = max(fMax, GetWindowSeconds(TW_W3));
	fMax = max(fMax, GetWindowSeconds(TW_W2));
	fMax = max(fMax, GetWindowSeconds(TW_W1));
	float f = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate * fMax;
	return f + m_fMaxInputLatencySeconds;
}

void
Player::FadeToFail()
{
	if (m_pNoteField != nullptr)
		m_pNoteField->FadeToFail();

	// clear miss combo
	SetCombo(0, 0);
}

void
Player::CacheAllUsedNoteSkins()
{
	if (m_pNoteField != nullptr)
		m_pNoteField->CacheAllUsedNoteSkins();
}

/* Reworked the judgment messages. Header file states that -1 should be sent as
the offset for misses. This was not the case and 0s were being sent. Now it just
sends nothing so params.Judgment
~= nil can be used to filter messages with and without offsets. Also now there's
a params.Judgment that just gives the judgment for taps holds and mines in
aggregate for things that need to be done with any judgment. Params.Type is used
to diffrentiate between those attributes for things that are done differently
between the types. Current values for taps/holds are sent in params.Val. Like it
all should have been to begin with. Not sure where checkpoints are but I also
don't care, so.

Update: both message types are being sent out currently for compatability.
-Mina*/
//#define autoplayISHUMAN
void
Player::SetMineJudgment(TapNoteScore tns, int iTrack)
{
	if (m_bSendJudgmentAndComboMessages) {
		Message msg("Judgment");
		msg.SetParam("Player", m_pPlayerState->m_PlayerNumber);
		msg.SetParam("TapNoteScore", tns);
		msg.SetParam("FirstTrack", iTrack);
		msg.SetParam("Judgment", tns);
		msg.SetParam("Type", std::string("Mine"));

		// Ms scoring implemenation - Mina
		if (tns == TNS_HitMine)
			curwifescore += wife3_mine_hit_weight;

		if (m_pPlayerStageStats != nullptr) {
			if (maxwifescore == 0.f)
				msg.SetParam("WifePercent", 0);
			else
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
				curwifescore -= 6666666.f; // sail hatan
			}
#endif
		}

		MESSAGEMAN->Broadcast(msg);
		if (m_pPlayerStageStats &&
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
	if (tns == TNS_Miss && m_pPlayerStageStats != nullptr)
		AddNoteToReplayData(
		  GAMESTATE->CountNotesSeparately() ? iTrack : -1, &tn, iRow);

	if (m_bSendJudgmentAndComboMessages) {
		Message msg("Judgment");
		msg.SetParam("Player", m_pPlayerState->m_PlayerNumber);
		msg.SetParam("MultiPlayer", m_pPlayerState->m_mp);
		msg.SetParam("FirstTrack", iTrack);
		msg.SetParam("TapNoteScore", tns);
		msg.SetParam("Early", fTapNoteOffset < 0.0f);
		msg.SetParam("Judgment", tns);
		msg.SetParam("NoteRow", iRow);
		msg.SetParam("Type", std::string("Tap"));
		msg.SetParam("TapNoteOffset", tn.result.fTapNoteOffset);
		if (m_pPlayerStageStats)
			msg.SetParam("Val", m_pPlayerStageStats->m_iTapNoteScores[tns] + 1);

		if (tns != TNS_Miss)
			msg.SetParam("Offset",
						 tn.result.fTapNoteOffset * 1000); // don't send out ms
														   // offsets for
														   // misses, multiply
														   // by 1000 for
														   // convenience - Mina

		if (m_pPlayerStageStats != nullptr) {
			if (tns == TNS_Miss)
				curwifescore += wife3_miss_weight;
			else
				curwifescore +=
				  wife3(tn.result.fTapNoteOffset, m_fTimingWindowScale);
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
				curwifescore -= 666.f; // hail satan
			}

#endif
		}

		Lua* L = LUA->Get();
		lua_createtable(L, 0, m_NoteData.GetNumTracks()); // TapNotes this row
		lua_createtable(
		  L,
		  0,
		  m_NoteData.GetNumTracks()); // HoldHeads of tracks held at this row.
		if (GAMESTATE->CountNotesSeparately()) {
			for (int jTrack = 0; jTrack < m_NoteData.GetNumTracks(); ++jTrack) {
				NoteData::iterator tn = m_NoteData.FindTapNote(jTrack, iRow);
				if (tn != m_NoteData.end(jTrack) && jTrack == iTrack) {
					tn->second.PushSelf(L);
					lua_rawseti(L, -3, jTrack + 1);
				} else {
					int iHeadRow;
					if (m_NoteData.IsHoldNoteAtRow(jTrack, iRow, &iHeadRow)) {
						NoteData::iterator hold =
						  m_NoteData.FindTapNote(jTrack, iHeadRow);
						hold->second.PushSelf(L);
						lua_rawseti(L, -2, jTrack + 1);
					}
				}
			}
		} else {
			for (int jTrack = 0; jTrack < m_NoteData.GetNumTracks(); ++jTrack) {
				NoteData::iterator tn = m_NoteData.FindTapNote(jTrack, iRow);
				if (tn != m_NoteData.end(jTrack)) {
					tn->second.PushSelf(L);
					lua_rawseti(L, -3, jTrack + 1);
				} else {
					int iHeadRow;
					if (m_NoteData.IsHoldNoteAtRow(jTrack, iRow, &iHeadRow)) {
						NoteData::iterator hold =
						  m_NoteData.FindTapNote(jTrack, iHeadRow);
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
	if (m_vpHoldJudgment[iTrack])
		m_vpHoldJudgment[iTrack]->SetHoldJudgment(tn.HoldResult.hns);

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
				tn.HoldResult.hns == HNS_Missed)
				curwifescore += wife3_hold_drop_weight;

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

		Lua* L = LUA->Get();
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

	bool b25Milestone = false;
	bool b50Milestone = false;
	bool b100Milestone = false;
	bool b250Milestone = false;
	bool b1000Milestone = false;

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

	if (b25Milestone)
		this->PlayCommand("TwentyFiveMilestone");
	if (b50Milestone)
		this->PlayCommand("FiftyMilestone");
	if (b100Milestone)
		this->PlayCommand("HundredMilestone");
	if (b250Milestone)
		this->PlayCommand("TwoHundredFiftyMilestone");
	if (b1000Milestone)
		this->PlayCommand("ThousandMilestone");

	/* Colored combo logic differs between Songs and Courses.
	 *	Songs:
	 *	The theme decides how far into the song the combo color should appear.
	 *	(PERCENT_UNTIL_COLOR_COMBO)
	 *
	 *	Courses:
	 *	PERCENT_UNTIL_COLOR_COMBO refers to how long through the course the
	 *	combo color should appear (scaling to the number of songs). This may
	 *	not be desired behavior, however. -aj
	 *
	 *	TODO: Add a metric that determines Course combo colors logic?
	 *	Or possibly move the logic to a Lua function? -aj */
	bool bPastBeginning = false;

	bPastBeginning =
	  m_pPlayerState->m_Position.m_fMusicSeconds >
	  GAMESTATE->m_pCurSong->m_fMusicLengthSeconds * PERCENT_UNTIL_COLOR_COMBO;

	if (m_bSendJudgmentAndComboMessages) {
		Message msg("Combo");
		if (iCombo)
			msg.SetParam("Combo", iCombo);
		if (iMisses)
			msg.SetParam("Misses", iMisses);
		if (bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W1))
			msg.SetParam("FullComboW1", true);
		if (bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W2))
			msg.SetParam("FullComboW2", true);
		if (bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W3))
			msg.SetParam("FullComboW3", true);
		if (bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W4))
			msg.SetParam("FullComboW4", true);
		this->HandleMessage(msg);
	}
}

void
Player::IncrementComboOrMissCombo(bool bComboOrMissCombo)
{
	const unsigned int iOldCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const unsigned int iOldMissCombo =
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
	int firstRow = 0;
	int lastRow = m_NoteData.GetLastRow() + 1;

	// Go over every single non empty row and their tracks
	FOREACH_NONEMPTY_ROW_ALL_TRACKS(m_NoteData, row)
	{
		for (int track = 0; track < m_NoteData.GetNumTracks(); track++) {
			// Find the tapnote we are on
			TapNote* pTN = NULL;
			NoteData::iterator iter = m_NoteData.FindTapNote(track, row);
			DEBUG_ASSERT(iter != m_NoteData.end(track));
			pTN = &iter->second;

			// Reset the score so it can be visible
			if (iter != m_NoteData.end(track)) {
				if (pTN->type == TapNoteType_Empty)
					continue;
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

	for (int i = 0;
		 i < GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
			   ->m_iColsPerPlayer;
		 ++i) {
		lastHoldHeadsSeconds[i] = -1000.f;
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
	static int SetLife(T* p, lua_State* L)
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
	static int ChangeLife(T* p, lua_State* L)
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
	static int SetActorWithJudgmentPosition(T* p, lua_State* L)
	{
		Actor* pActor = Luna<Actor>::check(L, 1);
		p->SetActorWithJudgmentPosition(pActor);
		COMMON_RETURN_SELF;
	}
	static int SetActorWithComboPosition(T* p, lua_State* L)
	{
		Actor* pActor = Luna<Actor>::check(L, 1);
		p->SetActorWithComboPosition(pActor);
		COMMON_RETURN_SELF;
	}
	static int GetPlayerTimingData(T* p, lua_State* L)
	{
		p->GetPlayerTimingData().PushSelf(L);
		return 1;
	}

	LunaPlayer()
	{
		ADD_METHOD(SetLife);
		ADD_METHOD(ChangeLife);
		ADD_METHOD(SetActorWithJudgmentPosition);
		ADD_METHOD(SetActorWithComboPosition);
		ADD_METHOD(GetPlayerTimingData);
	}
};

LUA_REGISTER_DERIVED_CLASS(Player, ActorFrame)
// lua end
