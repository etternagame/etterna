#include "Grade.h"
#include "GameConstantsAndTypes.h"
#include <map>
#include <unordered_map>



// Scores for a specific rate for a specific chart
struct ScoresAtRate
{
public:
	HighScore* PBptr = nullptr;

	// -technically- your pb could be a fail grade so use "bestgrade" -mina
	Grade bestGrade = Grade_Invalid;

	void AddScore(HighScore& hs);

	vector<string> GetSortedKeys();

	void PushSelf(lua_State *L);
	unordered_map<string, HighScore> scores;

	XNode* CreateNode(const int& rate) const;
	void LoadFromNode(const XNode* node, const RString& key, const float& rate);
private:
	
};


// All scores for a specific chart
struct ScoresForChart
{
public:
	Grade bestGrade = Grade_Invalid;	// best grade for any rate 

	HighScore* GetPBAt(float& rate);
	HighScore* GetPBUpTo(float& rate);

	vector<HighScore*> GetAllPBPtrs();


	void AddScore(HighScore& hs);

	vector<float> GetPlayedRates();
	vector<int> GetPlayedRateKeys();
	int RateToKey(float& rate) { return lround(rate * 10000.f); }
	float KeyToRate(int key) { return static_cast<float>(key) / 10000.f; }

	void PushSelf(lua_State *L);

	/* It makes sense internally to have the map keys sorted highest rate to lowest
	however my experience in lua is that it tends to be more friendly to approach things
	in the reverse -mina */ 
	map<int, ScoresAtRate, greater<int>> ScoresByRate;

	
	XNode *CreateNode(const string& ck) const;
	void LoadFromNode(const XNode* node, const RString& ck);
private:

	
};

class PlayerScores
{
public:
	// at what? rate. Duh. -mina
	HighScore* GetChartPBAt(string& ck, float& rate);

	// technically "up to and including rate: x" but that's a mouthful -mina
	HighScore* GetChartPBUpTo(string& ck, float& rate);

	Grade GetBestGradeFor(string& ck) { if (pscores.count(ck)) return pscores[ck].bestGrade; return Grade_Invalid; }

	// for scores achieved during this session
	void AddScore(const HighScore& hs_) { HighScore hs = hs_; pscores[hs.GetChartKey()].AddScore(hs); }





	// Player Rating and SSR functions
	void SortTopSSRPtrs(Skillset ss);
	void CalcPlayerRating(float& prating, float* pskillsets);

	// perhaps we will need a generalized version again someday, but not today
	float AggregateSSRs(Skillset ss, float rating, float res, int iter) const;	
	
	bool IsChartLoaded(const string& ck) { return true; }	// obviously not functioning as intended yet


	unordered_map<string, ScoresForChart> pscores;
	map<string, HighScore&> AllScores;
	vector<HighScore*> TopSSRs[NUM_Skillset];

	XNode *CreateNode() const;
	void LoadFromNode(const XNode* node);
private:


};