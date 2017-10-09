#ifndef ScoreManager_H
#define ScoreManager_H

#include "Grade.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "SongManager.h"

#include <map>
#include <unordered_map>
#include <string>

using std::string;

// Scores for a specific rate for a specific chart
struct ScoresAtRate
{
public:
	ScoresAtRate();

	HighScore* PBptr;
	HighScore* noccPBptr;

	// -technically- your pb could be a fail grade so use "bestgrade" -mina
	Grade bestGrade;

	void AddScore(HighScore& hs);

	vector<string> GetSortedKeys();
	void PushSelf(lua_State *L);
	

	XNode* CreateNode(const int& rate) const;
	void LoadFromNode(const XNode* node, const string& key, const float& rate);

	const vector<HighScore*> GetScores(const string) const;
	unordered_map<string, HighScore> scores;
private:

};


// All scores for a specific chart
struct ScoresForChart
{
public:
	ScoresForChart();


	Grade bestGrade = Grade_Invalid;	// best grade for any rate 

	HighScore* GetPBAt(float& rate);
	HighScore* GetPBUpTo(float& rate);

	vector<HighScore*> GetAllPBPtrs();

	void AddScore(HighScore& hs);

	vector<float> GetPlayedRates();
	vector<int> GetPlayedRateKeys();
	vector<string> GetPlayedRateDisplayStrings();
	string RateKeyToDisplayString(float rate);
	int RateToKey(float& rate) { return lround(rate * 10000.f); }
	float KeyToRate(int key) { return static_cast<float>(key) / 10000.f; }

	void PushSelf(lua_State *L);

	Chart ch;

	ScoresAtRate* GetScoresAtRate(const int& rate);
	XNode *CreateNode(const string& ck) const;
	void LoadFromNode(const XNode* node, const string& ck);

	ScoresAtRate operator[](const int rate) { return ScoresByRate.at(rate); }
	map<int, ScoresAtRate, greater<int>> ScoresByRate;
private:
	/* It makes sense internally to have the map keys sorted highest rate to lowest
	however my experience in lua is that it tends to be more friendly to approach things
	in the reverse -mina */
	
};

class ScoreManager
{
public:
	ScoreManager();
	~ScoreManager();


	HighScore* GetChartPBAt(const string& ck, float& rate);

	// technically "up to and including rate: x" but that's a mouthful -mina
	HighScore* GetChartPBUpTo(const string& ck, float& rate);

	Grade GetBestGradeFor(const string& ck) { if (pscores.count(ck)) return pscores[ck].bestGrade; return Grade_Invalid; }

	// for scores achieved during this session
	void AddScore(const HighScore& hs_) { HighScore hs = hs_; pscores[hs.GetChartKey()].AddScore(hs); }
	void ImportScore(const HighScore& hs_);

	// don't save scores under this percentage
	float minpercent = PREFSMAN->m_fMinPercentToSaveScores;

	// Player Rating and SSR functions
	void SortTopSSRPtrs(Skillset ss);
	void RecalculateSSRs(LoadingWindow *ld);
	void EnableAllScores();
	void CalcPlayerRating(float& prating, float* pskillsets);
	float AggregateSSRs(Skillset ss, float rating, float res, int iter) const;

	float GetTopSSRValue(unsigned int rank, int ss);

	HighScore * GetTopSSRHighScore(unsigned int rank, int ss);
	
	bool KeyHasScores(const string& ck) { return pscores.count(ck) == 1; }
	bool HasAnyScores() { return !AllScores.empty(); }
	void RatingOverTime();

	XNode *CreateNode() const;
	void LoadFromNode(const XNode* node);

	ScoresForChart* GetScoresForChart(const string& ck);
	vector<string> GetSortedKeys();

	void PushSelf(lua_State *L);
	HighScore* GetMostRecentScore() { return AllScores.back(); }
	vector<HighScore*> GetAllScores() { return AllScores; }
	void RegisterScore(HighScore* hs) {	AllScores.emplace_back(hs); }
	void AddToKeyedIndex(HighScore* hs) { ScoresByKey.emplace(hs->GetScoreKey(), hs); }
	unordered_map<string, ScoresForChart>* GetProfileScores() { return &pscores; };

private:
	unordered_map<string, ScoresForChart> pscores;	// Profile scores

	// Instead of storing pointers for each skillset just reshuffle the same set of pointers
	// it's inexpensive and not called often
	vector<HighScore*> TopSSRs;
	vector<HighScore*> AllScores;

	// pointers in a keyed index (by scorekey, in case it's not immediately obvious)
	unordered_map<string, HighScore*> ScoresByKey;
};

extern ScoreManager* SCOREMAN;

#endif