#include "global.h"
#include "SongManager.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "ActorUtil.h"
#include "AnnouncerManager.h"
#include "BackgroundUtil.h"
#include "BannerCache.h"
#include "CommonMetrics.h"
#include "Foreach.h"
#include "GameManager.h"
#include "GameState.h"
#include "LocalizedString.h"
#include "MsdFile.h"
#include "NoteSkinManager.h"
#include "NotesLoaderDWI.h"
#include "NotesLoaderSSC.h"
#include "NotesLoaderSM.h"
#include "PrefsManager.h"
#include "Profile.h"
#include "ProfileManager.h"
#include "RageFile.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "Song.h"
#include "SongCacheIndex.h"
#include "SongUtil.h"
#include "Sprite.h"
#include "StatsManager.h"
#include "Steps.h"
#include "StepsUtil.h"
#include "Style.h"
#include "ThemeManager.h"
#include "TitleSubstitution.h"
#include "SpecialFiles.h"
#include "ScreenTextEntry.h"

SongManager*	SONGMAN = NULL;	// global and accessible from anywhere in our program

const RString ADDITIONAL_SONGS_DIR	= "/AdditionalSongs/";
const RString EDIT_SUBDIR		= "Edits/";

/** @brief The file that contains various random attacks. */
const RString ATTACK_FILE		= "/Data/RandomAttacks.txt";

static const ThemeMetric<RageColor>	EXTRA_COLOR			( "SongManager", "ExtraColor" );
static const ThemeMetric<int>		EXTRA_COLOR_METER		( "SongManager", "ExtraColorMeter" );
static const ThemeMetric<bool>		USE_PREFERRED_SORT_COLOR	( "SongManager", "UsePreferredSortColor" );
static const ThemeMetric<int>		EXTRA_STAGE2_DIFFICULTY_MAX	( "SongManager", "ExtraStage2DifficultyMax" );

static Preference<RString> g_sDisabledSongs( "DisabledSongs", "" );

RString SONG_GROUP_COLOR_NAME( size_t i )   { return ssprintf( "SongGroupColor%i", (int) i+1 ); }

static const float next_loading_window_update= 0.02f;

AutoScreenMessage(SM_BackFromNamePlaylist);

SongManager::SongManager()
{
	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring( L, "SONGMAN" );
		this->PushSelf( L );
		lua_settable( L, LUA_GLOBALSINDEX );
		LUA->Release( L );
	}

	NUM_SONG_GROUP_COLORS	.Load( "SongManager", "NumSongGroupColors" );
	SONG_GROUP_COLOR		.Load( "SongManager", SONG_GROUP_COLOR_NAME, NUM_SONG_GROUP_COLORS );
}

SongManager::~SongManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal( "SONGMAN" );

	// Courses depend on Songs and Songs don't depend on Courses.
	// So, delete the Courses first.
	FreeSongs();
}

void SongManager::InitAll( LoadingWindow *ld )
{
	vector<RString> never_cache;
	split(PREFSMAN->m_NeverCacheList, ",", never_cache);
	for(vector<RString>::iterator group= never_cache.begin();
			group != never_cache.end(); ++group)
	{
		m_GroupsToNeverCache.insert(*group);
	}
	InitSongsFromDisk( ld );
}

static LocalizedString RELOADING ( "SongManager", "Reloading..." );
static LocalizedString UNLOADING_SONGS ( "SongManager", "Unloading songs..." );
static LocalizedString SANITY_CHECKING_GROUPS("SongManager", "Sanity checking groups...");

void SongManager::Reload( bool bAllowFastLoad, LoadingWindow *ld )
{
	FILEMAN->FlushDirCache( SpecialFiles::SONGS_DIR );
	FILEMAN->FlushDirCache( ADDITIONAL_SONGS_DIR );
	FILEMAN->FlushDirCache( EDIT_SUBDIR );

	if( ld )
		ld->SetText( RELOADING );

	// save scores before unloading songs, or the scores will be lost

	if( ld )
		ld->SetText( UNLOADING_SONGS );

	FreeSongs();

	const bool OldVal = PREFSMAN->m_bFastLoad;
	if( !bAllowFastLoad )
		PREFSMAN->m_bFastLoad.Set( false );

	InitAll( ld );

	if( !bAllowFastLoad )
		PREFSMAN->m_bFastLoad.Set( OldVal );

	UpdatePreferredSort();
}

// See InitSongsFromDisk for any comment clarification -mina
int SongManager::DifferentialReload() {
	int newsongs = 0;
	SONGINDEX->delay_save_cache = true;
	newsongs += DifferentialReloadDir(SpecialFiles::SONGS_DIR);

	const bool bOldVal = PREFSMAN->m_bFastLoad;
	PREFSMAN->m_bFastLoad.Set(PREFSMAN->m_bFastLoadAdditionalSongs);
	newsongs += DifferentialReloadDir(ADDITIONAL_SONGS_DIR);
	PREFSMAN->m_bFastLoad.Set(bOldVal);
	LoadEnabledSongsFromPref();
	SONGINDEX->SaveCacheIndex();
	SONGINDEX->delay_save_cache = false;

	return newsongs;
}

// See LoadStepManiaSongDir for any comment clarification -mina
int SongManager::DifferentialReloadDir(string dir) {
	if (dir.substr(dir.size()) != "/")
		dir += "/";

	int newsongs = 0;

	vector<RString> arrayGroupDirs;
	GetDirListing(dir + "*", arrayGroupDirs, true);
	StripCvsAndSvn(arrayGroupDirs);
	StripMacResourceForks(arrayGroupDirs);
	SortRStringArray(arrayGroupDirs);

	vector< vector<RString> > arrayGroupSongDirs;
	int groupIndex, songCount, songIndex;

	groupIndex = 0;
	songCount = 0;
	FOREACH_CONST(RString, arrayGroupDirs, s) {
		RString sGroupDirName = *s;
		SanityCheckGroupDir(dir + sGroupDirName);

		vector<RString> arraySongDirs;
		GetDirListing(dir + sGroupDirName + "/*", arraySongDirs, true, true);
		StripCvsAndSvn(arraySongDirs);
		StripMacResourceForks(arraySongDirs);
		SortRStringArray(arraySongDirs);

		arrayGroupSongDirs.emplace_back(arraySongDirs);
		songCount += arraySongDirs.size();
	}

	if (songCount == 0) return 0;

	groupIndex = 0;
	songIndex = 0;
	
	FOREACH_CONST(RString, arrayGroupDirs, s) {
		RString sGroupDirName = *s;
		vector<RString> &arraySongDirs = arrayGroupSongDirs[groupIndex++];
		int loaded = 0;

		SongPointerVector& index_entry = m_mapSongGroupIndex[sGroupDirName];
		RString group_base_name = Basename(sGroupDirName);
		for (size_t j = 0; j < arraySongDirs.size(); ++j) {
			RString sSongDirName = arraySongDirs[j];
			
			// skip any dir we've already loaded -mina
			RString hur = sSongDirName + "/";
			hur.MakeLower();
			if (m_SongsByDir.count(hur))
				continue;

			Song* pNewSong = new Song;
			if (!pNewSong->LoadFromSongDir(sSongDirName)) {
				delete pNewSong;
				continue;
			}

			AddSongToList(pNewSong);
			AddKeyedPointers(pNewSong);

			index_entry.emplace_back(pNewSong);

			loaded++;
			songIndex++;
			newsongs++;
		}

		if (!loaded) continue;
		LOG->Trace("Differential load of %i songs from \"%s\"", loaded, (dir + sGroupDirName).c_str());

		AddGroup(dir, sGroupDirName);
		BANNERCACHE->CacheBanner(GetSongGroupBannerPath(sGroupDirName));
	}
	return newsongs;
}

void SongManager::InitSongsFromDisk( LoadingWindow *ld )
{
	RageTimer tm;
	// Tell SONGINDEX to not write the cache index file every time a song adds
	// an entry. -Kyz
	SONGINDEX->delay_save_cache= true;
	LoadStepManiaSongDir( SpecialFiles::SONGS_DIR, ld );

	const bool bOldVal = PREFSMAN->m_bFastLoad;
	PREFSMAN->m_bFastLoad.Set( PREFSMAN->m_bFastLoadAdditionalSongs );
	LoadStepManiaSongDir( ADDITIONAL_SONGS_DIR, ld );
	PREFSMAN->m_bFastLoad.Set( bOldVal );
	LoadEnabledSongsFromPref();
	SONGINDEX->SaveCacheIndex();
	SONGINDEX->delay_save_cache= false;

	LOG->Trace( "Found %i songs in %f seconds.", m_pSongs.size(), tm.GetDeltaTime() );
}

void Chart::FromKey(const string& ck) { 
	Song* song = SONGMAN->GetSongByChartkey(ck);
	key = ck;

	if (song) {
		Steps* steps = SONGMAN->GetStepsByChartkey(ck);
		lastpack = song->m_sGroupName;
		lastsong = song->GetDisplayMainTitle();
		lastdiff = steps->GetDifficulty();
		loaded = true;
		songptr = song;
		stepsptr = steps;
		return;
	}
	loaded = false;
}

XNode* Chart::CreateNode(bool includerate) const {
	XNode* ch = new XNode("Chart");
	ch->AppendAttr("Key", key);
	ch->AppendAttr("Pack", lastpack);
	ch->AppendAttr("Song", lastsong);
	ch->AppendAttr("Steps", DifficultyToString(lastdiff));

	if(includerate)
		ch->AppendAttr("Rate", ssprintf("%.3f",rate));
	return ch;
}

void Chart::LoadFromNode(const XNode* node) {
	ASSERT(node->GetName() == "Chart");

	RString s;
	node->GetAttrValue("Pack", lastpack);
	node->GetAttrValue("Song", lastsong);
	node->GetAttrValue("Steps", s); lastdiff = StringToDifficulty(s);
	node->GetAttrValue("Rate", s);	rate = StringToFloat(s);
	node->GetAttrValue("Key", s); key = s;

	// check if this chart is loaded and overwrite any last-seen values with updated ones
	key = SONGMAN->ReconcileBustedKeys(key);
	FromKey(key);
}

void Playlist::AddChart(const string & ck) {
	float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	Chart ch;
	ch.FromKey(ck);
	ch.rate = rate;
	chartlist.emplace_back(ch);
}

void Playlist::DeleteChart(int i) {
	if (chartlist.size() == 0 || i < 0 || i > static_cast<int>(chartlist.size()))
		return;

	chartlist.erase(chartlist.begin() + i);
}


XNode* Playlist::CreateNode() const {
	XNode* pl = new XNode("Playlist");
	pl->AppendAttr("Name", name);

	XNode* cl = new XNode("Chartlist");
	FOREACH_CONST(Chart, chartlist, ch)
		cl->AppendChild(ch->CreateNode(true));

	XNode* cr = new XNode("CourseRuns");
	FOREACH_CONST(vector<string>, courseruns, run) {
		XNode* r = new XNode("Run");
		FOREACH_CONST(string, *run, sk)
			r->AppendChild(*sk);
		cr->AppendChild(r);
	}

	if (!cl->ChildrenEmpty())
		pl->AppendChild(cl);

	if (!cr->ChildrenEmpty())
		pl->AppendChild(cr);
	
	return pl;
}

void Playlist::LoadFromNode(const XNode* node) {
	ASSERT(node->GetName() == "Playlist");

	node->GetAttrValue("Name", name);
	if (!node->ChildrenEmpty()) {
		const XNode* cl = node->GetChild("Chartlist");
		FOREACH_CONST_Child(cl, chart) {
			Chart ch;
			ch.LoadFromNode(chart);
			chartlist.emplace_back(ch);
		}

		const XNode* cr = node->GetChild("CourseRuns");
		if (cr) {
			FOREACH_CONST_Child(cr, run) {
				vector<string> tmp;
				FOREACH_CONST_Child(run, sk)
					tmp.emplace_back(sk->GetName());
				courseruns.emplace_back(tmp);
			}
		}
	}
}

void SongManager::MakeSongGroupsFromPlaylists() {
	for (auto& p : allplaylists) {
		vector<Song*> playlistgroup;
		for (auto& n : p.second.chartlist)
			if (n.loaded)
				playlistgroup.emplace_back(SONGMAN->GetSongByChartkey(n.key));

		groupderps.erase(p.first);
		groupderps[p.first] = playlistgroup;
		SongUtil::SortSongPointerArrayByTitle(groupderps[p.first]);
	}
}

float Playlist::GetAverageRating() {
	if (chartlist.empty())
		return 0;
	float o = 0.f;
	int numloaded = 0;
	for (auto& n : chartlist) {
		if (n.loaded) {
			o += n.stepsptr->GetMSD(n.rate, 0);
			++numloaded;
		}
	}
	if (numloaded == 0)
		return 0.f;
	return o / static_cast<float>(numloaded);
}

vector<string> Playlist::GetKeys() {
	vector<string> o;
	for (size_t i = 0; i < chartlist.size(); ++i)
		o.emplace_back(chartlist[i].key);	
	return o;
}

void SongManager::DeletePlaylist(const string& pl) {
	allplaylists.erase(pl);

	// stuff gets weird if all playlists have been deleted and a chart is added - mina
	if(allplaylists.size() > 0)
		activeplaylist = allplaylists.begin()->first;

	// clear out the entry for the music wheel as well or it'll crash -mina
	groupderps.erase(pl);
}

void SongManager::MakePlaylistFromFavorites(set<string>& favs) {
	Playlist pl;
	pl.name = "Favorites";
	for (auto& n : favs)
		pl.AddChart(n);

	// kinda messy but, trim unloaded charts from the favorites playlist -mina
	for (size_t i = 0; i < pl.chartlist.size(); ++i)
		if (!pl.chartlist[i].loaded)
			pl.DeleteChart(i);

	allplaylists.emplace("Favorites", pl);
}

string SongManager::ReconcileBustedKeys(const string& ck) {
	if (StepsByKey.count(ck))
		return ck;

	if (keyconversionmap.count(ck))
		return keyconversionmap[ck];
	return ck;
}

// Only store 1 steps/song pointer per key -Mina
void SongManager::AddKeyedPointers(Song* new_song) {
	const vector<Steps*> steps = new_song->GetAllSteps();
	for (size_t i = 0; i < steps.size(); ++i) {
		const RString& ck = steps[i]->GetChartKey();
		if (!StepsByKey.count(ck)) {
			StepsByKey.emplace(ck, steps[i]);
			if (!SongsByKey.count(ck)) {
				SongsByKey.emplace(ck, new_song);
			}
		}
	}

	groupderps[new_song->m_sGroupName].emplace_back(new_song);
}

// Get a steps pointer given a chartkey, the assumption here is we want _a_ matching steps, not the original steps - mina
Steps* SongManager::GetStepsByChartkey(RString ck) {
	if (StepsByKey.count(ck))
		return StepsByKey[ck];
	return NULL;
}

Song* SongManager::GetSongByChartkey(RString ck) {
	if (SongsByKey.count(ck))
		return SongsByKey[ck];
	return NULL;
}

Steps* SongManager::GetStepsByChartkey(const StepsID& sid) {
	return GetStepsByChartkey(sid.GetKey());
}

Song* SongManager::GetSongByChartkey(const StepsID& sid) {
	return GetSongByChartkey(sid.GetKey());
}

static LocalizedString FOLDER_CONTAINS_MUSIC_FILES( "SongManager", "The folder \"%s\" appears to be a song folder.  All song folders must reside in a group folder.  For example, \"Songs/Originals/My Song\"." );
void SongManager::SanityCheckGroupDir( const RString &sDir ) const
{
	// Check to see if they put a song directly inside the group folder.
	vector<RString> arrayFiles;
	GetDirListing( sDir + "/*", arrayFiles );
	const vector<RString>& audio_exts= ActorUtil::GetTypeExtensionList(FT_Sound);
	FOREACH(RString, arrayFiles, fname)
	{
		const RString ext= GetExtension(*fname);
		FOREACH_CONST(RString, audio_exts, aud)
		{
			if(ext == *aud)
			{
				RageException::Throw(
					FOLDER_CONTAINS_MUSIC_FILES.GetValue(), sDir.c_str());
			}
		}
	}
}

void SongManager::AddGroup( const RString &sDir, const RString &sGroupDirName )
{
	unsigned j;
	for(j = 0; j < m_sSongGroupNames.size(); ++j)
		if( sGroupDirName == m_sSongGroupNames[j] )
			break;

	if( j != m_sSongGroupNames.size() )
		return; // the group is already added

	// Look for a group banner in this group folder
	vector<RString> arrayGroupBanners;
	
	FILEMAN->GetDirListingWithMultipleExtensions(sDir + sGroupDirName + "/", ActorUtil::GetTypeExtensionList(FT_Bitmap), arrayGroupBanners);

	RString sBannerPath;
	if( !arrayGroupBanners.empty() )
		sBannerPath = sDir+sGroupDirName+"/"+arrayGroupBanners[0] ;
	else
	{
		// Look for a group banner in the parent folder
		FILEMAN->GetDirListingWithMultipleExtensions(sDir + sGroupDirName, ActorUtil::GetTypeExtensionList(FT_Bitmap), arrayGroupBanners);
		if( !arrayGroupBanners.empty() )
			sBannerPath = sDir+arrayGroupBanners[0];
	}

	/* Other group graphics are a bit trickier, and usually don't exist.
	 * A themer has a few options, namely checking the aspect ratio and
	 * operating on it. -aj
	 * TODO: Once the files are implemented in Song, bring the extensions
	 * from there into here. -aj */
	// Group background

	//vector<RString> arrayGroupBackgrounds;
	//GetDirListing( sDir+sGroupDirName+"/*-bg.png", arrayGroupBanners );
	//GetDirListing( sDir+sGroupDirName+"/*-bg.jpg", arrayGroupBanners );
	//GetDirListing( sDir+sGroupDirName+"/*-bg.jpeg", arrayGroupBanners );
	//GetDirListing( sDir+sGroupDirName+"/*-bg.gif", arrayGroupBanners );
	//GetDirListing( sDir+sGroupDirName+"/*-bg.bmp", arrayGroupBanners );
/*
	RString sBackgroundPath;
	if( !arrayGroupBackgrounds.empty() )
		sBackgroundPath = sDir+sGroupDirName+"/"+arrayGroupBackgrounds[0];
	else
	{
		// Look for a group background in the parent folder
		GetDirListing( sDir+sGroupDirName+"-bg.png", arrayGroupBackgrounds );
		GetDirListing( sDir+sGroupDirName+"-bg.jpg", arrayGroupBackgrounds );
		GetDirListing( sDir+sGroupDirName+"-bg.jpeg", arrayGroupBackgrounds );
		GetDirListing( sDir+sGroupDirName+"-bg.gif", arrayGroupBackgrounds );
		GetDirListing( sDir+sGroupDirName+"-bg.bmp", arrayGroupBackgrounds );
		if( !arrayGroupBackgrounds.empty() )
			sBackgroundPath = sDir+arrayGroupBackgrounds[0];
	}
*/
	/*
	LOG->Trace( "Group banner for '%s' is '%s'.", sGroupDirName.c_str(), 
				sBannerPath != ""? sBannerPath.c_str():"(none)" );
	*/

	m_sSongGroupNames.emplace_back( sGroupDirName );
	m_sSongGroupBannerPaths.emplace_back( sBannerPath );
	//m_sSongGroupBackgroundPaths.emplace_back( sBackgroundPath );
}

static LocalizedString LOADING_SONGS ( "SongManager", "Loading songs..." );
void SongManager::LoadStepManiaSongDir( RString sDir, LoadingWindow *ld )
{
	// Compositors and other stuff can impose some overhead on updating the
	// loading window, which slows down startup time for some people.
	// loading_window_last_update_time provides a timer so the loading window
	// isn't updated after every song and course. -Kyz
	RageTimer loading_window_last_update_time;
	loading_window_last_update_time.Touch();
	// Make sure sDir has a trailing slash.
	if( sDir.Right(1) != "/" )
		sDir += "/";

	// Find all group directories in "Songs" folder
	vector<RString> arrayGroupDirs;
	GetDirListing( sDir+"*", arrayGroupDirs, true );
	StripCvsAndSvn( arrayGroupDirs );
	StripMacResourceForks( arrayGroupDirs );
	SortRStringArray(arrayGroupDirs);

	vector< vector<RString> > arrayGroupSongDirs;
	int groupIndex, songCount, songIndex;

	groupIndex = 0;
	songCount = 0;
	if(ld)
	{
		ld->SetIndeterminate(false);
		ld->SetTotalWork(arrayGroupDirs.size());
	}
	int sanity_index= 0;
	FOREACH_CONST( RString, arrayGroupDirs, s )	// foreach dir in /Songs/
	{
		RString sGroupDirName = *s;
		if(ld && loading_window_last_update_time.Ago() > next_loading_window_update)
		{
			loading_window_last_update_time.Touch();
			ld->SetProgress(sanity_index);
			ld->SetText(SANITY_CHECKING_GROUPS.GetValue() + ssprintf("\n%s",
					Basename(sGroupDirName).c_str()));
		}
		// TODO: If this check fails, log a warning instead of crashing.
		SanityCheckGroupDir(sDir+sGroupDirName);

		// Find all Song folders in this group directory
		vector<RString> arraySongDirs;
		GetDirListing( sDir+sGroupDirName + "/*", arraySongDirs, true, true );
		StripCvsAndSvn( arraySongDirs );
		StripMacResourceForks( arraySongDirs );
		SortRStringArray( arraySongDirs );

		arrayGroupSongDirs.emplace_back(arraySongDirs);
		songCount += arraySongDirs.size();

	}

	if( songCount==0 ) return;

	if( ld ) {
		ld->SetIndeterminate( false );
		ld->SetTotalWork( songCount );
	}

	groupIndex = 0;
	songIndex = 0;
	FOREACH_CONST( RString, arrayGroupDirs, s )	// foreach dir in /Songs/
	{
		RString sGroupDirName = *s;	
		vector<RString> &arraySongDirs = arrayGroupSongDirs[groupIndex++];

		LOG->Trace("Attempting to load %i songs from \"%s\"", static_cast<int>(arraySongDirs.size()),
				   (sDir+sGroupDirName).c_str() );
		int loaded = 0;

		SongPointerVector& index_entry = m_mapSongGroupIndex[sGroupDirName];
		RString group_base_name= Basename(sGroupDirName);
		for( unsigned j=0; j< arraySongDirs.size(); ++j )	// for each song dir
		{
			RString sSongDirName = arraySongDirs[j];

			// this is a song directory. Load a new song.
			if(ld && loading_window_last_update_time.Ago() > next_loading_window_update)
			{
				loading_window_last_update_time.Touch();
				ld->SetProgress(songIndex);
				ld->SetText( LOADING_SONGS.GetValue() +
					ssprintf("\n%s\n%s",
						group_base_name.c_str(),
						Basename(sSongDirName).c_str()
					)
				);
			}
			Song* pNewSong = new Song;
			if( !pNewSong->LoadFromSongDir( sSongDirName ) )
			{
				// The song failed to load.
				delete pNewSong;
				continue;
			}
			AddSongToList(pNewSong);
			AddKeyedPointers(pNewSong);

			index_entry.emplace_back( pNewSong );

			loaded++;
			songIndex++;
		}

		LOG->Trace("Loaded %i songs from \"%s\"", loaded, (sDir+sGroupDirName).c_str() );

		// Don't add the group name if we didn't load any songs in this group.
		if(!loaded) continue;

		// Add this group to the group array.
		AddGroup(sDir, sGroupDirName);

		// Cache and load the group banner. (and background if it has one -aj)
		BANNERCACHE->CacheBanner( GetSongGroupBannerPath(sGroupDirName) );
	}

	if( ld ) {
		ld->SetIndeterminate( true );
	}
}

void SongManager::PreloadSongImages()
{
	if( PREFSMAN->m_BannerCache != BNCACHE_FULL )
		return;

	/* Load textures before unloading old ones, so we don't reload textures
	 * that we don't need to. */
	RageTexturePreloader preload;

	const vector<Song*> &songs = GetAllSongs();
	for( unsigned i = 0; i < songs.size(); ++i )
	{
		if( !songs[i]->HasBanner() )
			continue;

		const RageTextureID ID = Sprite::SongBannerTexture( songs[i]->GetBannerPath() );
		preload.Load( ID );
	}
	preload.Swap( m_TexturePreload );
}

void SongManager::FreeSongs()
{
	m_sSongGroupNames.clear();
	m_sSongGroupBannerPaths.clear();
	//m_sSongGroupBackgroundPaths.clear();

	for( unsigned i=0; i<m_pSongs.size(); i++ )
		SAFE_DELETE( m_pSongs[i] );
	m_pSongs.clear();
	m_SongsByDir.clear();

	// also free the songs that have been deleted from disk
	for ( unsigned i=0; i<m_pDeletedSongs.size(); ++i ) 
		SAFE_DELETE( m_pDeletedSongs[i] );
	m_pDeletedSongs.clear();

	m_mapSongGroupIndex.clear();
	m_sSongGroupBannerPaths.clear();

	SongsByKey.clear();
	StepsByKey.clear();
	groupderps.clear();

	m_pPopularSongs.clear();
	m_pShuffledSongs.clear();
}

void SongManager::UnlistSong(Song *song)
{
	// cannot immediately free song data, as it is needed temporarily for smooth audio transitions, etc.
	// Instead, remove it from the m_pSongs list and store it in a special place where it can safely be deleted later.
	m_pDeletedSongs.emplace_back(song);

	// remove all occurences of the song in each of our song vectors
	vector<Song*>* songVectors[3] = { &m_pSongs, &m_pPopularSongs, &m_pShuffledSongs };
	for (int songVecIdx=0; songVecIdx<3; ++songVecIdx) {
		vector<Song*>& v = *songVectors[songVecIdx];
		for (size_t i=0; i<v.size(); ++i) {
			if (v[i] == song) {
				v.erase(v.begin()+i);
				--i;
			}
		}
	}
}

bool SongManager::IsGroupNeverCached(const RString& group) const
{
	return m_GroupsToNeverCache.find(group) != m_GroupsToNeverCache.end();
}

void SongManager::SetFavoritedStatus(set<string>& favs) {
	FOREACH(Song*, m_pSongs, song)
		FOREACH_CONST(Steps*, (*song)->GetAllSteps(), steps)
		if (favs.count((*steps)->GetChartKey()))
			(*song)->SetFavorited(true);
}

void SongManager::SetPermaMirroredStatus(set<string>& pmir) {
	FOREACH(Song*, m_pSongs, song)
		FOREACH_CONST(Steps*, (*song)->GetAllSteps(), steps)
		if (pmir.count((*steps)->GetChartKey()))
			(*song)->SetPermaMirror(true);
}

// hurr should probably redo both (all three) of these -mina
void SongManager::SetHasGoal(unordered_map<string, GoalsForChart>& goalmap) {
	FOREACH(Song*, m_pSongs, song)
		FOREACH_CONST(Steps*, (*song)->GetAllSteps(), steps)
			if(goalmap.count((*steps)->GetChartKey()))
					(*song)->SetHasGoal(true);
}

RString SongManager::GetSongGroupBannerPath( const RString &sSongGroup ) const
{
	for( unsigned i = 0; i < m_sSongGroupNames.size(); ++i )
	{
		if( sSongGroup == m_sSongGroupNames[i] ) 
			return m_sSongGroupBannerPaths[i];
	}

	return RString();
}
/*
RString SongManager::GetSongGroupBackgroundPath( RString sSongGroup ) const
{
	for( unsigned i = 0; i < m_sSongGroupNames.size(); ++i )
	{
		if( sSongGroup == m_sSongGroupNames[i] ) 
			return m_sSongGroupBackgroundPaths[i];
	}

	return RString();
}
*/
void SongManager::GetSongGroupNames( vector<RString> &AddTo ) const
{
	AddTo.insert(AddTo.end(), m_sSongGroupNames.begin(), m_sSongGroupNames.end() );
}

const vector<RString>& SongManager::GetSongGroupNames() const
{
	return m_sSongGroupNames;
}

bool SongManager::DoesSongGroupExist( const RString &sSongGroup ) const
{
	return find( m_sSongGroupNames.begin(), m_sSongGroupNames.end(), sSongGroup ) != m_sSongGroupNames.end();
}

RageColor SongManager::GetSongGroupColor( const RString &sSongGroup ) const
{
	for( unsigned i=0; i<m_sSongGroupNames.size(); i++ )
	{
		if( m_sSongGroupNames[i] == sSongGroup || allplaylists.count(sSongGroup))
		{
			return SONG_GROUP_COLOR.GetValue( i%NUM_SONG_GROUP_COLORS );
		}
	}

	ASSERT_M( 0, ssprintf("requested color for song group '%s' that doesn't exist",sSongGroup.c_str()) );
	return RageColor(1,1,1,1);
}

RageColor SongManager::GetSongColor( const Song* pSong ) const
{
	ASSERT( pSong != NULL );
	if( USE_PREFERRED_SORT_COLOR )
	{
		FOREACH_CONST( PreferredSortSection, m_vPreferredSongSort, v )
		{
			FOREACH_CONST( Song*, v->vpSongs, s )
			{
				if( *s == pSong )
				{
					int i = v - m_vPreferredSongSort.begin();
					return SONG_GROUP_COLOR.GetValue( i%NUM_SONG_GROUP_COLORS );
				}
			}
		}

		int i = m_vPreferredSongSort.size();
		return SONG_GROUP_COLOR.GetValue( i%NUM_SONG_GROUP_COLORS );
	}
	else // TODO: Have a better fallback plan with colors?
	{
		/* XXX: Previously, this matched all notes, which set a song to "extra"
		 * if it had any 10-foot steps at all, even edits or doubles.
		 *
		 * For now, only look at notes for the current note type. This means
		 * that if a song has 10-foot steps on Doubles, it'll only show up red
		 * in Doubles. That's not too bad, I think. This will also change it
		 * in the song scroll, which is a little odd but harmless. 
		 *
		 * XXX: Ack. This means this function can only be called when we have
		 * a style set up, which is too restrictive. How to handle this? */
		//const StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;
		const vector<Steps*>& vpSteps = pSong->GetAllSteps();
		for( unsigned i=0; i<vpSteps.size(); i++ )
		{
			const Steps* pSteps = vpSteps[i];
			switch( pSteps->GetDifficulty() )
			{
				case Difficulty_Challenge:
				case Difficulty_Edit:
					continue;
				default: break;
			}

			//if(pSteps->m_StepsType != st)
			//	continue;

			if( pSteps->GetMeter() >= EXTRA_COLOR_METER )
				return (RageColor)EXTRA_COLOR;
		}

		return GetSongGroupColor( pSong->m_sGroupName );
	}
}

void SongManager::ResetGroupColors()
{
	// Reload song/course group colors to prevent a crash when switching
	// themes in-game. (apparently not, though.) -aj
	SONG_GROUP_COLOR.Clear();

	NUM_SONG_GROUP_COLORS	.Load( "SongManager", "NumSongGroupColors" );
	SONG_GROUP_COLOR		.Load( "SongManager", SONG_GROUP_COLOR_NAME, NUM_SONG_GROUP_COLORS );
}

const vector<Song*> &SongManager::GetSongs( const RString &sGroupName ) const
{
	static const vector<Song*> vEmpty;

	if( sGroupName == GROUP_ALL )
		return m_pSongs;
	map<RString, SongPointerVector, Comp>::const_iterator iter = m_mapSongGroupIndex.find( sGroupName );
	if ( iter != m_mapSongGroupIndex.end() )
		return iter->second;
	return vEmpty;
}

void SongManager::GetFavoriteSongs(vector<Song*>& songs) const {
	FOREACH_CONST(Song*, m_pSongs, song) {
		if ((*song)->IsFavorited())
			songs.emplace_back((*song));
	}
}

void SongManager::GetPreferredSortSongs( vector<Song*> &AddTo ) const
{
	if( m_vPreferredSongSort.empty() )
	{
		AddTo.insert( AddTo.end(), m_pSongs.begin(), m_pSongs.end() );
		return;
	}

	FOREACH_CONST( PreferredSortSection, m_vPreferredSongSort, v )
		AddTo.insert( AddTo.end(), v->vpSongs.begin(), v->vpSongs.end() );
}

RString SongManager::SongToPreferredSortSectionName( const Song *pSong ) const
{
	FOREACH_CONST( PreferredSortSection, m_vPreferredSongSort, v )
	{
		FOREACH_CONST( Song*, v->vpSongs, s )
		{
			if( *s == pSong )
				return v->sName;
		}
	}
	return RString();
}

int SongManager::GetNumSongs() const
{
	return m_pSongs.size();
}

int SongManager::GetNumAdditionalSongs() const
{
	int iNum = 0;
	FOREACH_CONST( Song*, m_pSongs, i )
	{
		if( WasLoadedFromAdditionalSongs( *i ) )
			++iNum;
	}
	return iNum;
}

int SongManager::GetNumSongGroups() const
{
	return m_sSongGroupNames.size();
}

RString SongManager::ShortenGroupName( const RString &sLongGroupName )
{
	static TitleSubst tsub("Groups");

	TitleFields title;
	title.Title = sLongGroupName;
	tsub.Subst( title );
	return title.Title;
}

/* Called periodically to wipe out cached NoteData. This is called when we
 * change screens. */
void SongManager::Cleanup()
{
	for( unsigned i=0; i<m_pSongs.size(); i++ )
	{
		Song* pSong = m_pSongs[i];
		if (pSong)
		{
			const vector<Steps*>& vpSteps = pSong->GetAllSteps();
			for( unsigned n=0; n<vpSteps.size(); n++ )
			{
				Steps* pSteps = vpSteps[n];
				pSteps->Compress();
			}
		}
	}
}

/* Flush all Song*, Steps* and Course* caches. This is when a Song or its Steps
 * are removed or changed. This doesn't touch GAMESTATE and StageStats
 * pointers. Currently, the only time Steps are altered independently of the
 * Courses and Songs is in Edit Mode, which updates the other pointers it needs. */
void SongManager::Invalidate( const Song *pStaleSong )
{
	UpdatePopular();
	UpdateShuffled();
}

void SongManager::SetPreferences()
{
}

void SongManager::SaveEnabledSongsToPref()
{
	vector<RString> vsDisabledSongs;

	// Intentionally drop disabled song entries for songs that aren't currently loaded.

	const vector<Song*> &apSongs = SONGMAN->GetAllSongs();
	FOREACH_CONST( Song *, apSongs, s )
	{
		Song *pSong = (*s);
		SongID sid;
		sid.FromSong( pSong );
		if( !pSong->GetEnabled() )
			vsDisabledSongs.emplace_back( sid.ToString() );
	}
	g_sDisabledSongs.Set( join(";", vsDisabledSongs) );
}

void SongManager::LoadEnabledSongsFromPref()
{
	vector<RString> asDisabledSongs;
	split( g_sDisabledSongs, ";", asDisabledSongs, true );

	FOREACH_CONST( RString, asDisabledSongs, s )
	{
		SongID sid;
		sid.FromString( *s );
		Song *pSong = sid.ToSong();
		if( pSong )
			pSong->SetEnabled( false );
	}
}

void SongManager::GetStepsLoadedFromProfile( vector<Steps*> &AddTo, ProfileSlot slot ) const
{
	const vector<Song*> &vSongs = GetAllSongs();
	FOREACH_CONST( Song*, vSongs, song )
	{
		(*song)->GetStepsLoadedFromProfile( slot, AddTo );
	}
}

void SongManager::DeleteSteps( Steps *pSteps )
{
	pSteps->m_pSong->DeleteSteps( pSteps );
}

bool SongManager::WasLoadedFromAdditionalSongs( const Song *pSong ) const
{
	RString sDir = pSong->GetSongDir();
	return BeginsWith( sDir, ADDITIONAL_SONGS_DIR );
}

// Return true if n1 < n2.
bool CompareNotesPointersForExtra(const Steps *n1, const Steps *n2)
{
	// Equate CHALLENGE to HARD.
	Difficulty d1 = min(n1->GetDifficulty(), Difficulty_Hard);
	Difficulty d2 = min(n2->GetDifficulty(), Difficulty_Hard);

	if(d1 < d2) return true;
	if(d1 > d2) return false;
	// n1 difficulty == n2 difficulty 

	if(StepsUtil::CompareNotesPointersByMeter(n1,n2)) return true;
	if(StepsUtil::CompareNotesPointersByMeter(n2,n1)) return false;
	// n1 meter == n2 meter

	return StepsUtil::CompareNotesPointersByRadarValues(n1,n2);
}

void SongManager::GetExtraStageInfo( bool bExtra2, const Style *sd, Song*& pSongOut, Steps*& pStepsOut )
{
	RString sGroup = GAMESTATE->m_sPreferredSongGroup;
	if( sGroup == GROUP_ALL )
	{
		if( GAMESTATE->m_pCurSong == NULL )
		{
			// This normally shouldn't happen, but it's helpful to permit it for testing.
			LuaHelpers::ReportScriptErrorFmt( "GetExtraStageInfo() called in GROUP_ALL, but GAMESTATE->m_pCurSong == NULL" );
			GAMESTATE->m_pCurSong.Set( GetRandomSong() );
		}
		sGroup = GAMESTATE->m_pCurSong->m_sGroupName;
	}

	ASSERT_M( sGroup != "", ssprintf("%p '%s' '%s'",
		GAMESTATE->m_pCurSong.Get(),
		GAMESTATE->m_pCurSong? GAMESTATE->m_pCurSong->GetSongDir().c_str():"",
		GAMESTATE->m_pCurSong? GAMESTATE->m_pCurSong->m_sGroupName.c_str():"") );

	// Choose a hard song for the extra stage
	Song*	pExtra1Song = NULL;		// the absolute hardest Song and Steps.  Use this for extra stage 1.
	Steps*	pExtra1Notes = NULL;
	Song*	pExtra2Song = NULL;		// a medium-hard Song and Steps.  Use this for extra stage 2.
	Steps*	pExtra2Notes = NULL;
	
	const vector<Song*> &apSongs = GetSongs( sGroup );
	for( unsigned s=0; s<apSongs.size(); s++ )	// foreach song
	{
		Song* pSong = apSongs[s];

		vector<Steps*> apSteps;
		SongUtil::GetSteps( pSong, apSteps, sd->m_StepsType );
		for( unsigned n=0; n<apSteps.size(); n++ )	// foreach Steps
		{
			Steps* pSteps = apSteps[n];

			if( pExtra1Notes == NULL || CompareNotesPointersForExtra(pExtra1Notes,pSteps) )	// pSteps is harder than pHardestNotes
			{
				pExtra1Song = pSong;
				pExtra1Notes = pSteps;
			}

			// for extra 2, we don't want to choose the hardest notes possible.  So, we'll disgard Steps with meter > 8 (assuming dance)
			if( bExtra2 && pSteps->GetMeter() > EXTRA_STAGE2_DIFFICULTY_MAX )	
				continue;	// skip
			if( pExtra2Notes == NULL  ||  CompareNotesPointersForExtra(pExtra2Notes,pSteps) )	// pSteps is harder than pHardestNotes
			{
				pExtra2Song = pSong;
				pExtra2Notes = pSteps;
			}
		}
	}

	if( pExtra2Song == NULL  &&  pExtra1Song != NULL )
	{
		pExtra2Song = pExtra1Song;
		pExtra2Notes = pExtra1Notes;
	}

	// If there are any notes at all that match this StepsType, everything should be filled out.
	// Also, it's guaranteed that there is at least one Steps that matches the StepsType because the player
	// had to play something before reaching the extra stage!
	ASSERT( pExtra2Song && pExtra1Song && pExtra2Notes && pExtra1Notes );

	pSongOut = (bExtra2 ? pExtra2Song : pExtra1Song);
	pStepsOut = (bExtra2 ? pExtra2Notes : pExtra1Notes);
}

Song* SongManager::GetRandomSong()
{
	if( m_pShuffledSongs.empty() )
		return NULL;

	static int i = 0;

	for( int iThrowAway=0; iThrowAway<100; iThrowAway++ )
	{
		i++;
		wrap( i, m_pShuffledSongs.size() );
		Song *pSong = m_pShuffledSongs[ i ];
		if( pSong->IsTutorial() )
			continue;
		return pSong;
	}

	return NULL;
}


Song* SongManager::GetSongFromDir(RString dir) const
{
	if(dir.Right(1) != "/")
	{ dir += "/"; }

	dir.Replace('\\', '/');
	dir.MakeLower();
	map<RString, Song*>::const_iterator entry= m_SongsByDir.find(dir);
	if(entry != m_SongsByDir.end())
	{
		return entry->second;
	}
	return NULL;
}

/* GetSongDir() contains a path to the song, possibly a full path, eg:
 * Songs\Group\SongName                   or 
 * My Other Song Folder\Group\SongName    or
 * c:\Corny J-pop\Group\SongName
 *
 * Most course group names are "Group\SongName", so we want to match against the
 * last two elements. Let's also support "SongName" alone, since the group is
 * only important when it's potentially ambiguous.
 *
 * Let's *not* support "Songs\Group\SongName" in course files. That's probably
 * a common error, but that would result in course files floating around that
 * only work for people who put songs in "Songs"; we don't want that. */

Song *SongManager::FindSong( RString sPath ) const
{
	sPath.Replace( '\\', '/' );
	vector<RString> bits;
	split( sPath, "/", bits );

	if( bits.size() == 1 )
		return FindSong( "", bits[0] );
	else if( bits.size() == 2 )
		return FindSong( bits[0], bits[1] );

	return NULL;
}

Song *SongManager::FindSong( RString sGroup, RString sSong ) const
{
	// foreach song
	const vector<Song *> &vSongs = GetSongs( sGroup.empty()? GROUP_ALL:sGroup );
	FOREACH_CONST( Song*, vSongs, s )
	{
		if( (*s)->Matches(sGroup, sSong) )
			return *s;
	}

	return NULL;
}

void SongManager::UpdatePopular()
{
	// update players best
	vector<Song*> apBestSongs = m_pSongs;
	SongUtil::SortSongPointerArrayByTitle( apBestSongs );
}

void SongManager::UpdateShuffled()
{
	// update shuffled
	m_pShuffledSongs = m_pSongs;
	std::shuffle( m_pShuffledSongs.begin(), m_pShuffledSongs.end(), g_RandomNumberGenerator );
}

void SongManager::UpdatePreferredSort(const RString &sPreferredSongs, const RString &sPreferredCourses)
{
	{
		m_vPreferredSongSort.clear();

		vector<RString> asLines;
		RString sFile = THEME->GetPathO( "SongManager", sPreferredSongs );
		GetFileContents( sFile, asLines );
		if( asLines.empty() )
			return;

		PreferredSortSection section;
		map<Song *, float> mapSongToPri;

		FOREACH( RString, asLines, s )
		{
			RString sLine = *s;

			bool bSectionDivider = BeginsWith(sLine, "---");
			if( bSectionDivider )
			{
				if( !section.vpSongs.empty() )
				{
					m_vPreferredSongSort.emplace_back( section );
					section = PreferredSortSection();
				}

				section.sName = sLine.Right( sLine.length() - RString("---").length() );
				TrimLeft( section.sName );
				TrimRight( section.sName );
			}
			else
			{
				/* if the line ends in slash-star, check if the section exists,
				 * and if it does, add all the songs in that group to the list. */
				if( EndsWith(sLine,"/*") )
				{
					RString group = sLine.Left( sLine.length() - RString("/*").length() );
					if( DoesSongGroupExist(group) )
					{
						// add all songs in group
						const vector<Song *> &vSongs = GetSongs( group );
						FOREACH_CONST( Song*, vSongs, song )
						{
							section.vpSongs.emplace_back( *song );
						}
					}
				}

				Song *pSong = FindSong( sLine );
				if( pSong == NULL )
					continue;
				section.vpSongs.emplace_back( pSong );
			}
		}

		if( !section.vpSongs.empty() )
		{
			m_vPreferredSongSort.emplace_back( section );
			section = PreferredSortSection();
		}

		// prune empty groups
		for( int i=m_vPreferredSongSort.size()-1; i>=0; i-- )
			if( m_vPreferredSongSort[i].vpSongs.empty() )
				m_vPreferredSongSort.erase( m_vPreferredSongSort.begin()+i );

		FOREACH( PreferredSortSection, m_vPreferredSongSort, i )
			FOREACH( Song*, i->vpSongs, j )
				ASSERT( *j != NULL );
	}
}

void SongManager::SortSongs()
{
	SongUtil::SortSongPointerArrayByTitle( m_pSongs );
}

void SongManager::AddSongToList(Song* new_song)
{
	new_song->SetEnabled(true);
	m_pSongs.emplace_back(new_song);
	RString dir= new_song->GetSongDir();
	dir.MakeLower();
	m_SongsByDir.insert(make_pair(dir, new_song));
}

void SongManager::FreeAllLoadedFromProfile( ProfileSlot slot )
{
	// Popular and Shuffled may refer to courses that we just freed.
	UpdatePopular();
	UpdateShuffled();

	// Free profile steps.
	set<Steps*> setInUse;
	if( STATSMAN )
		STATSMAN->GetStepsInUse( setInUse );
	FOREACH( Song*, m_pSongs, s )
		(*s)->FreeAllLoadedFromProfile( slot, &setInUse );
}

int SongManager::GetNumStepsLoadedFromProfile()
{
	int iCount = 0;
	FOREACH( Song*, m_pSongs, s )
	{
		vector<Steps*> vpAllSteps = (*s)->GetAllSteps();

		FOREACH( Steps*, vpAllSteps, ss )
		{
			if( (*ss)->GetLoadedFromProfileSlot() != ProfileSlot_Invalid )
				iCount++;
		}
	}

	return iCount;
}

int SongManager::GetSongRank(Song* pSong)
{
	const int index = FindIndex( m_pPopularSongs.begin(), m_pPopularSongs.end(), pSong );
	return index; // -1 means we didn't find it
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the SongManager. */ 
class LunaSongManager: public Luna<SongManager>
{
public:
	static int SetPreferredSongs( T* p, lua_State *L )
	{
		p->UpdatePreferredSort( SArg(1), "PreferredCourses.txt" );
		COMMON_RETURN_SELF;
	}
	static int GetAllSongs( T* p, lua_State *L )
	{
		const vector<Song*> &v = p->GetAllSongs();
		LuaHelpers::CreateTableFromArray<Song*>( v, L );
		return 1;
	}
	static int GetPreferredSortSongs( T* p, lua_State *L )
	{
		vector<Song*> v;
		p->GetPreferredSortSongs(v);
		LuaHelpers::CreateTableFromArray<Song*>( v, L );
		return 1;
	}

	static int FindSong( T* p, lua_State *L )		{ Song *pS = p->FindSong(SArg(1)); if(pS) pS->PushSelf(L); else lua_pushnil(L); return 1; }
	static int GetRandomSong( T* p, lua_State *L )		{ Song *pS = p->GetRandomSong(); if(pS) pS->PushSelf(L); else lua_pushnil(L); return 1; }
	static int GetNumSongs( T* p, lua_State *L )		{ lua_pushnumber( L, p->GetNumSongs() ); return 1; }
	static int GetNumAdditionalSongs( T* p, lua_State *L )  { lua_pushnumber( L, p->GetNumAdditionalSongs() ); return 1; }
	static int GetNumSongGroups( T* p, lua_State *L )	{ lua_pushnumber( L, p->GetNumSongGroups() ); return 1; }
	/* Note: this could now be implemented as Luna<Steps>::GetSong */
	static int GetSongFromSteps( T* p, lua_State *L )
	{
		Song *pSong = NULL;
		if( lua_isnil(L,1) ) { pSong = NULL; }
		else { Steps *pSteps = Luna<Steps>::check(L,1); pSong = pSteps->m_pSong; }
		if(pSong) pSong->PushSelf(L);
		else lua_pushnil(L);
		return 1;
	}

	static int GetExtraStageInfo( T* p, lua_State *L )
	{
		bool bExtra2 = BArg( 1 );
		const Style *pStyle = Luna<Style>::check( L, 2 );
		Song *pSong;
		Steps *pSteps;

		p->GetExtraStageInfo( bExtra2, pStyle, pSong, pSteps );
		pSong->PushSelf( L );
		pSteps->PushSelf( L );

		return 2;
	}
	DEFINE_METHOD( GetSongColor, GetSongColor( Luna<Song>::check(L,1) ) )
	DEFINE_METHOD( GetSongGroupColor, GetSongGroupColor( SArg(1) ) )

	static int GetSongRank( T* p, lua_State *L )
	{
		Song *pSong = Luna<Song>::check(L,1);
		int index = p->GetSongRank(pSong);
		if( index != -1 )
			lua_pushnumber(L, index+1);
		else
			lua_pushnil(L);
		return 1;
	}
	/*
	static int GetSongRankFromProfile( T* p, lua_State *L )
	{
		// it's like the above but also takes in a ProfileSlot as well.
	}
	*/

	static int GetSongGroupNames( T* p, lua_State *L )
	{
		vector<RString> v;
		p->GetSongGroupNames( v );
		LuaHelpers::CreateTableFromArray<RString>( v, L );
		return 1;
	}

	static int GetSongsInGroup( T* p, lua_State *L )
	{
		vector<Song*> v = p->GetSongs(SArg(1));
		LuaHelpers::CreateTableFromArray<Song*>( v, L );
		return 1;
	}

	DEFINE_METHOD( ShortenGroupName, ShortenGroupName( SArg(1) ) )
	DEFINE_METHOD( GetSongGroupBannerPath, GetSongGroupBannerPath(SArg(1)) );
	DEFINE_METHOD( DoesSongGroupExist, DoesSongGroupExist(SArg(1)) );
	DEFINE_METHOD(IsChartLoaded, IsChartLoaded(SArg(1)));

	static int GetPopularSongs( T* p, lua_State *L )
	{
		const vector<Song*> &v = p->GetPopularSongs();
		LuaHelpers::CreateTableFromArray<Song*>( v, L );
		return 1;
	}
	static int SongToPreferredSortSectionName( T* p, lua_State *L )
	{
		const Song* pSong = Luna<Song>::check(L,1);
		lua_pushstring(L, p->SongToPreferredSortSectionName(pSong));
		return 1;
	}
	static int WasLoadedFromAdditionalSongs( T* p, lua_State *L )
	{
		const Song* pSong = Luna<Song>::check(L,1);
		lua_pushboolean(L, p->WasLoadedFromAdditionalSongs(pSong));
		return 1;
	}
	static int GetSongByChartKey(T* p, lua_State *L)
	{
		RString ck = SArg(1);
		Song* pSong = p->GetSongByChartkey(ck);
		if (pSong)
			pSong->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetStepsByChartKey(T* p, lua_State *L)
	{
		RString ck = SArg(1);
		Steps* pSteps = p->GetStepsByChartkey(ck);
		if (pSteps)
			pSteps->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	static int GetNumCourses(T* p, lua_State *L)
	{
		lua_pushnumber(L, 0);
		return 1;
	}

	static int GetActivePlaylist(T* p, lua_State *L)
	{
		p->allplaylists[p->activeplaylist].PushSelf(L);
		return 1;
	}

	static int SetActivePlaylist(T* p, lua_State *L)
	{
		p->activeplaylist= SArg(1);
		return 1;
	}

	static int NewPlaylist(T* p, lua_State *L)
	{
		ScreenTextEntry::TextEntry(SM_BackFromNamePlaylist, "Name Playlist", "", 128);
		return 1;
	}

	static int GetPlaylists(T* p, lua_State *L)
	{
		int idx = 1;
		lua_newtable(L);
		FOREACHM(string, Playlist, p->allplaylists, pl) {
			pl->second.PushSelf(L);
			lua_rawseti(L, -2, idx);
			++idx;
		}

		return 1;
	}

	static int DeletePlaylist(T* p, lua_State *L)
	{
		p->DeletePlaylist(SArg(1));
		return 1;
	}


	LunaSongManager()
	{
		ADD_METHOD( GetAllSongs );
		ADD_METHOD( FindSong );
		ADD_METHOD( GetRandomSong );
		ADD_METHOD( GetNumSongs );
		ADD_METHOD( GetNumAdditionalSongs );
		ADD_METHOD( GetNumSongGroups );
		ADD_METHOD( GetSongFromSteps );
		ADD_METHOD( GetExtraStageInfo );
		ADD_METHOD( GetSongColor );
		ADD_METHOD( GetSongGroupColor );
		ADD_METHOD( GetSongRank );
		ADD_METHOD( GetSongGroupNames );
		ADD_METHOD( GetSongsInGroup );
		ADD_METHOD( ShortenGroupName );
		ADD_METHOD( SetPreferredSongs );
		ADD_METHOD( GetPreferredSortSongs );
		ADD_METHOD( GetSongGroupBannerPath );
		ADD_METHOD( DoesSongGroupExist );
		ADD_METHOD( GetPopularSongs );
		ADD_METHOD( SongToPreferredSortSectionName );
		ADD_METHOD( WasLoadedFromAdditionalSongs );
		ADD_METHOD( GetSongByChartKey );
		ADD_METHOD( GetStepsByChartKey );
		ADD_METHOD(GetNumCourses);
		ADD_METHOD(GetActivePlaylist);
		ADD_METHOD(SetActivePlaylist);
		ADD_METHOD(NewPlaylist);
		ADD_METHOD(GetPlaylists);
		ADD_METHOD(DeletePlaylist);
	}
};

LUA_REGISTER_CLASS( SongManager )


class LunaPlaylist : public Luna<Playlist>
{
public:
	static int GetChartlist(T* p, lua_State *L) 
	{
		vector<string> keys = p->GetKeys();
		LuaHelpers::CreateTableFromArray(keys, L);
		return 1;
	}

	static int GetChartlistActual(T* p, lua_State *L)
	{
		lua_newtable(L);
		for (size_t i = 0; i < p->chartlist.size(); ++i)
		{
			p->chartlist[i].PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static int GetSonglist(T* p, lua_State *L)
	{
		lua_newtable(L);
		for (size_t i = 0; i < p->chartlist.size(); ++i)
		{
			p->chartlist[i].songptr->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	
	static int GetStepslist(T* p, lua_State *L) 
	{
		lua_newtable(L);
		for (size_t i = 0; i < p->chartlist.size(); ++i)
		{
			p->chartlist[i].stepsptr->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static int AddChart(T* p, lua_State *L)
	{
		p->AddChart(SArg(1));
		return 1;
	}

	static int DeleteChart(T* p, lua_State *L)
	{
		p->DeleteChart(IArg(1) - 1);
		return 1;
	}


	DEFINE_METHOD(GetAverageRating, GetAverageRating());
	DEFINE_METHOD(GetName, GetName());
	DEFINE_METHOD(GetNumCharts, GetNumCharts())
	LunaPlaylist()
	{
		ADD_METHOD(AddChart);
		ADD_METHOD(GetChartlist);
		ADD_METHOD(GetChartlistActual);
		ADD_METHOD(GetNumCharts);
		ADD_METHOD(GetName);
		ADD_METHOD(GetSonglist);
		ADD_METHOD(GetStepslist);
		ADD_METHOD(GetAverageRating);
		ADD_METHOD(DeleteChart);
	}
};

LUA_REGISTER_CLASS(Playlist)


class LunaChart : public Luna<Chart>
{
public:
	DEFINE_METHOD(GetRate, rate);
	DEFINE_METHOD(IsLoaded, IsLoaded());
	DEFINE_METHOD(GetDifficulty, lastdiff);
	DEFINE_METHOD(GetSongTitle, lastsong);
	DEFINE_METHOD(GetPackName, lastpack);

	static int ChangeRate(T* p, lua_State *L)
	{
		p->rate += FArg(1);
		CLAMP(p->rate, 0.7f, 3.f);
		return 1;
	}

	LunaChart()
	{
		ADD_METHOD(GetRate);
		ADD_METHOD(ChangeRate);
		ADD_METHOD(GetDifficulty);
		ADD_METHOD(GetSongTitle);
		ADD_METHOD(GetPackName);
		ADD_METHOD(IsLoaded);
	}
};

LUA_REGISTER_CLASS(Chart)
// lua end

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
