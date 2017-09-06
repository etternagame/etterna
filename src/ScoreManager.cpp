#include "global.h"
#include "RageLog.h"
#include "HighScore.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "GameConstantsAndTypes.h"
#include "Foreach.h"
#include "ScoreManager.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"
#include "Song.h"
#include "MinaCalc.h"
#include "NoteDataStructures.h"
#include "NoteData.h"
#include "RageTimer.h"

ScoreManager* SCOREMAN = NULL;

ScoreManager::ScoreManager() {
	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring(L, "SCOREMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

ScoreManager::~ScoreManager() {
	// Unregister with Lua.
	LUA->UnsetGlobal("SCOREMAN");
}




ScoresAtRate::ScoresAtRate() {
	bestGrade = Grade_Invalid;
	scores.clear();
	PBptr = nullptr;
}

void ScoresAtRate::AddScore(HighScore& hs) {
	const string& key = hs.GetScoreKey();
	bestGrade = min(hs.GetWifeGrade(), bestGrade);
	scores.emplace(key, hs);

	if(!PBptr || PBptr->GetWifeScore() < hs.GetWifeScore())
		PBptr = &scores.find(key)->second;

	SCOREMAN->RegisterScore(&scores.find(key)->second);
	SCOREMAN->AddToKeyedIndex(&scores.find(key)->second);
}

vector<string> ScoresAtRate::GetSortedKeys() {
	map<float, string, greater<float>> tmp;
	vector<string> o;
	FOREACHUM(string, HighScore, scores, i)
		 tmp.emplace(i->second.GetWifeScore(), i->first);
	FOREACHM(float, string, tmp, j)
		 o.emplace_back(j->second);
	return o;
}

/*
const vector<HighScore*> ScoresAtRate::GetScores() const {
	map<float, string, greater<float>> tmp;
	vector<HighScore*> o;
	FOREACHUM_CONST(string, HighScore, scores, i)
		tmp.emplace(i->second.GetWifeScore(), i->first);
	FOREACHM(float, string, tmp, j)
		o.emplace_back(j->second);
	return o;
}
*/

void ScoreManager::RatingOverTime() {
	auto compdate = [](HighScore* a, HighScore* b) { return (a->GetDateTime() < b->GetDateTime()); };


	vector<bool> wasvalid;
	sort(AllScores.begin(), AllScores.end(), compdate);

	for (auto& n : AllScores) {
		wasvalid.push_back(n->GetEtternaValid());
		n->SetEtternaValid(false);
	}

	float doot = 10.f;
	float doot2[8];
	LOG->Warn("wer");
	if (AllScores.empty())
		return;

	DateTime lastvalidday = AllScores.front()->GetDateTime();
	lastvalidday.StripTime();

	CalcPlayerRating(doot, doot2);
	LOG->Warn(lastvalidday.GetString());

	DateTime finalvalidday = AllScores.back()->GetDateTime();
	finalvalidday.StripTime();
	while (lastvalidday != finalvalidday) {
		for (auto& n : AllScores) {
			DateTime date = n->GetDateTime();
			date.StripTime();

			if (lastvalidday < date) {
				lastvalidday = date;
				break;
			}

			n->SetEtternaValid(true);
		}
		CalcPlayerRating(doot, doot2);
		LOG->Trace("%f", doot);
	}
}

ScoresForChart::ScoresForChart() {
	bestGrade = Grade_Invalid;
	ScoresByRate.clear();
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

vector<string> ScoresForChart::GetPlayedRateDisplayStrings() {
	vector<float> rates = GetPlayedRates();
	vector<string> o;
	for(size_t i = 0; i < rates.size(); ++i)
		o.emplace_back(RateKeyToDisplayString(rates[i]));
	return o;
}

string ScoresForChart::RateKeyToDisplayString(float rate) {
	string rs = ssprintf("%.2f", rate);
	int j = 1;
	if (rs.find_last_not_of('0') == rs.find('.'))
		j = 2;
	rs.erase(rs.find_last_not_of('0') + j, rs.npos);
	rs.append("x");
	return rs;
}


vector<HighScore*> ScoresForChart::GetAllPBPtrs() {
	vector<HighScore*> o;
	FOREACHM(int, ScoresAtRate, ScoresByRate, i)
		o.emplace_back(i->second.PBptr);
	return o;
}

HighScore* ScoreManager::GetChartPBAt(const string& ck, float& rate) {
	if (pscores.count(ck))
		return pscores.at(ck).GetPBAt(rate);
	return NULL;
}

HighScore* ScoreManager::GetChartPBUpTo(const string& ck, float& rate) {
	if (pscores.count(ck))
		return pscores.at(ck).GetPBUpTo(rate);
	return NULL;
}



static const float ld_update = 0.02f;
void ScoreManager::RecalculateSSRs(LoadingWindow *ld) {
	RageTimer ld_timer;
	if (ld) {
		ld_timer.Touch();
		ld->SetIndeterminate(false);
		ld->SetTotalWork(AllScores.size());
		ld->SetText("Updating SSR Calculations for Scores...");
	}

	int scoreindex = 0;
	for(size_t i = 0; i < AllScores.size(); ++i) {
		if (ld && ld_timer.Ago() > ld_update) {
			ld_timer.Touch();
			ld->SetProgress(scoreindex);
		}
		++scoreindex;

		HighScore* hs = AllScores[i];
		if (hs->GetSSRCalcVersion() == GetCalcVersion())
			continue;

		Steps* steps = SONGMAN->GetStepsByChartkey(hs->GetChartKey());

		if (!steps)
			continue;

		if (!steps->IsRecalcValid()) {
			hs->ResetSkillsets();
			continue;
		}

		float ssrpercent = hs->GetSSRNormPercent();
		float musicrate = hs->GetMusicRate();

		if (ssrpercent <= 0.f || hs->GetGrade() == Grade_Failed) {
			hs->ResetSkillsets();
			continue;
		}

		TimingData* td = steps->GetTimingData();
		NoteData nd;
		steps->GetNoteData(nd);

		nd.LogNonEmptyRows();
		auto& nerv = nd.GetNonEmptyRowVector();
		auto& etaner = td->BuildAndGetEtaner(nerv);
		auto& serializednd = nd.SerializeNoteData(etaner);

		auto dakine = MinaSDCalc(serializednd, steps->GetNoteData().GetNumTracks(), musicrate, ssrpercent, 1.f, td->HasWarps());
		FOREACH_ENUM(Skillset, ss)
			hs->SetSkillsetSSR(ss, dakine[ss]);
		hs->SetSSRCalcVersion(GetCalcVersion());

		td->UnsetEtaner();
		nd.UnsetNerv();
		nd.UnsetSerializedNoteData();
		steps->Compress();
	}
	return;
}

// should deal with this misnomer - mina
void ScoreManager::EnableAllScores() {
	for (size_t i = 0; i < AllScores.size(); ++i)
		AllScores[i]->SetEtternaValid(true);

	return;
}

void ScoreManager::CalcPlayerRating(float& prating, float* pskillsets) {
	vector<float> skillz;
	FOREACH_ENUM(Skillset, ss) {
		// actually skip overall, and jack stamina for now
		if (ss == Skill_Overall || ss == Skill_JackStamina)
			continue;

		SortTopSSRPtrs(ss);
		pskillsets[ss] = AggregateSSRs(ss, 0.f, 10.24f, 1)*0.975f;
		CLAMP(pskillsets[ss], 0.f, 100.f);
		skillz.push_back (pskillsets[ss]);
	}

	sort(skillz.begin(), skillz.end());

	skillz[0] *= 0.1f;
	skillz[1] *= 0.1f;
	skillz[2] *= 0.1f;
	skillz[3] *= 0.1f;
	skillz[4] *= 0.25f;
	skillz[5] *= 0.35f;

	float skillsetsum = 0.f;
	for (auto& n : skillz)
		skillsetsum += n;

	prating = skillsetsum;
}

// perhaps we will need a generalized version again someday, but not today
float ScoreManager::AggregateSSRs(Skillset ss, float rating, float res, int iter) const {
	double sum;
	do {
		rating += res;
		sum = 0.0;
		for (int i = 0; i < static_cast<int>(TopSSRs.size()); i++) {
			if(TopSSRs[i]->GetSSRCalcVersion() == GetCalcVersion() && TopSSRs[i]->GetEtternaValid())
				sum += max(0.0, 2.f / erfc(0.1*(TopSSRs[i]->GetSkillsetSSR(ss) - rating)) - 1.5);
		}
	} while (pow(2, rating * 0.1) < sum);
	if (iter == 11)
		return rating;
	return AggregateSSRs(ss, rating - res, res / 2.f, iter + 1);
}

void ScoreManager::SortTopSSRPtrs(Skillset ss) {
	TopSSRs.clear();
	FOREACHUM(string, ScoresForChart, pscores, i) {
		if (!SONGMAN->IsChartLoaded(i->first))
			continue;
		vector<HighScore*> pbs = i->second.GetAllPBPtrs();
		FOREACH(HighScore*, pbs, hs) {
			TopSSRs.emplace_back(*hs);
		}
	}

	auto ssrcomp = [&ss](HighScore* a, HighScore* b) { return (a->GetSkillsetSSR(ss) > b->GetSkillsetSSR(ss)); };
	sort(TopSSRs.begin(), TopSSRs.end(), ssrcomp);
}

HighScore* ScoreManager::GetTopSSRHighScore(unsigned int rank, int ss) {
	if (rank < 0)
		rank = 0;

	if (ss >= 0 && ss < NUM_Skillset && rank < TopSSRs.size())
		return TopSSRs[rank];

	return NULL;
}

void ScoreManager::ImportScore(const HighScore& hs_) {
	HighScore hs = hs_;

	// don't import duplicated scores
	// this may have strange ramifications - mina
	// actually i'll just disable this for the time being and give myself time to test it later
	//if(!ScoresByKey.count(hs.GetScoreKey()))

	pscores[hs.GetChartKey()].AddScore(hs);
}





// Write scores to xml
XNode* ScoresAtRate::CreateNode(const int& rate) const {
	XNode* o = new XNode("ScoresAt");
	int saved = 0;

	// prune out sufficiently low scores
	FOREACHUM_CONST(string, HighScore, scores, i) {
		if (i->second.GetWifeScore() > SCOREMAN->minpercent) {
			o->AppendChild(i->second.CreateEttNode());
			saved++;
		}
	}

	if (o->ChildrenEmpty())
		return o;

	string rs = ssprintf("%.3f", static_cast<float>(rate) / 10000.f);
	// should be safe as this is only called if there is at least 1 score (which would be the pb)
	o->AppendAttr("PBKey", PBptr->GetScoreKey());
	o->AppendAttr("BestGrade", GradeToString(bestGrade));
	o->AppendAttr("Rate", rs);

	return o;
}

XNode * ScoresForChart::CreateNode(const string& ck) const {
	Chart loot = ch;
	loot.FromKey(ck);	// needs to be here (or somewhere along the line, maybe not exactly here) -mina
	XNode* o = loot.CreateNode(false);

	FOREACHM_CONST(int, ScoresAtRate, ScoresByRate, i) {
		auto node = i->second.CreateNode(i->first);
		if (!node->ChildrenEmpty())
			o->AppendChild(node);
	}
	return o;
}

XNode * ScoreManager::CreateNode() const {
	XNode* o = new XNode("PlayerScores");
	FOREACHUM_CONST(string, ScoresForChart, pscores, ch) {
		auto node = ch->second.CreateNode(ch->first);
		if (!node->ChildrenEmpty())
			o->AppendChild(node);
	}

	return o;
}


// Read scores from xml
void ScoresAtRate::LoadFromNode(const XNode* node, const string& ck, const float& rate) {
	RString sk;
	FOREACH_CONST_Child(node, p) {
		p->GetAttrValue("Key", sk);
		scores[sk].LoadFromEttNode(p);

		// Set any pb
		if(PBptr == nullptr)
			PBptr = &scores.find(sk)->second;
		else {
			// update pb if a better score is found
			if (PBptr->GetWifeScore() < scores[sk].GetWifeScore())
				PBptr = &scores.find(sk)->second;
		}

		// Fill in stuff for the highscores
		scores[sk].SetChartKey(ck);
		scores[sk].SetScoreKey(sk);
		scores[sk].SetMusicRate(rate);

		bestGrade = min(scores[sk].GetWifeGrade(), bestGrade);

		// Very awkward, need to figure this out better so there isn't unnecessary redundancy between loading and adding
		SCOREMAN->RegisterScore(&scores.find(sk)->second);
		SCOREMAN->AddToKeyedIndex(&scores.find(sk)->second);
	}
}

void ScoresForChart::LoadFromNode(const XNode* node, const string& ck) {
	RString rs = "";
	int rate;

	if(node->GetName() == "Chart")
		ch.LoadFromNode(node);

	if (node->GetName() == "ChartScores") {
		node->GetAttrValue("Key", rs);
		ch.key = rs;
		node->GetAttrValue("Song", ch.lastsong);
		node->GetAttrValue("Pack", ch.lastpack);
	}

	FOREACH_CONST_Child(node, p) {
		ASSERT(p->GetName() == "ScoresAt");
		p->GetAttrValue("Rate", rs);
		rate = 10 * StringToInt(rs.substr(0, 1) + rs.substr(2, 4));
		ScoresByRate[rate].LoadFromNode(p, ck, KeyToRate(rate));
		bestGrade = min(ScoresByRate[rate].bestGrade, bestGrade);
	}
}

void ScoreManager::LoadFromNode(const XNode * node) {
	FOREACH_CONST_Child(node, p) {
		//ASSERT(p->GetName() == "Chart");
		RString tmp;
		p->GetAttrValue("Key", tmp);
		string doot = SONGMAN->ReconcileBustedKeys(tmp);
		const string ck = doot;
		pscores[ck].LoadFromNode(p, ck);
	}
}



ScoresAtRate* ScoresForChart::GetScoresAtRate(const int& rate) {
	auto it = ScoresByRate.find(rate);
	if (it != ScoresByRate.end())
		return &it->second;
	return NULL;
}

ScoresForChart* ScoreManager::GetScoresForChart(const string& ck) {
	auto it = pscores.find(ck);
	if (it != pscores.end())
		return &it->second;
	return NULL;
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
	LunaScoresForChart()
	= default;
};

LUA_REGISTER_CLASS(ScoresForChart)



class LunaScoreManager : public Luna<ScoreManager>
{
public:
	static int GetScoresByKey(T* p, lua_State *L) {
		const string& ck = SArg(1);
		ScoresForChart* scores = p->GetScoresForChart(ck);

		if (scores) {
			lua_newtable(L);
			vector<int> ratekeys = scores->GetPlayedRateKeys();
			vector<string> ratedisplay = scores->GetPlayedRateDisplayStrings();
			for (size_t i = 0; i < ratekeys.size(); ++i) {
				LuaHelpers::Push(L, ratedisplay[i]);
				scores->GetScoresAtRate(ratekeys[i])->PushSelf(L);
				lua_rawset(L, -3);
			}

			return 1;
		}

		lua_pushnil(L);
		return 1;
	}

	static int SortSSRs(T* p, lua_State *L) {
		p->SortTopSSRPtrs(Enum::Check<Skillset>(L, 1));
		return 1;
	}

	static int ValidateAllScores(T* p, lua_State *L) {
		p->EnableAllScores();
		return 1;
	}

	static int GetTopSSRHighScore(T* p, lua_State *L) {
		HighScore* ths = p->GetTopSSRHighScore(IArg(1) - 1, Enum::Check<Skillset>(L, 2));
		if (ths)
			ths->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	static int GetMostRecentScore(T* p, lua_State *L) {
		// this _should_ always be viable if only called from eval
		HighScore* last = p->GetMostRecentScore();
		last->PushSelf(L);
		return 1;
	}

	LunaScoreManager()
	{
		ADD_METHOD(GetScoresByKey);
		ADD_METHOD(SortSSRs);
		ADD_METHOD(ValidateAllScores);
		ADD_METHOD(GetTopSSRHighScore);
		ADD_METHOD(GetMostRecentScore);
	}
};

LUA_REGISTER_CLASS(ScoreManager)
