#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/CryptManager.h"
#include "GameConstantsAndTypes.h"
#include "HighScore.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RadarValues.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/FileTypes/XmlFile.h"
#include "NoteTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/Misc/TimingData.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "RageUtil/File/RageFileManager.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

#include "Etterna/Singletons/ScoreManager.h"

const std::string BASIC_REPLAY_DIR =
  "Save/Replays/"; // contains only tap offset data for rescoring/plots -mina
const std::string FULL_REPLAY_DIR =
  "Save/ReplaysV2/"; // contains freeze drops and mine hits as well as tap
					 // offsets; fully "rewatchable" -mina

struct HighScoreImpl
{
	std::string sName; // name that shows in the machine's ranking screen

	/* a half-misnomer now- since all scores are keyed by the chart key this
	should never change/be different, but its historical correctness is still
	correct, though it should prolly be renamed tbh -mina*/
	std::string ChartKey;

	std::string ScoreKey;
	int SSRCalcVersion;
	Grade grade;
	unsigned int iScore;
	float fPercentDP;
	float fWifeScore;
	float fWifePoints;
	float fSSRNormPercent;
	float played_seconds;
	float fMusicRate;
	float fJudgeScale;
	bool bNoChordCohesion;
	bool bEtternaValid;
	std::vector<std::string> uploaded;
	std::vector<float> vOffsetVector;
	std::vector<int> vNoteRowVector;
	std::vector<int> vTrackVector;
	std::vector<TapNoteType> vTapNoteTypeVector;
	std::vector<HoldReplayResult> vHoldReplayDataVector;
	std::vector<float> vOnlineReplayTimestampVector;
	std::vector<int> vRescoreJudgeVector;
	unsigned int iMaxCombo; // maximum combo obtained [SM5 alpha 1a+]
	std::string sModifiers;
	DateTime dateTime; // return value of time() for when the highscore object
					   // was created (immediately after achieved)
	std::string sPlayerGuid;  // who made this high score
	std::string sMachineGuid; // where this high score was made
	std::string countryCode;
	int iProductID;
	int iTapNoteScores[NUM_TapNoteScore]{};
	int tnsnorm[NUM_TapNoteScore]{};
	int iHoldNoteScores[NUM_HoldNoteScore]{};
	int hnsnorm[NUM_HoldNoteScore]{};
	float fSkillsetSSRs[NUM_Skillset]{};
	std::string ValidationKeys[NUM_ValidationKey];
	RadarValues radarValues;
	float fLifeRemainingSeconds;
	bool bDisqualified;
	std::string ValidationKey;
	int TopScore;

	HighScoreImpl();

	[[nodiscard]] auto CreateEttNode() const -> XNode*;
	void LoadFromEttNode(const XNode* pNode);
	[[nodiscard]] auto GetWifeGrade() const -> Grade;
	void UnloadReplayData();
	void ResetSkillsets();

	auto WriteReplayData() -> bool;
	int ReplayType; // 0 = no loaded replay, 1 = basic, 2 = full; currently
					// unused but here for when we need it (not to be confused
					// with hasreplay()) -mina

	bool is39import = false;
	int WifeVersion = 0;

	auto operator==(const HighScoreImpl& other) const -> bool;
	auto operator!=(const HighScoreImpl& other) const -> bool
	{
		return !(*this == other);
	}
};

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

auto
HighScoreImpl::GetWifeGrade() const -> Grade
{
	if (grade == Grade_Failed) {
		return Grade_Failed;
	}

	auto pc = fWifeScore;

	if (PREFSMAN->m_bSortBySSRNorm) {
		pc = fSSRNormPercent;
	}

	return GetGradeFromPercent(pc);
}

void
HighScoreImpl::ResetSkillsets()
{
	FOREACH_ENUM(Skillset, ss)
	fSkillsetSSRs[ss] = 0.F;
}

HighScoreImpl::HighScoreImpl()
{
	sName = "";
	ChartKey = "";
	ScoreKey = "";
	SSRCalcVersion = 0;
	grade = Grade_Invalid;
	iScore = 0;
	fPercentDP = 0.F;
	fWifeScore = 0.F;
	fWifePoints = 0.F;
	fSSRNormPercent = 0.F;
	fMusicRate = 0.F;
	fJudgeScale = 0.F;
	bEtternaValid = true;
	vOffsetVector.clear();
	vNoteRowVector.clear();
	vRescoreJudgeVector.clear();
	played_seconds = 0.F;
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
	std::string ValidationKey;
	TopScore = 0;
	ReplayType = 2;
	bNoChordCohesion = false;
	bDisqualified = false;
	WifeVersion = 0;
}

auto
HighScoreImpl::CreateEttNode() const -> XNode*
{
	auto* pNode = new XNode("Score");

	if (ScoreKey.empty()) {
		pNode->AppendAttr("Key",
						  "S" + BinaryToHex(CryptManager::GetSHA1ForString(
								  dateTime.GetString())));
	} else {
		pNode->AppendAttr("Key", ScoreKey);
	}

	pNode->AppendChild("SSRCalcVersion", SSRCalcVersion);
	pNode->AppendChild("Grade", GradeToString(GetWifeGrade()));
	pNode->AppendChild("WifeScore", fWifeScore);

	if (fWifePoints > 0.F) {
		pNode->AppendChild("WifePoints", fWifePoints);
	}

	pNode->AppendChild("SSRNormPercent", fSSRNormPercent);
	pNode->AppendChild("JudgeScale", fJudgeScale);
	pNode->AppendChild("NoChordCohesion", bNoChordCohesion);
	pNode->AppendChild("EtternaValid", bEtternaValid);
	pNode->AppendChild("PlayedSeconds", played_seconds);
	pNode->AppendChild("MaxCombo", iMaxCombo);
	pNode->AppendChild("Modifiers", sModifiers);
	pNode->AppendChild("MachineGuid", sMachineGuid);
	pNode->AppendChild("DateTime", dateTime.GetString());
	pNode->AppendChild("TopScore", TopScore);
	if (!uploaded.empty()) {
		auto* pServerNode = pNode->AppendChild("Servs");
		for (const auto& server : uploaded) {
			pServerNode->AppendChild("server", server);
		}
	}

	auto* pTapNoteScores = pNode->AppendChild("TapNoteScores");
	FOREACH_ENUM(TapNoteScore, tns)
	if (tns != TNS_None && tns != TNS_CheckpointMiss &&
		tns != TNS_CheckpointHit) {
		pTapNoteScores->AppendChild(TapNoteScoreToString(tns),
									iTapNoteScores[tns]);
	}

	auto* pHoldNoteScores = pNode->AppendChild("HoldNoteScores");
	FOREACH_ENUM(HoldNoteScore, hns)
	if (hns != HNS_None) { // HACK: don't save meaningless "none" count
		pHoldNoteScores->AppendChild(HoldNoteScoreToString(hns),
									 iHoldNoteScores[hns]);
	}

	// dont bother writing skillset ssrs for non-applicable scores
	if (fWifeScore > 0.F && grade != Grade_Failed &&
		fSkillsetSSRs[Skill_Overall] > 0.F) {
		auto* pSkillsetSSRs = pNode->AppendChild("SkillsetSSRs");
		FOREACH_ENUM(Skillset, ss)
		pSkillsetSSRs->AppendChild(
		  SkillsetToString(ss), FloatToString(fSkillsetSSRs[ss]).substr(0, 5));
	}

	auto* pValidationKeys = pNode->AppendChild("ValidationKeys");
	pValidationKeys->AppendChild(ValidationKeyToString(ValidationKey_Brittle),
								 ValidationKeys[ValidationKey_Brittle]);
	pValidationKeys->AppendChild(ValidationKeyToString(ValidationKey_Weak),
								 ValidationKeys[ValidationKey_Weak]);
	pNode->AppendChild("wv", WifeVersion);
	return pNode;
}

void
HighScoreImpl::LoadFromEttNode(const XNode* pNode)
{
	// ASSERT(pNode->GetName() == "Score");

	std::string s;
	pNode->GetChildValue("SSRCalcVersion", SSRCalcVersion);
	if (pNode->GetChildValue("Grade", s)) {
		grade = StringToGrade(s);
	}
	pNode->GetChildValue("WifeScore", fWifeScore);
	pNode->GetChildValue("WifePoints", fWifePoints);
	pNode->GetChildValue("SSRNormPercent", fSSRNormPercent);
	pNode->GetChildValue("Rate", fMusicRate);
	pNode->GetChildValue("JudgeScale", fJudgeScale);
	pNode->GetChildValue("NoChordCohesion", bNoChordCohesion);
	pNode->GetChildValue("EtternaValid", bEtternaValid);
	const auto* pUploadedServers = pNode->GetChild("Servs");
	if (pUploadedServers != nullptr) {
		FOREACH_CONST_Child(pUploadedServers, p)
		{
			std::string server;
			p->GetTextValue(server);
			uploaded.emplace_back(server.c_str());
		}
	}
	
	// Attempt to recover SurviveSeconds or otherwise try to
	// load a correct number here
	auto psSuccess = pNode->GetChildValue("PlayedSeconds", played_seconds);
	if (!psSuccess || played_seconds <= 1.F) {
		played_seconds = 0.F;
		
		// Attempt to use the chart length for PlayedSeconds
		// Only if the grade is a fail
		auto* steps = SONGMAN->GetStepsByChartkey(ChartKey);
		float sLength = 0.F;
		if (steps != nullptr) {
			// division by zero guard
			auto r = 1.F;
			if (fMusicRate > 0.F)
				r = fMusicRate;
			
			sLength = steps->GetLengthSeconds(r);
		}
		float survSeconds = 0.F;
		auto ssSuccess = pNode->GetChildValue("SurviveSeconds", survSeconds);
		if (grade == Grade_Failed && survSeconds > 0.F && ssSuccess) {
			// fail: didnt finish chart
			// go ahead and use SurviveSeconds if present
			played_seconds = survSeconds;
		} else if (sLength > 0.F) {
			// chart was loaded
			played_seconds = sLength;
		} else {
			// chart wasnt loaded
			played_seconds = survSeconds;
		}
		// the end result if SurviveSeconds is not present and the chart is not loaded
		// is simply that played_seconds is 0. there is no recovery.
		// (setting it to 0 grants potential to fix it later)
	}

	
	pNode->GetChildValue("MaxCombo", iMaxCombo);
	if (pNode->GetChildValue("Modifiers", s)) {
		sModifiers = s;
	}
	if (pNode->GetChildValue("DateTime", s)) {
		dateTime.FromString(s);
	}
	if (pNode->GetChildValue("ScoreKey", s)) {
		ScoreKey = s;
	}
	if (pNode->GetChildValue("MachineGuid", s)) {
		sMachineGuid = s;
	}

	const auto* pTapNoteScores = pNode->GetChild("TapNoteScores");
	if (pTapNoteScores != nullptr) {
		FOREACH_ENUM(TapNoteScore, tns)
		pTapNoteScores->GetChildValue(TapNoteScoreToString(tns),
									  iTapNoteScores[tns]);
	}

	const auto* pHoldNoteScores = pNode->GetChild("HoldNoteScores");
	if (pHoldNoteScores != nullptr) {
		FOREACH_ENUM(HoldNoteScore, hns)
		pHoldNoteScores->GetChildValue(HoldNoteScoreToString(hns),
									   iHoldNoteScores[hns]);
	}

	if (fWifeScore > 0.F) {
		const auto* pSkillsetSSRs = pNode->GetChild("SkillsetSSRs");
		if (pSkillsetSSRs != nullptr) {
			FOREACH_ENUM(Skillset, ss)
			pSkillsetSSRs->GetChildValue(SkillsetToString(ss),
										 fSkillsetSSRs[ss]);
		}
	}

	if (fWifeScore > 0.F) {
		const auto* pValidationKeys = pNode->GetChild("ValidationKeys");
		if (pValidationKeys != nullptr) {
			if (pValidationKeys->GetChildValue(
				  ValidationKeyToString(ValidationKey_Brittle), s)) {
				ValidationKeys[ValidationKey_Brittle] = s;
			}
			if (pValidationKeys->GetChildValue(
				  ValidationKeyToString(ValidationKey_Weak), s)) {
				ValidationKeys[ValidationKey_Weak] = s;
			}
		}
	}

	if (ScoreKey.empty()) {
		ScoreKey =
		  "S" +
		  BinaryToHex(CryptManager::GetSHA1ForString(dateTime.GetString()));
	}

	pNode->GetChildValue("wv", WifeVersion);

	// Validate input.
	grade = std::clamp(grade, Grade_Tier01, Grade_Failed);
}

auto
HighScoreImpl::WriteReplayData() -> bool
{
	Locator::getLogger()->trace("Writing out replay data to disk.");
	std::string append;
	std::string profiledir;
	// These two lines should probably be somewhere else
	if (!FILEMAN->IsADirectory(FULL_REPLAY_DIR)) {
		FILEMAN->CreateDir(FULL_REPLAY_DIR);
	}
	const auto path = FULL_REPLAY_DIR + ScoreKey;
	std::ofstream fileStream(path, std::ios::binary);
	// check file

	ASSERT(!vNoteRowVector.empty());

	if (!fileStream) {
		Locator::getLogger()->warn("Failed to create replay file at {}", path.c_str());
		return false;
	}

	const unsigned int idx = vNoteRowVector.size() - 1;
	// loop for writing both vectors side by side
	for (unsigned int i = 0; i <= idx; i++) {
		append = std::to_string(vNoteRowVector[i]) + " " +
				 std::to_string(vOffsetVector[i]) + " " +
				 std::to_string(vTrackVector[i]) +
				 (vTapNoteTypeVector[i] != TapNoteType_Tap
					? " " + std::to_string(vTapNoteTypeVector[i])
					: "") +
				 "\n";
		fileStream.write(append.c_str(), append.size());
	}
	for (auto& hold : vHoldReplayDataVector) {
		append = "H " + std::to_string(hold.row) + " " +
				 std::to_string(hold.track) +
				 (hold.subType != TapNoteSubType_Hold
					? " " + std::to_string(hold.subType)
					: "") +
				 "\n";
		fileStream.write(append.c_str(), append.size());
	}
	fileStream.close();
	Locator::getLogger()->trace("Created replay file at {}", path.c_str());
	return true;
}

auto
HighScore::WriteInputData(const std::vector<float>& oop) -> bool
{
	std::string append;
	std::string profiledir;

	const auto path = FULL_REPLAY_DIR + m_Impl->ScoreKey;
	std::ofstream fileStream(path, std::ios::binary);
	// check file

	ASSERT(!oop.empty());

	if (!fileStream) {
		Locator::getLogger()->warn("Failed to create replay file at {}", path.c_str());
		return false;
	}

	const unsigned int idx = oop.size() - 1;
	// loop for writing both vectors side by side
	for (unsigned int i = 0; i <= idx; i++) {
		append = std::to_string(oop[i]) + "\n";
		fileStream.write(append.c_str(), append.size());
	}
	fileStream.close();
	Locator::getLogger()->trace("Created replay file at {}", path.c_str());
	return true;
}

// should just get rid of impl -mina
auto
HighScore::LoadReplayData() -> bool
{ // see dir definition comments at the top -mina
	if (LoadReplayDataFull(FULL_REPLAY_DIR)) {
		return true;
	}
	return LoadReplayDataBasic(BASIC_REPLAY_DIR);
}

auto
HighScore::LoadReplayDataBasic(const std::string& dir) -> bool
{
	// already exists
	if (m_Impl->vNoteRowVector.size() > 4 && m_Impl->vOffsetVector.size() > 4) {
		return true;
	}

	std::string profiledir;
	std::vector<int> vNoteRowVector;
	std::vector<float> vOffsetVector;
	auto path = dir + m_Impl->ScoreKey;

	std::ifstream fileStream(path, std::ios::binary);
	std::string line;
	std::string buffer;
	std::vector<string> tokens;
	int noteRow;
	float offset;

	// check file
	if (!fileStream) {
		Locator::getLogger()->trace("Failed to load replay data at {}", path.c_str());
		return false;
	}

	// loop until eof
	try {

		while (getline(fileStream, line)) {
			std::stringstream ss(line);
			// split line into tokens
			while (ss >> buffer) {
				tokens.emplace_back(buffer);
			}

			if (tokens.size() > 2) {
				Locator::getLogger()->warn(
				  "looks like u got v2 replays in the v1 folder, move them "
				  "into Save/ReplaysV2 folder if you want them to load. If {} "
				  "is not a v2 replay that you placed into the Save/Replays "
				  "folder by accident, then it is probably corrupted and you "
				  "should delete it or move it out",
				  GetScoreKey().c_str());
				ASSERT(tokens.size() < 2);
			}

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
		Locator::getLogger()->warn("Failed to load replay data at {} due to runtime exception: {}",
		  path.c_str(),
		  e.what());
		fileStream.close();
		return false;
	}
	fileStream.close();
	SetNoteRowVector(vNoteRowVector);
	SetOffsetVector(vOffsetVector);

	m_Impl->ReplayType = 1;
	Locator::getLogger()->trace("Loaded replay data type 1 at {}", path.c_str());
	return true;
}

auto
HighScore::LoadReplayDataFull(const std::string& dir) -> bool
{
	if (m_Impl->vNoteRowVector.size() > 4 && m_Impl->vOffsetVector.size() > 4 &&
		m_Impl->vTrackVector.size() > 4) {
		m_Impl->ReplayType = 2;
		return true;
	}

	std::string profiledir;
	std::vector<int> vNoteRowVector;
	std::vector<float> vOffsetVector;
	std::vector<int> vTrackVector;
	std::vector<TapNoteType> vTapNoteTypeVector;
	std::vector<HoldReplayResult> vHoldReplayDataVector;
	auto path = dir + m_Impl->ScoreKey;

	std::ifstream fileStream(path, std::ios::binary);
	std::string line;
	std::string buffer;
	std::vector<std::string> tokens;
	int noteRow;
	float offset;
	int track;
	TapNoteType tnt;
	int tmp;

	// check file
	if (!fileStream) {
		return false;
	}

	// loop until eof
	while (getline(fileStream, line)) {
		std::stringstream ss(line);
		// split line into tokens
		while (ss >> buffer) {
			tokens.emplace_back(buffer);
		}

		// probably replaydatav1 in the wrong folder, we could throw a trace or
		// a warn but i feel like nobody will care or do anything about it and
		// it will just pollute the log, nobody is going to parse the log and
		// properly split up their replays back into the respective folders
		// so...
		if (tokens.size() < 3) {
			return LoadReplayDataBasic(dir);
		}

		if (tokens[0] == "H") {
			HoldReplayResult hrr;
			hrr.row = std::stoi(tokens[1]);
			hrr.track = std::stoi(tokens[2]);
			tmp =
			  tokens.size() > 3 ? std::stoi(tokens[3]) : TapNoteSubType_Hold;
			if (tmp < 0 || tmp >= NUM_TapNoteSubType ||
				!(typeid(tmp) == typeid(int))) {
				Locator::getLogger()->warn("Failed to load replay data at {} (\"Tapnotesubtype value is not of type TapNoteSubType\")",
						  path.c_str());
			}
			hrr.subType = static_cast<TapNoteSubType>(tmp);
			vHoldReplayDataVector.emplace_back(hrr);
			tokens.clear();
			continue;
		}

		auto a = buffer == "1";
		a = buffer == "2" || a;
		a = buffer == "3" || a;
		a = buffer == "4" || a;
		a = buffer == "5" || a;
		a = buffer == "6" || a;
		a = buffer == "7" || a;
		a = buffer == "8" || a;
		a = buffer == "9" || a;
		a = buffer == "0" || a;
		if (!a) {
			Locator::getLogger()->warn("Replay data at {} appears to be HOT BROKEN GARBAGE WTF", path.c_str());
			return false;
		}

		noteRow = std::stoi(tokens[0]);
		if (!(typeid(noteRow) == typeid(int))) {
			Locator::getLogger()->warn("Failed to load replay data at {} (\"NoteRow value is "
					  "not of type: int\")",
					  path.c_str());
		}
		vNoteRowVector.emplace_back(noteRow);

		offset = std::stof(tokens[1]);
		if (!(typeid(offset) == typeid(float))) {
			Locator::getLogger()->warn("Failed to load replay data at {} (\"Offset value is not "
					  "of type: float\")",
					  path.c_str());
		}
		vOffsetVector.emplace_back(offset);

		track = std::stoi(tokens[2]);
		if (!(typeid(track) == typeid(int))) {
			Locator::getLogger()->warn("Failed to load replay data at {} (\"Track/Column value "
					  "is not of type: int\")",
					  path.c_str());
		}
		vTrackVector.emplace_back(track);

		tmp = tokens.size() >= 4 ? std::stoi(tokens[3]) : TapNoteType_Tap;
		if (tmp < 0 || tmp >= TapNoteType_Invalid ||
			!(typeid(tmp) == typeid(int))) {
			Locator::getLogger()->warn("Failed to load replay data at {} (\"Tapnotetype value "
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
	Locator::getLogger()->trace("Loaded replay data type 2 at {}", path.c_str());
	return true;
}

auto
HighScore::HasReplayData() -> bool
{
	const auto fullpath = FULL_REPLAY_DIR + m_Impl->ScoreKey;
	const auto basicpath = BASIC_REPLAY_DIR + m_Impl->ScoreKey;
	if (DoesFileExist(fullpath)) { // check for full replays first then default
								   // to basic replays -mina
		return true;
	}
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

auto
HighScore::IsEmpty() const -> bool
{
	if ((m_Impl->iTapNoteScores[TNS_W1] != 0) ||
		(m_Impl->iTapNoteScores[TNS_W2] != 0) ||
		(m_Impl->iTapNoteScores[TNS_W3] != 0) ||
		(m_Impl->iTapNoteScores[TNS_W4] != 0) ||
		(m_Impl->iTapNoteScores[TNS_W5] != 0)) {
		return false;
	}
	if (m_Impl->iHoldNoteScores[HNS_Held] > 0) {
		return false;
	}
	return true;
}

auto
HighScore::GetName() const -> const std::string&
{
	return m_Impl->sName;
}
auto
HighScore::GetChartKey() const -> const std::string&
{
	return m_Impl->ChartKey;
}
auto
HighScore::GetSSRCalcVersion() const -> int
{
	return m_Impl->SSRCalcVersion;
}
auto
HighScore::GetGrade() const -> Grade
{
	return m_Impl->grade;
}
auto
HighScore::GetScore() const -> unsigned int
{
	return m_Impl->iScore;
}
auto
HighScore::GetMaxCombo() const -> unsigned int
{
	return m_Impl->iMaxCombo;
}

auto
HighScore::GetPercentDP() const -> float
{
	return m_Impl->fPercentDP;
}
auto
HighScore::GetWifeScore() const -> float
{
	return m_Impl->fWifeScore;
}
auto
HighScore::GetWifePoints() const -> float
{
	return m_Impl->fWifePoints;
}
auto
HighScore::GetSSRNormPercent() const -> float
{
	return m_Impl->fSSRNormPercent;
}
auto
HighScore::GetMusicRate() const -> float
{
	return m_Impl->fMusicRate;
}
auto
HighScore::GetJudgeScale() const -> float
{
	return m_Impl->fJudgeScale;
}
auto
HighScore::GetChordCohesion() const -> bool
{
	return !m_Impl->bNoChordCohesion;
}
auto
HighScore::GetEtternaValid() const -> bool
{
	return m_Impl->bEtternaValid;
}
auto
HighScore::IsUploadedToServer(const std::string& s) const -> bool
{
	return find(m_Impl->uploaded.begin(), m_Impl->uploaded.end(), s) !=
		   m_Impl->uploaded.end();
}
auto
HighScore::GetCopyOfOffsetVector() const -> std::vector<float>
{
	return m_Impl->vOffsetVector;
}
auto
HighScore::GetCopyOfNoteRowVector() const -> std::vector<int>
{
	return m_Impl->vNoteRowVector;
}
auto
HighScore::GetCopyOfTrackVector() const -> std::vector<int>
{
	return m_Impl->vTrackVector;
}
auto
HighScore::GetCopyOfTapNoteTypeVector() const -> std::vector<TapNoteType>
{
	return m_Impl->vTapNoteTypeVector;
}
auto
HighScore::GetCopyOfHoldReplayDataVector() const
  -> std::vector<HoldReplayResult>
{
	return m_Impl->vHoldReplayDataVector;
}
auto
HighScore::GetCopyOfSetOnlineReplayTimestampVector() const -> std::vector<float>
{
	return m_Impl->vOnlineReplayTimestampVector;
}
auto
HighScore::GetOffsetVector() const -> const std::vector<float>&
{
	return m_Impl->vOffsetVector;
}
auto
HighScore::GetNoteRowVector() const -> const std::vector<int>&
{
	return m_Impl->vNoteRowVector;
}
auto
HighScore::GetTrackVector() const -> const std::vector<int>&
{
	return m_Impl->vTrackVector;
}
auto
HighScore::GetTapNoteTypeVector() const -> const std::vector<TapNoteType>&
{
	return m_Impl->vTapNoteTypeVector;
}
auto
HighScore::GetHoldReplayDataVector() const
  -> const std::vector<HoldReplayResult>&
{
	return m_Impl->vHoldReplayDataVector;
}
auto
HighScore::GetScoreKey() const -> const std::string&
{
	return m_Impl->ScoreKey;
}
auto
HighScore::GetPlayedSeconds() const -> float
{
	return m_Impl->played_seconds;
}
auto
HighScore::GetModifiers() const -> const std::string&
{
	return m_Impl->sModifiers;
}
auto
HighScore::GetDateTime() const -> DateTime
{
	return m_Impl->dateTime;
}
auto
HighScore::GetPlayerGuid() const -> const std::string&
{
	return m_Impl->sPlayerGuid;
}
auto
HighScore::GetMachineGuid() const -> const std::string&
{
	return m_Impl->sMachineGuid;
}
auto
HighScore::GetCountryCode() const -> const std::string&
{
	return m_Impl->countryCode;
}
auto
HighScore::GetProductID() const -> int
{
	return m_Impl->iProductID;
}
auto
HighScore::GetTapNoteScore(TapNoteScore tns) const -> int
{
	return m_Impl->iTapNoteScores[tns];
}
auto
HighScore::GetHoldNoteScore(HoldNoteScore hns) const -> int
{
	return m_Impl->iHoldNoteScores[hns];
}
auto
HighScore::GetSkillsetSSR(Skillset ss) const -> float
{
	return m_Impl->fSkillsetSSRs[ss];
}
auto
HighScore::GetWifeVersion() const -> int
{
	return m_Impl->WifeVersion;
}
auto
HighScore::GetRadarValues() const -> const RadarValues&
{
	return m_Impl->radarValues;
}
auto
HighScore::GetLifeRemainingSeconds() const -> float
{
	return m_Impl->fLifeRemainingSeconds;
}
auto
HighScore::GetDisqualified() const -> bool
{
	return m_Impl->bDisqualified;
}
auto
HighScore::GetTopScore() const -> int
{
	return m_Impl->TopScore;
}
auto
HighScore::GetReplayType() const -> int
{
	return m_Impl->ReplayType;
}

void
HighScore::SetName(const std::string& sName)
{
	m_Impl->sName = sName;
}
void
HighScore::SetChartKey(const std::string& ck)
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
HighScore::SetPlayedSeconds(float f)
{
	m_Impl->played_seconds = f;
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
HighScore::AddUploadedServer(const std::string& s)
{
	if (find(m_Impl->uploaded.begin(), m_Impl->uploaded.end(), s) ==
		m_Impl->uploaded.end()) {
		m_Impl->uploaded.emplace_back(s);
	}
}
void
HighScore::SetOffsetVector(const std::vector<float>& v)
{
	m_Impl->vOffsetVector = v;
}
void
HighScore::SetNoteRowVector(const std::vector<int>& v)
{
	m_Impl->vNoteRowVector = v;
}
void
HighScore::SetTrackVector(const std::vector<int>& v)
{
	m_Impl->vTrackVector = v;
}
void
HighScore::SetTapNoteTypeVector(const std::vector<TapNoteType>& v)
{
	m_Impl->vTapNoteTypeVector = v;
}
void
HighScore::SetHoldReplayDataVector(const std::vector<HoldReplayResult>& v)
{
	m_Impl->vHoldReplayDataVector = v;
}
void
HighScore::SetOnlineReplayTimestampVector(const std::vector<float>& v)
{
	m_Impl->vOnlineReplayTimestampVector = v;
}
void
HighScore::SetScoreKey(const std::string& sk)
{
	m_Impl->ScoreKey = sk;
}
void
HighScore::SetRescoreJudgeVector(const std::vector<int>& v)
{
	m_Impl->vRescoreJudgeVector = v;
}
void
HighScore::SetModifiers(const std::string& s)
{
	m_Impl->sModifiers = s;
}
void
HighScore::SetDateTime(DateTime d)
{
	m_Impl->dateTime = d;
}
void
HighScore::SetPlayerGuid(const std::string& s)
{
	m_Impl->sPlayerGuid = s;
}
void
HighScore::SetMachineGuid(const std::string& s)
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
	m_Impl->ValidationKeys[vk] = std::move(k);
}
void
HighScore::SetTopScore(int i)
{
	m_Impl->TopScore = i;
}
auto
HighScore::GetValidationKey(ValidationKey vk) const -> const std::string&
{
	return m_Impl->ValidationKeys[vk];
}
void
HighScore::SetWifeVersion(int i)
{
	m_Impl->WifeVersion = i;
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
auto
HighScore::GetNameMutable() -> std::string*
{
	return &m_Impl->sName;
}

auto
HighScore::GenerateValidationKeys() -> std::string
{
	std::string key;

	FOREACH_ENUM(TapNoteScore, tns)
	{

		if (tns == TNS_AvoidMine || tns == TNS_CheckpointHit ||
			tns == TNS_CheckpointMiss || tns == TNS_None) {
			continue;
		}

		key.append(std::to_string(GetTapNoteScore(tns)));
	}

	FOREACH_ENUM(HoldNoteScore, hns)
	{
		if (hns == HNS_None) {
			continue;
		}

		key.append(std::to_string(GetHoldNoteScore(hns)));
	}

	norms = lround(GetSSRNormPercent() * 1000000.F);
	musics = lround(GetMusicRate() * 100.F);
	judges = lround(GetJudgeScale() * 100.F);

	key.append(GetScoreKey());
	key.append(GetChartKey());
	key.append(GetModifiers());
	key.append(GetMachineGuid());
	key.append(std::to_string(norms));
	key.append(std::to_string(musics));
	key.append(std::to_string(judges));
	key.append(std::to_string(static_cast<int>(!GetChordCohesion())));
	key.append(std::to_string(static_cast<int>(GetEtternaValid())));
	key.append(GradeToString(GetWifeGrade()));

	std::string hash_string = CryptManager::GetSHA256ForString(key);
	std::string hash_hex_str =
	  BinaryToHex(hash_string.data(), hash_string.size());

	SetValidationKey(ValidationKey_Brittle, hash_hex_str);

	// just testing stuff
	// hs.SetValidationKey(ValidationKey_Weak,
	// GenerateWeakValidationKey(m_iTapNoteScores, m_iHoldNoteScores));
	return key;
}

auto
HighScore::operator<(HighScore const& other) const -> bool
{
	return GetWifeScore() < other.GetWifeScore();
}

auto
HighScore::operator>(HighScore const& other) const -> bool
{
	return other.operator<(*this);
}

auto
HighScore::operator<=(const HighScore& other) const -> bool
{
	return !operator>(other);
}

auto
HighScore::operator>=(const HighScore& other) const -> bool
{
	return !operator<(other);
}

auto
HighScore::operator==(const HighScore& other) const -> bool
{
	return m_Impl->ScoreKey == other.m_Impl->ScoreKey;
}

auto
HighScore::operator!=(const HighScore& other) const -> bool
{
	return !operator==(other);
}

auto
HighScore::CreateEttNode() const -> XNode*
{
	return m_Impl->CreateEttNode();
}

// Used to load from etterna.xml -mina
void
HighScore::LoadFromEttNode(const XNode* pNode)
{
	m_Impl->LoadFromEttNode(pNode);
	m_Impl->vNoteRowVector.clear();
	m_Impl->vOffsetVector.clear();
}

auto
HighScore::GetDisplayName() const -> const std::string&
{
	return GetName();
}

auto
Screenshot::CreateNode() const -> XNode*
{
	auto* pNode = new XNode("Screenshot");

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
	const auto* pHighScore = pNode->GetChild("HighScore");
}

auto
HighScore::RescoreToWife2Judge(int x) -> float
{
	if (!LoadReplayData()) {
		return m_Impl->fWifeScore;
	}

	const float tso[] = { 1.50F, 1.33F, 1.16F, 1.00F, 0.84F,
						  0.66F, 0.50F, 0.33F, 0.20F };
	const auto ts = tso[x - 1];
	float p = 0;

	// the typevector is only available for full replays
	if (m_Impl->ReplayType == 2) {
		for (size_t i = 0; i < m_Impl->vOffsetVector.size(); i++) {
			// by the powers of god invested in me i declare these vectors the
			// same size so this works all the time no matter what
			auto& type = m_Impl->vTapNoteTypeVector[i];
			if (type == TapNoteType_Tap || type == TapNoteType_HoldHead ||
				type == TapNoteType_Lift) {
				p += wife2(m_Impl->vOffsetVector[i], ts);
			}
		}
	} else {
		// blindly assume the offset vector is correct for old replays
		for (auto& n : m_Impl->vOffsetVector) {
			p += wife2(n, ts);
		}
	}

	p += (m_Impl->iHoldNoteScores[HNS_LetGo] +
		  m_Impl->iHoldNoteScores[HNS_Missed]) *
		 -6.F;
	p += m_Impl->iTapNoteScores[TNS_HitMine] * -8.F;

	// this is a bad assumption but im leaving it here
	auto pmax = static_cast<float>(m_Impl->vOffsetVector.size() * 2);

	/* we don't want to have to access notedata when loading or rescording
	scores so we use the vector length of offset replay data to determine point
	denominators however full replays store mine hits as offsets, meaning
	we have to screen them out when calculating the max points*/
	if (m_Impl->ReplayType == 2) {
		pmax += m_Impl->iTapNoteScores[TNS_HitMine] * -2.F;

		// we screened out extra offsets due to mines in the replay from the
		// denominator but we've still increased the numerator with 0.00f
		// offsets (2pts)
		p += m_Impl->iTapNoteScores[TNS_HitMine] * -2.F;
	}

	return p / pmax;
}

auto
HighScore::RescoreToWife3(float pmax) -> bool
{
	// HAHAHA WE NEED TO LOAD THE REPLAY DATA EVEN IF WE KNOW WE HAVE IT
	if (!LoadReplayData()) {
		return false;
	}

	// SSRNormPercent
	auto p4 = 0.F;
	// WifeScore for HighScore Judge
	auto pj = 0.F;

	// the typevector is only available for full replays
	if (m_Impl->ReplayType == 2) {
		for (size_t i = 0; i < m_Impl->vOffsetVector.size(); i++) {
			// by the powers of god invested in me i declare these vectors the
			// same size so this works all the time no matter what
			auto& type = m_Impl->vTapNoteTypeVector[i];
			if (type == TapNoteType_Tap || type == TapNoteType_HoldHead ||
				type == TapNoteType_Lift) {
				p4 += wife3(m_Impl->vOffsetVector[i], 1);
				pj += wife3(m_Impl->vOffsetVector[i], m_Impl->fJudgeScale);
			}
		}
	} else {
		// blindly assume the offset vector is correct for old replays
		for (auto& n : m_Impl->vOffsetVector) {
			p4 += wife3(n, 1);
			pj += wife3(n, m_Impl->fJudgeScale);
		}
	}

	const auto holdpoints = (m_Impl->iHoldNoteScores[HNS_LetGo] +
							 m_Impl->iHoldNoteScores[HNS_Missed]) *
							wife3_hold_drop_weight;
	const auto minepoints =
	  m_Impl->iTapNoteScores[TNS_HitMine] * wife3_mine_hit_weight;

	p4 += holdpoints + minepoints;
	pj += holdpoints + minepoints;

	m_Impl->fSSRNormPercent = p4 / pmax;
	m_Impl->fWifeScore = pj / pmax;
	m_Impl->fWifePoints = pj;
	m_Impl->WifeVersion = 3;
	return true;
}

// DONT REALLY KNOW WHY THIS IS STILL HERE
auto
HighScore::RescoreToDPJudge(int x) -> float
{
	const float tso[] = { 1.50F, 1.33F, 1.16F, 1.00F, 0.84F,
						  0.66F, 0.50F, 0.33F, 0.20F };
	const auto ts = tso[x - 1];
	std::vector<int> vRescoreJudgeVector;
	auto marv = 0;
	auto perf = 0;
	auto great = 0;
	auto good = 0;
	auto boo = 0;
	auto miss = 0;
	auto m2 = 0;
	for (auto& f : m_Impl->vOffsetVector) {
		m2 += 2;
		const auto x = abs(f * 1000.F);
		if (x <= ts * 22.5F) {
			++marv;
		} else if (x <= ts * 45.F) {
			++perf;
		} else if (x <= ts * 90.F) {
			++great;
		} else if (x <= ts * 135.F) {
			++good;
		} else if (x <= ts * 180.F) {
			++boo;
		} else {
			++miss;
		}
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

	auto p = 0;
	p += (marv + perf) * 2;
	p += great * 1;
	p += boo * -4;
	p += miss * -8;
	p += m_Impl->iHoldNoteScores[HNS_Held] * 6;
	p += m_Impl->iTapNoteScores[TNS_HitMine] * -8;
	p += (m_Impl->iHoldNoteScores[HNS_LetGo] +
		  m_Impl->iHoldNoteScores[HNS_Missed]) *
		 -6;

	auto m = static_cast<float>(m_Impl->vOffsetVector.size() * 2);
	m += (m_Impl->radarValues[RadarCategory_Holds] +
		  m_Impl->radarValues[RadarCategory_Rolls]) *
		 6;
	return p / m;
}

auto
HighScore::GetRescoreJudgeVector(int x) -> std::vector<int>
{
	RescoreToDPJudge(x);
	return m_Impl->vRescoreJudgeVector;
}

auto
HighScore::GetWifeGrade() const -> Grade
{
	return m_Impl->GetWifeGrade();
}

auto
HighScore::WriteReplayData() -> bool
{
	// return DBProfile::WriteReplayData(this);
	return m_Impl->WriteReplayData();
}

// TO BE REMOVED SOON
// Ok I guess we can be more lenient and convert by midwindow values, but we
// still have to assume j4 - mina
auto
HighScore::ConvertDpToWife() -> float
{
	if (m_Impl->fWifeScore > 0.F) {
		if (PREFSMAN->m_bSortBySSRNorm) {
			return m_Impl->fSSRNormPercent;
		}
		return m_Impl->fWifeScore;
	}

	if (m_Impl->grade == Grade_Failed) {
		return 0.F;
	}

	const auto ts = 1.F;
	auto estpoints = 0.F;
	auto maxpoints = 0.F;
	estpoints += m_Impl->iTapNoteScores[TNS_W1] * wife3(.01125F, ts);
	estpoints += m_Impl->iTapNoteScores[TNS_W2] * wife3(.03375F, ts);
	estpoints += m_Impl->iTapNoteScores[TNS_W3] * wife3(.0675F, ts);
	estpoints += m_Impl->iTapNoteScores[TNS_W4] * wife3(.1125F, ts);
	estpoints += m_Impl->iTapNoteScores[TNS_W5] * wife3(.1575F, ts);
	estpoints += m_Impl->iTapNoteScores[TNS_Miss] * wife3_miss_weight;

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
	static auto GetName(T* p, lua_State* L) -> int
	{
		lua_pushstring(L, p->GetName().c_str());
		return 1;
	}
	static auto GetScore(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetScore());
		return 1;
	}
	static auto GetPercentDP(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetPercentDP());
		return 1;
	}
	static auto GetWifeScore(T* p, lua_State* L) -> int
	{
		if (PREFSMAN->m_bSortBySSRNorm) {
			lua_pushnumber(L, p->GetSSRNormPercent());
		} else {
			lua_pushnumber(L, p->GetWifeScore());
		}
		return 1;
	}
	static auto GetWifePoints(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetWifePoints());
		return 1;
	}
	static auto GetMusicRate(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetMusicRate());
		return 1;
	}
	static auto GetJudgeScale(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetJudgeScale());
		return 1;
	}
	static auto GetDate(T* p, lua_State* L) -> int
	{
		lua_pushstring(L, p->GetDateTime().GetString().c_str());
		return 1;
	}
	static auto GetPlayedSeconds(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetPlayedSeconds());
		return 1;
	}
	static auto IsFillInMarker(T* p, lua_State* L) -> int
	{
		auto bIsFillInMarker = false;
		bIsFillInMarker |= p->GetName() == RANKING_TO_FILL_IN_MARKER;
		lua_pushboolean(L, static_cast<int>(bIsFillInMarker));
		return 1;
	}
	static auto GetMaxCombo(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetMaxCombo());
		return 1;
	}
	static auto GetModifiers(T* p, lua_State* L) -> int
	{
		lua_pushstring(L, p->GetModifiers().c_str());
		return 1;
	}
	static auto GetTapNoteScore(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetTapNoteScore(Enum::Check<TapNoteScore>(L, 1)));
		return 1;
	}
	static auto GetHoldNoteScore(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L,
					   p->GetHoldNoteScore(Enum::Check<HoldNoteScore>(L, 1)));
		return 1;
	}
	static auto RescoreJudges(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		for (auto i = 0; i < 6; ++i) {
			lua_pushnumber(L, p->GetRescoreJudgeVector(IArg(1))[i]);
			lua_rawseti(L, -2, i + 1);
		}

		return 1;
	}
	static auto GetRadarValues(T* p, lua_State* L) -> int
	{
		auto& rv = const_cast<RadarValues&>(p->GetRadarValues());
		rv.PushSelf(L);
		return 1;
	}
	static auto GetRadarPossible(T* p, lua_State* L) -> int
	{
		auto& rv = const_cast<RadarValues&>(
		  SONGMAN->GetStepsByChartkey(p->GetChartKey())->GetRadarValues());
		rv.PushSelf(L);
		return 1;
	}
	static auto GetSkillsetSSR(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetSkillsetSSR(Enum::Check<Skillset>(L, 1)));
		return 1;
	}
	static auto ToggleEtternaValidation(T* p, lua_State * /*L*/) -> int
	{
		p->SetEtternaValid(!p->GetEtternaValid());
		return 0;
	}

	// Convert to MS so lua doesn't have to
	// not exactly sure why i'm doing this fancy load garbage or if it works...
	// -mina
	static auto GetOffsetVector(T* p, lua_State* L) -> int
	{
		auto v = p->GetOffsetVector();
		const auto loaded = !v.empty();
		if (loaded || p->LoadReplayData()) {
			if (!loaded) {
				v = p->GetOffsetVector();
			}
			for (auto& i : v) {
				i = i * 1000;
			}
			LuaHelpers::CreateTableFromArray(v, L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	static auto GetNoteRowVector(T* p, lua_State* L) -> int
	{
		const auto* v = &(p->GetNoteRowVector());
		const auto loaded = !v->empty();

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
			auto nerv = nd.BuildAndGetNerv(td);
			auto sdifs = td->BuildAndGetEtaner(nerv);
			std::vector<int> noterows;
			for (auto t : timestamps) {
				auto timestamptobeat =
				  td->GetBeatFromElapsedTime(t * p->GetMusicRate());
				const auto somenumberscaledbyoffsets =
				  sdifs[0] - (timestamps[0] * p->GetMusicRate());
				timestamptobeat += somenumberscaledbyoffsets;
				auto noterowfrombeat = BeatToNoteRow(timestamptobeat);
				noterows.emplace_back(noterowfrombeat);
			}
			const auto noterowoffsetter = nerv[0] - noterows[0];
			for (auto& noterowwithoffset : noterows) {
				noterowwithoffset += noterowoffsetter;
			}
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

	static auto GetTrackVector(T* p, lua_State* L) -> int
	{
		const auto* v = &(p->GetTrackVector());
		const auto loaded = !v->empty();
		if (loaded || p->LoadReplayData()) {
			if (!loaded) {
				v = &(p->GetTrackVector());
			}
			LuaHelpers::CreateTableFromArray((*v), L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	static auto GetTapNoteTypeVector(T* p, lua_State* L) -> int
	{
		const auto* v = &(p->GetTapNoteTypeVector());
		const auto loaded = !v->empty();
		if (loaded || p->LoadReplayData()) {
			if (!loaded) {
				v = &(p->GetTapNoteTypeVector());
			}
			LuaHelpers::CreateTableFromArray((*v), L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}
	static auto GetJudgmentString(T* p, lua_State* L) -> int
	{
		const auto doot = ssprintf("%d I %d I %d I %d I %d I %d  x%d",
								   p->GetTapNoteScore(TNS_W1),
								   p->GetTapNoteScore(TNS_W2),
								   p->GetTapNoteScore(TNS_W3),
								   p->GetTapNoteScore(TNS_W4),
								   p->GetTapNoteScore(TNS_W5),
								   p->GetTapNoteScore(TNS_Miss),
								   p->GetMaxCombo());
		lua_pushstring(L, doot.c_str());
		return 1;
	}

	static auto GetUserid(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->userid);
		return 1;
	}
	static auto GetScoreid(T* p, lua_State* L) -> int
	{
		lua_pushstring(L, std::string(p->scoreid).c_str());
		return 1;
	}
	static auto GetAvatar(T* p, lua_State* L) -> int
	{
		lua_pushstring(L, std::string(p->avatar).c_str());
		return 1;
	}
	static auto GetWifeVers(T* p, lua_State* L) -> int
	{
		auto vers = p->GetWifeVersion();
		if (vers != 3) {
			vers = 2;
		}
		lua_pushnumber(L, vers);
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
		ADD_METHOD(RescoreJudges);
		ADD_METHOD(GetSkillsetSSR);
		ADD_METHOD(GetMusicRate);
		ADD_METHOD(GetJudgeScale);
		ADD_METHOD(GetChordCohesion);
		ADD_METHOD(GetDate);
		ADD_METHOD(GetPlayedSeconds);
		ADD_METHOD(IsFillInMarker);
		ADD_METHOD(GetModifiers);
		ADD_METHOD(GetTapNoteScore);
		ADD_METHOD(GetHoldNoteScore);
		ADD_METHOD(GetRadarValues);
		ADD_METHOD(GetRadarPossible);
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
		ADD_METHOD(GetWifeVers);
	}
};

LUA_REGISTER_CLASS(HighScore)
// lua end
