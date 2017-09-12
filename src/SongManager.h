#ifndef SONGMANAGER_H
#define SONGMANAGER_H

class LoadingWindow;
class Song;
class Style;
class Steps;
class PlayerOptions;
struct lua_State;

#include "Difficulty.h"
#include "GameConstantsAndTypes.h"
#include "PlayerNumber.h"
#include "PlayerOptions.h"
#include "Profile.h"
#include "RageTexturePreloader.h"
#include "RageTypes.h"
#include "RageUtil.h"
#include "SongOptions.h"
#include "ThemeMetric.h"

#include <unordered_map>
using std::string;

RString SONG_GROUP_COLOR_NAME( size_t i );
bool CompareNotesPointersForExtra(const Steps *n1, const Steps *n2);



struct Chart {
	string key;
	RString lastsong;
	RString lastpack;
	Difficulty lastdiff = Difficulty_Invalid;
	float rate = 1.f;
	Song* songptr;
	Steps* stepsptr;

	bool IsLoaded() { return loaded; }

	bool loaded = false;
	void FromKey(const string& ck);
	XNode * CreateNode(bool includerate) const;
	void LoadFromNode(const XNode * node);
	void PushSelf(lua_State *L);
};

struct Playlist {
	RString name;
	vector<Chart> chartlist;
	void Add(Chart ch) { chartlist.emplace_back(ch); }
	void AddChart(const string& ck);
	void SwapPosition();

	void Create();
	vector<vector<string>> courseruns;

	XNode* CreateNode() const;
	void LoadFromNode(const XNode* node);
	int GetNumCharts() { return chartlist.size(); }
	vector<string> GetKeys();
	string GetName() { return name; }
	float GetAverageRating();
	void DeleteChart(int i);

	void PushSelf(lua_State *L);
};





/** @brief The holder for the Songs and its Steps. */
class SongManager
{
public:
	SongManager();
	~SongManager();

	void InitSongsFromDisk( LoadingWindow *ld );
	void FreeSongs();
	void UnlistSong(Song *song);
	void Cleanup();

	void Invalidate( const Song *pStaleSong );

	void SetPreferences();
	void SaveEnabledSongsToPref();
	void LoadEnabledSongsFromPref();

	int GetNumStepsLoadedFromProfile();
	void FreeAllLoadedFromProfile( ProfileSlot slot = ProfileSlot_Invalid );

	void InitAll( LoadingWindow *ld );	// songs, groups - everything.
	void Reload( bool bAllowFastLoad, LoadingWindow *ld=nullptr );	// songs, groups - everything.
	int DifferentialReload();
	int DifferentialReloadDir(string dir);
	void PreloadSongImages();

	bool IsGroupNeverCached(const RString& group) const;
	void SetFavoritedStatus(set<string>& favs);
	void SetPermaMirroredStatus(set<string>& pmir);
	void SetHasGoal(unordered_map<string, GoalsForChart>& goalmap);

	RString GetSongGroupBannerPath( const RString &sSongGroup ) const;
	//RString GetSongGroupBackgroundPath( RString sSongGroup ) const;
	void GetSongGroupNames( vector<RString> &AddTo ) const;
	const vector<RString>& GetSongGroupNames() const;
	bool DoesSongGroupExist( const RString &sSongGroup ) const;
	RageColor GetSongGroupColor( const RString &sSongGroupName ) const;
	RageColor GetSongColor( const Song* pSong ) const;

	// temporary solution to reorganizing the entire songid/stepsid system - mina
	Steps* GetStepsByChartkey(RString ck);
	Song * GetSongByChartkey(RString ck);
	Steps* GetStepsByChartkey(const StepsID& sid);
	Song * GetSongByChartkey(const StepsID& sid);
	bool IsChartLoaded(RString ck) { return SongsByKey.count(ck) == 1; }

	void ResetGroupColors();

	static RString ShortenGroupName( const RString &sLongGroupName );

	// Lookup
	/**
	 * @brief Retrieve all of the songs that belong to a particular group.
	 * @param sGroupName the name of the group.
	 * @return the songs that belong in the group. */
	const vector<Song*> &GetSongs( const RString &sGroupName ) const;
	/**
	 * @brief Retrieve all of the songs in the game.
	 * @return all of the songs. */
	const vector<Song*> &GetAllSongs() const { return GetSongs(GROUP_ALL); }
	/**
	 * @brief Retrieve all of the popular songs.
	 *
	 * Popularity is determined specifically by the number of times
	 * a song is chosen.
	 * @return all of the popular songs. */
	const vector<Song*> &GetPopularSongs() const { return m_pPopularSongs; }

	/**
	 * @brief Retrieve all of the songs in a group that have at least one
	 * valid step for the current gametype.
	 * @param sGroupName the name of the group.
	 * @return the songs within the group that have at least one valid Step. */
	const vector<Song *> &GetSongsOfCurrentGame( const RString &sGroupName ) const;
	/**
	 * @brief Retrieve all of the songs in the game that have at least one
	 * valid step for the current gametype.
	 * @return the songs within the game that have at least one valid Step. */
	const vector<Song *> &GetAllSongsOfCurrentGame() const;

	void GetFavoriteSongs(vector<Song*>& songs) const;
	void GetPreferredSortSongs( vector<Song*> &AddTo ) const;
	RString SongToPreferredSortSectionName( const Song *pSong ) const;
	Song *FindSong( RString sPath ) const;
	Song *FindSong( RString sGroup, RString sSong ) const;
	/**
	 * @brief Retrieve the number of songs in the game.
	 * @return the number of songs. */
	int GetNumSongs() const;
	int GetNumAdditionalSongs() const;
	int GetNumSongGroups() const;
	Song* GetRandomSong();
	// sm-ssc addition:
	RString GetSongGroupByIndex(unsigned index) { return m_sSongGroupNames[index]; }
	int GetSongRank(Song* pSong);

	void GetStepsLoadedFromProfile( vector<Steps*> &AddTo, ProfileSlot slot ) const;
	void DeleteSteps( Steps *pSteps );	// transfers ownership of pSteps
	bool WasLoadedFromAdditionalSongs( const Song *pSong ) const;

	void GetExtraStageInfo( bool bExtra2, const Style *s, Song*& pSongOut, Steps*& pStepsOut );
	Song* GetSongFromDir( RString sDir ) const;

	void UpdatePopular();
	void UpdateShuffled();	// re-shuffle songs
	void UpdatePreferredSort(const RString &sPreferredSongs = "PreferredSongs.txt", const RString &sPreferredCourses = "PreferredCourses.txt"); 
	void SortSongs();		// sort m_pSongs by CompareSongPointersByTitle

	// Lua
	void PushSelf( lua_State *L );

	map<string, Playlist> allplaylists;
	string activeplaylist = "";
	string playlistcourse = "";
	string ReconcileBustedKeys(const string& ck);
	map<string, string> keyconversionmap;
	void MakeSongGroupsFromPlaylists();
	void DeletePlaylist(const string& ck);
	void MakePlaylistFromFavorites(set<string>& favs);

	map<string, vector<Song*>> groupderps;
protected:
	void LoadStepManiaSongDir( RString sDir, LoadingWindow *ld );
	void LoadDWISongDir( const RString &sDir );
	void SanityCheckGroupDir( const RString &sDir ) const;
	void AddGroup( const RString &sDir, const RString &sGroupDirName );

	void AddSongToList(Song* new_song);
	/** @brief All of the songs that can be played. */
	vector<Song*>		m_pSongs;
	map<RString, Song*> m_SongsByDir;

	// Indexed by chartkeys
	void AddKeyedPointers(Song* new_song);
	unordered_map<string, Song*> SongsByKey;
	unordered_map<string, Steps*> StepsByKey;

	set<RString> m_GroupsToNeverCache;
	/** @brief Hold pointers to all the songs that have been deleted from disk but must at least be kept temporarily alive for smooth audio transitions. */
	vector<Song*>       m_pDeletedSongs;
	/** @brief The most popular songs ranked by number of plays. */
	vector<Song*>		m_pPopularSongs;
	//vector<Song*>		m_pRecentSongs;	// songs recently played on the machine
	vector<Song*>		m_pShuffledSongs;	// used by GetRandomSong
	struct PreferredSortSection
	{
		RString sName;
		vector<Song*> vpSongs;
	};
	vector<PreferredSortSection> m_vPreferredSongSort;
	vector<RString>		m_sSongGroupNames;
	vector<RString>		m_sSongGroupBannerPaths; // each song group may have a banner associated with it
	//vector<RString>		m_sSongGroupBackgroundPaths; // each song group may have a background associated with it (very rarely)

	struct Comp { bool operator()(const RString& s, const RString &t) const { return CompareRStringsAsc(s,t); } };
	typedef vector<Song*> SongPointerVector;
	map<RString,SongPointerVector,Comp> m_mapSongGroupIndex;

	RageTexturePreloader m_TexturePreload;

	ThemeMetric<int>		NUM_SONG_GROUP_COLORS;
	ThemeMetric1D<RageColor>	SONG_GROUP_COLOR;
};

extern SongManager*	SONGMAN;	// global and accessible from anywhere in our program

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
