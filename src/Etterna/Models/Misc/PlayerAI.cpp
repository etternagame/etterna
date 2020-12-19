#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "PlayerAI.h"
#include "PlayerState.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RadarValues.h"
#include "PlayerStageStats.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Actor/Gameplay/LifeMeterBar.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Core/Services/Locator.hpp"

#include <map>
#include <algorithm>
#include <set>

using std::map;

HighScore* PlayerAI::pScoreData = nullptr;
TimingData* PlayerAI::pReplayTiming = nullptr;
map<int, vector<TapReplayResult>> PlayerAI::m_ReplayTapMap;
map<int, vector<HoldReplayResult>> PlayerAI::m_ReplayHoldMap;
map<int, vector<TapReplayResult>> PlayerAI::m_ReplayExactTapMap;
map<int, ReplaySnapshot> PlayerAI::m_ReplaySnapshotMap;
map<float, vector<TapReplayResult>> PlayerAI::m_ReplayTapMapByElapsedTime;
map<float, vector<HoldReplayResult>> PlayerAI::m_ReplayHoldMapByElapsedTime;
float PlayerAI::replayRate = 1.f;
std::string PlayerAI::replayModifiers;
bool PlayerAI::replayUsedMirror = false;
std::string PlayerAI::oldModifiers;
float PlayerAI::oldRate = 1.f;
std::string PlayerAI::oldNoteskin;
FailType PlayerAI::oldFailType = FailType_Immediate;

TapNoteScore
PlayerAI::GetTapNoteScore(const PlayerState* pPlayerState)
{
	if (pPlayerState->m_PlayerController == PC_REPLAY)
		return TNS_Miss;
	if (pPlayerState->m_PlayerController == PC_AUTOPLAY ||
		pPlayerState->m_PlayerController == PC_CPU)
		return TNS_W1;

	return TNS_Miss;
}

TapNoteScore
PlayerAI::GetTapNoteScoreForReplay(const PlayerState* pPlayerState,
								   float fNoteOffset,
								   float timingScale)
{
	// This code is basically a copy paste from somewhere in Player for grabbing
	// scores.

	// LOG->Trace("Given number %f ", fNoteOffset);
	const auto fSecondsFromExact = fabsf(fNoteOffset);
	// LOG->Trace("TapNoteScore For Replay Seconds From Exact: %f",
	// fSecondsFromExact);

	if (fSecondsFromExact >= 1.f)
		return TNS_Miss;

	if (fSecondsFromExact <=
		Player::GetWindowSecondsCustomScale(TW_W1, timingScale))
		return TNS_W1;
	else if (fSecondsFromExact <=
			 Player::GetWindowSecondsCustomScale(TW_W2, timingScale))
		return TNS_W2;
	else if (fSecondsFromExact <=
			 Player::GetWindowSecondsCustomScale(TW_W3, timingScale))
		return TNS_W3;
	else if (fSecondsFromExact <=
			 Player::GetWindowSecondsCustomScale(TW_W4, timingScale))
		return TNS_W4;
	else if (fSecondsFromExact <=
			 std::max(Player::GetWindowSecondsCustomScale(TW_W5, timingScale),
					  0.18f))
		return TNS_W5;
	return TNS_None;
}

float
PlayerAI::FindMissWindowBegin()
{
	// when this becomes useful, this will be finished
	return 0.18f;
}

void
PlayerAI::ResetScoreData()
{
	// should probably not null out scoredata here
	// but it is a highscore that exists somewhere so...
	pScoreData = nullptr;
	m_ReplayTapMap.clear();
	m_ReplayHoldMap.clear();
	m_ReplayExactTapMap.clear();
	m_ReplaySnapshotMap.clear();
	m_ReplayTapMapByElapsedTime.clear();
	m_ReplayHoldMapByElapsedTime.clear();
	replayRate = 1.f;
	replayModifiers.clear();
	replayUsedMirror = false;
	oldModifiers.clear();
	oldRate = 1.f;
	oldNoteskin.clear();
	oldFailType = FailType_Immediate;
}

void
PlayerAI::SetScoreData(HighScore* pHighScore, int firstRow, NoteData* pNoteData)
{
	Locator::getLogger()->trace("Setting PlayerAI Score Data");
	auto successful = false;
	if (pHighScore != nullptr)
		successful = pHighScore->LoadReplayData();

	pScoreData = pHighScore;
	m_ReplayTapMap.clear();
	m_ReplayHoldMap.clear();
	m_ReplayExactTapMap.clear();
	// we dont clear snapshot map here for a particular reason

	if (pNoteData != nullptr)
		m_ReplaySnapshotMap.clear();

	if (!successful || pHighScore == nullptr) {
		Locator::getLogger()->trace("Exiting Score Data setup - missing HS or ReplayData");
		return;
	}

	auto replayNoteRowVector = pHighScore->GetCopyOfNoteRowVector();
	auto replayOffsetVector = pHighScore->GetCopyOfOffsetVector();
	auto replayTapNoteTypeVector = pHighScore->GetCopyOfTapNoteTypeVector();
	auto replayTrackVector = pHighScore->GetCopyOfTrackVector();
	auto replayHoldVector = pHighScore->GetCopyOfHoldReplayDataVector();

	// Record valid noterows so that we don't waste a lot of time to do the last
	// step here
	std::set<int> validNoterows;

	// Generate TapReplayResults to put into a vector referenced by the song row
	// in a map
	for (size_t i = 0; i < replayNoteRowVector.size(); i++) {
		if (fabsf(replayOffsetVector[i]) > 0.18f)
			continue;
		if (replayNoteRowVector[i] < firstRow)
			continue;

		TapReplayResult trr;
		trr.row = replayNoteRowVector[i];
		trr.offset = replayOffsetVector[i];
		trr.offsetAdjustedRow = static_cast<int>(replayOffsetVector[i]);
		if (pScoreData->GetReplayType() ==
			2) // 2 means that this is a Full Replay
		{
			trr.track = replayTrackVector[i];
			trr.type = replayTapNoteTypeVector[i];
		} else // Anything else (and we got this far without crashing) means
			   // it's not a Full Replay
		{
			trr.track = NULL;
			trr.type = TapNoteType_Empty;
		}

		// Create or append to the vector
		if (m_ReplayTapMap.count(replayNoteRowVector[i]) != 0) {
			m_ReplayTapMap[replayNoteRowVector[i]].push_back(trr);
		} else {
			const vector<TapReplayResult> trrVector = { trr };
			m_ReplayTapMap[replayNoteRowVector[i]] = trrVector;
			validNoterows.insert(replayNoteRowVector[i]);
		}
	}

	// Generate vectors made of pregenerated HoldReplayResults referenced by the
	// song row in a map
	// Only present in replays with column data.
	for (auto& i : replayHoldVector) {
		if (i.row < firstRow)
			continue;

		// Create or append to the vector
		if (m_ReplayHoldMap.count(i.row) != 0) {
			m_ReplayHoldMap[i.row].push_back(i);
		} else {
			const vector<HoldReplayResult> hrrVector = { i };
			m_ReplayHoldMap[i.row] = hrrVector;
			validNoterows.insert(i.row);
		}
	}

	// Misses don't always show up in Replay Data properly.
	// We require the NoteData to validate the Judge count.
	// If we don't have it, don't care.
	if (pNoteData == nullptr) {
		Locator::getLogger()->trace("Exiting Score Data setup - missing NoteData");
		return;
	}

	// Set up a mapping of every noterow to a snapshot of what has happened up
	// to that point
	SetUpSnapshotMap(pNoteData, validNoterows);
	Locator::getLogger()->trace("Finished Score Data setup");
}

void
PlayerAI::SetUpExactTapMap(TimingData* timing)
{
	pReplayTiming = timing;

	m_ReplayExactTapMap.clear();
	m_ReplayTapMapByElapsedTime.clear();
	m_ReplayHoldMapByElapsedTime.clear();

	// Don't continue if the replay doesn't have column data.
	// We can't be accurate without it.
	if (pScoreData->GetReplayType() != 2)
		return;

	// For every row in the replay data...
	for (auto& row : m_ReplayTapMap) {
		// Get the current time and go over all taps on this row...
		const auto rowTime = timing->WhereUAtBro(row.first);
		for (auto& trr : row.second) {
			// Find the time adjusted for offset
			// Then the beat according to that time
			// Then the noterow according to that beat
			auto tapTime = rowTime + trr.offset;
			const auto tapBeat = timing->GetBeatFromElapsedTime(tapTime);
			auto tapRow = BeatToNoteRow(tapBeat);
			trr.offsetAdjustedRow = tapRow;

			// And put that into the exacttapmap :)
			if (m_ReplayExactTapMap.count(tapRow) != 0) {
				m_ReplayExactTapMap[tapRow].push_back(trr);
			} else {
				const vector<TapReplayResult> trrVector = { trr };
				m_ReplayExactTapMap[tapRow] = trrVector;
			}

			// Also put it into the elapsed time map :)
			if (m_ReplayTapMapByElapsedTime.count(tapTime) != 0) {
				m_ReplayTapMapByElapsedTime[tapTime].push_back(trr);
			} else {
				const vector<TapReplayResult> trrVector = { trr };
				m_ReplayTapMapByElapsedTime[tapTime] = trrVector;
			}
		}
	}

	// Sneak in the HoldMapByElapsedTime construction for consistency
	// Go over all of the elements, you know the deal.
	// We can avoid getting offset rows here since drops don't do that
	for (auto& row : m_ReplayHoldMap) {
		auto dropTime = timing->WhereUAtBro(row.first);
		for (auto& hrr : row.second) {
			if (m_ReplayHoldMapByElapsedTime.count(dropTime) != 0) {
				m_ReplayHoldMapByElapsedTime[dropTime].push_back(hrr);
			} else {
				const vector<HoldReplayResult> hrrVector = { hrr };
				m_ReplayHoldMapByElapsedTime[dropTime] = hrrVector;
			}
		}
	}

	// If things were in the right order by this point
	// then we know SnapshotMap is filled out.
	// This is how we can find misses quickly without having to keep
	// track of them in some other special way.
	if (!m_ReplaySnapshotMap.empty()) {
		auto curSnap = m_ReplaySnapshotMap.begin();
		++curSnap;
		auto prevSnap = m_ReplaySnapshotMap.begin();
		while (curSnap != m_ReplaySnapshotMap.end()) {
			auto csn = curSnap->second;
			auto psn = prevSnap->second;
			const auto missDiff =
			  csn.judgments[TNS_Miss] - psn.judgments[TNS_Miss];
			if (missDiff > 0) {
				const auto row = curSnap->first;
				// the tap time is pushed back by the smallest normal boo
				// window. the reason for this is that's about the point where
				// the game should usually count something as a miss. we dont
				// use this time for anything other than chronologically parsing
				// replay data for combo/life stuff so this is okay (i hope)
				auto tapTime = timing->WhereUAtBro(row) + .18f;
				for (auto i = 0; i < missDiff; i++) {
					// we dont really care about anything other than the offset
					// because we have the estimate time at the row in the map
					// and then we just need to know what judgment to assign it
					TapReplayResult trr;
					trr.row = row;
					trr.offset = 1.f;
					if (m_ReplayTapMapByElapsedTime.count(tapTime) != 0) {
						m_ReplayTapMapByElapsedTime[tapTime].push_back(trr);
					} else {
						const vector<TapReplayResult> trrVector = { trr };
						m_ReplayTapMapByElapsedTime[tapTime] = trrVector;
					}
				}
			}
			++curSnap;
			++prevSnap;
		}
	}
}

void
PlayerAI::SetUpSnapshotMap(NoteData* pNoteData,
						   std::set<int> validNoterows,
						   float timingScale)
{
	m_ReplaySnapshotMap.clear();

	// Don't continue if the scoredata used invalidating mods
	// (Particularly mods that make it difficult to match NoteData)
	if (pScoreData != nullptr) {
		PlayerOptions potmp;
		potmp.FromString(pScoreData->GetModifiers());
		if (potmp.ContainsTransformOrTurn())
			return;
	}

	// Fill out the Snapshot map now that the other maps are not so out of order
	// We leave out misses in this section because they aren't in the Replay
	// Data
	int tempJudgments[NUM_TapNoteScore] = { 0 };
	int tempHNS[NUM_HoldNoteScore] = { 0 };

	// Copy of the main hold map
	// Delete the values as we go on to make sure they get counted
	auto m_unjudgedholds(m_ReplayHoldMap);

	// If we don't have validnoterows, just do it the hard way
	if (validNoterows.empty()) {
		for (const auto& row : m_ReplayTapMap) {
			for (auto& trr : row.second) {
				if (trr.type == TapNoteType_Mine) {
					tempJudgments[TNS_HitMine]++;
				} else {
					auto tns = GetTapNoteScoreForReplay(
					  nullptr, trr.offset, timingScale);
					tempJudgments[tns]++;
				}
			}

			// Make the struct and cram it in there
			// ignore holds for now since the following block takes care of them
			ReplaySnapshot rs;
			FOREACH_ENUM(TapNoteScore, tns)
			rs.judgments[tns] = tempJudgments[tns];
			m_ReplaySnapshotMap[row.first] = rs;
		}
	} else {
		// Iterate over all the noterows we know are in the Replay Data
		for (auto validNoterow : validNoterows) {
			// Check for taps and mines
			if (m_ReplayTapMap.count(validNoterow) != 0) {
				for (auto instance = m_ReplayTapMap[validNoterow].begin();
					 instance != m_ReplayTapMap[validNoterow].end();
					 ++instance) {
					auto& trr = *instance;
					if (trr.type == TapNoteType_Mine) {
						tempJudgments[TNS_HitMine]++;
					} else {
						auto tns = GetTapNoteScoreForReplay(
						  nullptr, trr.offset, timingScale);
						tempJudgments[tns]++;
					}
				}
			}

			// Make the struct and cram it in there
			// ignore holds for now since the following block takes care of them
			ReplaySnapshot rs;
			FOREACH_ENUM(TapNoteScore, tns)
			rs.judgments[tns] = tempJudgments[tns];
			m_ReplaySnapshotMap[validNoterow] = rs;
		}
	}

	// Have to account for mirror being in the highscore options
	// please dont change styles in the middle of calculation and break this
	// thanks
	if (pScoreData->GetModifiers().find("mirror") != std::string::npos ||
		pScoreData->GetModifiers().find("Mirror") != std::string::npos) {
		PlayerOptions po;
		po.Init();
		po.m_bTurns[PlayerOptions::TURN_MIRROR] = true;
		NoteDataUtil::TransformNoteData(
		  *pNoteData,
		  *pReplayTiming,
		  po,
		  GAMESTATE->GetCurrentStyle(GAMESTATE->m_pPlayerState->m_PlayerNumber)
			->m_StepsType);
	}

	// Now handle misses and holds.
	// For every row in notedata...
	FOREACH_NONEMPTY_ROW_ALL_TRACKS(*pNoteData, row)
	{
		auto tapsMissedInRow = 0;

		// For every track in the row...
		for (auto track = 0; track < pNoteData->GetNumTracks(); track++) {
			// Find the tapnote we are on
			auto iter = pNoteData->FindTapNote(track, row);

			if (iter != pNoteData->end(track)) {
				auto pTN = &iter->second;
				// Deal with holds here
				if (pTN->type == TapNoteType_HoldHead) {
					auto isDropped = IsHoldDroppedInRowRangeForTrack(
					  row, row + pTN->iDuration, track);
					if (isDropped != -1) {
						tempHNS[HNS_LetGo]++;

						// erase the hold from the mapping of unjudged holds
						for (size_t i = 0;
							 i < m_unjudgedholds[isDropped].size();
							 i++) {
							if (m_unjudgedholds[isDropped][i].track == track) {
								m_unjudgedholds[isDropped].erase(
								  m_unjudgedholds[isDropped].begin() + i);
							}
						}
						if (m_unjudgedholds[isDropped].empty())
							m_unjudgedholds.erase(isDropped);
					} else {
						tempHNS[HNS_Held]++;
					}
				}

				// See if we passed the earliest dropped hold by now
				// This catches issues where a hold is missed completely
				// and the hold was short enough to bug out
				// and the reported row of the dropped hold is weirdly placed
				auto firstUnjudgedHold = m_unjudgedholds.begin();
				if (firstUnjudgedHold != m_unjudgedholds.end()) {
					auto hrrs = firstUnjudgedHold->second;
					for (size_t i = 0; i < hrrs.size(); i++) {
						if (hrrs[i].track == track) {
							m_unjudgedholds[firstUnjudgedHold->first].erase(
							  m_unjudgedholds[firstUnjudgedHold->first]
								.begin() +
							  i);
							tempHNS[HNS_LetGo]++;
						}
					}
					if (m_unjudgedholds[firstUnjudgedHold->first].empty())
						m_unjudgedholds.erase(firstUnjudgedHold->first);
				}

				// Deal with misses here
				// It is impossible to "miss" these notes
				// TapNoteType_HoldTail does not exist in NoteData
				if (pTN->type == TapNoteType_Mine ||
					pTN->type == TapNoteType_Fake ||
					pTN->type == TapNoteType_AutoKeysound)
					continue;

				// If this tap is missing from the replay data, we count it as a
				// miss.
				if (pScoreData->GetReplayType() == 2) {
					if (m_ReplayTapMap.count(row) != 0) {
						auto found = false;
						for (auto& trr : m_ReplayTapMap[row]) {
							if (trr.track == track)
								found = true;
						}
						if (!found) {
							tempJudgments[TNS_Miss]++;
							tapsMissedInRow++;
						}
					} else {
						tempJudgments[TNS_Miss]++;
						tapsMissedInRow++;
					}
				}
			}
		}

		// Count how many misses there are per row instead since we dont have
		// column data in type 1 replays
		if (pScoreData->GetReplayType() != 2) {
			unsigned notesOnRow = 0;
			unsigned notesInReplayData = 0;
			if (m_ReplayTapMap.count(row) != 0)
				notesInReplayData += m_ReplayTapMap[row].size();
			for (auto track = 0; track < pNoteData->GetNumTracks(); track++) {
				auto iter = pNoteData->FindTapNote(track, row);
				if (iter != pNoteData->end(track)) {
					auto pTN = &iter->second;
					if (pTN->type == TapNoteType_Fake ||
						pTN->type == TapNoteType_Mine ||
						pTN->type == TapNoteType_AutoKeysound)
						continue;
					notesOnRow++;
				}
			}
			tempJudgments[TNS_Miss] += (notesOnRow - notesInReplayData);
			tapsMissedInRow += (notesOnRow - notesInReplayData);
		}

		// We have to update every single row with the new miss & hns counts.
		// This unfortunately takes more time.
		// If current row is recorded in the snapshots, update the counts
		if (m_ReplaySnapshotMap.count(row) != 0) {
			m_ReplaySnapshotMap[row].judgments[TNS_Miss] =
			  tempJudgments[TNS_Miss];
			FOREACH_ENUM(HoldNoteScore, hns)
			m_ReplaySnapshotMap[row].hns[hns] = tempHNS[hns];

		} else {
			// If the current row is after the last recorded row, make a new one
			if (m_ReplaySnapshotMap.empty() ||
				m_ReplaySnapshotMap.rbegin()->first < row) {
				ReplaySnapshot rs;
				FOREACH_ENUM(TapNoteScore, tns)
				rs.judgments[tns] = tempJudgments[tns];
				FOREACH_ENUM(HoldNoteScore, hns)
				rs.hns[hns] = tempHNS[hns];
				m_ReplaySnapshotMap[row] = rs;
			}
			// If the current row is before the earliest recorded row, make a
			// new one
			else if (m_ReplaySnapshotMap.begin()->first > row) {
				ReplaySnapshot rs;
				rs.judgments[TNS_Miss] = tapsMissedInRow;
				m_ReplaySnapshotMap[row] = rs;
			} else // If the current row is in between recorded rows, copy an
				   // older one
			{
				ReplaySnapshot rs;
				auto prev = m_ReplaySnapshotMap.lower_bound(row);
				--prev;
				// it is expected at this point that prev is not somehow outside
				// the range if it is, we have bigger problems
				FOREACH_ENUM(TapNoteScore, tns)
				rs.judgments[tns] =
				  m_ReplaySnapshotMap[prev->first].judgments[tns];
				FOREACH_ENUM(HoldNoteScore, hns)
				rs.hns[hns] = m_ReplaySnapshotMap[prev->first].hns[hns];
				m_ReplaySnapshotMap[row] = rs;
			}
		}
	}
	/* The final output here has 2 minor issues:
	 * - Holds completely missed are not counted as HNS_Missed
	 * - Holds completed are not placed in Snapshot until after they are
	 * complete
	 * However, completely missed holds are present in replay data.
	 * The second issue does not cause miscounts, but does cause butchered holds
	 * to be missed (but not judged)
	 */

	// now update the wifescore values for each relevant snapshot.
	// some snapshots end up with 0 values due to being "missing" from the
	// replay data and we have to account for those
	vector<int> snapShotsUnused;
	snapShotsUnused.reserve(m_ReplaySnapshotMap.size());
	for (auto& it : m_ReplaySnapshotMap)
		snapShotsUnused.push_back(it.first);
	auto cws = 0.f;
	auto mws = 0.f;
	for (auto it = m_ReplayTapMap.begin(); it != m_ReplayTapMap.end();) {
		const auto r = it->first;
		if (r > snapShotsUnused.front()) {
			// if we somehow skipped a snapshot, the only difference should be
			// in misses and non taps
			auto rs = &m_ReplaySnapshotMap[snapShotsUnused.front()];
			rs->curwifescore = cws +
							   (rs->judgments[TNS_Miss] * wife3_miss_weight) +
							   ((rs->hns[HNS_Missed] + rs->hns[HNS_LetGo]) *
								wife3_hold_drop_weight);
			rs->maxwifescore = mws + (rs->judgments[TNS_Miss] * 2.f);
			snapShotsUnused.erase(snapShotsUnused.begin());

			continue; // retry the iteration (it++ is moved to below)
		}
		auto rs = GetReplaySnapshotForNoterow(r);
		for (auto& trr : it->second) {
			if (trr.type == TapNoteType_Mine) {
				cws += wife3_mine_hit_weight;
			} else {
				cws += wife3(trr.offset, timingScale);
				mws += 2.f;
			}
		}
		rs->curwifescore =
		  cws + (rs->judgments[TNS_Miss] * wife3_miss_weight) +
		  ((rs->hns[HNS_Missed] + rs->hns[HNS_LetGo]) * wife3_hold_drop_weight);
		rs->maxwifescore = mws + (rs->judgments[TNS_Miss] * 2.f);

		snapShotsUnused.erase(snapShotsUnused.begin());
		++it;
	}
	if (!snapShotsUnused.empty()) {
		for (auto row : snapShotsUnused) {
			// This might not be technically correct
			// But my logic is this:
			// A snapshot without associated replaydata is one for a row
			// which has no stat-affecting changes made to it.
			// So this applies to rows with all Mines
			// or rows with all Fakes (in the latest version)
			auto prevrs = GetReplaySnapshotForNoterow(row - 1);
			auto rs = &m_ReplaySnapshotMap[row];
			rs->curwifescore = prevrs->curwifescore;
			rs->maxwifescore = prevrs->maxwifescore;
		}
	}
}

void
PlayerAI::RemoveTapFromVectors(int row, int col)
{
	// if the row is in the replay data
	if (m_ReplayTapMap.count(row) != 0) {
		for (auto i = 0; i < (int)m_ReplayTapMap[row].size(); i++) {
			// if the column is in the row data
			auto& trr = m_ReplayTapMap[row][i];
			if (trr.track == col) {
				// delete
				m_ReplayTapMap[row].erase(m_ReplayTapMap[row].begin() + i);
				if (m_ReplayTapMap[row].empty())
					m_ReplayTapMap.erase(row);
			}
		}
	}
	// if the row is in the replay data
	if (m_ReplayExactTapMap.count(row) != 0) {
		for (auto i = 0; i < (int)m_ReplayExactTapMap[row].size(); i++) {
			// if the column is in the row data
			auto& trr = m_ReplayExactTapMap[row][i];
			if (trr.track == col) {
				// delete
				m_ReplayExactTapMap[row].erase(
				  m_ReplayExactTapMap[row].begin() + i);
				if (m_ReplayExactTapMap[row].empty())
					m_ReplayExactTapMap.erase(row);
			}
		}
	}
}

int
PlayerAI::GetAdjustedRowFromUnadjustedCoordinates(int row, int col)
{
	auto output = -1;

	if (m_ReplayTapMap.count(row) != 0) {
		for (auto& trr : m_ReplayTapMap[row]) {
			if (trr.track == col)
				output = trr.offsetAdjustedRow;
		}
	}
	return output;
}

std::shared_ptr<ReplaySnapshot>
PlayerAI::GetReplaySnapshotForNoterow(int row)
{
	// The row doesn't necessarily have to exist in the Snapshot map.
	// Because after a Snapshot, we can try this again for a later row
	// And if there are no new snapshots (no events) nothing changes
	// So we return that earlier snapshot.

	// If the lowest value in the map is above the given row, return an empty
	// snapshot
	if (m_ReplaySnapshotMap.begin()->first > row) {
		return std::shared_ptr<ReplaySnapshot>{ new ReplaySnapshot };
	}

	// For some reason I don't feel like figuring out, if the largest value in
	// the map is below the given row, it returns 0 So we need to return the
	// last value
	if (m_ReplaySnapshotMap.rbegin()->first < row) {
		return std::shared_ptr<ReplaySnapshot>{
			&m_ReplaySnapshotMap.rbegin()->second, [](ReplaySnapshot*) {}
		};
	}

	// Otherwise just go ahead and return what we want
	auto lb = m_ReplaySnapshotMap.lower_bound(row);
	auto foundRow = lb->first;

	// lower_bound gets an iter to the next element >= the given key
	// and we have to decrement if it is greater than the key (because we want
	// that)
	if (foundRow > row) {
		if (lb != m_ReplaySnapshotMap.begin()) {
			--lb;
			foundRow = lb->first;
		} else {
			return std::shared_ptr<ReplaySnapshot>{ new ReplaySnapshot };
		}
	}
	return std::shared_ptr<ReplaySnapshot>{ &m_ReplaySnapshotMap[foundRow],
											[](ReplaySnapshot*) {} };
}

bool
PlayerAI::DetermineIfHoldDropped(int noteRow, int col)
{
	// LOG->Trace("Checking for hold.");
	// Is the given row/column in our dropped hold map?
	if (m_ReplayHoldMap.count(noteRow) != 0) {
		// LOG->Trace("Hold row exists in the data");
		// It is, so let's go over each column, assuming we may have dropped
		// more than one hold at once.
		for (auto& hrr : m_ReplayHoldMap[noteRow]) {
			// We found the column we are looking for
			if (hrr.track == col) {
				// LOG->Trace("KILL IT NOW");
				return true;
			}
		}
	}
	return false;
}

int
PlayerAI::IsHoldDroppedInRowRangeForTrack(int firstRow, int endRow, int track)
{
	// 2 is a replay with column data
	if (pScoreData->GetReplayType() == 2) {
		// Go over all holds in Replay Data
		for (auto hiter = m_ReplayHoldMap.lower_bound(firstRow);
			 hiter != m_ReplayHoldMap.end();
			 ++hiter) {
			// If this row is before the start, skip it
			if (hiter->first < firstRow)
				continue;
			// If this row is after the end, skip it
			else if (hiter->first > endRow)
				return -1;
			// This row might work. Check what tracks might have dropped.
			for (const auto hrr : hiter->second) {
				if (hrr.track == track)
					return hiter->first;
			}
		}
		// Iteration finished without finding a drop.
		return -1;
	} else {
		// Replay Data doesn't contain hold data.
		return -1;
	}
}

bool
PlayerAI::TapExistsAtThisRow(int noteRow)
{
	// 2 is a replay with column data
	if (pScoreData->GetReplayType() == 2) {
		return m_ReplayExactTapMap.count(noteRow) != 0;
	} else {
		return m_ReplayTapMap.count(noteRow) != 0;
	}
}

bool
PlayerAI::TapExistsAtOrBeforeThisRow(int noteRow)
{
	// 2 is a replay with column data
	if (pScoreData->GetReplayType() == 2) {
		return m_ReplayExactTapMap.lower_bound(-20000)->first <= noteRow;
	} else {
		return m_ReplayTapMap.lower_bound(-20000)->first <= noteRow;
	}
}

bool
PlayerAI::IsTapAtRowAndColumn(int noteRow, int col)
{
	if (m_ReplayTapMap.count(noteRow) == 0)
		return false;
	for (auto& tap : m_ReplayTapMap[noteRow]) {
		if (tap.track == col)
			return true;
	}
	return false;
}

vector<TapReplayResult>
PlayerAI::GetTapsAtOrBeforeRow(int noteRow)
{
	vector<TapReplayResult> output;

	// 2 is a replay with column data
	if (pScoreData->GetReplayType() == 2) {
		const auto rowIt = m_ReplayExactTapMap.lower_bound(-20000);
		auto row = rowIt->first;
		for (; row <= noteRow && row != -20000;) {
			auto toMerge = GetTapsToTapForRow(row);
			output.insert(output.end(), toMerge.begin(), toMerge.end());
			row = GetNextRowNoOffsets(row);
		}
	} else {
		const auto rowIt = m_ReplayTapMap.lower_bound(-20000);
		auto row = rowIt->first;
		for (; row <= noteRow && row != -20000;) {
			auto toMerge = GetTapsToTapForRow(row);
			output.insert(output.end(), toMerge.begin(), toMerge.end());
			row = GetNextRowNoOffsets(row);
		}
	}
	return output;
}

vector<TapReplayResult>
PlayerAI::GetTapsToTapForRow(int noteRow)
{
	vector<TapReplayResult> output;

	// 2 is a replay with column data
	if (pScoreData->GetReplayType() == 2) {
		if (m_ReplayExactTapMap.count(noteRow) != 0) {
			for (auto& trr : m_ReplayExactTapMap[noteRow]) {
				output.push_back(trr);
			}
		}
	} else {
		if (m_ReplayTapMap.count(noteRow) != 0) {
			for (auto& trr : m_ReplayTapMap[noteRow]) {
				output.push_back(trr);
			}
		}
	}
	return output;
}

int
PlayerAI::GetReplayType()
{
	return pScoreData->GetReplayType();
}

int
PlayerAI::GetNextRowNoOffsets(int currentRow)
{
	if (pScoreData->GetReplayType() == 2) {
		const auto thing = m_ReplayExactTapMap.lower_bound(currentRow + 1);

		if (thing == m_ReplayExactTapMap.end()) {
			return -20000;
		} else {
			return thing->first;
		}
	} else {
		const auto thing = m_ReplayTapMap.lower_bound(currentRow + 1);

		if (thing == m_ReplayTapMap.end()) {
			return -20000;
		} else {
			return thing->first;
		}
	}
}

float
PlayerAI::GetTapNoteOffsetForReplay(TapNote* pTN, int noteRow, int col)
{
	/* Given the pTN coming from gameplay, we search for the matching note
	in the replay data. If it is not found, it is a miss. (1.f)
	*/
	if (pScoreData == nullptr) // possible cheat prevention
		return -1.f;

	// Current v0.60 Replay Data format:
	//	[noterow] [offset] [track] [optional: tap note type]
	// Current v0.60 Replay Data format (H section):
	//	H [noterow] [track] [optional: tap note subtype]
	// LOG->Trace("Note row %d", noteRow);

	// This replay has no column data or is considered Basic. (Pre-v0.60
	// Replays do this.)
	if (pScoreData->GetReplayType() == 1) {
		// mines are not preset in the old replay data, we just skip them
		// this gets caught by Player after it finds that the offset wasnt
		// -2.f (We check for an impossible offset of -2.f in Player to blow
		// up a mine)
		if (pTN->type == TapNoteType_Mine || m_ReplayTapMap.count(noteRow) == 0)
			return -1.f;

		const auto offset = m_ReplayTapMap[noteRow].back().offset;

		// this is done to be able to judge simultaneous taps differently
		// due to CC Off this results in possibly incorrect precise per tap
		// judges, but the correct judgement ends up being made overall.

		if (!pScoreData->GetChordCohesion()) {
			m_ReplayTapMap[noteRow].pop_back();
			if (m_ReplayTapMap[noteRow].empty()) {
				m_ReplayTapMap.erase(noteRow);
			}
		}

		return -offset;
	} else {

		// This is only reached if we have column data.
		noteRow = GetAdjustedRowFromUnadjustedCoordinates(noteRow, col);
		if (m_ReplayExactTapMap.count(noteRow) != 0) {
			for (auto i = 0; i < (int)m_ReplayExactTapMap[noteRow].size();
				 i++) // go over all elements in the row
			{
				const auto trr = m_ReplayExactTapMap[noteRow][i];
				if (trr.track == col) // if the column expected is the
									  // actual note, use it
				{
					if (trr.type == TapNoteType_Mine) // hack for mines
						return -2.f;
					if (pTN->type == TapNoteType_Lift) {
						if (trr.type != TapNoteType_Lift)
							continue;
					}
					m_ReplayExactTapMap[noteRow].erase(
					  m_ReplayExactTapMap[noteRow].begin() + i);
					if (m_ReplayExactTapMap[noteRow].empty())
						m_ReplayExactTapMap.erase(noteRow);
					return -trr.offset;
				}
			}
		}
	}

	return -1.f; // data missing or invalid, give them a miss
}

void
PlayerAI::CalculateRadarValuesForReplay(RadarValues& rv,
										RadarValues& possibleRV)
{
	Locator::getLogger()->trace("Calculating Radar Values from ReplayData");
	// We will do this thoroughly just in case someone decides to use the
	// other categories we don't currently use
	auto tapsHit = 0;
	auto jumpsHit = 0;
	auto handsHit = 0;
	auto holdsHeld = possibleRV[RadarCategory_Holds];
	auto rollsHeld = possibleRV[RadarCategory_Rolls];
	auto liftsHit = 0;
	auto fakes = possibleRV[RadarCategory_Fakes];
	auto totalNotesHit = 0;
	auto minesMissed = possibleRV[RadarCategory_Mines];

	// For every row recorded...
	for (auto& row : m_ReplayTapMap) {
		auto tapsOnThisRow = 0;
		// For every tap on these rows...
		for (auto& trr : row.second) {
			if (trr.type == TapNoteType_Fake) {
				fakes--;
				continue;
			}
			if (trr.type == TapNoteType_Mine) {
				minesMissed--;
				continue;
			}
			if (trr.type == TapNoteType_Lift) {
				liftsHit++;
				continue;
			}
			tapsOnThisRow++;
			// We handle Empties as well because that's what old replays are
			// loaded as.
			if (trr.type == TapNoteType_Tap ||
				trr.type == TapNoteType_HoldHead ||
				trr.type == TapNoteType_Empty) {
				totalNotesHit++;
				tapsHit++;
				if (tapsOnThisRow == 2) {
					// This is technically incorrect.
					jumpsHit++;
				} else if (tapsOnThisRow >= 3) {
					handsHit++;
				}
				continue;
			}
		}
	}

	// For every hold recorded...
	for (auto& row : m_ReplayHoldMap) {
		// For every hold on this row...
		for (auto& hrr : row.second) {
			if (hrr.subType == TapNoteSubType_Hold) {
				holdsHeld--;
				continue;
			} else if (hrr.subType == TapNoteSubType_Roll) {
				rollsHeld--;
				continue;
			}
		}
	}

	// lol just set them
	rv[RadarCategory_TapsAndHolds] = tapsHit;
	rv[RadarCategory_Jumps] = jumpsHit;
	rv[RadarCategory_Holds] = holdsHeld;
	rv[RadarCategory_Mines] = minesMissed;
	rv[RadarCategory_Hands] = handsHit;
	rv[RadarCategory_Rolls] = rollsHeld;
	rv[RadarCategory_Lifts] = liftsHit;
	rv[RadarCategory_Fakes] = fakes;
	rv[RadarCategory_Notes] = totalNotesHit;
	Locator::getLogger()->trace("Finished Calculating Radar Values from ReplayData");
}

void
PlayerAI::SetPlayerStageStatsForReplay(PlayerStageStats* pss, float ts)
{
	Locator::getLogger()->trace("Entered PSSFromReplayData function");
	// Radar values.
	// The possible radar values have already been handled, so we just do
	// the real values.
	RadarValues rrv;
	CalculateRadarValuesForReplay(rrv, pss->m_radarPossible);
	pss->m_radarActual.Zero();
	pss->m_radarActual += rrv;
	pss->everusedautoplay = true;

	// Judgments
	for (int i = TNS_Miss; i < NUM_TapNoteScore; i++) {
		pss->m_iTapNoteScores[i] = pScoreData->GetTapNoteScore((TapNoteScore)i);
	}
	for (auto i = 0; i < NUM_HoldNoteScore; i++) {
		pss->m_iHoldNoteScores[i] =
		  pScoreData->GetHoldNoteScore((HoldNoteScore)i);
	}

	// ReplayData
	pss->m_HighScore = *pScoreData;
	pss->CurWifeScore = pScoreData->GetWifeScore();
	pss->m_fWifeScore = pScoreData->GetWifeScore();
	pss->m_vHoldReplayData = pScoreData->GetHoldReplayDataVector();
	pss->m_vNoteRowVector = pScoreData->GetNoteRowVector();
	pss->m_vOffsetVector = pScoreData->GetOffsetVector();
	pss->m_vTapNoteTypeVector = pScoreData->GetTapNoteTypeVector();
	pss->m_vTrackVector = pScoreData->GetTrackVector();

	// Life record
	pss->m_fLifeRecord.clear();
	pss->m_fLifeRecord = GenerateLifeRecordForReplay(ts);
	pss->m_ComboList.clear();
	pss->m_ComboList = GenerateComboListForReplay(ts);
	Locator::getLogger()->trace("Finished PSSFromReplayData function");
}

std::pair<float, float>
PlayerAI::GetWifeScoreForRow(int row, float ts)
{
	// curwifescore, maxwifescore
	std::pair<float, float> out = { 0.f, 0.f };

	// Handle basic offset calculating and mines
	for (auto it = m_ReplayTapMap.begin();
		 it != m_ReplayTapMap.end() && it->first <= row;
		 ++it) {
		for (auto& trr : it->second) {
			if (trr.type == TapNoteType_Mine) {
				out.first += wife3_mine_hit_weight;
			} else {
				out.first += wife3(trr.offset, ts);
				out.second += 2.f;
			}
		}
	}

	// Take into account dropped holds and full misses
	auto rs = GetReplaySnapshotForNoterow(row);
	out.first += rs->judgments[TNS_Miss] * wife3_miss_weight;
	out.first += rs->hns[HNS_LetGo] * wife3_hold_drop_weight;
	out.second += rs->judgments[TNS_Miss] * 2.f;

	return out;
}

map<float, float>
PlayerAI::GenerateLifeRecordForReplay(float timingScale)
{
	Locator::getLogger()->trace("Generating LifeRecord from ReplayData");
	// Without a Snapshot Map, I assume we didn't calculate
	// the other necessary stuff and this is going to turn out bad
	if (m_ReplaySnapshotMap.empty())
		return map<float, float>({ { 0.f, 0.5f } });

	map<float, float> lifeRecord;
	auto lifeLevel = 0.5f;
	lifeRecord[0.f] = lifeLevel;
	const auto rateUsed = pScoreData->GetMusicRate();
	auto allOffset = 0.f;
	const auto firstSnapshotTime =
	  pReplayTiming->WhereUAtBro(m_ReplaySnapshotMap.begin()->first);

	auto holdIter = m_ReplayHoldMapByElapsedTime.begin();
	auto tapIter = m_ReplayTapMapByElapsedTime.begin();

	// offset everything by the first snapshot barely
	if (!m_ReplayTapMapByElapsedTime.empty())
		allOffset = firstSnapshotTime + 0.001f;

	// but if a hold messed with life before that somehow
	// offset by that instead
	// check for the offset less than the holditer because at this point
	// it is either 0 or a number greater than it should be
	// realistically this doesnt actually matter at all if we only track dropped
	// holds but im putting it here anyways
	if (!m_ReplayHoldMapByElapsedTime.empty() && holdIter->first > 0 &&
		allOffset > holdIter->first + 0.001f)
		allOffset = holdIter->first + 0.001f;

	// Continue until both iterators have finished
	while (holdIter != m_ReplayHoldMapByElapsedTime.end() ||
		   tapIter != m_ReplayTapMapByElapsedTime.end()) {
		auto now = 0.f;
		auto lifeDelta = 0.f;
		// Use tapIter for this iteration if:
		//	holdIter is finished
		//	tapIter comes first
		if (holdIter == m_ReplayHoldMapByElapsedTime.end() ||
			(tapIter != m_ReplayTapMapByElapsedTime.end() &&
			 holdIter != m_ReplayHoldMapByElapsedTime.end() &&
			 tapIter->first < holdIter->first)) {
			now = tapIter->first;
			for (auto& trr : tapIter->second) {
				TapNoteScore tns;
				if (trr.type != TapNoteType_Mine)
					tns = GetTapNoteScoreForReplay(
					  nullptr, trr.offset, timingScale);
				else
					tns = TNS_HitMine;
				lifeDelta += LifeMeterBar::MapTNSToDeltaLife(tns);
			}
			++tapIter;
		}
		// Use holdIter for this iteration if:
		//	tapIter is finished
		//	holdIter comes first
		else if (tapIter == m_ReplayTapMapByElapsedTime.end() ||
				 (holdIter != m_ReplayHoldMapByElapsedTime.end() &&
				  tapIter != m_ReplayTapMapByElapsedTime.end() &&
				  holdIter->first < tapIter->first)) {
			now = holdIter->first;
			for (auto& hrr : holdIter->second) {
				lifeDelta += LifeMeterBar::MapHNSToDeltaLife(HNS_LetGo);
			}
			++holdIter;
		} else {
			Locator::getLogger()->trace("Somehow while calculating the life graph, something "
					   "went wrong.");
			++holdIter;
			++tapIter;
		}

		lifeLevel += lifeDelta;
		CLAMP(lifeLevel, 0.f, 1.f);
		lifeRecord[(now - allOffset) / rateUsed] = lifeLevel;
	}

	Locator::getLogger()->trace("Finished Generating LifeRecord from ReplayData");
	return lifeRecord;
}

vector<PlayerStageStats::Combo_t>
PlayerAI::GenerateComboListForReplay(float timingScale)
{
	Locator::getLogger()->trace("Generating ComboList from ReplayData");
	vector<PlayerStageStats::Combo_t> combos;
	const PlayerStageStats::Combo_t firstCombo;
	const auto rateUsed = pScoreData->GetMusicRate();
	auto allOffset = 0.f;
	const auto firstSnapshotTime =
	  pReplayTiming->WhereUAtBro(m_ReplaySnapshotMap.begin()->first);
	combos.push_back(firstCombo);

	// Without a Snapshot Map, I assume we didn't calculate
	// the other necessary stuff and this is going to turn out bad
	if (m_ReplaySnapshotMap.empty())
		return combos;

	auto curCombo = &(combos[0]);
	auto rowOfComboStart = m_ReplayTapMapByElapsedTime.begin();

	// offset all entries by the offset of the first note
	// unless it's negative, then just ... dont
	if (!m_ReplayTapMapByElapsedTime.empty() && rowOfComboStart->first > 0)
		allOffset = firstSnapshotTime + 0.001f;

	// Go over all chronological tap rows (only taps should accumulate
	// combo)
	for (auto tapIter = m_ReplayTapMapByElapsedTime.begin();
		 tapIter != m_ReplayTapMapByElapsedTime.end();
		 ++tapIter) {
		auto trrv = tapIter->second;
		// Sort the vector of taps for this row
		// by their offset values so we manage them in order
		std::sort(trrv.begin(),
				  trrv.end(),
				  [](const TapReplayResult& lhs, const TapReplayResult& rhs) {
					  return lhs.offset < rhs.offset;
				  });

		// Handle the taps for this row in order
		for (const auto trr : trrv) {
			// Mines do not modify combo
			if (trr.type == TapNoteType_Mine)
				continue;

			// If CB, make a new combo
			// If not CB, increment combo
			const auto tns =
			  GetTapNoteScoreForReplay(nullptr, trr.offset, timingScale);
			if (tns == TNS_Miss || tns == TNS_W5 || tns == TNS_W4) {
				const auto start =
				  (rowOfComboStart->first - allOffset) / rateUsed;
				const auto finish = (tapIter->first - allOffset) / rateUsed;
				curCombo->m_fSizeSeconds = finish - start;
				curCombo->m_fStartSecond = start;

				PlayerStageStats::Combo_t nextcombo;
				combos.emplace_back(nextcombo);
				curCombo = &(combos.back());
				rowOfComboStart = tapIter;
			} else if (tns == TNS_W1 || tns == TNS_W2 || tns == TNS_W3) {
				curCombo->m_cnt++;
			}
		}
	}
	// The final combo may not have properly ended, end it here
	curCombo->m_fSizeSeconds =
	  (m_ReplayTapMapByElapsedTime.rbegin()->first - allOffset) / rateUsed -
	  (rowOfComboStart->first - allOffset) / rateUsed;
	curCombo->m_fStartSecond = (rowOfComboStart->first - allOffset) / rateUsed;

	Locator::getLogger()->trace("Finished Generating ComboList from ReplayData");
	return combos;
}
