#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "AnnouncerManager.h"
#include "Etterna/Models/Misc/ImageCache.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "NoteSkinManager.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderDWI.h"
#include "PrefsManager.h"
#include "Etterna/Models/Misc/Profile.h"
#include "ProfileManager.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "Etterna/Screen/Others/ScreenTextEntry.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Songs/SongCacheIndex.h"
#include "SongManager.h"
#include "Etterna/Models/Songs/SongUtil.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/Misc/TitleSubstitution.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "ScreenManager.h"
#include "NetworkSyncManager.h"
#include "Etterna/MinaCalc/MinaCalc.h"
#include "Etterna/FileTypes/XmlFileUtil.h"

#include <numeric>
#include <algorithm>
#include <mutex>
#include <utility>

using std::map;
using std::string;
using std::vector;

typedef std::string SongDir;
struct Group
{
	std::string name;
	vector<SongDir> songs;
	Group(std::string name)
	  : name(std::move(std::move(name)))
	{
	}
};

const std::string ADDITIONAL_SONGS_DIR = "/AdditionalSongs/";
const std::string EDIT_SUBDIR = "Edits/";

static const ThemeMetric<RageColor> EXTRA_COLOR("SongManager", "ExtraColor");
static const ThemeMetric<int> EXTRA_COLOR_METER("SongManager",
												"ExtraColorMeter");
static const ThemeMetric<bool> USE_PREFERRED_SORT_COLOR(
  "SongManager",
  "UsePreferredSortColor");
static const ThemeMetric<int> EXTRA_STAGE2_DIFFICULTY_MAX(
  "SongManager",
  "ExtraStage2DifficultyMax");

static Preference<std::string> g_sDisabledSongs("DisabledSongs", "");
static Preference<bool> PlaylistsAreSongGroups("PlaylistsAreSongGroups", false);

auto
SONG_GROUP_COLOR_NAME(size_t i) -> std::string
{
	return ssprintf("SongGroupColor%i", static_cast<int>(i) + 1);
}

AutoScreenMessage(SM_BackFromNamePlaylist);

namespace SONGMAN {
namespace { // Anonymous namespace (For internally-used functions and variables)
// Foward Function Declarations:
void
LoadEnabledSongsFromPref();
void
LoadStepManiaSongDir(std::string sDir, LoadingWindow* ld);
auto
IsSongDir(const std::string& sDir) -> bool;
auto
AddGroup(const std::string& sDir, const std::string& sGroupDirName) -> bool;
void
AddSongToList(Song* new_song);
// Indexed by chartkeys
void
AddKeyedPointers(Song* new_song);
void
InitSongsFromDisk(LoadingWindow* ld);
void
FreeSongs();

/** @brief All of the songs that can be played. */
std::vector<Song*> m_pSongs;
std::map<std::string, Song*> m_SongsByDir;

std::vector<std::pair<std::pair<std::string, unsigned int>, Song*>*> cache;

struct Comp
{
	auto operator()(const std::string& s, const std::string& t) const -> bool
	{
		return CompareStringsAsc(s, t);
	}
};
using SongPointerVector = std::vector<Song*>;
std::map<std::string, SongPointerVector, Comp> m_mapSongGroupIndex;
ThemeMetric<int> NUM_SONG_GROUP_COLORS;
ThemeMetric1D<RageColor> SONG_GROUP_COLOR;
} // End anonymous namespace

// Extern Variables:
std::unordered_map<std::string, Song*> SongsByKey;
std::unordered_map<std::string, Steps*> StepsByKey;

std::set<std::string> m_GroupsToNeverCache;
/** @brief The most popular songs ranked by number of plays. */
std::vector<Song*> m_pPopularSongs;

std::vector<std::string> m_sSongGroupNames;
std::vector<std::string> m_sSongGroupBannerPaths; // each song group may have a
												  // banner associated with it
												  // TODO: INTERNAL
std::string activeplaylist = "";
std::string playlistcourse = "";

std::map<std::string, std::vector<Song*>> groupderps;
std::vector<std::string> playlistGroups; // To delete from groupderps when
										 // rebuilding
										 // playlist groups

std::map<Skillset, CalcTestList> testChartList;
std::unique_ptr<Calc> calc;

void
Init()
{
	NUM_SONG_GROUP_COLORS.Load("SongManager", "NumSongGroupColors");
	SONG_GROUP_COLOR.Load(
	  "SongManager", SONG_GROUP_COLOR_NAME, NUM_SONG_GROUP_COLORS);

	// calc for debug/session scores
	calc = std::make_unique<Calc>();
}

void
End()
{
	FreeSongs();
}

void
InitAll(LoadingWindow* ld)
{
	vector<std::string> never_cache;
	split(PREFSMAN->m_NeverCacheList, ",", never_cache);
	for (auto& group : never_cache) {
		m_GroupsToNeverCache.insert(group);
	}
	InitSongsFromDisk(ld);
	LoadCalcTestNode();
}

static LocalizedString RELOADING("SongManager", "Reloading...");
static LocalizedString UNLOADING_SONGS("SongManager", "Unloading songs...");
static LocalizedString SANITY_CHECKING_GROUPS("SongManager",
											  "Sanity checking groups...");

// See InitSongsFromDisk for any comment clarification -mina
auto
DifferentialReload() -> int
{
	MESSAGEMAN->Broadcast("DFRStarted");
	FILEMAN->FlushDirCache(SpecialFiles::SONGS_DIR);
	FILEMAN->FlushDirCache(ADDITIONAL_SONGS_DIR);
	FILEMAN->FlushDirCache(EDIT_SUBDIR);
	auto newsongs = 0;
	newsongs += DifferentialReloadDir(SpecialFiles::SONGS_DIR);
	newsongs += DifferentialReloadDir(ADDITIONAL_SONGS_DIR);
	LoadEnabledSongsFromPref();

	Message msg("DFRFinished");
	msg.SetParam("newsongs", newsongs);
	MESSAGEMAN->Broadcast(msg);
	return newsongs;
}

// See LoadStepManiaSongDir for any comment clarification -mina
auto
DifferentialReloadDir(string dir) -> int
{
	if (dir.back() != '/') {
		dir += "/";
	}

	auto newsongs = 0;

	vector<std::string> folders;
	GetDirListing(dir + "*", folders, true);

	vector<Group> groups;
	Group unknownGroup("Unknown Group");
	int groupIndex;
	int songCount;
	int songIndex;

	groupIndex = 0;
	songCount = 0;
	for (const auto& folder : folders) {
		if (IsSongDir(dir + folder)) {
			songCount++;
			unknownGroup.songs.emplace_back(SongDir(folder));
		} else {
			vector<std::string> songdirs;
			GetDirListing(dir + folder + "/*", songdirs, true, true);
			Group group(folder);
			for (auto& song : songdirs) {
				group.songs.emplace_back(SongDir(song));
			}

			groups.emplace_back(group);
			songCount += group.songs.size();
		}
	}
	if (!unknownGroup.songs.empty()) {
		groups.emplace_back(unknownGroup);
		songCount += unknownGroup.songs.size();
	}

	if (songCount == 0) {
		return 0;
	}

	groupIndex = 0;
	songIndex = 0;

	for (auto& group : groups) {
		auto& songDirs = group.songs;
		auto loaded = 0;

		SongPointerVector& index_entry = m_mapSongGroupIndex[group.name];
		std::string group_base_name = Basename(group.name);
		for (auto& songDir : songDirs) {
			// skip any dir we've already loaded -mina
			std::string hur = make_lower(songDir + "/");
			if (m_SongsByDir.count(hur) != 0u) {
				continue;
			}

			auto pNewSong = new Song;
			if (!pNewSong->LoadFromSongDir(songDir)) {
				delete pNewSong;
				continue;
			}
			if (group.name == "Unknown Group") {
				pNewSong->m_sGroupName = "Ungrouped Songs";
			}
			AddSongToList(pNewSong);
			AddKeyedPointers(pNewSong);

			index_entry.emplace_back(pNewSong);

			// Update nsman to keep us from getting disconnected
			NSMAN->Update(0.0F);

			Message msg("DFRUpdate");
			msg.SetParam("txt",
						 "Loading:\n" + group.name + "\n" +
						   pNewSong->GetDisplayMainTitle());
			MESSAGEMAN->Broadcast(msg);
			SCREENMAN->Draw(); // not sure if this needs to be handled better
							   // (more safely?) or if its fine-mina

			loaded++;
			songIndex++;
			newsongs++;
		}

		if (loaded == 0) {
			continue;
		}
		LOG->Trace("Differential load of %i songs from \"%s\"",
				   loaded,
				   (dir + group.name).c_str());

		AddGroup(dir, group.name);
		IMAGECACHE->CacheImage("Banner",
							   GetSongGroupBannerPath(dir + group.name));
	}
	return newsongs;
}

template<typename T>
using it = typename vector<T>::iterator;
template<typename T>
using p = std::pair<it<T>, it<T>>;
template<typename T>
auto
split(vector<T>& v, size_t elementsPerThread) -> std::vector<p<T>>
{
	std::vector<p<T>> ranges;
	if (elementsPerThread <= 0 || elementsPerThread >= v.size()) {
		ranges.emplace_back(std::make_pair(v.begin(), v.end()));
		return ranges;
	}

	size_t range_count = (v.size() + 1) / elementsPerThread + 1;
	size_t ePT = v.size() / range_count;
	if (ePT == 0) {
		ranges.emplace_back(std::make_pair(v.begin(), v.end()));
		return ranges;
	}
	size_t i;

	it<T> b = v.begin();

	for (i = 0; i < v.size() - ePT; i += ePT)
		ranges.emplace_back(std::make_pair(b + i, b + i + ePT));

	ranges.emplace_back(std::make_pair(b + i, v.end()));
	return ranges;
}

void
FinalizeSong(Song* pNewSong, const std::string& dir)
{
	// never load stray songs from the cache -mina
	if (pNewSong->m_sGroupName == "Songs" ||
		pNewSong->m_sGroupName == "AdditionalSongs") {
		return;
	}
	AddSongToList(pNewSong);
	AddKeyedPointers(pNewSong);
	m_mapSongGroupIndex[pNewSong->m_sGroupName].emplace_back(pNewSong);
	if (AddGroup(dir.substr(0, dir.find('/', 1) + 1), pNewSong->m_sGroupName)) {
		IMAGECACHE->CacheImage("Banner",
							   GetSongGroupBannerPath(pNewSong->m_sGroupName));
	}
}

std::mutex songLoadingSONGMANMutex;

void
CalcTestStuff()
{
	vector<float> test_vals[NUM_Skillset];

	// output calc differences for chartkeys and targets and stuff
	for (const auto& p : testChartList) {
		auto ss = p.first;
		LOG->Trace("\nStarting calc test group %s\n",
				   SkillsetToString(ss).c_str());
		for (const auto& chart : p.second.filemapping) {

			if (StepsByKey.count(chart.first) != 0u) {
				test_vals[ss].emplace_back(
				  StepsByKey[chart.first]->DoATestThing(
					chart.second.ev, ss, chart.second.rate, calc.get()));
			}
		}
		LOG->Trace("\n\n");
	}

	FOREACH_ENUM(Skillset, ss)
	{
		if (!test_vals[ss].empty()) {
			LOG->Trace(
			  "%+0.2f avg delta for test group %s",
			  std::accumulate(begin(test_vals[ss]), end(test_vals[ss]), 0.F) /
				test_vals[ss].size(),
			  SkillsetToString(ss).c_str());
		}
	}

	// bzzzzzzzzzzzz this won't work for what i want unless we also make dummy
	// entries in testlist for stuff and don't set an ev
	// int counter = 0;
	// for (auto& ohno : StepsByKey){
	//	ohno.second->DoATestThing(40.f, Skill_Overall, 1.f);
	//	++counter;
	//	if (counter > 500)
	//		break;
	//}

	SaveCalcTestXmlToDir();
}

void
MakeSongGroupsFromPlaylists(map<string, Playlist>& playlists)
{
	if (!PlaylistsAreSongGroups) {
		return;
	}

	for (auto& plName : playlistGroups) {
		groupderps.erase(plName);
	}
	playlistGroups.clear();
	for (auto& p : playlists) {
		vector<Song*> playlistgroup;
		for (auto& n : p.second.chartlist) {
			if (n.loaded) {
				playlistgroup.emplace_back(GetSongByChartkey(n.key));
			}
		}

		groupderps.erase(p.first);
		groupderps[p.first] = playlistgroup;
		SongUtil::SortSongPointerArrayByTitle(groupderps[p.first]);
		playlistGroups.emplace_back(p.first);
	}
}

void
DeletePlaylist(const string& pl, map<string, Playlist>& playlists)
{
	playlists.erase(pl);

	// stuff gets weird if all playlists have been deleted and a chart
	// is added
	// - mina
	if (!playlists.empty()) {
		activeplaylist = playlists.begin()->first;
	}

	// clear out the entry for the music wheel as well or it'll crash
	// -mina
	groupderps.erase(pl);
}

void
MakePlaylistFromFavorites(std::set<string>& favs,
						  std::map<std::string, Playlist>& playlists)
{
	Playlist pl;
	pl.name = "Favorites";
	for (auto& n : favs) {
		pl.AddChart(n);
	}

	// kinda messy but, trim unloaded charts from the favorites playlist
	// -mina
	for (size_t i = 0; i < pl.chartlist.size(); ++i) {
		if (!pl.chartlist[i].loaded) {
			pl.DeleteChart(i);
		}
	}

	if (!pl.chartlist.empty()) {
		playlists.emplace("Favorites", pl);
	}
}

void
ReconcileChartKeysForReloadedSong(const Song* reloadedSong,
								  const std::vector<std::string>& oldChartkeys)
{
	for (const auto& ck : oldChartkeys) {
		StepsByKey.erase(ck);
	}
	auto stepses = reloadedSong->GetAllSteps();
	for (auto steps : stepses) {
		StepsByKey[steps->GetChartKey()] = steps;
	}
}

// Get a steps pointer given a chartkey, the assumption here is we want
// _a_ matching steps, not the original steps - mina
auto
GetStepsByChartkey(const std::string& ck) -> Steps*
{
	if (StepsByKey.count(ck) != 0u) {
		return StepsByKey[ck];
	}
	return nullptr;
}

auto
GetSongByChartkey(const std::string& ck) -> Song*
{
	if (SongsByKey.count(ck) != 0u) {
		return SongsByKey[ck];
	}
	return nullptr;
}

void
UnloadAllCalcDebugOutput()
{
	for (auto s : m_pSongs) {
		s->UnloadAllCalcDebugOutput();
	}
}

static LocalizedString FOLDER_CONTAINS_MUSIC_FILES(
  "SongManager",
  "The folder \"%s\" appears to be a song folder.  All song folders "
  "must "
  "reside in a group folder.  For example, \"Songs/Originals/My "
  "Song\".");

std::mutex diskLoadSongMutex;
std::mutex diskLoadGroupMutex;
static LocalizedString LOADING_SONGS("SongManager", "Loading songs...");

auto
IsGroupNeverCached(const std::string& group) -> bool
{
	return m_GroupsToNeverCache.find(group) != m_GroupsToNeverCache.end();
}

void
SetFavoritedStatus(std::set<string>& favs)
{
	for (auto song : m_pSongs) {
		auto fav = false;
		for (auto steps : song->GetAllSteps()) {
			if (favs.count(steps->GetChartKey()) != 0u) {
				fav = true;
			}
		}

		song->SetFavorited(fav);
	}
}

void
SetPermaMirroredStatus(std::set<string>& pmir)
{
	for (auto song : m_pSongs) {
		for (auto steps : song->GetAllSteps()) {
			if (pmir.count(steps->GetChartKey()) != 0u) {
				song->SetPermaMirror(true);
			}
		}
	}
}

// hurr should probably redo both (all three) of these -mina
void
SetHasGoal(std::unordered_map<string, GoalsForChart>& goalmap)
{
	for (auto song : m_pSongs) {
		auto hasGoal = false;
		for (auto steps : song->GetAllSteps()) {
			if (goalmap.count(steps->GetChartKey()) != 0u) {
				hasGoal = true;
			}
			song->SetHasGoal(hasGoal);
		}
	}
}

auto
GetSongGroupBannerPath(const std::string& sSongGroup) -> std::string
{
	for (unsigned i = 0; i < m_sSongGroupNames.size(); ++i) {
		if (sSongGroup == m_sSongGroupNames[i]) {
			return m_sSongGroupBannerPaths[i];
		}
	}

	return std::string();
}
/*
std::string SongManager::GetSongGroupBackgroundPath( std::string sSongGroup )
const
{
	for( unsigned i = 0; i < m_sSongGroupNames.size(); ++i )
	{
		if( sSongGroup == m_sSongGroupNames[i] )
			return m_sSongGroupBackgroundPaths[i];
	}

	return std::string();
}
*/
void
GetSongGroupNames(vector<std::string>& AddTo)
{
	AddTo.insert(
	  AddTo.end(), m_sSongGroupNames.begin(), m_sSongGroupNames.end());
}

auto
GetSongGroupNames() -> const vector<std::string>&
{
	return m_sSongGroupNames;
}

auto
DoesSongGroupExist(const std::string& sSongGroup) -> bool
{
	return find(m_sSongGroupNames.begin(),
				m_sSongGroupNames.end(),
				sSongGroup) != m_sSongGroupNames.end();
}

auto
GetSongGroupColor(const std::string& sSongGroup,
				  map<string, Playlist>& playlists) -> RageColor
{
	for (unsigned i = 0; i < m_sSongGroupNames.size(); i++) {
		if (m_sSongGroupNames[i] == sSongGroup ||
			(playlists.count(sSongGroup) != 0u)) {
			return SONG_GROUP_COLOR.GetValue(i % NUM_SONG_GROUP_COLORS);
		}
	}

	return RageColor(1, 1, 1, 1);
}

auto
GetSongColor(const Song* pSong) -> RageColor
{
	assert(pSong != nullptr);

	const auto& vpSteps = pSong->GetAllSteps();
	for (auto pSteps : vpSteps) {
		switch (pSteps->GetDifficulty()) {
			case Difficulty_Challenge:
			case Difficulty_Edit:
				continue;
			default:
				break;
		}

		if (pSteps->GetMeter() >= EXTRA_COLOR_METER) {
			return static_cast<RageColor>(EXTRA_COLOR);
		}
	}

	return GetSongGroupColor(pSong->m_sGroupName);
}

void
ResetGroupColors()
{
	// Reload song/course group colors to prevent a crash when switching
	// themes in-game. (apparently not, though.) -aj
	SONG_GROUP_COLOR.Clear();

	NUM_SONG_GROUP_COLORS.Load("SongManager", "NumSongGroupColors");
	SONG_GROUP_COLOR.Load(
	  "SongManager", SONG_GROUP_COLOR_NAME, NUM_SONG_GROUP_COLORS);
}

auto
GetSongs(const std::string& sGroupName) -> const vector<Song*>&
{
	static const vector<Song*> vEmpty;

	if (sGroupName == GROUP_ALL) {
		return m_pSongs;
	}
	auto iter = m_mapSongGroupIndex.find(sGroupName);
	if (iter != m_mapSongGroupIndex.end()) {
		return iter->second;
	}
	return vEmpty;
}
void
ForceReloadSongGroup(const std::string& sGroupName)
{
	auto songs = GetSongs(sGroupName);
	for (auto s : songs) {
		auto stepses = s->GetAllSteps();
		vector<string> oldChartkeys;
		oldChartkeys.reserve(stepses.size());
		for (auto steps : stepses) {
			oldChartkeys.emplace_back(steps->GetChartKey());
		}

		s->ReloadFromSongDir();
		ReconcileChartKeysForReloadedSong(s, oldChartkeys);
	}
}

void
GetFavoriteSongs(vector<Song*>& songs)
{
	for (const auto& song : m_pSongs) {
		if (song->IsFavorited()) {
			songs.emplace_back(song);
		}
	}
}

auto
GetNumSongs() -> int
{
	return m_pSongs.size();
}

auto
GetNumAdditionalSongs() -> int
{
	auto iNum = 0;
	for (auto song : m_pSongs) {
		if (WasLoadedFromAdditionalSongs(song)) {
			++iNum;
		}
	}
	return iNum;
}

auto
GetNumSongGroups() -> int
{
	return m_sSongGroupNames.size();
}

auto
ShortenGroupName(const std::string& sLongGroupName) -> std::string
{
	static TitleSubst tsub("Groups");

	TitleFields title;
	title.Title = sLongGroupName;
	tsub.Subst(title);
	return title.Title;
}

/* Called periodically to wipe out cached NoteData. This is called when
 * we change screens. */
void
Cleanup()
{
	for (auto pSong : m_pSongs) {
		if (pSong != nullptr) {
			const auto& vpSteps = pSong->GetAllSteps();
			for (auto pSteps : vpSteps) {
				pSteps->Compress();
			}
		}
	}
}

auto
GetPlaylists() -> map<string, Playlist>&
{
	return PROFILEMAN->GetProfile(PLAYER_1)->allplaylists;
}

auto
WasLoadedFromAdditionalSongs(const Song* pSong) -> bool
{
	const auto& sDir = pSong->GetSongDir();
	return BeginsWith(sDir, ADDITIONAL_SONGS_DIR);
}

auto
GetSongFromDir(std::string dir) -> Song*
{
	ensure_slash_at_end(dir);

	s_replace(dir, "\\", "/");
	dir = make_lower(dir);
	auto entry = m_SongsByDir.find(dir);
	if (entry != m_SongsByDir.end()) {
		return entry->second;
	}
	return nullptr;
}

void
SortSongs()
{
	SongUtil::SortSongPointerArrayByTitle(m_pSongs);
}

void
makePlaylist(const std::string& answer)
{
	Playlist pl;
	pl.name = answer;
	if (!pl.name.empty()) {
		SONGMAN::GetPlaylists().emplace(pl.name, pl);
		SONGMAN::activeplaylist = pl.name;
		MESSAGEMAN->Broadcast("DisplayAll");
		PROFILEMAN->SaveProfile(PLAYER_1);
	}
}
static const string calctest_XML = "CalcTestList.xml";

void
LoadCalcTestNode()
{
	auto fn = "Save/" + calctest_XML;
	int iError;
	std::unique_ptr<RageFileBasic> pFile(
	  FILEMAN->Open(fn, RageFile::READ, iError));
	if (pFile == nullptr) {
		LOG->Trace("Error opening %s: %s", fn.c_str(), strerror(iError));
		return;
	}

	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, *pFile)) {
		return;
	}

	CHECKPOINT_M("Loading the Calc Test node.");

	FOREACH_CONST_Child(&xml, chartlist) // "For Each Skillset
	{
		int ssI;
		chartlist->GetAttrValue("Skillset", ssI);
		auto ss = static_cast<Skillset>(ssI);
		CalcTestList tl;
		tl.skillset = ss;
		FOREACH_CONST_Child(chartlist, uhh) // For Each Chartlist (oops)
		{
			FOREACH_CONST_Child(uhh, entry) // For Each Chart
			{
				std::string key;
				float target;
				float rate;
				entry->GetAttrValue("aKey", key);
				entry->GetAttrValue("bRate", rate);
				entry->GetAttrValue("cTarget", target);
				CalcTest ct;
				ct.ck = key;
				ct.ev = target;
				ct.rate = rate;

				auto vers_hist = entry->GetChild("VersionHistory");
				if (vers_hist != nullptr) {
					FOREACH_CONST_Child(vers_hist, thing)
					{
						// don't load any values for the current version, it's
						// in flux
						if (stoi(thing->GetName()) != GetCalcVersion()) {
							auto mumbo = 0.F;
							thing->GetTextValue(mumbo);
							ct.version_history.emplace(std::pair<int, float>(
							  stoi(thing->GetName()), mumbo));
						}
					}
				}

				tl.filemapping[key] = ct;
			}
		}
		SONGMAN::testChartList[ss] = tl;
	}
}

auto
SaveCalcTestCreateNode() -> XNode*
{
	CHECKPOINT_M("Saving the Calc Test node.");

	auto* calctestlists = new XNode("CalcTest");
	for (const auto& i : testChartList) {
		calctestlists->AppendChild(i.second.CreateNode());
	}

	return calctestlists;
}

void
SaveCalcTestXmlToDir()
{
	auto fn = "Save/" + calctest_XML;
	// calc test hardcode stuff cuz ASDKLFJASKDJLFHASHDFJ
	const std::unique_ptr<XNode> xml(SaveCalcTestCreateNode());
	string err;
	RageFile f;
	if (!f.Open(fn, RageFile::WRITE)) {
		LuaHelpers::ReportScriptErrorFmt(
		  "Couldn't open %s for writing: %s", fn.c_str(), f.GetError().c_str());
		return;
	}
	XmlFileUtil::SaveToFile(xml.get(), f, "", false);
}
namespace { // Anonymous forward-declared functions
void
LoadEnabledSongsFromPref()
{
	vector<std::string> asDisabledSongs;
	split(g_sDisabledSongs, ";", asDisabledSongs, true);

	for (auto& s : asDisabledSongs) {
		SongID sid;
		sid.FromString(s);
		auto pSong = sid.ToSong();
		if (pSong != nullptr) {
			pSong->SetEnabled(false);
		}
	}
}

void
FreeSongs()
{
	m_sSongGroupNames.clear();
	m_sSongGroupBannerPaths.clear();
	// m_sSongGroupBackgroundPaths.clear();

	for (unsigned i = 0; i < m_pSongs.size(); ++i) {
		SAFE_DELETE(m_pSongs[i]);
	}

	m_pSongs.clear();
	m_SongsByDir.clear();

	m_mapSongGroupIndex.clear();
	m_sSongGroupBannerPaths.clear();

	SongsByKey.clear();
	StepsByKey.clear();
	groupderps.clear();

	m_pPopularSongs.clear();
}

void
InitSongsFromDisk(LoadingWindow* ld)
{
	RageTimer tm;
	// Tell SONGINDEX to not write the cache index file every time a song adds
	// an entry. -Kyz
	SONGINDEX->delay_save_cache = true;
	if (PREFSMAN->m_bFastLoad) {
		SONGINDEX->LoadCache(ld, cache);
	}
	if (ld != nullptr) {
		ld->SetIndeterminate(false);
		ld->SetTotalWork(cache.size());
		ld->SetText("Loading songs from cache");
		ld->SetProgress(0);
	}
	auto onePercent = std::max(static_cast<int>(cache.size() / 100), 1);

	std::function<void(
	  std::pair<
		vectorIt<std::pair<std::pair<std::string, unsigned int>, Song*>*>,
		vectorIt<std::pair<std::pair<std::string, unsigned int>, Song*>*>>,
	  ThreadData*)>
	  callback =
		[](std::pair<
			 vectorIt<std::pair<std::pair<std::string, unsigned int>, Song*>*>,
			 vectorIt<std::pair<std::pair<std::string, unsigned int>, Song*>*>>
			 workload,
		   ThreadData* data) {
			auto pair =
			  static_cast<std::pair<int, LoadingWindow*>*>(data->data);
			auto onePercent = pair->first;
			auto ld = pair->second;
			auto cacheIndex = 0;
			auto lastUpdate = 0;
			for (auto it = workload.first; it != workload.second; it++) {
				auto pair = *it;
				cacheIndex++;
				if (cacheIndex % onePercent == 0) {
					data->_progress += cacheIndex - lastUpdate;
					lastUpdate = cacheIndex;
					data->setUpdated(true);
				}
				auto& pNewSong = pair->second;
				const std::string& dir = pNewSong->GetSongDir();
				if (!FILEMAN->IsADirectory(dir.substr(0, dir.length() - 1)) ||
					(!PREFSMAN->m_bBlindlyTrustCache.Get() &&
					 pair->first.second != GetHashForDirectory(dir))) {
					if (PREFSMAN->m_bShrinkSongCache) {
						SONGINDEX->DeleteSongFromDB(pair->second);
					}
					delete pair->second;
					continue;
				}
				pNewSong->FinalizeLoading();
				{
					std::lock_guard<std::mutex> lock(songLoadingSONGMANMutex);
					FinalizeSong(pNewSong, dir);
				}
			}
		};
	auto onUpdate = [ld](int progress) {
		if (ld != nullptr) {
			ld->SetProgress(progress);
		}
	};
	parallelExecution<std::pair<std::pair<std::string, unsigned int>, Song*>*>(
	  cache,
	  onUpdate,
	  callback,
	  static_cast<void*>(new std::pair<int, LoadingWindow*>(onePercent, ld)));
	LoadStepManiaSongDir(SpecialFiles::SONGS_DIR, ld);
	LoadStepManiaSongDir(ADDITIONAL_SONGS_DIR, ld);
	LoadEnabledSongsFromPref();
	SONGINDEX->delay_save_cache = false;

	if (PREFSMAN->m_verbose_log > 1) {
		LOG->Trace("Found %u songs in %f seconds.",
				   static_cast<unsigned int>(m_pSongs.size()),
				   tm.GetDeltaTime());
	}
	for (auto& pair : cache) {
		delete pair;
	}

	cache.clear();
}

// Only store 1 steps/song pointer per key -Mina
void
AddKeyedPointers(Song* new_song)
{
	const auto steps = new_song->GetAllSteps();
	for (auto step : steps) {
		const auto& ck = step->GetChartKey();
		if (StepsByKey.count(ck) == 0u) {
			StepsByKey.emplace(ck, step);
			if (SongsByKey.count(ck) == 0u) {
				SongsByKey.emplace(ck, new_song);
			}
		}
	}

	groupderps[new_song->m_sGroupName].emplace_back(new_song);
}

void
LoadStepManiaSongDir(std::string sDir, LoadingWindow* ld)
{
	std::vector<std::string> songFolders;
	GetDirListing(sDir + "*", songFolders, true);
	auto songCount = 0;
	if (ld != nullptr) {
		ld->SetIndeterminate(false);
		ld->SetTotalWork(songFolders.size());
		ld->SetText("Checking song folders...");
	}
	std::vector<Group> groups;
	auto unknownGroup = Group(std::string("Unknown Group"));
	auto foldersChecked = 0;
	auto onePercent = std::max(static_cast<int>(songFolders.size() / 100), 1);
	for (const auto& folder : songFolders) {
		auto burp = sDir + folder;
		if (SONGMAN::IsSongDir(burp)) {
			unknownGroup.songs.emplace_back(burp);
		} else {
			auto group = Group(folder);
			GetDirListing(sDir + folder + "/*", group.songs, true, true);
			songCount += group.songs.size();
			groups.emplace_back(group);
		}
	}
	if (!unknownGroup.songs.empty()) {
		groups.emplace_back(unknownGroup);
	}

	if (ld != nullptr) {
		ld->SetIndeterminate(false);
		ld->SetTotalWork(groups.size());
		ld->SetText("Loading Songs From Disk\n");
		ld->SetProgress(0);
	}
	auto groupIndex = 0;
	onePercent = std::max(static_cast<int>(groups.size() / 100), 1);

	auto callback =
	  [&sDir](std::pair<vectorIt<Group>, vectorIt<Group>> workload,
			  ThreadData* data) {
		  auto per_thread_calc = std::make_unique<Calc>();

		  auto pair = static_cast<std::pair<int, LoadingWindow*>*>(data->data);
		  auto onePercent = pair->first;
		  auto ld = pair->second;
		  auto counter = 0;
		  auto lastUpdate = 0;
		  for (auto it = workload.first; it != workload.second; it++) {
			  auto pair = *it;
			  auto& sGroupName = it->name;
			  counter++;
			  auto& arraySongDirs = it->songs;
			  if (counter % onePercent == 0) {
				  data->_progress += counter - lastUpdate;
				  lastUpdate = counter;
				  data->setUpdated(true);
			  }
			  auto loaded = 0;
			  auto index_entry = m_mapSongGroupIndex[sGroupName];
			  const auto& group_base_name = sGroupName;
			  for (auto& sSongDirName : arraySongDirs) {
				  auto hur = make_lower(sSongDirName + "/");
				  if (m_SongsByDir.count(hur) != 0u) {
					  continue;
				  }
				  auto pNewSong = new Song;
				  if (!pNewSong->LoadFromSongDir(sSongDirName,
												 per_thread_calc.get())) {
					  delete pNewSong;
					  continue;
				  }
				  if (sGroupName == "Unknown Group") {
					  pNewSong->m_sGroupName = "Ungrouped Songs";
				  }
				  {
					  std::lock_guard<std::mutex> lk(diskLoadSongMutex);
					  AddSongToList(pNewSong);
					  SONGMAN::AddKeyedPointers(pNewSong);
				  }
				  index_entry.emplace_back(pNewSong);
				  loaded++;
			  }
			  if (loaded == 0) {
				  continue;
			  }
			  LOG->Trace("Loaded %i songs from \"%s\"",
						 loaded,
						 (sDir + sGroupName).c_str());
			  {
				  std::lock_guard<std::mutex> lk(diskLoadGroupMutex);
				  SONGMAN::AddGroup(sDir, sGroupName);
				  IMAGECACHE->CacheImage(
					"Banner", GetSongGroupBannerPath(sDir + sGroupName));
			  }
		  }
	  };
	auto onUpdate = [ld](int progress) {
		if (ld != nullptr) {
			ld->SetProgress(progress);
		}
	};
	vector<Group> workload;
	workload.reserve(groups.size());
	for (auto& group : groups) {
		workload.emplace_back(group);
	}

	if (!workload.empty()) {
		parallelExecution<Group>(
		  workload,
		  onUpdate,
		  callback,
		  static_cast<void*>(
			new std::pair<int, LoadingWindow*>(onePercent, ld)));
	}

	if (ld != nullptr) {
		ld->SetIndeterminate(true);
	}
}

auto
IsSongDir(const std::string& sDir) -> bool
{
	// Check to see if they put a song directly inside the group folder.
	std::vector<std::string> arrayFiles;
	GetDirListing(sDir + "/*", arrayFiles);
	const auto& audio_exts = ActorUtil::GetTypeExtensionList(FT_Sound);
	for (auto& fname : arrayFiles) {
		const std::string ext = GetExtension(fname);
		for (auto& aud : audio_exts) {
			if (ext == aud) {
				return true;
			}
		}
	}
	return false;
}

auto
AddGroup(const std::string& sDir, const std::string& sGroupDirName) -> bool
{
	unsigned j;
	for (j = 0; j < m_sSongGroupNames.size(); ++j) {
		if (sGroupDirName == m_sSongGroupNames[j]) {
			break;
		}
	}

	if (j != m_sSongGroupNames.size()) {
		return false; // the group is already added
	}

	// Look for a group banner in this group folder
	std::vector<std::string> arrayGroupBanners;

	FILEMAN->GetDirListingWithMultipleExtensions(
	  sDir + sGroupDirName + "/",
	  ActorUtil::GetTypeExtensionList(FT_Bitmap),
	  arrayGroupBanners);

	std::string sBannerPath;
	if (!arrayGroupBanners.empty()) {
		sBannerPath = sDir + sGroupDirName + "/" + arrayGroupBanners[0];
	} else {
		// Look for a group banner in the parent folder
		FILEMAN->GetDirListingWithMultipleExtensions(
		  sDir + sGroupDirName,
		  ActorUtil::GetTypeExtensionList(FT_Bitmap),
		  arrayGroupBanners);
		if (!arrayGroupBanners.empty()) {
			sBannerPath = sDir + arrayGroupBanners[0];
		}
	}

	m_sSongGroupNames.emplace_back(sGroupDirName);
	m_sSongGroupBannerPaths.emplace_back(sBannerPath);
	return true;
}

void
AddSongToList(Song* new_song)
{
	new_song->SetEnabled(true);
	m_pSongs.emplace_back(new_song);
	std::string dir = make_lower(new_song->GetSongDir());
	m_SongsByDir.insert(make_pair(dir, new_song));
}
} // End anonymous forward-declared functions
} // End SONGMAN namespace

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

namespace {
static auto
GetAllSongs(lua_State* L) -> int
{
	const auto& v = SONGMAN::GetAllSongs();
	LuaHelpers::CreateTableFromArray<Song*>(v, L);
	return 1;
}

static auto
DifferentialReload(lua_State* L) -> int
{
	lua_pushnumber(L, SONGMAN::DifferentialReload());
	return 1;
}

static auto
GetNumSongs(lua_State* L) -> int
{
	lua_pushnumber(L, SONGMAN::GetNumSongs());
	return 1;
}

static auto
GetNumAdditionalSongs(lua_State* L) -> int
{
	lua_pushnumber(L, SONGMAN::GetNumAdditionalSongs());
	return 1;
}

static auto
GetNumSongGroups(lua_State* L) -> int
{
	lua_pushnumber(L, SONGMAN::GetNumSongGroups());
	return 1;
}

/* Note: this could now be implemented as Luna<Steps>::GetSong */
static auto
GetSongFromSteps(lua_State* L) -> int
{
	Song* pSong = nullptr;
	if (lua_isnil(L, 1)) {
		pSong = nullptr;
	} else {
		auto pSteps = Luna<Steps>::check(L, 1);
		pSong = pSteps->m_pSong;
	}
	if (pSong != nullptr) {
		pSong->PushSelf(L);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static auto
GetSongColor(lua_State* L) -> int
{
	LuaHelpers::Push(L, SONGMAN::GetSongColor(Luna<Song>::check(L, 1)));
	return 1;
}

static auto
GetSongGroupColor(lua_State* L) -> int
{
	LuaHelpers::Push(L, SONGMAN::GetSongGroupColor(SArg(1)));
	return 1;
}

static auto
GetSongGroupNames(lua_State* L) -> int
{
	vector<std::string> v;
	SONGMAN::GetSongGroupNames(v);
	LuaHelpers::CreateTableFromArray<std::string>(v, L);
	return 1;
}

static auto
GetSongsInGroup(lua_State* L) -> int
{
	vector<Song*> v = SONGMAN::GetSongs(SArg(1));
	LuaHelpers::CreateTableFromArray<Song*>(v, L);
	return 1;
}

static auto
ShortenGroupName(lua_State* L) -> int
{
	LuaHelpers::Push(L, SONGMAN::ShortenGroupName(SArg(1)));
	return 1;
}

static auto
GetSongGroupBannerPath(lua_State* L) -> int
{
	LuaHelpers::Push(L, SONGMAN::GetSongGroupBannerPath(SArg(1)));
	return 1;
}

static auto
DoesSongGroupExist(lua_State* L) -> int
{
	LuaHelpers::Push(L, SONGMAN::DoesSongGroupExist(SArg(1)));
	return 1;
}

static auto
IsChartLoaded(lua_State* L) -> int
{
	LuaHelpers::Push(L, SONGMAN::IsChartLoaded(SArg(1)));
	return 1;
}

static auto
GetPopularSongs(lua_State* L) -> int
{
	const auto& v = SONGMAN::GetPopularSongs();
	LuaHelpers::CreateTableFromArray<Song*>(v, L);
	return 1;
}

static auto
WasLoadedFromAdditionalSongs(lua_State* L) -> int
{
	const Song* pSong = Luna<Song>::check(L, 1);
	lua_pushboolean(
	  L, static_cast<int>(SONGMAN::WasLoadedFromAdditionalSongs(pSong)));
	return 1;
}

static auto
GetSongByChartKey(lua_State* L) -> int
{
	std::string ck = SArg(1);
	Song* pSong = SONGMAN::GetSongByChartkey(ck);
	if (pSong != nullptr) {
		pSong->PushSelf(L);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static auto
GetStepsByChartKey(lua_State* L) -> int
{
	std::string ck = SArg(1);
	Steps* pSteps = SONGMAN::GetStepsByChartkey(ck);
	if (pSteps != nullptr) {
		pSteps->PushSelf(L);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static auto
GetActivePlaylist(lua_State* L) -> int
{
	SONGMAN::GetPlaylists()[SONGMAN::activeplaylist].PushSelf(L);
	return 1;
}

static auto
SetActivePlaylist(lua_State* L) -> int
{
	SONGMAN::activeplaylist = SArg(1);
	return 0;
}

static auto NewPlaylist(lua_State * /*L*/) -> int
{
	ScreenTextEntry::TextEntry(
	  SM_None, "Name Playlist", "", 128, nullptr, SONGMAN::makePlaylist);
	return 0;
}

static auto
GetPlaylists(lua_State* L) -> int
{
	auto idx = 1;
	lua_newtable(L);
	for (auto pl : SONGMAN::GetPlaylists()) {
		pl.second.PushSelf(L);
		lua_rawseti(L, -2, idx);
		++idx;
	}

	return 1;
}

static auto
DeletePlaylist(lua_State* L) -> int
{
	SONGMAN::DeletePlaylist(SArg(1));
	PROFILEMAN->SaveProfile(PLAYER_1);
	return 0;
}

const luaL_Reg SONGMANTable[] = { LIST_METHOD(GetAllSongs),
								  LIST_METHOD(DifferentialReload),
								  LIST_METHOD(GetNumSongs),
								  LIST_METHOD(GetNumAdditionalSongs),
								  LIST_METHOD(GetNumSongGroups),
								  LIST_METHOD(GetSongFromSteps),
								  LIST_METHOD(GetSongColor),
								  LIST_METHOD(GetSongGroupColor),
								  LIST_METHOD(GetSongGroupNames),
								  LIST_METHOD(GetSongsInGroup),
								  LIST_METHOD(ShortenGroupName),
								  LIST_METHOD(GetSongGroupBannerPath),
								  LIST_METHOD(DoesSongGroupExist),
								  LIST_METHOD(GetPopularSongs),
								  LIST_METHOD(WasLoadedFromAdditionalSongs),
								  LIST_METHOD(GetSongByChartKey),
								  LIST_METHOD(GetStepsByChartKey),
								  LIST_METHOD(GetActivePlaylist),
								  LIST_METHOD(SetActivePlaylist),
								  LIST_METHOD(NewPlaylist),
								  LIST_METHOD(GetPlaylists),
								  LIST_METHOD(DeletePlaylist),
								  { nullptr, nullptr } };

}; // Anonymous namespace

LUA_REGISTER_NAMESPACE(SONGMAN)

class LunaPlaylist : public Luna<Playlist>
{
	// TODO: This should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp

  public:
	static auto GetChartkeys(T* p, lua_State* L) -> int
	{
		auto keys = p->GetKeys();
		LuaHelpers::CreateTableFromArray(keys, L);
		return 1;
	}

	static auto GetAllSteps(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		for (size_t i = 0; i < p->chartlist.size(); ++i) {
			p->chartlist[i].PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static auto GetSonglist(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		for (size_t i = 0; i < p->chartlist.size(); ++i) {
			p->chartlist[i].songptr->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static auto GetStepslist(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		for (size_t i = 0; i < p->chartlist.size(); ++i) {
			p->chartlist[i].stepsptr->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static auto AddChart(T* p, lua_State* L) -> int
	{
		p->AddChart(SArg(1));
		PROFILEMAN->SaveProfile(PLAYER_1);
		return 1;
	}

	static auto DeleteChart(T* p, lua_State* L) -> int
	{
		p->DeleteChart(IArg(1) - 1);
		PROFILEMAN->SaveProfile(PLAYER_1);
		return 1;
	}

	DEFINE_METHOD(GetAverageRating, GetAverageRating());
	DEFINE_METHOD(GetName, GetName());
	DEFINE_METHOD(GetNumCharts, GetNumCharts())
	LunaPlaylist()
	{
		ADD_METHOD(AddChart);
		ADD_METHOD(GetChartkeys);
		ADD_METHOD(GetAllSteps);
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
	// TODO: This should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp
  public:
	DEFINE_METHOD(GetRate, rate);
	DEFINE_METHOD(IsLoaded, IsLoaded());
	DEFINE_METHOD(GetDifficulty, lastdiff);
	DEFINE_METHOD(GetSongTitle, lastsong);
	DEFINE_METHOD(GetPackName, lastpack);

	static auto ChangeRate(T* p, lua_State* L) -> int
	{
		p->rate += FArg(1);
		p->rate = std::clamp(p->rate, 0.7F, 3.F);
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
// Lua end

void
Chart::FromKey(const string& ck)
{
	// TODO: This function should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp
	auto song = SONGMAN::GetSongByChartkey(ck);
	key = ck;

	if (song != nullptr) {
		auto steps = SONGMAN::GetStepsByChartkey(ck);
		if (steps !=
			nullptr) { // happens when you edit a file for playtesting -mina
			lastpack = song->m_sGroupName;
			lastsong = song->GetDisplayMainTitle();
			lastdiff = steps->GetDifficulty();
			loaded = true;
			songptr = song;
			stepsptr = steps;
			return;
		}
	}
	loaded = false;
}

auto
Chart::CreateNode(bool includerate) const -> XNode*
{
	// TODO: This function should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp
	auto ch = new XNode("Chart");
	ch->AppendAttr("Key", key);
	ch->AppendAttr("Pack", lastpack);
	ch->AppendAttr("Song", lastsong);
	ch->AppendAttr("Steps", DifficultyToString(lastdiff));

	if (includerate) {
		ch->AppendAttr("Rate", ssprintf("%.3f", rate));
	}
	return ch;
}

void
Chart::LoadFromNode(const XNode* node)
{
	// TODO: This function should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp
	ASSERT(node->GetName() == "Chart");

	std::string s;
	node->GetAttrValue("Pack", lastpack);
	node->GetAttrValue("Song", lastsong);
	node->GetAttrValue("Steps", s);
	lastdiff = StringToDifficulty(s);
	node->GetAttrValue("Rate", s);
	rate = StringToFloat(s);
	node->GetAttrValue("Key", s);
	key = s;

	// check if this chart is loaded and overwrite any last-seen values
	// with updated ones
	FromKey(key);
}

void
Playlist::AddChart(const string& ck)
{
	// TODO: This function should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp
	auto rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	Chart ch;
	ch.FromKey(ck);
	ch.rate = rate;
	chartlist.emplace_back(ch);
}

void
Playlist::DeleteChart(int i)
{
	// TODO: This function should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp
	if (chartlist.empty() || i < 0 || i > static_cast<int>(chartlist.size())) {
		return;
	}

	chartlist.erase(chartlist.begin() + i);
}

auto
Playlist::CreateNode() const -> XNode*
{
	// TODO: This function should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp
	auto pl = new XNode("Playlist");
	pl->AppendAttr("Name", name);

	auto cl = new XNode("Chartlist");
	FOREACH_CONST(Chart, chartlist, ch)
	cl->AppendChild(ch->CreateNode(true));

	auto cr = new XNode("CourseRuns");
	FOREACH_CONST(vector<string>, courseruns, run)
	{
		auto r = new XNode("Run");
		FOREACH_CONST(string, *run, sk)
		r->AppendChild(*sk);
		cr->AppendChild(r);
	}

	if (!cl->ChildrenEmpty()) {
		pl->AppendChild(cl);
	} else {
		delete cl;
	}
	if (!cr->ChildrenEmpty()) {
		pl->AppendChild(cr);
	} else {
		delete cr;
	}

	return pl;
}

void
Playlist::LoadFromNode(const XNode* node)
{
	// TODO: This function should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp
	ASSERT(node->GetName() == "Playlist");

	node->GetAttrValue("Name", name);
	if (!node->ChildrenEmpty()) {
		auto cl = node->GetChild("Chartlist");
		if (cl != nullptr) {
			FOREACH_CONST_Child(cl, chart)
			{
				Chart ch;
				ch.LoadFromNode(chart);
				chartlist.emplace_back(ch);
			}
		}

		auto cr = node->GetChild("CourseRuns");
		if (cr != nullptr) {
			FOREACH_CONST_Child(cr, run)
			{
				vector<string> tmp;
				FOREACH_CONST_Child(run, sk) tmp.emplace_back(sk->GetName());
				courseruns.emplace_back(tmp);
			}
		}
	}
}
auto
Playlist::GetAverageRating() -> float
{
	// TODO: This function should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp
	if (chartlist.empty()) {
		return 0;
	}
	auto o = 0.F;
	auto numloaded = 0;
	for (auto& n : chartlist) {
		if (n.loaded) {
			auto rate = n.rate;
			CLAMP(rate, 0.7F, 3.F);
			o += n.stepsptr->GetMSD(rate, 0);
			++numloaded;
		}
	}
	if (numloaded == 0) {
		return 0.F;
	}
	return o / static_cast<float>(numloaded);
}

auto
Playlist::GetKeys() -> vector<string>
{
	// TODO: This function should really be defined in
	// Etterna/Models/Misc/Difficulty.cpp
	vector<string> o;
	for (auto& i : chartlist) {
		o.emplace_back(i.key);
	}
	return o;
}
auto
CalcTestList::CreateNode() const -> XNode*
{
	auto pl = new XNode("CalcTestList");
	pl->AppendAttr("Skillset", skillset);

	auto cl = new XNode("Chartlist");
	for (const auto& p : filemapping) {
		auto chart = new XNode("Chart");
		Chart loot;
		loot.FromKey(p.first);
		chart->AppendAttr("aKey", p.first);
		chart->AppendAttr("zSong", loot.lastsong);
		chart->AppendAttr("cTarget", ssprintf("%.2f", p.second.ev));
		chart->AppendAttr("bRate", ssprintf("%.2f", p.second.rate));
		auto vers_hist = chart->AppendChild("VersionHistory");
		for (const auto& vh : p.second.version_history) {
			vers_hist->AppendChild(std::to_string(vh.first), vh.second);
		}
		cl->AppendChild(chart);
	}

	if (!cl->ChildrenEmpty()) {
		pl->AppendChild(cl);
	} else {
		delete cl;
	}

	return pl;
}
