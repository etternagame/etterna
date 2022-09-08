#ifndef PlayerAI_H
#define PlayerAI_H

#include "GameConstantsAndTypes.h"
#include "Etterna/Models/HighScore/HighScore.h"
#include "PlayerStageStats.h"
#include "Etterna/Actor/Gameplay/Player.h"

class NoteData;
class PlayerState;
class TimingData;

// Basically contains a record for any given noterow of the essential info about
// the Player But only the info we can simply derive from the given ReplayData
struct ReplaySnapshot
{
	// Contains Marv->Miss and Mines Hit
	int judgments[NUM_TapNoteScore] = { 0 };
	// Hold note scores
	int hns[NUM_HoldNoteScore] = { 0 };
	float curwifescore = 0.F;
	float maxwifescore = 0.F;
	float standardDeviation = 0.F;
	float mean = 0.F;
};

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

	// A map with indices for each row of the chart, pointing to a snapshot
	// of the Replay at that moment
	static std::map<int, ReplaySnapshot> m_ReplaySnapshotMap;

	// For Life/Combo graph calculations
	// A reformatting of the ExactTapMap with elapsed times as keys
	static std::map<float, std::vector<TapReplayResult>> m_ReplayTapMapByElapsedTime;
	// A reformatting of the HoldMap with elapsed times as keys
	static std::map<float, std::vector<HoldReplayResult>>
	  m_ReplayHoldMapByElapsedTime;

	static std::string oldModifiers;
	static std::string replayModifiers;
	static bool replayUsedMirror;
	static std::string oldNoteskin;
	static float replayRate;
	static float oldRate;
	static FailType oldFailType;

	// For use in Autoplay if we ever want to do funny things to the judgments
	static auto GetTapNoteScore(const PlayerState* pPlayerState)
	  -> TapNoteScore;

	// Set the pointer to a HighScore
	static void SetScoreData(HighScore* pHighScore = pScoreData,
							 int firstRow = 0,
							 NoteData* = nullptr,
							 TimingData* = nullptr);
	static void ResetScoreData();

	static auto GetTapNoteScoreForReplay(
	  float fNoteOffset,
	  float timingScale = Player::GetTimingWindowScale()) -> TapNoteScore;
	// Locate the earliest value in Seconds that is counted as a miss
	static auto FindMissWindowBegin() -> float;
	// Returns the row of the dropped hold if the given range contains a dropped
	// hold on the track Returns -1 if no dropped hold is in range.
	static auto IsHoldDroppedInRowRangeForTrack(int firstRow,
												int endRow,
												int track) -> int;
	// Reset and populate the ReplayExactTapMap.
	// This is meant to be run once Gameplay begins.
	static void SetUpExactTapMap(TimingData* timing);
	// Generates the map for every noterow in a Replay that describes what is
	// happening
	static void SetUpSnapshotMap(
	  NoteData* pNotedata,
	  std::set<int> validNoterows = std::set<int>(),
	  float timingScale = Player::GetTimingWindowScale());
	// Given a row, retrieve the Snapshot for that row.
	static auto GetReplaySnapshotForNoterow(int row)
	  -> std::shared_ptr<ReplaySnapshot>;
	// Go through the replay data to fill out the radar values for the eval
	// screen
	static void CalculateRadarValuesForReplay(RadarValues& rv,
											  RadarValues& possibleRV);

	// Fake the player stage stats using the current replay data
	static void SetPlayerStageStatsForReplay(PlayerStageStats* pss,
											 float ts = 1.F);

	// Calculate the Wifescore for the given position in replay data
	static auto GetWifeScoreForRow(int row, float ts)
	  -> std::pair<float, float>;

	// Given the Replay Data and Snapshot map, we can make a simple estimated
	// life graph.
	static auto GenerateLifeRecordForReplay(
	  float timingScale = Player::GetTimingWindowScale())
	  -> std::map<float, float>;

	// Given the Replay Data and Snapshot map, we can make a simple estimate
	// combo graph.
	static auto GenerateComboListForReplay(
	  float timingScale = Player::GetTimingWindowScale())
	  -> std::vector<PlayerStageStats::Combo_t>;
};

#endif
