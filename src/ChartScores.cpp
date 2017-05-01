#include "global.h"
#include "RageLog.h"
#include "HighScore.h"
#include "GameConstantsAndTypes.h"
#include "Foreach.h"
#include "ChartScores.h"


void ScoreMap::AddScore(HighScore& hs) {
	string& key = hs.GetScoreKey();
	if (float newpb = hs.GetWifeScore() > pbScore) {
		pbScore = newpb;
		pbKey = key;
	}
	scores.emplace(key, hs);
}

vector<string> ScoreMap::GetSortedKeys() {
	map<float, string, greater<float>> tmp;
	vector<string> o;
	FOREACHM(string, HighScore, scores, i)
		tmp.emplace(i->second.GetWifeScore(), i->first);
	FOREACHM(float, string, tmp, j)
		o.emplace_back(j->second);
	return o;
}

HighScore& ChartScores::GetPBAt(float& rate) {
	int key = RateToKey(rate);
	if(ScoresByRate.count(key))
		return ScoresByRate.at(key).GetPB();
	return HighScore();
}

HighScore& ChartScores::GetPBUpTo(float& rate) {
	int key = RateToKey(rate);
	FOREACHM(int, ScoreMap, ScoresByRate, i) 
		if (i->first <= key)
			return i->second.GetPB();
		
	return HighScore();
}

void ChartScores::AddScore(HighScore& hs) {
	float rate = hs.GetMusicRate();
	int key = RateToKey(rate);
	ScoresByRate[key].AddScore(hs);
}

vector<float> ChartScores::GetPlayedRates() {
	vector<float> o;
	FOREACHM(int, ScoreMap, ScoresByRate, i)
		o.emplace_back(KeyToRate(i->first));
	return o;
}

vector<int> ChartScores::GetPlayedRateKeys() {
	vector<int> o;
	FOREACHM(int, ScoreMap, ScoresByRate, i)
		o.emplace_back(i->first);
	return o;
}

HighScore& PlayerScores::GetChartPBAt(string& ck, float& rate) {
	if (pscores.count(ck))
		return pscores.at(ck).GetPBAt(rate);
	return HighScore();
}

HighScore& PlayerScores::GetChartPBUpTo(string& ck, float& rate) {
	if (pscores.count(ck))
		return pscores.at(ck).GetPBUpTo(rate);
	return HighScore();
}



#include "LuaBinding.h"

class LunaScoreMap: public Luna<ScoreMap>
{
public:
	static int GetScores(T* p, lua_State *L) {
		lua_newtable(L);
		vector<string> keys = p->GetSortedKeys();
		for (size_t i = 0; i < keys.size(); ++i) {
			HighScore& wot = p->scores[keys[i]];
			wot.PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}

		return 1;
	}

	LunaScoreMap()
	{
		ADD_METHOD(GetScores);
	}
};

LUA_REGISTER_CLASS(ScoreMap)

class LunaChartScores : public Luna<ChartScores>
{
public:
	static int GetScoreMaps(T* p, lua_State *L) {
		lua_newtable(L);
		vector<float> rates = p->GetPlayedRates();
		for (size_t i = 0; i < rates.size(); ++i) {
			p->ScoresByRate[rates[i]].PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}

		return 1;
	}


	LunaChartScores()
	{
		ADD_METHOD(GetScoreMaps);
	}
};

LUA_REGISTER_CLASS(ChartScores)
