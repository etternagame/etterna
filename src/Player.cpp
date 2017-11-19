#include "global.h"
#include "Player.h"
#include "GameConstantsAndTypes.h"
#include "RageUtil.h"
#include "RageTimer.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "InputMapper.h"
#include "SongManager.h"
#include "GameState.h"
#include "ScoreKeeperNormal.h"
#include "RageLog.h"
#include "RageDisplay.h"
#include "ThemeManager.h"
#include "ScoreDisplay.h"
#include "LifeMeter.h"
#include "PlayerAI.h"
#include "NoteField.h"
#include "NoteDataUtil.h"
#include "ScreenMessage.h"
#include "ScreenManager.h"
#include "StageStats.h"
#include "ActorUtil.h"
#include "ArrowEffects.h"
#include "Game.h"
#include "NetworkSyncManager.h"	//used for sending timing offset 
#include "DancingCharacters.h"
#include "ScreenDimensions.h"
#include "RageSoundManager.h"
#include "ThemeMetric.h"
#include "PlayerState.h"
#include "GameSoundManager.h"
#include "Style.h"
#include "MessageManager.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "StatsManager.h"
#include "Song.h"
#include "Steps.h"
#include "GameCommand.h"
#include "LocalizedString.h"
#include "AdjustSync.h"
#include "ScoreManager.h"

RString ATTACK_DISPLAY_X_NAME( size_t p, size_t both_sides );
void TimingWindowSecondsInit( size_t /*TimingWindow*/ i, RString &sNameOut, float &defaultValueOut );

/**
 * @brief Helper class to ensure that each row is only judged once without taking too much memory.
 */
class JudgedRows
{
	vector<bool> m_vRows;
	int m_iStart{0};
	int m_iOffset{0};

	void Resize( size_t iMin )
	{
		size_t iNewSize = max( 2*m_vRows.size(), iMin );
		vector<bool> vNewRows( m_vRows.begin() + m_iOffset, m_vRows.end() );
		vNewRows.reserve( iNewSize );
		vNewRows.insert( vNewRows.end(), m_vRows.begin(), m_vRows.begin() + m_iOffset );
		vNewRows.resize( iNewSize, false );
		m_vRows.swap( vNewRows );
		m_iOffset = 0;
	}
public:
	JudgedRows()  { Resize( 32 ); }
	// Returns true if the row has already been judged.
	bool JudgeRow( int iRow )
	{
		if( iRow < m_iStart )
			return true;
		if( iRow >= m_iStart + static_cast<int>(m_vRows.size()) )
			Resize( iRow+1-m_iStart );
		const int iIndex = (iRow - m_iStart + m_iOffset) % m_vRows.size();
		const bool ret = m_vRows[iIndex];
		m_vRows[iIndex] = true;
		while( m_vRows[m_iOffset] )
		{
			m_vRows[m_iOffset] = false;
			++m_iStart;
			if( ++m_iOffset >= static_cast<int>(m_vRows.size()) )
				m_iOffset -= m_vRows.size();
		}
		return ret;
	}
	void Reset( int iStart )
	{
		m_iStart = iStart;
		m_iOffset = 0;
		m_vRows.assign( m_vRows.size(), false );
	}
};


RString ATTACK_DISPLAY_X_NAME( size_t p, size_t both_sides )	{ return "AttackDisplayXOffset" + (both_sides ? RString("BothSides") : ssprintf("OneSideP%d", static_cast<int>(p+1)) ); }

void TimingWindowSecondsInit( size_t /*TimingWindow*/ i, RString &sNameOut, float &defaultValueOut )
{
	sNameOut = "TimingWindowSeconds" + TimingWindowToString( (TimingWindow)i );
	switch( i )
	{
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
		case TW_Mine: // same as great
			defaultValueOut = 0.090f;
			break;
		case TW_Hold: // allow enough time to take foot off and put back on
			defaultValueOut = 0.250f;
			break;
		case TW_Roll:
			defaultValueOut = 0.500f;
			break;
		case TW_Checkpoint: // similar to TW_Hold, but a little more strict/accurate to Pump play.
			defaultValueOut = 0.1664f;
			break;
		default:
			FAIL_M(ssprintf("Invalid timing window: %i", static_cast<int>(i)));
	}
}

static Preference<float> m_fTimingWindowScale	( "TimingWindowScale",		1.0f );
static Preference<float> m_fTimingWindowAdd	( "TimingWindowAdd",		0 );
static Preference1D<float> m_fTimingWindowSeconds( TimingWindowSecondsInit, NUM_TimingWindow );
static Preference<float> m_fTimingWindowJump	( "TimingWindowJump",		0.25 );
static Preference<float> m_fMaxInputLatencySeconds	( "MaxInputLatencySeconds",	0.0 );
static Preference<bool> g_bEnableMineSoundPlayback	( "EnableMineHitSound", true );

/** @brief How much life is in a hold note when you start on it? */
ThemeMetric<float> INITIAL_HOLD_LIFE		( "Player", "InitialHoldLife" );
/**
 * @brief How much hold life is possible to have when holding a hold note?
 *
 * This was an sm-ssc addition. */
ThemeMetric<float> MAX_HOLD_LIFE		( "Player", "MaxHoldLife" );
ThemeMetric<bool> PENALIZE_TAP_SCORE_NONE	( "Player", "PenalizeTapScoreNone" );
ThemeMetric<bool> JUDGE_HOLD_NOTES_ON_SAME_ROW_TOGETHER	( "Player", "JudgeHoldNotesOnSameRowTogether" );
ThemeMetric<bool> CHECKPOINTS_FLASH_ON_HOLD ( "Player", "CheckpointsFlashOnHold" ); // sm-ssc addition
ThemeMetric<bool> IMMEDIATE_HOLD_LET_GO	( "Player", "ImmediateHoldLetGo" );
ThemeMetric<bool> COMBO_BREAK_ON_IMMEDIATE_HOLD_LET_GO ( "Player", "ComboBreakOnImmediateHoldLetGo" );
/**
 * @brief Must a Player step on a hold head for a hold to activate?
 *
 * If set to true, the Player must step on a hold head in order for the hold to activate.
 * If set to false, merely holding your foot down as the hold head approaches will suffice. */
ThemeMetric<bool> REQUIRE_STEP_ON_HOLD_HEADS	( "Player", "RequireStepOnHoldHeads" );
/**
 * @brief Must a Player step on a mine for it to activate?
 *
 * If set to true, the Player must step on a mine for it to blow up.
 * If set to false, merely holding your foot down as the mine approaches will suffice. */
ThemeMetric<bool> REQUIRE_STEP_ON_MINES	( "Player", "RequireStepOnMines" );
//ThemeMetric<bool> HOLD_TRIGGERS_TAP_NOTES	( "Player", "HoldTriggersTapNotes" ); // parastar stuff; leave in though
/**
 * @brief Does repeatedly stepping on a roll to keep it alive increment the combo?
 *
 * If set to true, repeatedly stepping on a roll will increment the combo.
 * If set to false, only the roll head causes the combo to be incremented.
 *
 * For those wishing to make a theme very accurate to In The Groove 2, set this to false. */
ThemeMetric<bool> ROLL_BODY_INCREMENTS_COMBO	( "Player", "RollBodyIncrementsCombo" );
/**
 * @brief Does not stepping on a mine increase the combo?
 *
 * If set to true, every mine missed will increment the combo.
 * If set to false, missing a mine will not affect the combo. */
ThemeMetric<bool> AVOID_MINE_INCREMENTS_COMBO	( "Gameplay", "AvoidMineIncrementsCombo" );
/**
 * @brief Does stepping on a mine increment the miss combo?
 *
 * If set to true, every mine stepped on will break the combo and increment the miss combo.
 * If set to false, stepping on a mine will not affect the combo. */
ThemeMetric<bool> MINE_HIT_INCREMENTS_MISS_COMBO	( "Gameplay", "MineHitIncrementsMissCombo" );
/**
 * @brief Are checkpoints and taps considered separate judgments?
 *
 * If set to true, they are considered separate.
 * If set to false, they are considered the same. */
ThemeMetric<bool> CHECKPOINTS_TAPS_SEPARATE_JUDGMENT	( "Player", "CheckpointsTapsSeparateJudgment" );
/**
 * @brief Do we score missed holds and rolls with HoldNoteScores?
 *
 * If set to true, missed holds and rolls are given LetGo judgments.
 * If set to false, missed holds and rolls are given no judgment on the hold side of things. */
ThemeMetric<bool> SCORE_MISSED_HOLDS_AND_ROLLS ( "Player", "ScoreMissedHoldsAndRolls" );
/** @brief How much of the song must have gone by before a Player's combo is colored? */
ThemeMetric<float> PERCENT_UNTIL_COLOR_COMBO ( "Player", "PercentUntilColorCombo" );
/** @brief How much combo must be earned before the announcer says "Combo Stopped"? */
ThemeMetric<int> COMBO_STOPPED_AT ( "Player", "ComboStoppedAt" );
ThemeMetric<float> ATTACK_RUN_TIME_RANDOM ( "Player", "AttackRunTimeRandom" );
ThemeMetric<float> ATTACK_RUN_TIME_MINE ( "Player", "AttackRunTimeMine" );

/**
 * @brief What is our highest cap for mMods?
 *
 * If set to 0 or less, assume the song takes over. */
ThemeMetric<float> M_MOD_HIGH_CAP("Player", "MModHighCap");

/** @brief Will battle modes have their steps mirrored or kept the same? */
ThemeMetric<bool> BATTLE_RAVE_MIRROR ( "Player", "BattleRaveMirror" );

float Player::GetWindowSeconds( TimingWindow tw )
{
	float fSecs = m_fTimingWindowSeconds[tw];
	fSecs *= m_fTimingWindowScale;
	fSecs += m_fTimingWindowAdd;
	return fSecs;
}

Player::Player( NoteData &nd, bool bVisibleParts ) : m_NoteData(nd)
{
	m_drawing_notefield_board= false;
	m_bLoaded = false;
	m_inside_lua_set_life= false;

	m_pPlayerState = NULL;
	m_pPlayerStageStats = NULL;
	m_fNoteFieldHeight = 0;

	m_pLifeMeter = NULL;
	m_pScoreDisplay = NULL;
	m_pSecondaryScoreDisplay = NULL;
	m_pPrimaryScoreKeeper = NULL;
	m_pSecondaryScoreKeeper = NULL;
	m_pIterNeedsTapJudging = NULL;
	m_pIterNeedsHoldJudging = NULL;
	m_pIterUncrossedRows = NULL;
	m_pIterUnjudgedRows = NULL;
	m_pIterUnjudgedMineRows = NULL;

	m_bPaused = false;
	m_bDelay = false;

	PlayerAI::InitFromDisk();

	m_pNoteField = NULL;
	if( bVisibleParts )
	{
		m_pNoteField = new NoteField;
		m_pNoteField->SetName( "NoteField" );
	}
	m_pJudgedRows = new JudgedRows;

	m_bSendJudgmentAndComboMessages = true;
}

Player::~Player()
{
	SAFE_DELETE( m_pNoteField );
	for( unsigned i = 0; i < m_vpHoldJudgment.size(); ++i )
		SAFE_DELETE( m_vpHoldJudgment[i] );
	SAFE_DELETE( m_pJudgedRows );
	SAFE_DELETE( m_pIterNeedsTapJudging );
	SAFE_DELETE( m_pIterNeedsHoldJudging );
	SAFE_DELETE( m_pIterUncrossedRows );
	SAFE_DELETE( m_pIterUnjudgedRows );
	SAFE_DELETE( m_pIterUnjudgedMineRows );
	
}

/* Init() does the expensive stuff: load sounds and noteskins.  Load() just loads a NoteData. */
void Player::Init(
	const RString &sType,
	PlayerState* pPlayerState, 
	PlayerStageStats* pPlayerStageStats,
	LifeMeter* pLM, 
	ScoreDisplay* pScoreDisplay, 
	ScoreDisplay* pSecondaryScoreDisplay, 
	ScoreKeeper* pPrimaryScoreKeeper, 
	ScoreKeeper* pSecondaryScoreKeeper )
{

	GRAY_ARROWS_Y_STANDARD.Load(			sType, "ReceptorArrowsYStandard" );
	GRAY_ARROWS_Y_REVERSE.Load(			sType, "ReceptorArrowsYReverse" );
	HOLD_JUDGMENT_Y_STANDARD.Load(			sType, "HoldJudgmentYStandard" );
	HOLD_JUDGMENT_Y_REVERSE.Load(			sType, "HoldJudgmentYReverse" );
	BRIGHT_GHOST_COMBO_THRESHOLD.Load(		sType, "BrightGhostComboThreshold" );
	TAP_JUDGMENTS_UNDER_FIELD.Load(			sType, "TapJudgmentsUnderField" );
	HOLD_JUDGMENTS_UNDER_FIELD.Load(		sType, "HoldJudgmentsUnderField" );
	COMBO_UNDER_FIELD.Load(		sType, "ComboUnderField" );
	DRAW_DISTANCE_AFTER_TARGET_PIXELS.Load(		sType, "DrawDistanceAfterTargetsPixels" );
	DRAW_DISTANCE_BEFORE_TARGET_PIXELS.Load(	sType, "DrawDistanceBeforeTargetsPixels" );

	{
		// Init judgment positions
		bool bPlayerUsingBothSides = GAMESTATE->GetCurrentStyle(pPlayerState->m_PlayerNumber)->GetUsesCenteredArrows();
		Actor TempJudgment;
		TempJudgment.SetName( "Judgment" );
		ActorUtil::LoadCommand( TempJudgment, sType, "Transform" );

		Actor TempCombo;
		TempCombo.SetName( "Combo" );
		ActorUtil::LoadCommand( TempCombo, sType, "Transform" );

		int iEnabledPlayerIndex = -1;
		int iNumEnabledPlayers = 0;
		if( GAMESTATE->m_bMultiplayer )
		{
			FOREACH_EnabledMultiPlayer( p )
			{
				if( p == pPlayerState->m_mp )
					iEnabledPlayerIndex = iNumEnabledPlayers;
				iNumEnabledPlayers++;
			}
		}
		else
		{
			FOREACH_EnabledPlayer( p )
			{
				if( p == pPlayerState->m_PlayerNumber )
					iEnabledPlayerIndex = iNumEnabledPlayers;
				iNumEnabledPlayers++;
			}
		}

		if( iNumEnabledPlayers == 0 )	// hack for ScreenHowToPlay where no players are joined
		{
			iEnabledPlayerIndex = 0;
			iNumEnabledPlayers = 1;
		}

		for( int i=0; i<NUM_REVERSE; i++ )
		{
			for( int j=0; j<NUM_CENTERED; j++ )
			{
				Message msg( "Transform" );
				msg.SetParam( "Player", pPlayerState->m_PlayerNumber );
				msg.SetParam( "MultiPlayer", pPlayerState->m_mp );
				msg.SetParam( "iEnabledPlayerIndex", iEnabledPlayerIndex );
				msg.SetParam( "iNumEnabledPlayers", iNumEnabledPlayers );
				msg.SetParam( "bPlayerUsingBothSides", bPlayerUsingBothSides );
				msg.SetParam( "bReverse", !!i );
				msg.SetParam( "bCentered", !!j );

				TempJudgment.HandleMessage( msg );
				m_tsJudgment[i][j] = TempJudgment.DestTweenState();

				TempCombo.HandleMessage( msg );
				m_tsCombo[i][j] = TempCombo.DestTweenState();
			}
		}
	}

	this->SortByDrawOrder();

	m_pPlayerState = pPlayerState;
	m_pPlayerStageStats = pPlayerStageStats;
	m_pLifeMeter = pLM;
	m_pScoreDisplay = pScoreDisplay;
	m_pSecondaryScoreDisplay = pSecondaryScoreDisplay;
	m_pPrimaryScoreKeeper = pPrimaryScoreKeeper;
	m_pSecondaryScoreKeeper = pSecondaryScoreKeeper;

	m_iLastSeenCombo      = 0;
	m_bSeenComboYet       = false;
	
	// set initial life
	if( m_pLifeMeter && m_pPlayerStageStats )
	{
		float fLife = m_pLifeMeter->GetLife();
		m_pPlayerStageStats->SetLifeRecordAt( fLife, STATSMAN->m_CurStageStats.m_fStepsSeconds );
		m_pPlayerStageStats->SetWifeRecordAt( 1.f, STATSMAN->m_CurStageStats.m_fStepsSeconds);
	}

	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	m_pPlayerState->SetNumCols(GAMESTATE->GetCurrentStyle(pn)->m_iColsPerPlayer);

	RageSoundLoadParams SoundParams;
	SoundParams.m_bSupportPan = true;
	m_soundMine.Load( THEME->GetPathS(sType,"mine"), true, &SoundParams );

	// calculate M-mod speed here, so we can adjust properly on a per-song basis.
	// XXX: can we find a better location for this?
	// Always calculate the reading bpm, to allow switching to an mmod mid-song.
	{
		DisplayBpms bpms;

		ASSERT( GAMESTATE->m_pCurSong != NULL );
		GAMESTATE->m_pCurSong->GetDisplayBpms( bpms );

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
		if( !bpms.IsSecret() )
		{
			fMaxBPM = (M_MOD_HIGH_CAP > 0 ? 
				   bpms.GetMaxWithin(M_MOD_HIGH_CAP) : 
				   bpms.GetMax());
			fMaxBPM = max( 0, fMaxBPM );
		}

		// we can't rely on the displayed BPMs, so manually calculate.
		if( fMaxBPM == 0 )
		{
			float fThrowAway = 0;

				if (M_MOD_HIGH_CAP > 0)
					GAMESTATE->m_pCurSong->m_SongTiming.GetActualBPM( fThrowAway, fMaxBPM, M_MOD_HIGH_CAP );
				else
					GAMESTATE->m_pCurSong->m_SongTiming.GetActualBPM( fThrowAway, fMaxBPM );
		}

		ASSERT( fMaxBPM > 0 );
		m_pPlayerState->m_fReadBPM= fMaxBPM;
	}

	float fBalance = GameSoundManager::GetPlayerBalance( pn );
	m_soundMine.SetProperty( "Pan", fBalance );

	if( HasVisibleParts() )
	{
		LuaThreadVariable var( "Player", LuaReference::Create(m_pPlayerState->m_PlayerNumber) );
		LuaThreadVariable var2( "MultiPlayer", LuaReference::Create(m_pPlayerState->m_mp) );

		m_sprCombo.Load( THEME->GetPathG(sType,"combo") );
		m_sprCombo->SetName( "Combo" );
		m_pActorWithComboPosition = &*m_sprCombo;
		this->AddChild( m_sprCombo );

		// todo: allow for judgments to be loaded per-column a la pop'n?
		// see how HoldJudgments are handled below for an example, though
		// it would need more work. -aj
		m_sprJudgment.Load( THEME->GetPathG(sType,"judgment") );
		m_sprJudgment->SetName( "Judgment" );
		m_pActorWithJudgmentPosition = &*m_sprJudgment;
		this->AddChild( m_sprJudgment );
	}
	else
	{
		m_pActorWithComboPosition = NULL;
		m_pActorWithJudgmentPosition = NULL;
	}

	// Load HoldJudgments
	m_vpHoldJudgment.resize( GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->m_iColsPerPlayer );
	for( int i = 0; i < GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->m_iColsPerPlayer; ++i )
		m_vpHoldJudgment[i] = NULL;

	if( HasVisibleParts() )
	{
		for( int i = 0; i < GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->m_iColsPerPlayer; ++i )
		{
			auto *pJudgment = new HoldJudgment;
			// xxx: assumes sprite; todo: don't force 1x2 -aj
			pJudgment->Load( THEME->GetPathG("HoldJudgment","label 1x2") );
			m_vpHoldJudgment[i] = pJudgment;
			this->AddChild( m_vpHoldJudgment[i] );
		}
	}

	m_fNoteFieldHeight = GRAY_ARROWS_Y_REVERSE-GRAY_ARROWS_Y_STANDARD;
	if( m_pNoteField )
	{
		m_pNoteField->Init( m_pPlayerState, m_fNoteFieldHeight );
		ActorUtil::LoadAllCommands( *m_pNoteField, sType );
		this->AddChild( m_pNoteField );
	}

	m_vbFretIsDown.resize( GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->m_iColsPerPlayer );
	FOREACH( bool, m_vbFretIsDown, b )
		*b = false;
}
/**
 * @brief Determine if a TapNote needs a tap note style judgment.
 * @param tn the TapNote in question.
 * @return true if it does, false otherwise. */
static bool NeedsTapJudging( const TapNote &tn )
{
	switch( tn.type )
	{
	DEFAULT_FAIL( tn.type );
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
static bool NeedsHoldJudging( const TapNote &tn )
{
	switch( tn.type )
	{
	DEFAULT_FAIL( tn.type );
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

static void GenerateCacheDataStructure(PlayerState *pPlayerState, const NoteData &notes) {

	pPlayerState->m_CacheDisplayedBeat.clear();

	const vector<TimingSegment*> vScrolls = pPlayerState->GetDisplayedTiming().GetTimingSegments( SEGMENT_SCROLL );

	float displayedBeat = 0.0f;
	float lastRealBeat = 0.0f;
	float lastRatio = 1.0f;
	for ( unsigned i = 0; i < vScrolls.size(); i++ )
	{
		ScrollSegment *seg = ToScroll( vScrolls[i] );
		displayedBeat += ( seg->GetBeat() - lastRealBeat ) * lastRatio;
		lastRealBeat = seg->GetBeat();
		lastRatio = seg->GetRatio();
		CacheDisplayedBeat c = { seg->GetBeat(), displayedBeat, seg->GetRatio() };
		pPlayerState->m_CacheDisplayedBeat.push_back( c );
	}
	
	pPlayerState->m_CacheNoteStat.clear();
	
	NoteData::all_tracks_const_iterator it = notes.GetTapNoteRangeAllTracks( 0, MAX_NOTE_ROW, true );
	int count = 0, lastCount = 0;
	for( ; !it.IsAtEnd(); ++it )
	{
		for( int t = 0; t < notes.GetNumTracks(); t++ )
		{
			if( notes.GetTapNote( t, it.Row() ) != TAP_EMPTY ) count ++;
		}
		CacheNoteStat c = { NoteRowToBeat(it.Row()), lastCount, count  };
		lastCount = count;
		pPlayerState->m_CacheNoteStat.push_back(c);
	}

}

void Player::Load()
{
	m_bLoaded = true;

	// Figured this is probably a little expensive so let's cache it
	m_bTickHolds = GAMESTATE->GetCurrentGame()->m_bTickHolds;

	m_LastTapNoteScore = TNS_None;
	// The editor can start playing in the middle of the song.
	const int iNoteRow = BeatToNoteRow( m_pPlayerState->m_Position.m_fSongBeat );
	m_iFirstUncrossedRow     = iNoteRow - 1;
	m_pJudgedRows->Reset( iNoteRow );

	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	bool bOniDead = m_pPlayerState->m_PlayerOptions.GetStage().m_LifeType == LifeType_Battery  &&  
		(m_pPlayerStageStats == NULL || m_pPlayerStageStats->m_bFailed);

	/* The editor reuses Players ... so we really need to make sure everything
	 * is reset and not tweening.  Perhaps ActorFrame should recurse to subactors;
	 * then we could just this->StopTweening()? -glenn */
	// hurr why don't you just set m_bPropagateCommands on it then -aj
	if( m_sprJudgment )
		m_sprJudgment->PlayCommand("Reset");
	if( m_pPlayerStageStats )
	{
		SetCombo( m_pPlayerStageStats->m_iCurCombo, m_pPlayerStageStats->m_iCurMissCombo );	// combo can persist between songs and games
	}

	/* Don't re-init this; that'll reload graphics.  Add a separate Reset() call
	 * if some ScoreDisplays need it. */
//	if( m_pScore )
//		m_pScore->Init( pn );

	// Mina garbage - Mina
	m_Timing = GAMESTATE->m_pCurSteps[pn]->GetTimingData();
	m_Timing->NegStopAndBPMCheck();
	int lastRow = m_NoteData.GetLastRow();
	m_Timing->BuildAndGetEtar(lastRow);
	
	totalwifescore = m_NoteData.WifeTotalScoreCalc(m_Timing, 0, 1073741824);
	curwifescore = 0.f;
	maxwifescore = 0.f;
	
	m_NoteData.LogNonEmptyRows();
	nerv = m_NoteData.GetNonEmptyRowVector();
	const vector<float>& etaner = m_Timing->BuildAndGetEtaner(nerv);
	m_pPlayerStageStats->serializednd = m_NoteData.SerializeNoteData(etaner);
	m_NoteData.UnsetSerializedNoteData();

	if (m_Timing->HasWarps())
		m_pPlayerStageStats->filehadnegbpms = true;
	

	Profile *pProfile = PROFILEMAN->GetProfile(pn);
	const HighScore* pb = SCOREMAN->GetChartPBAt(GAMESTATE->m_pCurSteps[pn]->GetChartKey(), GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate);
	if (pb)
		wifescorepersonalbest = pb->GetWifeScore();
	else
		wifescorepersonalbest = m_pPlayerState->playertargetgoal;
		
	if (m_pPlayerStageStats)
		m_pPlayerStageStats->m_fTimingScale = m_fTimingWindowScale;

	/* Apply transforms. */
	NoteDataUtil::TransformNoteData(m_NoteData, *m_Timing, m_pPlayerState->m_PlayerOptions.GetStage(), GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->m_StepsType);

	const Song* pSong = GAMESTATE->m_pCurSong;

	// Generate some cache data structure.
	GenerateCacheDataStructure(m_pPlayerState, m_NoteData);

	int iDrawDistanceAfterTargetsPixels = GAMESTATE->IsEditing() ? -100 : DRAW_DISTANCE_AFTER_TARGET_PIXELS;
	int iDrawDistanceBeforeTargetsPixels = GAMESTATE->IsEditing() ? 400 : DRAW_DISTANCE_BEFORE_TARGET_PIXELS;

	float fNoteFieldMiddle = (GRAY_ARROWS_Y_STANDARD+GRAY_ARROWS_Y_REVERSE)/2;
	
	if( m_pNoteField && !bOniDead )
	{
		m_pNoteField->SetY( fNoteFieldMiddle );
		m_pNoteField->Load( &m_NoteData, iDrawDistanceAfterTargetsPixels, iDrawDistanceBeforeTargetsPixels );
	}

	bool bPlayerUsingBothSides = GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->GetUsesCenteredArrows();

	// set this in Update 
	//m_pJudgment->SetX( JUDGMENT_X.GetValue(pn,bPlayerUsingBothSides) );
	//m_pJudgment->SetY( bReverse ? JUDGMENT_Y_REVERSE : JUDGMENT_Y );

	// Need to set Y positions of all these elements in Update since
	// they change depending on PlayerOptions.


	// Load keysounds.  If sounds are already loaded (as in the editor), don't reload them.
	// XXX: the editor will load several duplicate copies (in each NoteField), and each
	// player will load duplicate sounds.  Does this belong somewhere else (perhaps in
	// a separate object, used alongside ScreenGameplay::m_pSoundMusic and ScreenEdit::m_pSoundMusic?)
	// We don't have to load separate copies to set player fade: always make a copy, and set the
	// fade on the copy.
	RString sSongDir = pSong->GetSongDir();
	m_vKeysounds.resize( pSong->m_vsKeysoundFile.size() );

	// parameters are invalid somehow... -aj
	RageSoundLoadParams SoundParams;
	SoundParams.m_bSupportPan = true;

	float fBalance = GameSoundManager::GetPlayerBalance( pn );
	for( unsigned i=0; i<m_vKeysounds.size(); i++ )
	{
		RString sKeysoundFilePath = sSongDir + pSong->m_vsKeysoundFile[i];
		RageSound& sound = m_vKeysounds[i];
		if( sound.GetLoadedFilePath() != sKeysoundFilePath )
			sound.Load( sKeysoundFilePath, true, &SoundParams );
		sound.SetProperty( "Pan", fBalance );
		sound.SetStopModeFromString( "stop" );
	}

	if( m_pPlayerStageStats )
		SendComboMessages( m_pPlayerStageStats->m_iCurCombo, m_pPlayerStageStats->m_iCurMissCombo );

	SAFE_DELETE( m_pIterNeedsTapJudging );
	m_pIterNeedsTapJudging = new NoteData::all_tracks_iterator( m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW) );

	SAFE_DELETE( m_pIterNeedsHoldJudging );
	m_pIterNeedsHoldJudging = new NoteData::all_tracks_iterator( m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW ) );

	SAFE_DELETE( m_pIterUncrossedRows );
	m_pIterUncrossedRows = new NoteData::all_tracks_iterator( m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW ) );

	SAFE_DELETE( m_pIterUnjudgedRows );
	m_pIterUnjudgedRows = new NoteData::all_tracks_iterator( m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW ) );

	SAFE_DELETE( m_pIterUnjudgedMineRows );
	m_pIterUnjudgedMineRows = new NoteData::all_tracks_iterator( m_NoteData.GetTapNoteRangeAllTracks(iNoteRow, MAX_NOTE_ROW ) );
}

void Player::SendComboMessages( unsigned int iOldCombo, unsigned int iOldMissCombo )
{
	const unsigned int iCurCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurCombo : 0;
	if( iOldCombo > (unsigned int)COMBO_STOPPED_AT && iCurCombo < (unsigned int)COMBO_STOPPED_AT )
	{
		SCREENMAN->PostMessageToTopScreen( SM_ComboStopped, 0 );
	}

	if( m_bSendJudgmentAndComboMessages )
	{
		Message msg( "ComboChanged" );
		msg.SetParam( "Player", m_pPlayerState->m_PlayerNumber );
		msg.SetParam( "OldCombo", iOldCombo );
		msg.SetParam( "OldMissCombo", iOldMissCombo );
		if( m_pPlayerState )
			msg.SetParam( "PlayerState", LuaReference::CreateFromPush(*m_pPlayerState) );
		if( m_pPlayerStageStats )
			msg.SetParam( "PlayerStageStats", LuaReference::CreateFromPush(*m_pPlayerStageStats) );
		MESSAGEMAN->Broadcast( msg );
	}
}

void Player::Update( float fDeltaTime )
{
	//const RageTimer now;
	const auto now = std::chrono::steady_clock::now();
	// Don't update if we haven't been loaded yet.
	if( !m_bLoaded )
		return;

	//LOG->Trace( "Player::Update(%f)", fDeltaTime );

	if( GAMESTATE->m_pCurSong==NULL || IsOniDead() )
		return;

	ActorFrame::Update( fDeltaTime );

	if(m_pPlayerState->m_mp != MultiPlayer_Invalid)
	{
		/* In multiplayer, it takes too long to run player updates for every player each frame;
		 * with 32 players and three difficulties, we have 96 Players to update.  Stagger these
		 * updates, by only updating a few players each update; since we don't have screen elements
		 * tightly tied to user actions in this mode, this doesn't degrade gameplay.  Run 4 players
		 * per update, which means 12 Players in 3-difficulty mode.
		 */
		static int iCycle = 0;
		iCycle = (iCycle + 1) % 8;

		if((m_pPlayerState->m_mp % 8) != iCycle)
			return;
	}

	const float fSongBeat = m_pPlayerState->m_Position.m_fSongBeat;
	const int iSongRow = BeatToNoteRow( fSongBeat );

	ArrowEffects::SetCurrentOptions(&m_pPlayerState->m_PlayerOptions.GetCurrent());

	// Optimization: Don't spend time processing the things below that won't show 
	// if the Player doesn't show anything on the screen.
	if( HasVisibleParts() )
	{
		float fMiniPercent = m_pPlayerState->m_PlayerOptions.GetCurrent().m_fEffects[PlayerOptions::EFFECT_MINI];
		float fTinyPercent = m_pPlayerState->m_PlayerOptions.GetCurrent().m_fEffects[PlayerOptions::EFFECT_TINY];
		float fJudgmentZoom = min( powf(0.5f, fMiniPercent+fTinyPercent), 1.0f );

		// Update Y positions
		{
			for( int c=0; c<GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->m_iColsPerPlayer; c++ )
			{
				float fPercentReverse = m_pPlayerState->m_PlayerOptions.GetCurrent().GetReversePercentForColumn(c);
				float fHoldJudgeYPos = SCALE( fPercentReverse, 0.f, 1.f, HOLD_JUDGMENT_Y_STANDARD, HOLD_JUDGMENT_Y_REVERSE );
				//float fGrayYPos = SCALE( fPercentReverse, 0.f, 1.f, GRAY_ARROWS_Y_STANDARD, GRAY_ARROWS_Y_REVERSE );

				float fX = ArrowEffects::GetXPos( m_pPlayerState, c, 0 );
				const float fZ = ArrowEffects::GetZPos(c, 0);
				fX *= ( 1 - fMiniPercent * 0.5f );

				m_vpHoldJudgment[c]->SetX( fX );
				m_vpHoldJudgment[c]->SetY( fHoldJudgeYPos );
				m_vpHoldJudgment[c]->SetZ( fZ );
				m_vpHoldJudgment[c]->SetZoom( fJudgmentZoom );
			}
		}

		// NoteField accounts for reverse on its own now.
		//if( m_pNoteField )
		//	m_pNoteField->SetY( fGrayYPos );

		const bool bReverse = m_pPlayerState->m_PlayerOptions.GetCurrent().GetReversePercentForColumn(0) == 1;
		float fPercentCentered = m_pPlayerState->m_PlayerOptions.GetCurrent().m_fScrolls[PlayerOptions::SCROLL_CENTERED];

		if( m_pActorWithJudgmentPosition != NULL )
		{
			const Actor::TweenState &ts1 = m_tsJudgment[bReverse?1:0][0];
			const Actor::TweenState &ts2 = m_tsJudgment[bReverse?1:0][1];
			Actor::TweenState::MakeWeightedAverage( m_pActorWithJudgmentPosition->DestTweenState(), ts1, ts2, fPercentCentered );
		}

		if( m_pActorWithComboPosition != NULL )
		{
			const Actor::TweenState &ts1 = m_tsCombo[bReverse?1:0][0];
			const Actor::TweenState &ts2 = m_tsCombo[bReverse?1:0][1];
			Actor::TweenState::MakeWeightedAverage( m_pActorWithComboPosition->DestTweenState(), ts1, ts2, fPercentCentered );
		}

		float fNoteFieldZoom = 1 - fMiniPercent*0.5f;
		if( m_pNoteField )
			m_pNoteField->SetZoom( fNoteFieldZoom );
		if( m_pActorWithJudgmentPosition != NULL )
			m_pActorWithJudgmentPosition->SetZoom( m_pActorWithJudgmentPosition->GetZoom() * fJudgmentZoom );
		if( m_pActorWithComboPosition != NULL )
			m_pActorWithComboPosition->SetZoom( m_pActorWithComboPosition->GetZoom() * fJudgmentZoom );
	}

	// If we're paused, don't update tap or hold note logic, so hold notes can be released
	// during pause.
	if( m_bPaused )
		return;

	// update pressed flag
	const int iNumCols = GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->m_iColsPerPlayer;
	ASSERT_M( iNumCols <= MAX_COLS_PER_PLAYER, ssprintf("%i > %i", iNumCols, MAX_COLS_PER_PLAYER) );
	for( int col=0; col < iNumCols; ++col )
	{
		ASSERT( m_pPlayerState != NULL );

		// TODO: Remove use of PlayerNumber.
		vector<GameInput> GameI;
		GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->StyleInputToGameInput( col, m_pPlayerState->m_PlayerNumber, GameI );

		bool bIsHoldingButton= INPUTMAPPER->IsBeingPressed(GameI);

		// TODO: Make this work for non-human-controlled players
		if( bIsHoldingButton && !GAMESTATE->m_bDemonstrationOrJukebox && m_pPlayerState->m_PlayerController==PC_HUMAN )
			if( m_pNoteField )
				m_pNoteField->SetPressed( col );
	}

	// handle Autoplay for rolls
	if( m_pPlayerState->m_PlayerController != PC_HUMAN )
	{
		for( int iTrack=0; iTrack<m_NoteData.GetNumTracks(); ++iTrack )
		{
			// TODO: Make the CPU miss sometimes.
			int iHeadRow;
			if( !m_NoteData.IsHoldNoteAtRow(iTrack, iSongRow, &iHeadRow) )
				iHeadRow = iSongRow;

			const TapNote &tn = m_NoteData.GetTapNote( iTrack, iHeadRow );
			if( tn.type != TapNoteType_HoldHead || tn.subType != TapNoteSubType_Roll )
				continue;
			if( tn.HoldResult.hns != HNS_None )
				continue;
			if( tn.HoldResult.fLife >= 0.5f )
				continue;

			Step( iTrack, iHeadRow, now, false, false );
			if( m_pPlayerState->m_PlayerController == PC_AUTOPLAY )
			{
				STATSMAN->m_CurStageStats.m_bUsedAutoplay = true;
				if( m_pPlayerStageStats )
					m_pPlayerStageStats->m_bDisqualified = true;
			}
		}
	}


	// update HoldNotes logic
	{

		// Fast forward to the first that needs hold judging.
		{
			NoteData::all_tracks_iterator &iter = *m_pIterNeedsHoldJudging;
			while( !iter.IsAtEnd()  &&  iter.Row() <= iSongRow  &&  !NeedsHoldJudging(*iter) )
				++iter;
		}

		vector<TrackRowTapNote> vHoldNotesToGradeTogether;
		int iRowOfLastHoldNote = -1;
		NoteData::all_tracks_iterator iter = *m_pIterNeedsHoldJudging;	// copy
		for( ; !iter.IsAtEnd() &&  iter.Row() <= iSongRow; ++iter )
		{
			TapNote &tn = *iter;
			if( tn.type != TapNoteType_HoldHead )
				continue;

			int iTrack = iter.Track();
			int iRow = iter.Row();
			TrackRowTapNote trtn = { iTrack, iRow, &tn };

			/* All holds must be of the same subType because fLife is handled 
			 * in different ways depending on the SubType. Handle Rolls one at
			 * a time and don't mix with holds. */
			switch( tn.subType )
			{
			DEFAULT_FAIL( tn.subType );
			case TapNoteSubType_Hold:
				break;
			case TapNoteSubType_Roll:
				{
					vector<TrackRowTapNote> v;
					v.push_back( trtn );
					UpdateHoldNotes( iSongRow, fDeltaTime, v );
				}
				continue;	// don't process this below
			}
			/*
			case TapNoteSubType_Mine:
				break;
			*/

			if( iRow != iRowOfLastHoldNote  ||  !JUDGE_HOLD_NOTES_ON_SAME_ROW_TOGETHER )
			{
				if( !vHoldNotesToGradeTogether.empty() )
				{
					//LOG->Trace( ssprintf("UpdateHoldNotes; %i != %i || !judge holds on same row together",iRow,iRowOfLastHoldNote) );
					UpdateHoldNotes( iSongRow, fDeltaTime, vHoldNotesToGradeTogether );
					vHoldNotesToGradeTogether.clear();
				}
			}
			iRowOfLastHoldNote = iRow;
			vHoldNotesToGradeTogether.push_back( trtn );
		}

		if( !vHoldNotesToGradeTogether.empty() )
		{
			//LOG->Trace("UpdateHoldNotes since !vHoldNotesToGradeTogether.empty()");
			UpdateHoldNotes( iSongRow, fDeltaTime, vHoldNotesToGradeTogether );
			vHoldNotesToGradeTogether.clear();
 		}
	}

	{
		const int iRowNow = BeatToNoteRow( m_pPlayerState->m_Position.m_fSongBeat );
		if( iRowNow >= 0 )
		{
			if( GAMESTATE->IsPlayerEnabled(m_pPlayerState) )
			{
				if(m_pPlayerState->m_Position.m_bDelay)
				{
					if( !m_bDelay )
						m_bDelay = true;
				}
				else
				{
					if(m_bDelay)
					{
						if(m_pPlayerState->m_PlayerController != PC_HUMAN)
						{
							CrossedRows( iRowNow-1, now );
						}
						m_bDelay = false;
					}
					CrossedRows( iRowNow, now );
				}
			}
		}
	}

	// Check for completely judged rows.
	UpdateJudgedRows(fDeltaTime);
	UpdateTapNotesMissedOlderThan( GetMaxStepDistanceSeconds() );
}

// Update a group of holds with shared scoring/life. All of these holds will have the same start row.
void Player::UpdateHoldNotes( int iSongRow, float fDeltaTime, vector<TrackRowTapNote> &vTN )
{
	ASSERT( !vTN.empty() );

	//LOG->Trace("--------------------------------");
	/*
	LOG->Trace("[Player::UpdateHoldNotes] begins");
	LOG->Trace( ssprintf("song row %i, deltaTime = %f",iSongRow,fDeltaTime) );
	*/

	int iStartRow = vTN[0].iRow;
	int iMaxEndRow = INT_MIN;
	int iFirstTrackWithMaxEndRow = -1;

	TapNoteSubType subType = TapNoteSubType_Invalid;
	FOREACH( TrackRowTapNote, vTN, trtn )
	{
		int iTrack = trtn->iTrack;
		ASSERT( iStartRow == trtn->iRow );
		TapNote &tn = *trtn->pTN;
		int iEndRow = iStartRow + tn.iDuration;
		if( subType == TapNoteSubType_Invalid )
			subType = tn.subType;

		/* All holds must be of the same subType because fLife is handled 
		 * in different ways depending on the SubType. */
		ASSERT( tn.subType == subType );

		if( iEndRow > iMaxEndRow )
		{
			iMaxEndRow = iEndRow;
			iFirstTrackWithMaxEndRow = iTrack;
		}
	}

	ASSERT( iFirstTrackWithMaxEndRow != -1 );
	//LOG->Trace( ssprintf("start row: %i; max/end row: = %i",iStartRow,iMaxEndRow) );
	//LOG->Trace( ssprintf("first track with max end row = %i",iFirstTrackWithMaxEndRow) );
	//LOG->Trace( ssprintf("max end row - start row (in beats) = %f",NoteRowToBeat(iMaxEndRow)-NoteRowToBeat(iStartRow)) );

	FOREACH( TrackRowTapNote, vTN, trtn )
	{
		TapNote &tn = *trtn->pTN;

		// set hold flags so NoteField can do intelligent drawing
		tn.HoldResult.bHeld = false;
		tn.HoldResult.bActive = false;

		int iRow = trtn->iRow;
		//LOG->Trace( ssprintf("this row: %i",iRow) );

		// If the song beat is in the range of this hold:
		if( iRow <= iSongRow  &&  iRow <= iMaxEndRow )
		{
			//LOG->Trace( ssprintf("overlap time before: %f",tn.HoldResult.fOverlappedTime) );
			tn.HoldResult.fOverlappedTime += fDeltaTime;
			//LOG->Trace( ssprintf("overlap time after: %f",tn.HoldResult.fOverlappedTime) );
		}
		else
		{
			//LOG->Trace( "overlap time = 0" );
			tn.HoldResult.fOverlappedTime = 0;
		}
	}

	HoldNoteScore hns = vTN[0].pTN->HoldResult.hns;
	float fLife = vTN[0].pTN->HoldResult.fLife;

	if( hns != HNS_None )	// if this HoldNote already has a result
	{
		//LOG->Trace("hold note has a result, skipping.");
		return;	// we don't need to update the logic for this group
	}

	//LOG->Trace("hold note doesn't already have result, let's check.");

	//LOG->Trace( ssprintf("[C++] hold note score: %s",HoldNoteScoreToString(hns).c_str()) );
	//LOG->Trace(ssprintf("[Player::UpdateHoldNotes] fLife = %f",fLife));

	bool bSteppedOnHead = true;
	bool bHeadJudged = true;
	FOREACH( TrackRowTapNote, vTN, trtn )
	{
		TapNote &tn = *trtn->pTN;
		TapNoteScore tns = tn.result.tns;
		//LOG->Trace( ssprintf("[C++] tap note score: %s",StringConversion::ToString(tns).c_str()) );

		// TODO: When using JUDGE_HOLD_NOTES_ON_SAME_ROW_TOGETHER, require that the whole row of 
		// taps was hit before activating this group of holds.
		/* Something about the logic in this section is causing 192nd steps to
		 * fail for some odd reason. -aj */
		bSteppedOnHead &= (tns != TNS_Miss && tns != TNS_None);	// did they step on the start of this hold?
		bHeadJudged &= (tns != TNS_None);	// has this hold really even started yet?	

		/*
		if(bSteppedOnHead)
			LOG->Trace("[Player::UpdateHoldNotes] player stepped on head");
		else
			LOG->Trace("[Player::UpdateHoldNotes] player didn't step on the head");
		*/
	}

	bool bInitiatedNote;
	if( REQUIRE_STEP_ON_HOLD_HEADS )
	{
		// XXX HACK: Miniholds (a 64th or 192nd length hold) will not always
		// register as Held, even if you hit the note. This is considered a
		// major roadblock to adoption, so until a proper fix is found,
		// DON'T REMOVE THIS HACK! -aj
		/*if( iMaxEndRow-iStartRow <= 4 )
			bInitiatedNote = true;
		else*/
			bInitiatedNote = bSteppedOnHead;
	}
	else
	{
		bInitiatedNote = true;
		bHeadJudged = true;
	}

	bool bIsHoldingButton = true;
	FOREACH( TrackRowTapNote, vTN, trtn )
	{
		/*if this hold is already done, pretend it's always being pressed.
		fixes/masks the phantom hold issue. -FSX*/
		// That interacts badly with !IMMEDIATE_HOLD_LET_GO,
		// causing ALL holds to be judged HNS_Held whether they were or not.
		if( !IMMEDIATE_HOLD_LET_GO || (iStartRow + trtn->pTN->iDuration) > iSongRow )
		{
			int iTrack = trtn->iTrack;

			// TODO: Remove use of PlayerNumber.
			PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

			if( m_pPlayerState->m_PlayerController != PC_HUMAN )
			{
			// TODO: Make the CPU miss sometimes.
				if( m_pPlayerState->m_PlayerController == PC_AUTOPLAY )
				{
					STATSMAN->m_CurStageStats.m_bUsedAutoplay = true;
					if( m_pPlayerStageStats != NULL )
						m_pPlayerStageStats->m_bDisqualified = true;
				}
			}
			else
			{
				vector<GameInput> GameI;
				GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->StyleInputToGameInput( iTrack, pn, GameI );

				bIsHoldingButton &= INPUTMAPPER->IsBeingPressed(GameI, m_pPlayerState->m_mp);
			}
		}
	}

	if( bInitiatedNote && fLife != 0 && bHeadJudged )
	{
		//LOG->Trace("[Player::UpdateHoldNotes] initiated note, fLife != 0");
		/* This hold note is not judged and we stepped on its head.
		 * Update iLastHeldRow. Do this even if we're a little beyond the end
		 * of the hold note, to make sure iLastHeldRow is clamped to iEndRow
		 * if the hold note is held all the way. */
		FOREACH( TrackRowTapNote, vTN, trtn )
		{
			TapNote &tn = *trtn->pTN;
			int iEndRow = iStartRow + tn.iDuration;

			//LOG->Trace(ssprintf("trying for min between iSongRow (%i) and iEndRow (%i) (duration %i)",iSongRow,iEndRow,tn.iDuration));
			trtn->pTN->HoldResult.iLastHeldRow = min( iSongRow, iEndRow );
		}
	}

	// If the song beat is in the range of this hold:
	if( iStartRow <= iSongRow  &&  iStartRow <= iMaxEndRow && bHeadJudged )
	{
		switch( subType )
		{
		case TapNoteSubType_Hold:
			FOREACH( TrackRowTapNote, vTN, trtn )
			{
				TapNote &tn = *trtn->pTN;

				// set hold flag so NoteField can do intelligent drawing
				tn.HoldResult.bHeld = bIsHoldingButton && bInitiatedNote;
				tn.HoldResult.bActive = bInitiatedNote;
			}

			if( bInitiatedNote && bIsHoldingButton )
			{
				//LOG->Trace("bInitiatedNote && bIsHoldingButton; Increasing hold life to MAX_HOLD_LIFE");
				// Increase life
				fLife = MAX_HOLD_LIFE; // was 1 -aj
			}
			else
			{
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

				TimingWindow window = m_bTickHolds ? TW_Checkpoint : TW_Hold;
				//LOG->Trace("fLife before minus: %f",fLife);
				fLife -= fDeltaTime / GetWindowSeconds(window);
				//LOG->Trace("fLife before clamp: %f",fLife);
				fLife = max(0, fLife);
				//LOG->Trace("fLife after: %f",fLife);
			}
			break;
		case TapNoteSubType_Roll:
			FOREACH( TrackRowTapNote, vTN, trtn )
			{
				TapNote &tn = *trtn->pTN;
				tn.HoldResult.bHeld = true;
				tn.HoldResult.bActive = bInitiatedNote;
			}

			// give positive life in Step(), not here.

			// Decrease life
			fLife -= fDeltaTime/GetWindowSeconds(TW_Roll);
			fLife = max( fLife, 0 );	// clamp
			break;
		/*
		case TapNoteSubType_Mine:
			break;
		*/
		default:
			FAIL_M(ssprintf("Invalid tap note subtype: %i", subType));
		}
	}

	// TODO: Cap the active time passed to the score keeper to the actual start time and end time of the hold.
	if( vTN[0].pTN->HoldResult.bActive ) 
	{
		float fSecondsActiveSinceLastUpdate = fDeltaTime * GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		if( m_pPrimaryScoreKeeper )
			m_pPrimaryScoreKeeper->HandleHoldActiveSeconds( fSecondsActiveSinceLastUpdate );
		if( m_pSecondaryScoreKeeper )
			m_pSecondaryScoreKeeper->HandleHoldActiveSeconds( fSecondsActiveSinceLastUpdate );
	}

	// check for LetGo. If the head was missed completely, don't count an LetGo.
	/* Why? If you never step on the head, then it will be left as HNS_None,
	 * which doesn't seem correct. */
	if( IMMEDIATE_HOLD_LET_GO )
	{
		if( bInitiatedNote && fLife == 0 && bHeadJudged )	// the player has not pressed the button for a long time!
		{
			//LOG->Trace("LetGo from life == 0 (did initiate hold)");
			hns = HNS_LetGo;
		}
	}

	// score hold notes that have passed
	if( iSongRow >= iMaxEndRow && bHeadJudged )
	{
		bool bLetGoOfHoldNote = false;

		/* Score rolls that end with fLife == 0 as LetGo, even if
		 * m_bTickHolds is on. Rolls don't have iCheckpointsMissed set, so,
		 * unless we check Life == 0, rolls would always be scored as Held. */
		bool bAllowHoldCheckpoints;
		switch( subType )
		{
		DEFAULT_FAIL( subType );
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

		if( m_bTickHolds  &&  bAllowHoldCheckpoints )
		{
			//LOG->Trace("(hold checkpoints are allowed and enabled.)");
			int iCheckpointsHit = 0;
			int iCheckpointsMissed = 0;
			FOREACH( TrackRowTapNote, vTN, v )
			{
				iCheckpointsHit += v->pTN->HoldResult.iCheckpointsHit;
				iCheckpointsMissed += v->pTN->HoldResult.iCheckpointsMissed;
			}
			bLetGoOfHoldNote = iCheckpointsMissed > 0 || iCheckpointsHit == 0;

			// TRICKY: If the hold is so short that it has no checkpoints,
			// then mark it as Held if the head was stepped on.
			if( iCheckpointsHit == 0  &&  iCheckpointsMissed == 0 )
				bLetGoOfHoldNote = !bSteppedOnHead;

			/*
			if(bLetGoOfHoldNote)
				LOG->Trace("let go of hold note, life is 0");
			else
				LOG->Trace("did not let go of hold note :D");
			*/
		}
		else
		{
			//LOG->Trace("(hold checkpoints disabled.)");
			bLetGoOfHoldNote = fLife == 0;
			/*
			if(bLetGoOfHoldNote)
				LOG->Trace("let go of hold note, life is 0");
			else
				LOG->Trace("did not let go of hold note :D");
			*/
		}

		if( bInitiatedNote )
		{
			if(!bLetGoOfHoldNote)
			{
				//LOG->Trace("initiated note and didn't let go");
				fLife = 1; // xxx: should be MAX_HOLD_LIFE instead? -aj
				hns = HNS_Held;
				bool bBright = m_pPlayerStageStats && m_pPlayerStageStats->m_iCurCombo>(unsigned int)BRIGHT_GHOST_COMBO_THRESHOLD;
				if( m_pNoteField )
				{
					FOREACH( TrackRowTapNote, vTN, trtn )
					{
						int iTrack = trtn->iTrack;
						m_pNoteField->DidHoldNote( iTrack, HNS_Held, bBright );	// bright ghost flash
					}
				}
			}

			else
			{
				//LOG->Trace("initiated note and let go :(");
			}
		}
		else if( SCORE_MISSED_HOLDS_AND_ROLLS )
		{
			hns = HNS_LetGo;
		}
		else 
		{
			hns = HNS_Missed;
		}
	}

	float fLifeFraction = fLife / MAX_HOLD_LIFE;

	FOREACH( TrackRowTapNote, vTN, trtn )
	{
		TapNote &tn = *trtn->pTN;
		tn.HoldResult.fLife = fLife;
		tn.HoldResult.hns = hns;
		// Stop the playing keysound for the hold note.
		// I think this causes crashes too. -aj
		// This can still crash. I think it expects a full game and quit before the preference works:
		// otherwise, it causes problems on holds. At least, that hapened on my Mac. -wolfman2000

		static Preference<float> *pVolume = Preference<float>::GetPreferenceByName("SoundVolume");
		if (pVolume != NULL)
		{
			static float fVol = pVolume->Get();

			if( tn.iKeysoundIndex >= 0 && tn.iKeysoundIndex < static_cast<int>(m_vKeysounds.size()) )
			{
				float factor = (tn.subType == TapNoteSubType_Roll ? 2.0f * fLifeFraction : 10.0f * fLifeFraction - 8.5f);
				m_vKeysounds[tn.iKeysoundIndex].SetProperty ("Volume", max(0.0f, min(1.0f, factor)) * fVol);
			}
		}
	}

	if ( (hns == HNS_LetGo) && COMBO_BREAK_ON_IMMEDIATE_HOLD_LET_GO )
		IncrementMissCombo();
	
	if( hns != HNS_None )
	{
		//LOG->Trace("tap note scoring time.");
		TapNote &tn = *vTN[0].pTN;
		SetHoldJudgment( tn, iFirstTrackWithMaxEndRow );
		HandleHoldScore( tn );
		//LOG->Trace("hold result = %s",StringConversion::ToString(tn.HoldResult.hns).c_str());
	}
	//LOG->Trace("[Player::UpdateHoldNotes] ends");
}

void Player::DrawPrimitives()
{
	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	// May have both players in doubles (for battle play); only draw primary player.
	if( GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->m_StyleType == StyleType_OnePlayerTwoSides  &&
		pn != GAMESTATE->GetMasterPlayerNumber() )
		return;

	bool draw_notefield= m_pNoteField && !IsOniDead();

	const PlayerOptions& curr_options= m_pPlayerState->m_PlayerOptions.GetCurrent();
	float tilt= curr_options.m_fPerspectiveTilt;
	float skew= curr_options.m_fSkew;
	float mini= curr_options.m_fEffects[PlayerOptions::EFFECT_MINI];
	float center_y= GetY() + (GRAY_ARROWS_Y_STANDARD + GRAY_ARROWS_Y_REVERSE) / 2;
	bool reverse= curr_options.GetReversePercentForColumn(0) > .5;

	if(m_drawing_notefield_board)
	{
		// Ask the Notefield to draw its board primitive before everything else
		// so that things drawn under the field aren't behind the opaque board.
		// -Kyz
		if(draw_notefield)
		{
			PlayerNoteFieldPositioner poser(this, GetX(), tilt, skew, mini, center_y, reverse);
			m_pNoteField->DrawBoardPrimitive();
		}
		return;
	}

	// Draw these below everything else.
	if( COMBO_UNDER_FIELD && curr_options.m_fBlind == 0 )
	{
		if( m_sprCombo )
			m_sprCombo->Draw();
	}

	if( TAP_JUDGMENTS_UNDER_FIELD )
		DrawTapJudgments();

	if( HOLD_JUDGMENTS_UNDER_FIELD )
		DrawHoldJudgments();

	if(draw_notefield)
	{
		PlayerNoteFieldPositioner poser(this, GetX(), tilt, skew, mini, center_y, reverse);
		m_pNoteField->Draw();
	}

	// m_pNoteField->m_sprBoard->GetVisible()
	if( !COMBO_UNDER_FIELD && curr_options.m_fBlind == 0 )
		if( m_sprCombo )
			m_sprCombo->Draw();

	if( !(bool)TAP_JUDGMENTS_UNDER_FIELD )
		DrawTapJudgments();

	if( !(bool)HOLD_JUDGMENTS_UNDER_FIELD )
		DrawHoldJudgments();
}

void Player::PushPlayerMatrix(float x, float skew, float center_y)
{
	DISPLAY->CameraPushMatrix();
	DISPLAY->PushMatrix();
	DISPLAY->LoadMenuPerspective(45, SCREEN_WIDTH, SCREEN_HEIGHT,
		SCALE(skew, 0.1f, 1.0f, x, SCREEN_CENTER_X), center_y);
}

void Player::PopPlayerMatrix()
{
	DISPLAY->CameraPopMatrix();
	DISPLAY->PopMatrix();
}

void Player::DrawNoteFieldBoard()
{
	m_drawing_notefield_board= true;
	Draw();
	m_drawing_notefield_board= false;
}

Player::PlayerNoteFieldPositioner::PlayerNoteFieldPositioner(
	Player* p, float x, float tilt, float skew, float mini, float center_y, bool reverse)
	:player(p)
{
	player->PushPlayerMatrix(x, skew, center_y);
	int reverse_mult= (reverse ? -1 : 1);
	original_y= player->m_pNoteField->GetY();
	float tilt_degrees= SCALE(tilt, -1.f, +1.f, +30, -30) * reverse_mult;
	float zoom= SCALE(mini, 0.f, 1.f, 1.f, .5f);
	// Something strange going on here.  Notice that the range for tilt's
	// effect on y_offset goes to -45 when positive, but -20 when negative.
	// I don't know why it's done this why, simply preserving old behavior.
	// -Kyz
	if(tilt > 0)
	{
		zoom*= SCALE(tilt, 0.f, 1.f, 1.f, 0.9f);
		y_offset= SCALE(tilt, 0.f, 1.f, 0.f, -45.f) * reverse_mult;
	}
	else
	{
		zoom*= SCALE(tilt, 0.f, -1.f, 1.f, 0.9f);
		y_offset= SCALE(tilt, 0.f, -1.f, 0.f, -20.f) * reverse_mult;
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

void Player::DrawTapJudgments()
{
	if( m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind > 0 )
		return;

	if( m_sprJudgment )
		m_sprJudgment->Draw();
}

void Player::DrawHoldJudgments()
{
	if( m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind > 0 )
		return;

	for( int c=0; c<m_NoteData.GetNumTracks(); c++ )
		if( m_vpHoldJudgment[c] )
			m_vpHoldJudgment[c]->Draw();
}


void Player::ChangeLife( TapNoteScore tns )
{
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
	if( m_pLifeMeter )
		m_pLifeMeter->ChangeLife( tns );

	ChangeLifeRecord();
}

void Player::ChangeLife( HoldNoteScore hns, TapNoteScore tns )
{
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
	if( m_pLifeMeter )
		m_pLifeMeter->ChangeLife( hns, tns );

	ChangeLifeRecord();
}

void Player::ChangeLife(float delta)
{
	// If ChangeLifeRecord is not called before the change, then the life graph
	// will show a gradual change from the time of the previous step (or
	// change) to the time of this change, instead of the sharp change that
	// actually occurred. -Kyz
	ChangeLifeRecord();
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
	if(m_pLifeMeter)
	{
		m_pLifeMeter->ChangeLife(delta);
	}
	ChangeLifeRecord();
}

void Player::SetLife(float value)
{
	// If ChangeLifeRecord is not called before the change, then the life graph
	// will show a gradual change from the time of the previous step (or
	// change) to the time of this change, instead of the sharp change that
	// actually occurred. -Kyz
	ChangeLifeRecord();
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
	if(m_pLifeMeter)
	{
		m_pLifeMeter->SetLife(value);
	}
	ChangeLifeRecord();
}

void Player::ChangeLifeRecord()
{
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
	float fLife = -1;
	if( m_pLifeMeter )
	{
		fLife = m_pLifeMeter->GetLife();
	}
	if( fLife != -1 )
		if( m_pPlayerStageStats )
			m_pPlayerStageStats->SetLifeRecordAt( fLife, STATSMAN->m_CurStageStats.m_fStepsSeconds );
}

// seemsgood
void Player::ChangeWifeRecord() {
	if (m_pPlayerStageStats)
		m_pPlayerStageStats->SetLifeRecordAt(curwifescore / maxwifescore, STATSMAN->m_CurStageStats.m_fStepsSeconds);
}

int Player::GetClosestNoteDirectional( int col, int iStartRow, int iEndRow, bool bAllowGraded, bool bForward ) const
{
	NoteData::const_iterator begin, end;
	m_NoteData.GetTapNoteRange( col, iStartRow, iEndRow, begin, end );

	if( !bForward )
		swap( begin, end );

	while( begin != end )
	{
		if( !bForward )
			--begin;

		// Is this the row we want?
		do {
			const TapNote &tn = begin->second;
			if (!m_Timing->IsJudgableAtRow( begin->first ))
				break;
			// unsure if autoKeysounds should be excluded. -Wolfman2000
			if( tn.type == TapNoteType_Empty || tn.type == TapNoteType_AutoKeysound )
				break;
			if( !bAllowGraded && tn.result.tns != TNS_None )
				break;

			return begin->first;
		} while(0);

		if( bForward )
			++begin;
	}

	return -1;
}

// Find the closest note to fBeat.
int Player::GetClosestNote( int col, int iNoteRow, int iMaxRowsAhead, int iMaxRowsBehind, bool bAllowGraded ) const
{
	// Start at iIndexStartLookingAt and search outward.
	int iNextIndex = GetClosestNoteDirectional( col, iNoteRow, iNoteRow+iMaxRowsAhead, bAllowGraded, true );
	int iPrevIndex = GetClosestNoteDirectional( col, iNoteRow-iMaxRowsBehind, iNoteRow, bAllowGraded, false );

	if( iNextIndex == -1 && iPrevIndex == -1 )
		return -1;
	if( iNextIndex == -1 )
		return iPrevIndex;
	if( iPrevIndex == -1 )
		return iNextIndex;

	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	// Get the current time, previous time, and next time.
	float fNoteTime = m_pPlayerState->m_Position.m_fMusicSeconds;
	float fNextTime = m_Timing->WhereUAtBro(iNextIndex);
	float fPrevTime = m_Timing->WhereUAtBro(iPrevIndex);

	/* Figure out which row is closer. */
	if( fabsf(fNoteTime-fNextTime) > fabsf(fNoteTime-fPrevTime) )
		return iPrevIndex;
	else
		return iNextIndex;
}

int Player::GetClosestNonEmptyRowDirectional( int iStartRow, int iEndRow, bool /* bAllowGraded */, bool bForward ) const
{
	if( bForward )
	{
		NoteData::all_tracks_iterator iter = m_NoteData.GetTapNoteRangeAllTracks( iStartRow, iEndRow );

		while( !iter.IsAtEnd() )
		{
			if( NoteDataWithScoring::IsRowCompletelyJudged(m_NoteData, iter.Row()) )
			{
				++iter;
				continue;
			}
			if (!m_Timing->IsJudgableAtRow(iter.Row()))
			{
				++iter;
				continue;
			}
			return iter.Row();
		}
	}
	else
	{
		NoteData::all_tracks_reverse_iterator iter = m_NoteData.GetTapNoteRangeAllTracksReverse( iStartRow, iEndRow );

		while( !iter.IsAtEnd() )
		{
			if( NoteDataWithScoring::IsRowCompletelyJudged(m_NoteData, iter.Row()) )
			{
				++iter;
				continue;
			}
			return iter.Row();
		}
	}

	return -1;
}

// Find the closest note to fBeat.
int Player::GetClosestNonEmptyRow( int iNoteRow, int iMaxRowsAhead, int iMaxRowsBehind, bool bAllowGraded ) const
{
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	// Start at iIndexStartLookingAt and search outward.
	int iNextRow = GetClosestNonEmptyRowDirectional( iNoteRow, iNoteRow+iMaxRowsAhead, bAllowGraded, true );
	int iPrevRow = GetClosestNonEmptyRowDirectional( iNoteRow-iMaxRowsBehind, iNoteRow, bAllowGraded, false );

	if( iNextRow == -1 && iPrevRow == -1 )
		return -1;
	if( iNextRow == -1 )
		return iPrevRow;
	if( iPrevRow == -1 )
		return iNextRow;

	// Get the current time, previous time, and next time.
	float fNoteTime = m_pPlayerState->m_Position.m_fMusicSeconds;
	float fNextTime = m_Timing->WhereUAtBro(iNextRow);
	float fPrevTime = m_Timing->WhereUAtBro(iPrevRow);

	/* Figure out which row is closer. */
	if( fabsf(fNoteTime-fNextTime) > fabsf(fNoteTime-fPrevTime) )
		return iPrevRow;
	else
		return iNextRow;
}

bool Player::IsOniDead() const
{
	// If we're playing on oni and we've died, do nothing.
	return m_pPlayerState->m_PlayerOptions.GetStage().m_LifeType == LifeType_Battery && m_pPlayerStageStats  && m_pPlayerStageStats->m_bFailed;
}

void Player::DoTapScoreNone()
{
	Message msg( "ScoreNone" );
	MESSAGEMAN->Broadcast( msg );

	const unsigned int iOldCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurCombo : 0;
	const unsigned int iOldMissCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	/* The only real way to tell if a mine has been scored is if it has disappeared
	* but this only works for hit mines so update the scores for avoided mines here. */
	if( m_pPrimaryScoreKeeper )
		m_pPrimaryScoreKeeper->HandleTapScoreNone();
	if( m_pSecondaryScoreKeeper )
		m_pSecondaryScoreKeeper->HandleTapScoreNone();

	SendComboMessages( iOldCombo, iOldMissCombo );

	if( m_pLifeMeter )
		m_pLifeMeter->HandleTapScoreNone();
	// TODO: Remove use of PlayerNumber
	PlayerNumber pn = PLAYER_INVALID;

	if( PENALIZE_TAP_SCORE_NONE )
	{
		SetJudgment( BeatToNoteRow( m_pPlayerState->m_Position.m_fSongBeat ), -1, TAP_EMPTY, TNS_Miss, 0 );
		// the ScoreKeeper will subtract points later.
	}
}

void Player::ScoreAllActiveHoldsLetGo()
{
	if( PENALIZE_TAP_SCORE_NONE )
	{
		const float fSongBeat = m_pPlayerState->m_Position.m_fSongBeat;
		const int iSongRow = BeatToNoteRow( fSongBeat );

		// Score all active holds to NotHeld
		for( int iTrack=0; iTrack<m_NoteData.GetNumTracks(); ++iTrack )
		{
			// Since this is being called every frame, let's not check the whole array every time.
			// Instead, only check 1 beat back.  Even 1 is overkill.
			const int iStartCheckingAt = max( 0, iSongRow-BeatToNoteRow(1) );
			NoteData::TrackMap::iterator begin, end;
			m_NoteData.GetTapNoteRangeInclusive( iTrack, iStartCheckingAt, iSongRow+1, begin, end );
			for( ; begin != end; ++begin )
			{
				TapNote &tn = begin->second;
				if( tn.HoldResult.bActive )
				{
					tn.HoldResult.hns = HNS_LetGo;
					tn.HoldResult.fLife = 0;

					SetHoldJudgment( tn, iTrack );
					HandleHoldScore( tn );
				}
			}
		}
	}
}

void Player::PlayKeysound( const TapNote &tn, TapNoteScore score )
{
	// tap note must have keysound
	if( tn.iKeysoundIndex >= 0 && tn.iKeysoundIndex < static_cast<int>(m_vKeysounds.size()) )
	{
		// handle a case for hold notes
		if( tn.type == TapNoteType_HoldHead )
		{
			// if the hold is not already held
			if( tn.HoldResult.hns == HNS_None )
			{
				// if the hold is already activated
				TapNoteScore tns = tn.result.tns;
				if( tns != TNS_None && tns != TNS_Miss && score == TNS_None )
				{
					// the sound must also be already playing
					if( m_vKeysounds[tn.iKeysoundIndex].IsPlaying() )
					{
						// if all of these conditions are met, don't play the sound.
						return;
					}
				}
			}
		}
		m_vKeysounds[tn.iKeysoundIndex].Play(false);
		static Preference<float> *pVolume = Preference<float>::GetPreferenceByName("SoundVolume");
		static float fVol = pVolume->Get();
		m_vKeysounds[tn.iKeysoundIndex].SetProperty ("Volume", fVol);
	}
}

void Player::Step( int col, int row, const std::chrono::steady_clock::time_point &tm, bool bHeld, bool bRelease, float padStickSeconds )
{
	if( IsOniDead() )
		return;

	// TODO: remove use of PlayerNumber
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	// Do everything that depends on a timer here;
	// set your breakpoints somewhere after this block.
	std::chrono::duration<float> stepDelta = std::chrono::steady_clock::now() - tm;
	float stepAgo = stepDelta.count() - padStickSeconds;
	
	const float fLastBeatUpdate = m_pPlayerState->m_Position.m_LastBeatUpdate.Ago();
	const float fPositionSeconds = m_pPlayerState->m_Position.m_fMusicSeconds - stepAgo;
	const float fTimeSinceStep = stepAgo;

	// idk if this is the correct value for input logs but we'll use them to test -mina
	// ok this is 100% not the place to do this
	//m_pPlayerStageStats->InputData.emplace_back(fPositionSeconds);

	float fSongBeat = m_pPlayerState->m_Position.m_fSongBeat;
	
	if( GAMESTATE->m_pCurSteps[m_pPlayerState->m_PlayerNumber] )
		fSongBeat = m_Timing->GetBeatFromElapsedTime( fPositionSeconds );

	const int iSongRow = row == -1 ? BeatToNoteRow( fSongBeat ) : row;

	if( col != -1 && !bRelease )
	{
		// Update roll life
		// Let's not check the whole array every time.
		// Instead, only check 1 beat back.  Even 1 is overkill.
		// Just update the life here and let Update judge the roll.
		const int iStartCheckingAt = max( 0, iSongRow-BeatToNoteRow(1) );
		NoteData::TrackMap::iterator begin, end;
		m_NoteData.GetTapNoteRangeInclusive( col, iStartCheckingAt, iSongRow+1, begin, end );
		for( ; begin != end; ++begin )
		{
			TapNote &tn = begin->second;
			if( tn.type != TapNoteType_HoldHead )
				continue;

			switch( tn.subType )
			{
			DEFAULT_FAIL( tn.subType );
			case TapNoteSubType_Hold:
				continue;
			case TapNoteSubType_Roll:
				break;
			}

			const int iRow = begin->first;

			HoldNoteScore hns = tn.HoldResult.hns;
			if( hns != HNS_None )	// if this HoldNote already has a result
				continue;	// we don't need to update the logic for this one

			// if they got a bad score or haven't stepped on the corresponding tap yet
			const TapNoteScore tns = tn.result.tns;
			bool bInitiatedNote = true;
			if( REQUIRE_STEP_ON_HOLD_HEADS )
				bInitiatedNote = tns != TNS_None  &&  tns != TNS_Miss;	// did they step on the start?
			const int iEndRow = iRow + tn.iDuration;

			if( bInitiatedNote && tn.HoldResult.fLife != 0 )
			{
				/* This hold note is not judged and we stepped on its head.  Update iLastHeldRow.
				 * Do this even if we're a little beyond the end of the hold note, to make sure
				 * iLastHeldRow is clamped to iEndRow if the hold note is held all the way. */
				//LOG->Trace("setting iLastHeldRow to min of iSongRow (%i) and iEndRow (%i)",iSongRow,iEndRow);
				tn.HoldResult.iLastHeldRow = min( iSongRow, iEndRow );
			}

			// If the song beat is in the range of this hold:
			if( iRow <= iSongRow && iRow <= iEndRow )
			{
				if( bInitiatedNote )
				{
					// Increase life
					tn.HoldResult.fLife = 1;

					if( ROLL_BODY_INCREMENTS_COMBO && m_pPlayerState->m_PlayerController != PC_AUTOPLAY )
					{
						IncrementCombo();
						
						bool bBright = m_pPlayerStageStats && m_pPlayerStageStats->m_iCurCombo>(unsigned int)BRIGHT_GHOST_COMBO_THRESHOLD;
						if( m_pNoteField )
							m_pNoteField->DidHoldNote( col, HNS_Held, bBright );
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
	int skipstart = nerv[10]; // this is not robust need to come up with something better later - Mina

	if (iSongRow < skipstart || iSongRow > static_cast<int>(nerv.size()) -10 ) {
		iStepSearchRows = max(BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(m_pPlayerState->m_Position.m_fMusicSeconds + StepSearchDistance)) - iSongRow,
			iSongRow - BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(m_pPlayerState->m_Position.m_fMusicSeconds - StepSearchDistance))) + ROWS_PER_BEAT;
	}
	else
	{
		/* Buncha bullshit that speeds up searching for the rows that we're concerned about judging taps within
		by avoiding the invocation of the incredibly slow getbeatfromelapsedtime. Needs to be cleaned up a lot,
		whole system does. Only in use if sequential assumption remains valid. - Mina */

		if (nerv[nervpos] < iSongRow && nervpos < nerv.size())
			nervpos += 1;

		size_t SearchIndexBehind = nervpos;
		size_t SearchIndexAhead = nervpos;
		float SearchBeginTime = m_Timing->WhereUAtBro(nerv[nervpos]);

		while (SearchIndexBehind > 1 && SearchBeginTime - m_Timing->WhereUAtBro(nerv[SearchIndexBehind - 1]) < StepSearchDistance)
			SearchIndexBehind -= 1;

		while (SearchIndexAhead > 1 && SearchIndexAhead + 1 > nerv.size() && m_Timing->WhereUAtBro(nerv[SearchIndexAhead + 1]) - SearchBeginTime < StepSearchDistance)
			SearchIndexAhead += 1;

		int	MaxLookBehind = nerv[nervpos] - nerv[SearchIndexBehind];
		int MaxLookAhead = nerv[SearchIndexAhead] - nerv[nervpos];

		if (nervpos > 0)
			iStepSearchRows = (max(MaxLookBehind, MaxLookAhead) + ROWS_PER_BEAT);
	}

	// calculate TapNoteScore
	TapNoteScore score = TNS_None;

	int iRowOfOverlappingNoteOrRow = row;
	if (row == -1)
		iRowOfOverlappingNoteOrRow = GetClosestNote(col, iSongRow, iStepSearchRows, iStepSearchRows, false);

	if( iRowOfOverlappingNoteOrRow != -1 )
	{
		// compute the score for this hit
		float fNoteOffset = 0.f;
		// we need this later if we are autosyncing
		const float fStepBeat = NoteRowToBeat( iRowOfOverlappingNoteOrRow );
		const float fStepSeconds = m_Timing->WhereUAtBro(fStepBeat);

		if( row == -1 )
		{
			// We actually stepped on the note this long ago:
			//fTimeSinceStep

			/* GAMESTATE->m_fMusicSeconds is the music time as of GAMESTATE->m_LastBeatUpdate. Figure
			 * out what the music time is as of now. */
			const float fCurrentMusicSeconds = m_pPlayerState->m_Position.m_fMusicSeconds + (fLastBeatUpdate*GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate);

			// ... which means it happened at this point in the music:
			const float fMusicSeconds = fCurrentMusicSeconds - fTimeSinceStep * GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

			// The offset from the actual step in seconds:
			fNoteOffset = (fStepSeconds - fMusicSeconds) / GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;	// account for music rate
			/*
			LOG->Trace("step was %.3f ago, music is off by %f: %f vs %f, step was %f off", 
				fTimeSinceStep, GAMESTATE->m_LastBeatUpdate.Ago()/GAMESTATE->m_SongOptions.m_fMusicRate,
				fStepSeconds, fMusicSeconds, fNoteOffset );
			*/
		}

		const float fSecondsFromExact = fabsf( fNoteOffset );

		TapNote tnDummy = TAP_ORIGINAL_TAP;
		TapNote *pTN = NULL;
		NoteData::iterator iter = m_NoteData.FindTapNote( col, iRowOfOverlappingNoteOrRow );
		DEBUG_ASSERT( iter!= m_NoteData.end(col) );
		pTN = &iter->second;

		switch( m_pPlayerState->m_PlayerController )
		{
		case PC_HUMAN:
			switch( pTN->type )
			{
			case TapNoteType_Mine:
				// Stepped too close to mine?
				if( !bRelease && ( REQUIRE_STEP_ON_MINES == !bHeld ) && 
				   fSecondsFromExact <= GetWindowSeconds(TW_Mine) &&
				   m_Timing->IsJudgableAtRow(iSongRow))
					score = TNS_HitMine;   
				break;
			case TapNoteType_HoldHead:
				// oh wow, this was causing the trigger before the hold heads
				// bug. (It was fNoteOffset > 0.f before) -DaisuMaster
				if( !REQUIRE_STEP_ON_HOLD_HEADS && ( fNoteOffset <= GetWindowSeconds( TW_W5 ) && GetWindowSeconds( TW_W5 ) != 0 ) )
				{
					score = TNS_W1;
					break;
				}
				// Fall through to default.
			default:
				if( (pTN->type == TapNoteType_Lift) == bRelease )
				{
					if(	 fSecondsFromExact <= GetWindowSeconds(TW_W1) )	score = TNS_W1;
					else if( fSecondsFromExact <= GetWindowSeconds(TW_W2) )	score = TNS_W2;
					else if( fSecondsFromExact <= GetWindowSeconds(TW_W3) )	score = TNS_W3;
					else if( fSecondsFromExact <= GetWindowSeconds(TW_W4) )	score = TNS_W4;
					else if( fSecondsFromExact <= max(GetWindowSeconds(TW_W5), 0.18f) )	score = TNS_W5;
				}
				break;
			}
			break;

		case PC_CPU:
		case PC_AUTOPLAY:
			score = PlayerAI::GetTapNoteScore( m_pPlayerState );

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

			// TRICKY:  We're asking the AI to judge mines. Consider TNS_W4 and
			// below as "mine was hit" and everything else as "mine was avoided"
			if( pTN->type == TapNoteType_Mine )
			{
				// The CPU hits a lot of mines. Only consider hitting the
				// first mine for a row. We know we're the first mine if 
				// there are are no mines to the left of us.
				for( int t=0; t<col; t++ )
				{
					if( m_NoteData.GetTapNote(t,iRowOfOverlappingNoteOrRow).type == TapNoteType_Mine )	// there's a mine to the left of us
						return;	// avoid
				}

				// The CPU hits a lot of mines. Make it less likely to hit 
				// mines that don't have a tap note on the same row.
				bool bTapsOnRow = m_NoteData.IsThereATapOrHoldHeadAtRow( iRowOfOverlappingNoteOrRow );
				TapNoteScore get_to_avoid = bTapsOnRow ? TNS_W3 : TNS_W4;

				if( score >= get_to_avoid )
					return;	// avoided
				else
					score = TNS_HitMine;
			}

			if( score > TNS_W4 )
				score = TNS_W2; // sentinel

			/* AI will generate misses here. Don't handle a miss like a regular
			 * note because we want the judgment animation to appear delayed.
			 * Instead, return early if AI generated a miss, and let
			 * UpdateTapNotesMissedOlderThan() detect and handle the misses. */
			if( score == TNS_Miss )
				return;

			// Put some small, random amount in fNoteOffset so that demonstration 
			// show a mix of late and early. - Chris (StepMania r15628)
			//fNoteOffset = randomf( -0.1f, 0.1f );
			// Since themes may use the offset in a visual graph, the above
			// behavior is not the best thing to do. Instead, random numbers
			// should be generated based on the TapNoteScore, so that they can
			// logically match up with the current timing windows. -aj
			{
				float fWindowW1 = GetWindowSeconds(TW_W1);
				float fWindowW2 = GetWindowSeconds(TW_W2);
				float fWindowW3 = GetWindowSeconds(TW_W3);
				float fWindowW4 = GetWindowSeconds(TW_W4);
				float fWindowW5 = GetWindowSeconds(TW_W5);

				// W1 is the top judgment, there is no overlap.
				if( score == TNS_W1 )
					fNoteOffset = randomf(-fWindowW1, fWindowW1);
				else
				{
					// figure out overlap.
					float fLowerBound = 0.0f; // negative upper limit
					float fUpperBound = 0.0f; // positive lower limit
					float fCompareWindow = 0.0f; // filled in here:
					if( score == TNS_W2 )
					{
						fLowerBound = -fWindowW1;
						fUpperBound = fWindowW1;
						fCompareWindow = fWindowW2;
					}
					else if( score == TNS_W3 )
					{
						fLowerBound = -fWindowW2;
						fUpperBound = fWindowW2;
						fCompareWindow = fWindowW3;
					}
					else if( score == TNS_W4 )
					{
						fLowerBound = -fWindowW3;
						fUpperBound = fWindowW3;
						fCompareWindow = fWindowW4;
					}
					else if( score == TNS_W5 )
					{
						fLowerBound = -fWindowW4;
						fUpperBound = fWindowW4;
						fCompareWindow = fWindowW5;
					}
					float f1 = randomf(-fCompareWindow, fLowerBound);
					float f2 = randomf(fUpperBound, fCompareWindow);

					if(randomf() * 100 >= 50)
						fNoteOffset = f1;
					else
						fNoteOffset = f2;
				}
			}

			break;

		/*
		case PC_REPLAY:
			// based on where we are, see what grade to get.
			score = PlayerAI::GetTapNoteScore( m_pPlayerState );
			// row is the current row, col is current column (track)
			fNoteOffset = TapNoteOffset attribute
			break;
		*/
		default:
			FAIL_M(ssprintf("Invalid player controller type: %i", m_pPlayerState->m_PlayerController));
		}

		if( m_pPlayerState->m_PlayerController == PC_HUMAN && score >= TNS_W3 ) 
			AdjustSync::HandleAutosync( fNoteOffset, fStepSeconds );

		// Do game-specific and mode-specific score mapping.
		score = GAMESTATE->GetCurrentGame()->MapTapNoteScore( score );
		if( score == TNS_W1 && !GAMESTATE->ShowW1() )
			score = TNS_W2;


		if( score != TNS_None )
		{
			pTN->result.tns = score;
			pTN->result.fTapNoteOffset = -fNoteOffset;
		}

		m_LastTapNoteScore = score;
		if( GAMESTATE->CountNotesSeparately() )
		{
			if( pTN->type != TapNoteType_Mine )
			{
				const bool bBlind = (m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind != 0);
				// XXX: This is the wrong combo for shared players.
				// STATSMAN->m_CurStageStats.m_Player[pn] might work, but could be wrong.
				const bool bBright = ( m_pPlayerStageStats && m_pPlayerStageStats->m_iCurCombo > (unsigned int)BRIGHT_GHOST_COMBO_THRESHOLD ) || bBlind;
				if( m_pNoteField )
					m_pNoteField->DidTapNote( col, bBlind? TNS_W1:score, bBright );
				if( score >= m_pPlayerState->m_PlayerOptions.GetCurrent().m_MinTNSToHideNotes || bBlind )
					HideNote( col, iRowOfOverlappingNoteOrRow );

				if ( pTN->result.tns != TNS_None )
				{
					SetJudgment(iRowOfOverlappingNoteOrRow, col, *pTN);
					HandleTapRowScore(iRowOfOverlappingNoteOrRow);
				}
			}
		}
		else if( NoteDataWithScoring::IsRowCompletelyJudged(m_NoteData, iRowOfOverlappingNoteOrRow) )
		{
			FlashGhostRow( iRowOfOverlappingNoteOrRow );
		}
	}

	if( score == TNS_None )
		DoTapScoreNone();

	if( !bRelease )
	{
		/* Search for keyed sounds separately.  Play the nearest note. */
		/* XXX: This isn't quite right. As per the above XXX for iRowOfOverlappingNote, if iRowOfOverlappingNote
		 * is set to a previous note, the keysound could have changed and this would cause the wrong one to play,
		 * in essence playing two sounds in the opposite order. Maybe this should always perform the search. Still,
		 * even that doesn't seem quite right since it would then play the same (new) keysound twice which would
		 * sound wrong even though the notes were judged as being correct, above. Fixing the above problem would
		 * fix this one as well. */
		int iHeadRow;
		if( iRowOfOverlappingNoteOrRow != -1 && score != TNS_None )
		{
			// just pressing a note, use that row.
			// in other words, iRowOfOverlappingNoteOrRow = iRowOfOverlappingNoteOrRow
		}
		else if ( m_NoteData.IsHoldNoteAtRow( col, iSongRow, &iHeadRow ) )
		{
			// stepping on a hold, use it!
			iRowOfOverlappingNoteOrRow = iHeadRow;
		}
		else
		{
			// or else find the closest note.
			iRowOfOverlappingNoteOrRow = GetClosestNote( col, iSongRow, MAX_NOTE_ROW, MAX_NOTE_ROW, true );
		}
		if( iRowOfOverlappingNoteOrRow != -1 )
		{
			const TapNote &tn = m_NoteData.GetTapNote( col, iRowOfOverlappingNoteOrRow );
			PlayKeysound( tn, score );
		}
	}
	// XXX:
	if( !bRelease )
	{
		if( m_pNoteField )
		{
			m_pNoteField->Step( col, score );
		}
		Message msg( "Step" );
		msg.SetParam( "PlayerNumber", m_pPlayerState->m_PlayerNumber );
		msg.SetParam( "MultiPlayer", m_pPlayerState->m_mp );
		msg.SetParam( "Column", col );
		MESSAGEMAN->Broadcast( msg );
		// Backwards compatibility
		Message msg2( ssprintf("StepP%d", m_pPlayerState->m_PlayerNumber + 1) );
		MESSAGEMAN->Broadcast( msg2 );
	}
}

void Player::UpdateTapNotesMissedOlderThan( float fMissIfOlderThanSeconds )
{
	//LOG->Trace( "Steps::UpdateTapNotesMissedOlderThan(%f)", fMissIfOlderThanThisBeat );
	int iMissIfOlderThanThisRow;
	const float fEarliestTime = m_pPlayerState->m_Position.m_fMusicSeconds - fMissIfOlderThanSeconds;
	{
		TimingData::GetBeatArgs beat_info;
		beat_info.elapsed_time= fEarliestTime;
		m_Timing->GetBeatAndBPSFromElapsedTime(beat_info);

		iMissIfOlderThanThisRow = BeatToNoteRow(beat_info.beat);
		if(beat_info.freeze_out || beat_info.delay_out )
		{
			/* If there is a freeze on iMissIfOlderThanThisIndex, include this index too.
			 * Otherwise we won't show misses for tap notes on freezes until the
			 * freeze finishes. */
			if(!beat_info.delay_out)
				iMissIfOlderThanThisRow++;
		}
	}

	NoteData::all_tracks_iterator &iter = *m_pIterNeedsTapJudging;

	for( ; !iter.IsAtEnd() && iter.Row() < iMissIfOlderThanThisRow; ++iter )
	{
		TapNote &tn = *iter;

		if( !NeedsTapJudging(tn) )
			continue;

		// Ignore all notes in WarpSegments or FakeSegments.
		if (!m_Timing->IsJudgableAtRow(iter.Row()))
			continue;

		if( tn.type == TapNoteType_Mine )
		{
			tn.result.tns = TNS_AvoidMine;
			/* The only real way to tell if a mine has been scored is if it has disappeared
			 * but this only works for hit mines so update the scores for avoided mines here. */
			if( m_pPrimaryScoreKeeper )
				m_pPrimaryScoreKeeper->HandleTapScore( tn );
			if( m_pSecondaryScoreKeeper )
				m_pSecondaryScoreKeeper->HandleTapScore( tn );
		}
		else
		{
			tn.result.tns = TNS_Miss;
			if ( GAMESTATE->CountNotesSeparately() )
			{
				SetJudgment(iter.Row(), m_NoteData.GetFirstTrackWithTapOrHoldHead(iter.Row()), tn);
				HandleTapRowScore(iter.Row());
			}
		}
	}
}

void Player::UpdateJudgedRows(float fDeltaTime)
{
	// Look into the future only as far as we need to
	const int iEndRow = BeatToNoteRow( m_Timing->GetBeatFromElapsedTime( m_pPlayerState->m_Position.m_fMusicSeconds + GetMaxStepDistanceSeconds() ) );
	bool bAllJudged = true;

	if( !GAMESTATE->CountNotesSeparately() )
	{
		NoteData::all_tracks_iterator iter = *m_pIterUnjudgedRows;
		int iLastSeenRow = -1;
		for( ; !iter.IsAtEnd()  &&  iter.Row() <= iEndRow; ++iter )
		{
			int iRow = iter.Row();

			// Do not judge arrows in WarpSegments or FakeSegments
			if (!m_Timing->IsJudgableAtRow(iRow))
				continue;

			if( iLastSeenRow != iRow )
			{
				iLastSeenRow = iRow;

				// crossed a nonempty row
				if( !NoteDataWithScoring::IsRowCompletelyJudged(m_NoteData, iRow) )
				{
					bAllJudged = false;
					continue;
				}
				if( bAllJudged )
					*m_pIterUnjudgedRows = iter;
				if( m_pJudgedRows->JudgeRow(iRow) )
					continue;

				const TapNote &lastTN = NoteDataWithScoring::LastTapNoteWithResult( m_NoteData, iRow );

				if(lastTN.result.tns < TNS_Miss )
					continue;
				
				SetJudgment( iRow, m_NoteData.GetFirstTrackWithTapOrHoldHead(iRow), lastTN );
				HandleTapRowScore(iRow);
			}
		}
	}

	// handle mines.
	{
		bAllJudged = true;
		set<RageSound *> setSounds;
		NoteData::all_tracks_iterator iter = *m_pIterUnjudgedMineRows;	// copy
		int iLastSeenRow = -1;
		for( ; !iter.IsAtEnd()  &&  iter.Row() <= iEndRow; ++iter )
		{
			int iRow = iter.Row();

			// Do not worry about mines in WarpSegments or FakeSegments
			if (!m_Timing->IsJudgableAtRow(iRow))
				continue;

			TapNote &tn = *iter;

			if( iRow != iLastSeenRow )
			{
				iLastSeenRow = iRow;
				if( bAllJudged )
					*m_pIterUnjudgedMineRows = iter;
			}

			bool bMineNotHidden = tn.type == TapNoteType_Mine && !tn.result.bHidden;
			if( !bMineNotHidden )
				continue;

			switch( tn.result.tns )
			{
			DEFAULT_FAIL( tn.result.tns );
			case TNS_None:
				bAllJudged = false;
				continue;
			case TNS_AvoidMine:
				SetMineJudgment( tn.result.tns , iter.Track() );
				tn.result.bHidden= true;
				continue;
			case TNS_HitMine:
				SetMineJudgment( tn.result.tns , iter.Track() );
				break;
			}
			if( m_pNoteField )
				m_pNoteField->DidTapNote( iter.Track(), tn.result.tns, false );

			if( tn.iKeysoundIndex >= 0 && tn.iKeysoundIndex < static_cast<int>(m_vKeysounds.size()) )
				setSounds.insert( &m_vKeysounds[tn.iKeysoundIndex] );
			else if( g_bEnableMineSoundPlayback )
				setSounds.insert( &m_soundMine );

			ChangeLife( tn.result.tns );

			if( m_pScoreDisplay )
				m_pScoreDisplay->OnJudgment( tn.result.tns );
			if( m_pSecondaryScoreDisplay )
				m_pSecondaryScoreDisplay->OnJudgment( tn.result.tns );

			// Make sure hit mines affect the dance points.
			if( m_pPrimaryScoreKeeper )
				m_pPrimaryScoreKeeper->HandleTapScore( tn );
			if( m_pSecondaryScoreKeeper )
				m_pSecondaryScoreKeeper->HandleTapScore( tn );
			tn.result.bHidden = true;
		}
		// If we hit the end of the loop, m_pIterUnjudgedMineRows needs to be
		// updated. -Kyz
		if((iter.IsAtEnd() || iLastSeenRow == iEndRow) && bAllJudged)
		{
			*m_pIterUnjudgedMineRows= iter;
		}

		FOREACHS( RageSound *, setSounds, s )
		{
			// Only play one copy of each mine sound at a time per player.
			(*s)->Stop();
			(*s)->Play(false);
		}
	}
}

void Player::FlashGhostRow( int iRow )
{
	TapNoteScore lastTNS = NoteDataWithScoring::LastTapNoteWithResult( m_NoteData, iRow ).result.tns;
	const bool bBlind = (m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind != 0);
	const bool bBright = ( m_pPlayerStageStats && m_pPlayerStageStats->m_iCurCombo > (unsigned int)BRIGHT_GHOST_COMBO_THRESHOLD ) || bBlind;

	for( int iTrack = 0; iTrack < m_NoteData.GetNumTracks(); ++iTrack )
	{
		const TapNote &tn = m_NoteData.GetTapNote( iTrack, iRow );

		if(tn.type == TapNoteType_Empty || tn.type == TapNoteType_Mine ||
			tn.type == TapNoteType_Fake || tn.result.bHidden)
		{
			continue;
		}
		if( m_pNoteField )
		{
			m_pNoteField->DidTapNote( iTrack, lastTNS, bBright );
		}
		if( lastTNS >= m_pPlayerState->m_PlayerOptions.GetCurrent().m_MinTNSToHideNotes || bBlind )
		{
			HideNote( iTrack, iRow );
		}
	}
}

void Player::CrossedRows( int iLastRowCrossed, const std::chrono::steady_clock::time_point &now )
{
	//LOG->Trace( "Player::CrossedRows   %d    %d", iFirstRowCrossed, iLastRowCrossed );

	NoteData::all_tracks_iterator &iter = *m_pIterUncrossedRows;
	int iLastSeenRow = -1;
	for( ; !iter.IsAtEnd()  &&  iter.Row() <= iLastRowCrossed; ++iter )
	{
		// Apply InitialHoldLife.
		TapNote &tn = *iter;
		int iRow = iter.Row();
		int iTrack = iter.Track();
		switch( tn.type )
		{
			case TapNoteType_HoldHead:
			{
				tn.HoldResult.fLife = INITIAL_HOLD_LIFE;
				if( !REQUIRE_STEP_ON_HOLD_HEADS )
				{
					PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
					vector<GameInput> GameI;
					GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->StyleInputToGameInput( iTrack, pn, GameI );
					if( PREFSMAN->m_fPadStickSeconds > 0.f )
					{
						for(size_t i= 0; i < GameI.size(); ++i)
						{
							float fSecsHeld = INPUTMAPPER->GetSecsHeld(GameI[i], m_pPlayerState->m_mp);
							if(fSecsHeld >= PREFSMAN->m_fPadStickSeconds)
							{
								Step(iTrack, -1, now, true, false, PREFSMAN->m_fPadStickSeconds);
							}
						}
					}
					else
					{
						if(INPUTMAPPER->IsBeingPressed(GameI, m_pPlayerState->m_mp))
						{
							Step(iTrack, -1, now, true, false);
						}
					}
				}
				break;
			}
			case TapNoteType_Mine:
			{
				// Hold the panel while crossing a mine will cause the mine to explode
				// TODO: Remove use of PlayerNumber.
				PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
				vector<GameInput> GameI;
				GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)->StyleInputToGameInput( iTrack, pn, GameI );
				if( PREFSMAN->m_fPadStickSeconds > 0.0f )
				{
					for(size_t i= 0; i < GameI.size(); ++i)
					{
						float fSecsHeld = INPUTMAPPER->GetSecsHeld(GameI[i], m_pPlayerState->m_mp);
						if(fSecsHeld >= PREFSMAN->m_fPadStickSeconds)
						{
							Step( iTrack, -1, now, true, false, PREFSMAN->m_fPadStickSeconds );
						}
					}
				}
				else if(INPUTMAPPER->IsBeingPressed(GameI, m_pPlayerState->m_mp))
				{
					Step( iTrack, iRow, now, true, false );
				}
				break;
			}
			default: break;
		}

		// check to see if there's a note at the crossed row
		if( m_pPlayerState->m_PlayerController != PC_HUMAN )
		{
			if (tn.type != TapNoteType_Empty &&
				tn.type != TapNoteType_Fake &&
				tn.type != TapNoteType_AutoKeysound &&
				tn.result.tns == TNS_None &&
				this->m_Timing->IsJudgableAtRow(iRow) )
			{
				Step( iTrack, iRow, now, false, false );
				if( m_pPlayerState->m_PlayerController == PC_AUTOPLAY )
				{
					if( m_pPlayerStageStats )
						m_pPlayerStageStats->m_bDisqualified = true;
				}
			}
		}

		// TODO: Can we remove the iLastSeenRow logic and the
		// autokeysound for loop, since the iterator in this loop will
		// already be iterating over all of the tracks?
		if (iRow != iLastSeenRow)
		{
			// crossed a new not-empty row
			iLastSeenRow = iRow;

			for (int t = 0; t < m_NoteData.GetNumTracks(); ++t)
			{
				const TapNote &tap = m_NoteData.GetTapNote(t, iRow);
				if (tap.type == TapNoteType_AutoKeysound)
				{
					PlayKeysound(tap, TNS_None);
				}
			}
		}
	}


	/* Update hold checkpoints
	 *
	 * TODO: Move this to a separate function. */
	if( m_bTickHolds && m_pPlayerState->m_PlayerController != PC_AUTOPLAY )
	{
		// Few rows typically cross per update. Easier to check all crossed rows
		// than to calculate from timing segments.
		for( int r = m_iFirstUncrossedRow; r <= iLastRowCrossed; ++r )
		{
			int tickCurrent = m_Timing->GetTickcountAtRow( r );

			// There is a tick count at this row
			if( tickCurrent > 0 && r % ( ROWS_PER_BEAT / tickCurrent ) == 0 )
			{

				vector<int> viColsWithHold;
				int iNumHoldsHeldThisRow = 0;
				int iNumHoldsMissedThisRow = 0;

				// start at r-1 so that we consider holds whose end rows are equal to the checkpoint row
				NoteData::all_tracks_iterator nIter = m_NoteData.GetTapNoteRangeAllTracks( r-1, r, true );
				for( ; !nIter.IsAtEnd(); ++nIter )
				{
					TapNote &tn = *nIter;
					if( tn.type != TapNoteType_HoldHead )
						continue;

					int iTrack = nIter.Track();
					viColsWithHold.push_back( iTrack );

					if( tn.HoldResult.fLife > 0 )
					{
						++iNumHoldsHeldThisRow;
						++tn.HoldResult.iCheckpointsHit;
					}
					else
					{
						++iNumHoldsMissedThisRow;
						++tn.HoldResult.iCheckpointsMissed;
					}
				}
				GAMESTATE->SetProcessedTimingData(this->m_Timing);

				// TODO: Find a better way of handling hold checkpoints with other taps.
				if( !viColsWithHold.empty() && ( CHECKPOINTS_TAPS_SEPARATE_JUDGMENT || m_NoteData.GetNumTapNotesInRow( r ) == 0 ) )
				{
					HandleHoldCheckpoint(r,
								 iNumHoldsHeldThisRow,
								 iNumHoldsMissedThisRow,
								 viColsWithHold );
				}
			}
		}
	}

	m_iFirstUncrossedRow = iLastRowCrossed+1;
}

void Player::HandleTapRowScore( unsigned row )
{
	bool bNoCheating = true;
#ifdef DEBUG
	bNoCheating = false;
#endif

	if( GAMESTATE->m_bDemonstrationOrJukebox )
		bNoCheating = false;
	// don't accumulate points if AutoPlay is on.
	if( bNoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY )
		return;

	TapNoteScore scoreOfLastTap = NoteDataWithScoring::LastTapNoteWithResult(m_NoteData, row).result.tns;
	const unsigned int iOldCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurCombo : 0;
	const unsigned int iOldMissCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	if( scoreOfLastTap == TNS_Miss )
		m_LastTapNoteScore = TNS_Miss;

	for( int track = 0; track < m_NoteData.GetNumTracks(); ++track )
	{
		const TapNote &tn = m_NoteData.GetTapNote( track, row );
		// Mines cannot be handled here.
		if (tn.type == TapNoteType_Empty ||
			tn.type == TapNoteType_Fake ||
			tn.type == TapNoteType_Mine ||
			tn.type == TapNoteType_AutoKeysound)
			continue;
		if( m_pPrimaryScoreKeeper )
			m_pPrimaryScoreKeeper->HandleTapScore( tn );
		if( m_pSecondaryScoreKeeper )
			m_pSecondaryScoreKeeper->HandleTapScore( tn );
	}

	if( m_pPrimaryScoreKeeper != NULL )
		m_pPrimaryScoreKeeper->HandleTapRowScore( m_NoteData, row );
	if( m_pSecondaryScoreKeeper != NULL )
		m_pSecondaryScoreKeeper->HandleTapRowScore( m_NoteData, row );

	const unsigned int iCurCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurCombo : 0;
	const unsigned int iCurMissCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	SendComboMessages( iOldCombo, iOldMissCombo );

	if( m_pPlayerStageStats )
	{
		SetCombo( iCurCombo, iCurMissCombo );
	}

#define CROSSED( x ) (iOldCombo<x && iCurCombo>=x)
	if ( CROSSED(100) )
		SCREENMAN->PostMessageToTopScreen( SM_100Combo, 0 );
	else if( CROSSED(200) )
		SCREENMAN->PostMessageToTopScreen( SM_200Combo, 0 );
	else if( CROSSED(300) )
		SCREENMAN->PostMessageToTopScreen( SM_300Combo, 0 );
	else if( CROSSED(400) )
		SCREENMAN->PostMessageToTopScreen( SM_400Combo, 0 );
	else if( CROSSED(500) )
		SCREENMAN->PostMessageToTopScreen( SM_500Combo, 0 );
	else if( CROSSED(600) )
		SCREENMAN->PostMessageToTopScreen( SM_600Combo, 0 );
	else if( CROSSED(700) )
		SCREENMAN->PostMessageToTopScreen( SM_700Combo, 0 );
	else if( CROSSED(800) )
		SCREENMAN->PostMessageToTopScreen( SM_800Combo, 0 );
	else if( CROSSED(900) )
		SCREENMAN->PostMessageToTopScreen( SM_900Combo, 0 );
	else if( CROSSED(1000))
		SCREENMAN->PostMessageToTopScreen( SM_1000Combo, 0 );
	else if( (iOldCombo / 100) < (iCurCombo / 100) && iCurCombo > 1000 )
		SCREENMAN->PostMessageToTopScreen( SM_ComboContinuing, 0 );
#undef CROSSED

	// new max combo
	if( m_pPlayerStageStats )
		m_pPlayerStageStats->m_iMaxCombo = max(m_pPlayerStageStats->m_iMaxCombo, iCurCombo);

	/* Use the real current beat, not the beat we've been passed. That's because
	 * we want to record the current life/combo to the current time; eg. if it's
	 * a MISS, the beat we're registering is in the past, but the life is changing
	 * now. We need to include time from previous songs in a course, so we
	 * can't use GAMESTATE->m_fMusicSeconds. Use fStepsSeconds instead. */
	if( m_pPlayerStageStats )
		m_pPlayerStageStats->UpdateComboList( STATSMAN->m_CurStageStats.m_fStepsSeconds, false );

	if( m_pScoreDisplay )
	{
		if( m_pPlayerStageStats )
			m_pScoreDisplay->SetScore( m_pPlayerStageStats->m_iScore );
		m_pScoreDisplay->OnJudgment( scoreOfLastTap );
	}
	if( m_pSecondaryScoreDisplay )
	{
		if( m_pPlayerStageStats )
			m_pSecondaryScoreDisplay->SetScore( m_pPlayerStageStats->m_iScore );
		m_pSecondaryScoreDisplay->OnJudgment( scoreOfLastTap );
	}

	ChangeLife( scoreOfLastTap );
}

void Player::HandleHoldCheckpoint(int iRow, 
				  int iNumHoldsHeldThisRow, 
				  int iNumHoldsMissedThisRow, 
				  const vector<int> &viColsWithHold )
{
	bool bNoCheating = true;
#ifdef DEBUG
	bNoCheating = false;
#endif

	// WarpSegments and FakeSegments aren't judged in any way.
	if (!m_Timing->IsJudgableAtRow(iRow))
		return;

	// don't accumulate combo if AutoPlay is on.
	if( bNoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY )
		return;

	const unsigned int iOldCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurCombo : 0;
	const unsigned int iOldMissCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	if( m_pPrimaryScoreKeeper )
		m_pPrimaryScoreKeeper->HandleHoldCheckpointScore(m_NoteData, 
								 iRow, 
								 iNumHoldsHeldThisRow, 
								 iNumHoldsMissedThisRow );
	if( m_pSecondaryScoreKeeper )
		m_pSecondaryScoreKeeper->HandleHoldCheckpointScore(m_NoteData, 
								   iRow, 
								   iNumHoldsHeldThisRow, 
								   iNumHoldsMissedThisRow );

	if( iNumHoldsMissedThisRow == 0 )
	{
		// added for http://ssc.ajworld.net/sm-ssc/bugtracker/view.php?id=16 -aj
		if( CHECKPOINTS_FLASH_ON_HOLD )
		{
			FOREACH_CONST( int, viColsWithHold, i )
			{
				bool bBright = m_pPlayerStageStats 
					&& m_pPlayerStageStats->m_iCurCombo>(unsigned int)BRIGHT_GHOST_COMBO_THRESHOLD;
				if( m_pNoteField )
					m_pNoteField->DidHoldNote( *i, HNS_Held, bBright );
			}
		}
	}

	SendComboMessages( iOldCombo, iOldMissCombo );

	if( m_pPlayerStageStats )
	{
		SetCombo( m_pPlayerStageStats->m_iCurCombo, m_pPlayerStageStats->m_iCurMissCombo );
		m_pPlayerStageStats->UpdateComboList( STATSMAN->m_CurStageStats.m_fStepsSeconds, false );
	}

	ChangeLife( iNumHoldsMissedThisRow == 0? TNS_CheckpointHit:TNS_CheckpointMiss );

	SetJudgment( iRow, viColsWithHold[0], TAP_EMPTY, iNumHoldsMissedThisRow == 0? TNS_CheckpointHit:TNS_CheckpointMiss, 0 );
}

void Player::HandleHoldScore( const TapNote &tn )
{
	HoldNoteScore holdScore = tn.HoldResult.hns;
	TapNoteScore tapScore = tn.result.tns;
	bool bNoCheating = true;
#ifdef DEBUG
	bNoCheating = false;
#endif

	if( GAMESTATE->m_bDemonstrationOrJukebox )
		bNoCheating = false;
	// don't accumulate points if AutoPlay is on.
	if( bNoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY )
		return;

	if( m_pPrimaryScoreKeeper )
		m_pPrimaryScoreKeeper->HandleHoldScore( tn );
	if( m_pSecondaryScoreKeeper )
		m_pSecondaryScoreKeeper->HandleHoldScore( tn );

	if( m_pScoreDisplay )
	{
		if( m_pPlayerStageStats ) 
			m_pScoreDisplay->SetScore( m_pPlayerStageStats->m_iScore );
		m_pScoreDisplay->OnJudgment( holdScore, tapScore );
	}
	if( m_pSecondaryScoreDisplay )
	{
		if( m_pPlayerStageStats ) 
			m_pSecondaryScoreDisplay->SetScore( m_pPlayerStageStats->m_iScore );
		m_pSecondaryScoreDisplay->OnJudgment( holdScore, tapScore );
	}

	ChangeLife( holdScore, tapScore );
}

float Player::GetMaxStepDistanceSeconds()
{
	float fMax = 0;
	fMax = max( fMax, GetWindowSeconds(TW_W5) );
	fMax = max( fMax, GetWindowSeconds(TW_W4) );
	fMax = max( fMax, GetWindowSeconds(TW_W3) );
	fMax = max( fMax, GetWindowSeconds(TW_W2) );
	fMax = max( fMax, GetWindowSeconds(TW_W1) );
	float f = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate * fMax;
	return f + m_fMaxInputLatencySeconds;
}

void Player::FadeToFail()
{
	if( m_pNoteField )
		m_pNoteField->FadeToFail();

	// clear miss combo
	SetCombo( 0, 0 );
}

void Player::CacheAllUsedNoteSkins()
{
	if( m_pNoteField )
		m_pNoteField->CacheAllUsedNoteSkins();
}

/* Reworked the judgment messages. Header file states that -1 should be sent as the offset for 
misses. This was not the case and 0s were being sent. Now it just sends nothing so params.Judgment 
~= nil can be used to filter messages with and without offsets. Also now there's a params.Judgment
that just gives the judgment for taps holds and mines in aggregate for things that need to be done
with any judgment. Params.Type is used to diffrentiate between those attributes for things that are
done differently between the types. Current values for taps/holds are sent in params.Val. Like it 
all should have been to begin with. Not sure where checkpoints are but I also don't care, so. 

Update: both message types are being sent out currently for compatability. -Mina*/
//#define autoplayISHUMAN
void Player::SetMineJudgment( TapNoteScore tns , int iTrack )
{
	if( m_bSendJudgmentAndComboMessages )
	{
		Message msg("Judgment");
		msg.SetParam( "Player", m_pPlayerState->m_PlayerNumber );
		msg.SetParam( "TapNoteScore", tns );
		msg.SetParam( "FirstTrack", iTrack );
		msg.SetParam( "Judgment", tns);
		msg.SetParam( "Type", RString("Mine"));

		// Ms scoring implemenation - Mina
		if (tns == TNS_HitMine)
			curwifescore -= 8.f;

		if (m_pPlayerStageStats) {
			if(maxwifescore == 0.f)
				msg.SetParam("WifePercent", 0);
			else
				msg.SetParam("WifePercent", 100 * curwifescore / maxwifescore);
			
			msg.SetParam("CurWifeScore", curwifescore);
			msg.SetParam("MaxWifeScore", maxwifescore);
			msg.SetParam("WifeDifferential", curwifescore - maxwifescore * m_pPlayerState->playertargetgoal);
			msg.SetParam("TotalPercent", 100 * curwifescore / totalwifescore);
			if (wifescorepersonalbest != m_pPlayerState->playertargetgoal) {
				msg.SetParam("WifePBDifferential", curwifescore - maxwifescore * wifescorepersonalbest);
				msg.SetParam("WifePBGoal", wifescorepersonalbest);
			}
#ifdef autoplayISHUMAN
			ChangeWifeRecord();
			m_pPlayerStageStats->m_fWifeScore = curwifescore / totalwifescore;
			
#else
			if (m_pPlayerState->m_PlayerController == PC_HUMAN) {
				ChangeWifeRecord();
				m_pPlayerStageStats->m_fWifeScore = curwifescore / totalwifescore;
				m_pPlayerStageStats->CurWifeScore = curwifescore;
				m_pPlayerStageStats->MaxWifeScore = maxwifescore;
			}
			else {
				curwifescore -= 6666666.f;	// sail hatan
			}
#endif

		}

		MESSAGEMAN->Broadcast( msg );
		if( m_pPlayerStageStats &&
			( ( tns == TNS_AvoidMine && AVOID_MINE_INCREMENTS_COMBO ) || 
				( tns == TNS_HitMine && MINE_HIT_INCREMENTS_MISS_COMBO ))
		)
		{
			SetCombo( m_pPlayerStageStats->m_iCurCombo, m_pPlayerStageStats->m_iCurMissCombo );
		}
	}
}

void Player::SetJudgment( int iRow, int iTrack, const TapNote &tn, TapNoteScore tns, float fTapNoteOffset )
{
	if( m_bSendJudgmentAndComboMessages )
	{
		Message msg("Judgment");
		msg.SetParam( "Player", m_pPlayerState->m_PlayerNumber );
		msg.SetParam( "MultiPlayer", m_pPlayerState->m_mp );
		msg.SetParam( "FirstTrack", iTrack );
		msg.SetParam( "TapNoteScore", tns );
		msg.SetParam( "Early", fTapNoteOffset < 0.0f );
		msg.SetParam( "Judgment", tns);
		msg.SetParam( "NoteRow", iRow);
		msg.SetParam( "Type", RString("Tap"));
		msg.SetParam( "TapNoteOffset", tn.result.fTapNoteOffset );
		if ( m_pPlayerStageStats )
			msg.SetParam("Val", m_pPlayerStageStats->m_iTapNoteScores[tns] + 1);

		if (tns != TNS_Miss)
			msg.SetParam("Offset", tn.result.fTapNoteOffset * 1000);  // don't send out ms offsets for misses, multiply by 1000 for convenience - Mina

		if (m_pPlayerStageStats) {
			if (tns == TNS_Miss)
				curwifescore -= 8;
			else
				curwifescore += wife2(tn.result.fTapNoteOffset, m_fTimingWindowScale);
			maxwifescore += 2;
			
			msg.SetParam("WifePercent", 100 * curwifescore / maxwifescore);
			msg.SetParam("CurWifeScore", curwifescore);
			msg.SetParam("MaxWifeScore", maxwifescore);
			msg.SetParam("WifeDifferential", curwifescore - maxwifescore * m_pPlayerState->playertargetgoal);
			msg.SetParam("TotalPercent", 100 * curwifescore / totalwifescore);
			if (wifescorepersonalbest != m_pPlayerState->playertargetgoal) {
				msg.SetParam("WifePBDifferential", curwifescore - maxwifescore * wifescorepersonalbest);
				msg.SetParam("WifePBGoal", wifescorepersonalbest);
			}
#ifdef autoplayISHUMAN
			m_pPlayerStageStats->m_fWifeScore = curwifescore / totalwifescore;
			m_pPlayerStageStats->m_vOffsetVector.emplace_back(tn.result.fTapNoteOffset);
			m_pPlayerStageStats->m_vNoteRowVector.emplace_back(iRow);
			ChangeWifeRecord();
#else
			if (m_pPlayerState->m_PlayerController == PC_HUMAN) {
				m_pPlayerStageStats->m_fWifeScore = curwifescore / totalwifescore;
				m_pPlayerStageStats->CurWifeScore = curwifescore;
				m_pPlayerStageStats->MaxWifeScore = maxwifescore;
				m_pPlayerStageStats->m_vOffsetVector.emplace_back(tn.result.fTapNoteOffset);
				m_pPlayerStageStats->timeStamps.emplace_back(m_Timing->WhereUAtBroNoOffset(iRow));
				m_pPlayerStageStats->m_vNoteRowVector.emplace_back(iRow);
			}
			else {
				curwifescore -= 666.f;	// hail satan
			}

#endif
		}

		Lua* L= LUA->Get();
		lua_createtable( L, 0, m_NoteData.GetNumTracks() ); // TapNotes this row
		lua_createtable( L, 0, m_NoteData.GetNumTracks() ); // HoldHeads of tracks held at this row.

		for( int iTrack = 0; iTrack < m_NoteData.GetNumTracks(); ++iTrack )
		{
			NoteData::iterator tn = m_NoteData.FindTapNote(iTrack, iRow);
			if( tn != m_NoteData.end(iTrack) )
			{
				tn->second.PushSelf(L);
				lua_rawseti(L, -3, iTrack + 1);
			}
			else
			{
				int iHeadRow;
				if( m_NoteData.IsHoldNoteAtRow( iTrack, iRow, &iHeadRow ) )
				{
					NoteData::iterator hold = m_NoteData.FindTapNote(iTrack, iHeadRow);
					hold->second.PushSelf(L);
					lua_rawseti(L, -2, iTrack + 1);
				}
			}
		}
		msg.SetParamFromStack( L, "Holds" );
		msg.SetParamFromStack( L, "Notes" );

		LUA->Release( L );
		MESSAGEMAN->Broadcast( msg );
	}
}

void Player::SetHoldJudgment( TapNote &tn, int iTrack )
{
	ASSERT( iTrack < static_cast<int>(m_vpHoldJudgment.size()) );
	if( m_vpHoldJudgment[iTrack] )
		m_vpHoldJudgment[iTrack]->SetHoldJudgment( tn.HoldResult.hns );

	if (m_bSendJudgmentAndComboMessages)
	{
		Message msg("Judgment");
		msg.SetParam("Player", m_pPlayerState->m_PlayerNumber);
		msg.SetParam("MultiPlayer", m_pPlayerState->m_mp);
		msg.SetParam("FirstTrack", iTrack);
		msg.SetParam("NumTracks", static_cast<int>(m_vpHoldJudgment.size()));
		msg.SetParam("TapNoteScore", tn.result.tns);
		msg.SetParam("HoldNoteScore", tn.HoldResult.hns);
		msg.SetParam("Judgment", tn.HoldResult.hns);
		msg.SetParam("Type", RString("Hold"));
		if ( m_pPlayerStageStats) {
			msg.SetParam("Val", m_pPlayerStageStats->m_iHoldNoteScores[tn.HoldResult.hns] + 1);

			// Ms scoring implemenation - Mina
			if (tn.HoldResult.hns == HNS_LetGo || tn.HoldResult.hns == HNS_Missed)
				curwifescore -= 6.f;

			msg.SetParam("WifePercent", 100 * curwifescore / maxwifescore);
			msg.SetParam("CurWifeScore", curwifescore);
			msg.SetParam("MaxWifeScore", maxwifescore);
			msg.SetParam("WifeDifferential", curwifescore - maxwifescore *  m_pPlayerState->playertargetgoal);
			msg.SetParam("TotalPercent", 100 * curwifescore / totalwifescore);
			if (wifescorepersonalbest != m_pPlayerState->playertargetgoal) {
				msg.SetParam("WifePBDifferential", curwifescore - maxwifescore * wifescorepersonalbest);
				msg.SetParam("WifePBGoal", wifescorepersonalbest);
			}
#ifdef autoplayISHUMAN
			m_pPlayerStageStats->m_fWifeScore = curwifescore / totalwifescore;
			ChangeWifeRecord();
#else
			if (m_pPlayerState->m_PlayerController == PC_HUMAN) {
				m_pPlayerStageStats->m_fWifeScore = curwifescore / totalwifescore;
				m_pPlayerStageStats->CurWifeScore = curwifescore;
				m_pPlayerStageStats->MaxWifeScore = maxwifescore;
			}
		 		
#endif
		}
			
		Lua* L = LUA->Get();
		tn.PushSelf(L);
		msg.SetParamFromStack( L, "TapNote" );
		LUA->Release( L );

		MESSAGEMAN->Broadcast( msg );
	}
}

void Player::SetCombo( unsigned int iCombo, unsigned int iMisses )
{
	if( !m_bSeenComboYet )	// first update, don't set bIsMilestone=true
	{
		m_bSeenComboYet = true;
		m_iLastSeenCombo = iCombo;
	}
	
	bool b25Milestone = false;
	bool b50Milestone = false;
	bool b100Milestone = false;
	bool b250Milestone = false;
	bool b1000Milestone = false;

#define MILESTONE_CHECK(amount) ((iCombo / amount) > (m_iLastSeenCombo / amount))
	if(m_iLastSeenCombo < 600)
	{
		b25Milestone= MILESTONE_CHECK(25);
		b50Milestone= MILESTONE_CHECK(50);
		b100Milestone= MILESTONE_CHECK(100);
		b250Milestone= MILESTONE_CHECK(250);
		b1000Milestone= MILESTONE_CHECK(1000);
	}
	else
	{
		b1000Milestone= MILESTONE_CHECK(1000);
	}
#undef MILESTONE_CHECK

	m_iLastSeenCombo = iCombo;

	if( b25Milestone )
		this->PlayCommand( "TwentyFiveMilestone");
	if( b50Milestone )
		this->PlayCommand( "FiftyMilestone");
	if( b100Milestone )
		this->PlayCommand( "HundredMilestone" );
	if( b250Milestone )
		this->PlayCommand( "TwoHundredFiftyMilestone");
	if( b1000Milestone )
		this->PlayCommand( "ThousandMilestone" );

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

	bPastBeginning = m_pPlayerState->m_Position.m_fMusicSeconds > GAMESTATE->m_pCurSong->m_fMusicLengthSeconds * PERCENT_UNTIL_COLOR_COMBO;

	if( m_bSendJudgmentAndComboMessages )
	{
		Message msg("Combo");
		if( iCombo )
			msg.SetParam( "Combo", iCombo );
		if( iMisses )
			msg.SetParam( "Misses", iMisses );
		if( bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W1) )
			msg.SetParam( "FullComboW1", true );
		if( bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W2) )
			msg.SetParam( "FullComboW2", true );
		if( bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W3) )
			msg.SetParam( "FullComboW3", true );
		if( bPastBeginning && m_pPlayerStageStats->FullComboOfScore(TNS_W4) )
			msg.SetParam( "FullComboW4", true );
		this->HandleMessage( msg );
	}
}

void Player::IncrementComboOrMissCombo(bool bComboOrMissCombo)
{
		const unsigned int iOldCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurCombo : 0;
		const unsigned int iOldMissCombo = m_pPlayerStageStats ? m_pPlayerStageStats->m_iCurMissCombo : 0;

		if( m_pPlayerStageStats )
		{
			if( bComboOrMissCombo )
			{
				m_pPlayerStageStats->m_iCurCombo++;
				m_pPlayerStageStats->m_iCurMissCombo = 0;
			}
			else
			{
				m_pPlayerStageStats->m_iCurCombo = 0;
				m_pPlayerStageStats->m_iCurMissCombo++;
			}
			SetCombo( m_pPlayerStageStats->m_iCurCombo, m_pPlayerStageStats->m_iCurMissCombo );
		}

		SendComboMessages( iOldCombo, iOldMissCombo );
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the Player. */ 
class LunaPlayer: public Luna<Player>
{
public:
	static int SetLife(T* p, lua_State* L)
	{
		if(p->m_inside_lua_set_life)
		{
			luaL_error(L, "Do not call SetLife from inside LifeChangedMessageCommand because SetLife causes a LifeChangedMessageCommand.");
		}
		p->m_inside_lua_set_life= true;
		p->SetLife(FArg(1));
		p->m_inside_lua_set_life= false;
		COMMON_RETURN_SELF;
	}
	static int ChangeLife(T* p, lua_State* L)
	{
		if(p->m_inside_lua_set_life)
		{
			luaL_error(L, "Do not call ChangeLife from inside LifeChangedMessageCommand because ChangeLife causes a LifeChangedMessageCommand.");
		}
		p->m_inside_lua_set_life= true;
		p->ChangeLife(FArg(1));
		p->m_inside_lua_set_life= false;
		COMMON_RETURN_SELF;
	}
	static int SetActorWithJudgmentPosition( T* p, lua_State *L )
	{ 
		Actor *pActor = Luna<Actor>::check(L, 1); 
		p->SetActorWithJudgmentPosition(pActor); 
		COMMON_RETURN_SELF;
	}
	static int SetActorWithComboPosition( T* p, lua_State *L )
	{ 
		Actor *pActor = Luna<Actor>::check(L, 1); 
		p->SetActorWithComboPosition(pActor); 
		COMMON_RETURN_SELF;
	}
	static int GetPlayerTimingData( T* p, lua_State *L )
	{
		p->GetPlayerTimingData().PushSelf(L);
		return 1;
	}
	
	LunaPlayer()
	{
		ADD_METHOD(SetLife);
		ADD_METHOD(ChangeLife);
		ADD_METHOD( SetActorWithJudgmentPosition );
		ADD_METHOD( SetActorWithComboPosition );
		ADD_METHOD( GetPlayerTimingData );
	}
};

LUA_REGISTER_DERIVED_CLASS( Player, ActorFrame )
// lua end

/*
 * (c) 2001-2006 Chris Danford, Steve Checkoway
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
