#ifndef ScoreManager_H
#define ScoreManager_H

#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/Grade.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "ProfileManager.h"
#include <algorithm>

#include <map>
#include <string>
#include <unordered_map>

// Scores for a specific rate for a specific chart
struct ScoresAtRate
{
  public:
	ScoresAtRate();

	HighScore* PBptr;
	HighScore* noccPBptr;

	// -technically- your pb could be a fail grade so use "bestgrade" -mina
	Grade bestGrade;

	auto AddScore(HighScore& hs) -> HighScore*;

	[[nodiscard]] auto GetSortedKeys() const -> const vector<std::string>;
	void PushSelf(lua_State* L);

	auto HandleNoCCPB(HighScore& hs) -> bool;

	[[nodiscard]] auto CreateNode(const int& rate) const -> XNode*;
	void LoadFromNode(const XNode* node,
					  const std::string& ck,
					  const float& rate,
					  const std::string& profileID);

	auto GetAllScores() -> const vector<HighScore*>;
	unordered_map<std::string, HighScore> scores;
};

// All scores for a specific chart
struct ScoresForChart
{
  public:
	ScoresForChart();

	Grade bestGrade = Grade_Invalid; // best grade for any rate

	auto GetPBAt(float rate) -> HighScore*;
	auto GetPBUpTo(float rate) -> HighScore*;
	auto GetAllPBPtrs() -> const vector<HighScore*>;

	auto AddScore(HighScore& hs) -> HighScore*;

	[[nodiscard]] auto GetPlayedRates() const -> const vector<float>;
	[[nodiscard]] auto GetPlayedRateKeys() const -> const vector<int>;
	[[nodiscard]] auto GetPlayedRateDisplayStrings() const
	  -> const vector<std::string>;

	void PushSelf(lua_State* L);

	Chart ch;

	auto GetScoresAtRate(const int& rate) -> ScoresAtRate*;
	auto GetAllScores() -> const vector<HighScore*>;
	[[nodiscard]] auto CreateNode(const std::string& ck) const -> XNode*;
	void LoadFromNode(const XNode* node,
					  const std::string& ck,
					  const std::string& profileID);

	auto operator[](const int rate) -> ScoresAtRate
	{
		return ScoresByRate.at(rate);
	}

	// Sets rate indepdendent topscore tags inside highscores. 1 = best. 2 =
	// 2nd. 0 = the rest. -mina
	void SetTopScores();

	[[nodiscard]] auto GetNumScores() const -> int
	{
		return ScoresByRate.size();
	}

	/* It makes sense internally to have the map keys sorted highest rate to
	lowest however my experience in lua is that it tends to be more friendly
	to approach things in the reverse -mina */
	map<int, ScoresAtRate, greater<>> ScoresByRate;

	[[nodiscard]] static auto RateToKey(float rate) -> int
	{
		return lround(rate * 10000.F);
	}

	[[nodiscard]] static auto KeyToRate(int key) -> float
	{
		return static_cast<float>(key) / 10000.F;
	}
};

class ScoreManager
{
  public:
	ScoreManager();
	~ScoreManager();

	auto GetAllPBPtrs(const std::string& profileID =
						PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
	  -> const vector<vector<HighScore*>>;

	auto GetChartPBAt(const std::string& ck,
					  float rate,
					  const std::string& profileID =
						PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
	  -> HighScore*;

	// technically "up to and including rate: x" but that's a mouthful -mina
	// HighScore* GetChartPBUpTo(const std::string& ck, float& rate);
	auto GetChartPBUpTo(const std::string& ck,
						float rate,
						const std::string& profileID =
						  PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
	  -> HighScore*;

	[[nodiscard]] auto GetBestGradeFor(
	  const std::string& ck,
	  const std::string& profileID =
		PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID) const -> Grade
	{
		if (KeyHasScores(ck, profileID)) {
			return pscores.at(profileID).at(ck).bestGrade;
		}
		return Grade_Invalid;
	}

	// for scores achieved during this session
	// now returns top score status because i'm bad at coding --lurker
	auto AddScore(const HighScore& hs_,
				  const std::string& profileID =
					PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID) -> int
	{
		HighScore hs = hs_;
		RegisterScoreInProfile(
		  pscores[profileID][hs.GetChartKey()].AddScore(hs), profileID);
		return hs.GetTopScore();
	}
	void ImportScore(const HighScore& hs_,
					 const std::string& profileID =
					   PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);

	// don't save scores under this percentage
	const float minpercent = PREFSMAN->m_fMinPercentToSaveScores;

	// Player Rating and SSR functions
	void SortTopSSRPtrs(Skillset ss,
						const std::string& profileID =
						  PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);
	void RecalculateSSRs(LoadingWindow* ld, const std::string& profileID);
	void RecalculateSSRs(const std::string& profileID);
	void UnInvalidateAllScores(const string& profileID);
	void CalcPlayerRating(float& prating,
						  float* pskillsets,
						  const std::string& profileID);
	[[nodiscard]] auto AggregateSSRs(Skillset ss,
									 float rating,
									 float res,
									 int iter) const -> float;

	auto GetTopSSRValue(unsigned int rank, int ss) -> float;

	auto GetTopSSRHighScore(unsigned int rank, int ss) -> HighScore*;

	[[nodiscard]] auto KeyHasScores(
	  const std::string& ck,
	  const std::string& profileID =
		PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID) const -> bool
	{
		return pscores.at(profileID).count(ck) == 1;
	}
	[[nodiscard]] auto HasAnyScores() const -> bool
	{
		return !AllScores.empty();
	}

	[[nodiscard]] auto CreateNode(
	  const std::string& profileID =
		PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID) const -> XNode*;
	void LoadFromNode(const XNode* node,
					  const std::string& profileID =
						PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);

	auto GetScoresForChart(const std::string& ck,
						   const std::string& profileID =
							 PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
	  -> ScoresForChart*;
	auto GetSortedKeys() -> const vector<std::string>;

	void PushSelf(lua_State* L);
	auto GetMostRecentScore() -> HighScore*
	{
		if (camefromreplay) {
			ASSERT_M(tempscoreforonlinereplayviewing != nullptr,
					 "Temp score for Replay & Practice viewing was empty.");
			return tempscoreforonlinereplayviewing;
		}
		ASSERT_M(!AllScores.empty(), "Profile has no Scores.");
		return AllScores.back();
	}
	void PutScoreAtTheTop(const std::string& scorekey)
	{
		auto score = ScoresByKey[scorekey];
		std::swap(score, AllScores.back());
	}
	auto GetAllScores() -> const vector<HighScore*>& { return AllScores; }
	auto GetScoresByKey() -> const unordered_map<std::string, HighScore*>&
	{
		return ScoresByKey;
	}
	auto GetAllProfileScores(const std::string& profileID =
							   PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
	  -> const vector<HighScore*>&
	{
		return AllProfileScores[profileID];
	}
	void RegisterScore(HighScore* hs) { AllScores.emplace_back(hs); }
	void AddToKeyedIndex(HighScore* hs)
	{
		ScoresByKey.emplace(hs->GetScoreKey(), hs);
	}
	void RegisterScoreInProfile(HighScore* hs_, const std::string& profileID);

	void SetAllTopScores(const std::string& profileID =
						   PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);
	void PurgeScores();
	auto GetProfileScores(const std::string& profileID =
							PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
	  -> unordered_map<std::string, ScoresForChart>*
	{
		return &(pscores[profileID]);
	};

	void PurgeProfileScores(const std::string& profileID =
							  PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);
	void UnloadAllReplayData()
	{
		for (auto& s : AllScores) {
			s->UnloadReplayData();
		}
	}
	bool camefromreplay = false;
	HighScore* tempscoreforonlinereplayviewing;
	vector<HighScore*> scorestorecalc;

	// probably can avoid copying strings if we're sure it's safe
	set<HighScore*> rescores;

  private:
	unordered_map<std::string, unordered_map<std::string, ScoresForChart>>
	  pscores; // Profile scores

	// Instead of storing pointers for each skillset just reshuffle the same set
	// of pointers it's inexpensive and not called often
	vector<HighScore*> TopSSRs;
	vector<HighScore*> AllScores;
	unordered_map<std::string, vector<HighScore*>> AllProfileScores;

	// pointers in a keyed index (by scorekey, in case it's not immediately
	// obvious)
	unordered_map<std::string, HighScore*> ScoresByKey;
};

extern ScoreManager* SCOREMAN;

#endif
