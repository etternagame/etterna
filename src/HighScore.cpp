#include "global.h"
#include "RageLog.h"
#include "HighScore.h"
#include "PrefsManager.h"
#include "GameConstantsAndTypes.h"
#include "PlayerNumber.h"
#include "ThemeManager.h"
#include "XmlFile.h"
#include "Foreach.h"
#include "RadarValues.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include "CryptManager.h"
#include "ProfileManager.h"

ThemeMetric<string> EMPTY_NAME("HighScore","EmptyName");

const string REPLAY_DIR = "Save/Replays/";

struct HighScoreImpl
{
	string	sName;	// name that shows in the machine's ranking screen
	
	/* a half-misnomer now- since all scores are keyed by the chart key this should never change/be different,
	but its historical correctness is still correct, though it should prolly be renamed tbh -mina*/ 
	string ChartKey;

	string ScoreKey;
	int SSRCalcVersion;
	Grade grade;
	unsigned int iScore;
	float fPercentDP;
	float fWifeScore;
	float fWifePoints;
	float fSSRNormPercent;
	float fSurviveSeconds;
	float fMusicRate;
	float fJudgeScale;
	bool bNoChordCohesion;
	bool bEtternaValid;
	vector<float> vOffsetVector;
	vector<int> vNoteRowVector;
	vector<int> vRescoreJudgeVector;
	unsigned int iMaxCombo;			// maximum combo obtained [SM5 alpha 1a+]
	StageAward stageAward;	// stage award [SM5 alpha 1a+]
	PeakComboAward peakComboAward;	// peak combo award [SM5 alpha 1a+]
	string	sModifiers;
	DateTime dateTime;		// return value of time() for when the highscore object was created (immediately after achieved)
	string sPlayerGuid;	// who made this high score
	string sMachineGuid;	// where this high score was made
	int iProductID;
	int iTapNoteScores[NUM_TapNoteScore];
	int iHoldNoteScores[NUM_HoldNoteScore];
	float fSkillsetSSRs[NUM_Skillset];
	string ValidationKeys[NUM_ValidationKey];
	RadarValues radarValues;
	float fLifeRemainingSeconds;
	bool bDisqualified;
	string ValidationKey;

	HighScoreImpl();
	XNode *CreateNode() const;
	XNode *CreateEttNode() const;
	void LoadFromNode( const XNode *pNode );
	void LoadFromEttNode(const XNode *pNode);
	Grade GetWifeGrade() const;
	void UnloadReplayData();
	void ResetSkillsets();

	bool WriteReplayData();

	float RescoreToWifeTS(float ts);

	RString OffsetsToString(vector<float> v) const;
	vector<float> OffsetsToVector(RString s);
	RString NoteRowsToString(vector<int> v) const;
	vector<int> NoteRowsToVector(RString s);

	bool is39import = false;

	bool operator==( const HighScoreImpl& other ) const;
	bool operator!=( const HighScoreImpl& other ) const { return !(*this == other); }
};

bool HighScoreImpl::operator==( const HighScoreImpl& other ) const 
{
#define COMPARE(x)	if( x!=other.x )	return false;
	COMPARE( sName );
	COMPARE( grade );
	COMPARE( iScore );
	COMPARE( iMaxCombo );
	COMPARE( stageAward );
	COMPARE( peakComboAward );
	COMPARE( fPercentDP );
	COMPARE( fSurviveSeconds );
	COMPARE( sModifiers );
	COMPARE( dateTime );
	COMPARE( sPlayerGuid );
	COMPARE( sMachineGuid );
	COMPARE( iProductID );
	FOREACH_ENUM( TapNoteScore, tns )
		COMPARE( iTapNoteScores[tns] );
	FOREACH_ENUM( HoldNoteScore, hns )
		COMPARE( iHoldNoteScores[hns] );
	FOREACH_ENUM( Skillset, ss)
		COMPARE(fSkillsetSSRs[ss]);
	COMPARE( radarValues );
	COMPARE( fLifeRemainingSeconds );
	COMPARE( bDisqualified );
#undef COMPARE

	return true;
}

void HighScoreImpl::UnloadReplayData() {
	vector<int> tmpi;
	vector<float> tmpf;
	vNoteRowVector.swap(tmpi);
	vOffsetVector.swap(tmpf);
}

Grade HighScoreImpl::GetWifeGrade() const {
	if (grade == Grade_Failed)
		return Grade_Failed;

	if (fWifeScore >= 0.9997f)
		return Grade_Tier01;
	if (fWifeScore >= 0.9975f)
		return Grade_Tier02;
	if (fWifeScore >= 0.93f)
		return Grade_Tier03;
	if (fWifeScore >= 0.8f)
		return Grade_Tier04;
	if (fWifeScore >= 0.7f)
		return Grade_Tier05;
	if (fWifeScore >= 0.6f)
		return Grade_Tier06;
	return Grade_Tier07;
}

RString HighScoreImpl::OffsetsToString(vector<float> v) const{
	RString o = "";
	if (v.empty())
		return o;

	o = to_string(v[0]);
	for (size_t i = 1; i < v.size(); i++)
		o.append("," + to_string(v[i]));
	return o;
}

RString HighScoreImpl::NoteRowsToString(vector<int> v) const {
	RString o = "";
	if (v.empty())
		return o;

	o = to_string(v[0]);
	for (size_t i = 1; i < v.size(); i++)
		o.append("," + to_string(v[i]));
	return o;
}

vector<float> HighScoreImpl::OffsetsToVector(RString s) {
	vector<float> o;
	size_t startpos = 0;

	if (s == "")
		return o;

	do {
		size_t pos;
		pos = s.find(",", startpos);
		if (pos == s.npos)
			pos = s.size();

		if (pos - startpos > 0) {
			if (startpos == 0 && pos - startpos == s.size())
				o.emplace_back(StringToFloat(s));
			else {
				const RString AddRString = s.substr(startpos, pos - startpos);
				o.emplace_back(StringToFloat(AddRString));
			}
		}
		startpos = pos + 1;
	} while (startpos <= s.size());
	return o;
}

vector<int> HighScoreImpl::NoteRowsToVector(RString s) {
	vector<int> o;
	size_t startpos = 0;

	if (s == "")
		return o;

	do {
		size_t pos;
		pos = s.find(",", startpos);
		if (pos == s.npos)
			pos = s.size();

		if (pos - startpos > 0) {
			if (startpos == 0 && pos - startpos == s.size())
				o.emplace_back(StringToInt(s));
			else {
				const RString AddRString = s.substr(startpos, pos - startpos);
				o.emplace_back(StringToInt(AddRString));
			}
		}
		startpos = pos + 1;
	} while (startpos <= s.size());
	return o;
}

void HighScoreImpl::ResetSkillsets() {
	FOREACH_ENUM(Skillset, ss)
		fSkillsetSSRs[ss] = 0.f;
}

HighScoreImpl::HighScoreImpl()
{
	sName = "";
	ChartKey = "";
	ScoreKey = "";
	SSRCalcVersion = 0;
	grade = Grade_NoData;
	iScore = 0;
	fPercentDP = 0.f;
	fWifeScore = 0.f;
	fWifePoints = 0.f;
	fSSRNormPercent = 0.f;
	fMusicRate = 0.f;
	fJudgeScale = 0.f;
	bEtternaValid = true;
	vOffsetVector.clear();
	vNoteRowVector.clear();
	vRescoreJudgeVector.clear();
	fSurviveSeconds = 0.f;
	iMaxCombo = 0;
	stageAward = StageAward_Invalid;
	peakComboAward = PeakComboAward_Invalid;
	sModifiers = "";
	dateTime.Init();
	sPlayerGuid = "";
	sMachineGuid = "";
	iProductID = 0;
	ZERO( iTapNoteScores );
	ZERO( iHoldNoteScores );
	ZERO( fSkillsetSSRs );
	radarValues.MakeUnknown();
	fLifeRemainingSeconds = 0;
	string ValidationKey = "";
}

XNode *HighScoreImpl::CreateNode() const
{
	XNode *pNode = new XNode( "HighScore" );

	// TRICKY:  Don't write "name to fill in" markers.
	pNode->AppendChild( "Name",				IsRankingToFillIn(sName) ? RString("") : sName );
	pNode->AppendChild( "HistoricChartKey", ChartKey);
	pNode->AppendChild( "ScoreKey",			ScoreKey);
	pNode->AppendChild( "SSRCalcVersion",	SSRCalcVersion);
	pNode->AppendChild( "Grade",			GradeToString(grade) );
	pNode->AppendChild( "Score",			iScore );
	pNode->AppendChild( "PercentDP",		fPercentDP );
	pNode->AppendChild( "WifeScore",		fWifeScore);
	pNode->AppendChild( "SSRNormPercent",	fSSRNormPercent);
	pNode->AppendChild( "Rate",				fMusicRate);
	pNode->AppendChild( "JudgeScale",		fJudgeScale);
	pNode->AppendChild( "NoChordCohesion",	bNoChordCohesion);
	pNode->AppendChild( "EtternaValid",		bEtternaValid);

	if (vOffsetVector.size() > 1) {
		pNode->AppendChild("Offsets", OffsetsToString(vOffsetVector));
		pNode->AppendChild("NoteRows", NoteRowsToString(vNoteRowVector));
	}
	
	pNode->AppendChild( "SurviveSeconds",	fSurviveSeconds );
	pNode->AppendChild( "MaxCombo",			iMaxCombo );
	pNode->AppendChild( "StageAward",		StageAwardToString(stageAward) );
	pNode->AppendChild( "PeakComboAward",	PeakComboAwardToString(peakComboAward) );
	pNode->AppendChild( "Modifiers",		sModifiers );
	pNode->AppendChild( "DateTime",			dateTime.GetString() );
	pNode->AppendChild( "PlayerGuid",		sPlayerGuid );
	pNode->AppendChild( "MachineGuid",		sMachineGuid );
	pNode->AppendChild( "ProductID",		iProductID );

	XNode* pTapNoteScores = pNode->AppendChild( "TapNoteScores" );
	FOREACH_ENUM( TapNoteScore, tns )
		if( tns != TNS_None )	// HACK: don't save meaningless "none" count
			pTapNoteScores->AppendChild( TapNoteScoreToString(tns), iTapNoteScores[tns] );

	XNode* pHoldNoteScores = pNode->AppendChild( "HoldNoteScores" );
	FOREACH_ENUM( HoldNoteScore, hns )
		if( hns != HNS_None )	// HACK: don't save meaningless "none" count
			pHoldNoteScores->AppendChild( HoldNoteScoreToString(hns), iHoldNoteScores[hns] );

	// dont bother writing skillset ssrs for non-applicable scores
	if (fWifeScore > 0.f) {
		XNode* pSkillsetSSRs = pNode->AppendChild("SkillsetSSRs");
		FOREACH_ENUM(Skillset, ss)
			pSkillsetSSRs->AppendChild(SkillsetToString(ss), fSkillsetSSRs[ss]);
	}

	pNode->AppendChild( radarValues.CreateNode() );
	pNode->AppendChild( "LifeRemainingSeconds",	fLifeRemainingSeconds );
	pNode->AppendChild( "Disqualified",		bDisqualified);
	return pNode;
}

XNode *HighScoreImpl::CreateEttNode() const {
	XNode *pNode = new XNode("Score");

	if (ScoreKey == "")
		pNode->AppendAttr("Key", "S" + BinaryToHex(CryptManager::GetSHA1ForString(dateTime.GetString())));
	else
		pNode->AppendAttr("Key", ScoreKey);
	
	pNode->AppendChild("SSRCalcVersion", SSRCalcVersion);
	pNode->AppendChild("Grade", GradeToString(GetWifeGrade()));
	pNode->AppendChild("WifeScore", fWifeScore);
	
	if (fWifePoints > 0.f)
		pNode->AppendChild("WifePoints", fWifePoints);
	
	pNode->AppendChild("SSRNormPercent", fSSRNormPercent);
	pNode->AppendChild("JudgeScale", fJudgeScale);
	pNode->AppendChild("NoChordCohesion", bNoChordCohesion);
	pNode->AppendChild("EtternaValid", bEtternaValid);
	pNode->AppendChild("SurviveSeconds", fSurviveSeconds);
	pNode->AppendChild("MaxCombo", iMaxCombo);
	pNode->AppendChild("Modifiers", sModifiers);
	pNode->AppendChild("DateTime", dateTime.GetString());

	XNode* pTapNoteScores = pNode->AppendChild("TapNoteScores");
	FOREACH_ENUM(TapNoteScore, tns)
		if (tns != TNS_None && tns != TNS_CheckpointMiss && tns != TNS_CheckpointHit)
			pTapNoteScores->AppendChild(TapNoteScoreToString(tns), iTapNoteScores[tns]);

	XNode* pHoldNoteScores = pNode->AppendChild("HoldNoteScores");
	FOREACH_ENUM(HoldNoteScore, hns)
		if (hns != HNS_None)	// HACK: don't save meaningless "none" count
			pHoldNoteScores->AppendChild(HoldNoteScoreToString(hns), iHoldNoteScores[hns]);

	// dont bother writing skillset ssrs for non-applicable scores
	if (fWifeScore > 0.f && grade != Grade_Failed && fSkillsetSSRs[Skill_Overall] > 0.f) {
		XNode* pSkillsetSSRs = pNode->AppendChild("SkillsetSSRs");
		FOREACH_ENUM(Skillset, ss)
			pSkillsetSSRs->AppendChild(SkillsetToString(ss), FloatToString(fSkillsetSSRs[ss]).substr(0, 5));
	}

	XNode* pValidationKeys = pNode->AppendChild("ValidationKeys");
	pValidationKeys->AppendChild(ValidationKeyToString(ValidationKey_Brittle), ValidationKeys[ValidationKey_Brittle]);
	pValidationKeys->AppendChild(ValidationKeyToString(ValidationKey_Weak), ValidationKeys[ValidationKey_Weak]);

	return pNode;
}

void HighScoreImpl::LoadFromEttNode(const XNode *pNode) {
	//ASSERT(pNode->GetName() == "Score");

	RString s;	
	pNode->GetChildValue("SSRCalcVersion", SSRCalcVersion);
	pNode->GetChildValue("Grade", s);
	grade = StringToGrade(s);
	pNode->GetChildValue("WifeScore", fWifeScore);
	pNode->GetChildValue("WifePoints", fWifePoints);
	pNode->GetChildValue("SSRNormPercent", fSSRNormPercent);
	pNode->GetChildValue("Rate", fMusicRate);
	pNode->GetChildValue("JudgeScale", fJudgeScale);
	pNode->GetChildValue("NoChordCohesion", bNoChordCohesion);
	pNode->GetChildValue("EtternaValid", bEtternaValid);
	pNode->GetChildValue("SurviveSeconds", fSurviveSeconds);
	pNode->GetChildValue("MaxCombo", iMaxCombo);
	pNode->GetChildValue("Modifiers", s); sModifiers = s;
	pNode->GetChildValue("DateTime", s); dateTime.FromString(s);
	pNode->GetChildValue("ScoreKey", s); ScoreKey = s;

	const XNode* pTapNoteScores = pNode->GetChild("TapNoteScores");
	if (pTapNoteScores)
		FOREACH_ENUM(TapNoteScore, tns)
		pTapNoteScores->GetChildValue(TapNoteScoreToString(tns), iTapNoteScores[tns]);

	const XNode* pHoldNoteScores = pNode->GetChild("HoldNoteScores");
	if (pHoldNoteScores)
		FOREACH_ENUM(HoldNoteScore, hns)
		pHoldNoteScores->GetChildValue(HoldNoteScoreToString(hns), iHoldNoteScores[hns]);

	if (fWifeScore > 0.f) {
		const XNode* pSkillsetSSRs = pNode->GetChild("SkillsetSSRs");
		if (pSkillsetSSRs)
			FOREACH_ENUM(Skillset, ss)
			pSkillsetSSRs->GetChildValue(SkillsetToString(ss), fSkillsetSSRs[ss]);
	}

	if (fWifeScore > 0.f) {
		const XNode* pValidationKeys = pNode->GetChild("ValidationKeys");
		if (pValidationKeys) {
			pValidationKeys->GetChildValue(ValidationKeyToString(ValidationKey_Brittle), s); ValidationKeys[ValidationKey_Brittle] = s;
			pValidationKeys->GetChildValue(ValidationKeyToString(ValidationKey_Weak), s); ValidationKeys[ValidationKey_Weak] = s;
		}
	}

	if (ScoreKey == "")
		ScoreKey = "S" + BinaryToHex(CryptManager::GetSHA1ForString(dateTime.GetString()));

	// Validate input.
	grade = clamp(grade, Grade_Tier01, Grade_Failed);
}

void HighScoreImpl::LoadFromNode(const XNode *pNode)
{
	ASSERT(pNode->GetName() == "HighScore");

	RString s;

	pNode->GetChildValue("Name", s); sName = s;
	pNode->GetChildValue("HistoricChartKey", s); ChartKey = s;
	pNode->GetChildValue("SSRCalcVersion",		SSRCalcVersion);
	pNode->GetChildValue("Grade", s);
	grade = StringToGrade(s);
	pNode->GetChildValue("Score",				iScore);
	pNode->GetChildValue("PercentDP",			fPercentDP);
	pNode->GetChildValue("WifeScore",			fWifeScore);
	pNode->GetChildValue("SSRNormPercent",		fSSRNormPercent);
	pNode->GetChildValue("Rate",				fMusicRate);
	pNode->GetChildValue("JudgeScale",			fJudgeScale);
	pNode->GetChildValue("NoChordCohesion",		bNoChordCohesion);
	pNode->GetChildValue("EtternaValid",		bEtternaValid);
	pNode->GetChildValue("Offsets", s);			vOffsetVector = OffsetsToVector(s);
	pNode->GetChildValue("NoteRows", s);		vNoteRowVector = NoteRowsToVector(s);
	pNode->GetChildValue("SurviveSeconds",		fSurviveSeconds);
	pNode->GetChildValue("MaxCombo",			iMaxCombo);
	pNode->GetChildValue("StageAward", s);		stageAward = StringToStageAward(s);
	pNode->GetChildValue("PeakComboAward", s);	peakComboAward = StringToPeakComboAward(s);
	pNode->GetChildValue("Modifiers", s); sModifiers = s;
	if (fMusicRate == 0.f) {
		size_t ew = sModifiers.find("xMusic");
		size_t dew = string::npos;
		if (ew == string::npos)
			fMusicRate = 1.f;
		else {
			dew = sModifiers.find_last_of('.', ew) - 1;
			RString loot = sModifiers.substr(dew, ew - dew);
			fMusicRate = StringToFloat(loot);
		}
	}		
	pNode->GetChildValue( "DateTime",		s ); dateTime.FromString( s );
	pNode->GetChildValue("ScoreKey", s); ScoreKey = s;

	if (fWifeScore > 0.f)
		ScoreKey = "S" + BinaryToHex(CryptManager::GetSHA1ForString(dateTime.GetString()));
	else
		ScoreKey = "";

	pNode->GetChildValue("PlayerGuid", s); sPlayerGuid = s;
	pNode->GetChildValue("MachineGuid", s); sMachineGuid = s;
	pNode->GetChildValue( "ProductID",		iProductID );

	const XNode* pTapNoteScores = pNode->GetChild( "TapNoteScores" );
	if( pTapNoteScores )
		FOREACH_ENUM( TapNoteScore, tns )
			pTapNoteScores->GetChildValue( TapNoteScoreToString(tns), iTapNoteScores[tns] );

	const XNode* pHoldNoteScores = pNode->GetChild( "HoldNoteScores" );
	if (pHoldNoteScores)
		FOREACH_ENUM(HoldNoteScore, hns)
			pHoldNoteScores->GetChildValue(HoldNoteScoreToString(hns), iHoldNoteScores[hns]);

	if (fWifeScore > 0.f) {
		const XNode* pSkillsetSSRs = pNode->GetChild("SkillsetSSRs");
		if (pSkillsetSSRs)
			FOREACH_ENUM(Skillset, ss)
			pSkillsetSSRs->GetChildValue(SkillsetToString(ss), fSkillsetSSRs[ss]);
	}
	
	const XNode* pRadarValues = pNode->GetChild( "RadarValues" );
	if( pRadarValues )
		radarValues.LoadFromNode( pRadarValues );
	pNode->GetChildValue( "LifeRemainingSeconds",	fLifeRemainingSeconds );
	pNode->GetChildValue( "Disqualified",		bDisqualified);

	// special test case stuff - mina
	//if (vOffsetVector.size() > 1 && fWifeScore == 0.f)
	//	fWifeScore = RescoreToWifeTS(fJudgeScale);
	if (vNoteRowVector.size() + vOffsetVector.size() > 2 && (vNoteRowVector.size() == vOffsetVector.size()) && fWifeScore > 0.f) {
		bool writesuccess = WriteReplayData();

		// ensure data is written out somewhere else before destroying it
		if (writesuccess)
			UnloadReplayData();
	}
	// Validate input.

	// 3.9 conversion stuff (wtf is this code??) -mina
	if (pTapNoteScores)
		FOREACH_ENUM(TapNoteScore, tns) {
		pTapNoteScores->GetChildValue(TapNoteScoreToString(tns), iTapNoteScores[tns]);
		if (tns == TNS_W1 && iTapNoteScores[tns] == 0) {
			int old = iTapNoteScores[tns];
			pTapNoteScores->GetChildValue("Marvelous", iTapNoteScores[tns]);

			// only mark as import if loading the other tag changes the values
			if(old != iTapNoteScores[tns])
				is39import = true;
		}
		else if (tns == TNS_W2 && iTapNoteScores[tns] == 0)
			pTapNoteScores->GetChildValue("Perfect", iTapNoteScores[tns]);
		else if (tns == TNS_W3 && iTapNoteScores[tns] == 0)
			pTapNoteScores->GetChildValue("Great", iTapNoteScores[tns]);
		else if (tns == TNS_W4 && iTapNoteScores[tns] == 0)
			pTapNoteScores->GetChildValue("Good", iTapNoteScores[tns]);
		else if (tns == TNS_W5 && iTapNoteScores[tns] == 0)
			pTapNoteScores->GetChildValue("Boo", iTapNoteScores[tns]);
	}

	if (pHoldNoteScores)
		FOREACH_ENUM(HoldNoteScore, hns) {
		pHoldNoteScores->GetChildValue(HoldNoteScoreToString(hns), iHoldNoteScores[hns]);
		if (hns == HNS_Held && iHoldNoteScores[hns] == 0)
			pHoldNoteScores->GetChildValue("OK", iHoldNoteScores[hns]);
		else if (hns == HNS_LetGo && iHoldNoteScores[hns] == 0)
			pHoldNoteScores->GetChildValue("NG", iHoldNoteScores[hns]);
	}

	int basecmod = 0;

	// 3.9 doesnt save rates lol....
	if (is39import) {
		if (basecmod > 0) {
			string cmod = sModifiers.substr(0, sModifiers.find_first_of(","));
			if (cmod.substr(0, 1) == "C") {
				int playedcmod = StringToInt(cmod.substr(1, cmod.size()));
				float estrate = static_cast<float>(basecmod) / static_cast<float>(playedcmod);
				int herp = lround(estrate * 10);
				estrate = static_cast<float>(herp) / 10.f;
				fMusicRate = estrate;
			}
		}

		// auto validate 3.95 imports for now
		bEtternaValid = true;

		// have to assume j4
		fJudgeScale = 1.f;
	}

	if (ScoreKey == "")
		ScoreKey = "S" + BinaryToHex(CryptManager::GetSHA1ForString(dateTime.GetString()));

	grade = clamp( grade, Grade_Tier01, Grade_Failed );
}

bool HighScoreImpl::WriteReplayData() {
	string append;
	string profiledir;

	string path = REPLAY_DIR + ScoreKey;
	ofstream fileStream(path, ios::binary);
	//check file

	ASSERT(vNoteRowVector.size() > 0);

	if (!fileStream) {
		LOG->Warn("Failed to create replay file at %s", path.c_str());
		return false;
	}
	
	unsigned int idx = vNoteRowVector.size() - 1;
	//loop for writing both vectors side by side
	for (unsigned int i = 0; i < idx; i++) {
		append = to_string(vNoteRowVector[i]) + " " + to_string(vOffsetVector[i]) + "\n";
		fileStream.write(append.c_str(), append.size());
	}
	append = to_string(vNoteRowVector[idx]) + " " + to_string(vOffsetVector[idx]);
	fileStream.write(append.c_str(), append.size());
	fileStream.close();
	LOG->Trace("Created replay file at %s", path.c_str());
	return true;
}

bool HighScore::WriteInputData(const vector<float>& oop) {
	string append;
	string profiledir;

	string path = REPLAY_DIR + m_Impl->ScoreKey;
	ofstream fileStream(path, ios::binary);
	//check file

	ASSERT(oop.size() > 0);

	if (!fileStream) {
		LOG->Warn("Failed to create replay file at %s", path.c_str());
		return false;
	}

	unsigned int idx = oop.size() - 1;
	//loop for writing both vectors side by side
	for (unsigned int i = 0; i < idx; i++) {
		append = to_string(oop[i]) + "\n";
		fileStream.write(append.c_str(), append.size());
	}
	append = to_string(oop[idx]);
	fileStream.write(append.c_str(), append.size());
	fileStream.close();
	LOG->Trace("Created replay file at %s", path.c_str());
	return true;
}

// should just get rid of impl -mina
bool HighScore::LoadReplayData() {
	// already exists
	if (m_Impl->vNoteRowVector.size() > 4 && m_Impl->vOffsetVector.size() > 4)
		return true;

	// disable until presumed race condition crash is resolved -mina
	return false;

	string profiledir;
	vector<int> vNoteRowVector;
	vector<float> vOffsetVector;
	string path = REPLAY_DIR + m_Impl->ScoreKey;

	std::ifstream fileStream(path, ios::binary);
	string line;
	string buffer;
	vector<string> tokens;
	stringstream ss;
	int noteRow;
	float offset;

	//check file
	if (!fileStream) {
		LOG->Warn("Failed to load replay data at %s", path.c_str());
		return false;
	}

	//loop until eof
	while (getline(fileStream, line))
	{
		ss.str(line);
		//split line into tokens
		while (ss >> buffer)
			tokens.emplace_back(buffer);

		noteRow = std::stoi(tokens[0]);
		if (!(typeid(noteRow) == typeid(int))) {
			throw std::runtime_error("NoteRow value is not of type: int");
		}
		vNoteRowVector.emplace_back(noteRow);

		offset = std::stof(tokens[1]);
		if (!(typeid(offset) == typeid(float))) {
			throw std::runtime_error("Offset value is not of type: float");
		}
		vOffsetVector.emplace_back(offset);
		tokens.clear();
	}
	fileStream.close();
	SetNoteRowVector(vNoteRowVector);
	SetOffsetVector(vOffsetVector);
	LOG->Trace("Loaded replay data at %s", path.c_str());
	return true;
}

bool HighScore::HasReplayData() {
	string profiledir = PROFILEMAN->GetProfileDir(ProfileSlot_Player1).substr(1);
	string path = profiledir + "ReplayData/" + m_Impl->ScoreKey;
	return DoesFileExist(path);
}

REGISTER_CLASS_TRAITS( HighScoreImpl, new HighScoreImpl(*pCopy) )

HighScore::HighScore()
{
	m_Impl = new HighScoreImpl;
}

void HighScore::Unset()
{
	m_Impl = new HighScoreImpl;
}

bool HighScore::IsEmpty() const
{
	if(	m_Impl->iTapNoteScores[TNS_W1] ||
		m_Impl->iTapNoteScores[TNS_W2] ||
		m_Impl->iTapNoteScores[TNS_W3] ||
		m_Impl->iTapNoteScores[TNS_W4] ||
		m_Impl->iTapNoteScores[TNS_W5] )
		return false;
	if( m_Impl->iHoldNoteScores[HNS_Held] > 0 )
		return false;
	return true;
}

bool HighScore::Is39import() const { return m_Impl->is39import; }

string	HighScore::GetName() const { return m_Impl->sName; }
string HighScore::GetChartKey() const { return m_Impl->ChartKey; }
int HighScore::GetSSRCalcVersion() const { return m_Impl->SSRCalcVersion; }
Grade HighScore::GetGrade() const { return m_Impl->grade; }
unsigned int HighScore::GetScore() const { return m_Impl->iScore; }
unsigned int HighScore::GetMaxCombo() const { return m_Impl->iMaxCombo; }
StageAward HighScore::GetStageAward() const { return m_Impl->stageAward; }
PeakComboAward HighScore::GetPeakComboAward() const { return m_Impl->peakComboAward; }
float HighScore::GetPercentDP() const { return m_Impl->fPercentDP; }
float HighScore::GetWifeScore() const { return m_Impl->fWifeScore; }
float HighScore::GetWifePoints() const { return m_Impl->fWifePoints; }
float HighScore::GetSSRNormPercent() const { return m_Impl->fSSRNormPercent; }
float HighScore::GetMusicRate() const { return m_Impl->fMusicRate; }
float HighScore::GetJudgeScale() const { return m_Impl->fJudgeScale; }
bool HighScore::GetChordCohesion() const {
	return !m_Impl->bNoChordCohesion;  }
bool HighScore::GetEtternaValid() const { return m_Impl->bEtternaValid; }
vector<float> HighScore::GetOffsetVector() const { return m_Impl->vOffsetVector; }
vector<int> HighScore::GetNoteRowVector() const { return m_Impl->vNoteRowVector; }
string HighScore::GetScoreKey() const { return m_Impl->ScoreKey; }
float HighScore::GetSurviveSeconds() const { return m_Impl->fSurviveSeconds; }
float HighScore::GetSurvivalSeconds() const { return GetSurviveSeconds() + GetLifeRemainingSeconds(); }
string HighScore::GetModifiers() const { return m_Impl->sModifiers; }
DateTime HighScore::GetDateTime() const { return m_Impl->dateTime; }
string HighScore::GetPlayerGuid() const { return m_Impl->sPlayerGuid; }
string HighScore::GetMachineGuid() const { return m_Impl->sMachineGuid; }
int HighScore::GetProductID() const { return m_Impl->iProductID; }
int HighScore::GetTapNoteScore( TapNoteScore tns ) const { return m_Impl->iTapNoteScores[tns]; }
int HighScore::GetHoldNoteScore( HoldNoteScore hns ) const { return m_Impl->iHoldNoteScores[hns]; }
float HighScore::GetSkillsetSSR(Skillset ss) const { return m_Impl->fSkillsetSSRs[ss]; }
const RadarValues &HighScore::GetRadarValues() const { return m_Impl->radarValues; }
float HighScore::GetLifeRemainingSeconds() const { return m_Impl->fLifeRemainingSeconds; }
bool HighScore::GetDisqualified() const { return m_Impl->bDisqualified; }

void HighScore::SetName( const string &sName ) { m_Impl->sName = sName; }
void HighScore::SetChartKey( const string &ck) { m_Impl->ChartKey = ck; }
void HighScore::SetSSRCalcVersion(int cv) { m_Impl->SSRCalcVersion = cv; }
void HighScore::SetGrade( Grade g ) { m_Impl->grade = g; }
void HighScore::SetScore( unsigned int iScore ) { m_Impl->iScore = iScore; }
void HighScore::SetMaxCombo( unsigned int i ) { m_Impl->iMaxCombo = i; }
void HighScore::SetStageAward( StageAward a ) { m_Impl->stageAward = a; }
void HighScore::SetPeakComboAward( PeakComboAward a ) { m_Impl->peakComboAward = a; }
void HighScore::SetPercentDP( float f ) { m_Impl->fPercentDP = f; }
void HighScore::SetWifeScore(float f) {m_Impl->fWifeScore = f;}
void HighScore::SetWifePoints(float f) { m_Impl->fWifePoints= f; }
void HighScore::SetSSRNormPercent(float f) { m_Impl->fSSRNormPercent = f; }
void HighScore::SetMusicRate(float f) { m_Impl->fMusicRate = f; }
void HighScore::SetJudgeScale(float f) { m_Impl->fJudgeScale = f; }
void HighScore::SetChordCohesion(bool b) { m_Impl->bNoChordCohesion = b; }
void HighScore::SetEtternaValid(bool b) { m_Impl->bEtternaValid = b; }
void HighScore::SetOffsetVector(const vector<float>& v) { m_Impl->vOffsetVector = v; }
void HighScore::SetNoteRowVector(const vector<int>& v) { m_Impl->vNoteRowVector = v; }
void HighScore::SetScoreKey(const string& sk) { m_Impl->ScoreKey = sk; }
void HighScore::SetRescoreJudgeVector(const vector<int>& v) { m_Impl->vRescoreJudgeVector = v; }
void HighScore::SetAliveSeconds( float f ) { m_Impl->fSurviveSeconds = f; }
void HighScore::SetModifiers( const string &s ) { m_Impl->sModifiers = s; }
void HighScore::SetDateTime( DateTime d ) { m_Impl->dateTime = d; }
void HighScore::SetPlayerGuid( const string &s ) { m_Impl->sPlayerGuid = s; }
void HighScore::SetMachineGuid( const string &s ) { m_Impl->sMachineGuid = s; }
void HighScore::SetProductID( int i ) { m_Impl->iProductID = i; }
void HighScore::SetTapNoteScore( TapNoteScore tns, int i ) { m_Impl->iTapNoteScores[tns] = i; }
void HighScore::SetHoldNoteScore( HoldNoteScore hns, int i ) { m_Impl->iHoldNoteScores[hns] = i; }
void HighScore::SetSkillsetSSR(Skillset ss, float ssr) { m_Impl->fSkillsetSSRs[ss] = ssr; }
void HighScore::SetValidationKey(ValidationKey vk, string k) { m_Impl->ValidationKeys[vk] = k; }
void HighScore::SetRadarValues( const RadarValues &rv ) { m_Impl->radarValues = rv; }
void HighScore::SetLifeRemainingSeconds( float f ) { m_Impl->fLifeRemainingSeconds = f; }
void HighScore::SetDisqualified( bool b ) { m_Impl->bDisqualified = b; }

void HighScore::UnloadReplayData() {
	m_Impl->UnloadReplayData();
}

void HighScore::ResetSkillsets() {
	m_Impl->ResetSkillsets();
}

/* We normally don't give direct access to the members.  We need this one
 * for NameToFillIn; use a special accessor so it's easy to find where this
 * is used. */
string *HighScore::GetNameMutable() { return &m_Impl->sName; }

bool HighScore::operator<(HighScore const& other) const
{
	return GetWifeScore() < other.GetWifeScore();
}

bool HighScore::operator>(HighScore const& other) const
{
	return other.operator<(*this);
}

bool HighScore::operator<=( const HighScore& other ) const
{
	return !operator>(other);
}

bool HighScore::operator>=( const HighScore& other ) const
{
	return !operator<(other);
}

bool HighScore::operator==( const HighScore& other ) const 
{
	return *m_Impl == *other.m_Impl;
}

bool HighScore::operator!=( const HighScore& other ) const
{
	return !operator==(other);
}

XNode* HighScore::CreateNode() const
{
	return m_Impl->CreateNode();
}

XNode* HighScore::CreateEttNode() const
{
	return m_Impl->CreateEttNode();
}

// Used for importing from stats.xml -mina
void HighScore::LoadFromNode( const XNode* pNode ) 
{
	m_Impl->LoadFromNode( pNode );

	/* Convert any old dp scores to wife. It is possible to diffrentiate converted scores from non-converted via 
	highscore members that did not exist pre-etterna, such as timingscale or judgevalue, however is it better to 
	make an explicit flag or would that just be needlessly redundant? - mina */
	if (m_Impl->fWifeScore == 0.f) {
		m_Impl->fWifeScore = ConvertDpToWife();
		
		// auto flag any converted files as invalid and let the player choose whether or not to validate them
		m_Impl->bEtternaValid = false;
	}

	// If imported scores have no normpercent check for replays to calculate it or fallback to wifescore (assume j4) -mina
	if (m_Impl->fSSRNormPercent == 0.f) {
		if (m_Impl->grade != Grade_Failed)
			m_Impl->fSSRNormPercent = RescoreToWifeJudgeDuringLoad(4);
		else
			m_Impl->fSSRNormPercent = m_Impl->fWifeScore;

		m_Impl->vNoteRowVector.clear();
		m_Impl->vOffsetVector.clear();
	}
}

// Used to load from etterna.xml -mina
void HighScore::LoadFromEttNode(const XNode* pNode)
{
	m_Impl->LoadFromEttNode(pNode);

	if (m_Impl->fSSRNormPercent > 1000.f) {
		if (m_Impl->grade != Grade_Failed)
			m_Impl->fSSRNormPercent = RescoreToWifeJudgeDuringLoad(4);
		else
			m_Impl->fSSRNormPercent = m_Impl->fWifeScore;

		m_Impl->vNoteRowVector.clear();
		m_Impl->vOffsetVector.clear();
	}
}

string HighScore::GetDisplayName() const
{
	if( GetName().empty() )
		return EMPTY_NAME;
	else
		return GetName();
}

/* begin HighScoreList */
void HighScoreList::Init()
{
	iNumTimesPlayed = 0;
	vHighScores.clear();
	HighGrade = Grade_NoData;
}

void HighScoreList::AddHighScore( HighScore hs, int &iIndexOut, bool bIsMachine )
{
	int i;
	for( i=0; i<static_cast<int>(vHighScores.size()); i++ )
	{
		if( hs >= vHighScores[i] )
			break;
	}
	// Unlimited score saving - Mina
	vHighScores.insert( vHighScores.begin()+i, hs );
	iIndexOut = i;
	// Delete extra machine high scores in RemoveAllButOneOfEachNameAndClampSize
	// and not here so that we don't end up with fewer than iMaxScores after 
	// removing HighScores with duplicate names.
	//
	HighGrade = min( hs.GetWifeGrade(), HighGrade);
}

void HighScoreList::IncrementPlayCount( DateTime _dtLastPlayed )
{
	dtLastPlayed = _dtLastPlayed;
	iNumTimesPlayed++;
}

const HighScore& HighScoreList::GetTopScore() const
{
	if( vHighScores.empty() )
	{
		static HighScore hs;
		hs = HighScore();
		return hs;
	}
	else
	{
		return vHighScores[0];
	}
}

XNode* HighScoreList::CreateNode() const
{
	XNode* pNode = new XNode( "HighScoreList" );

	pNode->AppendChild( "NumTimesPlayed", iNumTimesPlayed );
	pNode->AppendChild( "LastPlayed", dtLastPlayed.GetString() );
	if( HighGrade != Grade_NoData )
		pNode->AppendChild( "HighGrade", GradeToString(HighGrade) );

	for( unsigned i=0; i<vHighScores.size(); i++ )
	{
		const HighScore &hs = vHighScores[i];
		pNode->AppendChild( hs.CreateNode() );
	}

	return pNode;
}

void HighScoreList::LoadFromNode( const XNode* pHighScoreList )
{
	Init();

	ASSERT( pHighScoreList->GetName() == "HighScoreList" );
	FOREACH_CONST_Child( pHighScoreList, p )
	{
		const RString &name = p->GetName();
		if( name == "NumTimesPlayed" )
		{
			p->GetTextValue( iNumTimesPlayed );
		}
		else if( name == "LastPlayed" )
		{
			RString s;
			p->GetTextValue( s );
			dtLastPlayed.FromString( s );
		}
		else if( name == "HighGrade" )
		{
			RString s;
			p->GetTextValue( s );
			HighGrade = StringToGrade( s );
		}
		else if( name == "HighScore" )
		{
			vHighScores.resize( vHighScores.size()+1 );
			vHighScores.back().LoadFromNode( p );

			// ignore all high scores that are 0
			if( vHighScores.back().GetScore() == 0 )
				vHighScores.pop_back();
			else
				HighGrade = min( vHighScores.back().GetWifeGrade(), HighGrade );
		}
	}
}

void HighScoreList::RemoveAllButOneOfEachName()
{
	FOREACH( HighScore, vHighScores, i )
	{
		for( vector<HighScore>::iterator j = i+1; j != vHighScores.end(); j++ )
		{
			if( i->GetName() == j->GetName() )
			{
				j--;
				vHighScores.erase( j+1 );
			}
		}
	}
}

void HighScoreList::MergeFromOtherHSL(HighScoreList& other, bool is_machine)
{
	iNumTimesPlayed+= other.iNumTimesPlayed;
	if(other.dtLastPlayed > dtLastPlayed) { dtLastPlayed= other.dtLastPlayed; }
	if(other.HighGrade > HighGrade) { HighGrade= other.HighGrade; }
	vHighScores.insert(vHighScores.end(), other.vHighScores.begin(),
		other.vHighScores.end());
	std::sort(vHighScores.begin(), vHighScores.end());
	// Remove non-unique scores because they probably come from an accidental
	// repeated merge. -Kyz
	vector<HighScore>::iterator unique_end=
		std::unique(vHighScores.begin(), vHighScores.end());
	vHighScores.erase(unique_end, vHighScores.end());
	// Reverse it because sort moved the lesser scores to the top.
	std::reverse(vHighScores.begin(), vHighScores.end());
}

XNode* Screenshot::CreateNode() const
{
	XNode* pNode = new XNode( "Screenshot" );

	// TRICKY:  Don't write "name to fill in" markers.
	pNode->AppendChild( "FileName",	sFileName );
	pNode->AppendChild( "MD5",	sMD5 );
	pNode->AppendChild( highScore.CreateNode() );

	return pNode;
}

void Screenshot::LoadFromNode( const XNode* pNode ) 
{
	ASSERT( pNode->GetName() == "Screenshot" );

	pNode->GetChildValue( "FileName",	sFileName );
	pNode->GetChildValue( "MD5",		sMD5 );
	const XNode* pHighScore = pNode->GetChild( "HighScore" );
	if( pHighScore )
		highScore.LoadFromNode( pHighScore );
}

float HighScore::RescoreToWifeJudge(int x) {
	if (!LoadReplayData())
		return m_Impl->fWifeScore;

	const float tso[] = { 1.50f,1.33f,1.16f,1.00f,0.84f,0.66f,0.50f,0.33f,0.20f };
	float ts = tso[x-1];
	float p = 0;
	FOREACH_CONST(float, m_Impl->vOffsetVector, f)
		p += wife2(*f, ts);

	p += (m_Impl->iHoldNoteScores[HNS_LetGo] + m_Impl->iHoldNoteScores[HNS_Missed]) * -6.f;
	p += m_Impl->iTapNoteScores[TNS_HitMine] * -8.f;

	return p / static_cast<float>(m_Impl->vOffsetVector.size() * 2);
}

float HighScore::RescoreToWifeJudgeDuringLoad(int x) {
	if (!LoadReplayData())
		return m_Impl->fWifeScore;

	const float tso[] = { 1.50f,1.33f,1.16f,1.00f,0.84f,0.66f,0.50f,0.33f,0.20f };
	float ts = tso[x - 1];
	float p = 0;
	FOREACH_CONST(float, m_Impl->vOffsetVector, f)
		p += wife2(*f, ts);

	p += (m_Impl->iHoldNoteScores[HNS_LetGo] + m_Impl->iHoldNoteScores[HNS_Missed]) * -6.f;
	p += m_Impl->iTapNoteScores[TNS_HitMine] * -8.f;

	UnloadReplayData();
	return p / static_cast<float>(m_Impl->vOffsetVector.size() * 2);
}

// do not use for now- mina
float HighScoreImpl::RescoreToWifeTS(float ts) {
	float p = 0;
	FOREACH_CONST(float, vOffsetVector, f)
		p += wife2(*f, ts);

	p += (iHoldNoteScores[HNS_LetGo] + iHoldNoteScores[HNS_Missed]) * -6.f;
	return p / static_cast<float>(vOffsetVector.size() * 2);
}

float HighScore::RescoreToDPJudge(int x) {
	const float tso[] = { 1.50f,1.33f,1.16f,1.00f,0.84f,0.66f,0.50f,0.33f,0.20f };
	float ts = tso[x - 1];
	vector<int> vRescoreJudgeVector;
	int marv = 0;
	int perf = 0;
	int great = 0;
	int good = 0;
	int boo = 0;
	int miss = 0;
	int m2 = 0;
	FOREACH_CONST(float, m_Impl->vOffsetVector, f) {
		m2 += 2;
		float x = abs(*f * 1000.f);
		if (x <= ts * 22.5f)
			++marv;
		else if (x <= ts * 45.f)
			++perf;
		else if (x <= ts * 90.f)
			++great;
		else if (x <= ts * 135.f)
			++good;
		else if (x <= ts *180.f)
			++boo;
		else
			++miss;
	}

	//LOG->Trace("Marv: %i Perf: %i, Great: %i, Good: %i, Boo: %i, Miss: %i", marv, perf, great, good, boo, miss);
	vRescoreJudgeVector.push_back(marv);
	vRescoreJudgeVector.push_back(perf);
	vRescoreJudgeVector.push_back(great);
	vRescoreJudgeVector.push_back(good);
	vRescoreJudgeVector.push_back(boo);
	vRescoreJudgeVector.push_back(miss);
	SetRescoreJudgeVector(vRescoreJudgeVector);

	int p = 0;
	p += (marv + perf) * 2;
	p += great * 1;
	p += boo * -4;
	p += miss * -8;
	p += m_Impl->iHoldNoteScores[HNS_Held] * 6;
	p += m_Impl->iTapNoteScores[TNS_HitMine] * -8;
	p += (m_Impl->iHoldNoteScores[HNS_LetGo] + m_Impl->iHoldNoteScores[HNS_Missed]) * -6;

	float m = static_cast<float>(m_Impl->vOffsetVector.size() * 2);
	m += (m_Impl->radarValues[RadarCategory_Holds] + m_Impl->radarValues[RadarCategory_Rolls]) * 6;
	return p / m;
}

vector<int> HighScore::GetRescoreJudgeVector(int x) {
	RescoreToDPJudge(x);
	return m_Impl->vRescoreJudgeVector;
}

Grade HighScore::GetWifeGrade() {
	return m_Impl->GetWifeGrade();
}

bool HighScore::WriteReplayData() {
	return m_Impl->WriteReplayData();
}

// Ok I guess we can be more lenient and convert by midwindow values, but we still have to assume j4 - mina
float HighScore::ConvertDpToWife() {
	if (m_Impl->fWifeScore > 0.f)
		return m_Impl->fWifeScore;

	if (m_Impl->grade == Grade_Failed)
		return 0.f;

	float ts = 1.f;
	float estpoints = 0.f;
	float maxpoints = 0.f;
	estpoints += m_Impl->iTapNoteScores[TNS_W1] * wife2(.01125f, ts);
	estpoints += m_Impl->iTapNoteScores[TNS_W2] * wife2(.03375f, ts);
	estpoints += m_Impl->iTapNoteScores[TNS_W3] * wife2(.0675f, ts);
	estpoints += m_Impl->iTapNoteScores[TNS_W4] * wife2(.1125f, ts);
	estpoints += m_Impl->iTapNoteScores[TNS_W5] * wife2(.1575f, ts);
	estpoints += m_Impl->iTapNoteScores[TNS_Miss] * -8;

	FOREACH_ENUM(TapNoteScore, tns)
		maxpoints += 2 * m_Impl->iTapNoteScores[tns];

	return estpoints / maxpoints;
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the HighScore. */
class LunaHighScore: public Luna<HighScore>
{
public:
	static int GetName( T* p, lua_State *L )			{ lua_pushstring(L, p->GetName().c_str()); return 1; }
	static int GetScore( T* p, lua_State *L )			{ lua_pushnumber(L, p->GetScore() ); return 1; }
	static int GetPercentDP( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetPercentDP() ); return 1; }
	static int GetWifeScore(T* p, lua_State *L)			{ lua_pushnumber(L, p->GetWifeScore()); return 1; }
	static int GetWifePoints(T* p, lua_State *L)		{ lua_pushnumber(L, p->GetWifePoints()); return 1; }
	static int GetMusicRate(T* p, lua_State *L)			{ lua_pushnumber(L, p->GetMusicRate()); return 1; }
	static int GetJudgeScale(T* p, lua_State *L)		{ lua_pushnumber(L, p->GetJudgeScale()); return 1; }
	static int GetDate( T* p, lua_State *L )			{ lua_pushstring(L, p->GetDateTime().GetString() ); return 1; }
	static int GetSurvivalSeconds( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetSurvivalSeconds() ); return 1; }
	static int IsFillInMarker( T* p, lua_State *L )
	{
		bool bIsFillInMarker = false;
		FOREACH_PlayerNumber( pn )
			bIsFillInMarker |= p->GetName() == RANKING_TO_FILL_IN_MARKER[pn];
		lua_pushboolean( L, bIsFillInMarker );
		return 1;
	}
	static int GetMaxCombo( T* p, lua_State *L )			{ lua_pushnumber(L, p->GetMaxCombo() ); return 1; }
	static int GetModifiers( T* p, lua_State *L )			{ lua_pushstring(L, p->GetModifiers().c_str() ); return 1; }
	static int GetTapNoteScore( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetTapNoteScore( Enum::Check<TapNoteScore>(L, 1) ) ); return 1; }
	static int GetHoldNoteScore( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetHoldNoteScore( Enum::Check<HoldNoteScore>(L, 1) ) ); return 1; }
	static int RescoreToWifeJudge(T* p, lua_State *L)		{ lua_pushnumber(L, p->RescoreToWifeJudge(IArg(1))); return 1; }
	static int RescoreToDPJudge(T* p, lua_State *L)			{ lua_pushnumber(L, p->RescoreToDPJudge(IArg(1))); return 1; }
	static int RescoreJudges( T* p, lua_State *L )
	{
		lua_newtable(L);
		for( int i = 0; i < 6; ++i )
		{
			lua_pushnumber(L, p->GetRescoreJudgeVector(IArg(1))[i]);
			lua_rawseti( L, -2			, i+1 );
		}

		return 1;
	}
	static int GetRadarValues( T* p, lua_State *L )
	{
		RadarValues &rv = const_cast<RadarValues &>(p->GetRadarValues());
		rv.PushSelf(L);
		return 1;
	}
	
	static int GetSkillsetSSR(T* p, lua_State *L) {
		lua_pushnumber(L, p->GetSkillsetSSR(Enum::Check<Skillset>(L, 1)));
		return 1;
	}
	static int ToggleEtternaValidation(T* p, lua_State *L) {
		p->SetEtternaValid(!p->GetEtternaValid());
		return 1;
	}

	// Convert to MS so lua doesn't have to
	static int GetOffsetVector(T* p, lua_State *L) {
		if (p->LoadReplayData()) {
			vector<float> doot = p->GetOffsetVector();
			for (size_t i = 0; i < doot.size(); ++i)
				doot[i] = doot[i] * 1000;
			LuaHelpers::CreateTableFromArray(doot, L);
			p->UnloadReplayData();
		}
		else
			lua_pushnil(L);
		return 1;
	}

	static int GetNoteRowVector(T* p, lua_State *L) {
		if (p->LoadReplayData()) {
			LuaHelpers::CreateTableFromArray(p->GetNoteRowVector(), L);
			p->UnloadReplayData();
		}
		else
			lua_pushnil(L);
		return 1;
	}

	DEFINE_METHOD( GetGrade, GetGrade() )
	DEFINE_METHOD( GetWifeGrade, GetWifeGrade() )
	DEFINE_METHOD( ConvertDpToWife, ConvertDpToWife() )
	DEFINE_METHOD( GetStageAward, GetStageAward() )
	DEFINE_METHOD( GetPeakComboAward, GetPeakComboAward() )
	DEFINE_METHOD( GetChordCohesion, GetChordCohesion() )
	DEFINE_METHOD( GetEtternaValid , GetEtternaValid() )
	DEFINE_METHOD( HasReplayData, HasReplayData() )
	DEFINE_METHOD( GetChartKey, GetChartKey())
	LunaHighScore()
	{
		ADD_METHOD( GetName );
		ADD_METHOD( GetScore );
		ADD_METHOD( GetPercentDP );
		ADD_METHOD( ConvertDpToWife );
		ADD_METHOD( GetWifeScore );
		ADD_METHOD( GetWifePoints );
		ADD_METHOD( RescoreToWifeJudge );
		ADD_METHOD( RescoreToDPJudge );
		ADD_METHOD( RescoreJudges );
		ADD_METHOD( GetSkillsetSSR );
		ADD_METHOD( GetMusicRate );
		ADD_METHOD( GetJudgeScale );
		ADD_METHOD( GetChordCohesion );
		ADD_METHOD( GetDate );
		ADD_METHOD( GetSurvivalSeconds );
		ADD_METHOD( IsFillInMarker );
		ADD_METHOD( GetModifiers );
		ADD_METHOD( GetTapNoteScore );
		ADD_METHOD( GetHoldNoteScore );
		ADD_METHOD( GetRadarValues );
		ADD_METHOD( GetGrade );
		ADD_METHOD( GetWifeGrade );
		ADD_METHOD( GetMaxCombo );
		ADD_METHOD( GetStageAward );
		ADD_METHOD( GetPeakComboAward );
		ADD_METHOD( ToggleEtternaValidation );
		ADD_METHOD( GetEtternaValid );
		ADD_METHOD( HasReplayData );
		ADD_METHOD( GetOffsetVector );
		ADD_METHOD( GetNoteRowVector );
		ADD_METHOD( GetChartKey );
	}
};

LUA_REGISTER_CLASS( HighScore )

/** @brief Allow Lua to have access to the HighScoreList. */
class LunaHighScoreList: public Luna<HighScoreList>
{
public:
	static int GetHighScores( T* p, lua_State *L )
	{
		lua_newtable(L);
		for( int i = 0; i < static_cast<int>(p->vHighScores.size()); ++i )
		{
			p->vHighScores[i].PushSelf(L);
			lua_rawseti( L, -2, i+1 );
		}

		return 1;
	}

	static int GetHighestScoreOfName( T* p, lua_State *L )
	{
		string name= SArg(1);
		for(size_t i= 0; i < p->vHighScores.size(); ++i)
		{
			if(name == p->vHighScores[i].GetName())
			{
				p->vHighScores[i].PushSelf(L);
				return 1;
			}
		}
		lua_pushnil(L);
		return 1;
	}

	static int GetRankOfName( T* p, lua_State *L )
	{
		string name= SArg(1);
		size_t rank= 0;
		for(size_t i= 0; i < p->vHighScores.size(); ++i)
		{
			if(name == p->vHighScores[i].GetName())
			{
				// Indices from Lua are one-indexed.  +1 to adjust.
				rank= i+1;
				break;
			}
		}
		// The themer is expected to check for validity before using.
		lua_pushnumber(L, rank);
		return 1;
	}

	LunaHighScoreList()
	{
		ADD_METHOD( GetHighScores );
		ADD_METHOD( GetHighestScoreOfName );
		ADD_METHOD( GetRankOfName );
	}
};

LUA_REGISTER_CLASS( HighScoreList )
// lua end

/*
 * (c) 2004 Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
