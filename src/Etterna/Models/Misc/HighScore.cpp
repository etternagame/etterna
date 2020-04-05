#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Foreach.h"
#include "GameConstantsAndTypes.h"
#include "HighScore.h"
#include "Etterna/Globals/picosha2.h"
#include "PlayerNumber.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RadarValues.h"
#include "RageUtil/Misc/RageLog.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "NoteTypes.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/Misc/TimingData.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "RageUtil/File/RageFileManager.h"

const string BASIC_REPLAY_DIR =
  "Save/Replays/"; // contains only tap offset data for rescoring/plots -mina
const string FULL_REPLAY_DIR =
  "Save/ReplaysV2/"; // contains freeze drops and mine hits as well as tap
					 // offsets; fully "rewatchable" -mina

struct HighScoreImpl
{
	string sName; // name that shows in the machine's ranking screen

	/* a half-misnomer now- since all scores are keyed by the chart key this
	should never change/be different, but its historical correctness is still
	correct, though it should prolly be renamed tbh -mina*/
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
	vector<string> uploaded;
	vector<float> vOffsetVector;
	vector<int> vNoteRowVector;
	vector<int> vTrackVector;
	vector<TapNoteType> vTapNoteTypeVector;
	vector<HoldReplayResult> vHoldReplayDataVector;
	vector<float> vOnlineReplayTimestampVector;
	vector<int> vRescoreJudgeVector;
	unsigned int iMaxCombo; // maximum combo obtained [SM5 alpha 1a+]
	string sModifiers;
	DateTime dateTime;   // return value of time() for when the highscore object
						 // was created (immediately after achieved)
	string sPlayerGuid;  // who made this high score
	string sMachineGuid; // where this high score was made
	string countryCode;
	int iProductID;
	int iTapNoteScores[NUM_TapNoteScore];
	int iHoldNoteScores[NUM_HoldNoteScore];
	float fSkillsetSSRs[NUM_Skillset];
	string ValidationKeys[NUM_ValidationKey];
	RadarValues radarValues;
	float fLifeRemainingSeconds;
	bool bDisqualified;
	string ValidationKey;
	int TopScore;

	HighScoreImpl();

	XNode* CreateEttNode() const;
	void LoadFromEttNode(const XNode* pNode);
	Grade GetWifeGrade() const;
	void UnloadReplayData();
	void ResetSkillsets();

	bool WriteReplayData();
	int ReplayType; // 0 = no loaded replay, 1 = basic, 2 = full; currently
					// unused but here for when we need it (not to be confused
					// with hasreplay()) -mina

	float RescoreToWifeTS(float ts);

	RString OffsetsToString(vector<float> v) const;
	vector<float> OffsetsToVector(RString s);
	RString NoteRowsToString(vector<int> v) const;
	vector<int> NoteRowsToVector(RString s);

	bool is39import = false;

	bool operator==(const HighScoreImpl& other) const;
	bool operator!=(const HighScoreImpl& other) const
	{
		return !(*this == other);
	}
};

bool
HighScoreImpl::operator==(const HighScoreImpl& other) const
{
#define COMPARE(x)                                                             \
	if ((x) != other.x)                                                        \
		return false;
	COMPARE(sName);
	COMPARE(grade);
	COMPARE(iScore);
	COMPARE(iMaxCombo);
	COMPARE(fPercentDP);
	COMPARE(fSurviveSeconds);
	COMPARE(sModifiers);
	COMPARE(dateTime);
	COMPARE(sPlayerGuid);
	COMPARE(sMachineGuid);
	COMPARE(iProductID);
	FOREACH_ENUM(TapNoteScore, tns)
	COMPARE(iTapNoteScores[tns]);
	FOREACH_ENUM(HoldNoteScore, hns)
	COMPARE(iHoldNoteScores[hns]);
	FOREACH_ENUM(Skillset, ss)
	COMPARE(fSkillsetSSRs[ss]);
	COMPARE(radarValues);
	COMPARE(fLifeRemainingSeconds);
	COMPARE(bDisqualified);
#undef COMPARE

	return true;
}

void
HighScoreImpl::UnloadReplayData()
{
	vNoteRowVector.clear();
	vOffsetVector.clear();
	vTrackVector.clear();
	vTapNoteTypeVector.clear();

	vNoteRowVector.shrink_to_fit();
	vOffsetVector.shrink_to_fit();
	vTrackVector.shrink_to_fit();
	vTapNoteTypeVector.shrink_to_fit();

	ReplayType = 0;
}

Grade
HighScoreImpl::GetWifeGrade() const
{
	if (grade == Grade_Failed)
		return Grade_Failed;

	auto prc = fWifeScore;

	if (PREFSMAN->m_bSortBySSRNorm)
		prc = fSSRNormPercent;

	if (prc >= 0.99999f)
		return Grade_Tier01;
	if (PREFSMAN->m_bUseMidGrades && prc >= 0.9999f)
		return Grade_Tier02;
	if (PREFSMAN->m_bUseMidGrades && prc >= 0.9998f)
		return Grade_Tier03;
	if (prc >= 0.9997f)
		return Grade_Tier04;
	if (PREFSMAN->m_bUseMidGrades && prc >= 0.9992f)
		return Grade_Tier05;
	if (PREFSMAN->m_bUseMidGrades && prc >= 0.9985f)
		return Grade_Tier06;
	if (prc >= 0.9975f)
		return Grade_Tier07;
	if (PREFSMAN->m_bUseMidGrades && prc >= 0.99f)
		return Grade_Tier08;
	if (PREFSMAN->m_bUseMidGrades && prc >= 0.965f)
		return Grade_Tier09;
	if (prc >= 0.93f)
		return Grade_Tier10;
	if (PREFSMAN->m_bUseMidGrades && prc >= 0.9f)
		return Grade_Tier11;
	if (PREFSMAN->m_bUseMidGrades && prc >= 0.85f)
		return Grade_Tier12;
	if (prc >= 0.8f)
		return Grade_Tier13;
	if (prc >= 0.7f)
		return Grade_Tier14;
	if (prc >= 0.6f)
		return Grade_Tier15;
	return Grade_Tier16;
}

RString
HighScoreImpl::OffsetsToString(vector<float> v) const
{
	RString o = "";
	if (v.empty())
		return o;

	o = to_string(v[0]);
	for (size_t i = 1; i < v.size(); i++)
		o.append("," + to_string(v[i]));
	return o;
}

RString
HighScoreImpl::NoteRowsToString(vector<int> v) const
{
	RString o = "";
	if (v.empty())
		return o;

	o = to_string(v[0]);
	for (size_t i = 1; i < v.size(); i++)
		o.append("," + to_string(v[i]));
	return o;
}

vector<float>
HighScoreImpl::OffsetsToVector(RString s)
{
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

vector<int>
HighScoreImpl::NoteRowsToVector(RString s)
{
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

void
HighScoreImpl::ResetSkillsets()
{
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
	sModifiers = "";
	dateTime.Init();
	sPlayerGuid = "";
	sMachineGuid = "";
	iProductID = 0;
	ZERO(iTapNoteScores);
	ZERO(iHoldNoteScores);
	ZERO(fSkillsetSSRs);
	radarValues.MakeUnknown();
	fLifeRemainingSeconds = 0;
	string ValidationKey = "";
	TopScore = 0;
	ReplayType = 2;
	bNoChordCohesion = false;
	bDisqualified = false;
}

XNode*
HighScoreImpl::CreateEttNode() const
{
	XNode* pNode = new XNode("Score");

	if (ScoreKey == "")
		pNode->AppendAttr("Key",
						  "S" + BinaryToHex(CryptManager::GetSHA1ForString(
								  dateTime.GetString())));
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
	pNode->AppendChild("MachineGuid", sMachineGuid);
	pNode->AppendChild("DateTime", dateTime.GetString());
	pNode->AppendChild("TopScore", TopScore);
	if (!uploaded.empty()) {
		XNode* pServerNode = pNode->AppendChild("Servs");
		for (auto server : uploaded)
			pServerNode->AppendChild("server", server);
	}

	XNode* pTapNoteScores = pNode->AppendChild("TapNoteScores");
	FOREACH_ENUM(TapNoteScore, tns)
	if (tns != TNS_None && tns != TNS_CheckpointMiss &&
		tns != TNS_CheckpointHit)
		pTapNoteScores->AppendChild(TapNoteScoreToString(tns),
									iTapNoteScores[tns]);

	XNode* pHoldNoteScores = pNode->AppendChild("HoldNoteScores");
	FOREACH_ENUM(HoldNoteScore, hns)
	if (hns != HNS_None) // HACK: don't save meaningless "none" count
		pHoldNoteScores->AppendChild(HoldNoteScoreToString(hns),
									 iHoldNoteScores[hns]);

	// dont bother writing skillset ssrs for non-applicable scores
	if (fWifeScore > 0.f && grade != Grade_Failed &&
		fSkillsetSSRs[Skill_Overall] > 0.f) {
		XNode* pSkillsetSSRs = pNode->AppendChild("SkillsetSSRs");
		FOREACH_ENUM(Skillset, ss)
		pSkillsetSSRs->AppendChild(
		  SkillsetToString(ss), FloatToString(fSkillsetSSRs[ss]).substr(0, 5));
	}

	XNode* pValidationKeys = pNode->AppendChild("ValidationKeys");
	pValidationKeys->AppendChild(ValidationKeyToString(ValidationKey_Brittle),
								 ValidationKeys[ValidationKey_Brittle]);
	pValidationKeys->AppendChild(ValidationKeyToString(ValidationKey_Weak),
								 ValidationKeys[ValidationKey_Weak]);

	return pNode;
}

void
HighScoreImpl::LoadFromEttNode(const XNode* pNode)
{
	// ASSERT(pNode->GetName() == "Score");

	RString s;
	pNode->GetChildValue("SSRCalcVersion", SSRCalcVersion);
	if (pNode->GetChildValue("Grade", s))
		grade = StringToGrade(s);
	pNode->GetChildValue("WifeScore", fWifeScore);
	pNode->GetChildValue("WifePoints", fWifePoints);
	pNode->GetChildValue("SSRNormPercent", fSSRNormPercent);
	pNode->GetChildValue("Rate", fMusicRate);
	pNode->GetChildValue("JudgeScale", fJudgeScale);
	pNode->GetChildValue("NoChordCohesion", bNoChordCohesion);
	pNode->GetChildValue("EtternaValid", bEtternaValid);
	const XNode* pUploadedServers = pNode->GetChild("Servs");
	if (pUploadedServers != nullptr) {
		FOREACH_CONST_Child(pUploadedServers, p)
		{
			RString server;
			p->GetTextValue(server);
			uploaded.emplace_back(server.c_str());
		}
	}
	pNode->GetChildValue("SurviveSeconds", fSurviveSeconds);
	pNode->GetChildValue("MaxCombo", iMaxCombo);
	if (pNode->GetChildValue("Modifiers", s))
		sModifiers = s;
	if (pNode->GetChildValue("DateTime", s))
		dateTime.FromString(s);
	if (pNode->GetChildValue("ScoreKey", s))
		ScoreKey = s;
	if (pNode->GetChildValue("MachineGuid", s))
		sMachineGuid = s;

	const XNode* pTapNoteScores = pNode->GetChild("TapNoteScores");
	if (pTapNoteScores)
		FOREACH_ENUM(TapNoteScore, tns)
	pTapNoteScores->GetChildValue(TapNoteScoreToString(tns),
								  iTapNoteScores[tns]);

	const XNode* pHoldNoteScores = pNode->GetChild("HoldNoteScores");
	if (pHoldNoteScores)
		FOREACH_ENUM(HoldNoteScore, hns)
	pHoldNoteScores->GetChildValue(HoldNoteScoreToString(hns),
								   iHoldNoteScores[hns]);

	if (fWifeScore > 0.f) {
		const XNode* pSkillsetSSRs = pNode->GetChild("SkillsetSSRs");
		if (pSkillsetSSRs)
			FOREACH_ENUM(Skillset, ss)
		pSkillsetSSRs->GetChildValue(SkillsetToString(ss), fSkillsetSSRs[ss]);
	}

	if (fWifeScore > 0.f) {
		const XNode* pValidationKeys = pNode->GetChild("ValidationKeys");
		if (pValidationKeys != nullptr) {
			if (pValidationKeys->GetChildValue(
				  ValidationKeyToString(ValidationKey_Brittle), s))
				ValidationKeys[ValidationKey_Brittle] = s;
			if (pValidationKeys->GetChildValue(
				  ValidationKeyToString(ValidationKey_Weak), s))
				ValidationKeys[ValidationKey_Weak] = s;
		}
	}

	if (ScoreKey == "")
		ScoreKey =
		  "S" +
		  BinaryToHex(CryptManager::GetSHA1ForString(dateTime.GetString()));

	// Validate input.
	grade = clamp(grade, Grade_Tier01, Grade_Failed);
}

bool
HighScoreImpl::WriteReplayData()
{
	CHECKPOINT_M("Writing out replay data to disk.");
	string append;
	string profiledir;
	// These two lines should probably be somewhere else
	if (!FILEMAN->IsADirectory(FULL_REPLAY_DIR))
		FILEMAN->CreateDir(FULL_REPLAY_DIR);
	string path = FULL_REPLAY_DIR + ScoreKey;
	ofstream fileStream(path, ios::binary);
	// check file

	ASSERT(vNoteRowVector.size() > 0);

	if (!fileStream) {
		LOG->Warn("Failed to create replay file at %s", path.c_str());
		return false;
	}

	unsigned int idx = vNoteRowVector.size() - 1;
	// loop for writing both vectors side by side
	for (unsigned int i = 0; i <= idx; i++) {
		append = to_string(vNoteRowVector[i]) + " " +
				 to_string(vOffsetVector[i]) + " " +
				 to_string(vTrackVector[i]) +
				 (vTapNoteTypeVector[i] != TapNoteType_Tap
					? " " + to_string(vTapNoteTypeVector[i])
					: "") +
				 "\n";
		fileStream.write(append.c_str(), append.size());
	}
	for (auto& hold : vHoldReplayDataVector) {
		append =
		  "H " + to_string(hold.row) + " " + to_string(hold.track) +
		  (hold.subType != TapNoteSubType_Hold ? " " + to_string(hold.subType)
											   : "") +
		  "\n";
		fileStream.write(append.c_str(), append.size());
	}
	fileStream.close();
	LOG->Trace("Created replay file at %s", path.c_str());
	return true;
}

bool
HighScore::WriteInputData(const vector<float>& oop)
{
	string append;
	string profiledir;

	string path = FULL_REPLAY_DIR + m_Impl->ScoreKey;
	ofstream fileStream(path, ios::binary);
	// check file

	ASSERT(oop.size() > 0);

	if (!fileStream) {
		LOG->Warn("Failed to create replay file at %s", path.c_str());
		return false;
	}

	unsigned int idx = oop.size() - 1;
	// loop for writing both vectors side by side
	for (unsigned int i = 0; i <= idx; i++) {
		append = to_string(oop[i]) + "\n";
		fileStream.write(append.c_str(), append.size());
	}
	fileStream.close();
	LOG->Trace("Created replay file at %s", path.c_str());
	return true;
}

// should just get rid of impl -mina
bool
HighScore::LoadReplayData()
{ // see dir definition comments at the top -mina
	if (LoadReplayDataFull())
		return true;
	return LoadReplayDataBasic();
}

bool
HighScore::LoadReplayDataBasic()
{
	// already exists
	if (m_Impl->vNoteRowVector.size() > 4 && m_Impl->vOffsetVector.size() > 4)
		return true;

	string profiledir;
	vector<int> vNoteRowVector;
	vector<float> vOffsetVector;
	string path = BASIC_REPLAY_DIR + m_Impl->ScoreKey;

	std::ifstream fileStream(path, ios::binary);
	string line;
	string buffer;
	vector<string> tokens;
	stringstream ss;
	int noteRow;
	float offset;

	// check file
	if (!fileStream) {
		LOG->Warn("Failed to load replay data at %s", path.c_str());
		return false;
	}

	// loop until eof
	try {

		while (getline(fileStream, line)) {
			stringstream ss(line);
			// split line into tokens
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
	} catch (std::runtime_error& e) {
		LOG->Warn(
		  "Failed to load replay data at %s due to runtime exception: %s",
		  path.c_str(),
		  e.what());
		fileStream.close();
		return false;
	}
	fileStream.close();
	SetNoteRowVector(vNoteRowVector);
	SetOffsetVector(vOffsetVector);

	m_Impl->ReplayType = 1;
	LOG->Trace("Loaded replay data type 1 at %s", path.c_str());
	return true;
}

bool
HighScore::LoadReplayDataFull()
{
	if (m_Impl->vNoteRowVector.size() > 4 && m_Impl->vOffsetVector.size() > 4 &&
		m_Impl->vTrackVector.size() > 4) {
		m_Impl->ReplayType = 2;
		return true;
	}

	string profiledir;
	vector<int> vNoteRowVector;
	vector<float> vOffsetVector;
	vector<int> vTrackVector;
	vector<TapNoteType> vTapNoteTypeVector;
	vector<HoldReplayResult> vHoldReplayDataVector;
	string path = FULL_REPLAY_DIR + m_Impl->ScoreKey;

	std::ifstream fileStream(path, ios::binary);
	string line;
	string buffer;
	vector<string> tokens;
	int noteRow;
	float offset;
	int track;
	TapNoteType tnt;
	int tmp;

	// check file
	if (!fileStream) {
		LOG->Trace(
		  "Failed to load replay data at %s, checking for older replay version",
		  path.c_str());
		return false;
	}

	// loop until eof
	while (getline(fileStream, line)) {
		stringstream ss(line);
		// split line into tokens
		while (ss >> buffer)
			tokens.emplace_back(buffer);

		if (tokens[0] == "H") {
			HoldReplayResult hrr;
			hrr.row = std::stoi(tokens[1]);
			hrr.track = std::stoi(tokens[2]);
			tmp = tokens.size() > 3 ? ::stoi(tokens[3]) : TapNoteSubType_Hold;
			if (tmp < 0 || tmp >= NUM_TapNoteSubType ||
				!(typeid(tmp) == typeid(int))) {
				LOG->Warn("Failed to load replay data at %s (\"Tapnotesubtype "
						  "value is not of type TapNoteSubType\")",
						  path.c_str());
			}
			hrr.subType = static_cast<TapNoteSubType>(tmp);
			vHoldReplayDataVector.emplace_back(hrr);
			tokens.clear();
			continue;
		}
		noteRow = std::stoi(tokens[0]);
		if (!(typeid(noteRow) == typeid(int))) {
			LOG->Warn("Failed to load replay data at %s (\"NoteRow value is "
					  "not of type: int\")",
					  path.c_str());
		}
		vNoteRowVector.emplace_back(noteRow);

		offset = std::stof(tokens[1]);
		if (!(typeid(offset) == typeid(float))) {
			LOG->Warn("Failed to load replay data at %s (\"Offset value is not "
					  "of type: float\")",
					  path.c_str());
		}
		vOffsetVector.emplace_back(offset);

		track = std::stoi(tokens[2]);
		if (!(typeid(track) == typeid(int))) {
			LOG->Warn("Failed to load replay data at %s (\"Track/Column value "
					  "is not of type: int\")",
					  path.c_str());
		}
		vTrackVector.emplace_back(track);

		tmp = tokens.size() >= 4 ? ::stoi(tokens[3]) : TapNoteType_Tap;
		if (tmp < 0 || tmp >= TapNoteType_Invalid ||
			!(typeid(tmp) == typeid(int))) {
			LOG->Warn("Failed to load replay data at %s (\"Tapnotetype value "
					  "is not of type TapNoteType\")",
					  path.c_str());
		}
		tnt = static_cast<TapNoteType>(tmp);
		vTapNoteTypeVector.emplace_back(tnt);

		tokens.clear();
	}
	fileStream.close();
	SetNoteRowVector(vNoteRowVector);
	SetOffsetVector(vOffsetVector);
	SetTrackVector(vTrackVector);
	SetTapNoteTypeVector(vTapNoteTypeVector);
	SetHoldReplayDataVector(vHoldReplayDataVector);

	m_Impl->ReplayType = 2;
	LOG->Trace("Loaded replay data type 2 at %s", path.c_str());
	return true;
}

bool
HighScore::HasReplayData()
{
	RString fullpath = FULL_REPLAY_DIR + m_Impl->ScoreKey;
	RString basicpath = BASIC_REPLAY_DIR + m_Impl->ScoreKey;
	if (DoesFileExist(fullpath)) // check for full replays first then default to
								 // basic replays -mina
		return true;
	return DoesFileExist(basicpath);
}

REGISTER_CLASS_TRAITS(HighScoreImpl, new HighScoreImpl(*pCopy))

HighScore::HighScore()
{
	m_Impl = new HighScoreImpl;
}

void
HighScore::Unset()
{
	m_Impl = new HighScoreImpl;
}

bool
HighScore::IsEmpty() const
{
	if (m_Impl->iTapNoteScores[TNS_W1] || m_Impl->iTapNoteScores[TNS_W2] ||
		m_Impl->iTapNoteScores[TNS_W3] || m_Impl->iTapNoteScores[TNS_W4] ||
		m_Impl->iTapNoteScores[TNS_W5])
		return false;
	if (m_Impl->iHoldNoteScores[HNS_Held] > 0)
		return false;
	return true;
}

string
HighScore::GenerateValidationKeys()
{
	std::string key = "";

	FOREACH_ENUM(TapNoteScore, tns)
	{

		if (tns == TNS_AvoidMine || tns == TNS_CheckpointHit ||
			tns == TNS_CheckpointMiss || tns == TNS_None) {
			continue;
		}

		key.append(to_string(GetTapNoteScore(tns)));
	}

	FOREACH_ENUM(HoldNoteScore, hns)
	{
		if (hns == HNS_None) {
			continue;
		}

		key.append(to_string(GetHoldNoteScore(hns)));
	}

	norms = lround(GetSSRNormPercent() * 1000000.f);
	musics = lround(GetMusicRate() * 100.f);
	judges = lround(GetJudgeScale() * 100.f);

	key.append(GetScoreKey());
	key.append(GetChartKey());
	key.append(GetModifiers());
	key.append(GetMachineGuid());
	key.append(to_string(norms));
	key.append(to_string(musics));
	key.append(to_string(judges));
	key.append(to_string(static_cast<int>(!GetChordCohesion())));
	key.append(to_string(static_cast<int>(GetEtternaValid())));
	key.append(GradeToString(GetWifeGrade()));

	std::string hash_hex_str;

	picosha2::hash256_hex_string(key, hash_hex_str);

	SetValidationKey(ValidationKey_Brittle, hash_hex_str);

	// just testing stuff
	// hs.SetValidationKey(ValidationKey_Weak,
	// GenerateWeakValidationKey(m_iTapNoteScores, m_iHoldNoteScores));
	return key;
}

string
HighScore::GetName() const
{
	return m_Impl->sName;
}
string
HighScore::GetChartKey() const
{
	return m_Impl->ChartKey;
}
int
HighScore::GetSSRCalcVersion() const
{
	return m_Impl->SSRCalcVersion;
}
Grade
HighScore::GetGrade() const
{
	return m_Impl->grade;
}
unsigned int
HighScore::GetScore() const
{
	return m_Impl->iScore;
}
unsigned int
HighScore::GetMaxCombo() const
{
	return m_Impl->iMaxCombo;
}

float
HighScore::GetPercentDP() const
{
	return m_Impl->fPercentDP;
}
float
HighScore::GetWifeScore() const
{
	return m_Impl->fWifeScore;
}
float
HighScore::GetWifePoints() const
{
	return m_Impl->fWifePoints;
}
float
HighScore::GetSSRNormPercent() const
{
	return m_Impl->fSSRNormPercent;
}
float
HighScore::GetMusicRate() const
{
	return m_Impl->fMusicRate;
}
float
HighScore::GetJudgeScale() const
{
	return m_Impl->fJudgeScale;
}
bool
HighScore::GetChordCohesion() const
{
	return !m_Impl->bNoChordCohesion;
}
bool
HighScore::GetEtternaValid() const
{
	return m_Impl->bEtternaValid;
}
bool
HighScore::IsUploadedToServer(string s) const
{
	return find(m_Impl->uploaded.begin(), m_Impl->uploaded.end(), s) !=
		   m_Impl->uploaded.end();
}
vector<float>
HighScore::GetCopyOfOffsetVector() const
{
	return m_Impl->vOffsetVector;
}
vector<int>
HighScore::GetCopyOfNoteRowVector() const
{
	return m_Impl->vNoteRowVector;
}
vector<int>
HighScore::GetCopyOfTrackVector() const
{
	return m_Impl->vTrackVector;
}
vector<TapNoteType>
HighScore::GetCopyOfTapNoteTypeVector() const
{
	return m_Impl->vTapNoteTypeVector;
}
vector<HoldReplayResult>
HighScore::GetCopyOfHoldReplayDataVector() const
{
	return m_Impl->vHoldReplayDataVector;
}
vector<float>
HighScore::GetCopyOfSetOnlineReplayTimestampVector() const
{
	return m_Impl->vOnlineReplayTimestampVector;
}
const vector<float>&
HighScore::GetOffsetVector() const
{
	return m_Impl->vOffsetVector;
}
const vector<int>&
HighScore::GetNoteRowVector() const
{
	return m_Impl->vNoteRowVector;
}
const vector<int>&
HighScore::GetTrackVector() const
{
	return m_Impl->vTrackVector;
}
const vector<TapNoteType>&
HighScore::GetTapNoteTypeVector() const
{
	return m_Impl->vTapNoteTypeVector;
}
const vector<HoldReplayResult>&
HighScore::GetHoldReplayDataVector() const
{
	return m_Impl->vHoldReplayDataVector;
}
string
HighScore::GetScoreKey() const
{
	return m_Impl->ScoreKey;
}
float
HighScore::GetSurviveSeconds() const
{
	return m_Impl->fSurviveSeconds;
}
float
HighScore::GetSurvivalSeconds() const
{
	return GetSurviveSeconds() + GetLifeRemainingSeconds();
}
string
HighScore::GetModifiers() const
{
	return m_Impl->sModifiers;
}
DateTime
HighScore::GetDateTime() const
{
	return m_Impl->dateTime;
}
string
HighScore::GetPlayerGuid() const
{
	return m_Impl->sPlayerGuid;
}
string
HighScore::GetMachineGuid() const
{
	return m_Impl->sMachineGuid;
}
string
HighScore::GetCountryCode() const
{
	return m_Impl->countryCode;
}
int
HighScore::GetProductID() const
{
	return m_Impl->iProductID;
}
int
HighScore::GetTapNoteScore(TapNoteScore tns) const
{
	return m_Impl->iTapNoteScores[tns];
}
int
HighScore::GetHoldNoteScore(HoldNoteScore hns) const
{
	return m_Impl->iHoldNoteScores[hns];
}
float
HighScore::GetSkillsetSSR(Skillset ss) const
{
	return m_Impl->fSkillsetSSRs[ss];
}
const RadarValues&
HighScore::GetRadarValues() const
{
	return m_Impl->radarValues;
}
float
HighScore::GetLifeRemainingSeconds() const
{
	return m_Impl->fLifeRemainingSeconds;
}
bool
HighScore::GetDisqualified() const
{
	return m_Impl->bDisqualified;
}
int
HighScore::GetTopScore() const
{
	return m_Impl->TopScore;
}
int
HighScore::GetReplayType() const
{
	return m_Impl->ReplayType;
}

void
HighScore::SetName(const string& sName)
{
	m_Impl->sName = sName;
}
void
HighScore::SetChartKey(const string& ck)
{
	m_Impl->ChartKey = ck;
}
void
HighScore::SetSSRCalcVersion(int cv)
{
	m_Impl->SSRCalcVersion = cv;
}
void
HighScore::SetGrade(Grade g)
{
	m_Impl->grade = g;
}
void
HighScore::SetScore(unsigned int iScore)
{
	m_Impl->iScore = iScore;
}
void
HighScore::SetMaxCombo(unsigned int i)
{
	m_Impl->iMaxCombo = i;
}

void
HighScore::SetPercentDP(float f)
{
	m_Impl->fPercentDP = f;
}
void
HighScore::SetWifeScore(float f)
{
	m_Impl->fWifeScore = f;
}
void
HighScore::SetWifePoints(float f)
{
	m_Impl->fWifePoints = f;
}
void
HighScore::SetSSRNormPercent(float f)
{
	m_Impl->fSSRNormPercent = f;
}
void
HighScore::SetMusicRate(float f)
{
	m_Impl->fMusicRate = f;
}
void
HighScore::SetSurviveSeconds(float f)
{
	m_Impl->fSurviveSeconds = f;
}
void
HighScore::SetJudgeScale(float f)
{
	m_Impl->fJudgeScale = f;
}
void
HighScore::SetChordCohesion(bool b)
{
	m_Impl->bNoChordCohesion = b;
}
void
HighScore::SetEtternaValid(bool b)
{
	m_Impl->bEtternaValid = b;
}
void
HighScore::AddUploadedServer(string s)
{
	if (find(m_Impl->uploaded.begin(), m_Impl->uploaded.end(), s) ==
		m_Impl->uploaded.end())
		m_Impl->uploaded.emplace_back(s);
}
void
HighScore::SetOffsetVector(const vector<float>& v)
{
	m_Impl->vOffsetVector = v;
}
void
HighScore::SetNoteRowVector(const vector<int>& v)
{
	m_Impl->vNoteRowVector = v;
}
void
HighScore::SetTrackVector(const vector<int>& v)
{
	m_Impl->vTrackVector = v;
}
void
HighScore::SetTapNoteTypeVector(const vector<TapNoteType>& v)
{
	m_Impl->vTapNoteTypeVector = v;
}
void
HighScore::SetHoldReplayDataVector(const vector<HoldReplayResult>& v)
{
	m_Impl->vHoldReplayDataVector = v;
}
void
HighScore::SetOnlineReplayTimestampVector(const vector<float>& v)
{
	m_Impl->vOnlineReplayTimestampVector = v;
}
void
HighScore::SetScoreKey(const string& sk)
{
	m_Impl->ScoreKey = sk;
}
void
HighScore::SetRescoreJudgeVector(const vector<int>& v)
{
	m_Impl->vRescoreJudgeVector = v;
}
void
HighScore::SetAliveSeconds(float f)
{
	m_Impl->fSurviveSeconds = f;
}
void
HighScore::SetModifiers(const string& s)
{
	m_Impl->sModifiers = s;
}
void
HighScore::SetDateTime(DateTime d)
{
	m_Impl->dateTime = d;
}
void
HighScore::SetPlayerGuid(const string& s)
{
	m_Impl->sPlayerGuid = s;
}
void
HighScore::SetMachineGuid(const string& s)
{
	m_Impl->sMachineGuid = s;
}
void
HighScore::SetProductID(int i)
{
	m_Impl->iProductID = i;
}
void
HighScore::SetTapNoteScore(TapNoteScore tns, int i)
{
	m_Impl->iTapNoteScores[tns] = i;
}
void
HighScore::SetHoldNoteScore(HoldNoteScore hns, int i)
{
	m_Impl->iHoldNoteScores[hns] = i;
}
void
HighScore::SetSkillsetSSR(Skillset ss, float ssr)
{
	m_Impl->fSkillsetSSRs[ss] = ssr;
}
void
HighScore::SetValidationKey(ValidationKey vk, string k)
{
	m_Impl->ValidationKeys[vk] = k;
}
void
HighScore::SetTopScore(int i)
{
	m_Impl->TopScore = i;
}
string
HighScore::GetValidationKey(ValidationKey vk) const
{
	return m_Impl->ValidationKeys[vk];
}
void
HighScore::SetRadarValues(const RadarValues& rv)
{
	m_Impl->radarValues = rv;
}
void
HighScore::SetLifeRemainingSeconds(float f)
{
	m_Impl->fLifeRemainingSeconds = f;
}
void
HighScore::SetDisqualified(bool b)
{
	m_Impl->bDisqualified = b;
}
void
HighScore::SetReplayType(int i)
{
	m_Impl->ReplayType = i;
}

void
HighScore::UnloadReplayData()
{
	m_Impl->UnloadReplayData();
}

void
HighScore::ResetSkillsets()
{
	m_Impl->ResetSkillsets();
}

/* We normally don't give direct access to the members.  We need this one
 * for NameToFillIn; use a special accessor so it's easy to find where this
 * is used. */
string*
HighScore::GetNameMutable()
{
	return &m_Impl->sName;
}

bool
HighScore::operator<(HighScore const& other) const
{
	return GetWifeScore() < other.GetWifeScore();
}

bool
HighScore::operator>(HighScore const& other) const
{
	return other.operator<(*this);
}

bool
HighScore::operator<=(const HighScore& other) const
{
	return !operator>(other);
}

bool
HighScore::operator>=(const HighScore& other) const
{
	return !operator<(other);
}

bool
HighScore::operator==(const HighScore& other) const
{
	return *m_Impl == *other.m_Impl;
}

bool
HighScore::operator!=(const HighScore& other) const
{
	return !operator==(other);
}

XNode*
HighScore::CreateEttNode() const
{
	return m_Impl->CreateEttNode();
}

// Used to load from etterna.xml -mina
void
HighScore::LoadFromEttNode(const XNode* pNode)
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

string
HighScore::GetDisplayName() const
{
	return GetName();
}

/* begin HighScoreList */
void
HighScoreList::Init()
{
	iNumTimesPlayed = 0;
	vHighScores.clear();
	HighGrade = Grade_NoData;
}

void
HighScoreList::AddHighScore(const HighScore& hs,
							int& iIndexOut,
							bool bIsMachine)
{
	int i;
	for (i = 0; i < static_cast<int>(vHighScores.size()); i++) {
		if (hs >= vHighScores[i])
			break;
	}
	// Unlimited score saving - Mina
	vHighScores.insert(vHighScores.begin() + i, hs);
	iIndexOut = i;
	// Delete extra machine high scores in RemoveAllButOneOfEachNameAndClampSize
	// and not here so that we don't end up with fewer than iMaxScores after
	// removing HighScores with duplicate names.
	//
	HighGrade = min(hs.GetWifeGrade(), HighGrade);
}

void
HighScoreList::IncrementPlayCount(DateTime _dtLastPlayed)
{
	dtLastPlayed = _dtLastPlayed;
	iNumTimesPlayed++;
}

const HighScore&
HighScoreList::GetTopScore() const
{
	if (vHighScores.empty()) {
		static HighScore hs;
		hs = HighScore();
		return hs;
	} else {
		return vHighScores[0];
	}
}

void
HighScoreList::RemoveAllButOneOfEachName()
{
	FOREACH(HighScore, vHighScores, i)
	{
		for (vector<HighScore>::iterator j = i + 1; j != vHighScores.end();
			 j++) {
			if (i->GetName() == j->GetName()) {
				j--;
				vHighScores.erase(j + 1);
			}
		}
	}
}

void
HighScoreList::MergeFromOtherHSL(HighScoreList& other, bool is_machine)
{
	iNumTimesPlayed += other.iNumTimesPlayed;
	if (other.dtLastPlayed > dtLastPlayed) {
		dtLastPlayed = other.dtLastPlayed;
	}
	if (other.HighGrade > HighGrade) {
		HighGrade = other.HighGrade;
	}
	vHighScores.insert(
	  vHighScores.end(), other.vHighScores.begin(), other.vHighScores.end());
	std::sort(vHighScores.begin(), vHighScores.end());
	// Remove non-unique scores because they probably come from an accidental
	// repeated merge. -Kyz
	vector<HighScore>::iterator unique_end =
	  std::unique(vHighScores.begin(), vHighScores.end());
	vHighScores.erase(unique_end, vHighScores.end());
	// Reverse it because sort moved the lesser scores to the top.
	std::reverse(vHighScores.begin(), vHighScores.end());
}

XNode*
Screenshot::CreateNode() const
{
	XNode* pNode = new XNode("Screenshot");

	// TRICKY:  Don't write "name to fill in" markers.
	pNode->AppendChild("FileName", sFileName);
	pNode->AppendChild("MD5", sMD5);
	return pNode;
}

void
Screenshot::LoadFromNode(const XNode* pNode)
{
	ASSERT(pNode->GetName() == "Screenshot");

	pNode->GetChildValue("FileName", sFileName);
	pNode->GetChildValue("MD5", sMD5);
	const XNode* pHighScore = pNode->GetChild("HighScore");
}

float
HighScore::RescoreToWifeJudge(int x)
{
	if (!LoadReplayData())
		return m_Impl->fWifeScore;

	const float tso[] = { 1.50f, 1.33f, 1.16f, 1.00f, 0.84f,
						  0.66f, 0.50f, 0.33f, 0.20f };
	float ts = tso[x - 1];
	float p = 0;
	for (auto& n : m_Impl->vOffsetVector)
		p += wife2(n, ts);

	p += (m_Impl->iHoldNoteScores[HNS_LetGo] +
		  m_Impl->iHoldNoteScores[HNS_Missed]) *
		 -6.f;
	p += m_Impl->iTapNoteScores[TNS_HitMine] * -8.f;

	float pmax = static_cast<float>(m_Impl->vOffsetVector.size() * 2);

	/* we don't want to have to access notedata when loading or rescording
	scores so we use the vector length of offset replay data to determine point
	denominators however full replays store mine hits as offsets, meaning
	we have to screen them out when calculating the max points*/
	if (m_Impl->ReplayType == 2) {
		pmax += m_Impl->iTapNoteScores[TNS_HitMine] * -2.f;

		// we screened out extra offsets due to mines in the replay from the
		// denominator but we've still increased the numerator with 0.00f
		// offsets (2pts)
		p += m_Impl->iTapNoteScores[TNS_HitMine] * -2.f;
	}

	return p / pmax;
}

float
HighScore::RescoreToWifeJoodge(int x)
{
	if (!LoadReplayData())
		return m_Impl->fWifeScore;

	const float tso[] = { 1.50f, 1.33f, 1.16f, 1.00f, 0.84f,
						  0.66f, 0.50f, 0.33f, 0.20f };
	float ts = tso[x - 1];
	float p = 0;
	for (auto& n : m_Impl->vOffsetVector)
		p += wife3(abs(n * 1000.f), ts);

	p += (m_Impl->iHoldNoteScores[HNS_LetGo] +
		  m_Impl->iHoldNoteScores[HNS_Missed]) *
		 -6.f;
	p += m_Impl->iTapNoteScores[TNS_HitMine] * -8.f;

	float pmax = static_cast<float>(m_Impl->vOffsetVector.size() * 2);

	/* we don't want to have to access notedata when loading or rescording
	scores so we use the vector length of offset replay data to determine point
	denominators however full replays store mine and hold drop offsets, meaning
	we have to screen them out when calculating the max points -mina*/
	if (m_Impl->ReplayType == 2) {
		pmax += m_Impl->iTapNoteScores[TNS_HitMine] * -2.f;

		// we screened out extra offsets due to mines in the replay from the
		// denominator but we've still increased the numerator with 0.00f
		// offsets (2pts)
		p += m_Impl->iTapNoteScores[TNS_HitMine] * -2.f;
	}

	return p / pmax;
}

float
HighScore::RescoreToWifeJoodge2(int x)
{
	if (!LoadReplayData())
		return m_Impl->fWifeScore;

	const float tso[] = { 1.50f, 1.33f, 1.16f, 1.00f, 0.84f,
						  0.66f, 0.50f, 0.33f, 0.20f };
	float ts = tso[x - 1];
	float p = 0;
	for (auto& n : m_Impl->vOffsetVector)
		p += wife4(abs(n * 1000.f), ts);

	p += (m_Impl->iHoldNoteScores[HNS_LetGo] +
		  m_Impl->iHoldNoteScores[HNS_Missed]) *
		 -6.f;
	p += m_Impl->iTapNoteScores[TNS_HitMine] * -8.f;

	float pmax = static_cast<float>(m_Impl->vOffsetVector.size() * 2);

	/* we don't want to have to access notedata when loading or rescording
	scores so we use the vector length of offset replay data to determine point
	denominators however full replays store mine and hold drop offsets, meaning
	we have to screen them out when calculating the max points -mina*/
	if (m_Impl->ReplayType == 2) {
		pmax += m_Impl->iTapNoteScores[TNS_HitMine] * -2.f;

		// we screened out extra offsets due to mines in the replay from the
		// denominator but we've still increased the numerator with 0.00f
		// offsets (2pts)
		p += m_Impl->iTapNoteScores[TNS_HitMine] * -2.f;
	}

	return p / pmax;
}


float
HighScore::RescoreToWifeJudgeDuringLoad(int x)
{
	if (!LoadReplayData())
		return m_Impl->fWifeScore;

	const float tso[] = { 1.50f, 1.33f, 1.16f, 1.00f, 0.84f,
						  0.66f, 0.50f, 0.33f, 0.20f };
	float ts = tso[x - 1];
	float p = 0;
	for (auto& n : m_Impl->vOffsetVector)
		p += wife2(n, ts);

	p += (m_Impl->iHoldNoteScores[HNS_LetGo] +
		  m_Impl->iHoldNoteScores[HNS_Missed]) *
		 -6.f;
	p += m_Impl->iTapNoteScores[TNS_HitMine] * -8.f;

	float pmax = static_cast<float>(m_Impl->vOffsetVector.size() * 2);

	/* we don't want to have to access notedata when loading or rescording
	scores so we use the vector length of offset replay data to determine point
	denominators however full replays store mine and hold drop offsets, meaning
	we have to screen them out when calculating the max points -mina*/
	if (m_Impl->ReplayType == 2) {
		pmax += m_Impl->iTapNoteScores[TNS_HitMine] * -2.f;

		// we screened out extra offsets due to mines in the replay from the
		// denominator but we've still increased the numerator with 0.00f
		// offsets (2pts)
		p += m_Impl->iTapNoteScores[TNS_HitMine] * -2.f;
	}

	float o = p / pmax;
	return o;
}

// do not use for now- mina
float
HighScoreImpl::RescoreToWifeTS(float ts)
{
	float p = 0;
	FOREACH_CONST(float, vOffsetVector, f)
	p += wife2(*f, ts);

	p += (iHoldNoteScores[HNS_LetGo] + iHoldNoteScores[HNS_Missed]) * -6.f;
	return p / static_cast<float>(vOffsetVector.size() * 2);
}

float
HighScore::RescoreToDPJudge(int x)
{
	const float tso[] = { 1.50f, 1.33f, 1.16f, 1.00f, 0.84f,
						  0.66f, 0.50f, 0.33f, 0.20f };
	float ts = tso[x - 1];
	vector<int> vRescoreJudgeVector;
	int marv = 0;
	int perf = 0;
	int great = 0;
	int good = 0;
	int boo = 0;
	int miss = 0;
	int m2 = 0;
	FOREACH_CONST(float, m_Impl->vOffsetVector, f)
	{
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
		else if (x <= ts * 180.f)
			++boo;
		else
			++miss;
	}

	// LOG->Trace("Marv: %i Perf: %i, Great: %i, Good: %i, Boo: %i, Miss: %i",
	// marv, perf, great, good, boo, miss);
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
	p += (m_Impl->iHoldNoteScores[HNS_LetGo] +
		  m_Impl->iHoldNoteScores[HNS_Missed]) *
		 -6;

	float m = static_cast<float>(m_Impl->vOffsetVector.size() * 2);
	m += (m_Impl->radarValues[RadarCategory_Holds] +
		  m_Impl->radarValues[RadarCategory_Rolls]) *
		 6;
	return p / m;
}

vector<int>
HighScore::GetRescoreJudgeVector(int x)
{
	RescoreToDPJudge(x);
	return m_Impl->vRescoreJudgeVector;
}

Grade
HighScore::GetWifeGrade() const
{
	return m_Impl->GetWifeGrade();
}

bool
HighScore::WriteReplayData()
{
	// return DBProfile::WriteReplayData(this);
	return m_Impl->WriteReplayData();
}

// Ok I guess we can be more lenient and convert by midwindow values, but we
// still have to assume j4 - mina
float
HighScore::ConvertDpToWife()
{
	if (m_Impl->fWifeScore > 0.f) {
		if (PREFSMAN->m_bSortBySSRNorm)
			return m_Impl->fSSRNormPercent;
		return m_Impl->fWifeScore;
	}

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
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the HighScore. */
class LunaHighScore : public Luna<HighScore>
{
  public:
	static int GetName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetName().c_str());
		return 1;
	}
	static int GetScore(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetScore());
		return 1;
	}
	static int GetPercentDP(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetPercentDP());
		return 1;
	}
	static int GetWifeScore(T* p, lua_State* L)
	{
		if (PREFSMAN->m_bSortBySSRNorm)
			lua_pushnumber(L, p->GetSSRNormPercent());
		else
			lua_pushnumber(L, p->GetWifeScore());
		return 1;
	}
	static int GetWifePoints(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetWifePoints());
		return 1;
	}
	static int GetMusicRate(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetMusicRate());
		return 1;
	}
	static int GetJudgeScale(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetJudgeScale());
		return 1;
	}
	static int GetDate(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetDateTime().GetString());
		return 1;
	}
	static int GetSurvivalSeconds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetSurvivalSeconds());
		return 1;
	}
	static int IsFillInMarker(T* p, lua_State* L)
	{
		bool bIsFillInMarker = false;
		bIsFillInMarker |= p->GetName() == RANKING_TO_FILL_IN_MARKER;
		lua_pushboolean(L, static_cast<int>(bIsFillInMarker));
		return 1;
	}
	static int GetMaxCombo(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetMaxCombo());
		return 1;
	}
	static int GetModifiers(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetModifiers().c_str());
		return 1;
	}
	static int GetTapNoteScore(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetTapNoteScore(Enum::Check<TapNoteScore>(L, 1)));
		return 1;
	}
	static int GetHoldNoteScore(T* p, lua_State* L)
	{
		lua_pushnumber(L,
					   p->GetHoldNoteScore(Enum::Check<HoldNoteScore>(L, 1)));
		return 1;
	}
	static int RescoreToWifeJudge(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->RescoreToWifeJudge(IArg(1)));
		return 1;
	}
	static int RescoreToDPJudge(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->RescoreToDPJudge(IArg(1)));
		return 1;
	}
	static int RescoreJudges(T* p, lua_State* L)
	{
		lua_newtable(L);
		for (int i = 0; i < 6; ++i) {
			lua_pushnumber(L, p->GetRescoreJudgeVector(IArg(1))[i]);
			lua_rawseti(L, -2, i + 1);
		}

		return 1;
	}
	static int GetRadarValues(T* p, lua_State* L)
	{
		RadarValues& rv = const_cast<RadarValues&>(p->GetRadarValues());
		rv.PushSelf(L);
		return 1;
	}

	static int GetSkillsetSSR(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetSkillsetSSR(Enum::Check<Skillset>(L, 1)));
		return 1;
	}
	static int ToggleEtternaValidation(T* p, lua_State* L)
	{
		p->SetEtternaValid(!p->GetEtternaValid());
		return 0;
	}

	// Convert to MS so lua doesn't have to
	// not exactly sure why i'm doing this fancy load garbage or if it works...
	// -mina
	static int GetOffsetVector(T* p, lua_State* L)
	{
		auto v = p->GetOffsetVector();
		bool loaded = v.size() > 0;
		if (loaded || p->LoadReplayData()) {
			if (!loaded)
				v = p->GetOffsetVector();
			for (size_t i = 0; i < v.size(); ++i)
				v[i] = v[i] * 1000;
			LuaHelpers::CreateTableFromArray(v, L);
		} else
			lua_pushnil(L);
		return 1;
	}

	static int GetNoteRowVector(T* p, lua_State* L)
	{
		auto* v = &(p->GetNoteRowVector());
		bool loaded = v->size() > 0;

		auto timestamps = p->GetCopyOfSetOnlineReplayTimestampVector();

		if (loaded || p->LoadReplayData()) {
			// this is a local highscore with a local replay
			// easy to just output the noterows loaded
			LuaHelpers::CreateTableFromArray((*v), L);
		} else if (!timestamps.empty() && v->empty()) {
			// this is a legacy online replay
			// missing rows but with timestamps instead
			// we can try our best to show the noterows by approximating
			GAMESTATE->SetProcessedTimingData(
			  GAMESTATE->m_pCurSteps->GetTimingData());
			auto* td = GAMESTATE->m_pCurSteps->GetTimingData();
			auto nd = GAMESTATE->m_pCurSteps->GetNoteData();
			auto nerv = nd.BuildAndGetNerv();
			auto sdifs = td->BuildAndGetEtaner(nerv);
			vector<int> noterows;
			for (auto t : timestamps) {
				auto timestamptobeat =
				  td->GetBeatFromElapsedTime(t * p->GetMusicRate());
				auto somenumberscaledbyoffsets =
				  sdifs[0] - (timestamps[0] * p->GetMusicRate());
				timestamptobeat += somenumberscaledbyoffsets;
				auto noterowfrombeat = BeatToNoteRow(timestamptobeat);
				noterows.emplace_back(noterowfrombeat);
			}
			int noterowoffsetter = nerv[0] - noterows[0];
			for (auto& noterowwithoffset : noterows)
				noterowwithoffset += noterowoffsetter;
			GAMESTATE->SetProcessedTimingData(nullptr);
			p->SetNoteRowVector(noterows);

			v = &(p->GetNoteRowVector()); // uhh

			LuaHelpers::CreateTableFromArray((*v), L);
		} else {
			// ok we got nothing, just throw null
			lua_pushnil(L);
		}
		return 1;
	}

	static int GetTrackVector(T* p, lua_State* L)
	{
		auto* v = &(p->GetTrackVector());
		bool loaded = v->size() > 0;
		if (loaded || p->LoadReplayData()) {
			if (!loaded)
				v = &(p->GetTrackVector());
			LuaHelpers::CreateTableFromArray((*v), L);
		} else
			lua_pushnil(L);
		return 1;
	}

	static int GetTapNoteTypeVector(T* p, lua_State* L)
	{
		auto* v = &(p->GetTapNoteTypeVector());
		bool loaded = v->size() > 0;
		if (loaded || p->LoadReplayData()) {
			if (!loaded)
				v = &(p->GetTapNoteTypeVector());
			LuaHelpers::CreateTableFromArray((*v), L);
		} else
			lua_pushnil(L);
		return 1;
	}
	static int GetJudgmentString(T* p, lua_State* L)
	{
		RString doot = ssprintf("%d I %d I %d I %d I %d I %d  x%d",
								p->GetTapNoteScore(TNS_W1),
								p->GetTapNoteScore(TNS_W2),
								p->GetTapNoteScore(TNS_W3),
								p->GetTapNoteScore(TNS_W4),
								p->GetTapNoteScore(TNS_W5),
								p->GetTapNoteScore(TNS_Miss),
								p->GetMaxCombo());
		lua_pushstring(L, doot);
		return 1;
	}

	static int GetUserid(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->userid);
		return 1;
	}
	static int GetScoreid(T* p, lua_State* L)
	{
		lua_pushstring(L, RString(p->scoreid));
		return 1;
	}
	static int GetAvatar(T* p, lua_State* L)
	{
		lua_pushstring(L, RString(p->avatar));
		return 1;
	}

	DEFINE_METHOD(GetGrade, GetGrade())
	DEFINE_METHOD(GetWifeGrade, GetWifeGrade())
	DEFINE_METHOD(ConvertDpToWife, ConvertDpToWife())
	DEFINE_METHOD(GetChordCohesion, GetChordCohesion())
	DEFINE_METHOD(GetEtternaValid, GetEtternaValid())
	DEFINE_METHOD(HasReplayData, HasReplayData())
	DEFINE_METHOD(GetChartKey, GetChartKey())
	DEFINE_METHOD(GetScoreKey, GetScoreKey())
	DEFINE_METHOD(GetReplayType, GetReplayType())
	DEFINE_METHOD(GetDisplayName, GetDisplayName())
	LunaHighScore()
	{
		ADD_METHOD(GetName);
		ADD_METHOD(GetScore);
		ADD_METHOD(GetPercentDP);
		ADD_METHOD(ConvertDpToWife);
		ADD_METHOD(GetWifeScore);
		ADD_METHOD(GetWifePoints);
		//ADD_METHOD(RescoreToWifeJudge);
		//ADD_METHOD( RescoreToDPJudge );
		ADD_METHOD(RescoreJudges);
		ADD_METHOD(GetSkillsetSSR);
		ADD_METHOD(GetMusicRate);
		ADD_METHOD(GetJudgeScale);
		ADD_METHOD(GetChordCohesion);
		ADD_METHOD(GetDate);
		ADD_METHOD(GetSurvivalSeconds);
		ADD_METHOD(IsFillInMarker);
		ADD_METHOD(GetModifiers);
		ADD_METHOD(GetTapNoteScore);
		ADD_METHOD(GetHoldNoteScore);
		ADD_METHOD(GetRadarValues);
		ADD_METHOD(GetGrade);
		ADD_METHOD(GetWifeGrade);
		ADD_METHOD(GetMaxCombo);
		ADD_METHOD(ToggleEtternaValidation);
		ADD_METHOD(GetEtternaValid);
		ADD_METHOD(HasReplayData);
		ADD_METHOD(GetOffsetVector);
		ADD_METHOD(GetNoteRowVector);
		ADD_METHOD(GetTrackVector);
		ADD_METHOD(GetTapNoteTypeVector);
		ADD_METHOD(GetChartKey);
		ADD_METHOD(GetReplayType);
		ADD_METHOD(GetJudgmentString);
		ADD_METHOD(GetDisplayName);
		ADD_METHOD(GetUserid);
		ADD_METHOD(GetScoreid);
		ADD_METHOD(GetScoreKey);
		ADD_METHOD(GetAvatar);
	}
};

LUA_REGISTER_CLASS(HighScore)

/** @brief Allow Lua to have access to the HighScoreList. */
class LunaHighScoreList : public Luna<HighScoreList>
{
  public:
	static int GetHighScores(T* p, lua_State* L)
	{
		lua_newtable(L);
		for (int i = 0; i < static_cast<int>(p->vHighScores.size()); ++i) {
			p->vHighScores[i].PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}

		return 1;
	}

	static int GetHighestScoreOfName(T* p, lua_State* L)
	{
		string name = SArg(1);
		for (size_t i = 0; i < p->vHighScores.size(); ++i) {
			if (name == p->vHighScores[i].GetName()) {
				p->vHighScores[i].PushSelf(L);
				return 1;
			}
		}
		lua_pushnil(L);
		return 1;
	}

	static int GetRankOfName(T* p, lua_State* L)
	{
		string name = SArg(1);
		size_t rank = 0;
		for (size_t i = 0; i < p->vHighScores.size(); ++i) {
			if (name == p->vHighScores[i].GetName()) {
				// Indices from Lua are one-indexed.  +1 to adjust.
				rank = i + 1;
				break;
			}
		}
		// The themer is expected to check for validity before using.
		lua_pushnumber(L, rank);
		return 1;
	}

	LunaHighScoreList()
	{
		ADD_METHOD(GetHighScores);
		ADD_METHOD(GetHighestScoreOfName);
		ADD_METHOD(GetRankOfName);
	}
};

LUA_REGISTER_CLASS(HighScoreList)
// lua end
