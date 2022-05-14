#include "Etterna/Globals/global.h"
#include "CryptManager.h"
#include "GameState.h"
#include "LuaManager.h"
#include "Etterna/Models/Misc/Profile.h"
#include "ProfileManager.h"
#include "ScoreManager.h"
#include "StatsManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"

#include <algorithm>

using std::set;

StatsManager* STATSMAN =
  nullptr; // global object accessible from anywhere in the program

StatsManager::StatsManager()
{
	// Register with Lua.
	{
		auto* L = LUA->Get();
		lua_pushstring(L, "STATSMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

StatsManager::~StatsManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("STATSMAN");
}

void
StatsManager::Reset()
{
	m_CurStageStats.Init();
	m_vPlayedStageStats.clear();
	m_AccumPlayedStageStats.Init();

	CalcAccumPlayedStageStats();
}

static auto
AccumPlayedStageStats(const std::vector<StageStats>& vss) -> StageStats
{
	StageStats ssreturn;

	for (const auto& ss : vss) {
		ssreturn.AddStats(ss);
	}

	const unsigned uNumSongs = ssreturn.m_vpPlayedSongs.size();

	if (uNumSongs == 0) {
		return ssreturn; // don't divide by 0 below
	}

	/* Scale radar percentages back down to roughly 0..1.  Don't scale
	 * RadarCategory_TapsAndHolds and the rest, which are counters. */
	// FIXME: Weight each song by the number of stages it took to account for
	// long, marathon.
	for (auto r = 0; r < RadarCategory_TapsAndHolds; ++r) {
		ssreturn.m_player.m_radarPossible[r] /= uNumSongs;
		ssreturn.m_player.m_radarActual[r] /= uNumSongs;
	}
	return ssreturn;
}

void
StatsManager::GetFinalEvalStageStats(StageStats& statsOut) const
{
	statsOut.Init();
	std::vector<StageStats> vssToCount;
	for (const auto& m_vPlayedStageStat : m_vPlayedStageStats) {
		vssToCount.push_back(m_vPlayedStageStat);
	}
	statsOut = AccumPlayedStageStats(vssToCount);
}

void
StatsManager::CalcAccumPlayedStageStats()
{
	m_AccumPlayedStageStats = AccumPlayedStageStats(m_vPlayedStageStats);
}

// fffff this is back here because some scores/files dont calc properly if
// called during load -mina
void
StatsManager::AddPlayerStatsToProfile(Profile* pProfile)
{
	SCOREMAN->CalcPlayerRating(pProfile->m_fPlayerRating,
							   pProfile->m_fPlayerSkillsets,
							   pProfile->m_sProfileID);
	MESSAGEMAN->Broadcast("PlayerRatingUpdated");
}

void
StatsManager::CommitStatsToProfiles(const StageStats* pSS)
{
	// Add step totals.  Use radarActual, since the player might have failed
	// part way through the song, in which case we don't want to give credit for
	// the rest of the song.
	const auto iNumTapsAndHolds =
	  static_cast<int>(pSS->m_player.m_radarActual[RadarCategory_TapsAndHolds]);
	const auto iNumJumps =
	  static_cast<int>(pSS->m_player.m_radarActual[RadarCategory_Jumps]);
	const auto iNumHolds =
	  static_cast<int>(pSS->m_player.m_radarActual[RadarCategory_Holds]);
	const auto iNumRolls =
	  static_cast<int>(pSS->m_player.m_radarActual[RadarCategory_Rolls]);
	const auto iNumMines =
	  static_cast<int>(pSS->m_player.m_radarActual[RadarCategory_Mines]);
	const auto iNumHands =
	  static_cast<int>(pSS->m_player.m_radarActual[RadarCategory_Hands]);
	const auto iNumLifts =
	  static_cast<int>(pSS->m_player.m_radarActual[RadarCategory_Lifts]);
	PROFILEMAN->AddStepTotals(PLAYER_1,
							  iNumTapsAndHolds,
							  iNumJumps,
							  iNumHolds,
							  iNumRolls,
							  iNumMines,
							  iNumHands,
							  iNumLifts);
}

void StatsManager::UnjoinPlayer(PlayerNumber /*pn*/)
{
	/* A player has been unjoined.  Clear his data from m_vPlayedStageStats, and
	 * purge any m_vPlayedStageStats that no longer have any player data because
	 * all of the players that were playing at the time have been unjoined. */
	for (auto& ss : m_vPlayedStageStats) {
		ss.m_player = PlayerStageStats();
	}

	for (auto i = 0; i < static_cast<int>(m_vPlayedStageStats.size()); ++i) {
		auto& ss = m_vPlayedStageStats[i];
		auto bIsActive = false;
		if (ss.m_player.m_bJoined) {
			bIsActive = true;
		}
		FOREACH_MultiPlayer(mp) if (ss.m_multiPlayer[mp].m_bJoined)
		{
			bIsActive = true;
		}
		if (bIsActive) {
			continue;
		}

		m_vPlayedStageStats.erase(m_vPlayedStageStats.begin() + i);
		--i;
	}
}

void
StatsManager::GetStepsInUse(set<Steps*>& apInUseOut) const
{
	for (const auto& m_vPlayedStageStat : m_vPlayedStageStats) {
		const auto& pss = m_vPlayedStageStat.m_player;
		apInUseOut.insert(pss.m_vpPossibleSteps.begin(),
						  pss.m_vpPossibleSteps.end());

		FOREACH_MultiPlayer(mp)
		{
			const auto& pss = m_vPlayedStageStat.m_multiPlayer[mp];
			apInUseOut.insert(pss.m_vpPossibleSteps.begin(),
							  pss.m_vpPossibleSteps.end());
		}
	}
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the StatsManager. */
class LunaStatsManager : public Luna<StatsManager>
{
  public:
	static auto GetCurStageStats(T* p, lua_State* L) -> int
	{
		p->m_CurStageStats.PushSelf(L);
		return 1;
	}
	static auto GetPlayedStageStats(T* p, lua_State* L) -> int
	{
		const auto iAgo = IArg(1);
		const int iIndex = p->m_vPlayedStageStats.size() - iAgo;
		if (iIndex < 0 ||
			iIndex >= static_cast<int>(p->m_vPlayedStageStats.size())) {
			return 0;
		}

		p->m_vPlayedStageStats[iIndex].PushSelf(L);
		return 1;
	}
	static auto Reset(T* p, lua_State * /*L*/) -> int
	{
		p->Reset();
		return 0;
	}
	static auto GetAccumPlayedStageStats(T* p, lua_State* L) -> int
	{
		p->GetAccumPlayedStageStats().PushSelf(L);
		return 1;
	}
	static auto GetFinalEvalStageStats(T* p, lua_State* L) -> int
	{
		StageStats stats;
		p->GetFinalEvalStageStats(stats);
		stats.PushSelf(L);
		return 1;
	}
	static auto GetFinalGrade(T* p, lua_State* L) -> int
	{
		const auto pn = PLAYER_1;

		if (!GAMESTATE->IsHumanPlayer(pn)) {
			lua_pushnumber(L, Grade_Invalid);
		} else {
			StageStats stats;
			p->GetFinalEvalStageStats(stats);
			lua_pushnumber(L, stats.m_player.GetGrade());
		}
		return 1;
	}
	static auto GetStagesPlayed(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->m_vPlayedStageStats.size());
		return 1;
	}

	static auto GetBestGrade(T* /*p*/, lua_State* L) -> int
	{
		auto g = NUM_Grade;
		g = std::min(g, STATSMAN->m_CurStageStats.m_player.GetGrade());
		lua_pushnumber(L, g);
		return 1;
	}

	static auto GetWorstGrade(T* /*p*/, lua_State* L) -> int
	{
		auto g = Grade_Tier01;
		g = std::max(g, STATSMAN->m_CurStageStats.m_player.GetGrade());
		lua_pushnumber(L, g);
		return 1;
	}

	static auto GetBestFinalGrade(T* t, lua_State* L) -> int
	{
		auto top_grade = Grade_Failed;
		StageStats stats;
		t->GetFinalEvalStageStats(stats);

		top_grade = std::min(top_grade, stats.m_player.GetGrade());

		Enum::Push(L, top_grade);
		return 1;
	}

	static auto UpdatePlayerRating(T* p, lua_State * /*L*/) -> int
	{
		auto* const profile = PROFILEMAN->GetProfile(PLAYER_1);
		p->AddPlayerStatsToProfile(profile);
		return 0;
	}

	LunaStatsManager()
	{
		ADD_METHOD(GetCurStageStats);
		ADD_METHOD(GetPlayedStageStats);
		ADD_METHOD(GetAccumPlayedStageStats);
		ADD_METHOD(GetFinalEvalStageStats);
		ADD_METHOD(Reset);
		ADD_METHOD(GetFinalGrade);
		ADD_METHOD(GetStagesPlayed);
		ADD_METHOD(GetBestGrade);
		ADD_METHOD(GetWorstGrade);
		ADD_METHOD(GetBestFinalGrade);
		ADD_METHOD(UpdatePlayerRating);
	}
};

LUA_REGISTER_CLASS(StatsManager)
// lua end
