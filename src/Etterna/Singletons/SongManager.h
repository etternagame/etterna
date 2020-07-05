#ifndef SONGMANAGER_H
#define SONGMANAGER_H

class LoadingWindow;
class Song;
class Style;
class Steps;
class PlayerOptions;
class Calc;
struct lua_State;
struct GoalsForChart;

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/ImageCache.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

#include <unordered_map>

std::string
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
	void CalcTestStuff();
	void FreeSongs();
	void Cleanup();

	void Invalidate(const Song* pStaleSong);
	static map<string, Playlist>& GetPlaylists();
	void SaveEnabledSongsToPref();
	void LoadEnabledSongsFromPref();

	int GetNumStepsLoadedFromProfile();
	void FreeAllLoadedFromProfile(ProfileSlot slot = ProfileSlot_Invalid);

	void InitAll(LoadingWindow* ld); // songs, groups - everything.
	int DifferentialReload();
	int DifferentialReloadDir(string dir);

	bool IsGroupNeverCached(const std::string& group) const;
	void SetFavoritedStatus(set<string>& favs);
	void SetPermaMirroredStatus(set<string>& pmir);
	void SetHasGoal(unordered_map<string, GoalsForChart>& goalmap);

	std::string GetSongGroupBannerPath(const std::string& sSongGroup) const;
	vector<std::string> GetSongGroupBannerPaths()
	{
		return m_sSongGroupBannerPaths;
	}
	// std::string GetSongGroupBackgroundPath( std::string sSongGroup ) const;
	void GetSongGroupNames(vector<std::string>& AddTo) const;
	const vector<std::string>& GetSongGroupNames() const;
	bool DoesSongGroupExist(const std::string& sSongGroup) const;
	RageColor GetSongGroupColor(
	  const std::string& sSongGroupName,
	  map<string, Playlist>& playlists = GetPlaylists()) const;
	RageColor GetSongColor(const Song* pSong) const;

	// temporary solution to reorganizing the entire songid/stepsid system -
	// mina
	Steps* GetStepsByChartkey(const std::string& ck);
	Song* GetSongByChartkey(const std::string& ck);
	void UnloadAllCalcDebugOutput();
	bool IsChartLoaded(const std::string& ck)
	{
		return SongsByKey.count(ck) == 1 &&
			   StepsByKey.count(ck) ==
				 1; // shouldn't be necessary but apparently it is -mina
	}

	void ResetGroupColors();

	static std::string ShortenGroupName(const std::string& sLongGroupName);

	// Lookup
	/**
	 * @brief Retrieve all of the songs that belong to a particular group.
	 * @param sGroupName the name of the group.
	 * @return the songs that belong in the group. */
	const vector<Song*>& GetSongs(const std::string& sGroupName) const;
	void ForceReloadSongGroup(const std::string& sGroupName) const;
	/**
	 * @brief Retrieve all of the songs in the game.
	 * @return all of the songs. */
	const vector<Song*>& GetAllSongs() const { return GetSongs(GROUP_ALL); }
	/**
	 * @brief Retrieve all of the popular songs.
	 *
	 * Popularity is determined specifically by the number of times
	 * a song is chosen.
	 * @return all of the popular songs. */
	const vector<Song*>& GetPopularSongs() const { return m_pPopularSongs; }

	/**
	 * @brief Retrieve all of the songs in a group that have at least one
	 * valid step for the current gametype.
	 * @param sGroupName the name of the group.
	 * @return the songs within the group that have at least one valid Step. */
	const vector<Song*>& GetSongsOfCurrentGame(const std::string& sGroupName) const;
	/**
	 * @brief Retrieve all of the songs in the game that have at least one
	 * valid step for the current gametype.
	 * @return the songs within the game that have at least one valid Step. */
	const vector<Song*>& GetAllSongsOfCurrentGame() const;

	void GetFavoriteSongs(vector<Song*>& songs) const;
	void GetPreferredSortSongs(vector<Song*>& AddTo) const;
	std::string SongToPreferredSortSectionName(const Song* pSong) const;
	Song* FindSong(std::string sPath) const;
	Song* FindSong(std::string sGroup, std::string sSong) const;
	/**
	 * @brief Retrieve the number of songs in the game.
	 * @return the number of songs. */
	int GetNumSongs() const;
	int GetNumAdditionalSongs() const;
	int GetNumSongGroups() const;
	Song* GetRandomSong();
	// sm-ssc addition:
	std::string GetSongGroupByIndex(unsigned index)
	{
		return m_sSongGroupNames[index];
	}

	void GetStepsLoadedFromProfile(vector<Steps*>& AddTo,
								   ProfileSlot slot) const;
	void DeleteSteps(Steps* pSteps); // transfers ownership of pSteps
	bool WasLoadedFromAdditionalSongs(const Song* pSong) const;

	void GetExtraStageInfo(bool bExtra2,
						   const Style* s,
						   Song*& pSongOut,
						   Steps*& pStepsOut);
	Song* GetSongFromDir(std::string sDir) const;

	void UpdateShuffled(); // re-shuffle songs
	void UpdatePreferredSort(
	  const std::string& sPreferredSongs = "PreferredSongs.txt",
	  const std::string& sPreferredCourses = "PreferredCourses.txt");
	void SortSongs(); // sort m_pSongs by CompareSongPointersByTitle

	// Lua
	void PushSelf(lua_State* L);

	string activeplaylist = "";
	string playlistcourse = "";
	void ReconcileChartKeysForReloadedSong(const Song* reloadedSong,
										   vector<string> oldChartkeys);
	void MakeSongGroupsFromPlaylists(
	  map<string, Playlist>& playlists = GetPlaylists());
	void DeletePlaylist(const std::string& ck,
						map<string, Playlist>& playlists = GetPlaylists());
	void MakePlaylistFromFavorites(
	  set<string>& favs,
	  map<string, Playlist>& playlists = GetPlaylists());

	map<string, vector<Song*>> groupderps;
	vector<string> playlistGroups; // To delete from groupderps when rebuilding
								   // playlist groups

	void FinalizeSong(Song* pNewSong, const std::string& dir);

	// calc test stuff
	XNode* SaveCalcTestCreateNode() const;
	void LoadCalcTestNode() const;
	void SaveCalcTestXmlToDir() const;
	map<Skillset, CalcTestList> testChartList;

	unique_ptr<Calc> calc;

  protected:
	void LoadStepManiaSongDir(std::string sDir, LoadingWindow* ld);
	void LoadDWISongDir(const std::string& sDir);
	bool IsSongDir(const std::string& sDir);
	bool AddGroup(const std::string& sDir, const std::string& sGroupDirName);

	void AddSongToList(Song* new_song);
	/** @brief All of the songs that can be played. */
	vector<Song*> m_pSongs;
	map<std::string, Song*> m_SongsByDir;

	vector<pair<pair<std::string, unsigned int>, Song*>*> cache;

	// Indexed by chartkeys
	void AddKeyedPointers(Song* new_song);
	unordered_map<string, Song*> SongsByKey;
	unordered_map<string, Steps*> StepsByKey;

	set<std::string> m_GroupsToNeverCache;
	/** @brief The most popular songs ranked by number of plays. */
	vector<Song*> m_pPopularSongs;
	// vector<Song*>		m_pRecentSongs;	// songs recently played on the
	// machine
	vector<Song*> m_pShuffledSongs; // used by GetRandomSong
	struct PreferredSortSection
	{
		std::string sName;
		vector<Song*> vpSongs;
	};
	vector<PreferredSortSection> m_vPreferredSongSort;
	vector<std::string> m_sSongGroupNames;
	vector<std::string> m_sSongGroupBannerPaths; // each song group may have a
											 // banner associated with it
	// vector<std::string>		m_sSongGroupBackgroundPaths; // each song group may
	// have a background associated with it (very rarely)

	struct Comp
	{
		bool operator()(const std::string& s, const std::string& t) const
		{
			return CompareRStringsAsc(s, t);
		}
	};
	typedef vector<Song*> SongPointerVector;
	map<std::string, SongPointerVector, Comp> m_mapSongGroupIndex;

	ThemeMetric<int> NUM_SONG_GROUP_COLORS;
	ThemeMetric1D<RageColor> SONG_GROUP_COLOR;
};

extern SongManager*
  SONGMAN; // global and accessible from anywhere in our program

#endif
