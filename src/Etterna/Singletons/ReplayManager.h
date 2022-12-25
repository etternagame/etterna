#pragma once
#ifndef REPLAYMAN
#define REPLAYMAN

#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Models/HighScore/Replay.h"
#include "Etterna/Models/Misc/PlayerStageStats.h"

#include <unordered_map>

/// Used to track settings used by a player previously
/// as well as the settings they should change to
/// to emulate a replay.
/// This lets you emulate a replay and then back out of that.
struct TemporaryReplaySettings
{
	bool replayUsedMirror = false;
	float replayRate = 1.F;
	std::string replayModifiers{};
	std::string oldModifiers{};
	float oldRate = 1.F;
	std::string oldNoteskin{};
	FailType oldFailType = FailType_Immediate;
	int replayRngSeed = 0;
	int oldRngSeed = 0;

	void reset()
	{
		replayRate = 1.F;
		replayModifiers = std::string();
		replayUsedMirror = false;
		replayRngSeed = 0;
		oldModifiers = std::string();
		oldRate = 1.F;
		oldNoteskin = std::string();
		oldFailType = FailType_Immediate;
		oldRngSeed = 0;
	}
};

struct HighScore;
class ReplayManager
{
  public:
	ReplayManager();
	~ReplayManager();

	/// Return the Replay for this HighScore. Never null
	Replay* GetReplay(HighScore* hs);
	void ReleaseReplay(Replay* replay);

	/// The use of this is for scores which are about to viewed
	/// via eval or ingame replay.
	/// It runs all processing on the replay.
	Replay* InitReplayPlaybackForScore(HighScore* hs,
									   float timingScale = 1.F,
									   int startRow = 0);

	void UnsetActiveReplay();

	/// Returns the Replay being watched by the player. Never null
	Replay* GetActiveReplay();

	HighScore* GetActiveReplayScore();

	void StoreActiveReplaySettings(float replayRate,
								   std::string& replayModifiers,
								   bool replayUsedMirror,
								   int replayRngSeed);

	void StoreOldSettings(float oldRate,
						  std::string& oldModifiers,
						  FailType oldFailType,
						  std::string& oldNoteskin,
						  int oldRngSeed);

	TemporaryReplaySettings GetActiveReplaySettings();
	void ResetActiveReplaySettings();

	static TapNoteScore GetTapNoteScoreForReplay(float fNoteOffset,
												 float timingScale = 1.F);

	auto CalculateRadarValuesForReplay(Replay& replay,
									   RadarValues& rv,
									   RadarValues& possibleRV) -> bool;

	auto RescoreReplay(Replay& replay,
									  PlayerStageStats* pss,
									  float timingScale = 1.F) -> bool;

	auto GenerateLifeRecordForReplay(Replay& replay, float timingScale = 1.F)
	  -> std::map<float, float>;

	auto GenerateComboListForReplay(Replay& replay, float timingScale = 1.F)
	  -> std::vector<PlayerStageStats::Combo_t>;

	void EnableCustomScoringFunctions() {
		customScoringFunctionsEnabled = true;
	}
	void DisableCustomScoringFunctions() {
		customScoringFunctionsEnabled = false;
	}
	bool isCustomScoringFunctionsEnabled() {
		return customScoringFunctionsEnabled;
	}

	////////
	// custom scoring functions
	// these will default to base game behavior if the custom functions arent set

	/// How much a particular judgment or offset is worth
	/// lua function params: tap offset, tapnotescore (str), timingScale (1.0 for j4)
	/// return: float
	auto CustomTapScoringFunction(float fOffsetSeconds,
								  TapNoteScore tns,
								  float timingScale) -> float;
	/// How much a hold note score is worth (a dropped hold)
	/// lua function params: holdnotescore (str)
	/// return: float
	auto CustomHoldNoteScoreScoringFunction(HoldNoteScore hns) -> float;
	/// How much a mine hit is worth
	/// lua function params: none
	/// return: float
	auto CustomMineScoringFunction() -> float;
	/// How much each note is worth at maximum
	/// lua function params: tapnotetype (str)
	/// return: float
	auto CustomTotalWifePointsCalculation(TapNoteType tnt) -> float;
	/// The TapNoteScore awarded to a particular offset
	/// lua function params: offset (in seconds), timing
	/// return: tapnotescore (str)
	auto CustomOffsetJudgingFunction(float fOffsetSeconds, float timingScale)
	  -> TapNoteScore;
	/// The time in seconds at which a note is not judged.
	/// If a note is further than this distance from a tap, it is not judged.
	/// If for some reason an assigned offset is outside this window, it is a miss.
	auto CustomMissWindowFunction() -> float;

	void SetTotalWifePointsCalcFunction(const LuaReference& ref) {
		m_totalWifePointsCalcFunc = ref;
	}
	void SetMineScoringFunction(const LuaReference& ref) {
		m_mineScoringFunc = ref;
	}
	void SetHoldNoteScoringFunction(const LuaReference& ref) {
		m_holdNoteScoreScoringFunc = ref;
	}
	void SetTapScoringFunction(const LuaReference& ref) {
		m_tapScoringFunc = ref;
	}
	void SetOffsetJudgingFunction(const LuaReference& ref) {
		m_offsetJudgingFunc = ref;
	}
	void SetMissWindowFunction(const LuaReference& ref) {
		m_missWindowFunc = ref;
	}
	//
	/////////

	/// Lua
	void PushSelf(lua_State* L);

  private:
	/// scorekey to {refcount, pointer}
	std::unordered_map<std::string, std::pair<unsigned, Replay*>>
	  scoresToReplays{};

	LuaReference m_totalWifePointsCalcFunc;
	LuaReference m_mineScoringFunc;
	LuaReference m_holdNoteScoreScoringFunc;
	LuaReference m_tapScoringFunc;
	LuaReference m_offsetJudgingFunc;
	LuaReference m_missWindowFunc;

	bool customScoringFunctionsEnabled = false;

};

extern std::shared_ptr<ReplayManager> REPLAYS;

#endif
