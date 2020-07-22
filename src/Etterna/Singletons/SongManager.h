#ifndef SONGMANAGER_H
#define SONGMANAGER_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/ImageCache.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/MinaCalc/MinaCalc.h"

#include <unordered_map>

class LoadingWindow;
class Song;
class Style;
class Steps;
class PlayerOptions;
class Calc;
struct lua_State;
struct GoalsForChart;

auto SONG_GROUP_COLOR_NAME(size_t i) -> std::string;
auto CompareNotesPointersForExtra(const Steps* n1, const Steps* n2) -> bool;

/** @brief The holder for the Songs and its Steps. */
namespace SongManager {

extern std::unordered_map<std::string, Song*> SongsByKey;
extern std::unordered_map<std::string, Steps*> StepsByKey;
/** @brief The most popular songs ranked by number of plays. */
extern std::vector<Song*> m_pPopularSongs;

extern std::vector<std::string> m_sSongGroupNames;
extern std::vector<std::string>
  m_sSongGroupBannerPaths; // each song group may have a
						   // banner associated with it
extern std::string activeplaylist;
extern std::string playlistcourse;

extern std::map<std::string, std::vector<Song*>> groupderps;
/**@brief To delete from groupderps when rebuilding playlist groups*/
extern std::vector<std::string> playlistGroups;

extern std::map<Skillset, CalcTestList> testChartList;
extern std::unique_ptr<Calc> calc;

void Init();
void End();
void CalcTestStuff();
void Cleanup();

auto GetPlaylists() -> std::map<std::string, Playlist>&;

void InitAll(LoadingWindow* ld); // songs, groups - everything.
auto DifferentialReload() -> int;
auto DifferentialReloadDir(std::string dir) -> int; // TODO: INTERNAL

auto IsGroupNeverCached(const std::string& group) -> bool;
void SetFavoritedStatus(std::set<std::string>& favs);
void SetPermaMirroredStatus(std::set<std::string>& pmir);
void SetHasGoal(std::unordered_map<std::string, GoalsForChart>& goalmap);

auto GetSongGroupBannerPath(const std::string& sSongGroup) -> std::string;

void GetSongGroupNames(std::vector<std::string>& AddTo);
auto GetSongGroupNames() -> const std::vector<std::string>&;
auto DoesSongGroupExist(const std::string& sSongGroup) -> bool;
auto GetSongGroupColor(const std::string& sSongGroupName,
				  std::map<std::string, Playlist>& playlists = GetPlaylists())
  -> RageColor;
auto GetSongColor(const Song* pSong) -> RageColor;

// temporary solution to reorganizing the entire songid/stepsid system -
// mina
auto GetStepsByChartkey(const std::string& ck) -> Steps*;
auto GetSongByChartkey(const std::string& ck) -> Song*;
void UnloadAllCalcDebugOutput(); // TODO: Should be used but isn't.
inline auto IsChartLoaded(const std::string& ck) -> bool //Lua binding only
{
	return SongsByKey.count(ck) == 1 &&
		   StepsByKey.count(ck) ==
			 1; // shouldn't be necessary but apparently it is -mina
}

void ResetGroupColors();

auto ShortenGroupName(const std::string& sLongGroupName)
  -> std::string; // Lua binding only

// Lookup
/**
 * @brief Retrieve all of the songs that belong to a particular group.
 * @param sGroupName the name of the group.
 * @return the songs that belong in the group. */
auto GetSongs(const std::string& sGroupName) -> const std::vector<Song*>&;
void ForceReloadSongGroup(const std::string& sGroupName);
/**
 * @brief Retrieve all of the songs in the game.
 * @return all of the songs. */
inline auto GetAllSongs() -> const std::vector<Song*>&
{
	return GetSongs(GROUP_ALL);
}
/**
 * @brief Retrieve all of the popular songs.
 *
 * Popularity is determined specifically by the number of times
 * a song is chosen.
 * @return all of the popular songs. */
inline auto GetPopularSongs() -> const std::vector<Song*>& // Lua binding only
{
	return m_pPopularSongs;
}

void GetFavoriteSongs(std::vector<Song*>& songs);
/**
 * @brief Retrieve the number of songs in the game.
 * @return the number of songs. */
auto GetNumSongs() -> int;
auto GetNumAdditionalSongs() -> int; // Lua binding only
auto GetNumSongGroups() -> int;
// sm-ssc addition:
inline auto GetSongGroupByIndex(const unsigned index) -> std::string
{
	return m_sSongGroupNames[index];
}

auto WasLoadedFromAdditionalSongs(const Song* pSong) -> bool;
auto GetSongFromDir(std::string sDir) -> Song*;
void SortSongs(); // sort m_pSongs by CompareSongPointersByTitle

void
ReconcileChartKeysForReloadedSong(const Song* reloadedSong,
								  const std::vector<std::string>& oldChartkeys);
// ^ TODO: INTERNAL?
void MakeSongGroupsFromPlaylists(
  std::map<std::string, Playlist>& playlists = GetPlaylists());
void DeletePlaylist(
  const std::string& pl,
  std::map<std::string, Playlist>& playlists = GetPlaylists()); // Lua only
void MakePlaylistFromFavorites(
  std::set<std::string>& favs,
  std::map<std::string, Playlist>& playlists = GetPlaylists());

void FinalizeSong(Song* pNewSong, const std::string& dir); // TODO: INTERNAL

// calc test stuff
auto SaveCalcTestCreateNode() -> XNode*; // TODO: INTERNAL
void LoadCalcTestNode(); // TODO: INTERNAL
void SaveCalcTestXmlToDir();
};
#endif
