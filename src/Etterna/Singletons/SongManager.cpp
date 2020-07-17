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
#include "Etterna/Models/StepsAndStyles/StepsUtil.h"
#include "ThemeManager.h"
#include "Etterna/Models/Misc/TitleSubstitution.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "ScreenManager.h"
#include "NetworkSyncManager.h"
#include "Etterna/Globals/rngthing.h"
#include "Etterna/MinaCalc/MinaCalc.h"
#include "Etterna/FileTypes/XmlFileUtil.h"

#include <numeric>
#include <algorithm>
#include <mutex>

using std::map;
using std::string;
using std::vector;

typedef std::string SongDir;
struct Group
{
	std::string name;
	vector<SongDir> songs;
	Group(std::string name)
	  : name(name)
	{
	}
};

SongManager* SONGMAN =
  nullptr; // global and accessible from anywhere in our program

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

std::string
SONG_GROUP_COLOR_NAME(size_t i)
{
	return ssprintf("SongGroupColor%i", static_cast<int>(i) + 1);
}

AutoScreenMessage(SM_BackFromNamePlaylist);

SongManager::SongManager()
{
	// Register with Lua.
	{
		auto L = LUA->Get();
		lua_pushstring(L, "SONGMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}

	NUM_SONG_GROUP_COLORS.Load("SongManager", "NumSongGroupColors");
	SONG_GROUP_COLOR.Load(
	  "SongManager", SONG_GROUP_COLOR_NAME, NUM_SONG_GROUP_COLORS);

	// calc for debug/session scores
	calc = std::make_unique<Calc>();
}

SongManager::~SongManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("SONGMAN");

	// Courses depend on Songs and Songs don't depend on Courses.
	// So, delete the Courses first.
	FreeSongs();
}

void
SongManager::InitAll(LoadingWindow* ld)
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
int
SongManager::DifferentialReload()
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
int
SongManager::DifferentialReloadDir(string dir)
{
	if (dir.back() != '/')
		dir += "/";

	auto newsongs = 0;

	vector<std::string> folders;
	GetDirListing(dir + "*", folders, true);

	vector<Group> groups;
	Group unknownGroup("Unknown Group");
	int groupIndex, songCount, songIndex;

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

	if (songCount == 0)
		return 0;

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
			if (m_SongsByDir.count(hur))
				continue;

			auto pNewSong = new Song;
			if (!pNewSong->LoadFromSongDir(songDir)) {
				delete pNewSong;
				continue;
			}
			if (group.name == "Unknown Group")
				pNewSong->m_sGroupName = "Ungrouped Songs";
			AddSongToList(pNewSong);
			AddKeyedPointers(pNewSong);

			index_entry.emplace_back(pNewSong);

			// Update nsman to keep us from getting disconnected
			NSMAN->Update(0.0f);

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

		if (!loaded)
			continue;
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
std::vector<p<T>>
split(vector<T>& v, size_t elementsPerThread)
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
SongManager::FinalizeSong(Song* pNewSong, const std::string& dir)
{
	// never load stray songs from the cache -mina
	if (pNewSong->m_sGroupName == "Songs" ||
		pNewSong->m_sGroupName == "AdditionalSongs")
		return;
	SONGMAN->AddSongToList(pNewSong);
	SONGMAN->AddKeyedPointers(pNewSong);
	SONGMAN->m_mapSongGroupIndex[pNewSong->m_sGroupName].emplace_back(pNewSong);
	if (SONGMAN->AddGroup(dir.substr(0, dir.find('/', 1) + 1),
						  pNewSong->m_sGroupName))
		IMAGECACHE->CacheImage(
		  "Banner", SONGMAN->GetSongGroupBannerPath(pNewSong->m_sGroupName));
}

std::mutex songLoadingSONGMANMutex;
void
SongManager::InitSongsFromDisk(LoadingWindow* ld)
{
	RageTimer tm;
	// Tell SONGINDEX to not write the cache index file every time a song adds
	// an entry. -Kyz
	SONGINDEX->delay_save_cache = true;
	if (PREFSMAN->m_bFastLoad)
		SONGINDEX->LoadCache(ld, cache);
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
					if (PREFSMAN->m_bShrinkSongCache)
						SONGINDEX->DeleteSongFromDB(pair->second);
					delete pair->second;
					continue;
				}
				pNewSong->FinalizeLoading();
				{
					std::lock_guard<std::mutex> lock(songLoadingSONGMANMutex);
					SONGMAN->FinalizeSong(pNewSong, dir);
				}
			}
		};
	auto onUpdate = [ld](int progress) {
		if (ld != nullptr)
			ld->SetProgress(progress);
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

	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("Found %u songs in %f seconds.",
				   static_cast<unsigned int>(m_pSongs.size()),
				   tm.GetDeltaTime());
	for (auto& pair : cache)
		delete pair;

	cache.clear();
}

void
SongManager::CalcTestStuff()
{
	vector<float> test_vals[NUM_Skillset];

	// output calc differences for chartkeys and targets and stuff
	for (const auto& p : testChartList) {
		auto ss = p.first;
		LOG->Trace("\nStarting calc test group %s\n",
				   SkillsetToString(ss).c_str());
		for (const auto& chart : p.second.filemapping) {

			if (StepsByKey.count(chart.first))
				test_vals[ss].emplace_back(
				  StepsByKey[chart.first]->DoATestThing(
					chart.second.ev, ss, chart.second.rate, calc.get()));
		}
		LOG->Trace("\n\n");
	}

	FOREACH_ENUM(Skillset, ss)
	{
		if (!test_vals[ss].empty())
			LOG->Trace(
			  "%+0.2f avg delta for test group %s",
			  std::accumulate(begin(test_vals[ss]), end(test_vals[ss]), 0.f) /
				test_vals[ss].size(),
			  SkillsetToString(ss).c_str());
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
Chart::FromKey(const string& ck)
{
	auto song = SONGMAN->GetSongByChartkey(ck);
	key = ck;

	if (song != nullptr) {
		auto steps = SONGMAN->GetStepsByChartkey(ck);
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

XNode*
Chart::CreateNode(bool includerate) const
{
	auto ch = new XNode("Chart");
	ch->AppendAttr("Key", key);
	ch->AppendAttr("Pack", lastpack);
	ch->AppendAttr("Song", lastsong);
	ch->AppendAttr("Steps", DifficultyToString(lastdiff));

	if (includerate)
		ch->AppendAttr("Rate", ssprintf("%.3f", rate));
	return ch;
}

void
Chart::LoadFromNode(const XNode* node)
{
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
	auto rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	Chart ch;
	ch.FromKey(ck);
	ch.rate = rate;
	chartlist.emplace_back(ch);
}

void
Playlist::DeleteChart(int i)
{
	if (chartlist.size() == 0 || i < 0 ||
		i > static_cast<int>(chartlist.size()))
		return;

	chartlist.erase(chartlist.begin() + i);
}

XNode*
Playlist::CreateNode() const
{
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

	if (!cl->ChildrenEmpty())
		pl->AppendChild(cl);
	else
		delete cl;
	if (!cr->ChildrenEmpty())
		pl->AppendChild(cr);
	else
		delete cr;

	return pl;
}

void
Playlist::LoadFromNode(const XNode* node)
{
	ASSERT(node->GetName() == "Playlist");

	node->GetAttrValue("Name", name);
	if (!node->ChildrenEmpty()) {
		auto cl = node->GetChild("Chartlist");
		if (cl) {
			FOREACH_CONST_Child(cl, chart)
			{
				Chart ch;
				ch.LoadFromNode(chart);
				chartlist.emplace_back(ch);
			}
		}

		auto cr = node->GetChild("CourseRuns");
		if (cr) {
			FOREACH_CONST_Child(cr, run)
			{
				vector<string> tmp;
				FOREACH_CONST_Child(run, sk) tmp.emplace_back(sk->GetName());
				courseruns.emplace_back(tmp);
			}
		}
	}
}

void
SongManager::MakeSongGroupsFromPlaylists(map<string, Playlist>& playlists)
{
	if (!PlaylistsAreSongGroups)
		return;

	for (auto& plName : playlistGroups)
		groupderps.erase(plName);
	playlistGroups.clear();
	for (auto& p : playlists) {
		vector<Song*> playlistgroup;
		for (auto& n : p.second.chartlist)
			if (n.loaded)
				playlistgroup.emplace_back(SONGMAN->GetSongByChartkey(n.key));

		groupderps.erase(p.first);
		groupderps[p.first] = playlistgroup;
		SongUtil::SortSongPointerArrayByTitle(groupderps[p.first]);
		playlistGroups.emplace_back(p.first);
	}
}

float
Playlist::GetAverageRating()
{
	if (chartlist.empty())
		return 0;
	auto o = 0.f;
	auto numloaded = 0;
	for (auto& n : chartlist) {
		if (n.loaded) {
			auto rate = n.rate;
			CLAMP(rate, 0.7f, 3.f);
			o += n.stepsptr->GetMSD(rate, 0);
			++numloaded;
		}
	}
	if (numloaded == 0)
		return 0.f;
	return o / static_cast<float>(numloaded);
}

vector<string>
Playlist::GetKeys()
{
	vector<string> o;
	for (auto& i : chartlist)
		o.emplace_back(i.key);
	return o;
}

void
SongManager::DeletePlaylist(const string& pl, map<string, Playlist>& playlists)
{
	playlists.erase(pl);

	// stuff gets weird if all playlists have been deleted and a chart
	// is added
	// - mina
	if (playlists.size() > 0)
		activeplaylist = playlists.begin()->first;

	// clear out the entry for the music wheel as well or it'll crash
	// -mina
	groupderps.erase(pl);
}

void
SongManager::MakePlaylistFromFavorites(std::set<string>& favs,
									   map<string, Playlist>& playlists)
{
	Playlist pl;
	pl.name = "Favorites";
	for (auto& n : favs)
		pl.AddChart(n);

	// kinda messy but, trim unloaded charts from the favorites playlist
	// -mina
	for (size_t i = 0; i < pl.chartlist.size(); ++i)
		if (!pl.chartlist[i].loaded)
			pl.DeleteChart(i);

	playlists.emplace("Favorites", pl);
}

void
SongManager::ReconcileChartKeysForReloadedSong(const Song* reloadedSong,
											   vector<string> oldChartkeys)
{
	for (const auto& ck : oldChartkeys)
		SONGMAN->StepsByKey.erase(ck);
	auto stepses = reloadedSong->GetAllSteps();
	for (auto steps : stepses)
		SONGMAN->StepsByKey[steps->GetChartKey()] = steps;
}

// Only store 1 steps/song pointer per key -Mina
void
SongManager::AddKeyedPointers(Song* new_song)
{
	const auto steps = new_song->GetAllSteps();
	for (auto step : steps) {
		const auto& ck = step->GetChartKey();
		if (!StepsByKey.count(ck)) {
			StepsByKey.emplace(ck, step);
			if (!SongsByKey.count(ck)) {
				SongsByKey.emplace(ck, new_song);
			}
		}
	}

	groupderps[new_song->m_sGroupName].emplace_back(new_song);
}

// Get a steps pointer given a chartkey, the assumption here is we want
// _a_ matching steps, not the original steps - mina
Steps*
SongManager::GetStepsByChartkey(const string& ck)
{
	if (StepsByKey.count(ck))
		return StepsByKey[ck];
	return nullptr;
}

Song*
SongManager::GetSongByChartkey(const string& ck)
{
	if (SongsByKey.count(ck))
		return SongsByKey[ck];
	return nullptr;
}

void
SongManager::UnloadAllCalcDebugOutput()
{
	for (auto s : m_pSongs)
		s->UnloadAllCalcDebugOutput();
}

static LocalizedString FOLDER_CONTAINS_MUSIC_FILES(
  "SongManager",
  "The folder \"%s\" appears to be a song folder.  All song folders "
  "must "
  "reside in a group folder.  For example, \"Songs/Originals/My "
  "Song\".");
bool
SongManager::IsSongDir(const std::string& sDir)
{
	// Check to see if they put a song directly inside the group folder.
	vector<std::string> arrayFiles;
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

bool
SongManager::AddGroup(const std::string& sDir, const std::string& sGroupDirName)
{
	unsigned j;
	for (j = 0; j < m_sSongGroupNames.size(); ++j)
		if (sGroupDirName == m_sSongGroupNames[j])
			break;

	if (j != m_sSongGroupNames.size())
		return false; // the group is already added

	// Look for a group banner in this group folder
	vector<std::string> arrayGroupBanners;

	FILEMAN->GetDirListingWithMultipleExtensions(
	  sDir + sGroupDirName + "/",
	  ActorUtil::GetTypeExtensionList(FT_Bitmap),
	  arrayGroupBanners);

	std::string sBannerPath;
	if (!arrayGroupBanners.empty())
		sBannerPath = sDir + sGroupDirName + "/" + arrayGroupBanners[0];
	else {
		// Look for a group banner in the parent folder
		FILEMAN->GetDirListingWithMultipleExtensions(
		  sDir + sGroupDirName,
		  ActorUtil::GetTypeExtensionList(FT_Bitmap),
		  arrayGroupBanners);
		if (!arrayGroupBanners.empty())
			sBannerPath = sDir + arrayGroupBanners[0];
	}

	m_sSongGroupNames.emplace_back(sGroupDirName);
	m_sSongGroupBannerPaths.emplace_back(sBannerPath);
	return true;
}

std::mutex diskLoadSongMutex;
std::mutex diskLoadGroupMutex;
static LocalizedString LOADING_SONGS("SongManager", "Loading songs...");
void
SongManager::LoadStepManiaSongDir(std::string sDir, LoadingWindow* ld)
{
	vector<std::string> songFolders;
	GetDirListing(sDir + "*", songFolders, true);
	auto songCount = 0;
	if (ld != nullptr) {
		ld->SetIndeterminate(false);
		ld->SetTotalWork(songFolders.size());
		ld->SetText("Checking song folders...");
	}
	vector<Group> groups;
	auto unknownGroup = Group(std::string("Unknown Group"));
	int foldersChecked = 0;
	int onePercent = std::max(static_cast<int>(songFolders.size() / 100), 1);
	for (const auto& folder : songFolders) {
		auto burp = sDir + folder;
		if (IsSongDir(burp)) {
			unknownGroup.songs.emplace_back(burp);
		} else {
			auto group = Group(folder);
			GetDirListing(sDir + folder + "/*", group.songs, true, true);
			songCount += group.songs.size();
			groups.emplace_back(group);
		}
	}
	if (!unknownGroup.songs.empty())
		groups.emplace_back(unknownGroup);

	if (ld != nullptr) {
		ld->SetIndeterminate(false);
		ld->SetTotalWork(groups.size());
		ld->SetText("Loading Songs From Disk\n");
		ld->SetProgress(0);
	}
	auto groupIndex = 0;
	onePercent = std::max(static_cast<int>(groups.size() / 100), 1);

	auto callback = [&sDir](
					  std::pair<vectorIt<Group>, vectorIt<Group>> workload,
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
			int loaded = 0;
			SongPointerVector& index_entry =
			  SONGMAN->m_mapSongGroupIndex[sGroupName];
			const auto& group_base_name = sGroupName;
			for (auto& sSongDirName : arraySongDirs) {
				std::string hur = make_lower(sSongDirName + "/");
				if (SONGMAN->m_SongsByDir.count(hur))
					continue;
				auto pNewSong = new Song;
				if (!pNewSong->LoadFromSongDir(sSongDirName,
											   per_thread_calc.get())) {
					delete pNewSong;
					continue;
				}
				if (sGroupName == "Unknown Group")
					pNewSong->m_sGroupName = "Ungrouped Songs";
				{
					std::lock_guard<std::mutex> lk(diskLoadSongMutex);
					SONGMAN->AddSongToList(pNewSong);
					SONGMAN->AddKeyedPointers(pNewSong);
				}
				index_entry.emplace_back(pNewSong);
				loaded++;
			}
			if (!loaded)
				continue;
			LOG->Trace("Loaded %i songs from \"%s\"",
					   loaded,
					   (sDir + sGroupName).c_str());
			{
				std::lock_guard<std::mutex> lk(diskLoadGroupMutex);
				SONGMAN->AddGroup(sDir, sGroupName);
				IMAGECACHE->CacheImage(
				  "Banner", SONGMAN->GetSongGroupBannerPath(sDir + sGroupName));
			}
		}
	};
	auto onUpdate = [ld](int progress) {
		if (ld != nullptr)
			ld->SetProgress(progress);
	};
	vector<Group> workload;
	workload.reserve(groups.size());
	for (auto& group : groups) {
		workload.emplace_back(group);
	}

	if (!workload.empty())
		parallelExecution<Group>(
		  workload,
		  onUpdate,
		  callback,
		  static_cast<void*>(
			new std::pair<int, LoadingWindow*>(onePercent, ld)));

	if (ld != nullptr) {
		ld->SetIndeterminate(true);
	}
}

void
SongManager::FreeSongs()
{
	m_sSongGroupNames.clear();
	m_sSongGroupBannerPaths.clear();
	// m_sSongGroupBackgroundPaths.clear();

	for (unsigned i = 0; i < m_pSongs.size(); i++)
		SAFE_DELETE(m_pSongs[i]);
	m_pSongs.clear();
	m_SongsByDir.clear();

	m_mapSongGroupIndex.clear();
	m_sSongGroupBannerPaths.clear();

	SongsByKey.clear();
	StepsByKey.clear();
	groupderps.clear();

	m_pPopularSongs.clear();
	m_pShuffledSongs.clear();
}

bool
SongManager::IsGroupNeverCached(const std::string& group) const
{
	return m_GroupsToNeverCache.find(group) != m_GroupsToNeverCache.end();
}

void
SongManager::SetFavoritedStatus(std::set<string>& favs)
{
	FOREACH(Song*, m_pSongs, song)
	{
		auto fav = false;
		FOREACH_CONST(Steps*, (*song)->GetAllSteps(), steps)
		if (favs.count((*steps)->GetChartKey()))
			fav = true;
		(*song)->SetFavorited(fav);
	}
}

void
SongManager::SetPermaMirroredStatus(std::set<string>& pmir)
{
	FOREACH(Song*, m_pSongs, song)
	FOREACH_CONST(Steps*, (*song)->GetAllSteps(), steps)
	if (pmir.count((*steps)->GetChartKey()))
		(*song)->SetPermaMirror(true);
}

// hurr should probably redo both (all three) of these -mina
void
SongManager::SetHasGoal(std::unordered_map<string, GoalsForChart>& goalmap)
{
	FOREACH(Song*, m_pSongs, song)
	{
		auto hasGoal = false;
		FOREACH_CONST(Steps*, (*song)->GetAllSteps(), steps)
		if (goalmap.count((*steps)->GetChartKey()))
			hasGoal = true;
		(*song)->SetHasGoal(hasGoal);
	}
}

std::string
SongManager::GetSongGroupBannerPath(const std::string& sSongGroup) const
{
	for (unsigned i = 0; i < m_sSongGroupNames.size(); ++i) {
		if (sSongGroup == m_sSongGroupNames[i])
			return m_sSongGroupBannerPaths[i];
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
SongManager::GetSongGroupNames(vector<std::string>& AddTo) const
{
	AddTo.insert(
	  AddTo.end(), m_sSongGroupNames.begin(), m_sSongGroupNames.end());
}

const vector<std::string>&
SongManager::GetSongGroupNames() const
{
	return m_sSongGroupNames;
}

bool
SongManager::DoesSongGroupExist(const std::string& sSongGroup) const
{
	return find(m_sSongGroupNames.begin(),
				m_sSongGroupNames.end(),
				sSongGroup) != m_sSongGroupNames.end();
}

RageColor
SongManager::GetSongGroupColor(const std::string& sSongGroup,
							   map<string, Playlist>& playlists) const
{
	for (unsigned i = 0; i < m_sSongGroupNames.size(); i++) {
		if (m_sSongGroupNames[i] == sSongGroup || playlists.count(sSongGroup)) {
			return SONG_GROUP_COLOR.GetValue(i % NUM_SONG_GROUP_COLORS);
		}
	}

	/*ASSERT_M(0,
			 ssprintf("requested color for song group '%s' that doesn't exist",
					  sSongGroup.c_str()));*/
	return RageColor(1, 1, 1, 1);
}

RageColor
SongManager::GetSongColor(const Song* pSong) const
{
	ASSERT(pSong != NULL);
	if (USE_PREFERRED_SORT_COLOR) {
		FOREACH_CONST(PreferredSortSection, m_vPreferredSongSort, v)
		{
			FOREACH_CONST(Song*, v->vpSongs, s)
			{
				if (*s == pSong) {
					int i = v - m_vPreferredSongSort.begin();
					return SONG_GROUP_COLOR.GetValue(i % NUM_SONG_GROUP_COLORS);
				}
			}
		}

		int i = m_vPreferredSongSort.size();
		return SONG_GROUP_COLOR.GetValue(i % NUM_SONG_GROUP_COLORS);
	} else // TODO: Have a better fallback plan with colors?
	{
		/* XXX: Previously, this matched all notes, which set a song to
		 * "extra" if it had any 10-foot steps at all, even edits or
		 * doubles.
		 *
		 * For now, only look at notes for the current note type. This
		 * means that if a song has 10-foot steps on Doubles, it'll only
		 * show up red in Doubles. That's not too bad, I think. This
		 * will also change it in the song scroll, which is a little odd
		 * but harmless.
		 *
		 * XXX: Ack. This means this function can only be called when we
		 * have
		 * a style set up, which is too restrictive. How to handle this?
		 */
		// const StepsType st =
		// GAMESTATE->GetCurrentStyle()->m_StepsType;
		const auto& vpSteps = pSong->GetAllSteps();
		for (auto pSteps : vpSteps) {
			switch (pSteps->GetDifficulty()) {
				case Difficulty_Challenge:
				case Difficulty_Edit:
					continue;
				default:
					break;
			}

			// if(pSteps->m_StepsType != st)
			//	continue;

			if (pSteps->GetMeter() >= EXTRA_COLOR_METER)
				return static_cast<RageColor>(EXTRA_COLOR);
		}

		return GetSongGroupColor(pSong->m_sGroupName);
	}
}

void
SongManager::ResetGroupColors()
{
	// Reload song/course group colors to prevent a crash when switching
	// themes in-game. (apparently not, though.) -aj
	SONG_GROUP_COLOR.Clear();

	NUM_SONG_GROUP_COLORS.Load("SongManager", "NumSongGroupColors");
	SONG_GROUP_COLOR.Load(
	  "SongManager", SONG_GROUP_COLOR_NAME, NUM_SONG_GROUP_COLORS);
}

const vector<Song*>&
SongManager::GetSongs(const std::string& sGroupName) const
{
	static const vector<Song*> vEmpty;

	if (sGroupName == GROUP_ALL)
		return m_pSongs;
	map<std::string, SongPointerVector, Comp>::const_iterator iter =
	  m_mapSongGroupIndex.find(sGroupName);
	if (iter != m_mapSongGroupIndex.end())
		return iter->second;
	return vEmpty;
}
void
SongManager::ForceReloadSongGroup(const std::string& sGroupName) const
{
	auto songs = GetSongs(sGroupName);
	for (auto s : songs) {
		auto stepses = s->GetAllSteps();
		vector<string> oldChartkeys;
		oldChartkeys.reserve(stepses.size());
		for (auto steps : stepses)
			oldChartkeys.emplace_back(steps->GetChartKey());

		s->ReloadFromSongDir();
		SONGMAN->ReconcileChartKeysForReloadedSong(s, oldChartkeys);
	}
}

void
SongManager::GetFavoriteSongs(vector<Song*>& songs) const
{
	FOREACH_CONST(Song*, m_pSongs, song)
	{
		if ((*song)->IsFavorited())
			songs.emplace_back((*song));
	}
}

int
SongManager::GetNumSongs() const
{
	return m_pSongs.size();
}

int
SongManager::GetNumAdditionalSongs() const
{
	auto iNum = 0;
	FOREACH_CONST(Song*, m_pSongs, i)
	{
		if (WasLoadedFromAdditionalSongs(*i))
			++iNum;
	}
	return iNum;
}

int
SongManager::GetNumSongGroups() const
{
	return m_sSongGroupNames.size();
}

std::string
SongManager::ShortenGroupName(const std::string& sLongGroupName)
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
SongManager::Cleanup()
{
	for (auto pSong : m_pSongs) {
		if (pSong) {
			const auto& vpSteps = pSong->GetAllSteps();
			for (auto pSteps : vpSteps) {
				pSteps->Compress();
			}
		}
	}
}

/* Flush all Song*, Steps* and Course* caches. This is when a Song or
 * its Steps are removed or changed. This doesn't touch GAMESTATE and
 * StageStats pointers. Currently, the only time Steps are altered
 * independently of the Courses and Songs is in Edit Mode, which updates
 * the other pointers it needs.
 */
void
SongManager::Invalidate(const Song* pStaleSong)
{
	UpdateShuffled();
}

map<string, Playlist>&
SongManager::GetPlaylists()
{
	return PROFILEMAN->GetProfile(PLAYER_1)->allplaylists;
}

void
SongManager::SaveEnabledSongsToPref()
{
	vector<std::string> vsDisabledSongs;

	// Intentionally drop disabled song entries for songs that aren't
	// currently loaded.

	for (auto& s : SONGMAN->GetAllSongs()) {
		SongID sid;
		sid.FromSong(s);
		if (!s->GetEnabled())
			vsDisabledSongs.emplace_back(sid.ToString());
	}
	g_sDisabledSongs.Set(join(";", vsDisabledSongs));
}

void
SongManager::LoadEnabledSongsFromPref()
{
	vector<std::string> asDisabledSongs;
	split(g_sDisabledSongs, ";", asDisabledSongs, true);

	for (auto& s : asDisabledSongs) {
		SongID sid;
		sid.FromString(s);
		auto pSong = sid.ToSong();
		if (pSong)
			pSong->SetEnabled(false);
	}
}

void
SongManager::DeleteSteps(Steps* pSteps)
{
	pSteps->m_pSong->DeleteSteps(pSteps);
}

bool
SongManager::WasLoadedFromAdditionalSongs(const Song* pSong) const
{
	const auto& sDir = pSong->GetSongDir();
	return BeginsWith(sDir, ADDITIONAL_SONGS_DIR);
}

// Return true if n1 < n2.
bool
CompareNotesPointersForExtra(const Steps* n1, const Steps* n2)
{
	// Equate CHALLENGE to HARD.
	auto d1 = std::min(n1->GetDifficulty(), Difficulty_Hard);
	auto d2 = std::min(n2->GetDifficulty(), Difficulty_Hard);

	if (d1 < d2)
		return true;
	if (d1 > d2)
		return false;
	// n1 difficulty == n2 difficulty

	if (StepsUtil::CompareNotesPointersByMeter(n1, n2))
		return true;
	if (StepsUtil::CompareNotesPointersByMeter(n2, n1))
		return false;
	// n1 meter == n2 meter

	return StepsUtil::CompareNotesPointersByRadarValues(n1, n2);
}

Song*
SongManager::GetRandomSong()
{
	if (m_pShuffledSongs.empty())
		return nullptr;

	static auto i = 0;

	Song* pSong = nullptr;

	for (auto iThrowAway = 0; iThrowAway < 100; iThrowAway++) {
		i++;
		wrap(i, m_pShuffledSongs.size());
		pSong = m_pShuffledSongs[i];
	}

	return pSong;
}

Song*
SongManager::GetSongFromDir(std::string dir) const
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

/* GetSongDir() contains a path to the song, possibly a full path, eg:
 * Songs\Group\SongName                   or
 * My Other Song Folder\Group\SongName    or
 * c:\Corny J-pop\Group\SongName
 *
 * Most course group names are "Group\SongName", so we want to match
 * against the last two elements. Let's also support "SongName" alone,
 * since the group is only important when it's potentially ambiguous.
 *
 * Let's *not* support "Songs\Group\SongName" in course files. That's
 * probably a common error, but that would result in course files
 * floating around that only work for people who put songs in "Songs";
 * we don't want that. */

Song*
SongManager::FindSong(std::string sPath) const
{
	s_replace(sPath, "\\", "/");
	vector<std::string> bits;
	split(sPath, "/", bits);

	if (bits.size() == 1)
		return FindSong("", bits[0]);
	if (bits.size() == 2)
		return FindSong(bits[0], bits[1]);

	return nullptr;
}

Song*
SongManager::FindSong(std::string sGroup, std::string sSong) const
{
	// foreach song
	const auto& vSongs = GetSongs(sGroup.empty() ? GROUP_ALL : sGroup);
	for (auto& s : vSongs) {
		if (s->Matches(sGroup, sSong))
			return s;
	}

	return nullptr;
}

void
SongManager::UpdateShuffled()
{
	// update shuffled
	m_pShuffledSongs = m_pSongs;
	std::shuffle(m_pShuffledSongs.begin(),
				 m_pShuffledSongs.end(),
				 g_RandomNumberGenerator);
}

void
SongManager::UpdatePreferredSort(const std::string& sPreferredSongs,
								 const std::string& sPreferredCourses)
{
	{
		m_vPreferredSongSort.clear();

		vector<std::string> asLines;
		std::string sFile = THEME->GetPathO("SongManager", sPreferredSongs);
		GetFileContents(sFile, asLines);
		if (asLines.empty())
			return;

		PreferredSortSection section;
		map<Song*, float> mapSongToPri;

		for (auto& sLine : asLines) {
			auto bSectionDivider = BeginsWith(sLine, "---");
			if (bSectionDivider) {
				if (!section.vpSongs.empty()) {
					m_vPreferredSongSort.emplace_back(section);
					section = PreferredSortSection();
				}

				section.sName =
				  tail(sLine, sLine.length() - std::string("---").length());
				TrimLeft(section.sName);
				TrimRight(section.sName);
			} else {
				/* if the line ends in slash-star, check if the section
				 * exists, and if it does, add all the songs in that
				 * group to the list.
				 */
				if (EndsWith(sLine, "/*")) {
					std::string group =
					  head(sLine, sLine.length() - std::string("/*").length());
					if (DoesSongGroupExist(group)) {
						// add all songs in group
						const auto& vSongs = GetSongs(group);
						for (auto& song : vSongs) {
							section.vpSongs.emplace_back(song);
						}
					}
				}

				Song* pSong = FindSong(sLine);
				if (pSong == nullptr)
					continue;
				section.vpSongs.emplace_back(pSong);
			}
		}

		if (!section.vpSongs.empty()) {
			m_vPreferredSongSort.emplace_back(section);
			section = PreferredSortSection();
		}

		// prune empty groups
		for (int i = m_vPreferredSongSort.size() - 1; i >= 0; i--)
			if (m_vPreferredSongSort[i].vpSongs.empty())
				m_vPreferredSongSort.erase(m_vPreferredSongSort.begin() + i);

		for (auto& i : m_vPreferredSongSort)
			for (auto& s : i.vpSongs) {
				ASSERT(s != nullptr);
			}
	}
}

void
SongManager::SortSongs()
{
	SongUtil::SortSongPointerArrayByTitle(m_pSongs);
}

void
SongManager::AddSongToList(Song* new_song)
{
	new_song->SetEnabled(true);
	m_pSongs.emplace_back(new_song);
	std::string dir = make_lower(new_song->GetSongDir());
	m_SongsByDir.insert(make_pair(dir, new_song));
}

void
makePlaylist(const std::string& answer)
{
	Playlist pl;
	pl.name = answer;
	if (pl.name != "") {
		SONGMAN->GetPlaylists().emplace(pl.name, pl);
		SONGMAN->activeplaylist = pl.name;
		Message msg("DisplayAll");
		MESSAGEMAN->Broadcast(msg);
		PROFILEMAN->SaveProfile(PLAYER_1);
	}
}
static const string calctest_XML = "CalcTestList.xml";

XNode*
CalcTestList::CreateNode() const
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
		for (const auto& vh : p.second.version_history)
			vers_hist->AppendChild(std::to_string(vh.first), vh.second);
		cl->AppendChild(chart);
	}

	if (!cl->ChildrenEmpty())
		pl->AppendChild(cl);
	else
		delete cl;

	return pl;
}

void
SongManager::LoadCalcTestNode() const
{
	auto fn = "Save/" + calctest_XML;
	int iError;
	std::unique_ptr<RageFileBasic> pFile(
	  FILEMAN->Open(fn, RageFile::READ, iError));
	if (pFile.get() == nullptr) {
		LOG->Trace("Error opening %s: %s", fn.c_str(), strerror(iError));
		return;
	}

	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, *pFile.get()))
		return;

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
				if (vers_hist) {
					FOREACH_CONST_Child(vers_hist, thing)
					{
						// don't load any values for the current version, it's
						// in flux
						if (stoi(thing->GetName()) != GetCalcVersion()) {
							auto mumbo = 0.f;
							thing->GetTextValue(mumbo);
							ct.version_history.emplace(std::pair<int, float>(
							  stoi(thing->GetName()), mumbo));
						}
					}
				}

				tl.filemapping[key.c_str()] = ct;
			}
		}
		SONGMAN->testChartList[ss] = tl;
	}
}

XNode*
SongManager::SaveCalcTestCreateNode() const
{
	CHECKPOINT_M("Saving the Calc Test node.");

	auto* calctestlists = new XNode("CalcTest");
	for (const auto& i : testChartList) {
		calctestlists->AppendChild(i.second.CreateNode());
	}

	return calctestlists;
}

void
SongManager::SaveCalcTestXmlToDir() const
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

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the SongManager. */
class LunaSongManager : public Luna<SongManager>
{
  public:
	static int SetPreferredSongs(T* p, lua_State* L)
	{
		p->UpdatePreferredSort(SArg(1), "PreferredCourses.txt");
		COMMON_RETURN_SELF;
	}

	static int GetAllSongs(T* p, lua_State* L)
	{
		const auto& v = p->GetAllSongs();
		LuaHelpers::CreateTableFromArray<Song*>(v, L);
		return 1;
	}

	static int DifferentialReload(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->DifferentialReload());
		return 1;
	}

	static int FindSong(T* p, lua_State* L)
	{
		Song* pS = p->FindSong(SArg(1));
		if (pS != nullptr)
			pS->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	static int GetRandomSong(T* p, lua_State* L)
	{
		auto pS = p->GetRandomSong();
		if (pS != nullptr)
			pS->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	static int GetNumSongs(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNumSongs());
		return 1;
	}

	static int GetNumAdditionalSongs(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNumAdditionalSongs());
		return 1;
	}

	static int GetNumSongGroups(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNumSongGroups());
		return 1;
	}

	/* Note: this could now be implemented as Luna<Steps>::GetSong */
	static int GetSongFromSteps(T* p, lua_State* L)
	{
		Song* pSong = nullptr;
		if (lua_isnil(L, 1)) {
			pSong = nullptr;
		} else {
			auto pSteps = Luna<Steps>::check(L, 1);
			pSong = pSteps->m_pSong;
		}
		if (pSong != nullptr)
			pSong->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	DEFINE_METHOD(GetSongColor, GetSongColor(Luna<Song>::check(L, 1)))
	DEFINE_METHOD(GetSongGroupColor, GetSongGroupColor(SArg(1)))

	static int GetSongGroupNames(T* p, lua_State* L)
	{
		vector<std::string> v;
		p->GetSongGroupNames(v);
		LuaHelpers::CreateTableFromArray<std::string>(v, L);
		return 1;
	}

	static int GetSongsInGroup(T* p, lua_State* L)
	{
		vector<Song*> v = p->GetSongs(SArg(1));
		LuaHelpers::CreateTableFromArray<Song*>(v, L);
		return 1;
	}

	DEFINE_METHOD(ShortenGroupName, ShortenGroupName(SArg(1)))
	DEFINE_METHOD(GetSongGroupBannerPath, GetSongGroupBannerPath(SArg(1)));
	DEFINE_METHOD(DoesSongGroupExist, DoesSongGroupExist(SArg(1)));
	DEFINE_METHOD(IsChartLoaded, IsChartLoaded(SArg(1)));

	static int GetPopularSongs(T* p, lua_State* L)
	{
		const auto& v = p->GetPopularSongs();
		LuaHelpers::CreateTableFromArray<Song*>(v, L);
		return 1;
	}

	static int WasLoadedFromAdditionalSongs(T* p, lua_State* L)
	{
		const Song* pSong = Luna<Song>::check(L, 1);
		lua_pushboolean(L, p->WasLoadedFromAdditionalSongs(pSong));
		return 1;
	}

	static int GetSongByChartKey(T* p, lua_State* L)
	{
		std::string ck = SArg(1);
		Song* pSong = p->GetSongByChartkey(ck);
		if (pSong != nullptr)
			pSong->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	static int GetStepsByChartKey(T* p, lua_State* L)
	{
		std::string ck = SArg(1);
		Steps* pSteps = p->GetStepsByChartkey(ck);
		if (pSteps != nullptr)
			pSteps->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	static int GetNumCourses(T* p, lua_State* L)
	{
		lua_pushnumber(L, 0);
		return 1;
	}

	static int GetActivePlaylist(T* p, lua_State* L)
	{
		p->GetPlaylists()[p->activeplaylist].PushSelf(L);
		return 1;
	}

	static int SetActivePlaylist(T* p, lua_State* L)
	{
		p->activeplaylist = SArg(1);
		return 0;
	}

	static int NewPlaylist(T* p, lua_State* L)
	{
		ScreenTextEntry::TextEntry(
		  SM_None, "Name Playlist", "", 128, nullptr, makePlaylist);
		return 0;
	}

	static int GetPlaylists(T* p, lua_State* L)
	{
		auto idx = 1;
		lua_newtable(L);
		FOREACHM(string, Playlist, p->GetPlaylists(), pl)
		{
			pl->second.PushSelf(L);
			lua_rawseti(L, -2, idx);
			++idx;
		}

		return 1;
	}

	static int DeletePlaylist(T* p, lua_State* L)
	{
		p->DeletePlaylist(SArg(1));
		PROFILEMAN->SaveProfile(PLAYER_1);
		return 0;
	}

	LunaSongManager()
	{
		ADD_METHOD(GetAllSongs);
		ADD_METHOD(DifferentialReload);
		ADD_METHOD(FindSong);
		ADD_METHOD(GetRandomSong);
		ADD_METHOD(GetNumSongs);
		ADD_METHOD(GetNumAdditionalSongs);
		ADD_METHOD(GetNumSongGroups);
		ADD_METHOD(GetSongFromSteps);
		ADD_METHOD(GetSongColor);
		ADD_METHOD(GetSongGroupColor);
		ADD_METHOD(GetSongGroupNames);
		ADD_METHOD(GetSongsInGroup);
		ADD_METHOD(ShortenGroupName);
		ADD_METHOD(SetPreferredSongs);
		ADD_METHOD(GetSongGroupBannerPath);
		ADD_METHOD(DoesSongGroupExist);
		ADD_METHOD(GetPopularSongs);
		ADD_METHOD(WasLoadedFromAdditionalSongs);
		ADD_METHOD(GetSongByChartKey);
		ADD_METHOD(GetStepsByChartKey);
		ADD_METHOD(GetNumCourses);
		ADD_METHOD(GetActivePlaylist);
		ADD_METHOD(SetActivePlaylist);
		ADD_METHOD(NewPlaylist);
		ADD_METHOD(GetPlaylists);
		ADD_METHOD(DeletePlaylist);
	}
};

LUA_REGISTER_CLASS(SongManager)

class LunaPlaylist : public Luna<Playlist>
{
  public:
	static int GetChartkeys(T* p, lua_State* L)
	{
		auto keys = p->GetKeys();
		LuaHelpers::CreateTableFromArray(keys, L);
		return 1;
	}

	static int GetAllSteps(T* p, lua_State* L)
	{
		lua_newtable(L);
		for (size_t i = 0; i < p->chartlist.size(); ++i) {
			p->chartlist[i].PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static int GetSonglist(T* p, lua_State* L)
	{
		lua_newtable(L);
		for (size_t i = 0; i < p->chartlist.size(); ++i) {
			p->chartlist[i].songptr->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static int GetStepslist(T* p, lua_State* L)
	{
		lua_newtable(L);
		for (size_t i = 0; i < p->chartlist.size(); ++i) {
			p->chartlist[i].stepsptr->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static int AddChart(T* p, lua_State* L)
	{
		p->AddChart(SArg(1));
		PROFILEMAN->SaveProfile(PLAYER_1);
		return 1;
	}

	static int DeleteChart(T* p, lua_State* L)
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
  public:
	DEFINE_METHOD(GetRate, rate);
	DEFINE_METHOD(IsLoaded, IsLoaded());
	DEFINE_METHOD(GetDifficulty, lastdiff);
	DEFINE_METHOD(GetSongTitle, lastsong);
	DEFINE_METHOD(GetPackName, lastpack);

	static int ChangeRate(T* p, lua_State* L)
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
