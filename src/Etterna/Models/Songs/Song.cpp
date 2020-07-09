#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Sound/RageSoundReader_FileReader.h"
#include "RageUtil/Graphics/RageSurface_Load.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Song.h"
#include "SongCacheIndex.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/TitleSubstitution.h"
#include "Etterna/Models/Misc/ImageCache.h"
#include "Etterna/Actor/Base/Sprite.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Graphics/RageSurface.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "SongUtil.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/StepsAndStyles/StepsUtil.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "Etterna/Models/NoteLoaders/NotesLoader.h"
#include "Etterna/Models/NoteWriters/NotesWriterDWI.h"
#include "Etterna/Models/NoteWriters/NotesWriterETT.h"
#include "Etterna/Models/NoteWriters/NotesWriterSM.h"
#include "Etterna/Models/NoteWriters/NotesWriterSSC.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/FilterManager.h"
#include "Etterna/Singletons/GameState.h"

#include <algorithm>
#include <cfloat>
#include <ctime>
#include <set>
#include <map>

using std::vector;

//-Nick12 Used for song file hashing
#include <Etterna/Singletons/CryptManager.h>

/** @brief How long does a song sample last by default? */
const float DEFAULT_MUSIC_SAMPLE_LENGTH = 25.f;

static Preference<float> g_fLongVerSongSeconds("LongVerSongSeconds", 60 * 2.5f);
static Preference<float> g_fMarathonVerSongSeconds("MarathonVerSongSeconds",
												   60 * 5.f);
static Preference<bool> g_BackUpAllSongSaves("BackUpAllSongSaves", false);

static const char* InstrumentTrackNames[] = {
	"Guitar",
	"Rhythm",
	"Bass",
};
XToString(InstrumentTrack);
StringToX(InstrumentTrack);

Song::Song()
{
	FOREACH_BackgroundLayer(i) m_BackgroundChanges[i] =
	  AutoPtrCopyOnWrite<VBackgroundChange>(new VBackgroundChange);
	m_ForegroundChanges =
	  AutoPtrCopyOnWrite<VBackgroundChange>(new VBackgroundChange);

	m_LoadedFromProfile = ProfileSlot_Invalid;
	m_fVersion = STEPFILE_VERSION_NUMBER;
	m_fMusicSampleStartSeconds = -1;
	m_fMusicSampleLengthSeconds = DEFAULT_MUSIC_SAMPLE_LENGTH;
	m_fMusicLengthSeconds = 0;
	firstSecond = -1;
	lastSecond = -1;
	specifiedLastSecond = -1;
	m_SelectionDisplay = SHOW_ALWAYS;
	m_bEnabled = true;
	m_DisplayBPMType = DISPLAY_BPM_ACTUAL;
	m_fSpecifiedBPMMin = 0;
	m_fSpecifiedBPMMax = 0;
	m_bIsSymLink = false;
	m_bHasMusic = false;
	m_bHasBanner = false;
	m_bHasBackground = false;
	ImageDir.clear();
	split(CommonMetrics::IMAGES_TO_CACHE, ",", ImageDir);
}

Song::~Song()
{
	for (auto& s : m_vpSteps) {
		SAFE_DELETE(s);
	}

	m_vpSteps.clear();

	for (auto& s : m_UnknownStyleSteps) {
		SAFE_DELETE(s);
	}

	m_UnknownStyleSteps.clear();

	// It's the responsibility of the owner of this Song to make sure
	// that all pointers to this Song and its Steps are invalidated.
}

void
Song::DetachSteps()
{
	m_vpSteps.clear();
	FOREACH_ENUM(StepsType, st)
	m_vpStepsByType[st].clear();
	m_UnknownStyleSteps.clear();
}

float
Song::GetFirstSecond() const
{
	return this->firstSecond;
}

float
Song::GetFirstBeat() const
{
	return this->m_SongTiming.GetBeatFromElapsedTime(this->firstSecond);
}

float
Song::GetLastSecond() const
{
	return this->lastSecond;
}

float
Song::GetLastBeat() const
{
	return this->m_SongTiming.GetBeatFromElapsedTime(this->lastSecond);
}

float
Song::GetSpecifiedLastSecond() const
{
	return this->specifiedLastSecond;
}

float
Song::GetSpecifiedLastBeat() const
{
	return this->m_SongTiming.GetBeatFromElapsedTime(this->specifiedLastSecond);
}

void
Song::SetFirstSecond(const float f)
{
	this->firstSecond = f;
}

void
Song::SetLastSecond(const float f)
{
	this->lastSecond = f;
}

void
Song::SetSpecifiedLastSecond(const float f)
{
	this->specifiedLastSecond = f;
}

// Reset to an empty song.
void
Song::Reset()
{
	for (auto& s : m_vpSteps) {
		SAFE_DELETE(s);
	}

	m_vpSteps.clear();

	FOREACH_ENUM(StepsType, st)
	m_vpStepsByType[st].clear();

	for (auto& s : m_UnknownStyleSteps) {
		SAFE_DELETE(s);
	}
	m_UnknownStyleSteps.clear();

	const Song empty;
	*this = empty;

	// It's the responsibility of the owner of this Song to make sure
	// that all pointers to this Song and its Steps are invalidated.
}

void
Song::AddBackgroundChange(BackgroundLayer iLayer, const BackgroundChange& seg)
{
	// Delete old background change at this start beat, if any.
	FOREACH(BackgroundChange, GetBackgroundChanges(iLayer), bgc)
	{
		if (bgc->m_fStartBeat == seg.m_fStartBeat) {
			GetBackgroundChanges(iLayer).erase(bgc);
			break;
		}
	}

	ASSERT(iLayer >= 0 && iLayer < NUM_BackgroundLayer);
	BackgroundUtil::AddBackgroundChange(GetBackgroundChanges(iLayer), seg);
}

void
Song::AddForegroundChange(const BackgroundChange& seg)
{
	BackgroundUtil::AddBackgroundChange(GetForegroundChanges(), seg);
}

void
Song::AddLyricSegment(const LyricSegment& seg)
{
	m_LyricSegments.push_back(seg);
}

Steps*
Song::CreateSteps()
{
	auto* pSteps = new Steps(this);
	InitSteps(pSteps);
	return pSteps;
}

void
Song::InitSteps(Steps* pSteps)
{
	// TimingData is initially empty (i.e. defaults to song timing)
	pSteps->SetDisplayBPM(this->m_DisplayBPMType);
	pSteps->SetMinBPM(this->m_fSpecifiedBPMMin);
	pSteps->SetMaxBPM(this->m_fSpecifiedBPMMax);
}

std::string
Song::GetOrTryAtLeastToGetSimfileAuthor()
{
	if (!m_sCredit.empty() && m_sCredit != "cdtitle")
		return m_sCredit;

	size_t begin = 0;
	size_t end = 0;

	auto o = GetSongDir();
	o = o.substr(0, o.size() - 1);
	o = o.substr(o.find_last_of('/') + 1);

	/*	Conform to current standard credit placement in songs folders,
	for those of you who don't follow convention - too bad. - mina */
	end = o.find_last_of(')');
	if (end != std::basic_string<char,
								 std::char_traits<char>,
								 std::allocator<char>>::npos) {
		begin = o.find_last_of('(');
		if (begin != std::basic_string<char,
									   std::char_traits<char>,
									   std::allocator<char>>::npos) {
			o = o.substr(begin + 1, end - begin - 1);
			return o;
		}
	}
	return "Author Unknown";
}

void
Song::GetDisplayBpms(DisplayBpms& AddTo) const
{
	const auto demratesboiz =
	  GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	if (m_DisplayBPMType == DISPLAY_BPM_SPECIFIED) {
		AddTo.Add(m_fSpecifiedBPMMin * demratesboiz);
		AddTo.Add(m_fSpecifiedBPMMax * demratesboiz);
	} else {
		float fMinBPM, fMaxBPM;
		m_SongTiming.GetActualBPM(fMinBPM, fMaxBPM);
		AddTo.Add(fMinBPM * demratesboiz);
		AddTo.Add(fMaxBPM * demratesboiz);
	}
}

const BackgroundChange&
Song::GetBackgroundAtBeat(BackgroundLayer iLayer, float fBeat) const
{
	for (unsigned i = 0; i < GetBackgroundChanges(iLayer).size() - 1; i++)
		if (GetBackgroundChanges(iLayer)[i + 1].m_fStartBeat > fBeat)
			return GetBackgroundChanges(iLayer)[i];
	return GetBackgroundChanges(iLayer)[0];
}

std::string
Song::GetCacheFilePath() const
{
	return SongCacheIndex::GetCacheFilePath("Songs", m_sSongDir);
}

// Get a path to the SM containing data for this song. It might be a cache
// file.
const std::string&
Song::GetSongFilePath() const
{
	ASSERT_M(!m_sSongFileName.empty(),
			 ssprintf("The song %s has no filename associated with it!",
					  this->m_sMainTitle.c_str()));
	return m_sSongFileName;
}

/* Hack: This should be a parameter to TidyUpData, but I don't want to pull
 * in <set> into Song.h, which is heavily used. */
static std::set<std::string> BlacklistedImages;

/* If PREFSMAN->m_bFastLoad is true, always load from cache if possible.
 * Don't read the contents of sDir if we can avoid it. That means we can't
 * call HasMusic(), HasBanner() or GetHashForDirectory(). If true, check the
 * directory hash and reload the song from scratch if it's changed.
 */
bool
Song::LoadFromSongDir(std::string sDir, Calc* calc)
{
	//	LOG->Trace( "Song::LoadFromSongDir(%s)", sDir.c_str() );
	ASSERT_M(!sDir.empty(), "Songs can't be loaded from an empty directory!");

	// make sure there is a trailing slash at the end of sDir
	ensure_slash_at_end(sDir);

	// save song dir
	m_sSongDir = sDir;

	// if (!SONGINDEX->LoadSongFromCache(this, sDir)) {
	// There was no entry in the cache for this song, or it was out of date.
	// Let's load it from a file, then write a cache entry.
	if (!NotesLoader::LoadFromDir(sDir, *this, BlacklistedImages)) {
		LOG->UserLog(
		  "Song", sDir, "has no SSC, SM, SMA, DWI, BMS, KSF, or OSU files.");

		vector<std::string> vs;
		FILEMAN->GetDirListingWithMultipleExtensions(
		  sDir, ActorUtil::GetTypeExtensionList(FT_Sound), vs, false, false);

		const auto bHasMusic = !vs.empty();

		if (!bHasMusic) {
			LOG->UserLog(
			  "Song",
			  sDir,
			  "has no music file either. Ignoring this song directory.");
			return false;
		}
		// Make sure we have a future filename figured out.
		vector<std::string> folders;
		split(sDir, "/", folders);
		const auto songName = folders[2] + ".ssc";
		this->m_sSongFileName = sDir + songName;
		// Continue on with a blank Song so that people can make adjustments
		// using the editor.
	}

	TidyUpData(false, true, calc);

	// save a cache file so we don't have to parse it all over again next
	// time
	SaveToCacheFile();

	FinalizeLoading();

	if (!m_bHasMusic) {
		LOG->UserLog("Song", sDir, "has no music; ignored.");
		return false; // don't load this song
	}
	return true; // do load this song
}

void
Song::FinalizeLoading()
{
	// save group name
	vector<std::string> sDirectoryParts;
	split(m_sSongDir, "/", sDirectoryParts, false);
	// ASSERT(sDirectoryParts.size() >= 4); /* e.g. "/Songs/Slow/Taps/" */
	m_sGroupName =
	  sDirectoryParts[sDirectoryParts.size() - 3]; // second from last item
	ASSERT(!m_sGroupName.empty());

	for (auto& s : m_vpSteps) {
		/* Compress all Steps. During initial caching, this will remove
		 * cached NoteData; during cached loads, this will just remove
		 * cached SMData.
		 */
		s->Compress();
	}

	// Load the cached Images, if it's not loaded already.
	if (PREFSMAN->m_ImageCache == IMGCACHE_LOW_RES_PRELOAD) {
		for (auto Image : ImageDir) {
			IMAGECACHE->LoadImage(Image, GetCacheFile(Image));
		}
	}
}
/* This function feels EXTREMELY hacky - copying things on top of pointers
 * so they don't break elsewhere.  Maybe it could be rewritten to politely
 * ask the Song/Steps objects to reload themselves. -- djpohly */
bool
Song::ReloadFromSongDir(const std::string& sDir)
{
	// Remove the cache file to force the song to reload from its dir
	// instead of loading from the cache. -Kyz
	FILEMAN->Remove(GetCacheFilePath());
	SONGINDEX->DeleteSongFromDBByDir(GetSongDir());

	auto vOldSteps = m_vpSteps;

	Song copy;
	if (!copy.LoadFromSongDir(sDir))
		return false;
	*this = copy;

	/* Go through the steps, first setting their Song pointer to this song
	 * (instead of the copy used above), and constructing a map to let us
	 * easily find the new steps. */
	std::map<StepsID, Steps*> mNewSteps;
	for (auto* m_vpStep : m_vpSteps) {
		m_vpStep->m_pSong = this;

		StepsID id;
		id.FromSteps(m_vpStep);
		mNewSteps[id] = m_vpStep;
	}

	// Now we wipe out the new pointers, which were shallow copied and not
	// deep copied...
	m_vpSteps.clear();
	FOREACH_ENUM(StepsType, i)
	m_vpStepsByType[i].clear();

	/* Then we copy as many Steps as possible on top of the old pointers.
	 * The only pointers that change are pointers to Steps that are not in
	 * the reverted file, which we delete, and pointers to Steps that are in
	 * the reverted file but not the original *this, which we create new
	 * copies of. We have to go through these hoops because many places
	 * assume the Steps pointers don't change - even though there are other
	 * ways they can change, such as deleting a Steps via the editor. */
	for (auto* vOldStep : vOldSteps) {
		StepsID id;
		id.FromSteps(vOldStep);
		auto itNew = mNewSteps.find(id);
		if (itNew == mNewSteps.end()) {
			// This stepchart didn't exist in the file we reverted from
			delete vOldStep;
		} else {
			auto* OldSteps = vOldStep;
			*OldSteps = *(itNew->second);
			AddSteps(OldSteps);
			mNewSteps.erase(itNew);
		}
	}
	// The leftovers in the map are steps that didn't exist before we
	// reverted
	for (const auto& mNewStep : mNewSteps) {
		auto* NewSteps = new Steps(this);
		*NewSteps = *(mNewStep.second);
		AddSteps(NewSteps);
	}

	// Reload any images associated with the song. -Kyz
	vector<std::string> to_reload;
	to_reload.reserve(7);
	to_reload.push_back(m_sBannerFile);
	to_reload.push_back(m_sJacketFile);
	to_reload.push_back(m_sCDFile);
	to_reload.push_back(m_sDiscFile);
	to_reload.push_back(m_sBackgroundFile);
	to_reload.push_back(m_sCDTitleFile);
	to_reload.push_back(m_sPreviewVidFile);
	for (auto& file : to_reload) {
		RageTextureID id(file);
		if (TEXTUREMAN->IsTextureRegistered(id)) {
			auto* tex = TEXTUREMAN->LoadTexture(id);
			if (tex) {
				tex->Reload();
			}
		}
	}
	return true;
}

// Check if the music file actually exists, if not, reload from disk, and if
// that fails, just crash - Terra
void
Song::ReloadIfNoMusic()
{
	const auto sMusicPath = GetMusicPath();
	if (sMusicPath.empty() || !DoesFileExist(sMusicPath)) {
		ASSERT_M(ReloadFromSongDir(),
				 "No music, so tried to reload from song dir but "
				 "ReloadFromSongDir failed");
	}
}

/* Fix up song paths. If there's a leading "./", be sure to keep it: it's
 * a signal that the path is from the root directory, not the song
 * directory. Other than a leading "./", song paths must never contain "."
 * or "..". */
void
FixupPath(std::string& path, const std::string& sSongPath)
{
	// Replace backslashes with slashes in all paths.
	FixSlashesInPlace(path);

	/* Many imported files contain erroneous whitespace before or after
	 * filenames. Paths usually don't actually start or end with spaces,
	 * so let's just remove it. */
	Trim(path);
}

// Songs in BlacklistImages will never be autodetected as song images.
void
Song::TidyUpData(bool from_cache, bool /* duringCache */, Calc* calc)
{
	FixupPath(m_sSongDir, "");
	FixupPath(m_sMusicFile, m_sSongDir);
	FOREACH_ENUM(InstrumentTrack, i)
	{
		if (!m_sInstrumentTrackFile[i].empty()) {
			FixupPath(m_sInstrumentTrackFile[i], m_sSongDir);
		}
	}
	FixupPath(m_sBannerFile, m_sSongDir);
	FixupPath(m_sJacketFile, m_sSongDir);
	FixupPath(m_sCDFile, m_sSongDir);
	FixupPath(m_sDiscFile, m_sSongDir);
	FixupPath(m_sLyricsFile, m_sSongDir);
	FixupPath(m_sBackgroundFile, m_sSongDir);
	FixupPath(m_sCDTitleFile, m_sSongDir);
	// CHECKPOINT_M("Looking for images...");

	m_SongTiming.TidyUpData(false);

	for (auto& s : m_vpSteps) {
		s->m_Timing.TidyUpData(true);
	}

	if (!from_cache) {
		if (this->m_sArtist == "The Dancing Monkeys Project" &&
			this->m_sMainTitle.find_first_of('-') != std::string::npos) {
			// Dancing Monkeys had a bug/feature where the artist was
			// replaced. Restore it.
			vector<std::string> titleParts;
			split(this->m_sMainTitle, "-", titleParts);
			this->m_sArtist = titleParts.front();
			Trim(this->m_sArtist);
			titleParts.erase(titleParts.begin());
			this->m_sMainTitle = join("-", titleParts);
			Trim(this->m_sMainTitle);
		}

		Trim(m_sMainTitle);
		Trim(m_sSubTitle);
		Trim(m_sArtist);

		// Fall back on the song directory name.
		if (m_sMainTitle.empty()) {
			NotesLoader::GetMainAndSubTitlesFromFullTitle(
			  Basename(this->GetSongDir()), m_sMainTitle, m_sSubTitle);
		}

		if (m_sArtist.empty()) {
			m_sArtist = "Unknown artist";
		}
		TranslateTitles();

		m_sMusicPath = GetSongAssetPath(m_sMusicFile, m_sSongDir);
		m_bHasMusic = IsAFile(GetMusicPath());
		m_sBannerPath = GetSongAssetPath(m_sBannerFile, m_sSongDir);
		m_bHasBanner = IsAFile(GetBannerPath());
		m_sBackgroundPath = GetSongAssetPath(m_sBackgroundFile, m_sSongDir);
		m_bHasBackground = IsAFile(GetBackgroundPath());
		for (auto Image : ImageDir) {
			IMAGECACHE->LoadImage(Image, GetCacheFile(Image));
		}

		// There are several things that need to find a file from the dir
		// with a particular extension or type of extension.  So fetch a
		// list of all files in the dir once, then split that list into the
		// different things we need. -Kyz
		vector<std::string> song_dir_listing;
		FILEMAN->GetDirListing(
		  m_sSongDir + "*", song_dir_listing, false, false);
		vector<std::string> music_list;
		vector<std::string> image_list;
		vector<std::string> movie_list;
		vector<std::string> lyric_list;
		vector<std::string> lyric_extensions(1, "lrc");
		// Using a pair didn't work, so these two vectors have to be kept in
		// sync instead. -Kyz
		vector<vector<std::string>*> lists_to_fill;
		vector<const vector<std::string>*> fill_exts;
		lists_to_fill.reserve(4);
		fill_exts.reserve(4);
		lists_to_fill.push_back(&music_list);
		fill_exts.push_back(&ActorUtil::GetTypeExtensionList(FT_Sound));
		lists_to_fill.push_back(&image_list);
		fill_exts.push_back(&ActorUtil::GetTypeExtensionList(FT_Bitmap));
		lists_to_fill.push_back(&movie_list);
		fill_exts.push_back(&ActorUtil::GetTypeExtensionList(FT_Movie));
		lists_to_fill.push_back(&lyric_list);
		fill_exts.push_back(&lyric_extensions);
		for (auto& filename : song_dir_listing) {
			auto matched_something = false;
			auto file_ext = make_lower(GetExtension(filename));
			if (!file_ext.empty()) {
				for (size_t tf = 0; tf < lists_to_fill.size(); ++tf) {
					for (auto ext = fill_exts[tf]->begin();
						 ext != fill_exts[tf]->end();
						 ++ext) {
						if (file_ext == *ext) {
							lists_to_fill[tf]->push_back(filename);
							matched_something = true;
							break;
						}
					}
					if (matched_something) {
						break;
					}
				}
			}
		}

		if (!m_bHasMusic) {
			// If the first song is "intro", and we have more than one
			// available, don't use it--it's probably a KSF intro music
			// file, which we don't (yet) support.
			if (!music_list.empty()) {
				LOG->Trace("Song '%s' points to a music file that doesn't "
						   "exist, found music file '%s'",
						   m_sSongDir.c_str(),
						   music_list[0].c_str());
				m_bHasMusic = true;
				m_sMusicFile = music_list[0];
				m_sMusicPath = GetSongAssetPath(m_sMusicFile, m_sSongDir);
				auto loot = m_sMusicFile.substr(0, 5);
				if (music_list.size() > 1 && (make_lower(loot) != "intro")) {
					m_sMusicFile = music_list[1];
				}
			}
		}

		// This must be done before radar calculation.
		if (m_bHasMusic) {
			std::string error;
			RageSoundReader* Sample =
			  RageSoundReader_FileReader::OpenFile(GetMusicPath(), error);
			/* XXX: Checking if the music file exists eliminates a warning
			 * originating from BMS files (which have no music file, per se)
			 * but it's something of a hack. */
			if (Sample == nullptr && !m_sMusicFile.empty()) {
				LOG->UserLog("Sound file",
							 GetMusicPath(),
							 "couldn't be opened: %s",
							 error.c_str());

				// Don't use this file.
				m_sMusicFile = "";
			} else if (Sample != nullptr) {
				m_fMusicLengthSeconds = Sample->GetLength() / 1000.0f;
				delete Sample;

				if (m_fMusicLengthSeconds < 0) {
					// It failed; bad file or something. It's already logged
					// a warning.
					m_fMusicLengthSeconds = 100; // guess
				} else if (m_fMusicLengthSeconds == 0) {
					LOG->UserLog("Sound file", GetMusicPath(), "is empty.");
				}
			}
		} else // ! HasMusic()
		{
			m_fMusicLengthSeconds = 100; // guess
			LOG->UserLog("Song",
						 GetSongDir(),
						 "has no music file; guessing at %f seconds",
						 m_fMusicLengthSeconds);
		}
		if (m_fMusicLengthSeconds < 0) {
			LOG->UserLog("Sound file",
						 GetMusicPath(),
						 "has a negative length %f.",
						 m_fMusicLengthSeconds);
			m_fMusicLengthSeconds = 0;
		}
		if (!m_PreviewFile.empty() &&
			m_fMusicSampleLengthSeconds <=
			  0.00f) { // if there's a preview file and sample length isn't
					   // specified, set sample length to length of preview
					   // file
			std::string error;
			RageSoundReader* Sample = RageSoundReader_FileReader::OpenFile(
			  GetPreviewMusicPath(), error);
			if (Sample == nullptr && !m_sMusicFile.empty()) {
				LOG->UserLog("Sound file",
							 GetPreviewMusicPath(),
							 "couldn't be opened: %s",
							 error.c_str());

				// Don't use this file.
				m_PreviewFile = "";
				m_fMusicSampleLengthSeconds = DEFAULT_MUSIC_SAMPLE_LENGTH;
			} else if (Sample != nullptr) {
				m_fMusicSampleLengthSeconds = Sample->GetLength() / 1000.0f;
				delete Sample;

				if (m_fMusicSampleLengthSeconds < 0) {
					// It failed; bad file or something. It's already logged
					// a warning.
					m_fMusicSampleLengthSeconds = DEFAULT_MUSIC_SAMPLE_LENGTH;
				} else if (m_fMusicSampleLengthSeconds == 0) {
					LOG->UserLog(
					  "Sound file", GetPreviewMusicPath(), "is empty.");
				}
			}
		} else { // no preview file, calculate sample from music as normal

			if (m_fMusicSampleStartSeconds == -1 ||
				m_fMusicSampleLengthSeconds == 0 ||
				m_fMusicSampleStartSeconds + m_fMusicSampleLengthSeconds >
				  this->m_fMusicLengthSeconds) {
				const auto& timing = this->m_SongTiming;
				m_fMusicSampleStartSeconds = timing.WhereUAtBro(100);

				if (m_fMusicSampleStartSeconds + m_fMusicSampleLengthSeconds >
					this->m_fMusicLengthSeconds) {
					// Attempt to get a reasonable default.
					auto iBeat = static_cast<int>(
					  this->m_SongTiming.GetBeatFromElapsedTime(
						this->GetLastSecond()) /
					  2);
					iBeat -= iBeat % 4;
					m_fMusicSampleStartSeconds =
					  timing.WhereUAtBro(static_cast<float>(iBeat));
				}
			}

			// The old logic meant that you couldn't have sample lengths
			// that go forever, e.g. those in Donkey Konga. I never liked
			// that. -freem
			if (m_fMusicSampleLengthSeconds <= 0.00f) {
				m_fMusicSampleLengthSeconds = DEFAULT_MUSIC_SAMPLE_LENGTH;
			}
		}

		// Here's the problem:  We have a directory full of images. We want
		// to determine which image is the banner, which is the background,
		// and which is the CDTitle.

		// For blank args to FindFirstFilenameContaining. -Kyz
		vector<std::string> empty_list;

		m_PreviewPath = GetSongAssetPath(m_PreviewFile, m_sSongDir);
		if (m_PreviewPath.empty())
			m_PreviewPath = m_sMusicPath;
		m_sLyricsPath = GetSongAssetPath(m_sLyricsFile, m_sSongDir);
		m_sPreviewVidPath = GetSongAssetPath(m_sPreviewVidFile, m_sSongDir);
		// uhh this should like... not be like... this... -mina
		m_sJacketPath = GetSongAssetPath(m_sJacketFile, m_sSongDir);
		auto has_jacket = IsAFile(m_sJacketPath);
		m_sCDPath = GetSongAssetPath(m_sCDFile, m_sSongDir);
		auto has_cdimage = IsAFile(m_sCDPath);
		m_sDiscPath = GetSongAssetPath(m_sDiscFile, m_sSongDir);
		auto has_disc = IsAFile(m_sDiscPath);
		m_sCDTitlePath = GetSongAssetPath(m_sCDTitleFile, m_sSongDir);
		auto has_cdtitle = IsAFile(m_sCDTitlePath);

		// First, check the file name for hints.
		if (!m_bHasBanner) {
			/* If a nonexistant banner file is specified, and we can't find
			 * a replacement, don't wipe out the old value. */
			// m_sBannerFile = "";

			// find an image with "banner" in the file name
			vector<std::string> contains(1, "banner");
			/* Some people do things differently for the sake of being
			 * different. Don't match eg. abnormal, numbness. */
			vector<std::string> ends_with(1, "bn");
			m_bHasBanner = FindFirstFilenameContaining(
			  image_list, m_sBannerFile, empty_list, contains, ends_with);
			if (m_bHasBanner)
				m_sBannerPath = GetSongAssetPath(m_sBannerFile, m_sSongDir);
		}

		if (!m_bHasBackground) {
			// m_sBackgroundFile = "";

			// find an image with "bg" or "background" in the file name
			vector<std::string> contains(1, "background");
			vector<std::string> ends_with(1, "bg");
			m_bHasBackground = FindFirstFilenameContaining(
			  image_list, m_sBackgroundFile, empty_list, contains, ends_with);
			if (m_bHasBackground)
				m_sBackgroundPath =
				  GetSongAssetPath(m_sBackgroundFile, m_sSongDir);
		}

		if (!has_jacket) {
			// find an image with "jacket" or "albumart" in the filename.
			vector<std::string> starts_with(1, "jk_");
			vector<std::string> contains;
			contains.reserve(2);
			contains.push_back("jacket");
			contains.push_back("albumart");
			has_jacket = FindFirstFilenameContaining(
			  image_list, m_sJacketFile, starts_with, contains, empty_list);
			if (has_jacket)
				m_sJacketPath = GetSongAssetPath(m_sJacketFile, m_sSongDir);
		}

		if (!has_cdimage) {
			// CD image, a la ddr 1st-3rd (not to be confused with CDTitles)
			// find an image with "-cd" at the end of the filename.
			vector<std::string> ends_with(1, "-cd");
			has_cdimage = FindFirstFilenameContaining(
			  image_list, m_sCDFile, empty_list, empty_list, ends_with);
			if (has_cdimage)
				m_sCDPath = GetSongAssetPath(m_sCDFile, m_sSongDir);
		}

		if (!has_disc) {
			// a rectangular graphic, not to be confused with CDImage above.
			vector<std::string> ends_with;
			ends_with.reserve(2);
			ends_with.push_back(" disc");
			ends_with.push_back(" title");
			has_disc = FindFirstFilenameContaining(
			  image_list, m_sDiscFile, empty_list, empty_list, ends_with);
			if (has_disc)
				m_sDiscPath = GetSongAssetPath(m_sDiscPath, m_sSongDir);
		}

		if (!has_cdtitle) {
			// find an image with "cdtitle" in the file name
			vector<std::string> contains(1, "cdtitle");
			has_cdtitle = FindFirstFilenameContaining(
			  image_list, m_sCDTitleFile, empty_list, contains, empty_list);
			if (has_cdtitle)
				m_sCDTitlePath = GetSongAssetPath(m_sCDTitleFile, m_sSongDir);
		}

		if (!HasLyrics()) {
			// Check if there is a lyric file in here
			if (!lyric_list.empty()) {
				m_sLyricsFile = lyric_list[0];
			}
			if (!m_sLyricsFile.empty())
				m_sLyricsPath = GetSongAssetPath(m_sLyricsFile, m_sSongDir);
		}

		/* Now, For the images we still haven't found,
		 * look at the image dimensions of the remaining unclassified
		 * images. */
		for (auto& i : image_list) // foreach image
		{
			if (m_bHasBanner && m_bHasBackground && has_cdtitle)
				break; // done

			// ignore DWI "-char" graphics
			auto lower = i;
			MakeLower(lower);
			if (BlacklistedImages.find(lower) != BlacklistedImages.end())
				continue; // skip

			// Skip any image that we've already classified

			if (m_bHasBanner && EqualsNoCase(m_sBannerFile, i))
				continue; // skip

			if (m_bHasBackground && EqualsNoCase(m_sBackgroundFile, i))
				continue; // skip

			if (has_cdtitle && EqualsNoCase(m_sCDTitleFile, i))
				continue; // skip

			if (has_jacket && EqualsNoCase(m_sJacketFile, i))
				continue; // skip

			if (has_disc && EqualsNoCase(m_sDiscFile, i))
				continue; // skip

			if (has_cdimage && EqualsNoCase(m_sCDFile, i))
				continue; // skip

			auto sPath = m_sSongDir + i;

			// We only care about the dimensions.
			std::string error;
			auto* img = RageSurfaceUtils::LoadFile(sPath, error, true);
			if (!img) {
				LOG->UserLog("Graphic file",
							 sPath,
							 "couldn't be loaded: %s",
							 error.c_str());
				continue;
			}

			const auto width = img->w;
			const auto height = img->h;
			delete img;

			if (!m_bHasBackground && width >= 320 && height >= 240) {
				m_sBackgroundFile = i;
				m_bHasBackground = true;
				m_sBackgroundPath =
				  GetSongAssetPath(m_sBackgroundFile, m_sSongDir);
				continue;
			}

			if (!m_bHasBanner && 100 <= width && width <= 320 && 50 <= height &&
				height <= 240) {
				m_sBannerFile = i;
				m_bHasBanner = true;
				m_sBannerPath = GetSongAssetPath(m_sBannerFile, m_sSongDir);
				continue;
			}

			/* Some songs have overlarge banners. Check if the ratio is
			 * reasonable (over 2:1; usually over 3:1), and large (not a
			 * cdtitle). */
			if (!m_bHasBanner && width > 200 &&
				static_cast<float>(width) / height > 2.0f) {
				m_sBannerFile = i;
				m_bHasBanner = true;
				m_sBannerPath = GetSongAssetPath(m_sBannerFile, m_sSongDir);
				continue;
			}

			/* Agh. DWI's inline title images are triggering this, resulting
			 * in kanji, etc., being used as a CDTitle for songs with none.
			 * Some sample data from random incarnations: 42x50 35x50 50x50
			 * 144x49 It looks like ~50 height is what people use to align
			 * to DWI's font.
			 *
			 * My tallest CDTitle is 44. Let's cut off in the middle and
			 * hope for the best. -(who? -aj) */
			/* The proper size of a CDTitle is 64x48 or sometihng. Simfile
			 * artists typically don't give a shit about this (see Cetaka's
			 * fucking banner -sized CDTitle). This is also subverted in
			 * certain designs (beta Mungyodance 3 simfiles, for instance,
			 * used the CDTitle to hold various information about the song
			 * in question). As it stands, I'm keeping this code until I
			 * figure out wtf to do -aj
			 */
			if (!has_cdtitle && width <= 100 && height <= 48) {
				m_sCDTitleFile = i;
				has_cdtitle = true;
				m_sCDTitlePath = GetSongAssetPath(m_sCDTitleFile, m_sSongDir);
				continue;
			}

			// Jacket files typically have the same width and height.
			if (!has_jacket && width == height) {
				m_sJacketFile = i;
				has_jacket = true;
				m_sJacketPath = GetSongAssetPath(m_sJacketFile, m_sSongDir);
				continue;
			}

			// Disc images are typically rectangular; make sure we have a
			// banner already.
			if (!has_disc && (width > height) && m_bHasBanner) {
				if (i != m_sBannerFile) {
					m_sDiscFile = i;
					has_disc = true;
				}
				m_sDiscPath = GetSongAssetPath(m_sDiscFile, m_sSongDir);
				continue;
			}

			// CD images are the same as Jackets, typically the same width
			// and height
			if (!has_cdimage && width == height) {
				m_sCDFile = i;
				has_cdimage = true;
				m_sCDPath = GetSongAssetPath(m_sCDFile, m_sSongDir);
				continue;
			}
		}
		// If no BGChanges are specified and there are movies in the song
		// directory, then assume they are DWI style where the movie begins
		// at beat 0.
		if (!HasBGChanges()) {
			/* Use this->GetBeatFromElapsedTime(0) instead of 0 to start
			 * when the music starts. */
			if (movie_list.size() == 1) {
				this->AddBackgroundChange(
				  BACKGROUND_LAYER_1,
				  BackgroundChange(
					0, movie_list[0], "", 1.f, SBE_StretchNoLoop));
			}
		}

		// Don't allow multiple Steps of the same StepsType and Difficulty
		// (except for edits). We should be able to use difficulty names as
		// unique identifiers for steps. */
		SongUtil::AdjustDuplicateSteps(this);

		// Clear fields for files that turned out to not exist.
#define CLEAR_NOT_HAS(has_name, field_name, field_name2)                       \
	if (!(has_name)) {                                                         \
		(field_name) = "";                                                     \
		(field_name2) = "";                                                    \
	}
		CLEAR_NOT_HAS(m_bHasBanner, m_sBannerFile, m_sBannerPath);
		CLEAR_NOT_HAS(m_bHasBackground, m_sBackgroundFile, m_sBackgroundPath);
		CLEAR_NOT_HAS(has_jacket, m_sJacketFile, m_sJacketPath);
		CLEAR_NOT_HAS(has_cdimage, m_sCDFile, m_sCDPath);
		CLEAR_NOT_HAS(has_disc, m_sDiscFile, m_sDiscPath);
		CLEAR_NOT_HAS(has_cdtitle, m_sCDTitleFile, m_sCDTitlePath);
#undef CLEAR_NOT_HAS
	}

	auto scoot = m_sMainTitle;
	auto mcgoot = m_sSubTitle;

	if (!mcgoot.empty())
		scoot += " " + mcgoot;
	displayfulltitle = scoot;

	auto doot = m_sMainTitleTranslit;
	auto loot = m_sSubTitleTranslit;

	if (!loot.empty())
		doot += " " + loot;
	translitfulltitle = doot;
	/* Generate these before we autogen notes, so the new notes can inherit
	 * their source's values. */
	ReCalculateRadarValuesAndLastSecond(from_cache, true, calc);
	// If the music length is suspiciously shorter than the last second,
	// adjust the length.  This prevents the ogg patch from setting a false
	// length. -Kyz
	if (m_fMusicLengthSeconds < lastSecond - 10.0f) {
		m_fMusicLengthSeconds = lastSecond;
	}
}

void
Song::TranslateTitles()
{
	static TitleSubst tsub("Songs");

	TitleFields title;
	title.LoadFromStrings(m_sMainTitle,
						  m_sSubTitle,
						  m_sArtist,
						  m_sMainTitleTranslit,
						  m_sSubTitleTranslit,
						  m_sArtistTranslit);
	tsub.Subst(title);
	title.SaveToStrings(m_sMainTitle,
						m_sSubTitle,
						m_sArtist,
						m_sMainTitleTranslit,
						m_sSubTitleTranslit,
						m_sArtistTranslit);
}

void
Song::ReCalculateRadarValuesAndLastSecond(bool fromCache,
										  bool duringCache,
										  Calc* calc)
{
	if (fromCache)
		return;

	auto localFirst = FLT_MAX; // inf
	// Make sure we're at least as long as the specified amount below.
	auto localLast = this->specifiedLastSecond;

	if (duringCache) {
		for (auto& n : m_vpSteps) {
			// Cache etterna stuff and 'radar values'

			// Skip difficulties without notes
			if (n->IsNoteDataEmpty())
				continue;

			// only ever decompress the notedata when writing the cache file
			// for this we don't use the etterna compressed format -mina
			n->Decompress();

			// calc etterna metadata will replace the unwieldy notedata
			// string with a compressed format for both cache and internal
			// use but not yet
			n->CalcEtternaMetadata(calc);
			n->CalculateRadarValues();

			// calculate lastSecond
			localFirst = std::min(localFirst, n->firstsecond);
			localLast = std::max(localLast, n->lastsecond);
		}
	}

	firstSecond = localFirst;
	lastSecond = localLast;
}

// Return whether the song is playable in the given style.
bool
Song::SongCompleteForStyle(const Style* st) const
{
	return HasStepsType(st->m_StepsType);
}

bool
Song::HasStepsType(StepsType st) const
{
	return SongUtil::GetOneSteps(this, st) != nullptr;
}

bool
Song::HasStepsTypeAndDifficulty(StepsType st, Difficulty dc) const
{
	return SongUtil::GetOneSteps(this, st, dc) != nullptr;
}

void
Song::Save()
{
	SONGINDEX->DeleteSongFromDBByDir(GetSongDir());
	LOG->Trace("Song::SaveToSongFile()");

	ReCalculateRadarValuesAndLastSecond();
	TranslateTitles();

	vector<std::string> backedDotOldFileNames;
	vector<std::string> backedOrigFileNames;
	vector<std::string> arrayOldFileNames;
	GetDirListing(m_sSongDir + "*.bms", arrayOldFileNames);
	GetDirListing(m_sSongDir + "*.pms", arrayOldFileNames);
	GetDirListing(m_sSongDir + "*.ksf", arrayOldFileNames);
	GetDirListing(m_sSongDir + "*.sm", arrayOldFileNames);
	GetDirListing(m_sSongDir + "*.dwi", arrayOldFileNames);
	for (auto& arrayOldFileName : arrayOldFileNames) {
		const auto sOldPath = m_sSongDir + arrayOldFileName;
		const auto sNewPath = sOldPath + ".old";

		if (!FileCopy(sOldPath, sNewPath)) {
			LOG->UserLog("Song file", sOldPath, "couldn't be backed up.");
		} else {
			backedDotOldFileNames.emplace_back(sNewPath);
			backedOrigFileNames.emplace_back(sOldPath);
		}
	}

	// Save the new files. These calls make backups on their own.
	if (!SaveToSSCFile(GetSongFilePath(), false)) {
		for (auto fileName : backedDotOldFileNames)
			FILEMAN->Remove(fileName);
		return;
	}
	for (auto fileName : backedOrigFileNames)
		FILEMAN->Remove(fileName);

	SaveToCacheFile();
	FILEMAN->FlushDirCache(GetSongDir());
}

bool
Song::SaveToSMFile()
{
	const auto sPath = SetExtension(GetSongFilePath(), "sm");
	LOG->Trace("Song::SaveToSMFile(%s)", sPath.c_str());

	// If the file exists, make a backup.
	if (IsAFile(sPath))
		FileCopy(sPath, sPath + ".old");

	const auto vpStepsToSave = GetStepsToSave();

	return NotesWriterSM::Write(sPath, *this, vpStepsToSave);
}

vector<Steps*>
Song::GetStepsToSave(bool bSavingCache, const std::string& path)
{
	vector<Steps*> vpStepsToSave;
	for (auto& s : m_vpSteps) {

		if (!bSavingCache)
			s->SetFilename(path);
		vpStepsToSave.push_back(s);
	}
	for (auto& s : m_UnknownStyleSteps) {
		vpStepsToSave.push_back(s);
	}
	return vpStepsToSave;
}

bool
Song::SaveToSSCFile(const std::string& sPath, bool bSavingCache)
{
	auto path = sPath;
	if (!bSavingCache)
		path = SetExtension(sPath, "ssc");

	LOG->Trace("Song::SaveToSSCFile('%s')", path.c_str());

	// If the file exists, make a backup.
	if (!bSavingCache && IsAFile(path)) {
		FileCopy(path, path + ".old");
		m_sSongFileName = path;
	}

	auto vpStepsToSave = GetStepsToSave(bSavingCache, sPath);

	if (!bSavingCache)
		for (auto* s : vpStepsToSave) {
			s->Decompress();
			s->CalcEtternaMetadata(SONGMAN->calc.get());
			s->SetFilename(path);
		}
	if (bSavingCache) {
		return SONGINDEX->CacheSong(*this, path);
	}

	if (!NotesWriterSSC::Write(path, *this, vpStepsToSave, bSavingCache))
		return false;

	if (g_BackUpAllSongSaves.Get()) {
		const auto sExt = GetExtension(path);
		auto sBackupFile = SetExtension(path, "");

		time_t cur_time;
		time(&cur_time);
		struct tm now;
		localtime_r(&cur_time, &now);

		sBackupFile += ssprintf("-%04i-%02i-%02i--%02i-%02i-%02i",
								1900 + now.tm_year,
								now.tm_mon + 1,
								now.tm_mday,
								now.tm_hour,
								now.tm_min,
								now.tm_sec);
		sBackupFile = SetExtension(sBackupFile, sExt);
		sBackupFile += ssprintf(".old");

		if (FileCopy(path, sBackupFile))
			LOG->Trace("Backed up %s to %s", path.c_str(), sBackupFile.c_str());
		else
			LOG->Trace(
			  "Failed to back up %s to %s", path.c_str(), sBackupFile.c_str());
	}

	// Mark these steps saved to disk.
	for (auto& s : vpStepsToSave)
		s->SetSavedToDisk(true);

	return true;
}

bool
Song::SaveToETTFile(const std::string& sPath, bool bSavingCache)
{
	auto path = sPath;
	if (!bSavingCache)
		path = SetExtension(sPath, "ett");

	LOG->Trace("Song::SaveToETTFile('%s')", path.c_str());

	// If the file exists, make a backup.
	if (!bSavingCache && IsAFile(path))
		FileCopy(path, path + ".old");

	auto vpStepsToSave = GetStepsToSave(bSavingCache, sPath);

	if (bSavingCache) {
		return NotesWriterETT::Write(path, *this, vpStepsToSave);
	}

	if (!NotesWriterETT::Write(path, *this, vpStepsToSave))
		return false;

	if (g_BackUpAllSongSaves.Get()) {
		const auto sExt = GetExtension(path);
		auto sBackupFile = SetExtension(path, "");

		time_t cur_time;
		time(&cur_time);
		struct tm now;
		localtime_r(&cur_time, &now);

		sBackupFile += ssprintf("-%04i-%02i-%02i--%02i-%02i-%02i",
								1900 + now.tm_year,
								now.tm_mon + 1,
								now.tm_mday,
								now.tm_hour,
								now.tm_min,
								now.tm_sec);
		sBackupFile = SetExtension(sBackupFile, sExt);
		sBackupFile += ssprintf(".old");

		if (FileCopy(path, sBackupFile))
			LOG->Trace("Backed up %s to %s", path.c_str(), sBackupFile.c_str());
		else
			LOG->Trace(
			  "Failed to back up %s to %s", path.c_str(), sBackupFile.c_str());
	}

	// Mark these steps saved to disk.
	for (auto& s : vpStepsToSave)
		s->SetSavedToDisk(true);

	return true;
}

bool
Song::SaveToCacheFile()
{
	if (SONGMAN->IsGroupNeverCached(m_sGroupName)) {
		return true;
	}
	return SONGINDEX->CacheSong(*this, m_sSongDir);
}

bool
Song::SaveToDWIFile()
{
	const auto sPath = SetExtension(GetSongFilePath(), "dwi");
	LOG->Trace("Song::SaveToDWIFile(%s)", sPath.c_str());

	// If the file exists, make a backup.
	if (IsAFile(sPath))
		FileCopy(sPath, sPath + ".old");

	return NotesWriterDWI::Write(sPath, *this);
}

// We want to return a filename, We use this function for that.
std::string
Song::GetCacheFile(const std::string& sType)
{
	// We put the Predefined images into a map.
	std::map<std::string, std::string> PreDefs;
	PreDefs["Banner"] = GetBannerPath();
	PreDefs["Background"] = GetBackgroundPath();
	PreDefs["CDTitle"] = GetCDTitlePath();
	PreDefs["Jacket"] = GetJacketPath();
	PreDefs["CDImage"] = GetCDImagePath();
	PreDefs["Disc"] = GetDiscPath();

	// Check if Predefined images exist, And return function if they do.
	if (PreDefs[sType].c_str()) // pretty sure this evaluates to true
								// even if the string is
								// "", but haven't tested extensively
		return PreDefs[sType.c_str()];

	// Get all image files and put them into a vector.
	vector<std::string> song_dir_listing;
	FILEMAN->GetDirListing(m_sSongDir + "*", song_dir_listing, false, false);
	vector<std::string> image_list;
	auto fill_exts = ActorUtil::GetTypeExtensionList(FT_Bitmap);
	for (auto Image : song_dir_listing) {
		auto FileExt = GetExtension(Image);
		std::transform(
		  FileExt.begin(), FileExt.end(), FileExt.begin(), tolower);
		for (auto FindExt : fill_exts) {
			if (FileExt == FindExt)
				image_list.push_back(Image);
		}
	}

	// Create a map that contains all the filenames to search for.
	std::map<std::string, std::map<int, std::string>> PreSets;
	PreSets["Banner"][1] = "bn";
	PreSets["Banner"][2] = "banner";
	PreSets["Background"][1] = "bg";
	PreSets["Background"][2] = "background";
	PreSets["CDTitle"][1] = "cdtitle";
	PreSets["Jacket"][1] = "jk_";
	PreSets["Jacket"][2] = "jacket";
	PreSets["Jacket"][3] = "albumart";
	PreSets["CDImage"][1] = "-cd";
	PreSets["Disc"][1] = " disc";
	PreSets["Disc"][2] = " title";

	for (auto Image : image_list) {
		// We want to make it lower case.
		std::transform(Image.begin(), Image.end(), Image.begin(), tolower);
		for (auto PreSet : PreSets[sType]) {
			// Search for image using PreSets.
			const auto Found = Image.find(PreSet.second);
			if (Found != std::string::npos)
				return GetSongAssetPath(Image, m_sSongDir);
		}
		// Search for the image directly if it doesnt exist in PreSets,
		// Or incase we define our own stuff.
		const auto Found = Image.find(sType);
		if (Found != std::string::npos)
			return GetSongAssetPath(Image, m_sSongDir);
	}
	// Return empty if nothing found.
	return "";
}

std::string
Song::GetFileHash()
{
	if (m_sFileHash.empty()) {
		auto sPath = SetExtension(GetSongFilePath(), "sm");
		if (!IsAFile(sPath))
			sPath = SetExtension(GetSongFilePath(), "dwi");
		if (!IsAFile(sPath))
			sPath = SetExtension(GetSongFilePath(), "sma");
		if (!IsAFile(sPath))
			sPath = SetExtension(GetSongFilePath(), "bms");
		if (!IsAFile(sPath))
			sPath = SetExtension(GetSongFilePath(), "ksf");
		if (!IsAFile(sPath))
			sPath = SetExtension(GetSongFilePath(), "json");
		if (!IsAFile(sPath))
			sPath = SetExtension(GetSongFilePath(), "jso");
		if (!IsAFile(sPath))
			sPath = SetExtension(GetSongFilePath(), "ssc");
		if (IsAFile(sPath))
			m_sFileHash = BinaryToHex(CryptManager::GetSHA1ForFile(sPath));
		else
			m_sFileHash = "";
	}
	return m_sFileHash;
}

bool
Song::HasEdits(StepsType st) const
{
	for (auto* pSteps : m_vpSteps) {
		if (pSteps->m_StepsType == st &&
			pSteps->GetDifficulty() == Difficulty_Edit) {
			return true;
		}
	}

	return false;
}
// Hack: see Song::TidyUpData comments.
bool
Song::HasMusic() const
{
	// If we have keys, we always have music.
	if (!m_vsKeysoundFile.empty())
		return true;

	return !m_sMusicFile.empty() && !GetMusicPath().empty();
}
bool
Song::HasBanner() const
{
	return !m_sBannerFile.empty() && !GetBannerPath().empty();
}
bool
Song::HasInstrumentTrack(InstrumentTrack it) const
{
	return !m_sInstrumentTrackFile[it].empty() &&
		   !GetInstrumentTrackPath(it).empty();
}
bool
Song::HasLyrics() const
{
	return !m_sLyricsFile.empty() && !GetLyricsPath().empty();
}
bool
Song::HasBackground() const
{
	return !m_sBackgroundFile.empty() && !GetBackgroundPath().empty();
}
bool
Song::HasCDTitle() const
{
	return !m_sCDTitleFile.empty() && !GetCDTitlePath().empty();
}
bool
Song::HasBGChanges() const
{
	FOREACH_BackgroundLayer(i)
	{
		if (!GetBackgroundChanges(i).empty())
			return true;
	}
	return false;
}
bool
Song::HasJacket() const
{
	return !m_sJacketFile.empty() && !GetJacketPath().empty();
}
bool
Song::HasDisc() const
{
	return !m_sDiscFile.empty() && !GetDiscPath().empty();
}
bool
Song::HasCDImage() const
{
	return !m_sCDFile.empty() && !GetCDImagePath().empty();
}
bool
Song::HasPreviewVid() const
{
	return !m_sPreviewVidFile.empty() && !GetPreviewVidPath().empty();
}

const vector<BackgroundChange>&
Song::GetBackgroundChanges(BackgroundLayer bl) const
{
	return *(m_BackgroundChanges[bl]);
}
vector<BackgroundChange>&
Song::GetBackgroundChanges(BackgroundLayer bl)
{
	return *(m_BackgroundChanges[bl].Get());
}

const vector<BackgroundChange>&
Song::GetForegroundChanges() const
{
	return *m_ForegroundChanges;
}
vector<BackgroundChange>&
Song::GetForegroundChanges()
{
	return *m_ForegroundChanges.Get();
}

vector<std::string>
Song::GetChangesToVectorString(const vector<BackgroundChange>& changes) const
{
	vector<std::string> ret;
	for (const auto& bgc : changes) {
		ret.push_back(bgc.ToString());
	}
	return ret;
}

vector<std::string>
Song::GetBGChanges1ToVectorString() const
{
	return this->GetChangesToVectorString(
	  this->GetBackgroundChanges(BACKGROUND_LAYER_1));
}

vector<std::string>
Song::GetBGChanges2ToVectorString() const
{
	return this->GetChangesToVectorString(
	  this->GetBackgroundChanges(BACKGROUND_LAYER_2));
}

vector<std::string>
Song::GetFGChanges1ToVectorString() const
{
	return this->GetChangesToVectorString(this->GetForegroundChanges());
}

vector<std::string>
Song::GetInstrumentTracksToVectorString() const
{
	vector<std::string> ret;
	FOREACH_ENUM(InstrumentTrack, it)
	{
		if (this->HasInstrumentTrack(it)) {
			ret.push_back(std::string(InstrumentTrackToString(it) + "=" +
									  this->m_sInstrumentTrackFile[it]));
		}
	}
	return ret;
}

std::string
Song::GetSongAssetPath(std::string sPath, const std::string& sSongPath)
{
	if (sPath.empty())
		return std::string();

	auto sRelPath = sSongPath + sPath;
	if (DoesFileExist(sRelPath))
		return sRelPath;

	/* If there's no path in the file, the file is in the same directory as
	 * the song. (This is the preferred configuration.) */
	if (sPath.find('/') == std::string::npos)
		return sRelPath;

	// The song contains a path; treat it as relative to the top SM
	// directory.
	if (sPath.substr(0, 3) == "../") {
		// The path begins with "../".  Resolve it wrt. the song directory.
		sPath = sRelPath;
	}

	CollapsePath(sPath);

	/* If the path still begins with "../", then there were an unreasonable
	 * number of them at the beginning of the path. Ignore the path
	 * entirely. */
	if (sPath.substr(0, 3) == "../")
		return std::string();

	return sPath;
}

float
Song::GetPreviewStartSeconds() const
{
	if (m_PreviewFile.empty()) {
		return m_fMusicSampleStartSeconds;
	}
	return 0.0f;
}

const vector<Steps*>
Song::GetChartsOfCurrentGameMode() const
{
	std::vector<StepsType> types;
	GAMEMAN->GetStepsTypesForGame(GAMESTATE->m_pCurGame, types);

	vector<Steps*> steps;
	for (auto type : types) {
		auto tmp = GetStepsByStepsType(type);
		steps.insert(steps.end(), tmp.begin(), tmp.end());
	}
	return steps;
}

float
Song::HighestMSDOfSkillset(Skillset skill, float rate) const
{
	CLAMP(rate, 0.7f, 2.f);
	auto highest = 0.f;

	const auto charts = GetChartsOfCurrentGameMode();

	for (auto* chart : charts) {
		const auto current = chart->GetMSD(rate, skill);
		if (current > highest)
			highest = current;
	}

	return highest;
}

bool
Song::IsSkillsetHighestOfAnySteps(Skillset ss, float rate) const
{
	auto vsteps = GetAllSteps();
	for (auto& steps : vsteps) {
		auto sortedstuffs = steps->SortSkillsetsAtRate(rate, false);
		if (sortedstuffs[0].first == ss)
			return true;
	}

	return false;
}

bool
Song::MatchesFilter(const float rate) const
{
	auto charts = GetChartsOfCurrentGameMode();

	for (auto* const chart : charts) {
		// Iterate over all maps of the given type
		auto addsong = FILTERMAN->ExclusiveFilter;
		/* The default behaviour of an exclusive filter is to accept
		 * by default, (i.e. addsong=true) and reject if any
		 * filters fail. The default behaviour of a non-exclusive filter is
		 * the exact opposite: reject by default (i.e.
		 * addsong=false), and accept if any filters match.
		 */
		for (auto ss = 0; ss < NUM_Skillset + 1; ss++) {
			// Iterate over all skillsets, up to and
			// including the placeholder NUM_Skillset
			const auto lb = FILTERMAN->SSFilterLowerBounds[ss];
			const auto ub = FILTERMAN->SSFilterUpperBounds[ss];
			if (lb > 0.f || ub > 0.f) { // If either bound is active, continue

				if (!FILTERMAN->ExclusiveFilter) { // Non-Exclusive filter
					if (FILTERMAN->HighestSkillsetsOnly)
						if (!IsSkillsetHighestOfAnySteps(
							  static_cast<Skillset>(ss), rate) &&
							ss < NUM_Skillset) // The current skill is not
											   // in highest in the chart
							continue;
				}
				float val;
				if (ss < NUM_Skillset)
					val = chart->GetMSD(rate, ss);
				else {
					// If we are on the placeholder skillset, look at song
					// length instead of a skill
					auto val = step->GetLengthSeconds(rate);
				}
				if (FILTERMAN->ExclusiveFilter) {
					/* Our behaviour is to accept by default,
					 * but reject if any filters don't match.*/
					if ((val < lb && lb > 0.f) || (val > ub && ub > 0.f)) {
						/* If we're below the lower bound and it's set,
						 * or above the upper bound and it's set*/
						addsong = false;
						break;
					}
				} else { // Non-Exclusive Filter
					/* Our behaviour is to reject by default,
					 * but accept if any filters match.*/
					if ((val > lb || !(lb > 0.f)) &&
						(val < ub || !(ub > 0.f))) {
						/* If we're above the lower bound or it's not set
						 * and also below the upper bound or it isn't set*/
						addsong = true;
						break;
					}
				}
			}
		}
		if (addsong)
			return true;
	}
	return false;
}

bool
Song::HasChartByHash(const std::string& hash)
{
	auto vsteps = GetAllSteps();
	for (auto& steps : vsteps) {
		if (steps->GetChartKey() == hash)
			return true;
	}
	return false;
}

const std::string&
Song::GetDisplayMainTitle() const
{
	if (!PREFSMAN->m_bShowNativeLanguage)
		return GetTranslitMainTitle();
	return m_sMainTitle;
}

const std::string&
Song::GetDisplaySubTitle() const
{
	if (!PREFSMAN->m_bShowNativeLanguage)
		return GetTranslitSubTitle();
	return m_sSubTitle;
}

const std::string&
Song::GetDisplayArtist() const
{
	if (!PREFSMAN->m_bShowNativeLanguage)
		return GetTranslitArtist();
	return m_sArtist;
}

const std::string&
Song::GetMainTitle() const
{
	return m_sMainTitle;
}

void
Song::AddSteps(Steps* pSteps)
{
	// Songs of unknown stepstype are saved as a forwards compatibility
	// feature so that editing a simfile made by a future version that has a
	// new style won't delete those steps. -Kyz
	if (pSteps->m_StepsType != StepsType_Invalid) {

		m_vpSteps.push_back(pSteps);
		ASSERT_M(pSteps->m_StepsType < NUM_StepsType,
				 ssprintf("%i", pSteps->m_StepsType));
		m_vpStepsByType[pSteps->m_StepsType].push_back(pSteps);
	} else {
		m_UnknownStyleSteps.push_back(pSteps);
	}
}

void
Song::DeleteSteps(const Steps* pSteps, bool bReAutoGen)
{
	auto& vpSteps = m_vpStepsByType[pSteps->m_StepsType];
	for (int j = vpSteps.size() - 1; j >= 0; j--) {
		if (vpSteps[j] == pSteps) {
			// delete vpSteps[j]; // delete below
			vpSteps.erase(vpSteps.begin() + j);
			break;
		}
	}

	for (int j = m_vpSteps.size() - 1; j >= 0; j--) {
		if (m_vpSteps[j] == pSteps) {
			delete m_vpSteps[j];
			m_vpSteps.erase(m_vpSteps.begin() + j);
			break;
		}
	}
}

bool
Song::Matches(const std::string& sGroup, const std::string& sSong) const
{
	if (!sGroup.empty() && CompareNoCase(sGroup, this->m_sGroupName) != 0)
		return false;

	auto sDir = this->GetSongDir();
	auto voop = sDir;
	s_replace(voop, "\\", "/");
	sDir = voop;
	vector<std::string> bits;
	split(sDir, "/", bits);
	ASSERT(bits.size() >= 2); // should always have at least two parts
	const auto& sLastBit = bits[bits.size() - 1];

	// match on song dir or title (ala DWI)
	if (!CompareNoCase(sSong, sLastBit))
		return true;
	if (!CompareNoCase(sSong, this->GetTranslitFullTitle()))
		return true;

	return false;
}

bool
Song::IsEditAlreadyLoaded(Steps* pSteps) const
{
	ASSERT_M(pSteps->GetDifficulty() == Difficulty_Edit,
			 ssprintf("The %s chart for %s is no edit, thus it can't be "
					  "checked for loading.",
					  DifficultyToString(pSteps->GetDifficulty()).c_str(),
					  this->m_sMainTitle.c_str()));

	for (auto* pOther : m_vpSteps) {
		if (pOther->GetDifficulty() == Difficulty_Edit &&
			pOther->m_StepsType == pSteps->m_StepsType &&
			pOther->GetHash() == pSteps->GetHash()) {
			return true;
		}
	}

	return false;
}

bool
Song::IsStepsUsingDifferentTiming(Steps* pSteps) const
{
	// XXX This no longer depends on Song at all
	return !pSteps->m_Timing.empty();
}

void
Song::UnloadAllCalcDebugOutput()
{
	for (auto* s : m_vpSteps)
		s->UnloadCalcDebugOutput();
}

bool
Song::AnyChartUsesSplitTiming() const
{
	for (const auto& s : m_vpSteps) {
		if (!s->m_Timing.empty()) {
			return true;
		}
	}
	return false;
}

bool
Song::HasSignificantBpmChangesOrStops() const
{
	if (m_SongTiming.HasStops() || m_SongTiming.HasDelays())
		return true;

	// Don't consider BPM changes that only are only for maintaining sync as
	// a real BpmChange.
	if (m_DisplayBPMType == DISPLAY_BPM_SPECIFIED) {
		if (m_fSpecifiedBPMMin != m_fSpecifiedBPMMax)
			return true;
	} else if (m_SongTiming.HasBpmChanges()) {
		return true;
	}

	return false;
}

float
Song::GetStepsSeconds() const
{
	return this->GetLastSecond() - this->GetFirstSecond();
}

bool
Song::IsLong() const
{
	return !IsMarathon() && m_fMusicLengthSeconds >= g_fLongVerSongSeconds;
}

bool
Song::IsMarathon() const
{
	return m_fMusicLengthSeconds >= g_fMarathonVerSongSeconds;
}

void
Song::PlaySampleMusicExtended()
{
	GameSoundManager::PlayMusicParams PlayParams;
	PlayParams.sFile = GetMusicPath();
	PlayParams.pTiming = nullptr;
	PlayParams.bForceLoop = true;
	PlayParams.fStartSecond = m_fMusicSampleStartSeconds;
	PlayParams.fLengthSeconds =
	  GetLastSecond() - m_fMusicSampleStartSeconds + 2.f;
	PlayParams.fFadeOutLengthSeconds = 1.f;
	PlayParams.bAlignBeat = true;
	PlayParams.bApplyMusicRate = true;
	PlayParams.bAccurateSync = true;

	GameSoundManager::PlayMusicParams FallbackMusic;
	FallbackMusic.sFile = "";
	FallbackMusic.fFadeInLengthSeconds = 1.f;
	FallbackMusic.bAlignBeat = true;
	FallbackMusic.bAccurateSync = true;

	if (PlayParams.fLengthSeconds <
		3.f) { // if the songpreview is after the last note
		PlayParams.fStartSecond =
		  5.f; // chartpreview wont play, just set it near the start -mina
		PlayParams.fLengthSeconds = GetLastSecond() + 2.f;
	}
	SOUND->PlayMusic(PlayParams, FallbackMusic);
	GAMESTATE->SetPaused(false);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the Song. */
class LunaSong : public Luna<Song>
{
  public:
	static int GetDisplayFullTitle(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetDisplayFullTitle().c_str());
		return 1;
	}
	static int GetTranslitFullTitle(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetTranslitFullTitle().c_str());
		return 1;
	}
	static int GetDisplayMainTitle(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetDisplayMainTitle().c_str());
		return 1;
	}
	static int GetMainTitle(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetDisplayMainTitle().c_str());
		return 1;
	}
	static int GetTranslitMainTitle(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetTranslitMainTitle().c_str());
		return 1;
	}
	static int GetDisplaySubTitle(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetDisplaySubTitle().c_str());
		return 1;
	}
	static int GetTranslitSubTitle(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetTranslitSubTitle().c_str());
		return 1;
	}
	static int GetDisplayArtist(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetDisplayArtist().c_str());
		return 1;
	}
	static int GetTranslitArtist(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetTranslitArtist().c_str());
		return 1;
	}
	static int GetGenre(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sGenre.c_str());
		return 1;
	}
	static int GetOrigin(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sOrigin.c_str());
		return 1;
	}
	static int GetAllSteps(T* p, lua_State* L)
	{
		const auto& v = p->GetAllSteps();
		LuaHelpers::CreateTableFromArray<Steps*>(v, L);
		return 1;
	}
	static int GetStepsByStepsType(T* p, lua_State* L)
	{
		const auto st = Enum::Check<StepsType>(L, 1);
		const auto& v = p->GetStepsByStepsType(st);
		LuaHelpers::CreateTableFromArray<Steps*>(v, L);
		return 1;
	}
	static int GetSongDir(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetSongDir().c_str());
		return 1;
	}
	static int GetMusicPath(T* p, lua_State* L)
	{
		const auto s = p->GetMusicPath();
		if (!s.empty())
			lua_pushstring(L, s.c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetBannerPath(T* p, lua_State* L)
	{
		const auto s = p->GetBannerPath();
		if (!s.empty())
			lua_pushstring(L, s.c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetBackgroundPath(T* p, lua_State* L)
	{
		const auto s = p->GetBackgroundPath();
		if (!s.empty())
			lua_pushstring(L, s.c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetPreviewVidPath(T* p, lua_State* L)
	{
		const auto s = p->GetPreviewVidPath();
		if (!s.empty())
			lua_pushstring(L, s.c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetPreviewMusicPath(T* p, lua_State* L)
	{
		const auto s = p->GetPreviewMusicPath();
		lua_pushstring(L, s.c_str());
		return 1;
	}
	static int GetJacketPath(T* p, lua_State* L)
	{
		const auto s = p->GetJacketPath();
		if (!s.empty())
			lua_pushstring(L, s.c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetCDImagePath(T* p, lua_State* L)
	{
		const auto s = p->GetCDImagePath();
		if (!s.empty())
			lua_pushstring(L, s.c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetDiscPath(T* p, lua_State* L)
	{
		const auto s = p->GetDiscPath();
		if (!s.empty())
			lua_pushstring(L, s.c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetCDTitlePath(T* p, lua_State* L)
	{
		const auto s = p->GetCDTitlePath();
		if (!s.empty())
			lua_pushstring(L, s.c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetLyricsPath(T* p, lua_State* L)
	{
		const auto s = p->GetLyricsPath();
		if (!s.empty())
			lua_pushstring(L, s.c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetSongFilePath(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetSongFilePath().c_str());
		return 1;
	}
	static int IsEnabled(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->GetEnabled());
		return 1;
	}
	static int GetGroupName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sGroupName.c_str());
		return 1;
	}
	static int MusicLengthSeconds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_fMusicLengthSeconds);
		return 1;
	}
	static int GetSampleStart(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetPreviewStartSeconds());
		return 1;
	}
	static int GetSampleLength(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_fMusicSampleLengthSeconds);
		return 1;
	}
	static int IsLong(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->IsLong());
		return 1;
	}
	static int IsMarathon(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->IsMarathon());
		return 1;
	}
	static int HasStepsType(T* p, lua_State* L)
	{
		const auto st = Enum::Check<StepsType>(L, 1);
		lua_pushboolean(L, p->HasStepsType(st));
		return 1;
	}
	static int HasStepsTypeAndDifficulty(T* p, lua_State* L)
	{
		const auto st = Enum::Check<StepsType>(L, 1);
		const auto dc = Enum::Check<Difficulty>(L, 2);
		lua_pushboolean(L, p->HasStepsTypeAndDifficulty(st, dc));
		return 1;
	}
	static int IsStepsUsingDifferentTiming(T* p, lua_State* L)
	{
		lua_pushboolean(
		  L, p->IsStepsUsingDifferentTiming(Luna<Steps>::check(L, 1, true)));
		return 1;
	}
	/* TODO: HasStepsTypeAndDifficulty and GetOneSteps should be in
	 * a SongUtil Lua table and a method of Steps. */
	static int GetOneSteps(T* p, lua_State* L)
	{
		const auto st = Enum::Check<StepsType>(L, 1);
		const auto dc = Enum::Check<Difficulty>(L, 2);
		auto* pSteps = SongUtil::GetOneSteps(p, st, dc);
		if (pSteps != nullptr)
			pSteps->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetTimingData(T* p, lua_State* L)
	{
		p->m_SongTiming.PushSelf(L);
		return 1;
	}
	static int GetBGChanges(T* p, lua_State* L)
	{
		const auto& changes = p->GetBackgroundChanges(BACKGROUND_LAYER_1);
		lua_createtable(L, changes.size(), 0);
		for (size_t c = 0; c < changes.size(); ++c) {
			lua_createtable(L, 0, 8);
			lua_pushnumber(L, changes[c].m_fStartBeat);
			lua_setfield(L, -2, "start_beat");
			lua_pushnumber(L, changes[c].m_fRate);
			lua_setfield(L, -2, "rate");
			LuaHelpers::Push(L, changes[c].m_sTransition);
			lua_setfield(L, -2, "transition");
			LuaHelpers::Push(L, changes[c].m_def.m_sEffect);
			lua_setfield(L, -2, "effect");
			LuaHelpers::Push(L, changes[c].m_def.m_sFile1);
			lua_setfield(L, -2, "file1");
			LuaHelpers::Push(L, changes[c].m_def.m_sFile2);
			lua_setfield(L, -2, "file2");
			LuaHelpers::Push(L, changes[c].m_def.m_sColor1);
			lua_setfield(L, -2, "color1");
			LuaHelpers::Push(L, changes[c].m_def.m_sColor2);
			lua_setfield(L, -2, "color2");
			lua_rawseti(L, -2, c + 1);
		}
		return 1;
	}
	static int IsFavorited(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->IsFavorited());
		return 1;
	}
	// has functions
	static int HasMusic(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasMusic());
		return 1;
	}
	static int HasBanner(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasBanner());
		return 1;
	}
	static int HasBackground(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasBackground());
		return 1;
	}
	static int HasPreviewVid(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasPreviewVid());
		return 1;
	}
	static int HasJacket(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasJacket());
		return 1;
	}
	static int HasDisc(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasDisc());
		return 1;
	}
	static int HasCDImage(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasCDImage());
		return 1;
	}
	static int HasCDTitle(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasCDTitle());
		return 1;
	}
	static int HasBGChanges(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasBGChanges());
		return 1;
	}
	static int HasLyrics(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasLyrics());
		return 1;
	}
	// functions that AJ loves
	static int HasSignificantBPMChangesOrStops(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasSignificantBpmChangesOrStops());
		return 1;
	}
	static int HasEdits(T* p, lua_State* L)
	{
		const auto st = Enum::Check<StepsType>(L, 1);
		lua_pushboolean(L, p->HasEdits(st));
		return 1;
	}
	static int GetStepsSeconds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetStepsSeconds());
		return 1;
	}
	static int GetFirstSecond(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetFirstSecond());
		return 1;
	}
	static int GetLastSecond(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetLastSecond());
		return 1;
	}
	static int GetFirstBeat(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetFirstBeat());
		return 1;
	}
	static int GetLastBeat(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetLastBeat());
		return 1;
	}
	static int GetDisplayBpms(T* p, lua_State* L)
	{
		DisplayBpms temp;
		p->GetDisplayBpms(temp);
		const auto fMin = temp.GetMin();
		const auto fMax = temp.GetMax();
		vector<float> fBPMs;
		fBPMs.push_back(fMin);
		fBPMs.push_back(fMax);
		LuaHelpers::CreateTableFromArray(fBPMs, L);
		return 1;
	}
	static int IsDisplayBpmSecret(T* p, lua_State* L)
	{
		DisplayBpms temp;
		p->GetDisplayBpms(temp);
		lua_pushboolean(L, static_cast<int>(temp.IsSecret()));
		return 1;
	}
	static int IsDisplayBpmConstant(T* p, lua_State* L)
	{
		DisplayBpms temp;
		p->GetDisplayBpms(temp);
		lua_pushboolean(L, static_cast<int>(temp.BpmIsConstant()));
		return 1;
	}
	static int IsDisplayBpmRandom(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->m_DisplayBPMType == DISPLAY_BPM_RANDOM);
		return 1;
	}
	static int ReloadFromSongDir(T* p, lua_State* L)
	{
		p->ReloadFromSongDir();
		COMMON_RETURN_SELF;
	}
	static int GetOrTryAtLeastToGetSimfileAuthor(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetOrTryAtLeastToGetSimfileAuthor().c_str());
		return 1;
	}
	static int PlaySampleMusicExtended(T* p, lua_State* L)
	{
		p->PlaySampleMusicExtended();
		return 0;
	}
	LunaSong()
	{
		ADD_METHOD(GetDisplayFullTitle);
		ADD_METHOD(GetTranslitFullTitle);
		ADD_METHOD(GetDisplayMainTitle);
		ADD_METHOD(GetMainTitle);
		ADD_METHOD(GetTranslitMainTitle);
		ADD_METHOD(GetDisplaySubTitle);
		ADD_METHOD(GetTranslitSubTitle);
		ADD_METHOD(GetDisplayArtist);
		ADD_METHOD(GetTranslitArtist);
		ADD_METHOD(GetGenre);
		ADD_METHOD(GetOrigin);
		ADD_METHOD(GetAllSteps);
		ADD_METHOD(GetStepsByStepsType);
		ADD_METHOD(GetSongDir);
		ADD_METHOD(GetMusicPath);
		ADD_METHOD(GetBannerPath);
		ADD_METHOD(GetBackgroundPath);
		ADD_METHOD(GetJacketPath);
		ADD_METHOD(GetCDImagePath);
		ADD_METHOD(GetDiscPath);
		ADD_METHOD(GetCDTitlePath);
		ADD_METHOD(GetLyricsPath);
		ADD_METHOD(GetSongFilePath);
		ADD_METHOD(IsEnabled);
		ADD_METHOD(GetGroupName);
		ADD_METHOD(MusicLengthSeconds);
		ADD_METHOD(GetSampleStart);
		ADD_METHOD(GetSampleLength);
		ADD_METHOD(IsLong);
		ADD_METHOD(IsMarathon);
		ADD_METHOD(HasStepsType);
		ADD_METHOD(HasStepsTypeAndDifficulty);
		ADD_METHOD(GetOneSteps);
		ADD_METHOD(GetTimingData);
		ADD_METHOD(GetBGChanges);
		ADD_METHOD(GetOrTryAtLeastToGetSimfileAuthor);
		ADD_METHOD(HasMusic);
		ADD_METHOD(HasBanner);
		ADD_METHOD(HasBackground);
		ADD_METHOD(HasJacket);
		ADD_METHOD(HasCDImage);
		ADD_METHOD(HasDisc);
		ADD_METHOD(HasCDTitle);
		ADD_METHOD(HasBGChanges);
		ADD_METHOD(HasLyrics);
		ADD_METHOD(HasSignificantBPMChangesOrStops);
		ADD_METHOD(HasEdits);
		ADD_METHOD(IsFavorited);
		ADD_METHOD(GetStepsSeconds);
		ADD_METHOD(GetFirstBeat);
		ADD_METHOD(GetFirstSecond);
		ADD_METHOD(GetLastBeat);
		ADD_METHOD(GetLastSecond);
		ADD_METHOD(GetDisplayBpms);
		ADD_METHOD(IsDisplayBpmSecret);
		ADD_METHOD(IsDisplayBpmConstant);
		ADD_METHOD(IsDisplayBpmRandom);
		ADD_METHOD(IsStepsUsingDifferentTiming);
		ADD_METHOD(HasPreviewVid);
		ADD_METHOD(GetPreviewVidPath);
		ADD_METHOD(GetPreviewMusicPath);
		ADD_METHOD(ReloadFromSongDir);
		ADD_METHOD(PlaySampleMusicExtended);
	}
};

LUA_REGISTER_CLASS(Song)
// lua end
