#include "Etterna/Globals/global.h"

#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Misc/RageThreads.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "SongCacheIndex.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSSC.h"
#include <algorithm>
#include "Etterna/Models/NoteWriters/NotesWriterSSC.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
#include "sqlite3.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <algorithm>
#include <numeric>

/*
 * A quick explanation of song cache hashes: Each song has two hashes; a hash of
 * the song path, and a hash of the song directory.  The former is
 * Song::GetCacheFilePath; it stays the same if the contents of the directory
 * change.  The latter is GetHashForDirectory(m_sSongDir), and changes on each
 * modification.
 *
 * The file hash is used as the cache filename.  We don't want to use the
 * directory hash: if we do that, then we'll write a new cache file every time
 * the song changes, and they'll accumulate or we'll have to be careful to
 * delete them.
 *
 * The directory hash is stored in here, indexed by the song path, and used to
 * determine if a song has changed.
 *
 * Another advantage of this system is that we can load songs from cache given
 * only their path; we don't have to actually look in the directory (to find out
 * the directory hash) in order to find the cache file.
 */
const string CACHE_DB = SpecialFiles::CACHE_DIR + "cache.db";
const unsigned int CACHE_DB_VERSION = 239;

SongCacheIndex* SONGINDEX; // global and accessible from anywhere in our program

RString
SongCacheIndex::GetCacheFilePath(const RString& sGroup, const RString& sPath)
{
	/* Don't use GetHashForFile, since we don't want to spend time
	 * checking the file size and date. */
	RString s;

	if (sPath.size() > 2 && sPath[0] == '/' && sPath[sPath.size() - 1] == '/')
		s.assign(sPath, 1, sPath.size() - 2);
	else if (sPath.size() > 1 && sPath[0] == '/')
		s.assign(sPath, 1, sPath.size() - 1);
	else
		s = sPath;
	/* Change slashes and invalid utf-8 characters to _.
	 * http://en.wikipedia.org/wiki/UTF-8
	 * Mac OS X doesn't support precomposed unicode characters in files names
	 * and so we should probably replace them with combining diacritics.
	 * XXX How do we do this and is it even worth it? */
	const char* invalid =
	  "/\xc0\xc1\xfe\xff\xf8\xf9\xfa\xfb\xfc\xfd\xf5\xf6\xf7";
	for (size_t pos = s.find_first_of(invalid); pos != RString::npos;
		 pos = s.find_first_of(invalid, pos))
		s[pos] = '_';
	// CACHE_DIR ends with a /.
	return ssprintf(
	  "%s%s/%s", SpecialFiles::CACHE_DIR.c_str(), sGroup.c_str(), s.c_str());
}

SongCacheIndex::SongCacheIndex()
{
	// Should prevent crashes when /Cache/ doesnt exist
	if (!FILEMAN->IsADirectory(SpecialFiles::CACHE_DIR))
		FILEMAN->CreateDir(SpecialFiles::CACHE_DIR);
	DBEmpty = !OpenDB();
	delay_save_cache = false;
}

int64_t
SongCacheIndex::InsertStepsTimingData(const TimingData& timing)
{
	SQLite::Statement insertTimingData(*db,
									   "INSERT INTO timingdatas VALUES (NULL, "
									   "?, ?, ?, "
									   "?, ?, ?, ?, "
									   "?, ?, ?, ?, ?)");
	/*SQLite::Statement insertTimingData(*db, "INSERT INTO timingdatas VALUES
	   (NULL, " "OFFSET=?, BPMS=?, STOPS=?, " "DELAYS=?, WARPS=?,
	   TIMESIGNATURESEGMENT=?, TICKCOUNTS=?, " "COMBOS=?, SPEEDS=?, SCROLLS=?,
	   FAKES=?, LABELS=?)");*/
	unsigned int timingDataIndex = 1;
	insertTimingData.bind(timingDataIndex++, timing.m_fBeat0OffsetInSeconds);
	{
		vector<TimingSegment*> const& segs =
		  timing.GetTimingSegments(SEGMENT_BPM);
		string bpms = "";
		if (!segs.empty()) {
			for (auto&& seg : segs) {
				const BPMSegment* segment = ToBPM(seg);
				bpms.append(ssprintf("%.6f=%.6f,",
									 NoteRowToBeat(segment->GetRow()),
									 segment->GetBPM()));
			}
			bpms = bpms.substr(0, bpms.size() - 1); // Remove trailing ','
		}
		insertTimingData.bind(timingDataIndex++, bpms);
	}
	{
		vector<TimingSegment*> const& segs =
		  timing.GetTimingSegments(SEGMENT_STOP);
		string stops = "";
		if (!segs.empty()) {
			for (auto&& seg : segs) {
				const StopSegment* segment = ToStop(seg);
				stops.append(ssprintf("%.6f=%.6f",
									  NoteRowToBeat(segment->GetRow()),
									  segment->GetPause()));
			}
		}
		insertTimingData.bind(timingDataIndex++, stops);
	}
	{
		vector<TimingSegment*> const& segs =
		  timing.GetTimingSegments(SEGMENT_DELAY);
		string delays = "";
		if (!segs.empty()) {
			for (auto&& seg : segs) {
				const DelaySegment* segment = ToDelay(seg);
				delays.append(ssprintf("%.6f=%.6f",
									   NoteRowToBeat(segment->GetRow()),
									   segment->GetPause()));
			}
		}
		insertTimingData.bind(timingDataIndex++, delays);
	}
	{
		vector<TimingSegment*> const& segs =
		  timing.GetTimingSegments(SEGMENT_WARP);
		string warps = "";
		if (!segs.empty()) {
			for (auto&& seg : segs) {
				const WarpSegment* segment = ToWarp(seg);
				warps.append(ssprintf("%.6f=%.6f",
									  NoteRowToBeat(segment->GetRow()),
									  segment->GetLength()));
			}
		}
		insertTimingData.bind(timingDataIndex++, warps);
	}
	{
		vector<TimingSegment*> const& segs =
		  timing.GetTimingSegments(SEGMENT_TIME_SIG);
		string timesigs = "";
		if (!segs.empty()) {
			for (auto&& seg : segs) {
				const TimeSignatureSegment* segment = ToTimeSignature(seg);
				timesigs.append(ssprintf("%.6f=%d=%d",
										 NoteRowToBeat(segment->GetRow()),
										 segment->GetNum(),
										 segment->GetDen()));
			}
		}
		insertTimingData.bind(timingDataIndex++, timesigs);
	}
	{
		vector<TimingSegment*> const& segs =
		  timing.GetTimingSegments(SEGMENT_TICKCOUNT);
		string ticks = "";
		if (!segs.empty()) {
			for (auto&& seg : segs) {
				const TickcountSegment* segment = ToTickcount(seg);
				ticks.append(ssprintf("%.6f=%d",
									  NoteRowToBeat(segment->GetRow()),
									  segment->GetTicks()));
			}
		}
		insertTimingData.bind(timingDataIndex++, ticks);
	}
	{
		vector<TimingSegment*> const& segs =
		  timing.GetTimingSegments(SEGMENT_COMBO);
		string combos = "";
		if (!segs.empty()) {
			for (auto&& seg : segs) {
				const ComboSegment* segment = ToCombo(seg);
				if (segment->GetCombo() == segment->GetMissCombo()) {
					combos.append(ssprintf("%.6f=%d",
										   NoteRowToBeat(segment->GetRow()),
										   segment->GetCombo()));
				} else {
					combos.append(ssprintf("%.6f=%d=%d",
										   NoteRowToBeat(segment->GetRow()),
										   segment->GetCombo(),
										   segment->GetMissCombo()));
				}
			}
		}
		insertTimingData.bind(timingDataIndex++, combos);
	}
	{
		vector<TimingSegment*> const& segs =
		  timing.GetTimingSegments(SEGMENT_SPEED);
		string speeds = "";
		if (!segs.empty()) {
			for (auto&& seg : segs) {
				const SpeedSegment* segment = ToSpeed(seg);
				speeds.append(ssprintf("%.6f=%.6f=%.6f=%hd",
									   NoteRowToBeat(segment->GetRow()),
									   segment->GetRatio(),
									   segment->GetDelay(),
									   segment->GetUnit()));
			}
		}
		insertTimingData.bind(timingDataIndex++, speeds);
	}
	{
		vector<TimingSegment*> const& segs =
		  timing.GetTimingSegments(SEGMENT_SCROLL);
		string scrolls = "";
		if (!segs.empty()) {
			for (auto&& seg : segs) {
				const ScrollSegment* segment = ToScroll(seg);
				scrolls.append(ssprintf("%.6f=%.6f",
										NoteRowToBeat(segment->GetRow()),
										segment->GetRatio()));
			}
		}
		insertTimingData.bind(timingDataIndex++, scrolls);
	}
	{
		vector<TimingSegment*> const& segs =
		  timing.GetTimingSegments(SEGMENT_LABEL);
		string labels = "";
		if (!segs.empty()) {
			for (auto&& seg : segs) {
				const LabelSegment* segment = ToLabel(seg);
				if (!segment->GetLabel().empty()) {
					labels.append(ssprintf("%.6f=%s",
										   NoteRowToBeat(segment->GetRow()),
										   segment->GetLabel().c_str()));
				}
			}
		}
		insertTimingData.bind(timingDataIndex++, labels);
	}
	try {
		insertTimingData.exec();
	} catch (exception e) {
		LOG->Warn(
		  "Failed to execute statement to insert TimingData from Cache: %s",
		  e.what());
	}
	return sqlite3_last_insert_rowid(db->getHandle());
}

int64_t
SongCacheIndex::InsertSteps(const Steps* pSteps, int64_t songID)
{
	SQLite::Statement insertSteps(*db,
								  "INSERT INTO steps VALUES (NULL, "
								  "?, ?, ?, ?, ?, "
								  "?, ?, ?, "
								  "?, ?, ?, "
								  "?, ?, ?, ?, ?)");
	vector<RString> lines;
	int stepsIndex = 1;
	insertSteps.bind(stepsIndex++, pSteps->GetChartName());
	insertSteps.bind(stepsIndex++, pSteps->m_StepsTypeStr);
	insertSteps.bind(stepsIndex++, pSteps->GetDescription());
	insertSteps.bind(stepsIndex++, pSteps->GetChartStyle());
	insertSteps.bind(stepsIndex++, pSteps->GetDifficulty());
	insertSteps.bind(stepsIndex++, pSteps->GetMeter());

	MinaSD o = pSteps->GetAllMSD();
	insertSteps.bind(stepsIndex++, NotesWriterSSC::MSDToString(o));

	insertSteps.bind(stepsIndex++,
					 SmEscape(pSteps->GetChartKey()).c_str()); // chartkey

	insertSteps.bind(stepsIndex++, pSteps->GetMusicFile()); // musicfile

	vector<RString> asRadarValues;
	const RadarValues& rv = pSteps->GetRadarValues();
	FOREACH_ENUM(RadarCategory, rc)
	asRadarValues.emplace_back(ssprintf("%i", rv[rc]));
	insertSteps.bind(stepsIndex++, join(",", asRadarValues).c_str());

	insertSteps.bind(stepsIndex++, pSteps->GetCredit());

	// If the Steps TimingData is not empty, then they have their own
	// timing.  Write out the corresponding tags.
	if (pSteps->m_Timing.empty()) {
		insertSteps.bind(stepsIndex++);
	} else {
		sqlite3_int64 timingDataID = InsertStepsTimingData(pSteps->m_Timing);
		insertSteps.bind(stepsIndex++, timingDataID);
	}

	switch (pSteps->GetDisplayBPM()) {
		case DISPLAY_BPM_ACTUAL:
			insertSteps.bind(stepsIndex++);
			insertSteps.bind(stepsIndex++);
			break;
		case DISPLAY_BPM_SPECIFIED: {
			insertSteps.bind(stepsIndex++, pSteps->GetMinBPM());
			insertSteps.bind(stepsIndex++, pSteps->GetMaxBPM());
			break;
		}
		case DISPLAY_BPM_RANDOM:
			insertSteps.bind(stepsIndex++);
			insertSteps.bind(stepsIndex++, 0.0);
			break;
		default:
			insertSteps.bind(stepsIndex++);
			insertSteps.bind(stepsIndex++);
			break;
	}
	insertSteps.bind(stepsIndex++, pSteps->GetFilename().c_str());
	insertSteps.bind(stepsIndex++, static_cast<long long int>(songID));

	try {
		insertSteps.exec();
	} catch (exception e) {
		LOG->Warn("Failed to execute statement to insert Steps from Cache: %s",
				  e.what());
	}
	return sqlite3_last_insert_rowid(db->getHandle());
}
/*	Save a song to the cache db*/
bool
SongCacheIndex::CacheSong(Song& song, string dir)
{
	DeleteSongFromDBByDir(dir);
	try {
		SQLite::Statement insertSong(*db,
									 "INSERT INTO songs VALUES (NULL, "
									 "?, ?, ?, ?, ?, "
									 "?, ?, ?, "
									 "?, ?, ?, ?, "
									 "?, ?, ?, ?, "
									 "?, ?, ?, ?, ?, "
									 "?, ?, ?, ?, "
									 "?, ?, ?, ?, ?, ?, "
									 "?, ?, ?, ?, "
									 "?, ?, ?, ?, "
									 "?, ?, ?, "
									 "?, ?, ?, "
									 "?, ?, ?, ?, ?, ?, "
									 "?, ?, ?, ?, "
									 "?, ?, ?, ?, "
									 "?, ?)");
		unsigned int index = 1;
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
		if (!vs.empty()) {
			std::string s = join(",", vs);
			insertSong.bind(index++, s);
		} else {
			insertSong.bind(index++);
		}
		insertSong.bind(index++, song.m_SongTiming.m_fBeat0OffsetInSeconds);
		insertSong.bind(index++, song.m_fMusicSampleStartSeconds);
		insertSong.bind(index++, song.m_fMusicSampleLengthSeconds);
		// Selectable should be stored as int
		switch (song.m_SelectionDisplay) {
			default:
				ASSERT_M(0,
						 "An invalid selectable value was found for this "
						 "song!"); // fall through
			case Song::SHOW_ALWAYS:
				insertSong.bind(index++, 0);
				break;
			case Song::SHOW_NEVER:
				insertSong.bind(index++, 1);
				break;
		}

		switch (song.m_DisplayBPMType) {
			case DISPLAY_BPM_ACTUAL:
				// write nothing(Both nulls)
				insertSong.bind(index++);
				insertSong.bind(index++);
				break;
			case DISPLAY_BPM_SPECIFIED:
				insertSong.bind(index++, song.m_fSpecifiedBPMMin);
				insertSong.bind(index++, song.m_fSpecifiedBPMMax);
				break;
			case DISPLAY_BPM_RANDOM:
				// Write only one as null
				insertSong.bind(index++);
				insertSong.bind(index++, 0.0);
				break;
			default:
				insertSong.bind(index++);
				insertSong.bind(index++);
				break;
		}
		insertSong.bind(
		  index++,
		  join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_BPM, 3)));
		insertSong.bind(
		  index++,
		  join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_STOP, 3)));
		insertSong.bind(
		  index++,
		  join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_DELAY, 3)));
		insertSong.bind(
		  index++,
		  join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_WARP, 3)));
		insertSong.bind(
		  index++,
		  join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_TIME_SIG, 3)));
		insertSong.bind(
		  index++,
		  join(",\r\n",
			   song.m_SongTiming.ToVectorString(SEGMENT_TICKCOUNT, 3)));
		insertSong.bind(
		  index++,
		  join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_COMBO, 3)));
		insertSong.bind(
		  index++,
		  join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_SPEED, 3)));
		insertSong.bind(
		  index++,
		  join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_SCROLL, 3)));
		insertSong.bind(
		  index++,
		  join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_FAKE, 3)));
		insertSong.bind(
		  index++,
		  join(",\r\n", song.m_SongTiming.ToVectorString(SEGMENT_LABEL, 3)));
		if (song.GetSpecifiedLastSecond() > 0) {
			insertSong.bind(index++, song.GetSpecifiedLastSecond());
		} else {
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
			if (song.GetBackgroundChanges(b).empty()) {
				insertSong.bind(index++);
				continue; // skip
			}
			for (auto& bgc : song.GetBackgroundChanges(b)) {
				bgchanges.append(bgc.ToString() + ",");
			}

			/* If there's an animation plan at all, add a dummy "-nosongbg-" tag
			 * to indicate that this file doesn't want a song BG entry added at
			 * the end. See SSCLoader::TidyUpData. This tag will be removed on
			 * load. Add it at a very high beat, so it won't cause problems if
			 * loaded in older versions. */
			if (b == 0 && !song.GetBackgroundChanges(b).empty()) {
				bgchanges.append("99999=-nosongbg-=1.000=0=0=0");
			}
			insertSong.bind(index++, bgchanges);
		}

		if (song.GetForegroundChanges().size()) {
			string fgchanges;
			for (auto const& bgc : song.GetForegroundChanges()) {
				fgchanges.append(bgc.ToString() + ",");
			}
			insertSong.bind(index++, fgchanges);
		} else {
			insertSong.bind(index++);
		}

		if (!song.m_vsKeysoundFile.empty()) {
			string keysounds = "";
			for (unsigned i = 0; i < song.m_vsKeysoundFile.size(); i++) {
				keysounds.append(song.m_vsKeysoundFile[i]);
				if (i != song.m_vsKeysoundFile.size() - 1) {
					keysounds.append(",");
				}
			}
			insertSong.bind(index++, keysounds);
		} else {
			insertSong.bind(index++);
		}
		insertSong.bind(index++, song.GetFirstSecond());
		insertSong.bind(index++, song.GetLastSecond());
		insertSong.bind(index++, song.m_sSongFileName.c_str());
		insertSong.bind(index++, song.m_bHasMusic);
		insertSong.bind(index++, song.m_bHasBanner);
		insertSong.bind(index++, song.m_fMusicLengthSeconds);
		insertSong.bind(index++, GetHashForDirectory(song.GetSongDir()));
		insertSong.bind(index++, song.GetSongDir());

		insertSong.bind(index++, song.m_sMusicPath);
		insertSong.bind(index++, song.m_PreviewPath);
		insertSong.bind(index++, song.m_sBannerPath);
		insertSong.bind(index++, song.m_sJacketPath);
		insertSong.bind(index++, song.m_sCDPath);
		insertSong.bind(index++, song.m_sDiscPath);
		insertSong.bind(index++, song.m_sLyricsPath);
		insertSong.bind(index++, song.m_sBackgroundPath);
		insertSong.bind(index++, song.m_sCDTitlePath);
		insertSong.bind(index++, song.m_sPreviewVidPath);

		insertSong.exec();
		int64_t songID = sqlite3_last_insert_rowid(db->getHandle());
		vector<Steps*> vpStepsToSave = song.GetStepsToSave();
		FOREACH_CONST(Steps*, vpStepsToSave, s)
		{
			const Steps* pSteps = *s;
			if (pSteps->GetChartKey() == "") { // Avoid writing cache tags for
											   // invalid chartkey files(empty
											   // steps) -Mina
				LOG->Info("Not caching empty difficulty in file %s",
						  dir.c_str());
				continue;
			}
			int64_t stepsID = InsertSteps(pSteps, songID);
		}
		return true;
	} catch (std::exception& e) {
		LOG->Trace(
		  "Error saving song %s to cache db: %s", dir.c_str(), e.what());
		return false;
	}
}

/*	Reset the DB/
Must be open already	*/
void
SongCacheIndex::DeleteDB()
{
	if (db != nullptr)
		delete db;
	FILEMAN->Remove(CACHE_DB);
	try {
		db = new SQLite::Database(FILEMAN->ResolvePath(CACHE_DB),
								  SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE |
									SQLITE_OPEN_FULLMUTEX);
	} catch (std::exception& e) {
		LOG->Trace("Error reading cache db: %s", e.what());
		if (curTransaction != nullptr) {
			delete curTransaction;
			curTransaction = nullptr;
		}
	}
	return;
}
void
SongCacheIndex::ResetDB()
{
	DeleteDB();
	CreateDBTables();
}
void
SongCacheIndex::CreateDBTables()
{
	try {
		db->exec("CREATE TABLE IF NOT EXISTS dbinfo (ID INTEGER PRIMARY KEY, "
				 "VERSION INTEGER)");
		db->exec(
		  "CREATE TABLE IF NOT EXISTS timingdatas (ID INTEGER PRIMARY KEY, "
		  "OFFSET TEXT, BPMS TEXT, STOPS TEXT, "
		  "DELAYS TEXT, WARPS TEXT, TIMESIGNATURESEGMENT TEXT, TICKCOUNTS "
		  "TEXT, "
		  "COMBOS TEXT, SPEEDS TEXT, SCROLLS TEXT, FAKES TEXT, LABELS TEXT)");
		db->exec(
		  "CREATE TABLE IF NOT EXISTS songs (ID INTEGER PRIMARY KEY, "
		  "VERSION FLOAT, TITLE TEXT, SUBTITLE TEXT, ARTIST TEXT, "
		  "TITLETRANSLIT TEXT, "
		  "SUBTITLETRANSLIT TEXT, ARTISTTRANSLIT TEXT, GENRE TEXT, "
		  "ORIGIN TEXT, CREDIT TEXT, BANNER TEXT, BACKGROUND TEXT, "
		  "PREVIEWVID TEXT, JACKET TEXT, CDIMAGE TEXT, DISCIMAGE TEXT, "
		  "LYRICSFILE TEXT, CDTITLE TEXT, MUSIC TEXT, PREVIEW TEXT, "
		  "INSTRUMENTTRACK TEXT, "
		  "OFFSET FLOAT, SAMPLESTART FLOAT, SAMPLELENGTH FLOAT, SELECTABLE "
		  "INTEGER, "
		  "DISPLAYBPMMIN FLOAT, DISPLAYBPMMAX FLOAT, BPMS TEXT, STOPS TEXT, "
		  "DELAYS TEXT, WARPS TEXT, "
		  "TIMESIGNATURES TEXT, TICKCOUNTS TEXT, COMBOS TEXT, SPEEDS TEXT, "
		  "SCROLLS TEXT, FAKES TEXT, LABELS TEXT, LASTSECONDHINT FLOAT, "
		  "BGCHANGESLAYER1 TEXT, BGCHANGESLAYER2 TEXT, FGCHANGES TEXT, "
		  "KEYSOUNDS TEXT, FIRSTSECOND FLOAT, LASTSECOND FLOAT, "
		  "SONGFILENAME TEXT, HASMUSIC INTEGER, HASBANNER INTEGER, "
		  "MUSICLENGTH FLOAT, DIRHASH INTEGER, DIR TEXT, "
		  "MUSICPATH TEXT, PREVIEWPATH TEXT, BANNERPATH TEXT, JACKETPATH TEXT, "
		  "CDPATH TEXT, DISCPATH TEXT, LYRICSPATH TEXT, BACKGROUNDPATH TEXT, "
		  "CDTITLEPATH TEXT, PREVIEWVIDPATH TEXT)");
		db->exec(
		  "CREATE TABLE IF NOT EXISTS steps (id INTEGER PRIMARY KEY, "
		  "CHARTNAME TEXT, STEPSTYPE TEXT, DESCRIPTION TEXT, CHARTSTYLE "
		  "TEXT, DIFFICULTY INTEGER, "
		  "METER INTEGER, MSD TEXT, CHARTKEY TEXT, "
		  "MUSIC TEXT, RADARVALUES TEXT, CREDIT TEXT, "
		  "TIMINGDATAID INTEGER, DISPLAYBPMMIN FLOAT, DISPLAYBPMMAX FLOAT, "
		  "STEPFILENAME TEXT, SONGID INTEGER, "
		  "CONSTRAINT fk_songid FOREIGN KEY (SONGID) REFERENCES songs(ID), "
		  "CONSTRAINT fk_timingdataid FOREIGN KEY (TIMINGDATAID) REFERENCES "
		  "timingdatas(ID), "
		  "CONSTRAINT unique_diff UNIQUE(SONGID, DIFFICULTY, STEPSTYPE, "
		  "CHARTNAME))");
		db->exec("CREATE INDEX IF NOT EXISTS idx_dirs "
				 "ON songs(DIR, DIRHASH)");
		db->exec("CREATE INDEX IF NOT EXISTS idx_timingdatas "
				 "ON timingdatas(ID)");
		db->exec("CREATE INDEX IF NOT EXISTS idx_steps "
				 "ON steps(SONGID)");
		db->exec("INSERT INTO dbinfo VALUES (NULL, " +
				 to_string(CACHE_DB_VERSION) + ")");
	} catch (SQLite::Exception& e) {
		LOG->Warn("Failed to create Cache DB Tables: %s", e.what());
	}
}
/*	Returns weather or not the db had valid data*/
bool
SongCacheIndex::OpenDB()
{
	bool ret = FILEMAN->IsAFile(("/" + CACHE_DB).c_str());
	if (!ret) {
		ResetDB();
		return false;
	}
	// Try to open ane existing db
	try {
		db = new SQLite::Database(FILEMAN->ResolvePath(CACHE_DB),
								  SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE |
									SQLITE_OPEN_FULLMUTEX);
		StartTransaction();
		SQLite::Statement qDBInfo(*db, "SELECT * FROM dbinfo");

		// Should only have one row so no executeStep loop
		if (!qDBInfo.executeStep()) {
			if (curTransaction != nullptr) {
				delete curTransaction;
				curTransaction = nullptr;
			}
			ResetDB();
			return false;
		}
		unsigned int cacheVersion;
		cacheVersion = qDBInfo.getColumn(1);
		if (cacheVersion == CACHE_DB_VERSION) {
			FinishTransaction();
			return true;
		}
	} catch (std::exception& e) {
		LOG->Trace("Error reading cache db: %s", e.what());
		if (curTransaction != nullptr) {
			delete curTransaction;
			curTransaction = nullptr;
		}
	}
	FinishTransaction();
	ResetDB();
	return false;
}

SongCacheIndex::~SongCacheIndex()
{
	if (db != nullptr) {
		delete db;
		db = nullptr;
	}
	if (curTransaction != nullptr) {
		try {
			curTransaction->commit();
		} catch (exception e) {
			// DB transaction commit failed, we're destructing so we dont care.
			// There really shouldnt be a transaction left anyways
		}
		delete curTransaction;
		curTransaction = nullptr;
	}
}

void
SongCacheIndex::LoadHyperCache(LoadingWindow* ld,
							   map<RString, Song*>& hyperCache)
{
	int count = db->execAndGet("SELECT COUNT(*) FROM songs");
	if (ld && count > 0) {
		ld->SetIndeterminate(false);
		ld->SetProgress(0);
		ld->SetTotalWork(count);
	}
	RString lastDir;
	int progress = 0;
	int onePercent = std::max(count / 100, 1);
	try {
		SQLite::Statement query(*db, "SELECT * FROM songs");

		while (query.executeStep()) {
			Song* s = new Song;
			auto songID = SongFromStatement(s, query);
			hyperCache[songID.first] = s;
			lastDir = songID.first;
			lastDir = lastDir.substr(0, lastDir.find_last_of("/"));
			lastDir = lastDir.substr(0, lastDir.find_last_of("/"));
			// this is a song directory. Load a new song.
			progress++;
			if (ld && progress % onePercent == 0) {
				ld->SetProgress(progress);
				ld->SetText(("Loading Cache\n" + lastDir).c_str());
			}
		}

	} catch (std::exception& e) {
		LOG->Trace("Error reading cache. last dir: %s . Error: %s",
				   lastDir.c_str(),
				   e.what());
		ResetDB();
		return;
	}
	return;
}

template<template<class, class...> class R1,
		 template<class, class...>
		 class R2,
		 class T,
		 class... A1,
		 class... A2>
R1<T, A2...>
join(R1<R2<T, A2...>, A1...> const& outer)
{
	R1<T, A2...> joined;
	joined.reserve(std::accumulate(outer.begin(),
								   outer.end(),
								   std::size_t{},
								   [](size_t size, R2<T, A2...> const& inner) {
									   return size + inner.size();
								   }));
	for (R2<T, A2...> const& inner : outer)
		joined.insert(joined.end(), inner.begin(), inner.end());
	return joined;
}

void
SongCacheIndex::LoadCache(
  LoadingWindow* ld,
  vector<pair<pair<RString, unsigned int>, Song*>*>& cache)
{
	int count = 0;
	try {
		count = db->execAndGet("SELECT COUNT(*) FROM songs");
		if (ld != nullptr && count > 0) {
			ld->SetIndeterminate(false);
			ld->SetText("Loading Cache\n");
			ld->SetProgress(0);
			ld->SetTotalWork(count);
		}
	} catch (exception e) {
		LOG->Warn("Failed to count all from songs table in Cache DB: %s",
				  e.what());
	}
	cache.reserve(count);
	int fivePercent = std::max(count / 100 * 5, 1);
	const int threads = std::thread::hardware_concurrency();
	const int limit = count / threads;
	ThreadData data;
	atomic<bool> abort(false);
	auto threadCallback =
	  [&data, fivePercent, &abort](
		int limit,
		int offset,
		vector<pair<pair<RString, unsigned int>, Song*>*>* cachePart) {
		  int counter = 0, lastUpdate = 0;
		  try {
			  SQLite::Statement query(*SONGINDEX->db,
									  "SELECT * FROM songs LIMIT " +
										to_string(limit) + " OFFSET " +
										to_string(offset));
			  while (query.executeStep()) {
				  if (abort) {
					  return;
				  }
				  Song* s = new Song;
				  auto songID = SONGINDEX->SongFromStatement(s, query);
				  cachePart->emplace_back(
					new pair<pair<RString, unsigned int>, Song*>(songID, s));
				  // this is a song directory. Load a new song.
				  counter++;
				  if (counter % fivePercent == 0) {
					  data._progress += counter - lastUpdate;
					  lastUpdate = counter;
					  data.setUpdated(true);
				  }
			  }

		  } catch (std::exception& e) {
			  LOG->Trace("Error reading cache. Error: %s", e.what());
			  if (abort)
				  return;
			  abort = true;
			  data.setUpdated(true);
			  SONGINDEX->ResetDB();
			  return;
		  }
		  data._threadsFinished++;
		  data.setUpdated(true);
	  };
	vector<thread> threadpool;
	vector<vector<pair<pair<RString, unsigned int>, Song*>*>> cacheParts;
	for (int i = 0; i < threads; i++)
		cacheParts.emplace_back(
		  vector<pair<pair<RString, unsigned int>, Song*>*>());
	for (int i = 0; i < threads; i++)
		threadpool.emplace_back(
		  thread(threadCallback, limit, i * limit, &(cacheParts[i])));
	while (data._threadsFinished < threads) {
		data.waitForUpdate();
		if (abort) {
			for (auto& thread : threadpool)
				thread.join();
			return;
		}
		if (ld) {
			ld->SetProgress(data._progress);
		}
		data.setUpdated(false);
	}
	for (auto& thread : threadpool)
		thread.join();
	cache = join(cacheParts);
	return;
}
void
SongCacheIndex::DeleteSongFromDBByCondition(string& condition)
{
	try {
		db->exec(
		  ("DELETE FROM timingdatas WHERE ID IN (SELECT TIMINGDATAID FROM "
		   "steps WHERE SONGID IN(SELECT ID from songs WHERE " +
		   condition + "))")
			.c_str());
		db->exec(
		  ("DELETE FROM steps WHERE SONGID IN (SELECT ID from songs WHERE " +
		   condition + ")")
			.c_str());
		db->exec(("DELETE FROM songs WHERE " + condition).c_str());
	} catch (exception e) {
		LOG->Warn("Failed to execute Song Deletion from DB with condition "
				  "'%s'\nException: %s",
				  condition.c_str(),
				  e.what());
	}
}
void
SongCacheIndex::DeleteSongFromDB(Song* songPtr)
{
	string cond = "dir = \"" + songPtr->GetSongDir() + "\" AND hash = \"" +
				  to_string(GetHashForDirectory(songPtr->GetSongDir())) + "\"";
	DeleteSongFromDBByCondition(cond);
}
void
SongCacheIndex::DeleteSongFromDBByDir(string dir)
{
	string cond = "dir=\"" + dir + "\"";
	DeleteSongFromDBByCondition(cond);
}
void
SongCacheIndex::DeleteSongFromDBByDirHash(unsigned int hash)
{
	string cond = "hash=\"" + to_string(hash) + "\"";
	DeleteSongFromDBByCondition(cond);
}

unsigned
SongCacheIndex::GetCacheHash(const RString& path) const
{
	unsigned iDirHash = 0;
	if (!CacheIndex.GetValue("Cache", MangleName(path), iDirHash))
		return 0;
	if (iDirHash == 0)
		++iDirHash; /* no 0 hash values */
	return iDirHash;
}

RString
SongCacheIndex::MangleName(const RString& Name)
{
	/* We store paths in an INI.  We can't store '='. */
	RString ret = Name;
	ret.Replace("=", "");
	return ret;
}

void
SongCacheIndex::StartTransaction()
{
	if (db == nullptr)
		return;
	if (curTransaction != nullptr)
		return;
	try {
		curTransaction = new SQLite::Transaction(*db);
	} catch (exception e) {
		LOG->Warn("Failed to start transaction due to exception: %s", e.what());
	}
	return;
}
void
SongCacheIndex::FinishTransaction()
{
	if (curTransaction == nullptr)
		return;
	try {
		curTransaction->commit();
	} catch (exception e) {
		// DB transaction commit failed, we're destructing so we dont care.
		// There really shouldnt be a transaction left anyways
	}
	delete curTransaction;
	curTransaction = nullptr;
	return;
}

inline pair<RString, int>
SongCacheIndex::SongFromStatement(Song* song, SQLite::Statement& query)
{
	// SSC::StepsTagInfo reused_steps_info(&*song, &out, dir, true);
	SSCLoader loader;
	string dir;
	int dirhash;

	try {
		int songid = query.getColumn(0);
		int index = 1;
		song->m_fVersion =
		  static_cast<float>(static_cast<double>(query.getColumn(index++)));
		song->m_sMainTitle = static_cast<const char*>(query.getColumn(index++));
		song->m_sSubTitle = static_cast<const char*>(query.getColumn(index++));
		song->m_sArtist = static_cast<const char*>(query.getColumn(index++));
		song->m_sMainTitleTranslit =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sSubTitleTranslit =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sArtistTranslit =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sGenre = static_cast<const char*>(query.getColumn(index++));
		song->m_sOrigin = static_cast<const char*>(query.getColumn(index++));
		song->m_sCredit = static_cast<const char*>(query.getColumn(index++));
		Trim(song->m_sCredit);
		song->m_sBannerFile =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sBackgroundFile =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sPreviewVidFile =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sJacketFile =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sCDFile = static_cast<const char*>(query.getColumn(index++));
		song->m_sDiscFile = static_cast<const char*>(query.getColumn(index++));
		song->m_sLyricsFile =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sCDTitleFile =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sMusicFile = static_cast<const char*>(query.getColumn(index++));
		song->m_PreviewFile =
		  static_cast<const char*>(query.getColumn(index++));
		loader.ProcessInstrumentTracks(
		  *song, static_cast<const char*>(query.getColumn(index++)));
		song->m_SongTiming.m_fBeat0OffsetInSeconds =
		  static_cast<float>(static_cast<double>(query.getColumn(index++)));
		song->m_fMusicSampleStartSeconds =
		  static_cast<float>(static_cast<double>(query.getColumn(index++)));
		song->m_fMusicSampleLengthSeconds =
		  static_cast<float>(static_cast<double>(query.getColumn(index++)));

		int selection = static_cast<int>(query.getColumn(index++));
		if (selection == 0)
			song->m_SelectionDisplay = song->SHOW_ALWAYS;
		else
			song->m_SelectionDisplay = song->SHOW_NEVER;

		int bpmminIndex = index++;
		int bpmmaxIndex = index++;
		float BPMmin =
		  static_cast<float>(static_cast<double>(query.getColumn(bpmminIndex)));
		float BPMmax =
		  static_cast<float>(static_cast<double>(query.getColumn(bpmmaxIndex)));
		if (query.isColumnNull(bpmminIndex) ||
			query.isColumnNull(bpmmaxIndex)) {
			if (query.isColumnNull(bpmminIndex) &&
				query.isColumnNull(bpmmaxIndex))
				song->m_DisplayBPMType = DISPLAY_BPM_ACTUAL;
			else
				song->m_DisplayBPMType = DISPLAY_BPM_RANDOM;
		} else {
			song->m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
			song->m_fSpecifiedBPMMin = BPMmin;
			song->m_fSpecifiedBPMMax = BPMmax;
		}

		auto title = song->GetMainTitle();
		loader.ProcessBPMs(song->m_SongTiming,
						   static_cast<const char*>(query.getColumn(index++)),
						   title);
		loader.ProcessStops(song->m_SongTiming,
							static_cast<const char*>(query.getColumn(index++)),
							title);
		loader.ProcessDelays(
		  song->m_SongTiming,
		  static_cast<const char*>(query.getColumn(index++)));
		loader.ProcessWarps(song->m_SongTiming,
							static_cast<const char*>(query.getColumn(index++)),
							song->m_fVersion,
							title);
		loader.ProcessTimeSignatures(
		  song->m_SongTiming,
		  static_cast<const char*>(query.getColumn(index++)));
		loader.ProcessTickcounts(
		  song->m_SongTiming,
		  static_cast<const char*>(query.getColumn(index++)));
		loader.ProcessCombos(
		  song->m_SongTiming,
		  static_cast<const char*>(query.getColumn(index++)));
		loader.ProcessSpeeds(
		  song->m_SongTiming,
		  static_cast<const char*>(query.getColumn(index++)));
		loader.ProcessScrolls(
		  song->m_SongTiming,
		  static_cast<const char*>(query.getColumn(index++)),
		  title);
		loader.ProcessFakes(song->m_SongTiming,
							static_cast<const char*>(query.getColumn(index++)));
		loader.ProcessLabels(song->m_SongTiming,
							 static_cast<const char*>(query.getColumn(index++)),
							 title);

		song->SetSpecifiedLastSecond(
		  static_cast<float>(static_cast<double>(query.getColumn(index++))));

		string animations = static_cast<const char*>(query.getColumn(index++));
		string animationstwo =
		  static_cast<const char*>(query.getColumn(index++));

		vector<RString> aFGChangeExpressions;
		split(static_cast<const char*>(query.getColumn(index++)),
			  ",",
			  aFGChangeExpressions);

		for (size_t b = 0; b < aFGChangeExpressions.size(); ++b) {
			BackgroundChange change;
			if (loader.LoadFromBGChangesString(change,
											   aFGChangeExpressions[b])) {
				song->AddForegroundChange(change);
			}
		}
		RString keysounds = static_cast<const char*>(query.getColumn(index++));
		if (keysounds.length() >= 2 && keysounds.substr(0, 2) == "\\#") {
			keysounds = keysounds.substr(1);
		}
		split(keysounds, ",", song->m_vsKeysoundFile);
		song->SetFirstSecond(
		  static_cast<float>(static_cast<double>(query.getColumn(index++))));
		song->SetLastSecond(
		  static_cast<float>(static_cast<double>(query.getColumn(index++))));
		song->m_sSongFileName =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_bHasMusic = static_cast<int>(query.getColumn(index++)) != 0;
		song->m_bHasBanner = static_cast<int>(query.getColumn(index++)) != 0;
		song->m_fMusicLengthSeconds =
		  static_cast<float>(static_cast<double>(query.getColumn(index++)));
		dirhash = query.getColumn(index++);
		string dirt = query.getColumn(index++);
		dir = dirt;

		song->SetSongDir(dir);

		loader.ProcessBGChanges(*song, "BGCHANGES1", dir, animations);
		loader.ProcessBGChanges(*song, "BGCHANGES2", dir, animationstwo);

		Steps* pNewNotes = nullptr;
		
		SQLite::Statement qSteps(
		  *db, "SELECT * FROM steps WHERE SONGID=" + to_string(songid));

		while (qSteps.executeStep()) {
			int stepsIndex = 0;

			pNewNotes = song->CreateSteps();
			int stepsID = qSteps.getColumn(stepsIndex++);
			RString chartName =
			  static_cast<const char*>(qSteps.getColumn(stepsIndex++));
			pNewNotes->SetChartName(chartName);
			string stepsType =
			  static_cast<const char*>(qSteps.getColumn(stepsIndex++));
			pNewNotes->m_StepsType = GAMEMAN->StringToStepsType(stepsType);
			pNewNotes->m_StepsTypeStr = stepsType;
			RString description =
			  static_cast<const char*>(qSteps.getColumn(stepsIndex++));
			pNewNotes->SetDescription(description);
			pNewNotes->SetChartStyle(
			  static_cast<const char*>(qSteps.getColumn(stepsIndex++)));
			pNewNotes->SetDifficulty(static_cast<Difficulty>(
			  static_cast<int>(qSteps.getColumn(stepsIndex++))));
			pNewNotes->SetMeter(qSteps.getColumn(stepsIndex++));

			MinaSD o;
			stringstream msds;
			msds.str(static_cast<const char*>(qSteps.getColumn(stepsIndex++)));
			string msdsatrate;
			while (std::getline(msds, msdsatrate, ':')) {
				auto m = SSC::msdsplit(msdsatrate);
				o.push_back({m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7] });
			}
			pNewNotes->SetAllMSD(o);

			pNewNotes->SetChartKey(
			  static_cast<const char*>(qSteps.getColumn(stepsIndex++)));
			pNewNotes->SetMusicFile(
			  static_cast<const char*>(qSteps.getColumn(stepsIndex++)));
			string radarValues =
			  static_cast<const char*>(qSteps.getColumn(stepsIndex++));
			vector<RString> values;
			split(radarValues, ",", values, true);
			RadarValues rv;
			rv.Zero();
			for (size_t i = 0; i < NUM_RadarCategory; ++i)
				rv[i] = StringToInt(values[i]);
			pNewNotes->SetCachedRadarValues(rv);
			pNewNotes->SetCredit(
			  static_cast<const char*>(qSteps.getColumn(stepsIndex++)));

			/* If this is called, the chart does not use the same attacks
			 * as the Song's timing. No other changes are required. */
			if (!qSteps.isColumnNull(stepsIndex)) {
				int timingID = qSteps.getColumn(stepsIndex++);
				int timingIndex = 1; // Skip the first value, the id
				SQLite::Statement qTiming(
				  *db,
				  "SELECT * FROM timingdatas WHERE ID=" + to_string(timingID));
				if (qTiming.executeStep()) {
					TimingData stepsTiming =
					  TimingData(song->m_SongTiming.m_fBeat0OffsetInSeconds);
					// Load timing data
					stepsTiming.m_fBeat0OffsetInSeconds = static_cast<float>(
					  static_cast<double>(qTiming.getColumn(timingIndex++)));
					SSCLoader::ProcessBPMs(stepsTiming,
										   static_cast<const char*>(
											 qTiming.getColumn(timingIndex++)),
										   dir);
					// steps_tag_handlers["BPMS"] = &SetStepsBPMs;
					SSCLoader::ProcessStops(stepsTiming,
											static_cast<const char*>(
											  qTiming.getColumn(timingIndex++)),
											dir);
					// steps_tag_handlers["STOPS"] = &SetStepsStops;
					SSCLoader::ProcessDelays(
					  stepsTiming,
					  static_cast<const char*>(
						qTiming.getColumn(timingIndex++)),
					  dir);
					// steps_tag_handlers["DELAYS"] = &SetStepsDelays;
					SSCLoader::ProcessWarps(stepsTiming,
											static_cast<const char*>(
											  qTiming.getColumn(timingIndex++)),
											song->m_fVersion,
											dir);
					// steps_tag_handlers["WARPS"] = &SetStepsWarps;
					SSCLoader::ProcessTimeSignatures(
					  stepsTiming,
					  static_cast<const char*>(
						qTiming.getColumn(timingIndex++)),
					  dir);
					// steps_tag_handlers["TIMESIGNATURES"] =
					// &SetStepsTimeSignatures;
					SSCLoader::ProcessTickcounts(
					  stepsTiming,
					  static_cast<const char*>(
						qTiming.getColumn(timingIndex++)),
					  dir);
					// steps_tag_handlers["TICKCOUNTS"] = &SetStepsTickCounts;
					SSCLoader::ProcessCombos(
					  stepsTiming,
					  static_cast<const char*>(
						qTiming.getColumn(timingIndex++)),
					  dir);
					// steps_tag_handlers["COMBOS"] = &SetStepsCombos;
					SSCLoader::ProcessSpeeds(
					  stepsTiming,
					  static_cast<const char*>(
						qTiming.getColumn(timingIndex++)),
					  dir);
					// steps_tag_handlers["SPEEDS"] = &SetStepsSpeeds;
					SSCLoader::ProcessScrolls(
					  stepsTiming,
					  static_cast<const char*>(
						qTiming.getColumn(timingIndex++)),
					  dir);
					// steps_tag_handlers["SCROLLS"] = &SetStepsScrolls;
					SSCLoader::ProcessFakes(stepsTiming,
											static_cast<const char*>(
											  qTiming.getColumn(timingIndex++)),
											dir);
					// steps_tag_handlers["FAKES"] = &SetStepsFakes;
					SSCLoader::ProcessLabels(
					  stepsTiming,
					  static_cast<const char*>(
						qTiming.getColumn(timingIndex++)),
					  dir);
					pNewNotes->m_Timing = stepsTiming;
				}
			} else
				qSteps.getColumn(stepsIndex++);

			int bpmminIndex = stepsIndex++;
			int bpmmaxIndex = stepsIndex++;
			float BPMmin = static_cast<float>(
			  static_cast<double>(qSteps.getColumn(bpmminIndex)));
			float BPMmax = static_cast<float>(
			  static_cast<double>(qSteps.getColumn(bpmmaxIndex)));
			if (qSteps.isColumnNull(bpmminIndex) ||
				qSteps.isColumnNull(bpmmaxIndex)) {
				if (qSteps.isColumnNull(bpmminIndex) &&
					qSteps.isColumnNull(bpmmaxIndex))
					pNewNotes->SetDisplayBPM(DISPLAY_BPM_ACTUAL);
				else
					pNewNotes->SetDisplayBPM(DISPLAY_BPM_RANDOM);
			} else {
				pNewNotes->SetDisplayBPM(DISPLAY_BPM_SPECIFIED);
				pNewNotes->SetMinBPM(BPMmin);
				pNewNotes->SetMaxBPM(BPMmax);
			}

			// pNewNotes->SetSMNoteData("");
			pNewNotes->TidyUpData();
			pNewNotes->SetFilename(
			  static_cast<const char*>(qSteps.getColumn(stepsIndex++)));
			song->AddSteps(pNewNotes);
		}

		song->m_SongTiming.m_sFile =
		  dir; // songs still have their fallback timing.
		song->m_fVersion = STEPFILE_VERSION_NUMBER;

		song->m_sMusicPath = static_cast<const char*>(query.getColumn(index++));
		song->m_PreviewPath =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sBannerPath =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sJacketPath =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sCDPath = static_cast<const char*>(query.getColumn(index++));
		song->m_sDiscPath = static_cast<const char*>(query.getColumn(index++));
		song->m_sLyricsPath =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sBackgroundPath =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sCDTitlePath =
		  static_cast<const char*>(query.getColumn(index++));
		song->m_sPreviewVidPath =
		  static_cast<const char*>(query.getColumn(index++));
	} catch (exception e) {
		LOG->Warn("Exception occurred while loading file from cache: %s",
				  e.what());
	}

	SMLoader::TidyUpData(*song, true);

	if (song->m_sMainTitle == "" ||
		(song->m_sMusicFile == "" && song->m_vsKeysoundFile.empty())) {
		LOG->Warn("Main title or music file for '%s' came up blank, forced to "
				  "fall back on TidyUpData to fix title and paths.  Do not use "
				  "# or ; in a song title.",
				  dir.c_str());
		// Tell TidyUpData that it's not loaded from the cache because it needs
		// to hit the song folder to find the files that weren't found. -Kyz
		song->TidyUpData(false, false);
	}
	return { dir, dirhash };
}
/*	Load a song from Cache DB
	Returns true if it was loaded**/
bool
SongCacheIndex::LoadSongFromCache(Song* song, string dir)
{
	try {
		SQLite::Statement query(
		  *db, "SELECT * FROM songs WHERE DIR=? AND DIRHASH=?");
		query.bind(1, dir);
		query.bind(2, GetHashForDirectory(song->GetSongDir()));

		// No cache entry => return false
		if (!query.executeStep())
			return false;

		SongFromStatement(song, query);
	} catch (std::exception& e) {
		LOG->Trace(
		  "Error reading song %s from cache: %s", dir.c_str(), e.what());
		ResetDB();
		return false;
	}
	return true;
}
