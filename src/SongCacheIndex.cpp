#include "global.h"

#include "SongCacheIndex.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageFileManager.h"
#include "GameManager.h"
#include "Song.h"
#include "SpecialFiles.h"
#include "Steps.h"
#include "NotesLoaderSSC.h"
#include "NotesWriterSSC.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
#include "sqlite3.h"

/*
 * A quick explanation of song cache hashes: Each song has two hashes; a hash of the
 * song path, and a hash of the song directory.  The former is Song::GetCacheFilePath;
 * it stays the same if the contents of the directory change.  The latter is 
 * GetHashForDirectory(m_sSongDir), and changes on each modification.
 *
 * The file hash is used as the cache filename.  We don't want to use the directory
 * hash: if we do that, then we'll write a new cache file every time the song changes,
 * and they'll accumulate or we'll have to be careful to delete them.
 *
 * The directory hash is stored in here, indexed by the song path, and used to determine
 * if a song has changed.
 *
 * Another advantage of this system is that we can load songs from cache given only their
 * path; we don't have to actually look in the directory (to find out the directory hash)
 * in order to find the cache file.
 */
const string CACHE_INDEX = SpecialFiles::CACHE_DIR + "index.cache";
const string CACHE_DB = SpecialFiles::CACHE_DIR + "cache.db";


SongCacheIndex *SONGINDEX; // global and accessible from anywhere in our program

RString SongCacheIndex::GetCacheFilePath( const RString &sGroup, const RString &sPath )
{
	/* Don't use GetHashForFile, since we don't want to spend time
	 * checking the file size and date. */
	RString s;
	
	if( sPath.size() > 2 && sPath[0] == '/' && sPath[sPath.size()-1] == '/' )
		s.assign( sPath, 1, sPath.size() - 2 );
	else if( sPath.size() > 1 && sPath[0] == '/' )
		s.assign( sPath, 1, sPath.size() - 1 );
	else
		s = sPath;
	/* Change slashes and invalid utf-8 characters to _.
	 * http://en.wikipedia.org/wiki/UTF-8
	 * Mac OS X doesn't support precomposed unicode characters in files names and
	 * so we should probably replace them with combining diacritics.
	 * XXX How do we do this and is it even worth it? */
	const char *invalid = "/\xc0\xc1\xfe\xff\xf8\xf9\xfa\xfb\xfc\xfd\xf5\xf6\xf7";
	for( size_t pos = s.find_first_of(invalid); pos != RString::npos; pos = s.find_first_of(invalid, pos) )
		s[pos] = '_';
	// CACHE_DIR ends with a /.
	return ssprintf( "%s%s/%s", SpecialFiles::CACHE_DIR.c_str(), sGroup.c_str(), s.c_str() );
}

SongCacheIndex::SongCacheIndex()
{
	ReadCacheIndex();
	DBEmpty = !OpenDB();
}

int SongCacheIndex::InsertStepsTimingData(TimingData timing)
{
	SQLite::Statement insertTimingData(*db, "INSERT INTO timingdatas VALUES (NULL, "
		"OFFSET=?, BPMS=?, STOPS=?, "
		"DELAYS=?, WARPS=?, TIMESIGNATURESEGMENT=?, TICKCOUNTS=?, "
		"COMBOS=?, SPEEDS=?, SCROLLS=?, FAKES=?, LABELS=?)");
	unsigned int timingDataIndex = 0;
	insertTimingData.bind(timingDataIndex++, timing.m_fBeat0OffsetInSeconds);
	{
		vector<TimingSegment*> const& segs = timing.GetTimingSegments(SEGMENT_BPM);
		string bpms = "";
		if (!segs.empty())
		{
			for (auto&& seg : segs)
			{
				const  BPMSegment* segment = ToBPM(seg);
				bpms.append(ssprintf("%.6f=%.6f", NoteRowToBeat(segment->GetRow()), segment->GetBPM()));
			}
		}
		insertTimingData.bind(timingDataIndex++, bpms);
	}
	{
		vector<TimingSegment*> const& segs = timing.GetTimingSegments(SEGMENT_STOP);
		string stops = "";
		if (!segs.empty())
		{
			for (auto&& seg : segs)
			{
				const  StopSegment* segment = ToStop(seg);
				stops.append(ssprintf("%.6f=%.6f", NoteRowToBeat(segment->GetRow()), segment->GetPause()));
			}
		}
		insertTimingData.bind(timingDataIndex++, stops);
	}
	{
		vector<TimingSegment*> const& segs = timing.GetTimingSegments(SEGMENT_DELAY);
		string delays = "";
		if (!segs.empty())
		{
			for (auto&& seg : segs)
			{
				const  DelaySegment* segment = ToDelay(seg);
				delays.append(ssprintf("%.6f=%.6f", NoteRowToBeat(segment->GetRow()), segment->GetPause()));
			}
		}
		insertTimingData.bind(timingDataIndex++, delays);
	}
	{
		vector<TimingSegment*> const& segs = timing.GetTimingSegments(SEGMENT_WARP);
		string warps = "";
		if (!segs.empty())
		{
			for (auto&& seg : segs)
			{
				const  WarpSegment* segment = ToWarp(seg);
				warps.append(ssprintf("%.6f=%.6f", NoteRowToBeat(segment->GetRow()), segment->GetLength()));
			}
		}
		insertTimingData.bind(timingDataIndex++, warps);
	}
	{
		vector<TimingSegment*> const& segs = timing.GetTimingSegments(SEGMENT_TIME_SIG);
		string timesigs = "";
		if (!segs.empty())
		{
			for (auto&& seg : segs)
			{
				const  TimeSignatureSegment* segment = ToTimeSignature(seg);
				timesigs.append(ssprintf("%.6f=%d=%d", NoteRowToBeat(segment->GetRow()), segment->GetNum(), segment->GetDen()));
			}
		}
		insertTimingData.bind(timingDataIndex++, timesigs);
	}
	{
		vector<TimingSegment*> const& segs = timing.GetTimingSegments(SEGMENT_TICKCOUNT);
		string ticks = "";
		if (!segs.empty())
		{
			for (auto&& seg : segs)
			{
				const  TickcountSegment* segment = ToTickcount(seg);
				ticks.append(ssprintf("%.6f=%d", NoteRowToBeat(segment->GetRow()), segment->GetTicks()));
			}
		}
		insertTimingData.bind(timingDataIndex++, ticks);
	}
	{
		vector<TimingSegment*> const& segs = timing.GetTimingSegments(SEGMENT_COMBO);
		string combos = "";
		if (!segs.empty())
		{
			for (auto&& seg : segs)
			{
				const  ComboSegment* segment = ToCombo(seg);
				if (segment->GetCombo() == segment->GetMissCombo())
				{
					combos.append(ssprintf("%.6f=%d", NoteRowToBeat(segment->GetRow()), segment->GetCombo()));
				}
				else
				{
					combos.append(ssprintf("%.6f=%d=%d", NoteRowToBeat(segment->GetRow()), segment->GetCombo(), segment->GetMissCombo()));
				}
			}
		}
		insertTimingData.bind(timingDataIndex++, combos);
	}
	{
		vector<TimingSegment*> const& segs = timing.GetTimingSegments(SEGMENT_SPEED);
		string speeds = "";
		if (!segs.empty())
		{
			for (auto&& seg : segs)
			{
				const  SpeedSegment* segment = ToSpeed(seg);
				speeds.append(ssprintf("%.6f=%.6f=%.6f=%hd", NoteRowToBeat(segment->GetRow()), segment->GetRatio(), segment->GetDelay(), segment->GetUnit()));
			}
		}
		insertTimingData.bind(timingDataIndex++, speeds);
	}
	{
		vector<TimingSegment*> const& segs = timing.GetTimingSegments(SEGMENT_SCROLL);
		string scrolls = "";
		if (!segs.empty())
		{
			for (auto&& seg : segs)
			{
				const  ScrollSegment* segment = ToScroll(seg);
				scrolls.append(ssprintf("%.6f=%.6f", NoteRowToBeat(segment->GetRow()), segment->GetRatio()));
			}
		}
		insertTimingData.bind(timingDataIndex++, scrolls);
	}
	{
		vector<TimingSegment*> const& segs = timing.GetTimingSegments(SEGMENT_LABEL);
		string labels = "";
		if (!segs.empty())
		{
			for (auto&& seg : segs)
			{
				const  LabelSegment* segment = ToLabel(seg);
				if (!segment->GetLabel().empty())
				{
					labels.append(ssprintf("%.6f=%s", NoteRowToBeat(segment->GetRow()), segment->GetLabel()));
				}
			}
		}
		insertTimingData.bind(timingDataIndex++, labels);
	}
	insertTimingData.exec();
	return sqlite3_last_insert_rowid(db->getHandle());
}

int SongCacheIndex::InsertSteps(const Steps* pSteps, int songID)
{
	SQLite::Statement insertSteps(*db, "INSERT INTO steps VALUES (NULL, "
		"CHARTNAME=?, STEPSTYPE=?, DESCRIPTION=?, CHARTSTYLE=?, DIFFICULTY=?, "
		"METER=?, MSDVALUES=?, CHARTKEY=?, MUSIC=?, RADARVALUES=?, CREDIT=?, "
		"TIMINGDATAID=?, DISPLAYBPM=?, STEPFILENAME=?, SONGID=?)");
	vector<RString> lines;
	int stepsIndex = 0;
	insertSteps.bind(stepsIndex++, pSteps->GetChartName());
	insertSteps.bind(stepsIndex++, pSteps->m_StepsTypeStr);
	insertSteps.bind(stepsIndex++, pSteps->GetDescription());
	insertSteps.bind(stepsIndex++, pSteps->GetChartStyle());
	insertSteps.bind(stepsIndex++, pSteps->GetDifficulty());
	insertSteps.bind(stepsIndex++, pSteps->GetMeter());
	insertSteps.bind(stepsIndex++, NotesWriterSSC::MSDToString(pSteps->GetAllMSD()).c_str());//msdvalues
	insertSteps.bind(stepsIndex++, SmEscape(pSteps->GetChartKey()).c_str());//chartkey

	insertSteps.bind(stepsIndex++, pSteps->GetMusicFile());//musicfile

	vector<RString> asRadarValues;
	const RadarValues &rv = pSteps->GetRadarValues();
	FOREACH_ENUM(RadarCategory, rc)
		asRadarValues.emplace_back(ssprintf("%i", rv[rc]));
	insertSteps.bind(stepsIndex++, join(",", asRadarValues).c_str());

	insertSteps.bind(stepsIndex++, pSteps->GetCredit());

	// If the Steps TimingData is not empty, then they have their own
	// timing.  Write out the corresponding tags.
	if (pSteps->m_Timing.empty()) {
		insertSteps.bind(stepsIndex++);
	}
	else {
		int timingDataID = InsertStepsTimingData(pSteps->m_Timing);
		insertSteps.bind(stepsIndex++, timingDataID);
	}

	switch (pSteps->GetDisplayBPM())
	{
	case DISPLAY_BPM_ACTUAL:
		insertSteps.bind(stepsIndex++);
		// write nothing
		break;
	case DISPLAY_BPM_SPECIFIED:
	{
		float small = pSteps->GetMinBPM();
		float big = pSteps->GetMaxBPM();
		if (small == big)
			insertSteps.bind(stepsIndex++, ssprintf("%.6f;", small));
		else
			insertSteps.bind(stepsIndex++, ssprintf("%.6f:%.6f;", small, big));
		break;
	}
	case DISPLAY_BPM_RANDOM:
		insertSteps.bind(stepsIndex++, "*");
		break;
	default:
		insertSteps.bind(stepsIndex++);
		break;
	}
	insertSteps.bind(stepsIndex++, pSteps->GetFilename().c_str());
	insertSteps.bind(stepsIndex++, songID);
	insertSteps.exec();
	return sqlite3_last_insert_rowid(db->getHandle());
}
/*	Save a song to the cache db*/
bool SongCacheIndex::SaveSong(Song& song, string dir)
{
	SQLite::Statement insertSong(*db, "INSERT INTO songs VALUES (NULL, "
		"VERSION=?, TITLE=?, SUBTITLE=?, ARTIST=?, TITLETRANSLIT=?, "
		"SUBTITLETRANSLIT=?, ARTISTTRANSLIT=?, GENRE=?, "
		"ORIGIN=?, CREDIT=?, BANNER=?, BACKGROUND=?"
		"PREVIEWVID=?, JACKET=?, CDIMAGE=?, DISCIMAGE=?, "
		"LYRICSPATH=?, CDTITLE=?, MUSIC=?, PREVIEW=?, INSTRUMENTTRACK=?, "
		"OFFSET=?, SAMPLESTART=?, SAMPLELENGTH=?, SELECTABLE=?, "
		"DISPLAYBPM=?, BPMS=?, STOPS=?, DELAYS=?, WARPS=?, "
		"TIMESIGNATURES=?, TICKCOUNTS=?, COMBOS=?, SPEEDS=?, "
		"SCROLLS=?, FAKES=?, LABELS=?, LASTSECONDHINT=?, "
		"BGCHANGESLAYER1=?, BGCHANGESLAYER2=?, FGCHANGES=?, "
		"KEYSOUNDS=?, FIRSTSECOND=?, LASTSECOND=?, "
		"SONGFILENAME=?, HASMUSIC=?, HASBANNER=?, MUSICLENGTH=?, DIRHASH=?)");
	unsigned int index = 0;
	insertSong.bind(index++, STEPFILE_VERSION_NUMBER);
	insertSong.bind(index++, song.m_sMainTitle);
	insertSong.bind(index++, song.m_sSubTitle);
	insertSong.bind(index++, song.m_sArtist);
	insertSong.bind(index++, song.m_sMainTitleTranslit);
	insertSong.bind(index++, song.m_sSubTitleTranslit);
	insertSong.bind(index++, song.m_sArtistTranslit);
	insertSong.bind(index++, song.m_sGenre);
	insertSong.bind(index++, song.m_sOrigin);
	insertSong.bind(index++, song.m_sCredit);
	insertSong.bind(index++, song.m_sBannerFile);
	insertSong.bind(index++, song.m_sBackgroundFile);
	insertSong.bind(index++, song.m_sPreviewVidFile);
	insertSong.bind(index++, song.m_sJacketFile);
	insertSong.bind(index++, song.m_sCDFile);
	insertSong.bind(index++, song.m_sDiscFile);
	insertSong.bind(index++, song.m_sLyricsFile);
	insertSong.bind(index++, song.m_sCDTitleFile);
	insertSong.bind(index++, song.m_sMusicFile);
	insertSong.bind(index++, song.m_PreviewFile);
	auto vs = song.GetInstrumentTracksToVectorString();
	if (!vs.empty())
	{
		std::string s = join(",", vs);
		insertSong.bind(index++, s);
	}
	else {
		insertSong.bind(index++);
	}
	insertSong.bind(index++, song.m_SongTiming.m_fBeat0OffsetInSeconds);
	insertSong.bind(index++, song.m_fMusicSampleStartSeconds);
	insertSong.bind(index++, song.m_fMusicSampleLengthSeconds);
	//Selectable should be stored as int
	switch (song.m_SelectionDisplay)
	{
	default: ASSERT_M(0, "An invalid selectable value was found for this song!"); // fall through
	case Song::SHOW_ALWAYS:	insertSong.bind(index++, 0);		break;
	case Song::SHOW_NEVER:		insertSong.bind(index++, 1);		break;
	}

	switch (song.m_DisplayBPMType)
	{
	case DISPLAY_BPM_ACTUAL:
		// write nothing(Both nulls)
		insertSong.bind(index++);
		insertSong.bind(index++);
		break;
	case DISPLAY_BPM_SPECIFIED:
		if (song.m_fSpecifiedBPMMin == song.m_fSpecifiedBPMMax)
		{
			insertSong.bind(index++, song.m_fSpecifiedBPMMin);
			insertSong.bind(index++, song.m_fSpecifiedBPMMin);
		}
		else
		{
			insertSong.bind(index++, song.m_fSpecifiedBPMMin);
			insertSong.bind(index++, song.m_fSpecifiedBPMMax);
		}
		break;
	case DISPLAY_BPM_RANDOM:
		//Write only one as null
		insertSong.bind(index++);
		insertSong.bind(index++, 0.0);
		break;
	default:
		insertSong.bind(index++);
		insertSong.bind(index++);
		break;
	}
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_BPM, 3)));
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_STOP, 3)));
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_DELAY, 3)));
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_WARP, 3)));
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_TIME_SIG, 3)));
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_TICKCOUNT, 3)));
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_COMBO, 3)));
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_SPEED, 3)));
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_SCROLL, 3)));
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_FAKE, 3)));
	insertSong.bind(index++, join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_LABEL, 3)));
	if (song.GetSpecifiedLastSecond() > 0)
	{
		insertSong.bind(index++, song.GetSpecifiedLastSecond());
	}
	else {
		insertSong.bind(index++);
	}
	/* Theres only 2 background layers
	// @brief The different background layers available. 
	enum BackgroundLayer
	{
		BACKGROUND_LAYER_1,
		BACKGROUND_LAYER_2,
		//BACKGROUND_LAYER_3, // StepCollection get
		NUM_BackgroundLayer,
		BACKGROUND_LAYER_Invalid
	}; */
	FOREACH_BackgroundLayer(b)
	{
		string bgchanges = "";
		if (song.GetBackgroundChanges(b).empty())
		{
			insertSong.bind(index++);
			continue;	// skip
		}
		for (auto &bgc : song.GetBackgroundChanges(b))
		{
			bgchanges.append(bgc.ToString() + ",");
		}

		/* If there's an animation plan at all, add a dummy "-nosongbg-" tag to
		* indicate that this file doesn't want a song BG entry added at the end.
		* See SSCLoader::TidyUpData. This tag will be removed on load. Add it
		* at a very high beat, so it won't cause problems if loaded in older versions. */
		if (b == 0 && !song.GetBackgroundChanges(b).empty())
		{
			bgchanges.append("99999=-nosongbg-=1.000=0=0=0");
		}
		insertSong.bind(index++, bgchanges);
	}

	if (song.GetForegroundChanges().size())
	{
		string fgchanges;
		for (auto const &bgc : song.GetForegroundChanges())
		{
			fgchanges.append(bgc.ToString() + ",");
		}
		insertSong.bind(index++, fgchanges);
	}
	else {
		insertSong.bind(index++);
	}

	if (!song.m_vsKeysoundFile.empty()) {
		for (unsigned i = 0; i < song.m_vsKeysoundFile.size(); i++)
		{
			if (i == 0 && song.m_vsKeysoundFile[i].size() > 0 && song.m_vsKeysoundFile[i][0] == '#')
			{
				insertSong.bind(index++, song.m_vsKeysoundFile[i].substr(1, song.m_vsKeysoundFile[i].size()-1));
			}
			else {
				insertSong.bind(index++, song.m_vsKeysoundFile[i]);
			}
		}
	}
	else {
		insertSong.bind(index++);
	}
	insertSong.bind(index++, song.GetFirstSecond());
	insertSong.bind(index++, song.GetLastSecond());
	insertSong.bind(index++, song.m_sSongFileName.c_str());
	insertSong.bind(index++, song.m_bHasMusic);
	insertSong.bind(index++, song.m_bHasBanner);
	insertSong.bind(index++, song.m_fMusicLengthSeconds);
	insertSong.bind(index++, GetHashForDirectory(song.GetSongDir()));
	insertSong.exec();
	int songID = sqlite3_last_insert_rowid(db->getHandle());
	vector<Steps*> vpStepsToSave = song.GetStepsToSave();
	FOREACH_CONST(Steps*, vpStepsToSave, s)
	{
		const Steps* pSteps = *s;
		if (pSteps->GetChartKey() == "") {		// Avoid writing cache tags for invalid chartkey files(empty steps) -Mina
			LOG->Info("Not caching empty difficulty in file %s", dir.c_str());
			continue;
		}
		int stepsID = InsertSteps(pSteps, songID);
	}
	return true;
}

/*	Reset the DB/
Must be open already	*/
void SongCacheIndex::DeleteDB()
{
	if (db == nullptr)
		return;
	SQLite::Statement   qTables(*db, "SELECT name FROM sqlite_master WHERE type='table'");
	qTables.exec();
	while (qTables.executeStep())
	{
		string table = static_cast<const char*>(qTables.getColumn(0));
		db->exec("DROP TABLE IF EXISTS  " + table);
	}
	LOG->Trace("Cache database is out of date.  Deleting all cache files.");
	db->exec("VACUUM"); //Shrink to fit
}
void SongCacheIndex::ResetDB()
{
	ResetDB();
	db->exec("CREATE TABLE IF NOT EXISTS dbinfo (ID INTEGER PRIMARY KEY, "
		"VERSION INTEGER)");
	db->exec("CREATE TABLE IF NOT EXISTS timingdatas (ID INTEGER PRIMARY KEY, "
		"OFFSET TEXT, BPMS TEXT, STOPS TEXT, "
		"DELAYS TEXT, WARPS TEXT, TIMESIGNATURESEGMENT TEXT, TICKCOUNTS TEXT, "
		"COMBOS TEXT, SPEEDS TEXT, SCROLLS TEXT, FAKES TEXT, LABELS TEXT)");
	db->exec("CREATE TABLE IF NOT EXISTS songs (ID INTEGER PRIMARY KEY, "
		"VERSION TEXT, TITLE TEXT, SUBTITLE TEXT, ARTIST TEXT, TITLETRANSLIT TEXT, "
		"SUBTITLETRANSLIT TEXT, ARTISTTRANSLIT TEXT, GENRE TEXT, "
		"ORIGIN TEXT, CREDIT TEXT, BANNER TEXT, BACKGROUND TEXT, "
		"PREVIEWVID TEXT, JACKET TEXT, CDIMAGE TEXT, DISCIMAGE TEXT, "
		"LYRICSPATH TEXT, CDTITLE TEXT, MUSIC TEXT, PREVIEW TEXT, INSTRUMENTTRACK TEXT, "
		"OFFSET FLOAT, SAMPLESTART FLOAT, SAMPLELENGTH FLOAT, SELECTABLE TEXT, "
		"DISPLAYBPMMIN FLOAT, DISPLAYBPM MAX FLOAT, BPMS TEXT, STOPS TEXT, DELAYS TEXT, WARPS TEXT, "
		"TIMESIGNATURES TEXT, TICKCOUNTS TEXT, COMBOS TEXT, SPEEDS TEXT, "
		"SCROLLS TEXT, FAKES TEXT, LABELS TEXT, LASTSECONDHINT FLOAT, "
		"BGCHANGESLAYER1 TEXT, BGCHANGESLAYER2 TEXT, FGCHANGES TEXT, "
		"KEYSOUNDS TEXT, FIRSTSECOND FLOAT, LASTSECOND FLOAT, "
		"SONGFILENAME TEXT, HASMUSIC TEXT, HASBANNER TEXT, MUSICLENGTH FLOAT, DIRHASH INTEGER)");
	db->exec("CREATE TABLE IF NOT EXISTS steps (id INTEGER PRIMARY KEY, "
		"CHARTNAME TEXT, STEPSTYPE TEXT, DESCRIPTION TEXT, CHARTSTYLE TEXT, DIFFICULTY INTEGER, "
		"METER INTEGER, MSDVALUES TEXT, CHARTKEY TEXT, MUSIC TEXT, RADARVALUES TEXT, CREDIT TEXT, "
		"TIMINGDATAID TEXT, DISPLAYBPM TEXT, STEPFILENAME TEXT, SONGID INTEGER, "
		"CONSTRAINT fk_songid FOREIGN KEY (SONGID) REFERENCES songs(id), "
		"CONSTRAINT fk_timingdataid FOREIGN KEY (TIMINGDATAID) REFERENCES songs(ID))");
}
/*	Returns weather or not the db had valid data*/
bool SongCacheIndex::OpenDB()
{
	bool ret = IsAFile(CACHE_DB);
	//Try to open ane existing db
	try {
		db = new SQLite::Database(FILEMAN->ResolvePath(CACHE_INDEX), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
		if (!ret) {
			ResetDB();
			return false;
		}
		SQLite::Statement   qDBInfo(*db, "SELECT * FROM dbinfo");

		//Should only have one row so no executeStep loop
		if (!qDBInfo.executeStep()) {
			ResetDB();
			return false;
		}
		int iCacheVersion = -1;
		iCacheVersion = qDBInfo.getColumn(0);
		if (iCacheVersion == FILE_CACHE_VERSION)
			return true;
		ResetDB();
	}
	catch (std::exception& e)
	{
		LOG->Trace("Error reading cache db: %s", e.what());
		db = nullptr;
		return false;
	}
	return false;
}

SongCacheIndex::~SongCacheIndex()
= default;

void SongCacheIndex::ReadFromDisk()
{
	ReadCacheIndex();
}

static void EmptyDir(RString dir)
{
	ASSERT(dir[dir.size() - 1] == '/');

	vector<RString> asCacheFileNames;
	GetDirListing(dir, asCacheFileNames);
	for (unsigned i = 0; i<asCacheFileNames.size(); i++)
	{
		if (!IsADirectory(dir + asCacheFileNames[i]))
			FILEMAN->Remove(dir + asCacheFileNames[i]);
	}
}

void SongCacheIndex::ReadCacheIndex()
{
	CacheIndex.ReadFile(CACHE_INDEX);	// don't care if this fails

	int iCacheVersion = -1;
	CacheIndex.GetValue("Cache", "CacheVersion", iCacheVersion);
	if (iCacheVersion == FILE_CACHE_VERSION)
		return; // OK

	LOG->Trace("Cache format is out of date.  Deleting all cache files.");
	EmptyDir(SpecialFiles::CACHE_DIR);
	EmptyDir(SpecialFiles::CACHE_DIR + "Banners/");
	//EmptyDir( SpecialFiles::CACHE_DIR+"Backgrounds/" );
	EmptyDir(SpecialFiles::CACHE_DIR + "Songs/");
	EmptyDir(SpecialFiles::CACHE_DIR + "Courses/");

	CacheIndex.Clear();
	/* This is right now in place because our song file paths are apparently being
	* cached in two distinct areas, and songs were loading from paths in FILEMAN.
	* This is admittedly a hack for now, but this does bring up a good question on
	* whether we really need a dedicated cache for future versions of StepMania.
	*/
	FILEMAN->FlushDirCache();
}

void SongCacheIndex::SaveCacheIndex()
{
	CacheIndex.WriteFile(CACHE_INDEX);
}

void SongCacheIndex::AddCacheIndex(const RString &path, unsigned hash)
{
	if (hash == 0)
		++hash; /* no 0 hash values */
	CacheIndex.SetValue("Cache", "CacheVersion", FILE_CACHE_VERSION);
	CacheIndex.SetValue("Cache", MangleName(path), hash);
	if (!delay_save_cache)
	{
		CacheIndex.WriteFile(CACHE_INDEX);
	}
}

unsigned SongCacheIndex::GetCacheHash(const RString &path) const
{
	unsigned iDirHash = 0;
	if (!CacheIndex.GetValue("Cache", MangleName(path), iDirHash))
		return 0;
	if (iDirHash == 0)
		++iDirHash; /* no 0 hash values */
	return iDirHash;
}

RString SongCacheIndex::MangleName(const RString &Name)
{
	/* We store paths in an INI.  We can't store '='. */
	RString ret = Name;
	ret.Replace("=", "");
	return ret;
}

/*	Load a song from Cache DB
	Returns true if it was loaded**/
bool SongCacheIndex::LoadSongFromCache(Song* song, string dir)
{
	db->exec("CREATE TABLE IF NOT EXISTS songs (ID INTEGER PRIMARY KEY, "
		"VERSION TEXT, TITLE TEXT, SUBTITLE TEXT, ARTIST TEXT, TITLETRANSLIT TEXT, "
		"SUBTITLETRANSLIT TEXT, ARTISTTRANSLIT TEXT, GENRE TEXT, "
		"ORIGIN TEXT, CREDIT TEXT, BANNER TEXT, BACKGROUND TEXT, "
		"PREVIEWVID TEXT, JACKET TEXT, CDIMAGE TEXT, DISCIMAGE TEXT, "
		"LYRICSPATH TEXT, CDTITLE TEXT, MUSIC TEXT, PREVIEW TEXT, INSTRUMENTTRACK TEXT, "
		"OFFSET FLOAT, SAMPLESTART FLOAT, SAMPLELENGTH FLOAT, SELECTABLE TEXT, "
		"DISPLAYBPMMIN FLOAT, DISPLAYBPM MAX FLOAT, BPMS TEXT, STOPS TEXT, DELAYS TEXT, WARPS TEXT, "
		"TIMESIGNATURES TEXT, TICKCOUNTS TEXT, COMBOS TEXT, SPEEDS TEXT, "
		"SCROLLS TEXT, FAKES TEXT, LABELS TEXT, LASTSECONDHINT FLOAT, "
		"BGCHANGESLAYER1 TEXT, BGCHANGESLAYER2 TEXT, FGCHANGES TEXT, "
		"KEYSOUNDS TEXT, FIRSTSECOND FLOAT, LASTSECOND FLOAT, "
		"SONGFILENAME TEXT, HASMUSIC TEXT, HASBANNER TEXT, MUSICLENGTH FLOAT, DIRHASH INTEGER)");

	SQLite::Statement query(*db, "SELECT * FROM songs WHERE DIR=? AND DIRHASH=?");
	query.bind(1, dir);
	query.bind(2, GetHashForDirectory(song->GetSongDir()));

	//No cache entry => return false
	if (!query.tryExecuteStep())
		return false;

	//SSC::StepsTagInfo reused_steps_info(&*song, &out, dir, true);
	SSCLoader loader;
	int songid = query.getColumn(0);
	int index = 1;
	song->m_fVersion = static_cast<double>(query.getColumn(index++));
	song->m_sMainTitle = static_cast<const char *>(query.getColumn(index++));
	song->m_sSubTitle = static_cast<const char *>(query.getColumn(index++));
	song->m_sArtist = static_cast<const char *>(query.getColumn(index++));
	song->m_sMainTitleTranslit = static_cast<const char *>(query.getColumn(index++));
	song->m_sSubTitleTranslit = static_cast<const char *>(query.getColumn(index++));
	song->m_sArtistTranslit = static_cast<const char *>(query.getColumn(index++));
	song->m_sGenre = static_cast<const char *>(query.getColumn(index++));
	song->m_sOrigin = static_cast<const char *>(query.getColumn(index++));
	song->m_sCredit = static_cast<const char *>(query.getColumn(index++));
	Trim(song->m_sCredit);
	song->m_sBannerFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sBackgroundFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sPreviewVidFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sJacketFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sCDFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sDiscFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sLyricsFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sCDTitleFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sMusicFile = static_cast<const char *>(query.getColumn(index++));
	song->m_PreviewFile = static_cast<const char *>(query.getColumn(index++));
	loader.ProcessInstrumentTracks(*song, static_cast<const char *>(query.getColumn(index++))); 
	song->SetSpecifiedLastSecond(static_cast<double>(query.getColumn(index++)));
	song->m_fMusicSampleStartSeconds = static_cast<double>(query.getColumn(index++));
	song->m_fMusicSampleLengthSeconds = static_cast<double>(query.getColumn(index++));
	song->m_SongTiming.m_fBeat0OffsetInSeconds = static_cast<double>(query.getColumn(index++));

	int selection = static_cast<int>(query.getColumn(index++));
	if (selection == 0)
		song->m_SelectionDisplay = song->SHOW_ALWAYS;
	else
		song->m_SelectionDisplay = song->SHOW_NEVER;

	int bpmminIndex = index++;
	int bpmmaxIndex = index++;
	float BPMmin = static_cast<double>(query.getColumn(bpmminIndex));
	float BPMmax = static_cast<double>(query.getColumn(bpmmaxIndex));
	if (query.isColumnNull(bpmminIndex) || query.isColumnNull(bpmmaxIndex))
	{
		if (query.isColumnNull(bpmminIndex) && query.isColumnNull(bpmmaxIndex))
			song->m_DisplayBPMType = DISPLAY_BPM_RANDOM;
		else
			song->m_DisplayBPMType = DISPLAY_BPM_ACTUAL;
	}	
	else
	{
		song->m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
		song->m_fSpecifiedBPMMin = BPMmin;
		song->m_fSpecifiedBPMMax = BPMmax;
	}

	loader.ProcessBPMs(song->m_SongTiming, song->GetMainTitle(), static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessStops(song->m_SongTiming, song->GetMainTitle(), static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessDelays(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessWarps(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)), song->m_fVersion, song->GetMainTitle());
	loader.ProcessTimeSignatures(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessTickcounts(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessCombos(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessSpeeds(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessScrolls(song->m_SongTiming, song->GetMainTitle(), static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessFakes(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessLabels(song->m_SongTiming, song->GetMainTitle(), static_cast<const char *>(query.getColumn(index++)));

	song->SetSpecifiedLastSecond(static_cast<double>(query.getColumn(index++)));

	string animations = static_cast<const char *>(query.getColumn(index++));
	string animationstwo = static_cast<const char *>(query.getColumn(index++));
	loader.ProcessBGChanges(*song, animations,
		dir, animationstwo);
	vector<RString> aFGChangeExpressions;
	split(static_cast<const char *>(query.getColumn(index++)), ",", aFGChangeExpressions);

	for (size_t b = 0; b < aFGChangeExpressions.size(); ++b)
	{
		BackgroundChange change;
		if (loader.LoadFromBGChangesString(change, aFGChangeExpressions[b]))
		{
			song->AddForegroundChange(change);
		}
	}
	RString keysounds = static_cast<const char *>(query.getColumn(index++));
	if (keysounds.length() >= 2 && keysounds.substr(0, 2) == "\\#")
	{
		keysounds = keysounds.substr(1);
	}
	split(keysounds, ",", song->m_vsKeysoundFile);
	song->SetFirstSecond(static_cast<double>(query.getColumn(index++)));
	song->SetLastSecond(static_cast<double>(query.getColumn(index++)));
	song->m_sSongFileName = static_cast<const char *>(query.getColumn(index++));
	song->m_bHasMusic = static_cast<int>(query.getColumn(index++)) != 0;
	song->m_bHasBanner = static_cast<int>(query.getColumn(index++)) != 0;
	song->m_fMusicLengthSeconds = static_cast<double>(query.getColumn(index++));

	Steps* pNewNotes = nullptr;

	SQLite::Statement qSteps(*db, "SELECT * FROM steps WHERE SONGID=" + to_string(songid));

	while (qSteps.tryExecuteStep()) {
		int stepsIndex = 0;
		//state = GETTING_STEP_INFO;
		pNewNotes = song->CreateSteps();
		pNewNotes->SetFilename(dir);
		TimingData stepsTiming = TimingData(song->m_SongTiming.m_fBeat0OffsetInSeconds);
		song->m_fVersion = static_cast<double>(qSteps.getColumn(stepsIndex++));
		RString chartName = static_cast<const char*>(qSteps.getColumn(stepsIndex++));
		Trim(chartName);
		pNewNotes->SetChartName(chartName);
		string stepsType = static_cast<const char*>(qSteps.getColumn(stepsIndex++));
		pNewNotes->m_StepsType = GAMEMAN->StringToStepsType(stepsType);
		pNewNotes->m_StepsTypeStr = stepsType;
		pNewNotes->SetChartStyle(static_cast<const char*>(qSteps.getColumn(stepsIndex++)));
		RString description = static_cast<const char*>(qSteps.getColumn(stepsIndex++));
		Trim(description);
		pNewNotes->SetDescription(description);
		pNewNotes->SetDifficulty(static_cast<Difficulty>(static_cast<int>(qSteps.getColumn(stepsIndex++))));
		pNewNotes->SetMeter(static_cast<int>(qSteps.getColumn(stepsIndex++)));
		pNewNotes->SetMusicFile(static_cast<const char*>(qSteps.getColumn(stepsIndex++)));
		string radarValues = static_cast<const char*>(qSteps.getColumn(stepsIndex++));
		vector<RString> values;
		split(radarValues, ",", values, true);
		RadarValues rv;
		rv.Zero();
		for (size_t i = 0; i < NUM_RadarCategory; ++i)
			rv[i] = StringToInt(values[i]);
		pNewNotes->SetCachedRadarValues(rv);
		pNewNotes->SetCredit(static_cast<const char*>(qSteps.getColumn(stepsIndex++)));

		/* If this is called, the chart does not use the same attacks
		* as the Song's timing. No other changes are required. */
		int bpmminIndex = stepsIndex++;
		int bpmmaxIndex = stepsIndex++;
		float BPMmin = static_cast<double>(qSteps.getColumn(bpmminIndex));
		float BPMmax = static_cast<double>(qSteps.getColumn(bpmmaxIndex));
		if (qSteps.isColumnNull(bpmminIndex) || qSteps.isColumnNull(bpmmaxIndex))
		{
			if (qSteps.isColumnNull(bpmminIndex) && qSteps.isColumnNull(bpmmaxIndex))
				pNewNotes->SetDisplayBPM(DISPLAY_BPM_RANDOM);
			else
				pNewNotes->SetDisplayBPM(DISPLAY_BPM_ACTUAL);
		}
		else
		{
			pNewNotes->SetDisplayBPM(DISPLAY_BPM_SPECIFIED);
			pNewNotes->SetMinBPM(BPMmin);
			pNewNotes->SetMaxBPM(BPMmax);
		}
		pNewNotes->SetChartKey(static_cast<const char*>(qSteps.getColumn(stepsIndex++)));
		MinaSD o;
		for (size_t i = 0; i <= NUM_Skillset; i++)
			o.emplace_back(SSC::msdsplit(static_cast<const char*>(qSteps.getColumn(stepsIndex++))));
		pNewNotes->SetAllMSD(o);

		if(!qSteps.isColumnNull(stepsIndex+1)) {
			int timingID = qSteps.getColumn(stepsIndex++);
			SQLite::Statement qTiming(*db, "SELECT * FROM timingdatas WHERE STEPSID=" + to_string(timingID));
			if (qTiming.tryExecuteStep()) {
				//Load timing data
				SSCLoader::ProcessBPMs(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), dir);
				//steps_tag_handlers["BPMS"] = &SetStepsBPMs;
				SSCLoader::ProcessStops(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), dir);
				//steps_tag_handlers["STOPS"] = &SetStepsStops;
				SSCLoader::ProcessDelays(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), dir);
				//steps_tag_handlers["DELAYS"] = &SetStepsDelays;
				SSCLoader::ProcessTimeSignatures(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), dir);
				//steps_tag_handlers["TIMESIGNATURES"] = &SetStepsTimeSignatures;
				SSCLoader::ProcessTickcounts(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), dir);
				//steps_tag_handlers["TICKCOUNTS"] = &SetStepsTickCounts;
				SSCLoader::ProcessCombos(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), dir);
				//steps_tag_handlers["COMBOS"] = &SetStepsCombos;
				SSCLoader::ProcessWarps(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), song->m_fVersion, dir);
				//steps_tag_handlers["WARPS"] = &SetStepsWarps;
				SSCLoader::ProcessSpeeds(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), dir);
				//steps_tag_handlers["SPEEDS"] = &SetStepsSpeeds;
				SSCLoader::ProcessScrolls(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), dir);
				//steps_tag_handlers["SCROLLS"] = &SetStepsScrolls;
				SSCLoader::ProcessFakes(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), dir);
				//steps_tag_handlers["FAKES"] = &SetStepsFakes;
				SSCLoader::ProcessLabels(stepsTiming, static_cast<const char*>(qSteps.getColumn(stepsIndex++)), dir);
				stepsTiming.m_fBeat0OffsetInSeconds = static_cast<double>(qTiming.getColumn(stepsIndex++));
			}
		}
		pNewNotes->m_Timing = stepsTiming;
		pNewNotes->TidyUpData();
		song->AddSteps(pNewNotes);
	}

	song->m_SongTiming.m_sFile = dir; // songs still have their fallback timing.
	song->m_sSongFileName = dir;
	song->m_fVersion = STEPFILE_VERSION_NUMBER;
	SMLoader::TidyUpData(*song, true);


	if (song->m_sMainTitle == "" || (song->m_sMusicFile == "" && song->m_vsKeysoundFile.empty()))
	{
		LOG->Warn("Main title or music file for '%s' came up blank, forced to fall back on TidyUpData to fix title and paths.  Do not use # or ; in a song title.", dir.c_str());
		// Tell TidyUpData that it's not loaded from the cache because it needs
		// to hit the song folder to find the files that weren't found. -Kyz
		song->TidyUpData(false, false);
	}
}
/*
bool SongCacheIndex::LoadSongFromCache(Song* song, string dir)
{

	db->exec("CREATE TABLE IF NOT EXISTS songs (VERSION, "
		"TITLE, SUBTITLE, ARTIST, TITLETRANSLIT, SUBTITLETRANSLIT, "
		"ARTISTTRANSLIT, GENRE, ORIGIN, CREDIT, BANNER, BACKGROUND, "
		"PREVIEWVID, JACKET, CDIMAGE, DISCIMAGE, LYRICSPATH, CDTITLE, "
		"MUSIC, PREVIEW, INSTRUMENTTRACK, MUSICLENGTH, LASTSECONDHINT, "
		"SAMPLESTART, SAMPLELENGTH, "
		"DISPLAYBPM, SELECTABLE, ANIMATIONS, "
		"FGCHANGES, KEYSOUNDS, OFFSET, "
		"STOPS, DELAYS, BPMS, WARPS, LABELS, "
		"TIMESIGNATURES, TICKCOUNTS, COMBOS, SPEEDS, SCROLLS, FAKES, "
		"FIRSTSECOND, LASTSECOND, SONGFILENAME, HASMUSIC, HASBANNER)");//SSC

	db->exec("CREATE TABLE IF NOT EXISTS charts (VERSION, "
		"CHARTNAME, STEPSTYPE, CHARTSTYLE, DESCRIPTION, DIFFICULTY, "
		"METER, RADARVALUES, CREDIT, MUSIC, BPMS, STOPS, DELAYS, "
		"TIMESIGNATURES, TICKCOUNTS, COMBOS, WARPS, SPEEDS, SCROLLS, "
		"FAKES, LABELS, OFFSET, DISPLAYBPM, CHARTKEY, MSDVALUES");


	SQLite::Statement query(*db, "SELECT * FROM songs WHERE DIR=" + dir);
	if (!query.tryExecuteStep())
		return false;

	//SSC::StepsTagInfo reused_steps_info(&*song, &out, dir, true);
	SSCLoader loader;
	int songid = query.getColumn(0);
	int index = 1;
	song->m_fVersion = static_cast<double>(query.getColumn(index++));
	song->m_sMainTitle = static_cast<const char *>(query.getColumn(index++));
	song->m_sSubTitle = static_cast<const char *>(query.getColumn(index++));
	song->m_sArtist = static_cast<const char *>(query.getColumn(index++));
	song->m_sMainTitleTranslit = static_cast<const char *>(query.getColumn(index++));
	song->m_sSubTitleTranslit = static_cast<const char *>(query.getColumn(index++));
	song->m_sArtistTranslit = static_cast<const char *>(query.getColumn(index++));
	song->m_sGenre = static_cast<const char *>(query.getColumn(index++));
	song->m_sOrigin = static_cast<const char *>(query.getColumn(index++));
	song->m_sCredit = static_cast<const char *>(query.getColumn(index++));
	Trim(song->m_sCredit);
	song->m_sBannerFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sBackgroundFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sPreviewVidFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sJacketFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sCDFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sDiscFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sLyricsFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sCDTitleFile = static_cast<const char *>(query.getColumn(index++));
	song->m_sMusicFile = static_cast<const char *>(query.getColumn(index++));
	song->m_PreviewFile = static_cast<const char *>(query.getColumn(index++));
	loader.ProcessInstrumentTracks(*song, static_cast<const char *>(query.getColumn(index++)));
	song->m_fMusicLengthSeconds = static_cast<double>(query.getColumn(index++));
	song->SetSpecifiedLastSecond(static_cast<double>(query.getColumn(index++)));
	song->m_fMusicSampleStartSeconds = static_cast<double>(query.getColumn(index++));
	song->m_fMusicSampleLengthSeconds = static_cast<double>(query.getColumn(index++));
	song->m_SongTiming.m_fBeat0OffsetInSeconds = static_cast<double>(query.getColumn(index++));
	string BPMmin = static_cast<const char *>(query.getColumn(index++));
	string BPMmax = static_cast<const char *>(query.getColumn(index++));
	if (BPMmin == "*")
	{
		song->m_DisplayBPMType = DISPLAY_BPM_RANDOM;
	}
	else
	{
		song->m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
		song->m_fSpecifiedBPMMin = StringToFloat(BPMmin);
		if (BPMmax.empty())
		{
			song->m_fSpecifiedBPMMax = song->m_fSpecifiedBPMMin;
		}
		else
		{
			song->m_fSpecifiedBPMMax = StringToFloat(BPMmax);
		}
	}
	RString selection = static_cast<const char *>(query.getColumn(index++));
	if (selection.EqualsNoCase("YES"))
	{
		song->m_SelectionDisplay = song->SHOW_ALWAYS;
	}
	else if (selection.EqualsNoCase("NO"))
	{
		song->m_SelectionDisplay = song->SHOW_NEVER;
	}
	// ROULETTE from 3.9 is no longer in use.
	else if (selection.EqualsNoCase("ROULETTE"))
	{
		song->m_SelectionDisplay = song->SHOW_ALWAYS;
	}

	else if (selection.EqualsNoCase("ES") || selection.EqualsNoCase("OMES"))
	{
		song->m_SelectionDisplay = song->SHOW_ALWAYS;
	}
	else if (StringToInt(selection) > 0)
	{
		song->m_SelectionDisplay = song->SHOW_ALWAYS;
	}
	else
	{
		LOG->UserLog("Song file", dir, "has an unknown #SELECTABLE value, \"%s\"; ignored.", selection.c_str());
	}
	string animations = static_cast<const char *>(query.getColumn(index++));
	string animationstwo = static_cast<const char *>(query.getColumn(index++));
	loader.ProcessBGChanges(*song, animations,
		dir, animationstwo);
	vector<RString> aFGChangeExpressions;
	split(static_cast<const char *>(query.getColumn(index++)), ",", aFGChangeExpressions);

	for (size_t b = 0; b < aFGChangeExpressions.size(); ++b)
	{
		BackgroundChange change;
		if (loader.LoadFromBGChangesString(change, aFGChangeExpressions[b]))
		{
			song->AddForegroundChange(change);
		}
	}
	RString keysounds = static_cast<const char *>(query.getColumn(index++));
	if (keysounds.length() >= 2 && keysounds.substr(0, 2) == "\\#")
	{
		keysounds = keysounds.substr(1);
	}
	split(keysounds, ",", song->m_vsKeysoundFile);
	loader.ProcessStops(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessDelays(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessBPMs(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessWarps(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)), song->m_fVersion);
	loader.ProcessLabels(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessTimeSignatures(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessTickcounts(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessCombos(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessSpeeds(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessScrolls(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	loader.ProcessFakes(song->m_SongTiming, static_cast<const char *>(query.getColumn(index++)));
	song->SetFirstSecond(static_cast<double>(query.getColumn(index++)));
	song->SetLastSecond(static_cast<double>(query.getColumn(index++)));
	song->m_sSongFileName = static_cast<const char *>(query.getColumn(index++));
	song->m_bHasMusic = static_cast<int>(query.getColumn(index++)) != 0;
	song->m_bHasBanner = static_cast<int>(query.getColumn(index++)) != 0;

	Steps* pNewNotes = nullptr;
	TimingData stepsTiming;
	pNewNotes->m_Timing = stepsTiming;
	SQLite::Statement qSteps(*db, "SELECT * FROM steps WHERE SONGID=" + to_string(songid));

	while (query.tryExecuteStep()) {

		//state = GETTING_STEP_INFO;
		pNewNotes = song->CreateSteps();
		stepsTiming = TimingData(song->m_SongTiming.m_fBeat0OffsetInSeconds);
		reused_steps_info.has_own_timing = false;
		reused_steps_info.steps = pNewNotes;
		reused_steps_info.timing = &stepsTiming;

		if (handler != parser_helper.steps_tag_handlers.end())
		{
			handler->second(reused_steps_info);
		}
		else if (sValueName == "NOTES" || sValueName == "NOTES2")
		{
			//state = GETTING_SONG_INFO;
			if (reused_steps_info.has_own_timing)
			{
				pNewNotes->m_Timing = stepsTiming;
			}
			reused_steps_info.has_own_timing = false;
			pNewNotes->SetSMNoteData(sParams[1]);
			pNewNotes->TidyUpData();
			pNewNotes->SetFilename(sPath);
			out.AddSteps(pNewNotes);
		}
		else if (sValueName == "STEPFILENAME")
		{
			state = GETTING_SONG_INFO;
			if (reused_steps_info.has_own_timing)
			{
				pNewNotes->m_Timing = stepsTiming;
			}
			reused_steps_info.has_own_timing = false;
			pNewNotes->SetFilename(sParams[1]);
			out.AddSteps(pNewNotes);
		}
	}

	//Up to here its in order
	//wip



	song->m_fVersion = StringToFloat(static_cast<const char *>(query.getColumn(index++))); 
	RString name = static_cast<const char *>(query.getColumn(index++));
	Trim(name);
	info.steps->SetChartName(name);
	info.steps->m_StepsType = GAMEMAN->StringToStepsType(static_cast<const char *>(query.getColumn(index++)));
	info.steps->m_StepsTypeStr = static_cast<const char *>(query.getColumn(index++));
	info.steps->SetChartStyle(static_cast<const char *>(query.getColumn(index++)));
	RString name = static_cast<const char *>(query.getColumn(index++));
	Trim(name);
	info.steps->SetChartName(name);
	info.steps->SetDifficulty(StringToDifficulty(static_cast<const char *>(query.getColumn(index++)));
	info.steps->SetMeter(StringToInt(static_cast<const char *>(query.getColumn(index++)));
	vector<RString> values;
	split((*info.params)[1], ",", values, true);
	RadarValues rv;
	rv.Zero();
	for (size_t i = 0; i < NUM_RadarCategory; ++i)
		rv[i] = StringToInt(values[i]);
	//info.steps->SetCachedRadarValues(rv);
	//info.steps->SetCredit((*info.params)[1]);
	//info.steps->SetMusicFile((*info.params)[1]);
	loader.ProcessBPMs(*info.timing, (*info.params)[1]);
	loader.ProcessStops(*info.timing, (*info.params)[1]);
	loader.ProcessDelays(*info.timing, (*info.params)[1]);
	loader.ProcessTimeSignatures(*info.timing, (*info.params)[1]);
	loader.ProcessTickcounts(*info.timing, (*info.params)[1]);
	loader.ProcessCombos(*info.timing, (*info.params)[1]);
	loader.ProcessWarps(*info.timing, (*info.params)[1], song->m_fVersion);
	loader.ProcessSpeeds(*info.timing, (*info.params)[1]);
	loader.ProcessScrolls(*info.timing, (*info.params)[1]);
	loader.ProcessFakes(*info.timing, (*info.params)[1]);
	loader.ProcessLabels(*info.timing, (*info.params)[1]);
	info.timing->m_fBeat0OffsetInSeconds = StringToFloat((*info.params)[1]);
	void SetStepsDisplayBPM(SSC::StepsTagInfo& info)
	{
		// #DISPLAYBPM:[xxx][xxx:xxx]|[*];
		if ((*info.params)[1] == "*")
		{
			info.steps->SetDisplayBPM(DISPLAY_BPM_RANDOM);
		}
		else
		{
			info.steps->SetDisplayBPM(DISPLAY_BPM_SPECIFIED);
			float min = StringToFloat((*info.params)[1]);
			info.steps->SetMinBPM(min);
			if ((*info.params)[2].empty())
			{
				info.steps->SetMaxBPM(min);
			}
			else
			{
				info.steps->SetMaxBPM(StringToFloat((*info.params)[2]));
			}
		}
	}

	//info.steps->SetChartKey((*info.params)[1]);

	vector<float> msdsplit(const RString& s) {
		vector<float> o;
		for (size_t i = 0; i < s.size(); ++i) {
			o.emplace_back(StringToFloat(s.substr(i, 5)));
			i += 5;
		}
		return o;
	}

	void SetMSDValues(SSC::StepsTagInfo& info) {
		MinaSD o;

		// Optimize by calling those only once instead of multiple times inside the loop.
		auto params = (*info.params);
		auto size = params.params.size();
		// Start from index 1
		for (size_t i = 1; i <= size; i++)
			o.emplace_back(msdsplit(params[i]));
		info.steps->SetAllMSD(o);
	}
	//end wip
	song->m_SongTiming.m_sFile = dir; // songs still have their fallback timing.
	song->m_sSongFileName = dir;


	song->m_fVersion = STEPFILE_VERSION_NUMBER;
	SMLoader::TidyUpData(*song, true);


	if (song->m_sMainTitle == "" || (song->m_sMusicFile == "" && song->m_vsKeysoundFile.empty()))
	{
		LOG->Warn("Main title or music file for '%s' came up blank, forced to fall back on TidyUpData to fix title and paths.  Do not use # or ; in a song title.", dir.c_str());
		// Tell TidyUpData that it's not loaded from the cache because it needs
		// to hit the song folder to find the files that weren't found. -Kyz
		song->TidyUpData(false, false);
	}
}
	*/
/*
 * (c) 2002-2003 Glenn Maynard
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
