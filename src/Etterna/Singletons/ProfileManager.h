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

	auto FixedProfiles() const
	  -> bool; // If true, profiles shouldn't be added/deleted

	// local profiles
	void UnloadAllLocalProfiles();
	void RefreshLocalProfilesFromDisk();
	void RefreshLocalProfilesFromDisk(LoadingWindow* ld);
	auto GetLocalProfile(const std::string& sProfileID) const -> const Profile*;
	auto GetLocalProfile(const std::string& sProfileID) -> Profile*
	{
		return const_cast<Profile*>(
		  static_cast<const ProfileManager*>(this)->GetLocalProfile(
			sProfileID));
	}
	auto GetLocalProfileFromIndex(int iIndex) -> Profile*;
	auto GetLocalProfileIDFromIndex(int iIndex) -> std::string;

	auto CreateLocalProfile(const std::string& sName,
							std::string& sProfileIDOut) -> bool;
	void AddLocalProfileByID(
	  Profile* pProfile,
	  const std::string& sProfileID); // transfers ownership of pProfile
	auto RenameLocalProfile(const std::string& sProfileID,
							const std::string& sNewName) -> bool;
	void GetLocalProfileIDs(vector<std::string>& vsProfileIDsOut) const;
	void GetLocalProfileDisplayNames(
	  vector<std::string>& vsProfileDisplayNamesOut) const;
	auto GetLocalProfileIndexFromID(const std::string& sProfileID) const -> int;
	auto GetNumLocalProfiles() const -> int;

	auto GetStatsPrefix() -> std::string { return m_stats_prefix; }
	void SetStatsPrefix(std::string const& prefix);

	auto LoadFirstAvailableProfile(PlayerNumber pn, bool bLoadEdits = true)
	  -> bool;
	auto LoadLocalProfileFromMachine(PlayerNumber pn) -> bool;
	auto SaveProfile(PlayerNumber pn) const -> bool;
	auto SaveLocalProfile(const std::string& sProfileID) -> bool;
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
	auto GetProfile(PlayerNumber pn) const -> const Profile*;
	auto GetProfile(PlayerNumber pn) -> Profile*
	{
		return const_cast<Profile*>(
		  static_cast<const ProfileManager*>(this)->GetProfile(pn));
	}
	auto GetProfile(ProfileSlot slot) const -> const Profile*;
	auto GetProfile(ProfileSlot slot) -> Profile*
	{
		return const_cast<Profile*>(
		  static_cast<const ProfileManager*>(this)->GetProfile(slot));
	}

	auto GetProfileDir(ProfileSlot slot) const -> const std::string&;

	auto GetPlayerName(PlayerNumber pn) const -> std::string;
	auto LastLoadWasTamperedOrCorrupt(PlayerNumber pn) const -> bool;
	auto LastLoadWasFromLastGood(PlayerNumber pn) const -> bool;

	void IncrementStepsPlayCount(const Song* pSong,
								 const Steps* pSteps,
								 PlayerNumber pn);
	// Lua
	void PushSelf(lua_State* L);

	static Preference1D<std::string> m_sDefaultLocalProfileID;

  private:
	auto LoadProfile(PlayerNumber pn, const std::string& sProfileDir)
	  -> ProfileLoadResult;

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
