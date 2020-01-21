#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/PlayerAI.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "ArrowEffects.h"
#include "NoteField.h"
#include "Etterna/Models/Misc/AdjustSync.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/NoteData/NoteDataWithScoring.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "RageUtil/Utils/RageUtil.h"
#include "PlayerPractice.h"

PlayerPractice::PlayerPractice(NoteData& nd, bool bVisibleParts)
  : Player(nd, bVisibleParts)
{
	// eh
}

PlayerPractice::~PlayerPractice()
{
	// dont have to do anything here
}

void
PlayerPractice::Init(const std::string& sType,
					 PlayerState* pPlayerState,
					 PlayerStageStats* pPlayerStageStats,
					 LifeMeter* pLM,
					 ScoreKeeper* pPrimaryScoreKeeper)
{
	Player::Init(
	  sType, pPlayerState, pPlayerStageStats, pLM, pPrimaryScoreKeeper);
	if (m_pPlayerStageStats)
		m_pPlayerStageStats->m_bDisqualified = true;
}

void
PlayerPractice::Update(float fDeltaTime)
{
	const auto now = std::chrono::steady_clock::now();
	if (!m_bLoaded || GAMESTATE->m_pCurSong == NULL)
		return;

	ActorFrame::Update(fDeltaTime);

	const float fSongBeat = m_pPlayerState->m_Position.m_fSongBeat;
	const int iSongRow = BeatToNoteRow(fSongBeat);

	ArrowEffects::SetCurrentOptions(
	  &m_pPlayerState->m_PlayerOptions.GetCurrent());

	UpdateVisibleParts();

	// Sure, why not?
	if (GAMESTATE->GetPaused())
		return;

	// Tell the NoteField we pressed (or didnt press) certain columns
	UpdatePressedFlags();

	// Don't judge anything if we aren't counting stats
	// But also don't not judge anything if we are in Autoplay.
	// I don't know why you would use Autoplay in Practice, but yeah
	if (!countStats && m_pPlayerState->m_PlayerController == PC_HUMAN)
		return;

	// Tell Rolls to update (if in Autoplay)
	// Tell Holds to update (lose life)
	UpdateHoldsAndRolls(fDeltaTime, now);

	// A lot of logic... basically everything not listed here
	UpdateCrossedRows(now);

	// Check for completely judged rows.
	UpdateJudgedRows(fDeltaTime);

	UpdateTapNotesMissedOlderThan(GetMaxStepDistanceSeconds());
}

void
PlayerPractice::Step(int col,
					 int row,
					 const std::chrono::steady_clock::time_point& tm,
					 bool bHeld,
					 bool bRelease,
					 float padStickSeconds)
{
	if (GamePreferences::m_AutoPlay != PC_HUMAN)
		countStats = true;

	if (!countStats) {
		// big brained override to toggle this boolean and do everything else
		// normally
		countStats = true;

		// reset all iterators to this point so we can ignore the things we dont
		// care about
		const float fSongTime = GAMESTATE->m_Position.m_fMusicSeconds;
		const float fNotesBeatAdjusted =
		  GAMESTATE->m_pCurSteps->GetTimingData()->GetBeatFromElapsedTime(
			fSongTime - GetMaxStepDistanceSeconds());
		const float fNotesBeat =
		  GAMESTATE->m_pCurSteps->GetTimingData()->GetBeatFromElapsedTime(
			fSongTime);
		const int rowNowAdjusted = BeatToNoteRow(fNotesBeatAdjusted);
		const int rowNow = BeatToNoteRow(fNotesBeat);

		SAFE_DELETE(m_pIterNeedsTapJudging);
		m_pIterNeedsTapJudging = new NoteData::all_tracks_iterator(
		  m_NoteData.GetTapNoteRangeAllTracks(rowNowAdjusted, MAX_NOTE_ROW));

		SAFE_DELETE(m_pIterNeedsHoldJudging);
		m_pIterNeedsHoldJudging = new NoteData::all_tracks_iterator(
		  m_NoteData.GetTapNoteRangeAllTracks(rowNowAdjusted, MAX_NOTE_ROW));

		SAFE_DELETE(m_pIterUncrossedRows);
		m_pIterUncrossedRows = new NoteData::all_tracks_iterator(
		  m_NoteData.GetTapNoteRangeAllTracks(rowNow, MAX_NOTE_ROW));

		SAFE_DELETE(m_pIterUnjudgedRows);
		m_pIterUnjudgedRows = new NoteData::all_tracks_iterator(
		  m_NoteData.GetTapNoteRangeAllTracks(rowNowAdjusted, MAX_NOTE_ROW));

		SAFE_DELETE(m_pIterUnjudgedMineRows);
		m_pIterUnjudgedMineRows = new NoteData::all_tracks_iterator(
		  m_NoteData.GetTapNoteRangeAllTracks(rowNow, MAX_NOTE_ROW));
	}

	// then just do the normal stuff
	Player::Step(col, row, tm, bHeld, bRelease, padStickSeconds);
}

void
PlayerPractice::PositionReset()
{
	// Reset stage stats and stuff
	countStats = false;

	// wife points
	curwifescore = 0;
	maxwifescore = 0;

	// combo color
	m_bSeenComboYet = false;
	m_iLastSeenCombo = 0;
	m_pPlayerStageStats->m_bPlayerCanAchieveFullCombo = true;

	// judgments
	FOREACH_ENUM(TapNoteScore, tns)
	m_pPlayerStageStats->m_iTapNoteScores[tns] = 0;
	FOREACH_ENUM(TapNoteScore, hns)
	m_pPlayerStageStats->m_iHoldNoteScores[hns] = 0;

	// stage stats general info and replay data
	m_pPlayerStageStats->m_fWifeScore = 0;
	m_pPlayerStageStats->CurWifeScore = 0;
	m_pPlayerStageStats->MaxWifeScore = 0;
	m_pPlayerStageStats->UnloadReplayData();
	m_pPlayerStageStats->m_iCurCombo = 0;
	m_pPlayerStageStats->m_iMaxCombo = 0;
	m_pPlayerStageStats->m_iCurMissCombo = 0;
	m_pPlayerStageStats->m_radarActual.Zero();

	// combo graph, life graph
	m_pPlayerStageStats->m_ComboList.clear();
	m_pPlayerStageStats->m_ComboList.shrink_to_fit();
	m_pPlayerStageStats->m_fLifeRecord.clear();

	// misc judge info
	m_iFirstUncrossedRow = -1;
	m_pJudgedRows->Reset(-1);
	for (int i = 0;
		 i < GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
			   ->m_iColsPerPlayer;
		 ++i) {
		lastHoldHeadsSeconds[i] = -1000.f;
	}
}
