#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/FileTypes/IniFile.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "PlayerAI.h"
#include "PlayerState.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RadarValues.h"
#include "PlayerStageStats.h"
#include "Etterna/Actor/Gameplay/LifeMeterBar.h"

HighScore* PlayerAI::pScoreData = nullptr;
TimingData* PlayerAI::pReplayTiming = nullptr;
map<int, vector<TapReplayResult>> PlayerAI::m_ReplayTapMap;
map<int, vector<HoldReplayResult>> PlayerAI::m_ReplayHoldMap;
map<int, vector<TapReplayResult>> PlayerAI::m_ReplayExactTapMap;
map<int, ReplaySnapshot> PlayerAI::m_ReplaySnapshotMap;
float PlayerAI::replayRate = 1.f;
RString PlayerAI::replayModifiers = "";
RString PlayerAI::oldModifiers = "";
float PlayerAI::oldRate = 1.f;
RString PlayerAI::oldNoteskin = "";

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
								   float fNoteOffset)
{
	// This code is basically a copy paste from somewhere in Player for grabbing
	// scores.

	// LOG->Trace("Given number %f ", fNoteOffset);
	const float fSecondsFromExact = fabsf(fNoteOffset);
	// LOG->Trace("TapNoteScore For Replay Seconds From Exact: %f",
	// fSecondsFromExact);

	if (fSecondsFromExact >= 1.f)
		return TNS_Miss;

	if (fSecondsFromExact <= Player::GetWindowSeconds(TW_W1))
		return TNS_W1;
	else if (fSecondsFromExact <= Player::GetWindowSeconds(TW_W2))
		return TNS_W2;
	else if (fSecondsFromExact <= Player::GetWindowSeconds(TW_W3))
		return TNS_W3;
	else if (fSecondsFromExact <= Player::GetWindowSeconds(TW_W4))
		return TNS_W4;
	else if (fSecondsFromExact <= max(Player::GetWindowSeconds(TW_W5), 0.18f))
		return TNS_W5;
	return TNS_None;
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
	replayRate = 1.f;
	replayModifiers.clear();
	oldModifiers.clear();
	oldRate = 1.f;
	oldNoteskin.clear();
}

void
PlayerAI::SetScoreData(HighScore* pHighScore, int firstRow, NoteData* pNoteData)
{
	bool successful = pHighScore->LoadReplayData();
	pScoreData = pHighScore;
	m_ReplayTapMap.clear();
	m_ReplayHoldMap.clear();
	m_ReplayExactTapMap.clear();
	// we dont clear snapshot map here for a particular reason

	if (!successful) {
		return;
	}

	if (pHighScore != nullptr && pNoteData != nullptr)
		m_ReplaySnapshotMap.clear();

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
			vector<TapReplayResult> trrVector = { trr };
			m_ReplayTapMap[replayNoteRowVector[i]] = trrVector;
			validNoterows.insert(replayNoteRowVector[i]);
		}
	}

	// Generate vectors made of pregenerated HoldReplayResults referenced by the
	// song row in a map
	// Only present in replays with column data.
	for (size_t i = 0; i < replayHoldVector.size(); i++) {
		if (replayHoldVector[i].row < firstRow)
			continue;

		// Create or append to the vector
		if (m_ReplayHoldMap.count(replayHoldVector[i].row) != 0) {
			m_ReplayHoldMap[replayHoldVector[i].row].push_back(
			  replayHoldVector[i]);
		} else {
			vector<HoldReplayResult> hrrVector = { replayHoldVector[i] };
			m_ReplayHoldMap[replayHoldVector[i].row] = hrrVector;
			validNoterows.insert(replayHoldVector[i].row);
		}
	}

	// Misses don't always show up in Replay Data properly.
	// We require the NoteData to validate the Judge count.
	// If we don't have it, don't care.
	if (pNoteData == nullptr)
		return;

	// Set up a mapping of every noterow to a snapshot of what has happened up
	// to that point
	SetUpSnapshotMap(pNoteData, validNoterows);
}

void
PlayerAI::SetUpExactTapMap(TimingData* timing)
{
	pReplayTiming = timing;

	// Don't continue if the replay doesn't have column data.
	// We can't be accurate without it.
	if (pScoreData->GetReplayType() != 2)
		return;

	m_ReplayExactTapMap.clear();

	// For every row in the replay data...
	for (auto& row : m_ReplayTapMap) {
		// Get the current time and go over all taps on this row...
		float rowTime = timing->WhereUAtBro(row.first);
		for (TapReplayResult& trr : row.second) {
			// Find the time adjusted for offset
			// Then the beat according to that time
			// Then the noterow according to that beat
			float tapTime = rowTime + trr.offset;
			float tapBeat = timing->GetBeatFromElapsedTime(tapTime);
			int tapRow = BeatToNoteRow(tapBeat);
			trr.offsetAdjustedRow = tapRow;

			// And put that into the exacttapmap :)
			if (m_ReplayExactTapMap.count(tapRow) != 0) {
				m_ReplayExactTapMap[tapRow].push_back(trr);
			} else {
				vector<TapReplayResult> trrVector = { trr };
				m_ReplayExactTapMap[tapRow] = trrVector;
			}
		}
	}
}

void
PlayerAI::SetUpSnapshotMap(NoteData* pNoteData, set<int> validNoterows)
{
	m_ReplaySnapshotMap.clear();

	// Fill out the Snapshot map now that the other maps are not so out of order
	// We leave out misses in this section because they aren't in the Replay
	// Data
	int tempJudgments[NUM_TapNoteScore] = { 0 };
	int tempHNS[NUM_HoldNoteScore] = { 0 };

	// Copy of the main hold map
	// Delete the values as we go on to make sure they get counted
	map<int, vector<HoldReplayResult>> m_unjudgedholds(m_ReplayHoldMap);

	// If we don't have validnoterows, just do it the hard way
	if (validNoterows.empty()) {
		for (auto row : m_ReplayTapMap) {
			for (auto instance = row.second.begin();
				 instance != row.second.end();
				 instance++) {
				TapReplayResult& trr = *instance;
				if (trr.type == TapNoteType_Mine) {
					tempJudgments[TNS_HitMine]++;
				} else {
					TapNoteScore tns =
					  GetTapNoteScoreForReplay(nullptr, trr.offset);
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
		for (auto r = validNoterows.begin(); r != validNoterows.end(); r++) {
			// Check for taps and mines
			if (m_ReplayTapMap.count(*r) != 0) {
				for (auto instance = m_ReplayTapMap[*r].begin();
					 instance != m_ReplayTapMap[*r].end();
					 instance++) {
					TapReplayResult& trr = *instance;
					if (trr.type == TapNoteType_Mine) {
						tempJudgments[TNS_HitMine]++;
					} else {
						TapNoteScore tns =
						  GetTapNoteScoreForReplay(nullptr, trr.offset);
						tempJudgments[tns]++;
					}
				}
			}

			// Make the struct and cram it in there
			// ignore holds for now since the following block takes care of them
			ReplaySnapshot rs;
			FOREACH_ENUM(TapNoteScore, tns)
			rs.judgments[tns] = tempJudgments[tns];
			m_ReplaySnapshotMap[*r] = rs;
		}
	}

	// Now handle misses and holds.
	// For every row in notedata...
	FOREACH_NONEMPTY_ROW_ALL_TRACKS(*pNoteData, row)
	{
		// For every track in the row...
		for (int track = 0; track < pNoteData->GetNumTracks(); track++) {
			// Find the tapnote we are on
			TapNote* pTN = NULL;
			NoteData::iterator iter = pNoteData->FindTapNote(track, row);
			DEBUG_ASSERT(iter != pNoteData->end(track));
			pTN = &iter->second;

			if (iter != pNoteData->end(track)) {
				// Deal with holds here
				if (pTN->type == TapNoteType_HoldHead) {
					int isDropped = IsHoldDroppedInRowRangeForTrack(
					  row, row + pTN->iDuration, track);
					if (isDropped != -1) {
						tempHNS[HNS_LetGo]++;

						// erase the hold from the mapping of unjudged holds
						for (int i = 0; i < m_unjudgedholds[isDropped].size();
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
					for (int i = 0; i < hrrs.size(); i++) {
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
						bool found = false;
						for (auto& trr : m_ReplayTapMap[row]) {
							if (trr.track == track)
								found = true;
						}
						if (!found) {
							tempJudgments[TNS_Miss]++;
						}
					} else {
						tempJudgments[TNS_Miss]++;
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
			for (int track = 0; track < pNoteData->GetNumTracks(); track++) {
				NoteData::iterator iter = pNoteData->FindTapNote(track, row);
				if (iter != pNoteData->end(track)) {
					TapNote* pTN = &iter->second;
					if (pTN->type == TapNoteType_Fake ||
						pTN->type == TapNoteType_Mine ||
						pTN->type == TapNoteType_AutoKeysound)
						continue;
					notesOnRow++;
				}
			}
			tempJudgments[TNS_Miss] += (notesOnRow - notesInReplayData);
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
			if (m_ReplaySnapshotMap.rbegin()->first < row) {
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
				rs.judgments[TNS_Miss]++;
				m_ReplaySnapshotMap[row] = rs;
			} else // If the current row is in between recorded rows, copy an
				   // older one
			{
				ReplaySnapshot rs;
				auto prev = m_ReplaySnapshotMap.lower_bound(row);
				prev--;
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
	const float ts = Player::GetTimingWindowScale();
	// some snapshots end up with 0 values due to being "missing" from the
	// replay data and we have to account for those
	vector<int> snapShotsUnused;
	for (auto it = m_ReplaySnapshotMap.begin(); it != m_ReplaySnapshotMap.end();
		 it++)
		snapShotsUnused.push_back(it->first);
	float cws = 0.f;
	float mws = 0.f;
	for (auto it = m_ReplayTapMap.begin(); it != m_ReplayTapMap.end();) {
		const int r = it->first;
		if (r > snapShotsUnused.front()) {
			// if we somehow skipped a snapshot, the only difference should be
			// in misses and non taps
			ReplaySnapshot* rs = &m_ReplaySnapshotMap[snapShotsUnused.front()];
			rs->curwifescore = cws - (rs->judgments[TNS_Miss] * 8.f) -
							   (rs->hns[HNS_LetGo] * 8.f);
			rs->maxwifescore = mws + (rs->judgments[TNS_Miss] * 2.f);
			snapShotsUnused.erase(snapShotsUnused.begin());

			continue; // retry the iteration (it++ is moved to below)
		}
		ReplaySnapshot* rs = GetReplaySnapshotForNoterow(r);
		for (auto& trr : it->second) {
			if (trr.type == TapNoteType_Mine) {
				cws -= 8.f;
			} else {
				cws += wife2(trr.offset, ts);
				mws += 2.f;
			}
		}
		rs->curwifescore =
		  cws - (rs->judgments[TNS_Miss] * 8.f) - (rs->hns[HNS_LetGo] * 8.f);
		rs->maxwifescore = mws + (rs->judgments[TNS_Miss] * 2.f);

		snapShotsUnused.erase(snapShotsUnused.begin());
		it++;
	}
	if (!snapShotsUnused.empty()) {
		for (int row : snapShotsUnused) {
			// This might not be technically correct
			// But my logic is this:
			// A snapshot without associated replaydata is one for a row
			// which has no stat-affecting changes made to it.
			// So this applies to rows with all Mines
			// or rows with all Fakes (in the latest version)
			ReplaySnapshot* prevrs = GetReplaySnapshotForNoterow(row - 1);
			ReplaySnapshot* rs = &m_ReplaySnapshotMap[row];
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
		for (int i = 0; i < (int)m_ReplayTapMap[row].size(); i++) {
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
		for (int i = 0; i < (int)m_ReplayExactTapMap[row].size(); i++) {
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
	int output = -1;

	if (m_ReplayTapMap.count(row) != 0) {
		for (TapReplayResult& trr : m_ReplayTapMap[row]) {
			if (trr.track == col)
				output = trr.offsetAdjustedRow;
		}
	}
	return output;
}

ReplaySnapshot*
PlayerAI::GetReplaySnapshotForNoterow(int row)
{
	// The row doesn't necessarily have to exist in the Snapshot map.
	// Because after a Snapshot, we can try this again for a later row
	// And if there are no new snapshots (no events) nothing changes
	// So we return that earlier snapshot.

	// If the lowest value in the map is above the given row, return an empty
	// snapshot
	if (m_ReplaySnapshotMap.begin()->first > row) {
		ReplaySnapshot rs;
		return &rs;
	}

	// For some reason I don't feel like figuring out, if the largest value in
	// the map is below the given row, it returns 0 So we need to return the
	// last value
	if (m_ReplaySnapshotMap.rbegin()->first < row) {
		return &m_ReplaySnapshotMap.rbegin()->second;
	}

	// Otherwise just go ahead and return what we want
	auto lb = m_ReplaySnapshotMap.lower_bound(row);
	int foundRow = lb->first;

	// lower_bound gets an iter to the next element >= the given key
	// and we have to decrement if it is greater than the key (because we want
	// that)
	if (foundRow > row) {
		if (lb != m_ReplaySnapshotMap.begin()) {
			lb--;
			foundRow = lb->first;
		} else {
			ReplaySnapshot rs;
			return &rs;
		}
	}
	return &m_ReplaySnapshotMap[foundRow];
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
			 hiter++) {
			// If this row is before the start, skip it
			if (hiter->first < firstRow)
				continue;
			// If this row is after the end, skip it
			else if (hiter->first > endRow)
				return -1;
			// This row might work. Check what tracks might have dropped.
			for (auto hrr : hiter->second) {
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
		auto rowIt = m_ReplayExactTapMap.lower_bound(-20000);
		int row = rowIt->first;
		for (; row <= noteRow && row != -20000;) {
			vector<TapReplayResult> toMerge = GetTapsToTapForRow(row);
			output.insert(output.end(), toMerge.begin(), toMerge.end());
			row = GetNextRowNoOffsets(row);
		}
	} else {
		auto rowIt = m_ReplayTapMap.lower_bound(-20000);
		int row = rowIt->first;
		for (; row <= noteRow && row != -20000;) {
			vector<TapReplayResult> toMerge = GetTapsToTapForRow(row);
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
		auto thing = m_ReplayExactTapMap.lower_bound(currentRow + 1);

		if (thing == m_ReplayExactTapMap.end()) {
			return -20000;
		} else {
			return thing->first;
		}
	} else {
		auto thing = m_ReplayTapMap.lower_bound(currentRow + 1);

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
	/* Given the pTN coming from gameplay, we search for the matching note in
	the replay data. If it is not found, it is a miss. (1.f)
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

		float offset = m_ReplayTapMap[noteRow].back().offset;

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
			for (int i = 0; i < (int)m_ReplayExactTapMap[noteRow].size();
				 i++) // go over all elements in the row
			{
				auto trr = m_ReplayExactTapMap[noteRow][i];
				if (trr.track ==
					col) // if the column expected is the actual note, use it
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
	// We will do this thoroughly just in case someone decides to use the other
	// categories we don't currently use
	int tapsHit = 0;
	int jumpsHit = 0;
	int handsHit = 0;
	int holdsHeld = possibleRV[RadarCategory_Holds];
	int rollsHeld = possibleRV[RadarCategory_Rolls];
	int liftsHit = 0;
	int fakes = possibleRV[RadarCategory_Fakes];
	int totalNotesHit = 0;
	int minesMissed = possibleRV[RadarCategory_Mines];

	// For every row recorded...
	for (auto& row : m_ReplayTapMap) {
		int tapsOnThisRow = 0;
		// For every tap on these rows...
		for (TapReplayResult& trr : row.second) {
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
		for (HoldReplayResult& hrr : row.second) {
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
}

void
PlayerAI::SetPlayerStageStatsForReplay(PlayerStageStats* pss)
{
	// Radar values.
	// The possible radar values have already been handled, so we just do the
	// real values.
	RadarValues rrv;
	CalculateRadarValuesForReplay(rrv, pss->m_radarPossible);
	pss->m_radarActual.Zero();
	pss->m_radarActual += rrv;
	pss->everusedautoplay = true;

	// Judgments
	for (int i = TNS_Miss; i < NUM_TapNoteScore; i++) {
		pss->m_iTapNoteScores[i] = pScoreData->GetTapNoteScore((TapNoteScore)i);
	}
	for (int i = 0; i < NUM_HoldNoteScore; i++) {
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
	pss->m_fLifeRecord = GenerateLifeRecordForReplay();
	pss->m_ComboList.clear();
	pss->m_ComboList = GenerateComboListForReplay();
}

pair<float, float>
PlayerAI::GetWifeScoreForRow(int row, float ts)
{
	// curwifescore, maxwifescore
	pair<float, float> out = { 0.f, 0.f };

	// Handle basic offset calculating and mines
	for (auto it = m_ReplayTapMap.begin();
		 it != m_ReplayTapMap.end() && it->first <= row;
		 it++) {
		for (auto& trr : it->second) {
			if (trr.type == TapNoteType_Mine) {
				out.first -= 8.f;
			} else {
				out.first += wife2(trr.offset, ts);
				out.second += 2.f;
			}
		}
	}

	// Take into account dropped holds and full misses
	ReplaySnapshot* rs = GetReplaySnapshotForNoterow(row);
	out.first += rs->judgments[TNS_Miss] * -8.f;
	out.first += rs->hns[HNS_LetGo] * -8.f;
	out.second += rs->judgments[TNS_Miss] * 2.f;

	return out;
}

map<float, float>
PlayerAI::GenerateLifeRecordForReplay()
{
	// I don't want to do this without a snapshot map
	if (m_ReplaySnapshotMap.empty())
		return map<float, float>();

	map<float, float> lifeRecord;
	auto currentSnap = m_ReplaySnapshotMap.begin();
	currentSnap++;
	auto previousSnap = m_ReplaySnapshotMap.begin();
	float lifeLevel = 0.5f;
	const float rateUsed = pScoreData->GetMusicRate();

	lifeRecord[0.f] = lifeLevel;

	while (currentSnap != m_ReplaySnapshotMap.end()) {
		float now = pReplayTiming->WhereUAtBro(currentSnap->first) / rateUsed;
		ReplaySnapshot* cSnap = &currentSnap->second;
		ReplaySnapshot* pSnap = &previousSnap->second;
		// these arrays are not unsigned so that if they somehow end up negative
		// we dont sit here for a millenia waiting for it to finish
		int differencesTNS[NUM_TapNoteScore] = { 0 };
		int differencesHNS[NUM_HoldNoteScore] = { 0 };
		float lifeDelta = 0.f;

		FOREACH_ENUM(TapNoteScore, tns)
		differencesTNS[tns] = cSnap->judgments[tns] - pSnap->judgments[tns];

		FOREACH_ENUM(HoldNoteScore, hns)
		differencesHNS[hns] = cSnap->hns[hns] - pSnap->hns[hns];

		FOREACH_ENUM(TapNoteScore, tns)
		for (int i = 0; i < differencesTNS[tns]; i++)
			lifeDelta += LifeMeterBar::MapTNSToDeltaLife(tns);

		FOREACH_ENUM(HoldNoteScore, hns)
		for (int i = 0; i < differencesHNS[hns]; i++)
			lifeDelta += LifeMeterBar::MapHNSToDeltaLife(hns);

		lifeLevel += lifeDelta;
		CLAMP(lifeLevel, 0.f, 1.f);
		lifeRecord[now] = lifeLevel;
		currentSnap++;
		previousSnap++;
	}

	return lifeRecord;
}

vector<PlayerStageStats::Combo_t>
PlayerAI::GenerateComboListForReplay()
{
	// I don't want to do this without a snapshot map
	if (m_ReplaySnapshotMap.empty())
		return vector<PlayerStageStats::Combo_t>();

	vector<PlayerStageStats::Combo_t> combos;
	PlayerStageStats::Combo_t firstCombo;
	const float rateUsed = pScoreData->GetMusicRate();
	int taps = 0;

	combos.push_back(firstCombo);
	PlayerStageStats::Combo_t* curCombo = &combos[0];
	auto currentSnap = m_ReplaySnapshotMap.begin();
	currentSnap++;
	auto snapOfComboStart = m_ReplaySnapshotMap.begin();

	// Continue through all snapshots looking for combos
	while (currentSnap != m_ReplaySnapshotMap.end()) {
		ReplaySnapshot* cSnap = &currentSnap->second;
		ReplaySnapshot* sSnap = &snapOfComboStart->second;

		// If we have a combo breaker, count the combo and start a new one
		// (hardcoded combo breaker judgments because this is the true path)
		if (cSnap->judgments[TNS_W4] > sSnap->judgments[TNS_W4] ||
			cSnap->judgments[TNS_W5] > sSnap->judgments[TNS_W5] ||
			cSnap->judgments[TNS_Miss] > sSnap->judgments[TNS_Miss]) {
			taps += (cSnap->judgments[TNS_W1] - sSnap->judgments[TNS_W1]);
			taps += (cSnap->judgments[TNS_W2] - sSnap->judgments[TNS_W2]);
			taps += (cSnap->judgments[TNS_W3] - sSnap->judgments[TNS_W3]);
			curCombo->m_cnt = taps;
			float starttime =
			  pReplayTiming->WhereUAtBro(snapOfComboStart->first) / rateUsed;
			float finishtime =
			  pReplayTiming->WhereUAtBro(currentSnap->first) / rateUsed;
			curCombo->m_fSizeSeconds = finishtime - starttime;
			curCombo->m_fStartSecond = starttime;

			// Start a new combo
			PlayerStageStats::Combo_t nextcombo;
			combos.emplace_back(nextcombo);
			curCombo = &(combos.back());
			snapOfComboStart = currentSnap;
			currentSnap++;
			taps = 0;
		} else {
			currentSnap++;
		}
	}
	// The last combo probably didn't end on a combo breaker so
	// calculate it real quick
	ReplaySnapshot workingSnap = m_ReplaySnapshotMap.rbegin()->second;
	taps += (workingSnap.judgments[TNS_W1] -
			 snapOfComboStart->second.judgments[TNS_W1]);
	taps += (workingSnap.judgments[TNS_W2] -
			 snapOfComboStart->second.judgments[TNS_W2]);
	taps += (workingSnap.judgments[TNS_W3] -
			 snapOfComboStart->second.judgments[TNS_W3]);
	curCombo->m_cnt = taps;
	float starttime =
	  pReplayTiming->WhereUAtBro(snapOfComboStart->first) / rateUsed;
	float finishtime =
	  pReplayTiming->WhereUAtBro(m_ReplaySnapshotMap.rbegin()->first) /
	  rateUsed;
	curCombo->m_fSizeSeconds = finishtime - starttime;
	curCombo->m_fStartSecond = starttime;

	return combos;
}
