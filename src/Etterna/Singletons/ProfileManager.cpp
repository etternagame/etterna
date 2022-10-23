#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "GameState.h"
#include "PrefsManager.h"
#include "Etterna/Models/Misc/Profile.h"
#include "ProfileManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "SongManager.h"
#include "DownloadManager.h"

#include <algorithm>

ProfileManager* PROFILEMAN =
  nullptr; // global and accessible from anywhere in our program

#define ID_DIGITS 8
#define ID_DIGITS_STR "8"
#define MAX_ID 99999999

static void
DefaultLocalProfileIDInit(size_t /*PlayerNumber*/ i,
						  std::string& sNameOut,
						  std::string& defaultValueOut)
{
	sNameOut = ssprintf("DefaultLocalProfileIDP%d", static_cast<int>(i + 1));
	if (i == 0)
		defaultValueOut = "00000000";
	else
		defaultValueOut = "";
}

Preference1D<std::string> ProfileManager::m_sDefaultLocalProfileID(
  DefaultLocalProfileIDInit,
  NUM_PLAYERS);

const std::string USER_PROFILES_DIR = "/Save/LocalProfiles/";
const std::string LAST_GOOD_SUBDIR = "LastGood/";

static std::string
LocalProfileIDToDir(const std::string& sProfileID)
{
	return USER_PROFILES_DIR + sProfileID + "/";
}
static std::string
LocalProfileDirToID(const std::string& sDir)
{
	return Basename(sDir);
}

struct DirAndProfile
{
	std::string sDir;
	Profile profile;
	void swap(DirAndProfile& other)
	{
		sDir.swap(other.sDir);
		profile.swap(other.profile);
	}
};
static std::vector<DirAndProfile> g_vLocalProfile;

static ThemeMetric<bool> FIXED_PROFILES("ProfileManager", "FixedProfiles");
static ThemeMetric<int> NUM_FIXED_PROFILES("ProfileManager",
										   "NumFixedProfiles");

ProfileManager::ProfileManager()
  : m_stats_prefix("")
{
	dummy = nullptr;
	m_bLastLoadWasFromLastGood = false;
	m_bLastLoadWasTamperedOrCorrupt = false;
	m_bNeedToBackUpLastLoad = false;
	m_bNewProfile = false;

	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "PROFILEMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

ProfileManager::~ProfileManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("PROFILEMAN");
}

void
ProfileManager::Init(LoadingWindow* ld)
{
	m_bLastLoadWasTamperedOrCorrupt = false;
	m_bLastLoadWasFromLastGood = false;
	m_bNeedToBackUpLastLoad = false;
	m_bNewProfile = false;

	RefreshLocalProfilesFromDisk(ld);

	if (!g_vLocalProfile.empty())
		m_sProfileDir = g_vLocalProfile[0].sDir;

	dummy = new Profile;
}

bool
ProfileManager::FixedProfiles() const
{
	return FIXED_PROFILES;
}

ProfileLoadResult
ProfileManager::LoadProfile(PlayerNumber pn, const std::string& sProfileDir)
{
	Locator::getLogger()->debug("LoadingProfile P{}, {}", pn + 1, sProfileDir.c_str());

	ASSERT(!sProfileDir.empty());
	ASSERT(sProfileDir.back() == '/');

	m_sProfileDir = sProfileDir;
	m_bLastLoadWasFromLastGood = false;
	m_bNeedToBackUpLastLoad = false;

	// Try to load the original, non-backup data.
	ProfileLoadResult lr =
	  GetProfile(pn)->LoadAllFromDir(m_sProfileDir, nullptr);

	std::string sBackupDir = m_sProfileDir + LAST_GOOD_SUBDIR;

	if (lr == ProfileLoadResult_Success) {
		/* Next time the profile is written, move this good profile into
		 * LastGood. */
		m_bNeedToBackUpLastLoad = true;
	}

	m_bLastLoadWasTamperedOrCorrupt = lr == ProfileLoadResult_FailedTampered;

	//
	// Try to load from the backup if the original data fails to load
	//
	if (lr == ProfileLoadResult_FailedTampered) {
		lr = GetProfile(pn)->LoadAllFromDir(sBackupDir, nullptr);
		m_bLastLoadWasFromLastGood = lr == ProfileLoadResult_Success;

		/* If the LastGood profile doesn't exist at all, and the actual profile
		 * was failed_tampered, then the error should be failed_tampered and not
		 * failed_no_profile. */
		if (lr == ProfileLoadResult_FailedNoProfile) {
			Locator::getLogger()->warn("Profile was corrupt and LastGood for {} doesn't exist; "
					   "error is ProfileLoadResult_FailedTampered",
					   sProfileDir.c_str());
			lr = ProfileLoadResult_FailedTampered;
		}
	}

	Locator::getLogger()->info("Done loading profile - result {}", lr);

	return lr;
}

bool
ProfileManager::LoadLocalProfileFromMachine(PlayerNumber pn)
{
	std::string sProfileID = m_sDefaultLocalProfileID[pn];
	if (sProfileID.empty()) {
		m_sProfileDir = "";
		return false;
	}

	m_sProfileDir = LocalProfileIDToDir(sProfileID);
	m_bLastLoadWasFromLastGood = false;

	if (GetLocalProfile(sProfileID) == nullptr) {
		m_sProfileDir = "";
		return false;
	}

	GetProfile(pn)->LoadCustomFunction(m_sProfileDir);

	return true;
}

bool
ProfileManager::LoadFirstAvailableProfile(PlayerNumber pn, bool bLoadEdits)
{
	if (LoadLocalProfileFromMachine(pn))
		return true;

	return false;
}

bool
ProfileManager::SaveProfile(PlayerNumber pn) const
{
	if (m_sProfileDir.empty())
		return false;

	/*
	 * If the profile we're writing was loaded from the primary (non-backup)
	 * data, then we've validated it and know it's good.  Before writing our
	 * new data, move the old, good data to the backup.  (Only do this once;
	 * if we save the profile more than once, we haven't re-validated the
	 * newly written data.)
	 */
	if (m_bNeedToBackUpLastLoad) {
		m_bNeedToBackUpLastLoad = false;
		std::string sBackupDir = m_sProfileDir + LAST_GOOD_SUBDIR;
		Profile::MoveBackupToDir(m_sProfileDir, sBackupDir);
	}

	bool b = GetProfile(pn)->SaveAllToDir(m_sProfileDir);

	return b;
}

bool
ProfileManager::SaveLocalProfile(const std::string& sProfileID)
{
	const Profile* pProfile = GetLocalProfile(sProfileID);
	ASSERT(pProfile != NULL);
	std::string sDir = LocalProfileIDToDir(sProfileID);
	bool b = pProfile->SaveAllToDir(sDir);
	return b;
}

void
ProfileManager::UnloadProfile(PlayerNumber pn)
{
	if (m_sProfileDir.empty()) {
		// Don't bother unloading a profile that wasn't loaded in the first
		// place. Saves us an expensive and pointless trip through all the
		// songs.
		return;
	}
	m_sProfileDir = "";
	m_bLastLoadWasTamperedOrCorrupt = false;
	m_bLastLoadWasFromLastGood = false;
	m_bNeedToBackUpLastLoad = false;
}

const Profile*
ProfileManager::GetProfile(PlayerNumber pn) const
{

	std::string sProfileID = LocalProfileDirToID(m_sProfileDir);
	return GetLocalProfile(sProfileID);
}

std::string
ProfileManager::GetPlayerName(PlayerNumber pn) const
{
	const Profile* prof = GetProfile(pn);
	return prof ? prof->GetDisplayNameOrHighScoreName() : std::string();
}

void
ProfileManager::UnloadAllLocalProfiles()
{
	g_vLocalProfile.clear();
}

void
ProfileManager::RefreshLocalProfilesFromDisk(LoadingWindow* ld)
{
	if (ld)
		ld->SetText("Loading Profiles");
	UnloadAllLocalProfiles();

	std::vector<std::string> profile_ids;
	GetDirListing(USER_PROFILES_DIR + "*", profile_ids, true, true);

	for (auto& id : profile_ids) {
		DirAndProfile derp;
		derp.sDir = id + "/";
		derp.profile.m_sProfileID = derp.sDir;
		derp.profile.LoadTypeFromDir(derp.sDir);

		g_vLocalProfile.push_back(derp);
	}

	for (auto& p : g_vLocalProfile) {
		p.profile.LoadAllFromDir(p.sDir, ld);
	}
}

const Profile*
ProfileManager::GetLocalProfile(const std::string& sProfileID) const
{
	string sDir = LocalProfileIDToDir(sProfileID);
	for (auto& p : g_vLocalProfile) {
		const string& sOther = p.sDir;
		if (sOther == sDir)
			return &p.profile;
	}

	return dummy;
}

bool
ProfileManager::CreateLocalProfile(const std::string& sName,
								   std::string& sProfileIDOut)
{
	ASSERT(!sName.empty());

	// Find a directory directory name that's a number greater than all
	// existing numbers.  This preserves the "order by create date".
	// Profile IDs are actually the directory names, so they can be any string,
	// and we have to handle the case where the user renames one.
	// Since the user can rename them, they might have any number, wrapping our
	// counter or setting it to a ridiculous value.  That case must also be
	// handled. -Kyz
	int max_profile_number = -1;
	int first_free_number = 0;
	std::vector<std::string> profile_ids;
	GetLocalProfileIDs(profile_ids);
	for (auto& id : profile_ids) {
		int tmp = 0;
		if (id >> tmp) {
			// The profile ids are already in order, so we don't have to handle
			// the case where 5 is encountered before 3.
			if (tmp == first_free_number) {
				++first_free_number;
			}
			max_profile_number = std::max(tmp, max_profile_number);
		}
	}

	int profile_number = max_profile_number + 1;
	// Prevent profiles from going over the 8 digit limit.
	if (profile_number > MAX_ID || profile_number < 0) {
		profile_number = first_free_number;
	}
	ASSERT_M(profile_number >= 0 && profile_number <= MAX_ID,
			 "Too many profiles, cannot assign ID to new profile.");
	std::string profile_id = ssprintf("%0" ID_DIGITS_STR "d", profile_number);

	// make sure this id doesn't already exist
	// ASSERT_M(GetLocalProfile(profile_id) == NULL,
	// ssprintf("creating profile with ID \"%s\" that already exists",
	// profile_id.c_str()));

	// Create the new profile.
	Profile* pProfile = new Profile;
	pProfile->m_sDisplayName = sName;
	pProfile->m_sProfileID = profile_id;

	// Save it to disk.
	std::string sProfileDir = LocalProfileIDToDir(profile_id);
	if (!pProfile->SaveAllToDir(sProfileDir)) {
		delete pProfile;
		sProfileIDOut = "";
		return false;
	}

	AddLocalProfileByID(pProfile, profile_id);

	sProfileIDOut = profile_id;
	return true;
}

static void
InsertProfileIntoList(DirAndProfile& derp)
{
	bool inserted = false;
	derp.profile.m_ListPriority = 0;
	for (auto& curr : g_vLocalProfile) {
		std::ignore = curr;
		++derp.profile.m_ListPriority;
	}

	if (!inserted) {
		derp.profile.SaveTypeToDir(derp.sDir);
		g_vLocalProfile.push_back(derp);
	}
}

void
ProfileManager::AddLocalProfileByID(Profile* pProfile,
									const std::string& sProfileID)
{
	// make sure this id doesn't already exist
	// ASSERT_M( GetLocalProfile(sProfileID) == NULL,
	// ssprintf("creating \"%s\" \"%s\" that already exists",
	// pProfile->m_sDisplayName.c_str(), sProfileID.c_str()) );

	DirAndProfile derp;
	derp.sDir = LocalProfileIDToDir(sProfileID);
	derp.profile = *pProfile;
	InsertProfileIntoList(derp);
}

bool
ProfileManager::RenameLocalProfile(const std::string& sProfileID,
								   const std::string& sNewName)
{
	ASSERT(!sProfileID.empty());

	Profile* pProfile = ProfileManager::GetLocalProfile(sProfileID);
	ASSERT(pProfile != NULL);
	pProfile->m_sDisplayName = sNewName;

	std::string sProfileDir = LocalProfileIDToDir(sProfileID);
	return pProfile->SaveAllToDir(sProfileDir);
}

bool
ProfileManager::LastLoadWasTamperedOrCorrupt(PlayerNumber pn) const
{
	return !m_sProfileDir.empty() && m_bLastLoadWasTamperedOrCorrupt;
}

bool
ProfileManager::LastLoadWasFromLastGood(PlayerNumber pn) const
{
	return !m_sProfileDir.empty() && m_bLastLoadWasFromLastGood;
}

const std::string&
ProfileManager::GetProfileDir(ProfileSlot slot) const
{
	switch (slot) {
		case ProfileSlot_Player1:
		case ProfileSlot_Player2:
			return m_sProfileDir;
		default:
			FAIL_M("Invalid profile slot chosen: unable to get the directory!");
	}
}

const Profile*
ProfileManager::GetProfile(ProfileSlot slot) const
{
	switch (slot) {
		case ProfileSlot_Player1:
		case ProfileSlot_Player2:
			return GetProfile((PlayerNumber)slot);
		default:
			FAIL_M("Invalid profile slot chosen: unable to get the profile!");
	}
}

void
ProfileManager::MoveProfilePriority(int index, bool up)
{
	if (index < 0 || static_cast<size_t>(index) >= g_vLocalProfile.size()) {
		return;
	}
	// Changing the priority is complicated a bit because the profiles might
	// all have the same priority.  So this function has to assign priorities
	// to all the profiles of the same type.
	// bools are numbers, true evaluatues to 1.
	int swindex = index + ((up * -2) + 1);
	int priority = 0;
	for (size_t i = 0; i < g_vLocalProfile.size(); ++i) {
		DirAndProfile* curr = &g_vLocalProfile[i];

		if (curr->profile.m_ListPriority != priority) {
			curr->profile.m_ListPriority = priority;
			if (i != static_cast<size_t>(index) &&
				i != static_cast<size_t>(swindex)) {
				curr->profile.SaveTypeToDir(curr->sDir);
			}

			++priority;
		}
	}
	// Only swap if both indices are valid and the types match.
	if (swindex >= 0 && static_cast<size_t>(swindex) < g_vLocalProfile.size()) {
		g_vLocalProfile[index].swap(g_vLocalProfile[swindex]);
	}
}

//
// General
//
void
ProfileManager::IncrementToastiesCount(PlayerNumber pn)
{
	++GetProfile(pn)->m_iNumToasties;
}

void
ProfileManager::AddStepTotals(PlayerNumber pn,
							  int iNumTapsAndHolds,
							  int iNumJumps,
							  int iNumHolds,
							  int iNumRolls,
							  int iNumMines,
							  int iNumHands,
							  int iNumLifts)
{
	GetProfile(pn)->AddStepTotals(iNumTapsAndHolds,
								  iNumJumps,
								  iNumHolds,
								  iNumRolls,
								  iNumMines,
								  iNumHands,
								  iNumLifts);
}

void
ProfileManager::GetLocalProfileIDs(std::vector<std::string>& vsProfileIDsOut) const
{
	vsProfileIDsOut.clear();
	for (auto& i : g_vLocalProfile) {
		std::string sID = LocalProfileDirToID(i.sDir);
		vsProfileIDsOut.push_back(sID);
	}
}

void
ProfileManager::GetLocalProfileDisplayNames(
  std::vector<std::string>& vsProfileDisplayNamesOut) const
{
	vsProfileDisplayNamesOut.clear();
	for (auto& i : g_vLocalProfile) {
		vsProfileDisplayNamesOut.push_back(i.profile.m_sDisplayName);
	}
}

int
ProfileManager::GetLocalProfileIndexFromID(const std::string& sProfileID) const
{
	std::string sDir = LocalProfileIDToDir(sProfileID);
	int counter = 0;
	for (auto& i : g_vLocalProfile) {
		if (i.sDir == sDir) {
			return counter;
		}
		++counter;
	}
	return -1;
}

std::string
ProfileManager::GetLocalProfileIDFromIndex(int iIndex)
{
	std::string sID = LocalProfileDirToID(g_vLocalProfile[iIndex].sDir);
	return sID;
}

Profile*
ProfileManager::GetLocalProfileFromIndex(int iIndex)
{
	return &g_vLocalProfile[iIndex].profile;
}

int
ProfileManager::GetNumLocalProfiles() const
{
	return g_vLocalProfile.size();
}

void
ProfileManager::SetStatsPrefix(std::string const& prefix)
{
	m_stats_prefix = prefix;
	for (size_t i = 0; i < g_vLocalProfile.size(); ++i) {
		g_vLocalProfile[i].profile.HandleStatsPrefixChange(
		  g_vLocalProfile[i].sDir);
	}
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ProfileManager. */
class LunaProfileManager : public Luna<ProfileManager>
{
  public:
	static int GetStatsPrefix(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetStatsPrefix().c_str());
		return 1;
	}
	static int SetStatsPrefix(T* p, lua_State* L)
	{
		std::string prefix = SArg(1);
		p->SetStatsPrefix(prefix);
		COMMON_RETURN_SELF;
	}
	// concept of persistent profiles is deprecated
	static int IsPersistentProfile(T* p, lua_State* L)
	{
		lua_pushboolean(L, true);
		return 1;
	}
	static int GetProfile(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;
		Profile* pP = p->GetProfile(pn);
		pP->PushSelf(L);
		return 1;
	}
	static int GetMachineProfile(T* p, lua_State* L)
	{
		Profile* pP = p->GetProfile(PLAYER_1);
		pP->PushSelf(L);
		return 1;
	}
	static int GetLocalProfile(T* p, lua_State* L)
	{
		Profile* pProfile = p->GetLocalProfile(SArg(1));
		if (pProfile != nullptr)
			pProfile->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetLocalProfileFromIndex(T* p, lua_State* L)
	{
		int index = IArg(1);
		if (index >= p->GetNumLocalProfiles()) {
			luaL_error(L, "Profile index %d out of range.", index);
		}
		Profile* pProfile = p->GetLocalProfileFromIndex(index);
		if (pProfile == nullptr) {
			luaL_error(L, "No profile at index %d.", index);
		}
		pProfile->PushSelf(L);
		return 1;
	}
	static int GetLocalProfileIDFromIndex(T* p, lua_State* L)
	{
		int index = IArg(1);
		if (index >= p->GetNumLocalProfiles()) {
			luaL_error(L, "Profile index %d out of range.", index);
		}
		lua_pushstring(L, p->GetLocalProfileIDFromIndex(index).c_str());
		return 1;
	}
	static int GetLocalProfileIndexFromID(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetLocalProfileIndexFromID(SArg(1)));
		return 1;
	}
	static int GetNumLocalProfiles(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNumLocalProfiles());
		return 1;
	}
	static int GetProfileDir(T* p, lua_State* L)
	{
		lua_pushstring(
		  L, p->GetProfileDir(Enum::Check<ProfileSlot>(L, 1)).c_str());
		return 1;
	}
	// TODO: SCOREMAN
	static int IsSongNew(T* p, lua_State* L)
	{
		lua_pushboolean(L, false);
		return 1;
	}
	static int LastLoadWasTamperedOrCorrupt(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->LastLoadWasTamperedOrCorrupt(PLAYER_1));
		return 1;
	}
	static int GetPlayerName(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;
		lua_pushstring(L, p->GetPlayerName(pn).c_str());
		return 1;
	}

	static int LocalProfileIDToDir(T*, lua_State* L)
	{
		std::string dir = USER_PROFILES_DIR + SArg(1) + "/";
		lua_pushstring(L, dir.c_str());
		return 1;
	}
	static int SaveProfile(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->SaveProfile(PLAYER_1));
		return 1;
	}
	static int SaveLocalProfile(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->SaveLocalProfile(SArg(1)));
		return 1;
	}
	// TODO: SCOREMAN
	static int GetSongNumTimesPlayed(T* p, lua_State* L)
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	static int GetLocalProfileIDs(T* p, lua_State* L)
	{
		std::vector<std::string> vsProfileIDs;
		p->GetLocalProfileIDs(vsProfileIDs);
		LuaHelpers::CreateTableFromArray<std::string>(vsProfileIDs, L);
		return 1;
	}
	static int GetLocalProfileDisplayNames(T* p, lua_State* L)
	{
		std::vector<std::string> vsProfileNames;
		p->GetLocalProfileDisplayNames(vsProfileNames);
		LuaHelpers::CreateTableFromArray<std::string>(vsProfileNames, L);
		return 1;
	}
	static int CreateDefaultProfile(T* p, lua_State* L)
	{
		std::string profileID;
		p->CreateLocalProfile("Default Profile", profileID);
		p->m_sDefaultLocalProfileID[PLAYER_1].Set(profileID);
		p->LoadLocalProfileFromMachine(PLAYER_1);
		GAMESTATE->LoadCurrentSettingsFromProfile(PLAYER_1);

		Profile* pProfile = p->GetLocalProfile(profileID);
		if (pProfile != nullptr)
			pProfile->PushSelf(L);
		else
			lua_pushnil(L);
		
		return 1;
	}
	static int SetProfileIDToUse(T* p, lua_State* L)
	{
		p->UnloadProfile(PLAYER_1);
		// no checking to make sure this is right.
		// do it yourself. set this from an existing profile.
		std::string id = SArg(1);
		p->m_sDefaultLocalProfileID[PLAYER_1].Set(id);
		p->LoadLocalProfileFromMachine(PLAYER_1);
		GAMESTATE->LoadCurrentSettingsFromProfile(PLAYER_1);

		COMMON_RETURN_SELF;
	}
	LunaProfileManager()
	{
		ADD_METHOD(GetStatsPrefix);
		ADD_METHOD(SetStatsPrefix);
		ADD_METHOD(IsPersistentProfile);
		ADD_METHOD(GetProfile);
		ADD_METHOD(GetMachineProfile);
		ADD_METHOD(GetLocalProfile);
		ADD_METHOD(GetLocalProfileFromIndex);
		ADD_METHOD(GetLocalProfileIDFromIndex);
		ADD_METHOD(GetLocalProfileIndexFromID);
		ADD_METHOD(GetNumLocalProfiles);
		ADD_METHOD(GetProfileDir);
		ADD_METHOD(IsSongNew);
		ADD_METHOD(LastLoadWasTamperedOrCorrupt);
		ADD_METHOD(GetPlayerName);
		//
		ADD_METHOD(SaveProfile);
		ADD_METHOD(SaveLocalProfile);
		ADD_METHOD(GetSongNumTimesPlayed);
		ADD_METHOD(GetLocalProfileIDs);
		ADD_METHOD(GetLocalProfileDisplayNames);
		ADD_METHOD(LocalProfileIDToDir);
		ADD_METHOD(CreateDefaultProfile);
		ADD_METHOD(SetProfileIDToUse);
	}
};

LUA_REGISTER_CLASS(ProfileManager)
// lua end
