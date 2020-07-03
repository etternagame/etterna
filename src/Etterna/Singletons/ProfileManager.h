#ifndef ProfileManager_H
#define ProfileManager_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/PlayerNumber.h"
#include "Etterna/Models/Misc/Preference.h"
#include "Etterna/Models/Misc/Profile.h"
#include "arch/LoadingWindow/LoadingWindow.h"

class Song;
class Steps;
class Style;
struct HighScore;
struct lua_State;
class ProfileManager
{
  public:
	ProfileManager();
	~ProfileManager();

	void Init(LoadingWindow* ld);

	bool FixedProfiles() const; // If true, profiles shouldn't be added/deleted

	// local profiles
	void UnloadAllLocalProfiles();
	void RefreshLocalProfilesFromDisk();
	void RefreshLocalProfilesFromDisk(LoadingWindow* ld);
	const Profile* GetLocalProfile(const std::string& sProfileID) const;
	Profile* GetLocalProfile(const std::string& sProfileID)
	{
		return (Profile*)((const ProfileManager*)this)
		  ->GetLocalProfile(sProfileID);
	}
	Profile* GetLocalProfileFromIndex(int iIndex);
	std::string GetLocalProfileIDFromIndex(int iIndex);

	bool CreateLocalProfile(const std::string& sName,
							std::string& sProfileIDOut);
	void AddLocalProfileByID(
	  Profile* pProfile,
	  const std::string& sProfileID); // transfers ownership of pProfile
	bool RenameLocalProfile(const std::string& sProfileID,
							const std::string& sNewName);
	void GetLocalProfileIDs(vector<std::string>& vsProfileIDsOut) const;
	void GetLocalProfileDisplayNames(
	  vector<std::string>& vsProfileDisplayNamesOut) const;
	int GetLocalProfileIndexFromID(const std::string& sProfileID) const;
	int GetNumLocalProfiles() const;

	std::string GetStatsPrefix() { return m_stats_prefix; }
	void SetStatsPrefix(std::string const& prefix);

	bool LoadFirstAvailableProfile(PlayerNumber pn, bool bLoadEdits = true);
	bool LoadLocalProfileFromMachine(PlayerNumber pn);
	bool SaveProfile(PlayerNumber pn) const;
	bool SaveLocalProfile(const std::string& sProfileID);
	void UnloadProfile(PlayerNumber pn);

	void MergeLocalProfiles(std::string const& from_id,
							std::string const& to_id);
	void MoveProfilePriority(int index, bool up);

	// General data
	void IncrementToastiesCount(PlayerNumber pn);
	void AddStepTotals(PlayerNumber pn,
					   int iNumTapsAndHolds,
					   int iNumJumps,
					   int iNumHolds,
					   int iNumRolls,
					   int iNumMines,
					   int iNumHands,
					   int iNumLifts);

	// return a profile even if !IsUsingProfile
	const Profile* GetProfile(PlayerNumber pn) const;
	Profile* GetProfile(PlayerNumber pn)
	{
		return (Profile*)((const ProfileManager*)this)->GetProfile(pn);
	}
	const Profile* GetProfile(ProfileSlot slot) const;
	Profile* GetProfile(ProfileSlot slot)
	{
		return (Profile*)((const ProfileManager*)this)->GetProfile(slot);
	}

	const std::string& GetProfileDir(ProfileSlot slot) const;

	std::string GetPlayerName(PlayerNumber pn) const;
	bool LastLoadWasTamperedOrCorrupt(PlayerNumber pn) const;
	bool LastLoadWasFromLastGood(PlayerNumber pn) const;

	void IncrementStepsPlayCount(const Song* pSong,
								 const Steps* pSteps,
								 PlayerNumber pn);
	// Lua
	void PushSelf(lua_State* L);

	static Preference1D<std::string> m_sDefaultLocalProfileID;

  private:
	ProfileLoadResult LoadProfile(PlayerNumber pn,
								  const std::string& sProfileDir);

	// Directory that contains the profile.  Either on local machine or
	// on a memory card.
	std::string m_sProfileDir;

	std::string m_stats_prefix;
	Profile* dummy;
	bool m_bLastLoadWasTamperedOrCorrupt; // true if Stats.xml was
										  // present, but failed to
										  // load (probably because
										  // of a signature
										  // failure)
	bool m_bLastLoadWasFromLastGood;	  // if true, then
										  // m_bLastLoadWasTamperedOrCorrupt
										  // is also true
	mutable bool m_bNeedToBackUpLastLoad; // if true, back up
										  // profile on next save
	bool m_bNewProfile;
};

extern ProfileManager*
  PROFILEMAN; // global and accessible from anywhere in our program

#endif
