#include "Grade.h"
#include "GameConstantsAndTypes.h"
#include <map>



// Scores for a specific rate for a specific chart
struct ScoresAtRate
{
public:
	string pbKey = "";
	float pbScore = 0.f;
	
	HighScore* PBptr;

	HighScore** GetPBPtr() { return (&PBptr); }

	// -technically- your pb could be a fail grade so use "bestgrade" -mina
	Grade bestGrade = Grade_Invalid;

	HighScore& GetPB() { return scores.at(pbKey); }
	void AddScore(HighScore& hs);

	vector<string> GetSortedKeys();

	void PushSelf(lua_State *L);
	map<string, HighScore> scores;
private:
	
};


// All scores for a specific chart
struct ScoresForChart
{
public:
	Grade bestGrade = Grade_Invalid;	// best grade for any rate 

	HighScore& GetPBAt(float& rate);
	HighScore & GetPBUpTo(float& rate);

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
private:

	
};

class PlayerScores
{
public:

	// at what? rate. Duh. -mina
	HighScore& GetChartPBAt(string& ck, float& rate);

	// technically "up to and including rate: x" but that's a mouthful -mina
	HighScore& GetChartPBUpTo(string& ck, float& rate);

	// for scores achieved during this session
	void AddScore(const HighScore& hs_) { HighScore hs = hs_; pscores[hs.GetHistoricChartKey()].AddScore(hs); }


	void LoadScoreFromNode(string& ck, float& rate, const XNode* hs);



	// SSRs
	void RebuildTopSSRPtrs(int n, Skillset ss);



	vector<HighScore*> TopSSRs;
	map<string, ScoresForChart> pscores;
	map<string, HighScore&> AllScores;
private:


};