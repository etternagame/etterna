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
	void GetAnnouncerNames(std::vector<RString>& AddTo);
	/**
	 * @brief Determine if the specified announcer exists.
	 * @param sAnnouncerName the announcer we're checking for.
	 * @return true if it exists, false otherwise. */
	bool DoesAnnouncerExist(const RString& sAnnouncerName);
	/**
	 * @brief Switch to a new specified announcer.
	 * @param sNewAnnouncerName the new announcer the Player will be listening
	 * to. */
	void SwitchAnnouncer(const RString& sNewAnnouncerName);
	/**
	 * @brief Retrieve the current announcer's name.
	 * @return the current announcer's name. */
	RString GetCurAnnouncerName() const { return m_sCurAnnouncerName; };
	void NextAnnouncer();

	RString GetPathTo(const RString& sFolderName);
	bool HasSoundsFor(const RString& sFolderName);

	// Lua
	void PushSelf(lua_State* L);

  protected:
	static RString GetAnnouncerDirFromName(const RString& sAnnouncerName);
	RString GetPathTo(const RString& AnnouncerPath, const RString& sFolderName);
	/** @brief the current announcer's name. */
	RString m_sCurAnnouncerName;
};

extern AnnouncerManager*
  ANNOUNCER; // global and accessible from anywhere in our program

#endif
