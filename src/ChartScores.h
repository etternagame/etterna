#include "Grade.h"
#include "GameConstantsAndTypes.h"
#include <map>


struct ScoreMap
{
public:
	string pbKey = "";
	float pbScore = 0.f;
	HighScore& GetPB() { return scores.at(pbKey); }
	void AddScore(HighScore& hs);

	vector<string> GetSortedKeys();

	void PushSelf(lua_State *L);
	map<string, HighScore> scores;
private:
	
};

struct ChartScores
{
public:
	HighScore& GetPBAt(float& rate);
	HighScore & GetPBUpTo(float& rate);

	void AddScore(HighScore& hs);
	vector<float> GetPlayedRates();
	vector<int> GetPlayedRateKeys();
	int RateToKey(float& rate) { return lround(rate * 10000.f); }
	float KeyToRate(int key) { return static_cast<float>(key) / 10000.f; }

	void PushSelf(lua_State *L);

	/* It makes sense internally to have the map keys sorted highest rate to lowest
	however my experience in lua is that it tends to be more friendly to approach things
	in the reverse -mina */ 
	map<int, ScoreMap, greater<int>> ScoresByRate;
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

	// the loading process should actually really be done by constructing scores in place -mina
	// and any internal sorting should occur after all scores ares loaded
	void AddScore(string& ck, ChartScores& cs) { pscores.emplace(ck, cs); }

	map<string, ChartScores> pscores;
	map<string, HighScore&> AllScores;
private:


};


