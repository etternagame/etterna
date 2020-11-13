#ifndef ScoreManager_H
#define ScoreManager_H

#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/Grade.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "ProfileManager.h"

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
	float bestWifeScore = 0.F;

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
	std::unordered_map<std::string, HighScore> scores;
};

// All scores for a specific chart
struct ScoresForChart
{
  public:
	ScoresForChart();

	Grade bestGrade = Grade_Invalid; // best grade for any rate
	float bestWifeScore = 0.F;

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
	std::map<int, ScoresAtRate, std::greater<>> ScoresByRate;

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

	[[nodiscard]] auto GetBestWifeScoreFor(
	  const std::string& ck,
	  const std::string& profileID =
		PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID) const -> float
	{
		if (KeyHasScores(ck, profileID)) {
			return pscores.at(profileID).at(ck).bestWifeScore;
		}

		return 0.F;
	}

	// for scores achieved during this session
	// now returns top score status because i'm bad at coding --lurker
	auto AddScore(const HighScore& hs_,
				  const std::string& profileID =
					PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID) -> int
	{
		HighScore hs = hs_;
		HighScore* h = pscores[profileID][hs.GetChartKey()].AddScore(hs);
		RegisterScoreThisSession(h);
		RegisterScoreInProfile(h, profileID);
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
	void SortTopSSRPtrsForGame(
	  Skillset ss,
	  const string& profileID = PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);
	void RecalculateSSRs(LoadingWindow* ld);
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
	auto GetTopSSRHighScoreForGame(unsigned int rank, int ss) -> HighScore*;
	auto GetRecentScore(int rank) -> HighScore*;
	auto GetRecentScoreForGame(int rank) -> HighScore*;
	void SortRecentScores(const std::string& profileID =
							PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);
	void SortRecentScoresForGame(
	  const std::string& profileID =
		PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);

	[[nodiscard]] auto KeyHasScores(
	  const std::string& ck,
	  const std::string& profileID =
		PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID) const -> bool
	{
		return pscores.count(profileID) == 1 && pscores.at(profileID).count(ck) == 1;
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
		// Allow Lua to receive null HS here
		if (AllScores.empty())
			return nullptr;
		return AllScores.back();
	}
	void PutScoreAtTheTop(const std::string& scorekey)
	{
		auto score = ScoresByKey[scorekey];
		std::swap(score, AllScores.back());
	}
	auto GetAllScores() -> const vector<HighScore*>& { return AllScores; }
	auto GetScoresByKey() -> const std::unordered_map<std::string, HighScore*>&
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

	// return all skillsets ordered by number of plays
	std::vector<Skillset> GetTopPlayedSkillsets(
	  const std::string& profileID = PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);

	std::vector<int> GetPlaycountPerSkillset(
	  const std::string& profileID = PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);

	void SetAllTopScores(const std::string& profileID =
						   PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);
	void PurgeScores();
	auto GetProfileScores(const std::string& profileID =
							PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
	  -> std::unordered_map<std::string, ScoresForChart>*
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
	std::set<HighScore*> rescores;

	auto GetNumScoresThisSession() -> int
	{
		return scoresThisSession.size();
	}
	auto GetScoresThisSession() -> vector<HighScore*>
	{
		return scoresThisSession;
	}
	void RegisterScoreThisSession(HighScore* hs)
	{
		scoresThisSession.push_back(hs);
	}

  private:
	std::unordered_map<std::string,
					   std::unordered_map<std::string, ScoresForChart>>
	  pscores; // Profile scores

	// Instead of storing pointers for each skillset just reshuffle the same set
	// of pointers it's inexpensive and not called often
	vector<HighScore*> TopSSRs;
	vector<HighScore*> TopSSRsForGame;
	vector<HighScore*> AllScores;
	std::unordered_map<std::string, vector<HighScore*>> AllProfileScores;

	// pointers in a keyed index (by scorekey, in case it's not immediately
	// obvious)
	std::unordered_map<std::string, HighScore*> ScoresByKey;

	// a more thought out (not really) replacement for STATSMAN played stage stats
	// note: scoresThisSession is NOT meant to reset on profile load
	// (design choice)
	vector<HighScore*> scoresThisSession;
};

extern ScoreManager* SCOREMAN;

#endif
