#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Misc/NoteTypes.h"
#include "NotesWriterETT.h"
#include "Etterna/Models/Misc/Profile.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/File/RageFile.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"

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

	// Skip leading blanks.
	unsigned j = 0;
	while (j < lines.size() && lines.empty())
		++j;

	return join("\r\n", lines.begin() + j, lines.end());
}

std::string
MSDToString2(std::vector<std::vector<float>> x)
{
	std::string o = "";
	for (size_t i = 0; i < x.size(); i++) {
		auto msds = x[i];
		for (size_t ii = 0; ii < msds.size(); ii++) {
			o.append(std::to_string(msds[ii]).substr(0, 5));
			if (ii != msds.size() - 1)
				o.append(",");
		}
		if (i != x.size() - 1)
			o.append(":");
	}
	return o;
}

// A utility class to write timing tags more easily!
struct TimingTagWriter
{

	vector<std::string>* m_pvsLines;
	std::string m_sNext;

	TimingTagWriter(vector<std::string>* pvsLines)
	  : m_pvsLines(pvsLines)
	{
	}

	void Write(const int row, const char* value)
	{
		m_pvsLines->emplace_back(
		  m_sNext + ssprintf("%.6f=%s", NoteRowToBeat(row), value));
		m_sNext = ",";
	}

	void Write(const int row, const float value)
	{
		Write(row, ssprintf("%.6f", value).c_str());
	}
	void Write(const int row, const int value)
	{
		Write(row, ssprintf("%d", value).c_str());
	}
	void Write(const int row, const int a, const int b)
	{
		Write(row, ssprintf("%d=%d", a, b).c_str());
	}
	void Write(const int row, const float a, const float b)
	{
		Write(row, ssprintf("%.6f=%.6f", a, b).c_str());
	}
	void Write(const int row,
			   const float a,
			   const float b,
			   const unsigned short c)
	{
		Write(row, ssprintf("%.6f=%.6f=%hd", a, b, c).c_str());
	}

	void Init(const std::string sTag) { m_sNext = "#" + sTag + ":"; }
	void Finish()
	{
		m_pvsLines->emplace_back((m_sNext != "," ? m_sNext : "") + ";");
	}
};

static void
GetTimingTags(vector<std::string>& lines,
			  const TimingData& timing,
			  bool bIsSong = false)
{
	TimingTagWriter writer(&lines);

	// timing.TidyUpData(); // UGLY: done via const_cast. do we really -need-
	// this here?
#define WRITE_SEG_LOOP_OPEN(enum_type, seg_type, seg_name, to_func)            \
	{                                                                          \
		vector<TimingSegment*> const& segs =                                   \
		  timing.GetTimingSegments(enum_type);                                 \
		if (!segs.empty()) {                                                   \
			writer.Init(seg_name);                                             \
			for (auto&& seg : segs) {                                          \
				const seg_type* segment = to_func(seg);

#define WRITE_SEG_LOOP_CLOSE                                                   \
	}                                                                          \
	writer.Finish();                                                           \
	}                                                                          \
	}

	WRITE_SEG_LOOP_OPEN(SEGMENT_BPM, BPMSegment, "BPMS", ToBPM);
	writer.Write(segment->GetRow(), segment->GetBPM());
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_STOP, StopSegment, "STOPS", ToStop);
	writer.Write(segment->GetRow(), segment->GetPause());
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_DELAY, DelaySegment, "DELAYS", ToDelay);
	writer.Write(segment->GetRow(), segment->GetPause());
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_WARP, WarpSegment, "WARPS", ToWarp);
	writer.Write(segment->GetRow(), segment->GetLength());
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_TIME_SIG,
						TimeSignatureSegment,
						"TIMESIGNATURESEGMENT",
						ToTimeSignature);
	writer.Write(segment->GetRow(), segment->GetNum(), segment->GetDen());
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(
	  SEGMENT_TICKCOUNT, TickcountSegment, "TICKCOUNTS", ToTickcount);
	writer.Write(segment->GetRow(), segment->GetTicks());
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_COMBO, ComboSegment, "COMBOS", ToCombo);
	if (segment->GetCombo() == segment->GetMissCombo()) {
		writer.Write(segment->GetRow(), segment->GetCombo());
	} else {
		writer.Write(
		  segment->GetRow(), segment->GetCombo(), segment->GetMissCombo());
	}
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_SPEED, SpeedSegment, "SPEEDS", ToSpeed);
	writer.Write(segment->GetRow(),
				 segment->GetRatio(),
				 segment->GetDelay(),
				 segment->GetUnit());
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_SCROLL, ScrollSegment, "SCROLLS", ToScroll);
	writer.Write(segment->GetRow(), segment->GetRatio());
	WRITE_SEG_LOOP_CLOSE;

	// TODO: Investigate why someone wrote a condition for leaving fakes out of
	// the song timing tags, and put it in a function that is only used when
	// writing the steps timing tags. -Kyz
	if (!bIsSong) {
		WRITE_SEG_LOOP_OPEN(SEGMENT_FAKE, FakeSegment, "FAKES", ToFake);
		writer.Write(segment->GetRow(), segment->GetLength());
		WRITE_SEG_LOOP_CLOSE;
	}

	WRITE_SEG_LOOP_OPEN(SEGMENT_LABEL, LabelSegment, "LABELS", ToLabel);
	if (!segment->GetLabel().empty()) {
		writer.Write(segment->GetRow(), segment->GetLabel().c_str());
	}
	WRITE_SEG_LOOP_CLOSE;

#undef WRITE_SEG_LOOP_OPEN
#undef WRITE_SEG_LOOP_CLOSE
}

static void
write_tag(RageFile& f, std::string const& format, std::string const& value)
{
	if (!value.empty()) {
		f.PutLine(ssprintf(format, SmEscape(value).c_str()));
	}
}

static void
WriteTimingTags(RageFile& f, const TimingData& timing, bool bIsSong = false)
{
	write_tag(
	  f, "#BPMS:%s;", join(",\r\n", timing.ToVectorString(SEGMENT_BPM, 3)));
	write_tag(
	  f, "#STOPS:%s;", join(",\r\n", timing.ToVectorString(SEGMENT_STOP, 3)));
	write_tag(
	  f, "#DELAYS:%s;", join(",\r\n", timing.ToVectorString(SEGMENT_DELAY, 3)));
	write_tag(
	  f, "#WARPS:%s;", join(",\r\n", timing.ToVectorString(SEGMENT_WARP, 3)));
	write_tag(f,
			  "#TIMESIGNATURES:%s;",
			  join(",\r\n", timing.ToVectorString(SEGMENT_TIME_SIG, 3)));
	write_tag(f,
			  "#TICKCOUNTS:%s;",
			  join(",\r\n", timing.ToVectorString(SEGMENT_TICKCOUNT, 3)));
	write_tag(
	  f, "#COMBOS:%s;", join(",\r\n", timing.ToVectorString(SEGMENT_COMBO, 3)));
	write_tag(
	  f, "#SPEEDS:%s;", join(",\r\n", timing.ToVectorString(SEGMENT_SPEED, 3)));
	write_tag(f,
			  "#SCROLLS:%s;",
			  join(",\r\n", timing.ToVectorString(SEGMENT_SCROLL, 3)));
	write_tag(
	  f, "#FAKES:%s;", join(",\r\n", timing.ToVectorString(SEGMENT_FAKE, 3)));
	write_tag(
	  f, "#LABELS:%s;", join(",\r\n", timing.ToVectorString(SEGMENT_LABEL, 3)));
}

/**
 * @brief Write out the common tags for .SSC files.
 * @param f the file in question.
 * @param out the Song in question. */
static void
WriteGlobalTags(RageFile& f, const Song& out)
{
	f.PutLine(ssprintf("#VERSION:%.2f;", STEPFILE_VERSION_NUMBER));
	write_tag(f, "#TITLE:%s;", out.m_sMainTitle);
	write_tag(f, "#SUBTITLE:%s;", out.m_sSubTitle);
	write_tag(f, "#ARTIST:%s;", out.m_sArtist);
	write_tag(f, "#TITLETRANSLIT:%s;", out.m_sMainTitleTranslit);
	write_tag(f, "#SUBTITLETRANSLIT:%s;", out.m_sSubTitleTranslit);
	write_tag(f, "#ARTISTTRANSLIT:%s;", out.m_sArtistTranslit);
	write_tag(f, "#GENRE:%s;", out.m_sGenre);
	write_tag(f, "#ORIGIN:%s;", out.m_sOrigin);
	write_tag(f, "#CREDIT:%s;", out.m_sCredit);
	write_tag(f, "#BANNER:%s;", out.m_sBannerFile);
	write_tag(f, "#BACKGROUND:%s;", out.m_sBackgroundFile);
	write_tag(f, "#PREVIEWVID:%s;", out.m_sPreviewVidFile);
	write_tag(f, "#JACKET:%s;", out.m_sJacketFile);
	write_tag(f, "#CDIMAGE:%s;", out.m_sCDFile);
	write_tag(f, "#DISCIMAGE:%s;", out.m_sDiscFile);
	write_tag(f, "#LYRICSPATH:%s;", out.m_sLyricsFile);
	write_tag(f, "#CDTITLE:%s;", out.m_sCDTitleFile);
	write_tag(f, "#MUSIC:%s;", out.m_sMusicFile);
	write_tag(f, "#PREVIEW:%s;", out.m_PreviewFile);
	{
		auto vs = out.GetInstrumentTracksToVectorString();
		if (!vs.empty()) {
			auto s = join(",", vs);
			f.PutLine("#INSTRUMENTTRACK:" + s + ";\n");
		}
	}
	f.PutLine(
	  ssprintf("#OFFSET:%.6f;", out.m_SongTiming.m_fBeat0OffsetInSeconds));
	f.PutLine(ssprintf("#SAMPLESTART:%.6f;", out.m_fMusicSampleStartSeconds));
	f.PutLine(ssprintf("#SAMPLELENGTH:%.6f;", out.m_fMusicSampleLengthSeconds));

	f.Write("#SELECTABLE:");
	switch (out.m_SelectionDisplay) {
		default:
			ASSERT_M(
			  0,
			  "An invalid selectable value was found for this song!"); // fall
																	   // through
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
			if (out.m_fSpecifiedBPMMin == out.m_fSpecifiedBPMMax) {
				f.PutLine(
				  ssprintf("#DISPLAYBPM:%.6f;", out.m_fSpecifiedBPMMin));
			} else {
				f.PutLine(ssprintf("#DISPLAYBPM:%.6f:%.6f;",
								   out.m_fSpecifiedBPMMin,
								   out.m_fSpecifiedBPMMax));
			}
			break;
		case DISPLAY_BPM_RANDOM:
			f.PutLine(ssprintf("#DISPLAYBPM:*;"));
			break;
		default:
			break;
	}

	WriteTimingTags(f, out.m_SongTiming);

	if (out.GetSpecifiedLastSecond() > 0) {
		f.PutLine(
		  ssprintf("#LASTSECONDHINT:%.6f;", out.GetSpecifiedLastSecond()));
	}
	FOREACH_BackgroundLayer(b)
	{
		if (out.GetBackgroundChanges(b).empty()) {
			continue; // skip
		}
		if (b == 0) {
			f.Write("#BGCHANGES:");
		} else {
			f.Write(ssprintf("#BGCHANGES%d:", b + 1));
		}
		for (auto& bgc : out.GetBackgroundChanges(b)) {
			f.PutLine(bgc.ToString() + ",");
		}

		/* If there's an animation plan at all, add a dummy "-nosongbg-" tag to
		 * indicate that this file doesn't want a song BG entry added at the
		 * end. See SSCLoader::TidyUpData. This tag will be removed on load. Add
		 * it at a very high beat, so it won't cause problems if loaded in older
		 * versions. */
		if (b == 0 && !out.GetBackgroundChanges(b).empty()) {
			f.PutLine("99999=-nosongbg-=1.000=0=0=0 // don't automatically add "
					  "-songbackground-");
		}
		f.PutLine(";");
	}

	if (!out.GetForegroundChanges().empty()) {
		f.Write("#FGCHANGES:");
		for (auto const& bgc : out.GetForegroundChanges()) {
			f.PutLine(bgc.ToString() + ",");
		}
		f.PutLine(";");
	}

	if (!out.m_vsKeysoundFile.empty()) {
		f.Write("#KEYSOUNDS:");
		for (unsigned i = 0; i < out.m_vsKeysoundFile.size(); i++) {
			// some keysound files has the first sound that starts with #,
			// which makes MsdFile fail parsing the whole declaration.
			// in this case, add a backslash at the front
			// (#KEYSOUNDS:\#bgm.wav,01.wav,02.wav,..) and handle that on load.
			if (i == 0 && !out.m_vsKeysoundFile[i].empty() &&
				out.m_vsKeysoundFile[i][0] == '#') {
				f.Write("\\");
			}
			f.Write(out.m_vsKeysoundFile[i]);
			if (i != out.m_vsKeysoundFile.size() - 1) {
				f.Write(",");
			}
		}
		f.PutLine(";");
	}

	f.PutLine("");
}

static void
emplace_back_tag(std::vector<std::string>& lines,
				 std::string const& format,
				 std::string const& value)
{
	if (!value.empty()) {
		lines.emplace_back(ssprintf(format, SmEscape(value).c_str()));
	}
}

/**
 * @brief Retrieve the individual batches of NoteData.
 * @param song the Song in question.
 * @param in the Steps in question.
 * @param bSavingCache a flag to see if we're saving certain cache data.
 * @return the NoteData in std::string form. */
static std::string
GetETTNoteData(const Song& song, Steps& in)
{
	std::vector<std::string> lines;

	lines.emplace_back("");
	// Escape to prevent some clown from making a comment of "\r\n;"
	lines.emplace_back(ssprintf("//---------------%s - %s----------------",
								in.m_StepsTypeStr.c_str(),
								SmEscape(in.GetDescription()).c_str()));
	lines.emplace_back("#NOTEDATA:;"); // our new separator.
	emplace_back_tag(lines, "#CHARTNAME:%s;", in.GetChartName());
	emplace_back_tag(lines, "#STEPSTYPE:%s;", in.m_StepsTypeStr);
	emplace_back_tag(lines, "#DESCRIPTION:%s;", in.GetDescription());
	emplace_back_tag(lines, "#CHARTSTYLE:%s;", in.GetChartStyle());
	emplace_back_tag(
	  lines, "#DIFFICULTY:%s;", DifficultyToString(in.GetDifficulty()));
	lines.emplace_back(ssprintf("#METER:%d;", in.GetMeter()));
	lines.emplace_back(
	  ssprintf("#MSDVALUES:%s;", MSDToString2(in.GetAllMSD()).c_str()));
	lines.emplace_back(
	  ssprintf("#CHARTKEY:%s;", SmEscape(in.GetChartKey()).c_str()));

	emplace_back_tag(lines, "#MUSIC:%s;", in.GetMusicFile());

	std::vector<std::string> asRadarValues;
	const auto& rv = in.GetRadarValues();
	FOREACH_ENUM(RadarCategory, rc)
	asRadarValues.emplace_back(ssprintf("%i", rv[rc]));
	lines.emplace_back(
	  ssprintf("#RADARVALUES:%s;", join(",", asRadarValues).c_str()));

	emplace_back_tag(lines, "#CREDIT:%s;", in.GetCredit());

	// If the Steps TimingData is not empty, then they have their own
	// timing.  Write out the corresponding tags.
	if (!in.m_Timing.empty()) {
		lines.emplace_back(
		  ssprintf("#OFFSET:%.6f;", in.m_Timing.m_fBeat0OffsetInSeconds));
		GetTimingTags(lines, in.m_Timing);
	}

	switch (in.GetDisplayBPM()) {
		case DISPLAY_BPM_ACTUAL:
			// write nothing
			break;
		case DISPLAY_BPM_SPECIFIED: {
			auto small = in.GetMinBPM();
			auto big = in.GetMaxBPM();
			if (small == big)
				lines.emplace_back(ssprintf("#DISPLAYBPM:%.6f;", small));
			else
				lines.emplace_back(
				  ssprintf("#DISPLAYBPM:%.6f:%.6f;", small, big));
			break;
		}
		case DISPLAY_BPM_RANDOM:
			lines.emplace_back(ssprintf("#DISPLAYBPM:*;"));
			break;
		default:
			break;
	}

	std::string sNoteData;
	in.GetETTNoteData(sNoteData);
	lines.emplace_back(song.m_vsKeysoundFile.empty() ? "#NOTES:" : "#NOTES2:");

	TrimLeft(sNoteData);
	lines.emplace_back(sNoteData);
	lines.emplace_back(";");
	return JoinLineList(lines);
}

bool
NotesWriterETT::Write(std::string& sPath,
					  const Song& out,
					  const vector<Steps*>& vpStepsToSave)
{
	int flags = RageFile::WRITE;

	/* If we're not saving cache, we're saving real data, so enable SLOW_FLUSH
	 * to prevent data loss. If we're saving cache, this will slow things down
	 * too much. */

	RageFile f;
	if (!f.Open(sPath, flags)) {
        Locator::getLogger()->info("Song file \"{}\" couldn't be opened for writing: {}",
                                   sPath, f.GetError().c_str());
		return false;
	}

	WriteGlobalTags(f, out);

	f.PutLine(ssprintf("// cache tags:"));
	f.PutLine(ssprintf("#FIRSTSECOND:%.6f;", out.GetFirstSecond()));
	f.PutLine(ssprintf("#LASTSECOND:%.6f;", out.GetLastSecond()));
	f.PutLine(ssprintf("#SONGFILENAME:%s;", out.m_sSongFileName.c_str()));
	f.PutLine(ssprintf("#HASMUSIC:%i;", out.m_bHasMusic));
	f.PutLine(ssprintf("#HASBANNER:%i;", out.m_bHasBanner));
	f.PutLine(ssprintf("#MUSICLENGTH:%.6f;", out.m_fMusicLengthSeconds));
	f.PutLine(ssprintf("// end cache tags"));

	// Save specified Steps to this file
	FOREACH_CONST(Steps*, vpStepsToSave, s)
	{
		auto pSteps = *s;
		if (!pSteps->GetChartKey().empty()) { // Avoid writing cache tags for
			// invalid chartkey files(empty
			// steps) -Mina
			auto sTag = GetETTNoteData(out, *pSteps);
			f.PutLine(sTag);
		} else {
            //Locator::getLogger()->info("Not caching empty difficulty in file {}", sPath.c_str());
		}
	}
	if (f.Flush() == -1)
		return false;

	return true;
}

static LocalizedString DESTINATION_ALREADY_EXISTS(
  "NotesWriterSSC",
  "Error renaming file.  Destination file '%s' already exists.");
static LocalizedString ERROR_WRITING_FILE("NotesWriterSSC",
										  "Error writing file '%s'.");
