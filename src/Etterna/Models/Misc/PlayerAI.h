/* PlayerAI - Chooses which notes the AI steps on. */

#ifndef PlayerAI_H
#define PlayerAI_H

#include "GameConstantsAndTypes.h"
#include "HighScore.h"

class PlayerState;

const int NUM_SKILL_LEVELS = 6; // 0-5

class PlayerAI
{
  public:
	// Pointer to real high score data for a replay
	static HighScore* pScoreData;

	// Pointer to real timing data for a replay
	static TimingData* pReplayTiming;

	// Pulled from pScoreData on initialization

	// A map with indices for each row of the chart, pointing to nothing or a
	// Normal Result
	static std::map<int, std::vector<TapReplayResult>> m_ReplayTapMap;
	// A map with indices for each row of the chart, pointing to nothing or hold
	// drop results.
	static std::map<int, std::vector<HoldReplayResult>> m_ReplayHoldMap;
	// A map with indices for each row of the chart, pointing to nothing or a
	// Normal Result. However, note that the rows within are actually calculated
	// so that they are adjusted for offsets relative to the actual replay
	// data/notedata. This map is only useful for charts with column data.
	static std::map<int, std::vector<TapReplayResult>> m_ReplayExactTapMap;

	static void InitFromDisk();
	static TapNoteScore GetTapNoteScore(const PlayerState* pPlayerState);
	static void SetScoreData(HighScore* pHighScore);

	static float GetTapNoteOffsetForReplay(TapNote* pTN, int noteRow, int col);
	static TapNoteScore GetTapNoteScoreForReplay(
	  const PlayerState* pPlayerState,
	  float fNoteOffset);
	static bool DetermineIfHoldDropped(int noteRow, int col);
	// Returns the column that needs to be tapped.
	// Returns -1 if no column needs to be tapped.
	static int DetermineNextTapColumn(int noteRow,
									  int searchRowDistance,
									  TimingData* timing);
	// Literally get the next row in the replay data. Disregard offset
	// calculations.
	static int GetNextRowNoOffsets(int currentRow);
	// Reset and populate the ReplayExactTapMap.
	// This is meant to be run once Gameplay begins.
	static void SetUpExactTapMap(TimingData* timing);
	// Check the Tap Replay Data to see if a tap is on this row
	static bool TapExistsAtThisRow(int noteRow);
	static bool TapExistsAtOrBeforeThisRow(int noteRow);
	// Build a list of columns/tracks to tap based on the given noterow.
	static std::vector<TapReplayResult> GetTapsToTapForRow(int noteRow);
	static int GetReplayType();
	// Build a list of columns/tracks that happened at or before the given
	// noterow. (if we lag and somehow skip rows)
	static std::vector<TapReplayResult> GetTapsAtOrBeforeRow(int noteRow);
	// Given a column and row, retrieve the adjusted row.
	static int GetAdjustedRowFromUnadjustedCoordinates(int row, int col);
	// Remove a given Tap from the fallback and Full replay data vectors
	static void RemoveTapFromVectors(int row, int col);
	// Go through the replay data to fill out the radar values for the eval
	// screen
	static void CalculateRadarValuesForReplay(RadarValues& rv,
											  RadarValues& possibleRV);
};

#endif

/*
 * (c) 2003-2004 Chris Danford
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
