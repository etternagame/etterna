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

	map<int, ScoreMap, greater<int>> ScoresByRate;
private:

	
};

class PlayerScores
{
public:
	HighScore& GetChartPBAt(string& ck, float& rate);
	HighScore& GetChartPBUpTo(string& ck, float& rate);

	void AddScore(string& ck, HighScore& hs) { pscores[ck].AddScore(hs); }
	void AddScore(string& ck, ChartScores& cs) { pscores.emplace(ck, cs); }

	map<string, ChartScores> pscores;
	map<string, HighScore&> AllScores;
private:


};


