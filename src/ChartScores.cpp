#include "global.h"
#include "RageLog.h"
#include "HighScore.h"
#include "GameConstantsAndTypes.h"
#include "Foreach.h"
#include "ChartScores.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"


void ScoresAtRate::AddScore(HighScore& hs) {
	string& key = hs.GetScoreKey();	
	bestGrade = min(hs.GetWifeGrade(), bestGrade);
	scores.emplace(key, hs);

	if(!PBptr || PBptr->GetWifeScore() < hs.GetWifeScore())
		PBptr = &scores.find(key)->second;
}

vector<string> ScoresAtRate::GetSortedKeys() {
	map<float, string, greater<float>> tmp;
	vector<string> o;
	FOREACHM(string, HighScore, scores, i)
		tmp.emplace(i->second.GetWifeScore(), i->first);
	FOREACHM(float, string, tmp, j)
		o.emplace_back(j->second);
	return o;
}

HighScore* ScoresForChart::GetPBAt(float& rate) {
	int key = RateToKey(rate);
	if(ScoresByRate.count(key))
		return ScoresByRate.at(key).PBptr;
	return NULL;
}

HighScore* ScoresForChart::GetPBUpTo(float& rate) {
	int key = RateToKey(rate);
	FOREACHM(int, ScoresAtRate, ScoresByRate, i) 
		if (i->first <= key)
			return i->second.PBptr;
		
	return NULL;
}

void ScoresForChart::AddScore(HighScore& hs) {
	bestGrade = min(hs.GetWifeGrade(), bestGrade);

	float rate = hs.GetMusicRate();
	int key = RateToKey(rate);
	ScoresByRate[key].AddScore(hs);
}

vector<float> ScoresForChart::GetPlayedRates() {
	vector<float> o;
	FOREACHM(int, ScoresAtRate, ScoresByRate, i)
		o.emplace_back(KeyToRate(i->first));
	return o;
}

vector<int> ScoresForChart::GetPlayedRateKeys() {
	vector<int> o;
	FOREACHM(int, ScoresAtRate, ScoresByRate, i)
		o.emplace_back(i->first);
	return o;
}


vector<HighScore*> ScoresForChart::GetAllPBPtrs() {
	vector<HighScore*> o;
	FOREACHM(int, ScoresAtRate, ScoresByRate, i)
		o.emplace_back(i->second.PBptr);
	return o;
}

HighScore* PlayerScores::GetChartPBAt(string& ck, float& rate) {
	if (pscores.count(ck))
		return pscores.at(ck).GetPBAt(rate);
	return NULL;
}

HighScore* PlayerScores::GetChartPBUpTo(string& ck, float& rate) {
	if (pscores.count(ck))
		return pscores.at(ck).GetPBUpTo(rate);
	return NULL;
}






void PlayerScores::SortTopSSRPtrs(Skillset ss) {
	TopSSRs[ss].clear();
	FOREACHM(string, ScoresForChart, pscores, i) {
		if (!IsChartLoaded(i->first))
			continue;
		vector<HighScore*> pbs = i->second.GetAllPBPtrs();
		FOREACH(HighScore*, pbs, hs) {
			TopSSRs[ss].emplace_back(*hs);
		}
	}
	auto ssrcomp = [&ss](HighScore* a, HighScore* b) { return (a->GetSkillsetSSR(ss) > b->GetSkillsetSSR(ss)); };
	sort(TopSSRs[ss].begin(), TopSSRs[ss].end(), ssrcomp);
}


// also finish dealing with this later - mina
void PlayerScores::CalcPlayerRating(float& prating, float* pskillsets) {
	float skillsetsum = 0.f;
	FOREACH_ENUM(Skillset, ss) {
		// actually skip overall
		if (ss == Skill_Overall)
			continue;

		SortTopSSRPtrs(ss);
		pskillsets[ss] = AggregateSSRs(ss, 0.f, 10.24f, 1)*0.95f;
		CLAMP(pskillsets[ss], 0.f, 100.f);
		skillsetsum += pskillsets[ss];
	}

	prating = skillsetsum / (NUM_Skillset - 1);
}

float PlayerScores::AggregateSSRs(Skillset ss, float rating, float res, int iter) const {
	double sum;
	do {
		rating += res;
		sum = 0.0;
		for (int i = 0; i < static_cast<int>(TopSSRs[ss].size()); i++) {
			sum += max(0.0, 2.f / erfc(0.1*(TopSSRs[ss][i]->GetSkillsetSSR(ss) - rating)) - 1.5);
		}
	} while (pow(2, rating * 0.1) < sum);
	if (iter == 11)
		return rating;
	return AggregateSSRs(ss, rating - res, res / 2.f, iter + 1);
}




// Write scores to xml
XNode* ScoresAtRate::CreateNode(const int& rate) const {
	XNode* o = new XNode("ScoresAt");

	string rs = IntToString(rate);
	rs = rs.substr(0, 1) + "." + rs.substr(1, 3);
	o->AppendAttr("Rate", rs);

	// should be safe as this is only called if there is at least 1 score (which would be the pb)
	o->AppendAttr("PBKey", PBptr->GetScoreKey());
	o->AppendAttr("BestGrade", GradeToString(bestGrade));

	FOREACHM_CONST(string, HighScore, scores, i)
		o->AppendChild(i->second.CreateEttNode());

	return o;
}

XNode * ScoresForChart::CreateNode(const string& ck) const {
	XNode* o = new XNode("ChartScores");
	o->AppendAttr("Key", ck);

	FOREACHM_CONST(int, ScoresAtRate, ScoresByRate, i)
		o->AppendChild(i->second.CreateNode(i->first));

	return o;
}

XNode * PlayerScores::CreateNode() const {
	XNode* o = new XNode("PlayerScores");

	FOREACHM_CONST(string, ScoresForChart, pscores, ch)
		o->AppendChild(ch->second.CreateNode(ch->first));

	return o;
}


// Read scores from xml
void ScoresAtRate::LoadFromNode(const XNode* node, const RString& ck, const float& rate) {
	RString sk;
	FOREACH_CONST_Child(node, p) {
		p->GetAttrValue("Key", sk);
		scores[sk].LoadFromEttNode(p);

		// Fill in stuff for the highscores
		scores[sk].SetChartKey(ck);
		scores[sk].SetScoreKey(sk);
		scores[sk].SetMusicRate(rate);
	}

	// Set the pbptr
	PBptr = &scores.find(sk)->second;
	bestGrade = PBptr->GetWifeGrade();
}

void ScoresForChart::LoadFromNode(const XNode* node, const RString& ck) {
	RString rs;
	int rate;
	FOREACH_CONST_Child(node, p) {
		ASSERT(p->GetName() == "ScoresAt");
		p->GetAttrValue("Rate", rs);
		rate = 10 * StringToInt(rs.substr(0, 1) + rs.substr(2, 4));
		ScoresByRate[rate].LoadFromNode(p, ck,KeyToRate(rate));
	}
}

void PlayerScores::LoadFromNode(const XNode * node) {
	RString ck;
	FOREACH_CONST_Child(node, p) {
		ASSERT(p->GetName() == "ChartScores");
		p->GetAttrValue("Key", ck);
		pscores[ck].LoadFromNode(p, ck);
	}
}


#include "LuaBinding.h"

class LunaScoresAtRate: public Luna<ScoresAtRate>
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

	LunaScoresAtRate()
	{
		ADD_METHOD(GetScores);
	}
};

LUA_REGISTER_CLASS(ScoresAtRate)

class LunaScoresForChart : public Luna<ScoresForChart>
{
public:
	static int GetScoresAtRates(T* p, lua_State *L) {
		lua_newtable(L);
		vector<float> rates = p->GetPlayedRates();
		for (size_t i = 0; i < rates.size(); ++i) {
			p->ScoresByRate[rates[i]].PushSelf(L);	// broken
			lua_rawseti(L, -2, i + 1);
		}

		return 1;
	}


	LunaScoresForChart()
	{
		ADD_METHOD(GetScoresAtRates);
	}
};

LUA_REGISTER_CLASS(ScoresForChart)
