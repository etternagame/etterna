#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/FileTypes/IniFile.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "PlayerAI.h"
#include "PlayerState.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RadarValues.h"

HighScore* PlayerAI::pScoreData = nullptr;
TimingData* PlayerAI::pReplayTiming = nullptr;
map<int, vector<TapReplayResult>> PlayerAI::m_ReplayTapMap;
map<int, vector<HoldReplayResult>> PlayerAI::m_ReplayHoldMap;
map<int, vector<TapReplayResult>> PlayerAI::m_ReplayExactTapMap;

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
}

void
PlayerAI::SetScoreData(HighScore* pHighScore)
{
	bool successful = pHighScore->LoadReplayData();
	pScoreData = pHighScore;
	m_ReplayTapMap.clear();
	m_ReplayHoldMap.clear();
	m_ReplayExactTapMap.clear();

	if (!successful) {
		return;
	}

	auto replayNoteRowVector = pHighScore->GetCopyOfNoteRowVector();
	auto replayOffsetVector = pHighScore->GetCopyOfOffsetVector();
	auto replayTapNoteTypeVector = pHighScore->GetCopyOfTapNoteTypeVector();
	auto replayTrackVector = pHighScore->GetCopyOfTrackVector();
	auto replayHoldVector = pHighScore->GetCopyOfHoldReplayDataVector();

	// Generate TapReplayResults to put into a vector referenced by the song row
	// in a map
	for (size_t i = 0; i < replayNoteRowVector.size(); i++) {
		if (fabsf(replayOffsetVector[i]) > 0.18f)
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
		}
	}

	// Generate vectors made of pregenerated HoldReplayResults referenced by the
	// song row in a map
	for (size_t i = 0; i < replayHoldVector.size(); i++) {
		// Create or append to the vector
		if (m_ReplayHoldMap.count(replayHoldVector[i].row) != 0) {
			m_ReplayHoldMap[replayHoldVector[i].row].push_back(
			  replayHoldVector[i]);
		} else {
			vector<HoldReplayResult> hrrVector = { replayHoldVector[i] };
			m_ReplayHoldMap[replayHoldVector[i].row] = hrrVector;
		}
	}
}

void
PlayerAI::SetUpExactTapMap(TimingData* timing)
{
	// Don't continue if the replay doesn't have column data.
	// We can't be accurate without it.
	if (pScoreData->GetReplayType() != 2)
		return;

	m_ReplayExactTapMap.clear();

	pReplayTiming = timing;

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

	// Current v0.60 Replay Data format: [noterow] [offset] [track] [optional:
	// tap note type] Current v0.60 Replay Data format (H section): H [noterow]
	// [track] [optional: tap note subtype]
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
