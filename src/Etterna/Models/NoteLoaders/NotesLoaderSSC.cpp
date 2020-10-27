#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/FileTypes/MsdFile.h" // No JSON here.
#include "Etterna/Models/Misc/NoteTypes.h"
#include "NotesLoaderSM.h" // For programming shortcuts.
#include "NotesLoaderSSC.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"

// Everything from this line to the creation of parser_helper exists to
// speed up parsing by allowing the use of std::map.  All these functions
// are put into a map of function pointers which is used when loading.
// -Kyz
/****************************************************************/

using steps_tag_func_t = void (*)(SSC::StepsTagInfo&);
using song_tag_func_t = void (*)(SSC::SongTagInfo&);

// Functions for song tags go below this line. -Kyz
/****************************************************************/
void
SetVersion(SSC::SongTagInfo& info)
{
	info.song->m_fVersion = StringToFloat((*info.params)[1]);
}
void
SetTitle(SSC::SongTagInfo& info)
{
	info.song->m_sMainTitle = (*info.params)[1];
	info.loader->SetSongTitle((*info.params)[1]);
}
void
SetSubtitle(SSC::SongTagInfo& info)
{
	info.song->m_sSubTitle = (*info.params)[1];
}
void
SetArtist(SSC::SongTagInfo& info)
{
	info.song->m_sArtist = (*info.params)[1];
}
void
SetMainTitleTranslit(SSC::SongTagInfo& info)
{
	info.song->m_sMainTitleTranslit = (*info.params)[1];
}
void
SetSubtitleTranslit(SSC::SongTagInfo& info)
{
	info.song->m_sSubTitleTranslit = (*info.params)[1];
}
void
SetArtistTranslit(SSC::SongTagInfo& info)
{
	info.song->m_sArtistTranslit = (*info.params)[1];
}
void
SetGenre(SSC::SongTagInfo& info)
{
	info.song->m_sGenre = (*info.params)[1];
}
void
SetOrigin(SSC::SongTagInfo& info)
{
	info.song->m_sOrigin = (*info.params)[1];
}
void
SetCredit(SSC::SongTagInfo& info)
{
	info.song->m_sCredit = (*info.params)[1];
	Trim(info.song->m_sCredit);
}
void
SetBanner(SSC::SongTagInfo& info)
{
	info.song->m_sBannerFile = (*info.params)[1];
}
void
SetBackground(SSC::SongTagInfo& info)
{
	info.song->m_sBackgroundFile = (*info.params)[1];
}
void
SetPreviewVid(SSC::SongTagInfo& info)
{
	info.song->m_sPreviewVidFile = (*info.params)[1];
}
void
SetJacket(SSC::SongTagInfo& info)
{
	info.song->m_sJacketFile = (*info.params)[1];
}
void
SetCDImage(SSC::SongTagInfo& info)
{
	info.song->m_sCDFile = (*info.params)[1];
}
void
SetDiscImage(SSC::SongTagInfo& info)
{
	info.song->m_sDiscFile = (*info.params)[1];
}
void
SetLyricsPath(SSC::SongTagInfo& info)
{
	info.song->m_sLyricsFile = (*info.params)[1];
}
void
SetCDTitle(SSC::SongTagInfo& info)
{
	info.song->m_sCDTitleFile = (*info.params)[1];
}
void
SetMusic(SSC::SongTagInfo& info)
{
	info.song->m_sMusicFile = (*info.params)[1];
}
void
SetPreview(SSC::SongTagInfo& info)
{
	info.song->m_PreviewFile = (*info.params)[1];
}
void
SetInstrumentTrack(SSC::SongTagInfo& info)
{
	info.loader->ProcessInstrumentTracks(*info.song, (*info.params)[1]);
}
void
SetMusicLength(SSC::SongTagInfo& info)
{
	if (info.from_cache)
		info.song->m_fMusicLengthSeconds = StringToFloat((*info.params)[1]);
}
void
SetLastSecondHint(SSC::SongTagInfo& info)
{
	info.song->SetSpecifiedLastSecond(StringToFloat((*info.params)[1]));
}
void
SetSampleStart(SSC::SongTagInfo& info)
{
	info.song->m_fMusicSampleStartSeconds = HHMMSSToSeconds((*info.params)[1]);
}
void
SetSampleLength(SSC::SongTagInfo& info)
{
	info.song->m_fMusicSampleLengthSeconds = HHMMSSToSeconds((*info.params)[1]);
}
void
SetDisplayBPM(SSC::SongTagInfo& info)
{
	// #DISPLAYBPM:[xxx][xxx:xxx]|[*];
	if ((*info.params)[1] == "*") {
		info.song->m_DisplayBPMType = DISPLAY_BPM_RANDOM;
	} else {
		info.song->m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
		info.song->m_fSpecifiedBPMMin = StringToFloat((*info.params)[1]);
		if ((*info.params)[2].empty()) {
			info.song->m_fSpecifiedBPMMax = info.song->m_fSpecifiedBPMMin;
		} else {
			info.song->m_fSpecifiedBPMMax = StringToFloat((*info.params)[2]);
		}
	}
}
void
SetSelectable(SSC::SongTagInfo& info)
{
	if (EqualsNoCase((*info.params)[1], "YES")) {
		info.song->m_SelectionDisplay = info.song->SHOW_ALWAYS;
	} else if (EqualsNoCase((*info.params)[1], "NO")) {
		info.song->m_SelectionDisplay = info.song->SHOW_NEVER;
	}
	// ROULETTE from 3.9 is no longer in use.
	else if (EqualsNoCase((*info.params)[1], "ROULETTE")) {
		info.song->m_SelectionDisplay = info.song->SHOW_ALWAYS;
	}
	/* The following two cases are just fixes to make sure simfiles that
	 * used 3.9+ features are not excluded here */
	else if (EqualsNoCase((*info.params)[1], "ES") ||
			 EqualsNoCase((*info.params)[1], "OMES")) {
		info.song->m_SelectionDisplay = info.song->SHOW_ALWAYS;
	} else if (StringToInt((*info.params)[1]) > 0) {
		info.song->m_SelectionDisplay = info.song->SHOW_ALWAYS;
	} else {
//		LOG->UserLog("Song file",
//					 info.path,
//					 "has an unknown #SELECTABLE value, \"%s\"; ignored.",
//					 (*info.params)[1].c_str());
	}
}
void
SetBGChanges(SSC::SongTagInfo& info)
{
	info.loader->ProcessBGChanges(
	  *info.song, (*info.params)[0], info.path, (*info.params)[1]);
}
void
SetFGChanges(SSC::SongTagInfo& info)
{
	vector<std::string> aFGChangeExpressions;
	split((*info.params)[1], ",", aFGChangeExpressions);

	for (auto& aFGChangeExpression : aFGChangeExpressions) {
		BackgroundChange change;
		if (info.loader->LoadFromBGChangesString(change, aFGChangeExpression)) {
			info.song->AddForegroundChange(change);
		}
	}
}
void
SetKeysounds(SSC::SongTagInfo& info)
{
	std::string keysounds = (*info.params)[1];
	if (keysounds.length() >= 2 && keysounds.substr(0, 2) == "\\#") {
		keysounds = keysounds.substr(1);
	}
	split(keysounds, ",", info.song->m_vsKeysoundFile);
}
void
SetOffset(SSC::SongTagInfo& info)
{
	info.song->m_SongTiming.m_fBeat0OffsetInSeconds =
	  StringToFloat((*info.params)[1]);
}
void
SetSongStops(SSC::SongTagInfo& info)
{
	info.loader->ProcessStops(
	  info.song->m_SongTiming, (*info.params)[1], info.loader->GetSongTitle());
}
void
SetSongDelays(SSC::SongTagInfo& info)
{
	info.loader->ProcessDelays(info.song->m_SongTiming, (*info.params)[1]);
}
void
SetSongBPMs(SSC::SongTagInfo& info)
{
	info.loader->ProcessBPMs(
	  info.song->m_SongTiming, (*info.params)[1], info.loader->GetSongTitle());
}
void
SetSongWarps(SSC::SongTagInfo& info)
{
	info.loader->ProcessWarps(info.song->m_SongTiming,
							  (*info.params)[1],
							  info.song->m_fVersion,
							  info.loader->GetSongTitle());
}
void
SetSongLabels(SSC::SongTagInfo& info)
{
	auto name = info.loader->GetSongTitle();
	info.loader->ProcessLabels(
	  info.song->m_SongTiming, (*info.params)[1], name);
}
void
SetSongTimeSignatures(SSC::SongTagInfo& info)
{
	info.loader->ProcessTimeSignatures(info.song->m_SongTiming,
									   (*info.params)[1]);
}
void
SetSongTickCounts(SSC::SongTagInfo& info)
{
	info.loader->ProcessTickcounts(info.song->m_SongTiming, (*info.params)[1]);
}
void
SetSongCombos(SSC::SongTagInfo& info)
{
	info.loader->ProcessCombos(info.song->m_SongTiming, (*info.params)[1]);
}
void
SetSongSpeeds(SSC::SongTagInfo& info)
{
	info.loader->ProcessSpeeds(info.song->m_SongTiming, (*info.params)[1]);
}
void
SetSongScrolls(SSC::SongTagInfo& info)
{
	auto name = info.loader->GetSongTitle();
	info.loader->ProcessScrolls(
	  info.song->m_SongTiming, (*info.params)[1], name);
}
void
SetSongFakes(SSC::SongTagInfo& info)
{
	info.loader->ProcessFakes(info.song->m_SongTiming, (*info.params)[1]);
}
void
SetFirstSecond(SSC::SongTagInfo& info)
{
	if (info.from_cache) {
		info.song->SetFirstSecond(StringToFloat((*info.params)[1]));
	}
}
void
SetLastSecond(SSC::SongTagInfo& info)
{
	if (info.from_cache) {
		info.song->SetLastSecond(StringToFloat((*info.params)[1]));
	}
}
void
SetSongFilename(SSC::SongTagInfo& info)
{
	if (info.from_cache) {
		info.song->m_sSongFileName = (*info.params)[1];
	}
}
void
SetHasMusic(SSC::SongTagInfo& info)
{
	if (info.from_cache) {
		info.song->m_bHasMusic = StringToInt((*info.params)[1]) != 0;
	}
}
void
SetHasBanner(SSC::SongTagInfo& info)
{
	if (info.from_cache) {
		info.song->m_bHasBanner = StringToInt((*info.params)[1]) != 0;
	}
}

// Functions for steps tags go below this line. -Kyz
/****************************************************************/
void
SetStepsVersion(SSC::StepsTagInfo& info)
{
	info.song->m_fVersion = StringToFloat((*info.params)[1]);
}
void
SetChartName(SSC::StepsTagInfo& info)
{
	std::string name = (*info.params)[1];
	Trim(name);
	info.steps->SetChartName(name);
}
void
SetStepsType(SSC::StepsTagInfo& info)
{
	info.steps->m_StepsType = GAMEMAN->StringToStepsType((*info.params)[1]);
	info.steps->m_StepsTypeStr = (*info.params)[1];
	info.ssc_format = true;
}
void
SetChartStyle(SSC::StepsTagInfo& info)
{
	info.steps->SetChartStyle((*info.params)[1]);
	info.ssc_format = true;
}
void
SetDescription(SSC::StepsTagInfo& info)
{
	std::string name = (*info.params)[1];
	Trim(name);
	if (info.song->m_fVersion < VERSION_CHART_NAME_TAG && !info.for_load_edit) {
		info.steps->SetChartName(name);
	} else {
		info.steps->SetDescription(name);
	}
	info.ssc_format = true;
}
void
SetDifficulty(SSC::StepsTagInfo& info)
{
	info.steps->SetDifficulty(StringToDifficulty((*info.params)[1]));
	info.ssc_format = true;
}
void
SetMeter(SSC::StepsTagInfo& info)
{
	info.steps->SetMeter(StringToInt((*info.params)[1]));
	info.ssc_format = true;
}
void
SetRadarValues(SSC::StepsTagInfo& info)
{
	if (info.from_cache || info.for_load_edit) {
		vector<std::string> values;
		split((*info.params)[1], ",", values, true);
		RadarValues rv;
		rv.Zero();
		for (size_t i = 0; i < NUM_RadarCategory; ++i)
			rv[i] = StringToInt(values[i]);
		info.steps->SetCachedRadarValues(rv);
	} else {
		// just recalc at time.
	}
	info.ssc_format = true;
}
void
SetCredit(SSC::StepsTagInfo& info)
{
	info.steps->SetCredit((*info.params)[1]);
	info.ssc_format = true;
}
void
SetStepsMusic(SSC::StepsTagInfo& info)
{
	info.steps->SetMusicFile((*info.params)[1]);
}
void
SetStepsBPMs(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessBPMs(
		  *info.timing, (*info.params)[1], info.loader->GetSongTitle());
		info.has_own_timing = true;
	}

	// If the header did not contain a BPM default to the first step's #BPMS
	if ((info.song->m_SongTiming.GetSegmentIndexAtRow(SEGMENT_BPM, 0)) == -1) {
		info.loader->ProcessBPMs(info.song->m_SongTiming,
								 (*info.params)[1],
								 info.loader->GetSongTitle());
	}
	info.ssc_format = true;
}
void
SetStepsStops(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessStops(
		  *info.timing, (*info.params)[1], info.loader->GetSongTitle());
		info.has_own_timing = true;
	}
	info.ssc_format = true;
}
void
SetStepsDelays(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessDelays(*info.timing, (*info.params)[1]);
		info.has_own_timing = true;
	}
	info.ssc_format = true;
}
void
SetStepsTimeSignatures(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessTimeSignatures(*info.timing, (*info.params)[1]);
		info.has_own_timing = true;
	}
	info.ssc_format = true;
}
void
SetStepsTickCounts(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessTickcounts(*info.timing, (*info.params)[1]);
		info.has_own_timing = true;
	}
	info.ssc_format = true;
}
void
SetStepsCombos(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessCombos(*info.timing, (*info.params)[1]);
		info.has_own_timing = true;
	}
	info.ssc_format = true;
}
void
SetStepsWarps(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessWarps(*info.timing,
								  (*info.params)[1],
								  info.song->m_fVersion,
								  info.loader->GetSongTitle());
		info.has_own_timing = true;
	}
	info.ssc_format = true;
}
void
SetStepsSpeeds(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessSpeeds(*info.timing, (*info.params)[1]);
		info.has_own_timing = true;
	}
	info.ssc_format = true;
}
void
SetStepsScrolls(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessScrolls(
		  *info.timing, (*info.params)[1], info.loader->GetSongTitle());
		info.has_own_timing = true;
	}
	info.ssc_format = true;
}
void
SetStepsFakes(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessFakes(*info.timing, (*info.params)[1]);
		info.has_own_timing = true;
	}
	info.ssc_format = true;
}
void
SetStepsLabels(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.loader->ProcessLabels(
		  *info.timing, (*info.params)[1], info.loader->GetSongTitle());
		info.has_own_timing = true;
	}
	info.ssc_format = true;
}
void
SetStepsOffset(SSC::StepsTagInfo& info)
{
	if (info.song->m_fVersion >= VERSION_SPLIT_TIMING || info.for_load_edit) {
		info.timing->m_fBeat0OffsetInSeconds = StringToFloat((*info.params)[1]);
		info.has_own_timing = true;
	}
}
void
SetStepsDisplayBPM(SSC::StepsTagInfo& info)
{
	// #DISPLAYBPM:[xxx][xxx:xxx]|[*];
	if ((*info.params)[1] == "*") {
		info.steps->SetDisplayBPM(DISPLAY_BPM_RANDOM);
	} else {
		info.steps->SetDisplayBPM(DISPLAY_BPM_SPECIFIED);
		float min = StringToFloat((*info.params)[1]);
		info.steps->SetMinBPM(min);
		if ((*info.params)[2].empty()) {
			info.steps->SetMaxBPM(min);
		} else {
			info.steps->SetMaxBPM(StringToFloat((*info.params)[2]));
		}
	}
}

void
SetChartKey(SSC::StepsTagInfo& info)
{
	info.steps->SetChartKey((*info.params)[1]);
}

vector<float>
SSC::msdsplit(const std::string& s)
{
	vector<float> o;
	for (size_t i = 0; i < s.size(); i += 6)
		o.emplace_back(StringToFloat(s.substr(i, 5)));
	return o;
}

void
SetMSDValues(SSC::StepsTagInfo& info)
{
	std::vector<std::vector<float>> o;

	// Optimize by calling those only once instead of multiple times inside the
	// loop.
	auto params = (*info.params);
	auto size = params.params.size();
	// Start from index 1
	for (size_t i = 1; i <= size; i++) {
		auto m = SSC::msdsplit(params[i]);
		o.push_back(
		  std::vector<float>{ 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f });
	}
	info.steps->SetAllMSD(o);
}

typedef std::map<std::string, steps_tag_func_t> steps_handler_map_t;
typedef std::map<std::string, song_tag_func_t> song_handler_map_t;
typedef std::map<std::string, SSC::LoadNoteDataTagIDs>
  load_note_data_handler_map_t;

struct ssc_parser_helper_t
{
	steps_handler_map_t steps_tag_handlers;
	song_handler_map_t song_tag_handlers;
	load_note_data_handler_map_t load_note_data_handlers;
	// Unless signed, the comments in this tag list are not by me.  They were
	// moved here when converting from the else if chain. -Kyz
	ssc_parser_helper_t()
	{
		song_tag_handlers["VERSION"] = &SetVersion;
		song_tag_handlers["TITLE"] = &SetTitle;
		song_tag_handlers["SUBTITLE"] = &SetSubtitle;
		song_tag_handlers["ARTIST"] = &SetArtist;
		song_tag_handlers["TITLETRANSLIT"] = &SetMainTitleTranslit;
		song_tag_handlers["SUBTITLETRANSLIT"] = &SetSubtitleTranslit;
		song_tag_handlers["ARTISTTRANSLIT"] = &SetArtistTranslit;
		song_tag_handlers["GENRE"] = &SetGenre;
		song_tag_handlers["ORIGIN"] = &SetOrigin;
		song_tag_handlers["CREDIT"] = &SetCredit;
		song_tag_handlers["BANNER"] = &SetBanner;
		song_tag_handlers["BACKGROUND"] = &SetBackground;
		song_tag_handlers["PREVIEWVID"] = &SetPreviewVid;
		song_tag_handlers["JACKET"] = &SetJacket;
		song_tag_handlers["CDIMAGE"] = &SetCDImage;
		song_tag_handlers["DISCIMAGE"] = &SetDiscImage;
		song_tag_handlers["LYRICSPATH"] = &SetLyricsPath;
		song_tag_handlers["CDTITLE"] = &SetCDTitle;
		song_tag_handlers["MUSIC"] = &SetMusic;
		song_tag_handlers["PREVIEW"] = &SetPreview;
		song_tag_handlers["INSTRUMENTTRACK"] = &SetInstrumentTrack;
		song_tag_handlers["MUSICLENGTH"] = &SetMusicLength;
		song_tag_handlers["LASTSECONDHINT"] = &SetLastSecondHint;
		song_tag_handlers["SAMPLESTART"] = &SetSampleStart;
		song_tag_handlers["SAMPLELENGTH"] = &SetSampleLength;
		song_tag_handlers["DISPLAYBPM"] = &SetDisplayBPM;
		song_tag_handlers["SELECTABLE"] = &SetSelectable;
		// It's a bit odd to have the tag that exists for backwards
		// compatibility in this list and not the replacement, but the BGCHANGES
		// tag has a number on the end, allowing up to NUM_BackgroundLayer tags,
		// so it can't fit in the map. -Kyz
		song_tag_handlers["ANIMATIONS"] = &SetBGChanges;
		song_tag_handlers["FGCHANGES"] = &SetFGChanges;
		song_tag_handlers["KEYSOUNDS"] = &SetKeysounds;
		song_tag_handlers["OFFSET"] = &SetOffset;
		/* Below are the song based timings that should only be used
		 * if the steps do not have their own timing. */
		song_tag_handlers["STOPS"] = &SetSongStops;
		song_tag_handlers["DELAYS"] = &SetSongDelays;
		song_tag_handlers["BPMS"] = &SetSongBPMs;
		song_tag_handlers["WARPS"] = &SetSongWarps;
		song_tag_handlers["LABELS"] = &SetSongLabels;
		song_tag_handlers["TIMESIGNATURES"] = &SetSongTimeSignatures;
		song_tag_handlers["TICKCOUNTS"] = &SetSongTickCounts;
		song_tag_handlers["COMBOS"] = &SetSongCombos;
		song_tag_handlers["SPEEDS"] = &SetSongSpeeds;
		song_tag_handlers["SCROLLS"] = &SetSongScrolls;
		song_tag_handlers["FAKES"] = &SetSongFakes;
		/* The following are cache tags. Never fill their values
		 * directly: only from the cached version. */
		song_tag_handlers["FIRSTSECOND"] = &SetFirstSecond;
		song_tag_handlers["LASTSECOND"] = &SetLastSecond;
		song_tag_handlers["SONGFILENAME"] = &SetSongFilename;
		song_tag_handlers["HASMUSIC"] = &SetHasMusic;
		song_tag_handlers["HASBANNER"] = &SetHasBanner;
		/* Tags that no longer exist, listed for posterity.  May their names
		 * never be forgotten for their service to Stepmania. -Kyz
		 * LASTBEATHINT: // unable to parse due to tag position. Ignore.
		 * MUSICBYTES: // ignore
		 * FIRSTBEAT: // no longer used.
		 * LASTBEAT: // no longer used.
		 */

		steps_tag_handlers["VERSION"] = &SetStepsVersion;
		steps_tag_handlers["CHARTNAME"] = &SetChartName;
		steps_tag_handlers["STEPSTYPE"] = &SetStepsType;
		steps_tag_handlers["CHARTSTYLE"] = &SetChartStyle;
		steps_tag_handlers["DESCRIPTION"] = &SetDescription;
		steps_tag_handlers["DIFFICULTY"] = &SetDifficulty;
		steps_tag_handlers["METER"] = &SetMeter;
		steps_tag_handlers["RADARVALUES"] = &SetRadarValues;
		steps_tag_handlers["CREDIT"] = &SetCredit;
		steps_tag_handlers["MUSIC"] = &SetStepsMusic;
		steps_tag_handlers["BPMS"] = &SetStepsBPMs;
		steps_tag_handlers["STOPS"] = &SetStepsStops;
		steps_tag_handlers["DELAYS"] = &SetStepsDelays;
		steps_tag_handlers["TIMESIGNATURES"] = &SetStepsTimeSignatures;
		steps_tag_handlers["TICKCOUNTS"] = &SetStepsTickCounts;
		steps_tag_handlers["COMBOS"] = &SetStepsCombos;
		steps_tag_handlers["WARPS"] = &SetStepsWarps;
		steps_tag_handlers["SPEEDS"] = &SetStepsSpeeds;
		steps_tag_handlers["SCROLLS"] = &SetStepsScrolls;
		steps_tag_handlers["FAKES"] = &SetStepsFakes;
		steps_tag_handlers["LABELS"] = &SetStepsLabels;
		/* If this is called, the chart does not use the same attacks
		 * as the Song's timing. No other changes are required. */
		steps_tag_handlers["OFFSET"] = &SetStepsOffset;
		steps_tag_handlers["DISPLAYBPM"] = &SetStepsDisplayBPM;
		steps_tag_handlers["CHARTKEY"] = &SetChartKey;
		steps_tag_handlers["MSDVALUES"] = &SetMSDValues;

		load_note_data_handlers["VERSION"] = SSC::LNDID_version;
		load_note_data_handlers["STEPSTYPE"] = SSC::LNDID_stepstype;
		load_note_data_handlers["CHARTNAME"] = SSC::LNDID_chartname;
		load_note_data_handlers["DESCRIPTION"] = SSC::LNDID_description;
		load_note_data_handlers["DIFFICULTY"] = SSC::LNDID_difficulty;
		load_note_data_handlers["METER"] = SSC::LNDID_meter;
		load_note_data_handlers["CREDIT"] = SSC::LNDID_credit;
		load_note_data_handlers["NOTES"] = SSC::LNDID_notes;
		load_note_data_handlers["NOTES2"] = SSC::LNDID_notes2;
		load_note_data_handlers["NOTEDATA"] = SSC::LNDID_notedata;
	}
};
ssc_parser_helper_t parser_helper;
// End parser_helper related functions. -Kyz
/****************************************************************/

void
SSCLoader::ProcessBPMs(TimingData& out,
					   const std::string& sParam,
					   const std::string& songName)
{
	vector<std::string> arrayBPMExpressions;
	split(sParam, ",", arrayBPMExpressions);

	for (auto& arrayBPMExpression : arrayBPMExpressions) {
		vector<std::string> arrayBPMValues;
		split(arrayBPMExpression, "=", arrayBPMValues);
		if (arrayBPMValues.size() != 2) {
//			LOG->UserLog("Song file",
//						 songName,
//						 "has an invalid #BPMS value \"%s\" (must have exactly "
//						 "one '='), ignored.",
//						 arrayBPMExpression.c_str());
			continue;
		}

		const float fBeat = StringToFloat(arrayBPMValues[0]);
		const float fNewBPM = StringToFloat(arrayBPMValues[1]);
		if (fBeat >= 0 && fNewBPM > 0) {
			out.AddSegment(BPMSegment(BeatToNoteRow(fBeat), fNewBPM));
		} else {
			/*LOG->UserLog("Song file",
						 songName,
						 "has an invalid BPM at beat %f, BPM %f.",
						 fBeat,
						 fNewBPM);*/
		}
	}
}

void
SSCLoader::ProcessStops(TimingData& out,
						const std::string& sParam,
						const std::string& songName)
{
	vector<std::string> arrayStopExpressions;
	split(sParam, ",", arrayStopExpressions);

	for (auto& arrayStopExpression : arrayStopExpressions) {
		vector<std::string> arrayStopValues;
		split(arrayStopExpression, "=", arrayStopValues);
		if (arrayStopValues.size() != 2) {
//			LOG->UserLog("Song file",
//						 songName,
//						 "has an invalid #STOPS value \"%s\" (must have "
//						 "exactly one '='), ignored.",
//						 arrayStopExpression.c_str());
			continue;
		}

		const float fBeat = StringToFloat(arrayStopValues[0]);
		const float fNewStop = StringToFloat(arrayStopValues[1]);
		if (fBeat >= 0 && fNewStop > 0)
			out.AddSegment(StopSegment(BeatToNoteRow(fBeat), fNewStop));
		else {
//			LOG->UserLog("Song file",
//						 songName,
//						 "has an invalid Stop at beat %f, length %f.",
//						 fBeat,
//						 fNewStop);
		}
	}
}

void
SSCLoader::ProcessWarps(TimingData& out,
						const std::string& sParam,
						const float fVersion,
						const std::string& songName)
{
	vector<std::string> arrayWarpExpressions;
	split(sParam, ",", arrayWarpExpressions);

	for (auto& arrayWarpExpression : arrayWarpExpressions) {
		vector<std::string> arrayWarpValues;
		split(arrayWarpExpression, "=", arrayWarpValues);
		if (arrayWarpValues.size() != 2) {
//			LOG->UserLog("Song file",
//						 songName,
//						 "has an invalid #WARPS value \"%s\" (must have "
//						 "exactly one '='), ignored.",
//						 arrayWarpExpression.c_str());
			continue;
		}

		const float fBeat = StringToFloat(arrayWarpValues[0]);
		const float fNewBeat = StringToFloat(arrayWarpValues[1]);
		// Early versions were absolute in beats. They should be relative.
		if ((fVersion < VERSION_SPLIT_TIMING && fNewBeat > fBeat)) {
			out.AddSegment(WarpSegment(BeatToNoteRow(fBeat), fNewBeat - fBeat));
		} else if (fNewBeat > 0)
			out.AddSegment(WarpSegment(BeatToNoteRow(fBeat), fNewBeat));
		else {
//			LOG->UserLog("Song file",
//						 songName,
//						 "has an invalid Warp at beat %f, BPM %f.",
//						 fBeat,
//						 fNewBeat);
		}
	}
}

void
SSCLoader::ProcessLabels(TimingData& out,
						 const std::string& sParam,
						 const std::string& songName)
{
	vector<std::string> arrayLabelExpressions;
	split(sParam, ",", arrayLabelExpressions);

	for (auto& arrayLabelExpression : arrayLabelExpressions) {
		vector<std::string> arrayLabelValues;
		split(arrayLabelExpression, "=", arrayLabelValues);
		if (arrayLabelValues.size() != 2) {
//			LOG->UserLog("Song file",
//						 songName,
//						 "has an invalid #LABELS value \"%s\" (must have "
//						 "exactly one '='), ignored.",
//						 arrayLabelExpression.c_str());
			continue;
		}

		const float fBeat = StringToFloat(arrayLabelValues[0]);
		std::string sLabel = arrayLabelValues[1];
		TrimRight(sLabel);
		if (fBeat >= 0.0f)
			out.AddSegment(LabelSegment(BeatToNoteRow(fBeat), sLabel));
		else {
//			LOG->UserLog("Song file",
//						 songName,
//						 "has an invalid Label at beat %f called %s.",
//						 fBeat,
//						 sLabel.c_str());
		}
	}
}
void
SSCLoader::ProcessCombos(TimingData& out,
						 const std::string& line,
						 const int rowsPerBeat)
{
	auto name = this->GetSongTitle();
	ProcessCombos(out, line, name, rowsPerBeat);
}
void
SSCLoader::ProcessCombos(TimingData& out,
						 const std::string& line,
						 const std::string& songName,
						 const int rowsPerBeat)
{
	vector<std::string> arrayComboExpressions;
	split(line, ",", arrayComboExpressions);

	for (auto& arrayComboExpression : arrayComboExpressions) {
		vector<std::string> arrayComboValues;
		split(arrayComboExpression, "=", arrayComboValues);
		unsigned size = arrayComboValues.size();
		if (size < 2) {
//			LOG->UserLog("Song file",
//						 songName,
//						 "has an invalid #COMBOS value \"%s\" (must have at "
//						 "least one '='), ignored.",
//						 arrayComboExpression.c_str());
			continue;
		}
		const float fComboBeat = StringToFloat(arrayComboValues[0]);
		const int iCombos = StringToInt(arrayComboValues[1]);
		const int iMisses =
		  (size == 2 ? iCombos : StringToInt(arrayComboValues[2]));
		out.AddSegment(
		  ComboSegment(BeatToNoteRow(fComboBeat), iCombos, iMisses));
	}
}

void
SSCLoader::ProcessScrolls(TimingData& out,
						  const std::string sParam,
						  const std::string& songName)
{
	vector<std::string> vs1;
	split(sParam, ",", vs1);

	for (auto& s1 : vs1) {
		vector<std::string> vs2;
		split(s1, "=", vs2);

		if (vs2.size() < 2) {
//			LOG->UserLog("Song file",
//						 songName,
//						 "has an scroll change with %i values.",
//						 static_cast<int>(vs2.size()));
			continue;
		}

		const float fBeat = StringToFloat(vs2[0]);
		const float fRatio = StringToFloat(vs2[1]);

		if (fBeat < 0) {
//			LOG->UserLog("Song file",
//						 songName,
//						 "has an scroll change with beat %f.",
//						 fBeat);
			continue;
		}

		out.AddSegment(ScrollSegment(BeatToNoteRow(fBeat), fRatio));
	}
}

bool
SSCLoader::LoadNoteDataFromSimfile(const std::string& cachePath, Steps& out)
{
//	LOG->Trace("Loading notes from %s", cachePath.c_str());

	MsdFile msd;
	if (!msd.ReadFile(cachePath, true)) {
//		LOG->UserLog("Unable to load any notes from",
//					 cachePath,
//					 "for this reason: %s",
//					 msd.GetError().c_str());
		return false;
	}

	bool tryingSteps = false;
	float storedVersion = 0;
	const unsigned values = msd.GetNumValues();

	for (unsigned i = 0; i < values; i++) {
		const MsdFile::value_t& params = msd.GetValue(i);
		std::string valueName = make_upper(params[0]);
		std::string matcher = params[1]; // mainly for debugging.
		Trim(matcher);

		load_note_data_handler_map_t::iterator handler =
		  parser_helper.load_note_data_handlers.find(valueName);
		if (handler != parser_helper.load_note_data_handlers.end()) {
			if (tryingSteps) {
				switch (handler->second) {
					case SSC::LNDID_version:
						// Note that version is in both switches.  Formerly, it
						// was checked before the tryingSteps condition. -Kyz
						storedVersion = StringToFloat(matcher);
						break;
					case SSC::LNDID_stepstype:
						if (out.m_StepsType !=
							GAMEMAN->StringToStepsType(matcher)) {
							tryingSteps = false;
						}
						break;
					case SSC::LNDID_chartname:
						if (storedVersion >= VERSION_CHART_NAME_TAG &&
							out.GetChartName() != matcher) {
							tryingSteps = false;
						}
						break;
					case SSC::LNDID_description:
						if (storedVersion < VERSION_CHART_NAME_TAG) {
							if (out.GetChartName() != matcher) {
								tryingSteps = false;
							}
						} else if (out.GetDescription() != matcher) {
							tryingSteps = false;
						}
						break;
					case SSC::LNDID_difficulty:
						// Accept any difficulty if it's an edit because
						// LoadEditFromMsd forces edits onto Edit difficulty
						// even if they have a difficulty tag. -Kyz
						if (out.GetDifficulty() !=
							  StringToDifficulty(matcher) &&
							!(out.GetDifficulty() == Difficulty_Edit &&
							  make_lower(GetExtension(cachePath)) == "edit")) {
							tryingSteps = false;
						}
						break;
					case SSC::LNDID_meter:
						if (out.GetMeter() != StringToInt(matcher)) {
							tryingSteps = false;
						}
						break;
					case SSC::LNDID_credit:
						if (out.GetCredit() != matcher) {
							tryingSteps = false;
						}
						break;
					case SSC::LNDID_notes:
					case SSC::LNDID_notes2:
						out.SetSMNoteData(matcher);
						out.TidyUpData();
						return true;
					default:
						break;
				}
			} else {
				switch (handler->second) {
					case SSC::LNDID_version:
						// Note that version is in both switches.  Formerly, it
						// was checked before the tryingSteps condition. -Kyz
						storedVersion = StringToFloat(matcher);
						break;
					case SSC::LNDID_notedata:
						tryingSteps = true;
						break;
					default:
						break;
				}
			}
		} else {
			// Silently ignore unrecognized tags, as was done before. -Kyz
		}
	}
	return false;
}

bool
SSCLoader::LoadFromSimfile(const std::string& sPath, Song& out, bool bFromCache)
{
	// LOG->Trace( "Song::LoadFromSSCFile(%s)", sPath.c_str() );

	MsdFile msd;
	if (!msd.ReadFile(sPath, true)) {
//		LOG->UserLog(
//		  "Song file", sPath, "couldn't be opened: %s", msd.GetError().c_str());
		return false;
	}

	out.m_SongTiming.m_sFile = sPath; // songs still have their fallback timing.
	out.m_sSongFileName = sPath;

	int state = GETTING_SONG_INFO;
	const unsigned values = msd.GetNumValues();
	Steps* pNewNotes = nullptr;
	TimingData stepsTiming;

	SSC::SongTagInfo reused_song_info(&*this, &out, sPath, bFromCache);
	SSC::StepsTagInfo reused_steps_info(&*this, &out, sPath, bFromCache);

	for (unsigned i = 0; i < values; i++) {
		const MsdFile::value_t& sParams = msd.GetValue(i);
		std::string sValueName = make_upper(sParams[0]);

		switch (state) {
			case GETTING_SONG_INFO: {
				reused_song_info.params = &sParams;
				song_handler_map_t::iterator handler =
				  parser_helper.song_tag_handlers.find(sValueName);
				if (handler != parser_helper.song_tag_handlers.end()) {
					handler->second(reused_song_info);
				} else if (head(sValueName, 9) == "BGCHANGES") {
					SetBGChanges(reused_song_info);
				}
				// This tag will get us to the next section.
				else if (sValueName == "NOTEDATA") {
					state = GETTING_STEP_INFO;
					pNewNotes = out.CreateSteps();
					stepsTiming =
					  TimingData(out.m_SongTiming.m_fBeat0OffsetInSeconds);
					reused_steps_info.has_own_timing = false;
					reused_steps_info.steps = pNewNotes;
					reused_steps_info.timing = &stepsTiming;
				} else {
					// Silently ignore unrecognized tags, as was done before.
					// -Kyz
				}
				break;
			}
			case GETTING_STEP_INFO: {
				reused_steps_info.params = &sParams;
				steps_handler_map_t::iterator handler =
				  parser_helper.steps_tag_handlers.find(sValueName);
				if (handler != parser_helper.steps_tag_handlers.end()) {
					handler->second(reused_steps_info);
				} else if (sValueName == "NOTES" || sValueName == "NOTES2") {
					state = GETTING_SONG_INFO;
					if (reused_steps_info.has_own_timing) {
						pNewNotes->m_Timing = stepsTiming;
					}
					reused_steps_info.has_own_timing = false;
					pNewNotes->SetSMNoteData(sParams[1]);
					pNewNotes->TidyUpData();
					pNewNotes->SetFilename(sPath);
					out.AddSteps(pNewNotes);
				} else if (sValueName == "STEPFILENAME") {
					state = GETTING_SONG_INFO;
					if (reused_steps_info.has_own_timing) {
						pNewNotes->m_Timing = stepsTiming;
					}
					reused_steps_info.has_own_timing = false;
					pNewNotes->SetFilename(sParams[1]);
					out.AddSteps(pNewNotes);
				} else {
					// Silently ignore unrecognized tags, as was done before.
					// -Kyz
				}
				break;
			}
		}
	}
	out.m_fVersion = STEPFILE_VERSION_NUMBER;
	SMLoader::TidyUpData(out, bFromCache);
	return true;
}
