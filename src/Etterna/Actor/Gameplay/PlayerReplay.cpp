#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "ArrowEffects.h"
#include "NoteField.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/NoteData/NoteDataWithScoring.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "RageUtil/Utils/RageUtil.h"
#include "PlayerReplay.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Models/HighScore/Replay.h"
#include "Etterna/Singletons/ReplayManager.h"

#include <algorithm>

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

PlayerReplay::PlayerReplay(NoteData& nd, bool bVisibleParts)
  : Player(nd, bVisibleParts)
{
	// eh
}

PlayerReplay::~PlayerReplay()
{
	// dont have to do anything here
}

void
PlayerReplay::Init(const std::string& sType,
				   PlayerState* pPlayerState,
				   PlayerStageStats* pPlayerStageStats,
				   LifeMeter* pLM,
				   ScoreKeeper* pPrimaryScoreKeeper)
{
	Player::Init(
	  sType, pPlayerState, pPlayerStageStats, pLM, pPrimaryScoreKeeper);
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
	if (m_pPlayerStageStats)
		m_pPlayerStageStats->m_bDisqualified = true;
}

void
PlayerReplay::Load()
{
	Player::Load();

	const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const auto iSongRow = BeatToNoteRow(fSongBeat);
	// this replay will always be real or a dummy replay
	auto replay = REPLAYS->InitReplayPlaybackForScore(
	  REPLAYS->GetActiveReplayScore(), GetTimingWindowScale());
	SetPlaybackEvents(replay->GeneratePlaybackEvents(iSongRow));
	SetDroppedHolds(replay->GenerateDroppedHoldColumnsToRowsMap(iSongRow));
}

void
PlayerReplay::Reload()
{
	Player::Reload();

	const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const auto iSongRow = BeatToNoteRow(fSongBeat);
	// this replay will always be real or a dummy replay
	auto replay = REPLAYS->InitReplayPlaybackForScore(
	  REPLAYS->GetActiveReplayScore(), GetTimingWindowScale());
	SetPlaybackEvents(replay->GeneratePlaybackEvents(iSongRow));
	SetDroppedHolds(replay->GenerateDroppedHoldColumnsToRowsMap(iSongRow));
}

void
PlayerReplay::UpdateLoadedReplay(int startRow)
{
	auto replay = REPLAYS->GetActiveReplay();
	SetPlaybackEvents(replay->GeneratePlaybackEvents(startRow));
	SetDroppedHolds(replay->GenerateDroppedHoldColumnsToRowsMap(startRow));
}

void
PlayerReplay::UpdateHoldNotes(int iSongRow,
							  float fDeltaTime,
							  std::vector<TrackRowTapNote>& vTN)
{
	Player::UpdateHoldNotes(iSongRow, fDeltaTime, vTN);
	ASSERT(!vTN.empty());

	const auto iStartRow = vTN[0].iRow;
	auto iMaxEndRow = INT_MIN;
	auto iFirstTrackWithMaxEndRow = -1;

	auto subType = TapNoteSubType_Invalid;
	for (auto& trtn : vTN) {
		const auto iTrack = trtn.iTrack;
		ASSERT(iStartRow == trtn.iRow);
		const auto iEndRow = iStartRow + trtn.pTN->iDuration;
		if (subType == TapNoteSubType_Invalid)
			subType = trtn.pTN->subType;

		if (iEndRow > iMaxEndRow) {
			iMaxEndRow = iEndRow;
			iFirstTrackWithMaxEndRow = iTrack;
		}
	}

	ASSERT(iFirstTrackWithMaxEndRow != -1);

	// drop holds which should be dropped
	for (auto& trtn : vTN) {

		// find dropped holds in this column
		if (droppedHolds.count(trtn.iTrack) != 0) {

			// start no earlier than the top of the current hold
			// stop when the current row is reached or end of drops is reached
			// this loop behavior implies you CAN drop a single hold more than once.
			// that would be incorrect behavior because you cant score a note twice
			// but ... account for impossible scenarios anyways
			for (auto it = droppedHolds.at(trtn.iTrack).lower_bound(trtn.iRow);
				 it != droppedHolds.at(trtn.iTrack).end() && *it <= iSongRow;
				 it = droppedHolds.at(trtn.iTrack).lower_bound(*it + 1)) {

				// allow the game to treat a missed hold head
				// on its own and not force an extra drop
				// (check for None is for miniholds)
				if (trtn.pTN->result.tns != TNS_Miss &&
					trtn.pTN->result.tns != TNS_None) {
					trtn.pTN->HoldResult.bHeld = false;
					trtn.pTN->HoldResult.bActive = false;
					trtn.pTN->HoldResult.fLife = 0.f;
					trtn.pTN->HoldResult.hns = HNS_LetGo;

					// score the dead hold
					if (COMBO_BREAK_ON_IMMEDIATE_HOLD_LET_GO) {
						IncrementMissCombo();
					}
					SetHoldJudgment(
					  *trtn.pTN, iFirstTrackWithMaxEndRow, iSongRow);
					HandleHoldScore(*trtn.pTN);
				}

				droppedHolds.at(trtn.iTrack).erase(*it);
			}

			// remove the empty column to optimize this on next iteration
			if (droppedHolds.at(trtn.iTrack).size() == 0) {
				droppedHolds.erase(trtn.iTrack);
			}
		}
	}
}

void
PlayerReplay::UpdateHoldsAndRolls(
  float fDeltaTime,
  const std::chrono::steady_clock::time_point& now)
{
	const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const auto iSongRow = BeatToNoteRow(fSongBeat);

	// Auto tap rolls
	for (auto iTrack = 0; iTrack < m_NoteData.GetNumTracks(); ++iTrack) {
		int iHeadRow;
		if (!m_NoteData.IsHoldNoteAtRow(iTrack, iSongRow, &iHeadRow))
			iHeadRow = iSongRow;

		const auto& tn = m_NoteData.GetTapNote(iTrack, iHeadRow);
		if (tn.type != TapNoteType_HoldHead ||
			tn.subType != TapNoteSubType_Roll)
			continue;
		if (tn.HoldResult.hns != HNS_None)
			continue;
		if (tn.HoldResult.fLife >= 0.5f)
			continue;

		Step(iTrack, iHeadRow, now, true, false);
	}

	// HoldNotes logic
	{

		// Fast forward to the first that needs hold judging.
		{
			auto& iter = *m_pIterNeedsHoldJudging;
			while (!iter.IsAtEnd() && iter.Row() <= iSongRow &&
				   !NeedsHoldJudging(*iter))
				++iter;
		}

		std::vector<TrackRowTapNote> vHoldNotesToGradeTogether;
		auto iter = *m_pIterNeedsHoldJudging; // copy
		for (; !iter.IsAtEnd() && iter.Row() <= iSongRow; ++iter) {
			auto& tn = *iter;
			if (tn.type != TapNoteType_HoldHead)
				continue;

			const auto iTrack = iter.Track();
			const auto iRow = iter.Row();
			TrackRowTapNote trtn = { iTrack, iRow, &tn };

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
				// LOG->Trace( ssprintf("UpdateHoldNotes; %i != %i || !judge
				// holds on same row together",iRow,iRowOfLastHoldNote) );
				UpdateHoldNotes(
				  iSongRow, fDeltaTime, vHoldNotesToGradeTogether);
				vHoldNotesToGradeTogether.clear();
			}
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
PlayerReplay::Update(float fDeltaTime)
{
	const auto now = std::chrono::steady_clock::now();
	if (!m_bLoaded || GAMESTATE->m_pCurSong == nullptr)
		return;

	ActorFrame::Update(fDeltaTime); // NOLINT(bugprone-parent-virtual-call)

	const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const auto iSongRow = BeatToNoteRow(fSongBeat);

	ArrowEffects::SetCurrentOptions(
	  &m_pPlayerState->m_PlayerOptions.GetCurrent());

	UpdateVisibleParts();

	// Sure, why not?
	if (GAMESTATE->GetPaused())
		return;

	// Do replay playback updating
	CheckForSteps(now);

	// Update visual press state
	UpdatePressedFlags();

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
PlayerReplay::SetPlaybackEvents(
  const std::map<int, std::vector<PlaybackEvent>>& v)
{
	playbackEvents.clear();

	std::vector<PlaybackEvent> ghostTaps{};
	auto gapError = 0.F;

	auto musicRate = REPLAYS->GetActiveReplay()->GetMusicRate();

	for (auto& p : v) {
		auto noterow = p.first;
		auto& evts = p.second;

		auto rowpos = m_Timing->GetElapsedTimeFromBeat(NoteRowToBeat(noterow));
		for (auto& evt : evts) {
			auto supposedTime = evt.songPositionSeconds;

			if (evt.noterowJudged == -1) {
				ghostTaps.push_back(evt);
				continue;
			}

			// discrepancy in notedata/timing from actual replay
			// must update row and time to match current chart
			if (fabsf(rowpos - supposedTime) > 0.01F) {
				// haha oh my god
				noterow = BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(
				  m_Timing->GetElapsedTimeFromBeat(
					NoteRowToBeat(evt.noterowJudged)) +
				  (evt.offset * musicRate)));
				gapError = rowpos - supposedTime;
			}

			if (playbackEvents.count(noterow) == 0u) {
				playbackEvents.emplace(noterow, std::vector<PlaybackEvent>());
			}
			playbackEvents.at(noterow).push_back(evt);
		}
	}

	// handle ghost taps last because we didnt have enough data to fix gaps
	for (auto& evt : ghostTaps) {
		auto time =
		  m_Timing->GetElapsedTimeFromBeat(NoteRowToBeat(evt.noterow));
		auto newrow =
		  BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(time - gapError));
		if (playbackEvents.count(newrow) == 0u) {
			playbackEvents.emplace(newrow, std::vector<PlaybackEvent>());
		}
		playbackEvents.at(newrow).push_back(evt);
	}
}

void
PlayerReplay::CheckForSteps(const std::chrono::steady_clock::time_point& tm)
{
	if (playbackEvents.empty()) {
		return;
	}

	const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeat;
	const auto iSongRow = BeatToNoteRow(fSongBeat);
	
	std::vector<PlaybackEvent> evts;
	std::vector<int> rows;
	const auto it = playbackEvents.lower_bound(ARBITRARY_MIN_GAMEPLAY_NUMBER);
	auto row = it->first;

	// collect all playback events between the last update and now
	for (; row <= iSongRow && row != ARBITRARY_MIN_GAMEPLAY_NUMBER;) {
		rows.push_back(row);
		auto& rowevents = playbackEvents.at(row);
		evts.insert(evts.end(), rowevents.begin(), rowevents.end());

		auto nextit = playbackEvents.lower_bound(row + 1);
		if (nextit == playbackEvents.end()) {
			row = ARBITRARY_MIN_GAMEPLAY_NUMBER;
		} else {
			row = nextit->first;
		}
	}

	// execute all the events
	for (const auto& evt : evts) {
		if (evt.isPress) {
			if (holdingColumns.contains(evt.track)) {
				// it wont break the game, but we should track dupe presses
				Locator::getLogger()->warn(
				  "Please report an issue with this replay: {} - press {}, row "
				  "{}, judgerow {}, time {}, col {}",
				  REPLAYS->GetActiveReplay()->GetScoreKey(),
				  evt.isPress,
				  evt.noterow,
				  evt.noterowJudged,
				  evt.songPositionSeconds,
				  evt.track);
			}

			holdingColumns.insert(evt.track);
		} else {
			if (!holdingColumns.contains(evt.track)) {
				// it wont break the game, but we should track dupe releases
				Locator::getLogger()->warn(
				  "Please report an issue with this replay: {} - press {}, row "
				  "{}, judgerow {}, time {}, col {}",
				  REPLAYS->GetActiveReplay()->GetScoreKey(),
				  evt.isPress,
				  evt.noterow,
				  evt.noterowJudged,
				  evt.songPositionSeconds,
				  evt.track);
			}

			holdingColumns.erase(evt.track);
		}
		Step(evt.track,
			 evt.noterow,
			 tm,
			 false,
			 !evt.isPress,
			 0.F,
			 evt.noterowJudged,
			 evt.songPositionSeconds);
	}

	// delete all the used events
	if (!rows.empty()) {
		if (rows.size() == 1) {
			playbackEvents.erase(*rows.begin());
		} else {
			playbackEvents.erase(it,
								 playbackEvents.lower_bound(rows.back() + 1));
		}
	}
}

void
PlayerReplay::UpdatePressedFlags()
{
	const auto iNumCols =
	  GAMESTATE->GetCurrentStyle(GetPlayerState()->m_PlayerNumber)
		->m_iColsPerPlayer;

	for (auto col = 0; col < iNumCols; ++col) {
		ASSERT(m_pPlayerState != nullptr);

		const auto bIsHoldingButton = holdingColumns.count(col) != 0;
		if (bIsHoldingButton) {
			if (m_pNoteField != nullptr) {
				m_pNoteField->SetPressed(col);
			}
		}
	}
}

void
PlayerReplay::CrossedRows(int iLastRowCrossed,
						  const std::chrono::steady_clock::time_point& now)
{
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
				break;
			}
			case TapNoteType_Mine: {
				if (holdingColumns.count(iTrack))
					Step(iTrack, iRow, now, true, false, 0.F, iRow);
				break;
			}
			default:
				break;
		}

		// TODO: Can we remove the iLastSeenRow logic and the
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

	m_iFirstUncrossedRow = iLastRowCrossed + 1;
}

void
PlayerReplay::HandleTapRowScore(unsigned row)
{
	const auto scoreOfLastTap =
	  NoteDataWithScoring::LastTapNoteWithResult(m_NoteData, row).result.tns;
	const auto iOldCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const auto iOldMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	if (scoreOfLastTap == TNS_Miss)
		m_LastTapNoteScore = TNS_Miss;

	for (auto track = 0; track < m_NoteData.GetNumTracks(); ++track) {
		const auto& tn = m_NoteData.GetTapNote(track, row);
		// Mines cannot be handled here.
		if (tn.type == TapNoteType_Empty || tn.type == TapNoteType_Fake ||
			tn.type == TapNoteType_Mine || tn.type == TapNoteType_AutoKeysound)
			continue;
		if (m_pPrimaryScoreKeeper != nullptr && false)
			m_pPrimaryScoreKeeper->HandleTapScore(tn);
	}

	if (m_pPrimaryScoreKeeper != nullptr)
		m_pPrimaryScoreKeeper->HandleTapRowScore(m_NoteData, row);

	const auto iCurCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurCombo : 0;
	const auto iCurMissCombo =
	  m_pPlayerStageStats != nullptr ? m_pPlayerStageStats->m_iCurMissCombo : 0;

	SendComboMessages(iOldCombo, iOldMissCombo);

	if (m_pPlayerStageStats != nullptr) {
		SetCombo(iCurCombo, iCurMissCombo);
	}

	// mostly dead code for announcers and other stuff...
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
		  std::max(m_pPlayerStageStats->m_iMaxCombo, iCurCombo);

	/* Use the real current beat, not the beat we've been passed. That's because
	 * we want to record the current life/combo to the current time; eg. if it's
	 * a MISS, the beat we're registering is in the past, but the life is
	 * changing now. We need to include time from previous songs in a course, so
	 * we can't use GAMESTATE->m_fMusicSeconds. Use fStepsSeconds instead. */
	if (m_pPlayerStageStats)
		m_pPlayerStageStats->UpdateComboList(
		  GAMESTATE->m_Position.m_fMusicSeconds /
			GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate,
		  false);

	ChangeLife(scoreOfLastTap);
}

void
PlayerReplay::UpdateTapNotesMissedOlderThan(float fMissIfOlderThanSeconds)
{
	int iMissIfOlderThanThisRow;
	const auto fEarliestTime =
	  GAMESTATE->m_Position.m_fMusicSeconds - fMissIfOlderThanSeconds;
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

	auto& iter = *m_pIterNeedsTapJudging;

	for (; !iter.IsAtEnd() && iter.Row() < iMissIfOlderThanThisRow; ++iter) {
		auto& tn = *iter;

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

			if (GAMESTATE->CountNotesSeparately()) {
				SetJudgment(iter.Row(), iter.Track(), tn);
				HandleTapRowScore(iter.Row());
			}
		}
	}
}

// this is rewritten compared to Player::Step
// row is now steppedRow - we are remotely judging a row
// and then using rowToJudge to pick the note to judge, from this row
// the reason this matters is that rowToJudge is irrelevant if the
// judge ordering should need to change
void
PlayerReplay::Step(int col,
				   int steppedRow,
				   const std::chrono::steady_clock::time_point& tm,
				   bool bHeld,
				   bool bRelease,
				   float padStickSeconds,
				   int rowToJudge,
				   float forcedSongPositionSeconds)
{
	const std::chrono::duration<float> stepDelta =
	  std::chrono::steady_clock::now() - tm;
	const auto fPositionSeconds =
	  forcedSongPositionSeconds != 0.F
		? forcedSongPositionSeconds
		: m_Timing->GetElapsedTimeFromBeat(NoteRowToBeat(steppedRow));
	const auto fSongBeat =
	  forcedSongPositionSeconds != 0.F
		? m_Timing->GetBeatFromElapsedTime(forcedSongPositionSeconds)
		: NoteRowToBeat(steppedRow);

	if (col != -1 && !bRelease) {
		// Update roll life
		// Let's not check the whole array every time.
		// Instead, only check 1 beat back.  Even 1 is overkill.
		// Just update the life here and let Update judge the roll.
		const auto iStartCheckingAt =
		  std::max(0, steppedRow - BeatToNoteRow(1));
		NoteData::TrackMap::iterator begin, end;
		m_NoteData.GetTapNoteRangeInclusive(
		  col, iStartCheckingAt, steppedRow + 1, begin, end);
		for (; begin != end; ++begin) {
			auto& tn = begin->second;
			if (tn.type != TapNoteType_HoldHead)
				continue;

			switch (tn.subType) {
				DEFAULT_FAIL(tn.subType);
				case TapNoteSubType_Hold:
					continue;
				case TapNoteSubType_Roll:
					break;
			}

			const auto iRow = begin->first;

			const auto hns = tn.HoldResult.hns;
			if (hns != HNS_None) // if this HoldNote already has a result
				continue; // we don't need to update the logic for this one

			// if they got a bad score or haven't stepped on the corresponding
			// tap yet
			const auto tns = tn.result.tns;
			auto bInitiatedNote = true;
			bInitiatedNote =
			  tns != TNS_None && tns != TNS_Miss; // did they step on the start?
			const auto iEndRow = iRow + tn.iDuration;

			if (bInitiatedNote && tn.HoldResult.fLife != 0) {
				/* This hold note is not judged and we stepped on its head.
				 * Update iLastHeldRow. Do this even if we're a little beyond
				 * the end of the hold note, to make sure iLastHeldRow is
				 * clamped to iEndRow if the hold note is held all the way. */
				// LOG->Trace("setting iLastHeldRow to min of iSongRow (%i) and
				// iEndRow (%i)",iSongRow,iEndRow);
				tn.HoldResult.iLastHeldRow = std::min(steppedRow, iEndRow);
			}

			// If the song beat is in the range of this hold:
			if (iRow <= steppedRow && iRow <= iEndRow) {
				if (bInitiatedNote) {
					// Increase life
					tn.HoldResult.fLife = 1;

					if (ROLL_BODY_INCREMENTS_COMBO) {
						IncrementCombo();

						const auto bBright = m_pPlayerStageStats &&
											 m_pPlayerStageStats->m_iCurCombo >
											   static_cast<unsigned int>(
												 BRIGHT_GHOST_COMBO_THRESHOLD);
						if (m_pNoteField)
							m_pNoteField->DidHoldNote(col, HNS_Held, bBright);
					}
				}
				break;
			}
		}
	}

	// Check for tap
	static const auto StepSearchDistance = GetMaxStepDistanceSeconds();
	const auto iStepSearchRows =
	  std::max(BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(
				 fPositionSeconds + StepSearchDistance)) -
				 steppedRow,
			   steppedRow - BeatToNoteRow(m_Timing->GetBeatFromElapsedTime(
							  fPositionSeconds - StepSearchDistance))) +
	  ROWS_PER_BEAT;

	// calculate TapNoteScore
	auto score = TNS_None;

	auto rowBeingJudged = rowToJudge;
	if (rowToJudge == -1) {
		rowBeingJudged = GetClosestNote(col,
										steppedRow,
										iStepSearchRows,
										iStepSearchRows,
										false,
										false,
										false);
	}

	if (rowBeingJudged != -1 && col != -1) {
		// compute the score for this hit
		auto fNoteOffset = 0.f;

		const auto fStepBeat = NoteRowToBeat(rowBeingJudged);
		const auto fStepSeconds = m_Timing->WhereUAtBro(fStepBeat);

		// The offset from the actual step in seconds:
		fNoteOffset = (fStepSeconds - fPositionSeconds) /
					  REPLAYS->GetActiveReplay()->GetMusicRate();

		NOTESKIN->SetLastSeenColor(
		  NoteTypeToString(GetNoteType(rowBeingJudged)));

		TapNote* pTN = nullptr;
		auto closestNR = GetClosestNote(
		  col, rowBeingJudged, MAX_NOTE_ROW, MAX_NOTE_ROW, false, false);

		if (closestNR == -1 && !bRelease) {
			// the last notes of the file will trigger this due to releases
			// go ahead and skip releases
			Locator::getLogger()->warn(
			  "Please report an issue with this replay: {} - col {} steppedrow "
			  "{} rowtojudge {}",
			  REPLAYS->GetActiveReplay()->GetScoreKey(),
			  col,
			  steppedRow,
			  rowToJudge);
			return;
		}
		auto iter = m_NoteData.FindTapNote(col, closestNR);

		pTN = &iter->second;

		// We don't really have to care if we are releasing on a non-lift,
		// right? This fixes a weird noteskin bug with tap explosions.
		if (PREFSMAN->m_bFullTapExplosions && bRelease &&
			pTN->type != TapNoteType_Lift)
			return;

		// Score the Tap
		if (bHeld && pTN->type != TapNoteType_Mine) {
			// a hack to make Rolls not do weird things like
			// count as 0ms marvs.
			score = TNS_None;
			fNoteOffset = -1.f;
		} else {
			if (pTN->type == TapNoteType_Mine && pTN->result.tns == TNS_None) {
				if (!bRelease &&
					fabsf(fNoteOffset) <= GetWindowSeconds(TW_Mine) &&
					m_Timing->IsJudgableAtRow(closestNR)) {
					// we hit a mine
					score = TNS_HitMine;
				}
			} else {
				// every other case
				if (pTN->IsNote() || pTN->type == TapNoteType_Lift)
					score = ReplayManager::GetTapNoteScoreForReplay(
					  fNoteOffset, GetTimingWindowScale());
			}
		}

		// Do game-specific and mode-specific score mapping.
		score = GAMESTATE->GetCurrentGame()->MapTapNoteScore(score);

		if (score != TNS_None) {
			pTN->result.tns = score;
			pTN->result.fTapNoteOffset = -fNoteOffset;
		}

		m_LastTapNoteScore = score;
		if (pTN->result.tns != TNS_None)
			AddNoteToReplayData(GAMESTATE->CountNotesSeparately() ? col : -1,
								pTN,
								rowBeingJudged);
		if (GAMESTATE->CountNotesSeparately()) {
			if (pTN->type != TapNoteType_Mine) {
				const auto bBlind =
				  (m_pPlayerState->m_PlayerOptions.GetCurrent().m_fBlind != 0);
				const auto bBright =
				  (m_pPlayerStageStats &&
				   m_pPlayerStageStats->m_iCurCombo >
					 static_cast<unsigned int>(BRIGHT_GHOST_COMBO_THRESHOLD));
				if (m_pNoteField != nullptr)
					m_pNoteField->DidTapNote(
					  col, bBlind ? TNS_W1 : score, bBright);
				if (score >= m_pPlayerState->m_PlayerOptions.GetCurrent()
							   .m_MinTNSToHideNotes)
					HideNote(col, rowBeingJudged);

				if (pTN->result.tns != TNS_None) {
					if (pTN->type == TapNoteType_HoldHead && bHeld) {
						// odd hack to make roll taps (Step() with bHeld true)
						// not count as marvs
					} else {
						SetJudgment(rowBeingJudged, col, *pTN);
						HandleTapRowScore(rowBeingJudged);
					}
				}
			}
		} else if (NoteDataWithScoring::IsRowCompletelyJudged(
					 m_NoteData, rowBeingJudged)) {
			FlashGhostRow(rowBeingJudged);
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
		if (rowBeingJudged != -1 && score != TNS_None) {
			// just pressing a note, use that row.
			// in other words, iRowOfOverlappingNoteOrRow =
			// iRowOfOverlappingNoteOrRow
		} else if (m_NoteData.IsHoldNoteAtRow(col, steppedRow, &iHeadRow)) {
			// stepping on a hold, use it!
			rowBeingJudged = iHeadRow;
		} else {
			// or else find the closest note.
			rowBeingJudged = GetClosestNote(
			  col, steppedRow, MAX_NOTE_ROW, MAX_NOTE_ROW, true, false);
		}
		if (rowBeingJudged != -1) {
			const auto& tn =
			  m_NoteData.GetTapNote(col, rowBeingJudged);
			PlayKeysound(tn, score);
		}
	}

	if (!bRelease) {
		if (m_pNoteField != nullptr &&
			col != -1) { // skip misses to emulate missing.
			if (score != TNS_Miss) {
				m_pNoteField->Step(col, score);
			}
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
