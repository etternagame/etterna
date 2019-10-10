#ifndef PlayerAI_H
#define PlayerAI_H

#include "GameConstantsAndTypes.h"
#include "HighScore.h"

class NoteData;
class PlayerStageStats;
class PlayerState;
class TimingData;

const int NUM_SKILL_LEVELS = 6; // 0-5

// Basically contains a record for any given noterow of the essential info about
// the Player But only the info we can simply derive from the given ReplayData
struct ReplaySnapshot
{
	// Contains Marv->Miss and Mines Hit
	int judgments[NUM_TapNoteScore] = { 0 };
	// Hold note scores
	int hns[NUM_HoldNoteScore] = { 0 };
	float curwifescore = 0.f;
	float maxwifescore = 0.f;
};

// also known as ReplayManager
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
	static map<int, vector<TapReplayResult>> m_ReplayTapMap;
	// A map with indices for each row of the chart, pointing to nothing or hold
	// drop results.
	static map<int, vector<HoldReplayResult>> m_ReplayHoldMap;
	// A map with indices for each row of the chart, pointing to nothing or a
	// Normal Result. However, note that the rows within are actually calculated
	// so that they are adjusted for offsets relative to the actual replay
	// data/notedata. This map is only useful for charts with column data.
	static map<int, vector<TapReplayResult>> m_ReplayExactTapMap;

	// A map with indices for each row of the chart, pointing to a snapshot
	// of the Replay at that moment
	static map<int, ReplaySnapshot> m_ReplaySnapshotMap;

	// For use in Autoplay if we ever want to do funny things to the judgments
	static TapNoteScore GetTapNoteScore(const PlayerState* pPlayerState);

	// Set the pointer to a HighScore
	static void SetScoreData(HighScore* pHighScore = pScoreData,
							 int firstRow = 0,
							 NoteData* = nullptr);
	static void ResetScoreData();

	static float GetTapNoteOffsetForReplay(TapNote* pTN, int noteRow, int col);
	static TapNoteScore GetTapNoteScoreForReplay(
	  const PlayerState* pPlayerState,
	  float fNoteOffset);
	static bool DetermineIfHoldDropped(int noteRow, int col);
	// Returns true or false if the given range contains a dropped hold on the
	// track
	static bool IsHoldDroppedInRowRangeForTrack(int firstRow,
												int endRow,
												int track);
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
	static vector<TapReplayResult> GetTapsToTapForRow(int noteRow);
	static int GetReplayType();
	// Build a list of columns/tracks that happened at or before the given
	// noterow. (if we lag and somehow skip rows)
	static vector<TapReplayResult> GetTapsAtOrBeforeRow(int noteRow);
	// Given a column and row, retrieve the adjusted row.
	static int GetAdjustedRowFromUnadjustedCoordinates(int row, int col);
	// Given a row, retrieve the Snapshot for that row.
	static ReplaySnapshot GetReplaySnapshotForNoterow(int row);
	// Remove a given Tap from the fallback and Full replay data vectors
	static void RemoveTapFromVectors(int row, int col);
	// Go through the replay data to fill out the radar values for the eval
	// screen
	static void CalculateRadarValuesForReplay(RadarValues& rv,
											  RadarValues& possibleRV);
	// Find a tap at the given row and column
	static bool IsTapAtRowAndColumn(int noteRow, int col);

	// Fake the player stage stats using the current replay data
	static void SetPlayerStageStatsForReplay(PlayerStageStats* pss);

	// Calculate the Wifescore for the given position in replay data
	static pair<float, float> GetWifeScoreForRow(int row, float ts);
};

#endif
