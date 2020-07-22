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

extern std::unordered_map<std::string, Song*> SongsByKey;
extern std::unordered_map<std::string, Steps*> StepsByKey;
extern std::set<std::string> m_GroupsToNeverCache;
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

extern void
Init();
extern void
End();
extern void
CalcTestStuff();
extern void
Cleanup();

extern auto
GetPlaylists() -> std::map<std::string, Playlist>&;
extern void
SaveEnabledSongsToPref(); // TODO: DEAD CODE

extern void
InitAll(LoadingWindow* ld); // songs, groups - everything.
extern auto
DifferentialReload() -> int;
extern auto
DifferentialReloadDir(std::string dir) -> int; // TODO: INTERNAL

extern auto
IsGroupNeverCached(const std::string& group) -> bool;
extern void
SetFavoritedStatus(std::set<std::string>& favs);
extern void
SetPermaMirroredStatus(std::set<std::string>& pmir);
extern void
SetHasGoal(std::unordered_map<std::string, GoalsForChart>& goalmap);

extern auto
GetSongGroupBannerPath(const std::string& sSongGroup) -> std::string;
inline auto
GetSongGroupBannerPaths() -> std::vector<std::string>
{
	return m_sSongGroupBannerPaths;
} // TODO: DEAD CODE
// std::string GetSongGroupBackgroundPath( std::string sSongGroup ) const;
extern void
GetSongGroupNames(std::vector<std::string>& AddTo);
extern auto
GetSongGroupNames() -> const std::vector<std::string>&;
extern auto
DoesSongGroupExist(const std::string& sSongGroup) -> bool;
extern auto
GetSongGroupColor(const std::string& sSongGroupName,
				  std::map<std::string, Playlist>& playlists = GetPlaylists())
  -> RageColor;
extern auto
GetSongColor(const Song* pSong) -> RageColor;

// temporary solution to reorganizing the entire songid/stepsid system -
// mina
extern auto
GetStepsByChartkey(const std::string& ck) -> Steps*;
extern auto
GetSongByChartkey(const std::string& ck) -> Song*;
extern void
UnloadAllCalcDebugOutput(); // TODO: Should be used but isn't.
inline auto
IsChartLoaded(const std::string& ck) -> bool // TODO: INTERNAL
{
	return SongsByKey.count(ck) == 1 &&
		   StepsByKey.count(ck) ==
			 1; // shouldn't be necessary but apparently it is -mina
}

extern void
ResetGroupColors();

extern auto
ShortenGroupName(const std::string& sLongGroupName) -> std::string;

// Lookup
/**
 * @brief Retrieve all of the songs that belong to a particular group.
 * @param sGroupName the name of the group.
 * @return the songs that belong in the group. */
extern auto
GetSongs(const std::string& sGroupName) -> const std::vector<Song*>&;
extern void
ForceReloadSongGroup(const std::string& sGroupName);
/**
 * @brief Retrieve all of the songs in the game.
 * @return all of the songs. */
inline auto
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
inline auto
GetPopularSongs() -> const std::vector<Song*>& // Lua binding only
{
	return m_pPopularSongs;
}

extern void
GetFavoriteSongs(std::vector<Song*>& songs);
/**
 * @brief Retrieve the number of songs in the game.
 * @return the number of songs. */
extern auto
GetNumSongs() -> int;
extern auto
GetNumAdditionalSongs() -> int; // Lua binding only
extern auto
GetNumSongGroups() -> int;
// sm-ssc addition:
inline auto
GetSongGroupByIndex(const unsigned index) -> std::string
{
	return m_sSongGroupNames[index];
}

extern void
DeleteSteps(Steps* pSteps); // transfers ownership of pSteps
							// ^ TODO: DEAD CODE
extern auto
WasLoadedFromAdditionalSongs(const Song* pSong) -> bool;
extern auto
GetSongFromDir(std::string sDir) -> Song*;
extern void
SortSongs(); // sort m_pSongs by CompareSongPointersByTitle

extern void
ReconcileChartKeysForReloadedSong(const Song* reloadedSong,
								  const std::vector<std::string>& oldChartkeys);
// ^ TODO: INTERNAL
extern void
MakeSongGroupsFromPlaylists(
  std::map<std::string, Playlist>& playlists = GetPlaylists());
extern void
DeletePlaylist(
  const std::string& pl,
  std::map<std::string, Playlist>& playlists = GetPlaylists()); // Lua only
extern void
MakePlaylistFromFavorites(
  std::set<std::string>& favs,
  std::map<std::string, Playlist>& playlists = GetPlaylists());

extern void
FinalizeSong(Song* pNewSong, const std::string& dir); // TODO: INTERNAL

// calc test stuff
extern auto
SaveCalcTestCreateNode() -> XNode*; // TODO: INTERNAL
extern void
LoadCalcTestNode(); // TODO: INTERNAL
extern void
SaveCalcTestXmlToDir();
};
#endif
