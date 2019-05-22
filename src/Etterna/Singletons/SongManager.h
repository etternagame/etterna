#ifndef SONGMANAGER_H
#define SONGMANAGER_H

class LoadingWindow;
class Song;
class Style;
class Steps;
class PlayerOptions;
struct lua_State;
struct GoalsForChart;

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/PlayerNumber.h"
#include "Etterna/Models/Misc/PlayerOptions.h"
#include "Etterna/Models/Misc/ImageCache.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

#include <unordered_map>
using std::string;

RString
SONG_GROUP_COLOR_NAME(size_t i);
bool
CompareNotesPointersForExtra(const Steps* n1, const Steps* n2);

/** @brief The holder for the Songs and its Steps. */
class SongManager
{
  public:
	SongManager();
	~SongManager();

	void InitSongsFromDisk(LoadingWindow* ld);
	void FreeSongs();
	void Cleanup();

	void Invalidate(const Song* pStaleSong);
	static std::map<string, Playlist>& GetPlaylists();
	void SaveEnabledSongsToPref();
	void LoadEnabledSongsFromPref();

	int GetNumStepsLoadedFromProfile();
	void FreeAllLoadedFromProfile(ProfileSlot slot = ProfileSlot_Invalid);

	void InitAll(LoadingWindow* ld); // songs, groups - everything.
	int DifferentialReload();
	int DifferentialReloadDir(string dir);

	bool IsGroupNeverCached(const RString& group) const;
	void SetFavoritedStatus(set<string>& favs);
	void SetPermaMirroredStatus(set<string>& pmir);
	void SetHasGoal(unordered_map<string, GoalsForChart>& goalmap);

	RString GetSongGroupBannerPath(const RString& sSongGroup) const;
	std::vector<RString> GetSongGroupBannerPaths() {
		return 	m_sSongGroupBannerPaths;
	}
	// RString GetSongGroupBackgroundPath( RString sSongGroup ) const;
	void GetSongGroupNames(std::vector<RString>& AddTo) const;
	const std::vector<RString>& GetSongGroupNames() const;
	bool DoesSongGroupExist(const RString& sSongGroup) const;
	RageColor GetSongGroupColor(
	  const RString& sSongGroupName,
	  std::map<string, Playlist>& playlists = GetPlaylists()) const;
	RageColor GetSongColor(const Song* pSong) const;

	// temporary solution to reorganizing the entire songid/stepsid system -
	// mina
	Steps* GetStepsByChartkey(const string& ck);
	Song* GetSongByChartkey(const string& ck);
	bool IsChartLoaded(const string& ck)
	{
		return SongsByKey.count(ck) == 1 && StepsByKey.count(ck) == 1;	// shouldn't be necessary but apparently it is -mina
	}

	void ResetGroupColors();

	static RString ShortenGroupName(const RString& sLongGroupName);

	// Lookup
	/**
	 * @brief Retrieve all of the songs that belong to a particular group.
	 * @param sGroupName the name of the group.
	 * @return the songs that belong in the group. */
	const std::vector<Song*>& GetSongs(const RString& sGroupName) const;
	void ForceReloadSongGroup(const RString& sGroupName) const;
	/**
	 * @brief Retrieve all of the songs in the game.
	 * @return all of the songs. */
	const std::vector<Song*>& GetAllSongs() const { return GetSongs(GROUP_ALL); }
	/**
	 * @brief Retrieve all of the popular songs.
	 *
	 * Popularity is determined specifically by the number of times
	 * a song is chosen.
	 * @return all of the popular songs. */
	const std::vector<Song*>& GetPopularSongs() const { return m_pPopularSongs; }

	/**
	 * @brief Retrieve all of the songs in a group that have at least one
	 * valid step for the current gametype.
	 * @param sGroupName the name of the group.
	 * @return the songs within the group that have at least one valid Step. */
	const std::vector<Song*>& GetSongsOfCurrentGame(const RString& sGroupName) const;
	/**
	 * @brief Retrieve all of the songs in the game that have at least one
	 * valid step for the current gametype.
	 * @return the songs within the game that have at least one valid Step. */
	const std::vector<Song*>& GetAllSongsOfCurrentGame() const;

	void GetFavoriteSongs(std::vector<Song*>& songs) const;
	void GetPreferredSortSongs(std::vector<Song*>& AddTo) const;
	RString SongToPreferredSortSectionName(const Song* pSong) const;
	Song* FindSong(RString sPath) const;
	Song* FindSong(RString sGroup, RString sSong) const;
	/**
	 * @brief Retrieve the number of songs in the game.
	 * @return the number of songs. */
	int GetNumSongs() const;
	int GetNumAdditionalSongs() const;
	int GetNumSongGroups() const;
	Song* GetRandomSong();
	// sm-ssc addition:
	RString GetSongGroupByIndex(unsigned index)
	{
		return m_sSongGroupNames[index];
	}
	int GetSongRank(Song* pSong);

	void GetStepsLoadedFromProfile(std::vector<Steps*>& AddTo,
								   ProfileSlot slot) const;
	void DeleteSteps(Steps* pSteps); // transfers ownership of pSteps
	bool WasLoadedFromAdditionalSongs(const Song* pSong) const;

	void GetExtraStageInfo(bool bExtra2,
						   const Style* s,
						   Song*& pSongOut,
						   Steps*& pStepsOut);
	Song* GetSongFromDir(RString sDir) const;

	void UpdateShuffled(); // re-shuffle songs
	void UpdatePreferredSort(
	  const RString& sPreferredSongs = "PreferredSongs.txt",
	  const RString& sPreferredCourses = "PreferredCourses.txt");
	void SortSongs(); // sort m_pSongs by CompareSongPointersByTitle

	// Lua
	void PushSelf(lua_State* L);

	string activeplaylist = "";
	string playlistcourse = "";
	void ReconcileChartKeysForReloadedSong(const Song* reloadedSong,
										   std::vector<string> oldChartkeys);
	void MakeSongGroupsFromPlaylists(
	  std::map<string, Playlist>& playlists = GetPlaylists());
	void DeletePlaylist(const string& ck,
						std::map<string, Playlist>& playlists = GetPlaylists());
	void MakePlaylistFromFavorites(
	  set<string>& favs,
	  std::map<string, Playlist>& playlists = GetPlaylists());

	std::map<string, std::vector<Song*>> groupderps;
	std::vector<string> playlistGroups; // To delete from groupderps when rebuilding
								   // playlist groups

	void FinalizeSong(Song* pNewSong, const RString& dir);

  protected:
	void LoadStepManiaSongDir(RString sDir, LoadingWindow* ld);
	void LoadDWISongDir(const RString& sDir);
	bool IsSongDir(const RString& sDir);
	bool AddGroup(const RString& sDir, const RString& sGroupDirName);

	void AddSongToList(Song* new_song);
	/** @brief All of the songs that can be played. */
	std::vector<Song*> m_pSongs;
	std::map<RString, Song*> m_SongsByDir;

	std::vector<std::pair<std::pair<RString, unsigned int>, Song*>*> cache;

	// Indexed by chartkeys
	void AddKeyedPointers(Song* new_song);
	unordered_map<string, Song*> SongsByKey;
	unordered_map<string, Steps*> StepsByKey;

	set<RString> m_GroupsToNeverCache;
	/** @brief The most popular songs ranked by number of plays. */
	std::vector<Song*> m_pPopularSongs;
	// vector<Song*>		m_pRecentSongs;	// songs recently played on the
	// machine
	std::vector<Song*> m_pShuffledSongs; // used by GetRandomSong
	struct PreferredSortSection
	{
		RString sName;
		std::vector<Song*> vpSongs;
	};
	std::vector<PreferredSortSection> m_vPreferredSongSort;
	std::vector<RString> m_sSongGroupNames;
	std::vector<RString> m_sSongGroupBannerPaths; // each song group may have a
											 // banner associated with it
	// vector<RString>		m_sSongGroupBackgroundPaths; // each song group may
	// have a background associated with it (very rarely)

	struct Comp
	{
		bool operator()(const RString& s, const RString& t) const
		{
			return CompareRStringsAsc(s, t);
		}
	};
	typedef std::vector<Song*> SongPointerVector;
	std::map<RString, SongPointerVector, Comp> m_mapSongGroupIndex;

	ThemeMetric<int> NUM_SONG_GROUP_COLORS;
	ThemeMetric1D<RageColor> SONG_GROUP_COLOR;
};

extern SongManager*
  SONGMAN; // global and accessible from anywhere in our program

#endif

/**
 * @file
 * @author Chris Danford, Glenn Maynard (c) 2001-2004
 * @section LICENSE
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
