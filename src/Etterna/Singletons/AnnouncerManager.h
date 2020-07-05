#ifndef ANNOUNCER_MANAGER_H
#define ANNOUNCER_MANAGER_H

#include "RageUtil/Misc/RageTypes.h"
/** @brief The commentators who say seemlingly random things during gameplay. */
class AnnouncerManager
{
  public:
	AnnouncerManager();
	~AnnouncerManager();

	/**
	 * @brief Retrieve the announcer names.
	 * @param AddTo the list of announcer names. */
	void GetAnnouncerNames(vector<std::string>& AddTo);
	/**
	 * @brief Determine if the specified announcer exists.
	 * @param sAnnouncerName the announcer we're checking for.
	 * @return true if it exists, false otherwise. */
	bool DoesAnnouncerExist(const std::string& sAnnouncerName);
	/**
	 * @brief Switch to a new specified announcer.
	 * @param sNewAnnouncerName the new announcer the Player will be listening
	 * to. */
	void SwitchAnnouncer(const std::string& sNewAnnouncerName);
	/**
	 * @brief Retrieve the current announcer's name.
	 * @return the current announcer's name. */
	std::string GetCurAnnouncerName() const { return m_sCurAnnouncerName; };
	void NextAnnouncer();

	std::string GetPathTo(const std::string& sFolderName);
	bool HasSoundsFor(const std::string& sFolderName);

	// Lua
	void PushSelf(lua_State* L);

  protected:
	static std::string GetAnnouncerDirFromName(
	  const std::string& sAnnouncerName);
	std::string GetPathTo(const std::string& AnnouncerPath,
						  const std::string& sFolderName);
	/** @brief the current announcer's name. */
	std::string m_sCurAnnouncerName;
};

extern AnnouncerManager*
  ANNOUNCER; // global and accessible from anywhere in our program

#endif
