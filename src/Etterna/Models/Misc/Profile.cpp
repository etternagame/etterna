#include "Etterna/Globals/global.h"
#include "Profile.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/FileTypes/IniFile.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/MinaCalc/MinaCalc.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/NoteData/NoteDataWithScoring.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/Misc/XMLProfile.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Singletons/DownloadManager.h"

#include <algorithm>
#include <map>

/** @brief The filename for where one can edit their personal profile
 * information. */
const std::string EDITABLE_INI = "Editable.ini";
/** @brief A tiny file containing the type and list priority. */
const std::string TYPE_INI = "Type.ini";
/** @brief The filename containing the signature for STATS_XML's signature. */
const std::string PUBLIC_KEY_FILE = "public.key";
const std::string SCREENSHOTS_SUBDIR = "Screenshots/";
const std::string EDIT_STEPS_SUBDIR = "Edits/";
// const std::string UPLOAD_SUBDIR         = "Upload/";
const std::string RIVAL_SUBDIR = "Rivals/";

#define GUID_SIZE_BYTES 8

#if defined(_MSC_VER)
#pragma warning(disable : 4706) // assignment within conditional expression
#endif

void
Profile::InitEditableData()
{
	m_sDisplayName = "";
	m_sLastUsedHighScoreName = "";
}

void
Profile::ClearStats()
{
	// don't reset the Guid
	const auto sGuid = m_sGuid;
	InitAll();
	m_sGuid = sGuid;
}

std::string
Profile::MakeGuid()
{
	std::string s;
	s.reserve(GUID_SIZE_BYTES * 2);
	unsigned char buf[GUID_SIZE_BYTES];
	CryptManager::GetRandomBytes(buf, GUID_SIZE_BYTES);
	for (auto i : buf)
		s += ssprintf("%02x", i);
	return s;
}

void
Profile::InitGeneralData()
{
	m_sGuid = MakeGuid();

	m_SortOrder = SortOrder_Invalid;
	m_LastDifficulty = Difficulty_Invalid;
	m_LastStepsType = StepsType_Invalid;
	m_lastSong.Unset();
	m_iCurrentCombo = 0;
	m_iTotalSessions = 0;
	m_iTotalSessionSeconds = 0;
	m_iTotalGameplaySeconds = 0;
	m_iTotalDancePoints = 0;
	m_iNumExtraStagesPassed = 0;
	m_iNumExtraStagesFailed = 0;
	m_iNumToasties = 0;
	m_sLastPlayedMachineGuid = "";
	m_LastPlayedDate.Init();
	m_iTotalTapsAndHolds = 0;
	m_iTotalJumps = 0;
	m_iTotalHolds = 0;
	m_iTotalRolls = 0;
	m_iTotalMines = 0;
	m_iTotalHands = 0;
	m_iTotalLifts = 0;
	m_fPlayerRating = 0.f;

	m_iNumSongsPlayedByStyle.clear();
	FOREACH_ENUM(Difficulty, i)
	m_iNumSongsPlayedByDifficulty[i] = 0;
	for (auto& i : m_iNumSongsPlayedByMeter)
		i = 0;
	m_iNumTotalSongsPlayed = 0;
	ZERO(m_iNumStagesPassedByGrade);
	ZERO(m_fPlayerSkillsets);

	m_UserTable.Unset();
}

void
Profile::InitScreenshotData()
{
	m_vScreenshots.clear();
}

std::string
Profile::GetDisplayNameOrHighScoreName() const
{
	if (!m_sDisplayName.empty())
		return m_sDisplayName;
	if (!m_sLastUsedHighScoreName.empty())
		return m_sLastUsedHighScoreName;
	return std::string();
}

/*
 * Get the profile default modifiers.  Return true if set, in which case
 * sModifiersOut will be set.  Return false if no modifier string is set, in
 * which case the theme defaults should be used.  Note that the null string
 * means "no modifiers are active", which is distinct from no modifier string
 * being set at all.
 *
 * In practice, we get the default modifiers from the theme the first time a
 * game is played, and from the profile every time thereafter.
 */
bool
Profile::GetDefaultModifiers(const Game* pGameType,
							 std::string& sModifiersOut) const
{
	std::map<std::string, std::string>::const_iterator it;
	it = m_sDefaultModifiers.find(pGameType->m_szName);
	if (it == m_sDefaultModifiers.end())
		return false;
	sModifiersOut = it->second;
	return true;
}

void
Profile::SetDefaultModifiers(const Game* pGameType,
							 const std::string& sModifiers)
{
	if (sModifiers.empty())
		m_sDefaultModifiers.erase(pGameType->m_szName);
	else
		m_sDefaultModifiers[pGameType->m_szName] = sModifiers;
}

auto
Profile::GetBestGrade(const Song* song, const StepsType st) const -> Grade
{
	auto gradeBest = Grade_Invalid;
	if (song != nullptr) {
		for (const auto& s : song->GetAllSteps()) {
			if (s != nullptr && s->m_StepsType == st) {
				const auto dcg =
				  SCOREMAN->GetBestGradeFor(s->GetChartKey(), m_sProfileID);
				if (gradeBest >= dcg) {
					gradeBest = dcg;
				}
			}
		}
	}

	return gradeBest;
}

auto
Profile::GetBestWifeScore(const Song* song, const StepsType st) const -> float
{
	auto scorebest = 0.F;
	if (song != nullptr) {
		for (const auto& s : song->GetAllSteps()) {
			if (s != nullptr && s->m_StepsType == st) {
				const auto wsb =
				  SCOREMAN->GetBestWifeScoreFor(s->GetChartKey(), m_sProfileID);
				if (wsb >= scorebest) {
					scorebest = wsb;
				}
			}
		}
	}

	return scorebest;
}

void
Profile::swap(Profile& other)
{
// Type is skipped because this is meant to be used only on matching types,
// to move profiles after the priorities have been assigned. -Kyz
// A bit of a misnomer, since it actually works on any type that has its
// own swap function, which includes the standard containers.
#define SWAP_STR_MEMBER(member_name) member_name.swap(other.member_name)
#define SWAP_GENERAL(member_name) std::swap(member_name, other.member_name)
#define SWAP_ARRAY(member_name, size)                                          \
	for (int i = 0; i < (size); ++i) {                                         \
		std::swap((member_name)[i], other.member_name[i]);                     \
	}                                                                          \
	SWAP_GENERAL(m_ListPriority);
	SWAP_STR_MEMBER(m_sDisplayName);
	SWAP_STR_MEMBER(m_sLastUsedHighScoreName);
	SWAP_STR_MEMBER(m_sGuid);
	SWAP_GENERAL(m_iCurrentCombo);
	SWAP_GENERAL(m_iTotalSessions);
	SWAP_GENERAL(m_iTotalSessionSeconds);
	SWAP_GENERAL(m_iTotalGameplaySeconds);
	SWAP_GENERAL(m_iTotalDancePoints);
	SWAP_GENERAL(m_iNumExtraStagesPassed);
	SWAP_GENERAL(m_iNumExtraStagesFailed);
	SWAP_GENERAL(m_iNumToasties);
	SWAP_GENERAL(m_iTotalTapsAndHolds);
	SWAP_GENERAL(m_iTotalJumps);
	SWAP_GENERAL(m_iTotalHolds);
	SWAP_GENERAL(m_iTotalRolls);
	SWAP_GENERAL(m_iTotalMines);
	SWAP_GENERAL(m_iTotalHands);
	SWAP_GENERAL(m_iTotalLifts);
	SWAP_GENERAL(m_bNewProfile);
	SWAP_STR_MEMBER(m_sLastPlayedMachineGuid);
	SWAP_GENERAL(m_LastPlayedDate);
	SWAP_STR_MEMBER(m_iNumSongsPlayedByStyle);
	SWAP_ARRAY(m_iNumSongsPlayedByDifficulty, NUM_Difficulty);
	SWAP_ARRAY(m_iNumSongsPlayedByMeter, MAX_METER + 1);
	SWAP_GENERAL(m_iNumTotalSongsPlayed);
	SWAP_ARRAY(m_iNumStagesPassedByGrade, NUM_Grade);
	SWAP_GENERAL(m_UserTable);
	SWAP_STR_MEMBER(m_vScreenshots);
#undef SWAP_STR_MEMBER
#undef SWAP_GENERAL
#undef SWAP_ARRAY
}

void
Profile::LoadCustomFunction(const std::string& sDir)
{
	/* Get the theme's custom load function:
	 *   [Profile]
	 *   CustomLoadFunction=function(profile, profileDir) ... end
	 */
	auto* L = LUA->Get();
	const auto customLoadFunc =
	  THEME->GetMetricR("Profile", "CustomLoadFunction");
	customLoadFunc.PushSelf(L);
	ASSERT_M(!lua_isnil(L, -1), "CustomLoadFunction not defined");

	// Pass profile and profile directory as arguments
	this->PushSelf(L);
	LuaHelpers::Push(L, sDir);

	// Run it
	std::string Error = "Error running CustomLoadFunction: ";
	LuaHelpers::RunScriptOnStack(L, Error, 2, 0, true);

	LUA->Release(L);
}

void
Profile::HandleStatsPrefixChange(std::string dir)
{
	// Temp variables to preserve stuff across the reload.
	// Some stuff intentionally left out because the original reason for the
	// stats prefix was to allow scores from different game types to coexist.
	const auto display_name = m_sDisplayName;
	const auto last_high_score_name = m_sLastUsedHighScoreName;
	const auto priority = m_ListPriority;
	const auto guid = m_sGuid;
	const auto default_mods = m_sDefaultModifiers;
	const auto sort_order = m_SortOrder;
	const auto last_diff = m_LastDifficulty;
	const auto last_stepstype = m_LastStepsType;
	const auto last_song = m_lastSong;
	const auto total_sessions = m_iTotalSessions;
	const auto total_session_seconds = m_iTotalSessionSeconds;
	const auto total_gameplay_seconds = m_iTotalGameplaySeconds;
	const auto user_table = m_UserTable;
	auto need_to_create_file = false;
	if (IsAFile(dir + PROFILEMAN->GetStatsPrefix() + ETT_XML)) {
		LoadAllFromDir(dir, nullptr);
	} else {
		ClearStats();
		need_to_create_file = true;
	}
	m_sDisplayName = display_name;
	m_sLastUsedHighScoreName = last_high_score_name;
	m_ListPriority = priority;
	m_sGuid = guid;
	m_sDefaultModifiers = default_mods;
	m_SortOrder = sort_order;
	m_LastDifficulty = last_diff;
	m_LastStepsType = last_stepstype;
	m_lastSong = last_song;
	m_iTotalSessions = total_sessions;
	m_iTotalSessionSeconds = total_session_seconds;
	m_iTotalGameplaySeconds = total_gameplay_seconds;
	m_UserTable = user_table;
	if (need_to_create_file) {
		SaveAllToDir(dir);
	}
}

ProfileLoadResult
Profile::LoadAllFromDir(const std::string& sDir, LoadingWindow* ld)
{
	Locator::getLogger()->trace("Profile::LoadAllFromDir({})", sDir.c_str());
	ASSERT(sDir.back() == '/');

	InitAll();

	LoadTypeFromDir(sDir);
	DBProf.SetLoadingProfile(this);
	XMLProf.SetLoadingProfile(this);
	const auto ret = XMLProf.LoadEttFromDir(sDir);
	if (ret != ProfileLoadResult_Success)
		return ret;

	// Not critical if this fails
	LoadEditableDataFromDir(sDir);

	CalculateStatsFromScores(
	  ld); // note to self: figure out how to tell if this is necessary
	return ProfileLoadResult_Success;
}

void
Profile::LoadTypeFromDir(const std::string& dir)
{
	m_ListPriority = 0;
	const auto fn = dir + TYPE_INI;
	if (FILEMAN->DoesFileExist(fn)) {
		IniFile ini;
		if (ini.ReadFile(fn)) {
			XNode const* data = ini.GetChild("ListPosition");
			if (data != nullptr) {
				data->GetAttrValue("Priority", m_ListPriority);
			}
		}
	}
}

void
Profile::CalculateStatsFromScores(LoadingWindow* ld)
{
	Locator::getLogger()->trace("Calculating stats from scores");
	const auto& all = SCOREMAN->GetAllProfileScores(m_sProfileID);
	auto TotalGameplaySeconds = 0.f;
	m_iTotalTapsAndHolds = 0;
	m_iTotalHolds = 0;
	m_iTotalMines = 0;

	for (auto* hs : all) {
		TotalGameplaySeconds += hs->GetPlayedSeconds();
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W1);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W2);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W3);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W4);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W5);
		m_iTotalMines += hs->GetTapNoteScore(TNS_HitMine);
		m_iTotalTapsAndHolds += hs->GetHoldNoteScore(HNS_Held);
	}

	m_iNumTotalSongsPlayed = all.size();
	m_iTotalDancePoints = m_iTotalTapsAndHolds * 2;
	m_iTotalGameplaySeconds = static_cast<int>(TotalGameplaySeconds);

	SCOREMAN->RecalculateSSRs(ld);
	SCOREMAN->CalcPlayerRating(
	  m_fPlayerRating, m_fPlayerSkillsets, m_sProfileID);
}

void
Profile::CalculateStatsFromScores()
{
	CalculateStatsFromScores(nullptr);
}

bool
Profile::SaveAllToDir(const std::string& sDir) const
{
	m_LastPlayedDate = DateTime::GetNowDate();

	SaveTypeToDir(sDir);
	// Save editable.ini
	SaveEditableDataToDir(sDir);

	GAMESTATE->SaveCurrentSettingsToProfile(PLAYER_1);

	const auto bSaved = XMLProf.SaveEttXmlToDir(sDir, this);
	SaveStatsWebPageToDir(sDir);

	// Empty directories if none exist.
	FILEMAN->CreateDir(sDir + SCREENSHOTS_SUBDIR);

	return bSaved;
}

void
Profile::SaveTypeToDir(const std::string& dir) const
{
	IniFile ini;
	ini.SetValue("ListPosition", "Priority", m_ListPriority);
	ini.WriteFile(dir + TYPE_INI);
}

void
Profile::SaveEditableDataToDir(const std::string& sDir) const
{
	IniFile ini;

	ini.SetValue("Editable", "DisplayName", m_sDisplayName);
	ini.SetValue("Editable", "LastUsedHighScoreName", m_sLastUsedHighScoreName);

	ini.WriteFile(sDir + EDITABLE_INI);
}

ProfileLoadResult
Profile::LoadEditableDataFromDir(const std::string& sDir)
{
	const auto fn = sDir + EDITABLE_INI;

	if (!IsAFile(fn))
		return ProfileLoadResult_FailedNoProfile;

	IniFile ini;
	ini.ReadFile(fn);

	ini.GetValue("Editable", "DisplayName", m_sDisplayName);
	ini.GetValue("Editable", "LastUsedHighScoreName", m_sLastUsedHighScoreName);

	// This is data that the user can change, so we have to validate it.
	auto wstr = StringToWString(m_sDisplayName);
	if (wstr.size() > PROFILE_MAX_DISPLAY_NAME_LENGTH)
		wstr = wstr.substr(0, PROFILE_MAX_DISPLAY_NAME_LENGTH);
	m_sDisplayName = WStringToString(wstr);
	// TODO: strip invalid chars?

	return ProfileLoadResult_Success;
}

void
Profile::AddStepTotals(int iTotalTapsAndHolds,
					   int iTotalJumps,
					   int iTotalHolds,
					   int iTotalRolls,
					   int iTotalMines,
					   int iTotalHands,
					   int iTotalLifts)
{
	m_iTotalTapsAndHolds += iTotalTapsAndHolds;
	m_iTotalDancePoints = m_iTotalTapsAndHolds * 2;
	m_iTotalJumps += iTotalJumps;
	m_iTotalHolds += iTotalHolds;
	m_iTotalRolls += iTotalRolls;
	m_iTotalMines += iTotalMines;
	m_iTotalHands += iTotalHands;
	m_iTotalLifts += iTotalLifts;
}

void
Profile::RemoveFromFavorites(const string& ck)
{
	FavoritedCharts.erase(ck);
}

void
Profile::RemoveFromPermaMirror(const string& ck)
{
	PermaMirrorCharts.erase(ck);
}

// more future goalman stuff (perhaps this should be standardized to "add" in
// order to match scoreman nomenclature) -mina
bool
Profile::AddGoal(const string& ck)
{
	ScoreGoal goal;
	goal.timeassigned = DateTime::GetNowDateTime();
	goal.rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	goal.chartkey = ck;
	// duplication avoidance should be simpler than this? -mina
	if (goalmap.count(ck))
		for (auto& n : goalmap[ck].goals)
			if (n.rate == goal.rate && n.percent == goal.percent)
				return false;

	goal.CheckVacuity();
	goalmap[ck].Add(goal);
	DLMAN->AddGoal(ck, goal.percent, goal.rate, goal.timeassigned);
	FillGoalTable();
	MESSAGEMAN->Broadcast("GoalTableRefresh");
	return true;
}

void
Profile::FillGoalTable()
{
	goaltable.clear();
	for (auto& sgv : goalmap)
		for (auto& sg : sgv.second.goals)
			if (SONGMAN->GetStepsByChartkey(sg.chartkey))
				goaltable.emplace_back(&sg);

	auto comp = [](ScoreGoal* a, ScoreGoal* b) {
		return a->timeassigned > b->timeassigned;
	};
	sort(goaltable.begin(), goaltable.end(), comp);
}

XNode*
ScoreGoal::CreateNode() const
{
	auto* pNode = new XNode("ScoreGoal");

	pNode->AppendChild("Rate", rate);
	pNode->AppendChild("Percent", percent);
	pNode->AppendChild("Priority", priority);
	pNode->AppendChild("Achieved", achieved);
	pNode->AppendChild("TimeAssigned", timeassigned.GetString());
	if (achieved) {
		pNode->AppendChild("TimeAchieved", timeachieved.GetString());
		pNode->AppendChild("ScoreKey", scorekey);
	}

	pNode->AppendChild("Comment", comment);

	return pNode;
}

void
ScoreGoal::LoadFromNode(const XNode* pNode)
{
	ASSERT(pNode->GetName() == "ScoreGoal");

	std::string s;

	pNode->GetChildValue("Rate", rate);
	pNode->GetChildValue("Percent", percent);
	if (percent > 1.f) // goddamnit why didnt i think this through originally
		percent /= 100.f;
	pNode->GetChildValue("Priority", priority);
	pNode->GetChildValue("Achieved", achieved);
	pNode->GetChildValue("TimeAssigned", s);
	timeassigned.FromString(s);
	if (achieved) {
		pNode->GetChildValue("TimeAchieved", s);
		timeachieved.FromString(s);
		pNode->GetChildValue("ScoreKey", scorekey);
	}

	pNode->GetChildValue("Comment", comment);
}

HighScore*
ScoreGoal::GetPBUpTo() const
{
	return SCOREMAN->GetChartPBUpTo(chartkey, rate);
}

void
ScoreGoal::CheckVacuity()
{
	auto* const pb = SCOREMAN->GetChartPBAt(chartkey, rate);

	if (pb && pb->GetWifeScore() >= percent)
		vacuous = true;
	else
		vacuous = false;
}

void
ScoreGoal::UploadIfNotVacuous()
{
	if (!vacuous || !timeachieved.GetString().empty())
		DLMAN->UpdateGoal(
		  chartkey, percent, rate, achieved, timeassigned, timeachieved);
}

// aaa too lazy to write comparators rn -mina
ScoreGoal&
Profile::GetLowestGoalForRate(const string& ck, float rate)
{
	auto& sgv = goalmap[ck].Get();
	auto lowest = 100.f;
	auto lowestidx = 0;
	for (size_t i = 0; i < sgv.size(); ++i) {
		auto& tmp = sgv[i];
		if (tmp.rate == rate) {
			if (tmp.percent > lowest) {
				lowest = tmp.percent;
				lowestidx = i;
			}
		}
	}
	return sgv[lowestidx];
}

void
Profile::SetAnyAchievedGoals(const string& ck,
							 float& rate,
							 const HighScore& pscore)
{
	Locator::getLogger()->trace("Scanning for any goals that may have been accomplished.");

	if (!HasGoal(ck))
		return;

	auto& sgv = goalmap[ck].Get();
	for (auto& tmp : sgv) {
		if (lround(tmp.rate * 10000.f) == lround(rate * 10000.f) &&
			!tmp.achieved && tmp.percent < pscore.GetWifeScore()) {
			tmp.achieved = true;
			tmp.timeachieved = pscore.GetDateTime();
			tmp.scorekey = pscore.GetScoreKey();
			DLMAN->UpdateGoal(tmp.chartkey,
							  tmp.percent,
							  tmp.rate,
							  tmp.achieved,
							  tmp.timeassigned,
							  tmp.timeachieved);
		}
	}
}

void
Profile::RemoveGoal(const string& ck, DateTime assigned)
{
	auto& sgv = goalmap.at(ck).Get();
	for (size_t i = 0; i < sgv.size(); ++i) {
		if (sgv[i].timeassigned == assigned) {
			DLMAN->RemoveGoal(ck, sgv[i].percent, sgv[i].rate);
			sgv.erase(sgv.begin() + i);
		}
	}
}

void
Profile::SaveStatsWebPageToDir(const std::string& sDir) const
{
	ASSERT(PROFILEMAN != nullptr);
}

void
Profile::AddScreenshot(const Screenshot& screenshot)
{
	m_vScreenshots.push_back(screenshot);
}

void
Profile::MoveBackupToDir(const std::string& sFromDir, const std::string& sToDir)
{
	XMLProfile::MoveBackupToDir(sFromDir, sToDir);

	if (FILEMAN->IsAFile(sFromDir + EDITABLE_INI))
		FILEMAN->Move(sFromDir + EDITABLE_INI, sToDir + EDITABLE_INI);
	if (FILEMAN->IsAFile(sFromDir + DONT_SHARE_SIG))
		FILEMAN->Move(sFromDir + DONT_SHARE_SIG, sToDir + DONT_SHARE_SIG);
}

std::string
Profile::MakeUniqueFileNameNoExtension(const std::string& sDir,
									   const std::string& sFileNameBeginning)
{
	FILEMAN->FlushDirCache(sDir);
	// Find a file name for the screenshot
	std::vector<std::string> files;
	FILEMAN->GetDirListing(
	  sDir + sFileNameBeginning + "*", files, false, false);
	sort(files.begin(), files.end());

	auto iIndex = 0;

	for (int i = files.size() - 1; i >= 0; --i) {
		static Regex re("^" + sFileNameBeginning + "([0-9]{5})\\....$");
		std::vector<std::string> matches;
		if (!re.Compare(files[i], matches))
			continue;

		ASSERT(matches.size() == 1);
		iIndex = StringToInt(matches[0]) + 1;
		break;
	}

	return MakeFileNameNoExtension(sFileNameBeginning, iIndex);
}

std::string
Profile::MakeFileNameNoExtension(const std::string& sFileNameBeginning,
								 int iIndex)
{
	return sFileNameBeginning + ssprintf("%05d", iIndex);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the Profile. */
class LunaProfile : public Luna<Profile>
{
  public:
	static int AddScreenshot(T* p, lua_State* L)
	{
		auto* hs = Luna<HighScore>::check(L, 1);
		const std::string filename = SArg(2);
		Screenshot screenshot;
		screenshot.sFileName = filename;
		screenshot.sMD5 = BinaryToHex(CRYPTMAN->GetMD5ForFile(filename));
		screenshot.highScore = *hs;
		p->AddScreenshot(screenshot);
		COMMON_RETURN_SELF;
	}
	DEFINE_METHOD(GetPriority, m_ListPriority);

	static int GetDisplayName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sDisplayName.c_str());
		return 1;
	}
	static int SetDisplayName(T* p, lua_State* L)
	{
		p->m_sDisplayName = SArg(1);
		COMMON_RETURN_SELF;
	}
	static int GetLastUsedHighScoreName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sLastUsedHighScoreName.c_str());
		return 1;
	}
	static int SetLastUsedHighScoreName(T* p, lua_State* L)
	{
		p->m_sLastUsedHighScoreName = SArg(1);
		COMMON_RETURN_SELF;
	}

	static int GetTotalNumSongsPlayed(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iNumTotalSongsPlayed);
		return 1;
	}
	// TODO: SCOREMAN
	static int GetSongsActual(T* p, lua_State* L)
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	// TODO: SCOREMAN
	static int GetSongsPossible(T* p, lua_State* L)
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	// TODO: SCOREMAN
	static int GetSongsPercentComplete(T* p, lua_State* L)
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	// TODO: SCOREMAN
	static int GetTotalStepsWithTopGrade(T* p, lua_State* L)
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	static int GetNumTotalSongsPlayed(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iNumTotalSongsPlayed);
		return 1;
	}
	static int GetTotalSessions(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalSessions);
		return 1;
	}
	static int GetTotalSessionSeconds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalSessionSeconds);
		return 1;
	}
	static int GetTotalGameplaySeconds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalGameplaySeconds);
		return 1;
	}
	static int GetPlayerRating(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_fPlayerRating);
		return 1;
	}
	// TODO: SCOREMAN
	static int GetMostPopularSong(T* p, lua_State* L)
	{
		lua_pushnil(L);
		return 1;
	}
	// USE SCOREMAN FOR THIS
	// TODO: Remove?
	static int GetSongNumTimesPlayed(T* p, lua_State* L)
	{
		ASSERT(!lua_isnil(L, 1));
		lua_pushnumber(L, 0);
		return 1;
	}
	// USE SCOREMAN FOR THIS
	// TODO: Remove?
	static int HasPassedAnyStepsInSong(T* p, lua_State* L)
	{
		ASSERT(!lua_isnil(L, 1));
		lua_pushboolean(L, false);
		return 1;
	}
	static int GetNumToasties(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iNumToasties);
		return 1;
	}
	static int GetTotalTapsAndHolds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalTapsAndHolds);
		return 1;
	}
	static int GetTotalJumps(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalJumps);
		return 1;
	}
	static int GetTotalHolds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalHolds);
		return 1;
	}
	static int GetTotalRolls(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalRolls);
		return 1;
	}
	static int GetTotalMines(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalMines);
		return 1;
	}
	static int GetTotalHands(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalHands);
		return 1;
	}
	static int GetTotalLifts(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalLifts);
		return 1;
	}
	DEFINE_METHOD(GetTotalDancePoints, m_iTotalDancePoints);
	static int GetUserTable(T* p, lua_State* L)
	{
		p->m_UserTable.PushSelf(L);
		return 1;
	}
	static int GetNumFaves(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->FavoritedCharts.size());
		return 1;
	}
	static int GetLastPlayedSong(T* p, lua_State* L)
	{
		auto* pS = p->m_lastSong.ToSong();
		if (pS != nullptr)
			pS->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetPlayerSkillsetRating(T* p, lua_State* L)
	{
		const auto ss = Enum::Check<Skillset>(L, 1);
		lua_pushnumber(L, p->m_fPlayerSkillsets[ss]);
		return 1;
	}

	DEFINE_METHOD(GetGUID, m_sGuid);

	static int GetGoalTable(T* p, lua_State* L)
	{
		LuaHelpers::CreateTableFromArray(p->goaltable, L);
		return 1;
	}
	static int AddGoal(T* p, lua_State* L)
	{
		auto ck = SArg(1);
		auto success = p->AddGoal(ck);
		lua_pushboolean(L, success);
		return 1;
	}
	static int SetFromAll(T* p, lua_State* L)
	{
		p->FillGoalTable();
		p->filtermode = 1;
		p->asc = true;
		return 0;
	}

	static int SortByDate(T* p, lua_State* L)
	{
		if (p->sortmode == 1)
			if (p->asc) {
				auto comp = [](ScoreGoal* a, ScoreGoal* b) {
					return a->timeassigned < b->timeassigned;
				}; // custom operators?
				sort(p->goaltable.begin(), p->goaltable.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](ScoreGoal* a, ScoreGoal* b) {
			return a->timeassigned > b->timeassigned;
		};
		sort(p->goaltable.begin(), p->goaltable.end(), comp);
		p->sortmode = 1;
		p->asc = true;
		return 0;
	}

	static int SortByRate(T* p, lua_State* L)
	{
		if (p->sortmode == 2)
			if (p->asc) {
				auto comp = [](ScoreGoal* a, ScoreGoal* b) {
					return a->rate < b->rate;
				}; // custom operators?
				sort(p->goaltable.begin(), p->goaltable.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](ScoreGoal* a, ScoreGoal* b) {
			return a->rate > b->rate;
		};
		sort(p->goaltable.begin(), p->goaltable.end(), comp);
		p->sortmode = 2;
		p->asc = true;
		return 0;
	}

	static int SortByName(T* p, lua_State* L)
	{
		if (p->sortmode == 3)
			if (p->asc) {
				auto comp = [](ScoreGoal* a, ScoreGoal* b) {
					return make_lower(SONGMAN->GetSongByChartkey(a->chartkey)
										->GetDisplayMainTitle()) >
						   make_lower(SONGMAN->GetSongByChartkey(b->chartkey)
										->GetDisplayMainTitle());
				}; // custom operators?
				sort(p->goaltable.begin(), p->goaltable.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](ScoreGoal* a, ScoreGoal* b) {
			return make_lower(SONGMAN->GetSongByChartkey(a->chartkey)
								->GetDisplayMainTitle()) <
				   make_lower(SONGMAN->GetSongByChartkey(b->chartkey)
								->GetDisplayMainTitle());
		};
		sort(p->goaltable.begin(), p->goaltable.end(), comp);
		p->sortmode = 3;
		p->asc = true;
		return 0;
	}

	static int SortByPriority(T* p, lua_State* L)
	{
		if (p->sortmode == 4)
			if (p->asc) {
				auto comp = [](ScoreGoal* a, ScoreGoal* b) {
					return a->priority > b->priority;
				}; // custom operators?
				sort(p->goaltable.begin(), p->goaltable.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](ScoreGoal* a, ScoreGoal* b) {
			return a->priority < b->priority;
		};
		sort(p->goaltable.begin(), p->goaltable.end(), comp);
		p->sortmode = 4;
		p->asc = true;
		return 0;
	}

	static int SortByDiff(T* p, lua_State* L)
	{
		if (p->sortmode == 5)
			if (p->asc) {
				auto comp = [](ScoreGoal* a, ScoreGoal* b) {
					return SONGMAN->GetStepsByChartkey(a->chartkey)
							 ->GetMSD(a->rate, 0) <
						   SONGMAN->GetStepsByChartkey(b->chartkey)
							 ->GetMSD(b->rate, 0);
				};
				sort(p->goaltable.begin(), p->goaltable.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](ScoreGoal* a, ScoreGoal* b) {
			return SONGMAN->GetStepsByChartkey(a->chartkey)
					 ->GetMSD(a->rate, 0) >
				   SONGMAN->GetStepsByChartkey(b->chartkey)->GetMSD(b->rate, 0);
		};
		sort(p->goaltable.begin(), p->goaltable.end(), comp);
		p->sortmode = 5;
		p->asc = true;
		return 0;
	}

	static int ToggleFilter(T* p, lua_State* L)
	{
		p->FillGoalTable();

		if (p->filtermode == 3) {
			p->filtermode = 1;
			return 0;
		}

		std::vector<ScoreGoal*> doot;
		if (p->filtermode == 1) {
			for (auto& sg : p->goaltable)
				if (sg->achieved)
					doot.emplace_back(sg);
			p->goaltable = doot;
			p->filtermode = 2;
			return 0;
		}
		if (p->filtermode == 2) {
			for (auto& sg : p->goaltable)
				if (!sg->achieved)
					doot.emplace_back(sg);
			p->goaltable = doot;
			p->filtermode = 3;
			return 0;
		}
		return 0;
	}
	static int GetFilterMode(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->filtermode);
		return 1;
	}
	static int GetIgnoreStepCountCalories(T* p, lua_State* L)
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	static int CalculateCaloriesFromHeartRate(T* p, lua_State* L)
	{
		lua_pushnumber(L, 0);
		return 1;
	}

	static int IsCurrentChartPermamirror(T* p, lua_State* L)
	{
		auto o = false;

		if (GAMESTATE->m_pCurSteps) {
			const auto& ck = GAMESTATE->m_pCurSteps->GetChartKey();

			if (p->PermaMirrorCharts.count(ck))
				o = true;
		}

		lua_pushboolean(L, static_cast<int>(o));
		return 1;
	}

	// ok i should probably handle this better -mina
	static int GetEasiestGoalForChartAndRate(T* p, lua_State* L)
	{
		const string ck = SArg(1);
		if (!p->goalmap.count(ck)) {
			lua_pushnil(L);
			return 1;
		}

		auto& sgv = p->goalmap[ck].goals;
		auto herp = false;
		auto ez = 0;
		for (size_t i = 0; i < sgv.size(); ++i)
			if (lround(sgv[i].rate * 10000.f) == lround(FArg(2) * 10000.f) &&
				!sgv[i].achieved && sgv[i].percent <= sgv[ez].percent) {
				ez = i;
				herp = true;
			}
		if (herp)
			sgv[ez].PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	static int RenameProfile(T* p, lua_State* L)
	{
		p->m_sDisplayName = SArg(1);
		// roundabout way to force id to be a dir
		// sometimes its a dir and sometimes it a number
		const auto dir =
		  "/Save/LocalProfiles/" + Basename(p->m_sProfileID) + "/";
		p->SaveEditableDataToDir(dir);
		return 1;
	}

	static int ForceRecalcScores(T* p, lua_State* L)
	{
		SCOREMAN->RecalculateSSRs(p->m_sProfileID);
		return 0;
	}

	static int UnInvalidateAllScores(T* p, lua_State* L)
	{
		SCOREMAN->UnInvalidateAllScores(p->m_sProfileID);
		return 0;
	}

	LunaProfile()
	{
		ADD_METHOD(AddScreenshot);
		ADD_METHOD(GetPriority);
		ADD_METHOD(GetDisplayName);
		ADD_METHOD(SetDisplayName);
		ADD_METHOD(GetLastUsedHighScoreName);
		ADD_METHOD(SetLastUsedHighScoreName);
		ADD_METHOD(GetTotalNumSongsPlayed);
		ADD_METHOD(GetSongsActual);
		ADD_METHOD(GetSongsPossible);
		ADD_METHOD(GetSongsPercentComplete);
		ADD_METHOD(GetTotalStepsWithTopGrade);
		ADD_METHOD(GetNumTotalSongsPlayed);
		ADD_METHOD(GetTotalSessions);
		ADD_METHOD(GetTotalSessionSeconds);
		ADD_METHOD(GetTotalGameplaySeconds);
		ADD_METHOD(GetMostPopularSong);
		ADD_METHOD(GetSongNumTimesPlayed);
		ADD_METHOD(HasPassedAnyStepsInSong);
		ADD_METHOD(GetNumToasties);
		ADD_METHOD(GetTotalTapsAndHolds);
		ADD_METHOD(GetTotalJumps);
		ADD_METHOD(GetTotalHolds);
		ADD_METHOD(GetTotalRolls);
		ADD_METHOD(GetTotalMines);
		ADD_METHOD(GetTotalHands);
		ADD_METHOD(GetTotalLifts);
		ADD_METHOD(GetTotalDancePoints);
		ADD_METHOD(GetUserTable);
		ADD_METHOD(GetLastPlayedSong);
		ADD_METHOD(GetGUID);
		ADD_METHOD(GetPlayerRating);
		ADD_METHOD(GetPlayerSkillsetRating);
		ADD_METHOD(GetNumFaves);
		ADD_METHOD(GetGoalTable);
		ADD_METHOD(GetIgnoreStepCountCalories);
		ADD_METHOD(CalculateCaloriesFromHeartRate);
		ADD_METHOD(IsCurrentChartPermamirror);
		ADD_METHOD(GetEasiestGoalForChartAndRate);
		ADD_METHOD(RenameProfile);
		ADD_METHOD(AddGoal);
		ADD_METHOD(SetFromAll);
		ADD_METHOD(SortByDate);
		ADD_METHOD(SortByRate);
		ADD_METHOD(SortByPriority);
		ADD_METHOD(SortByName);
		ADD_METHOD(SortByDiff);
		ADD_METHOD(ToggleFilter);
		ADD_METHOD(GetFilterMode);
		ADD_METHOD(ForceRecalcScores);
		ADD_METHOD(UnInvalidateAllScores);
	}
};

LUA_REGISTER_CLASS(Profile)
class LunaScoreGoal : public Luna<ScoreGoal>
{
  public:
	DEFINE_METHOD(GetRate, rate);
	DEFINE_METHOD(GetPercent, percent);
	DEFINE_METHOD(GetPriority, priority);
	DEFINE_METHOD(IsAchieved, achieved);
	DEFINE_METHOD(GetComment, comment);
	DEFINE_METHOD(GetChartKey, chartkey);
	DEFINE_METHOD(WhenAssigned, timeassigned.GetString());

	static int WhenAchieved(T* p, lua_State* L)
	{
		if (p->achieved)
			lua_pushstring(L, p->timeachieved.GetString().c_str());
		else
			lua_pushnil(L);

		return 1;
	}

	static int SetRate(T* p, lua_State* L)
	{
		if (!p->achieved) {
			auto newrate = FArg(1);
			CLAMP(newrate, 0.7f, 3.0f);
			p->rate = newrate;
			p->CheckVacuity();
			p->UploadIfNotVacuous();
		}
		return 1;
	}

	static int SetPercent(T* p, lua_State* L)
	{
		if (!p->achieved) {
			auto newpercent = FArg(1);
			CLAMP(newpercent, .8f, 1.f);
			if (newpercent > 0.99f)
			{
				if (p->percent < 0.99700f)
					newpercent = 0.99700f; // AAA
				else if (p->percent < 0.99955f)
					newpercent = 0.99955f; // AAAA
				else if (p->percent < 0.999935f)
					newpercent = 0.999935f; // AAAAA
			}
			else if (newpercent > 0.985f)
			{
				if (p->percent > 0.999935f)
					newpercent = 0.999935f; // AAAAA
				else if (p->percent > 0.99955f)
					newpercent = 0.99955f; // AAAA
				else if (p->percent > 0.99700f)
					newpercent = 0.99700f; // AAA
				else
					newpercent = 0.99f;
			}


			p->percent = newpercent;
			p->CheckVacuity();
			p->UploadIfNotVacuous();
		}
		return 1;
	}

	static int SetPriority(T* p, lua_State* L)
	{
		if (!p->achieved) {
			auto newpriority = IArg(1);
			CLAMP(newpriority, 1, 100);
			p->priority = newpriority;
			p->UploadIfNotVacuous();
		}
		return 1;
	}

	static int SetComment(T* p, lua_State* L)
	{
		p->comment = SArg(1);
		return 1;
	}

	static int Delete(T* p, lua_State* L)
	{
		PROFILEMAN->GetProfile(PLAYER_1)->RemoveGoal(p->chartkey,
													 p->timeassigned);
		return 0;
	}

	static int GetPBUpTo(T* p, lua_State* L)
	{
		auto* pb = p->GetPBUpTo();
		if (pb == nullptr)
			lua_pushnil(L);
		else
			pb->PushSelf(L);
		return 1;
	}

	static int IsVacuous(T* p, lua_State* L)
	{
		if (p->achieved)
			lua_pushboolean(L, 0);

		p->CheckVacuity(); // might be redundant
		lua_pushboolean(L, p->vacuous);
		return 1;
	}

	static int AchievedBy(T* p, lua_State* L)
	{
		if (p->achieved)
			lua_pushstring(L, p->scorekey.c_str());
		else
			lua_pushnil(L);
		return 1;
	}

	LunaScoreGoal()
	{
		ADD_METHOD(GetRate);
		ADD_METHOD(GetPercent);
		ADD_METHOD(GetPriority);
		ADD_METHOD(IsAchieved);
		ADD_METHOD(GetComment);
		ADD_METHOD(WhenAssigned);
		ADD_METHOD(WhenAchieved);
		ADD_METHOD(GetChartKey);
		ADD_METHOD(SetRate);
		ADD_METHOD(SetPercent);
		ADD_METHOD(SetPriority);
		ADD_METHOD(SetComment);
		ADD_METHOD(Delete);
		ADD_METHOD(GetPBUpTo);
		ADD_METHOD(IsVacuous);
		ADD_METHOD(AchievedBy);
	}
};
LUA_REGISTER_CLASS(ScoreGoal)
// lua end
