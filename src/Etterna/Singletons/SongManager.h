#ifndef SONGMANAGER_H
#define SONGMANAGER_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/ImageCache.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

#include <unordered_map>

class LoadingWindow;
class Song;
class Style;
class Steps;
class PlayerOptions;
class Calc;
struct lua_State;
struct GoalsForChart;

auto
SONG_GROUP_COLOR_NAME(size_t i) -> std::string;
auto
CompareNotesPointersForExtra(const Steps* n1, const Steps* n2) -> bool;

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
	static auto GetPlaylists() -> std::map<std::string, Playlist>&;
	static void SaveEnabledSongsToPref();
	static void LoadEnabledSongsFromPref();

	void InitAll(LoadingWindow* ld); // songs, groups - everything.
	auto DifferentialReload() -> int;
	auto DifferentialReloadDir(std::string dir) -> int;

	auto IsGroupNeverCached(const std::string& group) const -> bool;
	void SetFavoritedStatus(std::set<std::string>& favs);
	void SetPermaMirroredStatus(std::set<std::string>& pmir);
	void SetHasGoal(std::unordered_map<std::string, GoalsForChart>& goalmap);

	auto GetSongGroupBannerPath(const std::string& sSongGroup) const
	  -> std::string;
	auto GetSongGroupBannerPaths() const -> std::vector<std::string>
	{
		return m_sSongGroupBannerPaths;
	}
	// std::string GetSongGroupBackgroundPath( std::string sSongGroup ) const;
	void GetSongGroupNames(std::vector<std::string>& AddTo) const;
	auto GetSongGroupNames() const -> const std::vector<std::string>&;
	auto DoesSongGroupExist(const std::string& sSongGroup) const -> bool;
	auto GetSongGroupColor(const std::string& sSongGroupName,
						   std::map<std::string, Playlist>& playlists =
							 GetPlaylists()) const -> RageColor;
	auto GetSongColor(const Song* pSong) const -> RageColor;

	// temporary solution to reorganizing the entire songid/stepsid system -
	// mina
	auto GetStepsByChartkey(const std::string& ck) -> Steps*;
	auto GetSongByChartkey(const std::string& ck) -> Song*;
	void UnloadAllCalcDebugOutput();
	auto IsChartLoaded(const std::string& ck) const -> bool
	{
		return SongsByKey.count(ck) == 1 &&
			   StepsByKey.count(ck) ==
				 1; // shouldn't be necessary but apparently it is -mina
	}

	void ResetGroupColors();

	static auto ShortenGroupName(const std::string& sLongGroupName)
	  -> std::string;

	// Lookup
	/**
	 * @brief Retrieve all of the songs that belong to a particular group.
	 * @param sGroupName the name of the group.
	 * @return the songs that belong in the group. */
	auto GetSongs(const std::string& sGroupName) const
	  -> const std::vector<Song*>&;
	void ForceReloadSongGroup(const std::string& sGroupName) const;
	/**
	 * @brief Retrieve all of the songs in the game.
	 * @return all of the songs. */
	auto GetAllSongs() const -> const std::vector<Song*>&
	{
		return GetSongs(GROUP_ALL);
	}
	/**
	 * @brief Retrieve all of the popular songs.
	 *
	 * Popularity is determined specifically by the number of times
	 * a song is chosen.
	 * @return all of the popular songs. */
	auto GetPopularSongs() const -> const std::vector<Song*>&
	{
		return m_pPopularSongs;
	}

	void GetFavoriteSongs(std::vector<Song*>& songs) const;
	/**
	 * @brief Retrieve the number of songs in the game.
	 * @return the number of songs. */
	auto GetNumSongs() const -> int;
	auto GetNumAdditionalSongs() const -> int;
	auto GetNumSongGroups() const -> int;
	// sm-ssc addition:
	auto GetSongGroupByIndex(const unsigned index) -> std::string
	{
		return m_sSongGroupNames[index];
	}

	static void DeleteSteps(Steps* pSteps); // transfers ownership of pSteps
	static auto WasLoadedFromAdditionalSongs(const Song* pSong) -> bool;

	auto GetSongFromDir(std::string sDir) const -> Song*;

	void SortSongs(); // sort m_pSongs by CompareSongPointersByTitle

	// Lua
	void PushSelf(lua_State* L);

	std::string activeplaylist = "";
	std::string playlistcourse = "";
	void ReconcileChartKeysForReloadedSong(
	  Song* reloadedSong,
	  const std::vector<std::string>& oldChartkeys);
	void MakeSongGroupsFromPlaylists(
	  std::map<std::string, Playlist>& playlists = GetPlaylists());
	void DeletePlaylist(
	  const std::string& pl,
	  std::map<std::string, Playlist>& playlists = GetPlaylists());
	static void MakePlaylistFromFavorites(
	  std::set<std::string>& favs,
	  std::map<std::string, Playlist>& playlists = GetPlaylists());

	std::map<std::string, std::vector<Song*>> groupderps;
	std::vector<std::string> playlistGroups; // To delete from groupderps when
											 // rebuilding
											 // playlist groups

	static void FinalizeSong(Song* pNewSong, const std::string& dir);

	// calc test stuff
	auto SaveCalcTestCreateNode() const -> XNode*;
	static void LoadCalcTestNode();
	void SaveCalcTestXmlToDir() const;
	std::map<Skillset, CalcTestList> testChartList;
	std::unique_ptr<Calc> calc;

  protected:
	void LoadStepManiaSongDir(std::string sDir, LoadingWindow* ld);
	static auto IsSongDir(const std::string& sDir) -> bool;
	auto AddGroup(const std::string& sDir, const std::string& sGroupDirName)
	  -> bool;

	void AddSongToList(Song* new_song);
	/** @brief All of the songs that can be played. */
	std::vector<Song*> m_pSongs;
	std::map<std::string, Song*> m_SongsByDir;

	std::vector<std::pair<std::pair<std::string, unsigned int>, Song*>*> cache;

	// Indexed by chartkeys
	void AddKeyedPointers(Song* new_song);
	std::unordered_map<std::string, Song*> SongsByKey;
	std::unordered_map<std::string, Steps*> StepsByKey;

	std::set<std::string> m_GroupsToNeverCache;
	/** @brief The most popular songs ranked by number of plays. */
	std::vector<Song*> m_pPopularSongs;

	std::vector<std::string> m_sSongGroupNames;
	std::vector<std::string>
	  m_sSongGroupBannerPaths; // each song group may have a
							   // banner associated with it
	// std::vector<std::string>		m_sSongGroupBackgroundPaths; // each song group
	// may have a background associated with it (very rarely)

	struct Comp
	{
		auto operator()(const std::string& s, const std::string& t) const
		  -> bool
		{
			return CompareStringsAsc(s, t);
		}
	};
	using SongPointerVector = std::vector<Song*>;
	std::map<std::string, SongPointerVector, Comp> m_mapSongGroupIndex;

	ThemeMetric<int> NUM_SONG_GROUP_COLORS;
	ThemeMetric1D<RageColor> SONG_GROUP_COLOR;
};

extern SongManager*
  SONGMAN; // global and accessible from anywhere in our program

#endif
