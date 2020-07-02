#include "Etterna/Globals/global.h"
#include "CryptManager.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "GameState.h"
#include "LuaManager.h"
#include "PrefsManager.h"
#include "Etterna/Models/Misc/Profile.h"
#include "Etterna/Models/Misc/Profile.h"
#include "ProfileManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "ScoreManager.h"
#include "StatsManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/StepsAndStyles/StyleUtil.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"

StatsManager* STATSMAN =
  NULL; // global object accessible from anywhere in the program

StatsManager::StatsManager()
{
	// Register with Lua.
	{
		Lua* L = LUA->Get();
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

static StageStats
AccumPlayedStageStats(const vector<StageStats>& vss)
{
	StageStats ssreturn;

	if (!vss.empty()) {
		ssreturn.m_playMode = vss[0].m_playMode;
	}

	FOREACH_CONST(StageStats, vss, ss)
	ssreturn.AddStats(*ss);

	unsigned uNumSongs = ssreturn.m_vpPlayedSongs.size();

	if (uNumSongs == 0)
		return ssreturn; // don't divide by 0 below

	/* Scale radar percentages back down to roughly 0..1.  Don't scale
	 * RadarCategory_TapsAndHolds and the rest, which are counters. */
	// FIXME: Weight each song by the number of stages it took to account for
	// long, marathon.
	for (int r = 0; r < RadarCategory_TapsAndHolds; r++) {
		ssreturn.m_player.m_radarPossible[r] /= uNumSongs;
		ssreturn.m_player.m_radarActual[r] /= uNumSongs;
	}
	return ssreturn;
}

void
StatsManager::GetFinalEvalStageStats(StageStats& statsOut) const
{
	statsOut.Init();
	vector<StageStats> vssToCount;
	for (size_t i = 0; i < m_vPlayedStageStats.size(); ++i) {
		vssToCount.push_back(m_vPlayedStageStats[i]);
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
	int iNumTapsAndHolds =
	  (int)pSS->m_player.m_radarActual[RadarCategory_TapsAndHolds];
	int iNumJumps = (int)pSS->m_player.m_radarActual[RadarCategory_Jumps];
	int iNumHolds = (int)pSS->m_player.m_radarActual[RadarCategory_Holds];
	int iNumRolls = (int)pSS->m_player.m_radarActual[RadarCategory_Rolls];
	int iNumMines = (int)pSS->m_player.m_radarActual[RadarCategory_Mines];
	int iNumHands = (int)pSS->m_player.m_radarActual[RadarCategory_Hands];
	int iNumLifts = (int)pSS->m_player.m_radarActual[RadarCategory_Lifts];
	PROFILEMAN->AddStepTotals(PLAYER_1,
							  iNumTapsAndHolds,
							  iNumJumps,
							  iNumHolds,
							  iNumRolls,
							  iNumMines,
							  iNumHands,
							  iNumLifts);

	// Update profile stats
	int iGameplaySeconds = (int)truncf(pSS->m_fGameplaySeconds);
}

void
StatsManager::UnjoinPlayer(PlayerNumber pn)
{
	/* A player has been unjoined.  Clear his data from m_vPlayedStageStats, and
	 * purge any m_vPlayedStageStats that no longer have any player data because
	 * all of the players that were playing at the time have been unjoined. */
	FOREACH(StageStats, m_vPlayedStageStats, ss)
	ss->m_player = PlayerStageStats();

	for (int i = 0; i < (int)m_vPlayedStageStats.size(); ++i) {
		StageStats& ss = m_vPlayedStageStats[i];
		bool bIsActive = false;
		if (ss.m_player.m_bJoined)
			bIsActive = true;
		FOREACH_MultiPlayer(mp) if (ss.m_multiPlayer[mp].m_bJoined) bIsActive =
		  true;
		if (bIsActive)
			continue;

		m_vPlayedStageStats.erase(m_vPlayedStageStats.begin() + i);
		--i;
	}
}

void
StatsManager::GetStepsInUse(set<Steps*>& apInUseOut) const
{
	for (int i = 0; i < (int)m_vPlayedStageStats.size(); ++i) {
		const PlayerStageStats& pss = m_vPlayedStageStats[i].m_player;
		apInUseOut.insert(pss.m_vpPossibleSteps.begin(),
						  pss.m_vpPossibleSteps.end());

		FOREACH_MultiPlayer(mp)
		{
			const PlayerStageStats& pss =
			  m_vPlayedStageStats[i].m_multiPlayer[mp];
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
	static int GetCurStageStats(T* p, lua_State* L)
	{
		p->m_CurStageStats.PushSelf(L);
		return 1;
	}
	static int GetPlayedStageStats(T* p, lua_State* L)
	{
		int iAgo = IArg(1);
		int iIndex = p->m_vPlayedStageStats.size() - iAgo;
		if (iIndex < 0 || iIndex >= (int)p->m_vPlayedStageStats.size())
			return 0;

		p->m_vPlayedStageStats[iIndex].PushSelf(L);
		return 1;
	}
	static int Reset(T* p, lua_State* L)
	{
		p->Reset();
		return 0;
	}
	static int GetAccumPlayedStageStats(T* p, lua_State* L)
	{
		p->GetAccumPlayedStageStats().PushSelf(L);
		return 1;
	}
	static int GetFinalEvalStageStats(T* p, lua_State* L)
	{
		StageStats stats;
		p->GetFinalEvalStageStats(stats);
		stats.PushSelf(L);
		return 1;
	}
	static int GetFinalGrade(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;

		if (!GAMESTATE->IsHumanPlayer(pn))
			lua_pushnumber(L, Grade_Invalid);
		else {
			StageStats stats;
			p->GetFinalEvalStageStats(stats);
			lua_pushnumber(L, stats.m_player.GetGrade());
		}
		return 1;
	}
	static int GetStagesPlayed(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_vPlayedStageStats.size());
		return 1;
	}

	static int GetBestGrade(T* p, lua_State* L)
	{
		Grade g = NUM_Grade;
		g = min(g, STATSMAN->m_CurStageStats.m_player.GetGrade());
		lua_pushnumber(L, g);
		return 1;
	}

	static int GetWorstGrade(T* p, lua_State* L)
	{
		Grade g = Grade_Tier01;
		g = max(g, STATSMAN->m_CurStageStats.m_player.GetGrade());
		lua_pushnumber(L, g);
		return 1;
	}

	static int GetBestFinalGrade(T* t, lua_State* L)
	{
		Grade top_grade = Grade_Failed;
		StageStats stats;
		t->GetFinalEvalStageStats(stats);
		// If this player failed any stage, then their final grade is an F.
		bool bPlayerFailedOneStage = false;
		FOREACH_CONST(StageStats, STATSMAN->m_vPlayedStageStats, ss)
		{
			if (ss->m_player.m_bFailed) {
				bPlayerFailedOneStage = true;
				break;
			}
		}

		top_grade = min(top_grade, stats.m_player.GetGrade());

		Enum::Push(L, top_grade);
		return 1;
	}

	static int UpdatePlayerRating(T* p, lua_State* L)
	{
		Profile* profile = PROFILEMAN->GetProfile(PLAYER_1);
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
