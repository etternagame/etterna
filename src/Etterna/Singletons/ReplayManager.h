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

	void reset()
	{
		replayRate = 1.F;
		replayModifiers = std::string();
		replayUsedMirror = false;
		oldModifiers = std::string();
		oldRate = 1.F;
		oldNoteskin = std::string();
		oldFailType = FailType_Immediate;
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
	Replay* InitReplayPlaybackForScore(HighScore* hs, float timingScale = 1.F);

	void UnsetActiveReplay();

	/// Returns the Replay being watched by the player. Never null
	Replay* GetActiveReplay();

	HighScore* GetActiveReplayScore();

	void StoreActiveReplaySettings(float replayRate,
								   std::string& replayModifiers,
								   bool replayUsedMirror,
								   float oldRate,
								   std::string& oldModifiers,
								   FailType oldFailType,
								   std::string& oldNoteskin);

	TemporaryReplaySettings GetActiveReplaySettings();

	static TapNoteScore GetTapNoteScoreForReplay(float fNoteOffset,
												 float timingScale = 1.F);

	auto CalculateRadarValuesForReplay(Replay& replay,
									   RadarValues& rv,
									   RadarValues& possibleRV) -> bool;

	auto SetPlayerStageStatsForReplay(Replay& replay,
									  PlayerStageStats* pss,
									  float timingScale = 1.F) -> bool;

	auto GenerateLifeRecordForReplay(Replay& replay, float timingScale = 1.F)
	  -> std::map<float, float>;

	auto GenerateComboListForReplay(Replay& replay, float timingScale = 1.F)
	  -> std::vector<PlayerStageStats::Combo_t>;

	/// Lua
	void PushSelf(lua_State* L);

  private:
	/// scorekey to {refcount, pointer}
	std::unordered_map<std::string, std::pair<unsigned, Replay*>>
	  scoresToReplays{};
};

extern std::shared_ptr<ReplayManager> REPLAYS;

#endif
