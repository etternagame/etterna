#ifndef StatsManager_H
#define StatsManager_H

#include "Etterna/Models/Misc/StageStats.h"

/** @brief Managed non-persisted statistics. */
class StatsManager
{
  public:
	StatsManager();
	~StatsManager();

	void Reset();

	/**
	 * @brief The current Stage stats.
	 *
	 * This is not necessarily passed stage stats if this is an Extra Stage. */
	StageStats m_CurStageStats;
	vector<StageStats> m_vPlayedStageStats;

	// Only the latest 3 normal songs + passed extra stages.
	void GetFinalEvalStageStats(StageStats& statsOut) const;

	// All stages played.  Returns a ref to the private member so that
	// the object will remain alive while Lua is operating on it.
	void CalcAccumPlayedStageStats();
	auto GetAccumPlayedStageStats() -> StageStats&
	{
		return m_AccumPlayedStageStats;
	}

	static void CommitStatsToProfiles(const StageStats* pSS);
	static void AddPlayerStatsToProfile(Profile* pProfile);

	void UnjoinPlayer(PlayerNumber pn);
	void GetStepsInUse(std::set<Steps*>& apInUseOut) const;

	// Lua
	void PushSelf(lua_State* L);

  private:
	StageStats m_AccumPlayedStageStats;
};

extern StatsManager*
  STATSMAN; // global and accessible from anywhere in our program

#endif
