#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "GameState.h"
#include "Etterna/Models/Misc/HighScore.h"
#include "PrefsManager.h"
#include "Etterna/Models/Misc/Profile.h"
#include "ProfileManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "SongManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "ThemeManager.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/Models/StepsAndStyles/StepsUtil.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/HighScore.h"
#include "DownloadManager.h"

ProfileManager* PROFILEMAN =
  NULL; // global and accessible from anywhere in our program

#define ID_DIGITS 8
#define ID_DIGITS_STR "8"
#define MAX_ID 99999999

static void
DefaultLocalProfileIDInit(size_t /*PlayerNumber*/ i,
						  RString& sNameOut,
						  RString& defaultValueOut)
{
	sNameOut = ssprintf("DefaultLocalProfileIDP%d", static_cast<int>(i + 1));
	if (i == 0)
		defaultValueOut = "00000000";
	else
		defaultValueOut = "";
}

Preference1D<RString> ProfileManager::m_sDefaultLocalProfileID(
  DefaultLocalProfileIDInit,
  NUM_PLAYERS);

const RString USER_PROFILES_DIR = "/Save/LocalProfiles/";
const RString LAST_GOOD_SUBDIR = "LastGood/";

static RString
LocalProfileIDToDir(const RString& sProfileID)
{
	return USER_PROFILES_DIR + sProfileID + "/";
}
static RString
LocalProfileDirToID(const RString& sDir)
{
	return Basename(sDir);
}

struct DirAndProfile
{
	RString sDir;
	Profile profile;
	void swap(DirAndProfile& other)
	{
		sDir.swap(other.sDir);
		profile.swap(other.profile);
	}
};
static vector<DirAndProfile> g_vLocalProfile;

static ThemeMetric<bool> FIXED_PROFILES("ProfileManager", "FixedProfiles");
static ThemeMetric<int> NUM_FIXED_PROFILES("ProfileManager",
										   "NumFixedProfiles");

ProfileManager::ProfileManager()
  : m_stats_prefix("")
{

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
ProfileManager::LoadProfile(PlayerNumber pn, const RString& sProfileDir)
{
	LOG->Trace("LoadingProfile P%d, %s", pn + 1, sProfileDir.c_str());

	ASSERT(!sProfileDir.empty());
	ASSERT(sProfileDir.Right(1) == "/");

	m_sProfileDir = sProfileDir;
	m_bLastLoadWasFromLastGood = false;
	m_bNeedToBackUpLastLoad = false;

	// Try to load the original, non-backup data.
	ProfileLoadResult lr = GetProfile(pn)->LoadAllFromDir(
	  m_sProfileDir, PREFSMAN->m_bSignProfileData, NULL);

	RString sBackupDir = m_sProfileDir + LAST_GOOD_SUBDIR;

	if (lr == ProfileLoadResult_Success) {
		/* Next time the profile is written, move this good profile into
		 * LastGood. */
		m_bNeedToBackUpLastLoad = true;
	}

	m_bLastLoadWasTamperedOrCorrupt =
	  lr == ProfileLoadResult_FailedTampered;

	//
	// Try to load from the backup if the original data fails to load
	//
	if (lr == ProfileLoadResult_FailedTampered) {
		lr = GetProfile(pn)->LoadAllFromDir(
		  sBackupDir, PREFSMAN->m_bSignProfileData, NULL);
		m_bLastLoadWasFromLastGood = lr == ProfileLoadResult_Success;

		/* If the LastGood profile doesn't exist at all, and the actual profile
		 * was failed_tampered, then the error should be failed_tampered and not
		 * failed_no_profile. */
		if (lr == ProfileLoadResult_FailedNoProfile) {
			LOG->Trace("Profile was corrupt and LastGood for %s doesn't exist; "
					   "error is ProfileLoadResult_FailedTampered",
					   sProfileDir.c_str());
			lr = ProfileLoadResult_FailedTampered;
		}
	}

	LOG->Trace("Done loading profile - result %d", lr);

	return lr;
}

bool
ProfileManager::LoadLocalProfileFromMachine(PlayerNumber pn)
{
	RString sProfileID = m_sDefaultLocalProfileID[pn];
	if (sProfileID.empty()) {
		m_sProfileDir = "";
		return false;
	}

	m_sProfileDir = LocalProfileIDToDir(sProfileID);
	m_bLastLoadWasFromLastGood = false;

	if (GetLocalProfile(sProfileID) == NULL) {
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
		RString sBackupDir = m_sProfileDir + LAST_GOOD_SUBDIR;
		Profile::MoveBackupToDir(m_sProfileDir, sBackupDir);
	}

	bool b = GetProfile(pn)->SaveAllToDir(m_sProfileDir,
										  PREFSMAN->m_bSignProfileData);

	return b;
}

bool
ProfileManager::SaveLocalProfile(const RString& sProfileID)
{
	const Profile* pProfile = GetLocalProfile(sProfileID);
	ASSERT(pProfile != NULL);
	RString sDir = LocalProfileIDToDir(sProfileID);
	bool b = pProfile->SaveAllToDir(sDir, PREFSMAN->m_bSignProfileData);
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
	SONGMAN->FreeAllLoadedFromProfile(static_cast<ProfileSlot>(pn));
}

const Profile*
ProfileManager::GetProfile(PlayerNumber pn) const
{

	RString sProfileID = LocalProfileDirToID(m_sProfileDir);
	return GetLocalProfile(sProfileID);
}

RString
ProfileManager::GetPlayerName(PlayerNumber pn) const
{
	const Profile* prof = GetProfile(pn);
	return prof ? prof->GetDisplayNameOrHighScoreName() : RString();
}

void
ProfileManager::UnloadAllLocalProfiles()
{
	g_vLocalProfile.clear();
}

static void
add_category_to_global_list(vector<DirAndProfile>& cat)
{
	g_vLocalProfile.insert(g_vLocalProfile.end(), cat.begin(), cat.end());
}

void
ProfileManager::RefreshLocalProfilesFromDisk(LoadingWindow* ld)
{
	if (ld)
		ld->SetText("Loading Profiles");
	UnloadAllLocalProfiles();

	vector<RString> profile_ids;
	GetDirListing(USER_PROFILES_DIR + "*", profile_ids, true, true);
	// Profiles have 3 types:
	// 1.  Guest profiles:
	//   Meant for use by guests, always at the top of the list.
	// 2.  Normal profiles:
	//   Meant for normal use, listed after guests.
	// e.  Test profiles:
	//   Meant for use when testing things, listed last.
	// If the user renames a profile directory manually, that should not be a
	// problem. -Kyz
	map<ProfileType, vector<DirAndProfile>> categorized_profiles;
	// The type data for a profile is in its own file so that loading isn't
	// slowed down by copying temporary profiles around to make sure the list
	// is sorted.  The profiles are loaded at the end. -Kyz
	FOREACH_CONST(RString, profile_ids, id)
	{
		DirAndProfile derp;
		derp.sDir = *id + "/";
		derp.profile.m_sProfileID = derp.sDir;
		derp.profile.LoadTypeFromDir(derp.sDir);
		map<ProfileType, vector<DirAndProfile>>::iterator category =
		  categorized_profiles.find(derp.profile.m_Type);
		if (category == categorized_profiles.end()) {
			categorized_profiles[derp.profile.m_Type].push_back(derp);
		} else {
			bool inserted = false;
			FOREACH(DirAndProfile, category->second, curr)
			{
				if (curr->profile.m_ListPriority >
					derp.profile.m_ListPriority) {
					category->second.insert(curr, derp);
					inserted = true;
					break;
				}
			}
			if (!inserted) {
				category->second.push_back(derp);
			}
		}
	}
	add_category_to_global_list(categorized_profiles[ProfileType_Guest]);
	add_category_to_global_list(categorized_profiles[ProfileType_Normal]);
	add_category_to_global_list(categorized_profiles[ProfileType_Test]);
	FOREACH(DirAndProfile, g_vLocalProfile, curr)
	{
		// curr->profile.EoBatchRecalc(curr->sDir, ld);
	}
	FOREACH(DirAndProfile, g_vLocalProfile, curr)
	{
		curr->profile.LoadAllFromDir(
		  curr->sDir, PREFSMAN->m_bSignProfileData, ld);
	}
}

const Profile*
ProfileManager::GetLocalProfile(const RString& sProfileID) const
{
	RString sDir = LocalProfileIDToDir(sProfileID);
	FOREACH_CONST(DirAndProfile, g_vLocalProfile, dap)
	{
		const RString& sOther = dap->sDir;
		if (sOther == sDir)
			return &dap->profile;
	}

	return dummy;
}

bool
ProfileManager::CreateLocalProfile(const RString& sName, RString& sProfileIDOut)
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
	vector<RString> profile_ids;
	GetLocalProfileIDs(profile_ids);
	FOREACH_CONST(RString, profile_ids, id)
	{
		int tmp = 0;
		if ((*id) >> tmp) {
			// The profile ids are already in order, so we don't have to handle
			// the case where 5 is encountered before 3.
			if (tmp == first_free_number) {
				++first_free_number;
			}
			max_profile_number = max(tmp, max_profile_number);
		}
	}

	int profile_number = max_profile_number + 1;
	// Prevent profiles from going over the 8 digit limit.
	if (profile_number > MAX_ID || profile_number < 0) {
		profile_number = first_free_number;
	}
	ASSERT_M(profile_number >= 0 && profile_number <= MAX_ID,
			 "Too many profiles, cannot assign ID to new profile.");
	RString profile_id = ssprintf("%0" ID_DIGITS_STR "d", profile_number);

	// make sure this id doesn't already exist
	// ASSERT_M(GetLocalProfile(profile_id) == NULL,
	// ssprintf("creating profile with ID \"%s\" that already exists",
	// profile_id.c_str()));

	// Create the new profile.
	Profile* pProfile = new Profile;
	pProfile->m_sDisplayName = sName;
	pProfile->m_sProfileID = profile_id;

	// Save it to disk.
	RString sProfileDir = LocalProfileIDToDir(profile_id);
	if (!pProfile->SaveAllToDir(sProfileDir, PREFSMAN->m_bSignProfileData)) {
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
	FOREACH(DirAndProfile, g_vLocalProfile, curr)
	{
		if (curr->profile.m_Type > derp.profile.m_Type) {
			derp.profile.SaveTypeToDir(derp.sDir);
			g_vLocalProfile.insert(curr, derp);
			inserted = true;
			break;
		} else if (curr->profile.m_Type == derp.profile.m_Type) {
			++derp.profile.m_ListPriority;
		}
	}
	if (!inserted) {
		derp.profile.SaveTypeToDir(derp.sDir);
		g_vLocalProfile.push_back(derp);
	}
}

void
ProfileManager::AddLocalProfileByID(Profile* pProfile,
									const RString& sProfileID)
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
ProfileManager::RenameLocalProfile(const RString& sProfileID,
								   const RString& sNewName)
{
	ASSERT(!sProfileID.empty());

	Profile* pProfile = ProfileManager::GetLocalProfile(sProfileID);
	ASSERT(pProfile != NULL);
	pProfile->m_sDisplayName = sNewName;

	RString sProfileDir = LocalProfileIDToDir(sProfileID);
	return pProfile->SaveAllToDir(sProfileDir, PREFSMAN->m_bSignProfileData);
}

bool
ProfileManager::DeleteLocalProfile(const RString& sProfileID)
{
	Profile* pProfile = ProfileManager::GetLocalProfile(sProfileID);
	ASSERT(pProfile != NULL);
	RString sProfileDir = LocalProfileIDToDir(sProfileID);

	// flush directory cache in an attempt to get this working
	FILEMAN->FlushDirCache(sProfileDir);

	FOREACH(DirAndProfile, g_vLocalProfile, i)
	{
		if (i->sDir == sProfileDir) {
			if (DeleteRecursive(sProfileDir)) {
				g_vLocalProfile.erase(i);

				// Delete all references to this profileID
				FOREACH_CONST(
				  Preference<RString>*, m_sDefaultLocalProfileID.m_v, j)
				{
					if ((*j)->Get() == sProfileID)
						(*j)->Set("");
				}
				return true;
			} else {
				LOG->Warn("[ProfileManager::DeleteLocalProfile] "
						  "DeleteRecursive(%s) failed",
						  sProfileID.c_str());
				return false;
			}
		}
	}

	LOG->Warn("DeleteLocalProfile: ProfileID '%s' doesn't exist",
			  sProfileID.c_str());
	return false;
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

const RString&
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
ProfileManager::MergeLocalProfiles(RString const& from_id, RString const& to_id)
{
	Profile* from = GetLocalProfile(from_id);
	Profile* to = GetLocalProfile(to_id);
	if (from == NULL || to == NULL) {
		return;
	}
	to->MergeScoresFromOtherProfile(
	  from, false, LocalProfileIDToDir(from_id), LocalProfileIDToDir(to_id));
}

void
ProfileManager::ChangeProfileType(int index, ProfileType new_type)
{
	if (index < 0 || static_cast<size_t>(index) >= g_vLocalProfile.size()) {
		return;
	}
	if (new_type == g_vLocalProfile[index].profile.m_Type) {
		return;
	}
	DirAndProfile derp = g_vLocalProfile[index];
	g_vLocalProfile.erase(g_vLocalProfile.begin() + index);
	derp.profile.m_Type = new_type;
	InsertProfileIntoList(derp);
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
	ProfileType type = g_vLocalProfile[index].profile.m_Type;
	int priority = 0;
	for (size_t i = 0; i < g_vLocalProfile.size(); ++i) {
		DirAndProfile* curr = &g_vLocalProfile[i];
		if (curr->profile.m_Type == type) {
			if (curr->profile.m_ListPriority != priority) {
				curr->profile.m_ListPriority = priority;
				if (i != static_cast<size_t>(index) &&
					i != static_cast<size_t>(swindex)) {
					curr->profile.SaveTypeToDir(curr->sDir);
				}
			}
			++priority;
		} else if (curr->profile.m_Type > type) {
			break;
		}
	}
	// Only swap if both indices are valid and the types match.
	if (swindex >= 0 && static_cast<size_t>(swindex) < g_vLocalProfile.size() &&
		g_vLocalProfile[swindex].profile.m_Type ==
		  g_vLocalProfile[index].profile.m_Type) {
		g_vLocalProfile[index].swap(g_vLocalProfile[swindex]);
	}
}

//
// General
//
void
ProfileManager::IncrementToastiesCount(PlayerNumber pn)
{
	if (IsPersistentProfile(pn))
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
	if (IsPersistentProfile(pn))
		GetProfile(pn)->AddStepTotals(iNumTapsAndHolds,
									  iNumJumps,
									  iNumHolds,
									  iNumRolls,
									  iNumMines,
									  iNumHands,
									  iNumLifts);
}

//
// Song stats
//
int
ProfileManager::GetSongNumTimesPlayed(const Song* pSong, ProfileSlot slot) const
{
	return GetProfile(slot)->GetSongNumTimesPlayed(pSong);
}

void
ProfileManager::AddStepsScore(const Song* pSong,
							  const Steps* pSteps,
							  PlayerNumber pn,
							  const HighScore& hs_,
							  int& iPersonalIndexOut,
							  int& iMachineIndexOut)
{
	HighScore hs = hs_;
	hs.SetPercentDP(max(0, hs.GetPercentDP())); // bump up negative scores

	iPersonalIndexOut = -1;
	iMachineIndexOut = -1;

	// In event mode, set the score's name immediately to the Profile's last
	// used name.  If no profile last used name exists, use "EVNT".
	if (GAMESTATE->IsEventMode()) {
		Profile* pProfile = GetProfile(pn);
		if (pProfile && !pProfile->m_sLastUsedHighScoreName.empty())
			hs.SetName(pProfile->m_sLastUsedHighScoreName);
		else
			hs.SetName("EVNT");
	} else {
		hs.SetName(RANKING_TO_FILL_IN_MARKER);
	}

	//
	// save high score
	//
	if (IsPersistentProfile(pn))
		GetProfile(pn)->AddStepsHighScore(pSong, pSteps, hs, iPersonalIndexOut);
}

void
ProfileManager::IncrementStepsPlayCount(const Song* pSong,
										const Steps* pSteps,
										PlayerNumber pn)
{
	if (IsPersistentProfile(pn))
		GetProfile(pn)->IncrementStepsPlayCount(pSong, pSteps);
}

bool
ProfileManager::IsPersistentProfile(ProfileSlot slot) const
{
	return true;
}

void
ProfileManager::GetLocalProfileIDs(vector<RString>& vsProfileIDsOut) const
{
	vsProfileIDsOut.clear();
	FOREACH_CONST(DirAndProfile, g_vLocalProfile, i)
	{
		RString sID = LocalProfileDirToID(i->sDir);
		vsProfileIDsOut.push_back(sID);
	}
}

void
ProfileManager::GetLocalProfileDisplayNames(
  vector<RString>& vsProfileDisplayNamesOut) const
{
	vsProfileDisplayNamesOut.clear();
	FOREACH_CONST(DirAndProfile, g_vLocalProfile, i)
	vsProfileDisplayNamesOut.push_back(i->profile.m_sDisplayName);
}

int
ProfileManager::GetLocalProfileIndexFromID(const RString& sProfileID) const
{
	RString sDir = LocalProfileIDToDir(sProfileID);
	FOREACH_CONST(DirAndProfile, g_vLocalProfile, i)
	{
		if (i->sDir == sDir)
			return i - g_vLocalProfile.begin();
	}
	return -1;
}

RString
ProfileManager::GetLocalProfileIDFromIndex(int iIndex)
{
	RString sID = LocalProfileDirToID(g_vLocalProfile[iIndex].sDir);
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
ProfileManager::SetStatsPrefix(RString const& prefix)
{
	m_stats_prefix = prefix;
	for (size_t i = 0; i < g_vLocalProfile.size(); ++i) {
		g_vLocalProfile[i].profile.HandleStatsPrefixChange(
		  g_vLocalProfile[i].sDir, PREFSMAN->m_bSignProfileData);
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
		RString prefix = SArg(1);
		p->SetStatsPrefix(prefix);
		COMMON_RETURN_SELF;
	}
	static int IsPersistentProfile(T* p, lua_State* L)
	{
		lua_pushboolean(
		  L, p->IsPersistentProfile(PLAYER_1));
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
		if (pProfile == NULL) {
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
		lua_pushstring(L, p->GetLocalProfileIDFromIndex(index));
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
		lua_pushstring(L, p->GetProfileDir(Enum::Check<ProfileSlot>(L, 1)));
		return 1;
	}
	static int IsSongNew(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->IsSongNew(Luna<Song>::check(L, 1)));
		return 1;
	}
	static int LastLoadWasTamperedOrCorrupt(T* p, lua_State* L)
	{
		lua_pushboolean(
		  L, p->LastLoadWasTamperedOrCorrupt(PLAYER_1));
		return 1;
	}
	static int GetPlayerName(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;
		lua_pushstring(L, p->GetPlayerName(pn));
		return 1;
	}

	static int LocalProfileIDToDir(T*, lua_State* L)
	{
		RString dir = USER_PROFILES_DIR + SArg(1) + "/";
		lua_pushstring(L, dir);
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
	static int GetSongNumTimesPlayed(T* p, lua_State* L)
	{
		lua_pushnumber(
		  L,
		  p->GetSongNumTimesPlayed(Luna<Song>::check(L, 1),
								   Enum::Check<ProfileSlot>(L, 2)));
		return 1;
	}
	static int GetLocalProfileIDs(T* p, lua_State* L)
	{
		vector<RString> vsProfileIDs;
		p->GetLocalProfileIDs(vsProfileIDs);
		LuaHelpers::CreateTableFromArray<RString>(vsProfileIDs, L);
		return 1;
	}
	static int GetLocalProfileDisplayNames(T* p, lua_State* L)
	{
		vector<RString> vsProfileNames;
		p->GetLocalProfileDisplayNames(vsProfileNames);
		LuaHelpers::CreateTableFromArray<RString>(vsProfileNames, L);
		return 1;
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
	}
};

LUA_REGISTER_CLASS(ProfileManager)
// lua end
