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

auto
SONG_GROUP_COLOR_NAME(size_t i) -> std::string;
auto
CompareNotesPointersForExtra(const Steps* n1, const Steps* n2) -> bool;

/** @brief The holder for the Songs and its Steps. */
namespace SONGMAN {
std::unordered_map<std::string, Song*> SongsByKey;
std::unordered_map<std::string, Steps*> StepsByKey;

std::set<std::string> m_GroupsToNeverCache;
/** @brief The most popular songs ranked by number of plays. */
std::vector<Song*> m_pPopularSongs;

std::vector<std::string> m_sSongGroupNames;
std::vector<std::string> m_sSongGroupBannerPaths; // each song group may have a
												  // banner associated with it

void
Init();
void
End();

void
InitSongsFromDisk(LoadingWindow* ld);
void
CalcTestStuff();
void
FreeSongs();
void
Cleanup();

void
Invalidate(const Song* pStaleSong);
auto
GetPlaylists() -> std::map<std::string, Playlist>&;
void
SaveEnabledSongsToPref();
void
LoadEnabledSongsFromPref();

void
InitAll(LoadingWindow* ld); // songs, groups - everything.
auto
DifferentialReload() -> int;
auto
DifferentialReloadDir(std::string dir) -> int;

auto
IsGroupNeverCached(const std::string& group) -> bool;
void
SetFavoritedStatus(std::set<std::string>& favs);
void
SetPermaMirroredStatus(std::set<std::string>& pmir);
void
SetHasGoal(std::unordered_map<std::string, GoalsForChart>& goalmap);

auto
GetSongGroupBannerPath(const std::string& sSongGroup) -> std::string;
auto
GetSongGroupBannerPaths() -> std::vector<std::string>
{
	return m_sSongGroupBannerPaths;
}
// std::string GetSongGroupBackgroundPath( std::string sSongGroup ) const;
void
GetSongGroupNames(std::vector<std::string>& AddTo);
auto
GetSongGroupNames() -> const std::vector<std::string>&;
auto
DoesSongGroupExist(const std::string& sSongGroup) -> bool;
auto
GetSongGroupColor(const std::string& sSongGroupName,
				  std::map<std::string, Playlist>& playlists = GetPlaylists())
  -> RageColor;
auto
GetSongColor(const Song* pSong) -> RageColor;

// temporary solution to reorganizing the entire songid/stepsid system -
// mina
auto
GetStepsByChartkey(const std::string& ck) -> Steps*;
auto
GetSongByChartkey(const std::string& ck) -> Song*;
void
UnloadAllCalcDebugOutput();
auto
IsChartLoaded(const std::string& ck) -> bool
{
	return SongsByKey.count(ck) == 1 &&
		   StepsByKey.count(ck) ==
			 1; // shouldn't be necessary but apparently it is -mina
}

void
ResetGroupColors();

auto
ShortenGroupName(const std::string& sLongGroupName) -> std::string;

// Lookup
/**
 * @brief Retrieve all of the songs that belong to a particular group.
 * @param sGroupName the name of the group.
 * @return the songs that belong in the group. */
auto
GetSongs(const std::string& sGroupName) -> const std::vector<Song*>&;
void
ForceReloadSongGroup(const std::string& sGroupName);
/**
 * @brief Retrieve all of the songs in the game.
 * @return all of the songs. */
auto
GetAllSongs() -> const std::vector<Song*>&
{
	return GetSongs(GROUP_ALL);
}
/**
 * @brief Retrieve all of the popular songs.
 *
 * Popularity is determined specifically by the number of times
 * a song is chosen.
 * @return all of the popular songs. */
auto
GetPopularSongs() -> const std::vector<Song*>&
{
	return m_pPopularSongs;
}

void
GetFavoriteSongs(std::vector<Song*>& songs);
/**
 * @brief Retrieve the number of songs in the game.
 * @return the number of songs. */
auto
GetNumSongs() -> int;
auto
GetNumAdditionalSongs() -> int;
auto
GetNumSongGroups() -> int;
// sm-ssc addition:
auto
GetSongGroupByIndex(const unsigned index) -> std::string
{
	return m_sSongGroupNames[index];
}

void
DeleteSteps(Steps* pSteps); // transfers ownership of pSteps
auto
WasLoadedFromAdditionalSongs(const Song* pSong) -> bool;

auto
GetSongFromDir(std::string sDir) -> Song*;

void
SortSongs(); // sort m_pSongs by CompareSongPointersByTitle

// Lua
void
PushSelf(lua_State* L);

std::string activeplaylist = "";
std::string playlistcourse = "";
void
ReconcileChartKeysForReloadedSong(const Song* reloadedSong,
								  const std::vector<std::string>& oldChartkeys);
void
MakeSongGroupsFromPlaylists(
  std::map<std::string, Playlist>& playlists = GetPlaylists());
void
DeletePlaylist(const std::string& pl,
			   std::map<std::string, Playlist>& playlists = GetPlaylists());
void
MakePlaylistFromFavorites(
  std::set<std::string>& favs,
  std::map<std::string, Playlist>& playlists = GetPlaylists());

std::map<std::string, std::vector<Song*>> groupderps;
std::vector<std::string> playlistGroups; // To delete from groupderps when
										 // rebuilding
										 // playlist groups

void
FinalizeSong(Song* pNewSong, const std::string& dir);

// calc test stuff
auto
SaveCalcTestCreateNode() -> XNode*;
void
LoadCalcTestNode();
void
SaveCalcTestXmlToDir();
std::map<Skillset, CalcTestList> testChartList;
std::unique_ptr<Calc> calc;
};
#endif
