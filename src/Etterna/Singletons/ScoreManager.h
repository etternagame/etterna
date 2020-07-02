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

	HighScore* AddScore(HighScore& hs);

	vector<std::string> GetSortedKeys();
	void PushSelf(lua_State* L);

	bool HandleNoCCPB(HighScore& hs);

	XNode* CreateNode(const int& rate) const;
	void LoadFromNode(const XNode* node,
					  const std::string& key,
					  const float& rate,
					  const std::string& profileID);

	const vector<HighScore*> GetAllScores();
	unordered_map<std::string, HighScore> scores;
};

// All scores for a specific chart
struct ScoresForChart
{
  public:
	ScoresForChart();

	Grade bestGrade = Grade_Invalid; // best grade for any rate

	HighScore* GetPBAt(float& rate);
	HighScore* GetPBUpTo(float& rate);

	vector<HighScore*> GetAllPBPtrs();

	HighScore* AddScore(HighScore& hs);

	vector<float> GetPlayedRates();
	vector<int> GetPlayedRateKeys();
	vector<std::string> GetPlayedRateDisplayStrings();
	std::string RateKeyToDisplayString(float rate);
	int RateToKey(float& rate) { return lround(rate * 10000.f); }
	float KeyToRate(int key) { return static_cast<float>(key) / 10000.f; }

	void PushSelf(lua_State* L);

	Chart ch;

	ScoresAtRate* GetScoresAtRate(const int& rate);
	const vector<HighScore*> GetAllScores();
	XNode* CreateNode(const std::string& ck) const;
	void LoadFromNode(const XNode* node,
					  const std::string& ck,
					  const std::string& profileID);

	ScoresAtRate operator[](const int rate) { return ScoresByRate.at(rate); }

	// Sets rate indepdendent topscore tags inside highscores. 1 = best. 2 =
	// 2nd. 0 = the rest. -mina
	void SetTopScores();

	auto GetNumScores() const -> int { return ScoresByRate.size(); }

	/* It makes sense internally to have the map keys sorted highest rate to
	lowest however my experience in lua is that it tends to be more friendly
	to approach things in the reverse -mina */
	map<int, ScoresAtRate, greater<int>> ScoresByRate;
};

class ScoreManager
{
  public:
	ScoreManager();
	~ScoreManager();

	vector<vector<HighScore*>> GetAllPBPtrs(
	  const std::string& profileID =
		PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);
	HighScore* GetChartPBAt(const std::string& ck,
							float& rate,
							const std::string& profileID =
							  PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);

	// technically "up to and including rate: x" but that's a mouthful -mina
	// HighScore* GetChartPBUpTo(const std::string& ck, float& rate);
	HighScore* GetChartPBUpTo(const std::string& ck,
							  float& rate,
							  const std::string& profileID =
								PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);

	Grade GetBestGradeFor(const std::string& ck,
						  const std::string& profileID =
							PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
	{
		if (KeyHasScores(ck, profileID))
			return pscores[profileID][ck].bestGrade;
		return Grade_Invalid;
	}

	// for scores achieved during this session
	// now returns top score status because i'm bad at coding --lurker
	int AddScore(const HighScore& hs_,
				 const std::string& profileID =
				   PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
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
	float minpercent = PREFSMAN->m_fMinPercentToSaveScores;

	// Player Rating and SSR functions
	void SortTopSSRPtrs(Skillset ss,
						const std::string& profileID =
						  PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);
	void RecalculateSSRs(LoadingWindow* ld, const std::string& profileID);
	void RecalculateSSRs(const std::string& profileID);
	void UnInvalidateAllScores();
	void CalcPlayerRating(float& prating,
						  float* pskillsets,
						  const std::string& profileID);
	float AggregateSSRs(Skillset ss, float rating, float res, int iter) const;

	float GetTopSSRValue(unsigned int rank, int ss);

	HighScore* GetTopSSRHighScore(unsigned int rank, int ss);

	bool KeyHasScores(const std::string& ck,
					  const std::string& profileID =
						PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
	{
		return pscores[profileID].count(ck) == 1;
	}
	bool HasAnyScores() { return !AllScores.empty(); }
	void RatingOverTime();

	XNode* CreateNode(const std::string& profileID =
						PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID) const;
	void LoadFromNode(const XNode* node,
					  const std::string& profileID =
						PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);

	ScoresForChart* GetScoresForChart(
	  const std::string& ck,
	  const std::string& profileID =
		PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);
	vector<std::string> GetSortedKeys();

	void PushSelf(lua_State* L);
	HighScore* GetMostRecentScore()
	{
		if (camefromreplay) {
			ASSERT_M(tempscoreforonlinereplayviewing != nullptr,
					 "Temp score for Replay & Practice viewing was empty.");
			return tempscoreforonlinereplayviewing;
		}
		ASSERT_M(AllScores.size() != 0, "Profile has no Scores.");
		return AllScores.back();
	}
	void PutScoreAtTheTop(const std::string& scorekey)
	{
		auto score = ScoresByKey[scorekey];
		std::swap(score, AllScores.back());
	}
	const vector<HighScore*>& GetAllScores() { return AllScores; }
	const unordered_map<std::string, HighScore*>& GetScoresByKey()
	{
		return ScoresByKey;
	}
	const vector<HighScore*>& GetAllProfileScores(
	  const std::string& profileID =
		PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
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
	unordered_map<std::string, ScoresForChart>* GetProfileScores(
	  const std::string& profileID =
		PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID)
	{
		return &(pscores[profileID]);
	};

	void PurgeProfileScores(const std::string& profileID =
							  PROFILEMAN->GetProfile(PLAYER_1)->m_sProfileID);
	void UnloadAllReplayData()
	{
		for (auto& s : AllScores)
			s->UnloadReplayData();
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
