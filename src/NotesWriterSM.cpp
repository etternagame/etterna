#include "global.h"
#include "BackgroundUtil.h"
#include "Foreach.h"
#include "GameManager.h"
#include "LocalizedString.h"
#include "NoteTypes.h"
#include "NotesWriterSM.h"
#include "ProfileManager.h"
#include "RageFile.h"
#include "Song.h"
#include "Steps.h"
#include "ThemeMetric.h"

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

	for (unsigned i = 0; i < stops.size(); i++) {
		const StopSegment* fs = ToStop(stops[i]);
		// Handle warps on the same row by summing the values.  Not sure this
		// plays out the same. -Kyz
		map<float, float>::iterator already_exists =
		  allPauses.find(fs->GetBeat());
		if (already_exists != allPauses.end()) {
			already_exists->second += fs->GetPause();
		} else {
			allPauses.insert(pair<float, float>(fs->GetBeat(), fs->GetPause()));
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
			allPauses.insert(pair<float, float>(fBeat, fPause));
		}
	}

	f.Write("#STOPS:");
	vector<RString> stopLines;
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

	if (out.GetForegroundChanges().size()) {
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
static RString
JoinLineList(vector<RString>& lines)
{
	for (unsigned i = 0; i < lines.size(); ++i)
		TrimRight(lines[i]);

	/* Skip leading blanks. */
	unsigned j = 0;
	while (j < lines.size() && lines.size() == 0)
		++j;

	return join("\r\n", lines.begin() + j, lines.end());
}

/**
 * @brief Retrieve the notes from the #NOTES tag.
 * @param song the Song in question.
 * @param in the Steps in question.
 * @return the #NOTES tag. */
static RString
GetSMNotesTag(const Song& song, const Steps& in)
{
	vector<RString> lines;

	lines.push_back("");
	// Escape to prevent some clown from making a comment of "\r\n;"
	lines.push_back(ssprintf("//---------------%s - %s----------------",
							 in.m_StepsTypeStr.c_str(),
							 SmEscape(in.GetDescription()).c_str()));
	lines.push_back(song.m_vsKeysoundFile.empty() ? "#NOTES:" : "#NOTES2:");
	lines.push_back(ssprintf("     %s:", in.m_StepsTypeStr.c_str()));
	RString desc = (USE_CREDIT ? in.GetCredit() : in.GetChartName());
	lines.push_back(ssprintf("     %s:", SmEscape(desc).c_str()));
	lines.push_back(
	  ssprintf("     %s:", DifficultyToString(in.GetDifficulty()).c_str()));
	lines.push_back(ssprintf("     %d:", in.GetMeter()));

	vector<RString> asRadarValues;
	int categories = 11;
	const RadarValues& rv = in.GetRadarValues();
	for (RadarCategory rc = (RadarCategory)0; rc < categories;
		 enum_add<RadarCategory>(rc, 1)) {
		asRadarValues.push_back(IntToString(rv[rc]));
	}
	lines.push_back(ssprintf("     %s:", join(",", asRadarValues).c_str()));

	RString sNoteData;
	in.GetSMNoteData(sNoteData);

	split(sNoteData, "\n", lines, true);
	lines.push_back(";");

	return JoinLineList(lines);
}

bool
NotesWriterSM::Write(const RString& sPath,
					 Song& out,
					 const vector<Steps*>& vpStepsToSave)
{
	int flags = RageFile::WRITE;

	flags |= RageFile::SLOW_FLUSH;

	RageFile f;
	if (!f.Open(sPath, flags)) {
		LOG->UserLog("Song file",
					 sPath,
					 "couldn't be opened for writing: %s",
					 f.GetError().c_str());
		return false;
	}

	WriteGlobalTags(f, out);

	FOREACH_CONST(Steps*, vpStepsToSave, s)
	{
		const Steps* pSteps = *s;
		RString sTag = GetSMNotesTag(out, *pSteps);
		f.PutLine(sTag);
	}
	if (f.Flush() == -1)
		return false;

	return true;
}

void
NotesWriterSM::GetEditFileContents(const Song* pSong,
								   const Steps* pSteps,
								   RString& sOut)
{
	sOut = "";
	RString sDir = pSong->GetSongDir();

	// "Songs/foo/bar"; strip off "Songs/".
	vector<RString> asParts;
	split(sDir, "/", asParts);
	if (asParts.size())
		sDir = join("/", asParts.begin() + 1, asParts.end());
	sOut += ssprintf("#SONG:%s;\r\n", sDir.c_str());
	sOut += GetSMNotesTag(*pSong, *pSteps);
}

RString
NotesWriterSM::GetEditFileName(const Song* pSong, const Steps* pSteps)
{
	/* Try to make a unique name. This isn't guaranteed. Edit descriptions are
	 * case-sensitive, filenames on disk are usually not, and we decimate
	 * certain characters for FAT filesystems. */
	RString sFile =
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
