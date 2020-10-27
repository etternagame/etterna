#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Misc/NoteTypes.h"
#include "NotesWriterSM.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/File/RageFile.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

#include <map>

using std::map;

ThemeMetric<bool> USE_CREDIT("NotesWriterSM", "DescriptionUsesCreditField");

/**
 * @brief Write out the common tags for .SM files.
 * @param f the file in question.
 * @param out the Song in question. */
static void
WriteGlobalTags(RageFile& f, Song& out)
{
	TimingData& timing = out.m_SongTiming;
	f.PutLine(ssprintf("#TITLE:%s;", SmEscape(out.m_sMainTitle).c_str()));
	f.PutLine(ssprintf("#SUBTITLE:%s;", SmEscape(out.m_sSubTitle).c_str()));
	f.PutLine(ssprintf("#ARTIST:%s;", SmEscape(out.m_sArtist).c_str()));
	f.PutLine(ssprintf("#TITLETRANSLIT:%s;",
					   SmEscape(out.m_sMainTitleTranslit).c_str()));
	f.PutLine(ssprintf("#SUBTITLETRANSLIT:%s;",
					   SmEscape(out.m_sSubTitleTranslit).c_str()));
	f.PutLine(
	  ssprintf("#ARTISTTRANSLIT:%s;", SmEscape(out.m_sArtistTranslit).c_str()));
	f.PutLine(ssprintf("#GENRE:%s;", SmEscape(out.m_sGenre).c_str()));
	f.PutLine(ssprintf("#CREDIT:%s;", SmEscape(out.m_sCredit).c_str()));
	f.PutLine(ssprintf("#BANNER:%s;", SmEscape(out.m_sBannerFile).c_str()));
	f.PutLine(
	  ssprintf("#BACKGROUND:%s;", SmEscape(out.m_sBackgroundFile).c_str()));
	f.PutLine(ssprintf("#LYRICSPATH:%s;", SmEscape(out.m_sLyricsFile).c_str()));
	f.PutLine(ssprintf("#CDTITLE:%s;", SmEscape(out.m_sCDTitleFile).c_str()));
	f.PutLine(ssprintf("#MUSIC:%s;", SmEscape(out.m_sMusicFile).c_str()));
	f.PutLine(
	  ssprintf("#OFFSET:%.6f;", out.m_SongTiming.m_fBeat0OffsetInSeconds));
	f.PutLine(ssprintf("#SAMPLESTART:%.6f;", out.m_fMusicSampleStartSeconds));
	f.PutLine(ssprintf("#SAMPLELENGTH:%.6f;", out.m_fMusicSampleLengthSeconds));
	float specBeat = out.GetSpecifiedLastBeat();
	if (specBeat > 0)
		f.PutLine(ssprintf("#LASTBEATHINT:%.6f;", specBeat));

	f.Write("#SELECTABLE:");
	switch (out.m_SelectionDisplay) {
		default:
			FAIL_M(ssprintf("Invalid selection display: %i",
							out.m_SelectionDisplay));
		case Song::SHOW_ALWAYS:
			f.Write("YES");
			break;
		case Song::SHOW_NEVER:
			f.Write("NO");
			break;
	}
	f.PutLine(";");

	switch (out.m_DisplayBPMType) {
		case DISPLAY_BPM_ACTUAL:
			// write nothing
			break;
		case DISPLAY_BPM_SPECIFIED:
			if (out.m_fSpecifiedBPMMin == out.m_fSpecifiedBPMMax)
				f.PutLine(
				  ssprintf("#DISPLAYBPM:%.6f;", out.m_fSpecifiedBPMMin));
			else
				f.PutLine(ssprintf("#DISPLAYBPM:%.6f:%.6f;",
								   out.m_fSpecifiedBPMMin,
								   out.m_fSpecifiedBPMMax));
			break;
		case DISPLAY_BPM_RANDOM:
			f.PutLine(ssprintf("#DISPLAYBPM:*;"));
			break;
		default:
			break;
	}

	f.Write("#BPMS:");
	const vector<TimingSegment*>& bpms = timing.GetTimingSegments(SEGMENT_BPM);
	for (unsigned i = 0; i < bpms.size(); i++) {
		const BPMSegment* bs = ToBPM(bpms[i]);

		f.PutLine(ssprintf("%.6f=%.6f", bs->GetBeat(), bs->GetBPM()));
		if (i != bpms.size() - 1)
			f.Write(",");
	}
	f.PutLine(";");

	const vector<TimingSegment*>& stops =
	  timing.GetTimingSegments(SEGMENT_STOP);
	const vector<TimingSegment*>& delays =
	  timing.GetTimingSegments(SEGMENT_DELAY);

	map<float, float> allPauses;
	const vector<TimingSegment*>& warps =
	  timing.GetTimingSegments(SEGMENT_WARP);
	unsigned wSize = warps.size();
	if (wSize > 0) {
		for (unsigned i = 0; i < wSize; i++) {
			const WarpSegment* ws = static_cast<WarpSegment*>(warps[i]);
			int iRow = ws->GetRow();
			float fBPS = 60 / out.m_SongTiming.GetBPMAtRow(iRow);
			float fSkip = fBPS * ws->GetLength();
			allPauses.insert(std::pair<float, float>(ws->GetBeat(), -fSkip));
		}
	}

	for (auto stop : stops) {
		const StopSegment* fs = ToStop(stop);
		// Handle warps on the same row by summing the values.  Not sure this
		// plays out the same. -Kyz
		map<float, float>::iterator already_exists =
		  allPauses.find(fs->GetBeat());
		if (already_exists != allPauses.end()) {
			already_exists->second += fs->GetPause();
		} else {
			allPauses.insert(
			  std::pair<float, float>(fs->GetBeat(), fs->GetPause()));
		}
	}
	// Delays can't be negative: thus, no effect.
	FOREACH_CONST(TimingSegment*, delays, ss)
	{
		float fBeat = NoteRowToBeat((*ss)->GetRow() - 1);
		float fPause = ToDelay(*ss)->GetPause();
		map<float, float>::iterator already_exists = allPauses.find(fBeat);
		if (already_exists != allPauses.end()) {
			already_exists->second += fPause;
		} else {
			allPauses.insert(std::pair<float, float>(fBeat, fPause));
		}
	}

	f.Write("#STOPS:");
	vector<std::string> stopLines;
	FOREACHM(float, float, allPauses, ap)
	{
		stopLines.push_back(ssprintf("%.6f=%.6f", ap->first, ap->second));
	}
	f.PutLine(join(",\n", stopLines));

	f.PutLine(";");

	FOREACH_BackgroundLayer(b)
	{
		if (b == 0)
			f.Write("#BGCHANGES:");
		else if (out.GetBackgroundChanges(b).empty())
			continue; // skip
		else
			f.Write(ssprintf("#BGCHANGES%d:", b + 1));

		FOREACH_CONST(BackgroundChange, out.GetBackgroundChanges(b), bgc)
		f.PutLine((*bgc).ToString() + ",");

		/* If there's an animation plan at all, add a dummy "-nosongbg-" tag to
		 * indicate that this file doesn't want a song BG entry added at the
		 * end.  See SMLoader::TidyUpData. This tag will be removed on load.
		 * Add it at a very high beat, so it won't cause problems if loaded in
		 * older versions. */
		if (b == 0 && !out.GetBackgroundChanges(b).empty())
			f.PutLine("99999=-nosongbg-=1.000=0=0=0 // don't automatically add "
					  "-songbackground-");
		f.PutLine(";");
	}

	if (!out.GetForegroundChanges().empty()) {
		f.Write("#FGCHANGES:");
		FOREACH_CONST(BackgroundChange, out.GetForegroundChanges(), bgc)
		{
			f.PutLine((*bgc).ToString() + ",");
		}
		f.PutLine(";");
	}

	f.Write("#KEYSOUNDS:");
	for (unsigned i = 0; i < out.m_vsKeysoundFile.size(); i++) {
		f.Write(out.m_vsKeysoundFile[i]);
		if (i != out.m_vsKeysoundFile.size() - 1)
			f.Write(",");
	}
	f.PutLine(";");
}

/**
 * @brief Turn a vector of lines into a single line joined by newline
 * characters.
 * @param lines the list of lines to join.
 * @return the joined lines. */
static std::string
JoinLineList(vector<std::string>& lines)
{
	for (auto& line : lines)
		TrimRight(line);

	/* Skip leading blanks. */
	unsigned j = 0;
	while (j < lines.size() && lines.empty())
		++j;

	return join("\r\n", lines.begin() + j, lines.end());
}

/**
 * @brief Retrieve the notes from the #NOTES tag.
 * @param song the Song in question.
 * @param in the Steps in question.
 * @return the #NOTES tag. */
static std::string
GetSMNotesTag(const Song& song, const Steps& in)
{
	vector<std::string> lines;

	lines.push_back("");
	// Escape to prevent some clown from making a comment of "\r\n;"
	lines.push_back(ssprintf("//---------------%s - %s----------------",
							 in.m_StepsTypeStr.c_str(),
							 SmEscape(in.GetDescription()).c_str()));
	lines.push_back(song.m_vsKeysoundFile.empty() ? "#NOTES:" : "#NOTES2:");
	lines.push_back(ssprintf("     %s:", in.m_StepsTypeStr.c_str()));
	std::string desc = (USE_CREDIT ? in.GetCredit() : in.GetChartName());
	lines.push_back(ssprintf("     %s:", SmEscape(desc).c_str()));
	lines.push_back(
	  ssprintf("     %s:", DifficultyToString(in.GetDifficulty()).c_str()));
	lines.push_back(ssprintf("     %d:", in.GetMeter()));

	vector<std::string> asRadarValues;
	int categories = 11;
	const RadarValues& rv = in.GetRadarValues();
	for (RadarCategory rc = (RadarCategory)0; rc < categories;
		 enum_add<RadarCategory>(rc, 1)) {
		asRadarValues.push_back(IntToString(rv[rc]));
	}
	lines.push_back(ssprintf("     %s:", join(",", asRadarValues).c_str()));

	std::string sNoteData;
	in.GetSMNoteData(sNoteData);

	split(sNoteData, "\n", lines, true);
	lines.push_back(";");

	return JoinLineList(lines);
}

bool
NotesWriterSM::Write(const std::string& sPath,
					 Song& out,
					 const vector<Steps*>& vpStepsToSave)
{
	int flags = RageFile::WRITE;

	flags |= RageFile::SLOW_FLUSH;

	RageFile f;
	if (!f.Open(sPath, flags)) {

        Locator::getLogger()->info("Song file \"{}\" couldn't be opened for writing: {}", sPath, f.GetError().c_str());
		return false;
	}

	WriteGlobalTags(f, out);

	FOREACH_CONST(Steps*, vpStepsToSave, s)
	{
		const Steps* pSteps = *s;
		std::string sTag = GetSMNotesTag(out, *pSteps);
		f.PutLine(sTag);
	}
	if (f.Flush() == -1)
		return false;

	return true;
}

void
NotesWriterSM::GetEditFileContents(const Song* pSong,
								   const Steps* pSteps,
								   std::string& sOut)
{
	sOut = "";
	std::string sDir = pSong->GetSongDir();

	// "Songs/foo/bar"; strip off "Songs/".
	vector<std::string> asParts;
	split(sDir, "/", asParts);
	if (!asParts.empty())
		sDir = join("/", asParts.begin() + 1, asParts.end());
	sOut += ssprintf("#SONG:%s;\r\n", sDir.c_str());
	sOut += GetSMNotesTag(*pSong, *pSteps);
}

std::string
NotesWriterSM::GetEditFileName(const Song* pSong, const Steps* pSteps)
{
	/* Try to make a unique name. This isn't guaranteed. Edit descriptions are
	 * case-sensitive, filenames on disk are usually not, and we decimate
	 * certain characters for FAT filesystems. */
	std::string sFile =
	  pSong->GetTranslitFullTitle() + " - " + pSteps->GetDescription();

	// HACK:
	if (pSteps->m_StepsType == StepsType_dance_double)
		sFile += " (doubles)";

	sFile += ".edit";

	MakeValidFilename(sFile);
	return sFile;
}

static LocalizedString DESTINATION_ALREADY_EXISTS(
  "NotesWriterSM",
  "Error renaming file.  Destination file '%s' already exists.");
static LocalizedString ERROR_WRITING_FILE("NotesWriterSM",
										  "Error writing file '%s'.");
