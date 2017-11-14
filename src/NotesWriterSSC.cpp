#include "global.h"
#include <cerrno>
#include <cstring>
#include "NotesWriterSSC.h"
#include "BackgroundUtil.h"
#include "Foreach.h"
#include "GameManager.h"
#include "LocalizedString.h"
#include "NoteTypes.h"
#include "Profile.h"
#include "ProfileManager.h"
#include "RageFile.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "Song.h"
#include "Steps.h"

/**
 * @brief Turn a vector of lines into a single line joined by newline characters.
 * @param lines the list of lines to join.
 * @return the joined lines. */
static RString JoinLineList( vector<RString> &lines )
{
	for( unsigned i = 0; i < lines.size(); ++i )
		TrimRight( lines[i] );

	// Skip leading blanks.
	unsigned j = 0;
	while( j < lines.size() && lines.size() == 0 )
		++j;

	return join( "\r\n", lines.begin()+j, lines.end() );
}

RString MSDToString(MinaSD x) {
	RString o = "";
	for (size_t i = 0; i < x.size(); i++) {
		for (size_t ii = 0; ii < x[i].size(); ii++) {
			o.append(to_string(x[i][ii]).substr(0,5));
			if (ii != x[i].size() - 1)
				o.append(",");
		}
		if (i != x.size() - 1)
			o.append(":");
	}
	return o;
}

// A utility class to write timing tags more easily!
struct TimingTagWriter {

	vector<RString> *m_pvsLines;
	RString m_sNext;

	TimingTagWriter( vector<RString> *pvsLines ): m_pvsLines (pvsLines) { }

	void Write( const int row, const char *value )
	{
		m_pvsLines->emplace_back( m_sNext + ssprintf( "%.6f=%s", NoteRowToBeat(row), value ) );
		m_sNext = ",";
	}

	void Write( const int row, const float value )        { Write( row, ssprintf( "%.6f",  value ) ); }
	void Write( const int row, const int value )          { Write( row, ssprintf( "%d",    value ) ); }
	void Write( const int row, const int a, const int b ) { Write( row, ssprintf( "%d=%d", a, b ) );  }
	void Write( const int row, const float a, const float b ) { Write( row, ssprintf( "%.6f=%.6f", a, b) ); }
	void Write( const int row, const float a, const float b, const unsigned short c )
		{ Write( row, ssprintf( "%.6f=%.6f=%hd", a, b, c) ); }

	void Init( const RString sTag ) { m_sNext = "#" + sTag + ":"; }
	void Finish( ) { m_pvsLines->emplace_back( ( m_sNext != "," ? m_sNext : "" ) + ";" ); }

};

static void GetTimingTags( vector<RString> &lines, const TimingData &timing, bool bIsSong = false )
{
	TimingTagWriter writer ( &lines );

	// timing.TidyUpData(); // UGLY: done via const_cast. do we really -need- this here?
#define WRITE_SEG_LOOP_OPEN(enum_type, seg_type, seg_name, to_func) \
	{ \
		vector<TimingSegment*> const& segs= timing.GetTimingSegments(enum_type); \
		if(!segs.empty()) \
		{ \
			writer.Init(seg_name); \
			for(auto&& seg : segs) \
			{ \
				const seg_type* segment= to_func(seg);

#define WRITE_SEG_LOOP_CLOSE } writer.Finish(); } }

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

	WRITE_SEG_LOOP_OPEN(SEGMENT_TIME_SIG, TimeSignatureSegment, "TIMESIGNATURESEGMENT", ToTimeSignature);
	writer.Write(segment->GetRow(), segment->GetNum(), segment->GetDen());
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_TICKCOUNT, TickcountSegment, "TICKCOUNTS", ToTickcount);
	writer.Write(segment->GetRow(), segment->GetTicks());
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_COMBO, ComboSegment, "COMBOS", ToCombo);
	if (segment->GetCombo() == segment->GetMissCombo())
	{
		writer.Write(segment->GetRow(), segment->GetCombo());
	}
	else
	{
		writer.Write(segment->GetRow(), segment->GetCombo(), segment->GetMissCombo());
	}
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_SPEED, SpeedSegment, "SPEEDS", ToSpeed);
	writer.Write(segment->GetRow(), segment->GetRatio(), segment->GetDelay(), segment->GetUnit());
	WRITE_SEG_LOOP_CLOSE;

	WRITE_SEG_LOOP_OPEN(SEGMENT_SCROLL, ScrollSegment, "SCROLLS", ToScroll);
	writer.Write(segment->GetRow(), segment->GetRatio());
	WRITE_SEG_LOOP_CLOSE;

	// TODO: Investigate why someone wrote a condition for leaving fakes out of
	// the song timing tags, and put it in a function that is only used when
	// writing the steps timing tags. -Kyz
	if (!bIsSong)
	{
		WRITE_SEG_LOOP_OPEN(SEGMENT_FAKE, FakeSegment, "FAKES", ToFake);
		writer.Write(segment->GetRow(), segment->GetLength());
		WRITE_SEG_LOOP_CLOSE;
	}

	WRITE_SEG_LOOP_OPEN(SEGMENT_LABEL, LabelSegment, "LABELS", ToLabel);
	if (!segment->GetLabel().empty())
	{
		writer.Write(segment->GetRow(), segment->GetLabel());
	}
	WRITE_SEG_LOOP_CLOSE;

#undef WRITE_SEG_LOOP_OPEN
#undef WRITE_SEG_LOOP_CLOSE
}

static void write_tag(RageFile& f, RString const& format,
	std::string const& value)
{
	if (!value.empty())
	{
		f.PutLine(ssprintf(format, SmEscape(value).c_str()));
	}
}

static void WriteTimingTags( RageFile &f, const TimingData &timing, bool bIsSong = false )
{
	write_tag(f, "#BPMS:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_BPM, 3)));
	write_tag(f, "#STOPS:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_STOP, 3)));
	write_tag(f, "#DELAYS:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_DELAY, 3)));
	write_tag(f, "#WARPS:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_WARP, 3)));
	write_tag(f, "#TIMESIGNATURES:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_TIME_SIG, 3)));
	write_tag(f, "#TICKCOUNTS:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_TICKCOUNT, 3)));
	write_tag(f, "#COMBOS:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_COMBO, 3)));
	write_tag(f, "#SPEEDS:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_SPEED, 3)));
	write_tag(f, "#SCROLLS:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_SCROLL, 3)));
	write_tag(f, "#FAKES:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_FAKE, 3)));
	write_tag(f, "#LABELS:%s;",
		join(",\r\n", timing.ToVectorString(SEGMENT_LABEL, 3)));

}

/**
 * @brief Write out the common tags for .SSC files.
 * @param f the file in question.
 * @param out the Song in question. */
static void WriteGlobalTags( RageFile &f, const Song &out )
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
		if (!vs.empty())
		{
			std::string s = join(",", vs);
			f.PutLine("#INSTRUMENTTRACK:" + s + ";\n");
		}
	}
	f.PutLine(ssprintf("#OFFSET:%.6f;", out.m_SongTiming.m_fBeat0OffsetInSeconds));
	f.PutLine(ssprintf("#SAMPLESTART:%.6f;", out.m_fMusicSampleStartSeconds));
	f.PutLine(ssprintf("#SAMPLELENGTH:%.6f;", out.m_fMusicSampleLengthSeconds));

	f.Write("#SELECTABLE:");
	switch (out.m_SelectionDisplay)
	{
	default: ASSERT_M(0, "An invalid selectable value was found for this song!"); // fall through
	case Song::SHOW_ALWAYS:	f.Write("YES");		break;
		//case Song::SHOW_NONSTOP:	f.Write( "NONSTOP" );	break;
	case Song::SHOW_NEVER:		f.Write("NO");		break;
	}
	f.PutLine(";");

	switch (out.m_DisplayBPMType)
	{
	case DISPLAY_BPM_ACTUAL:
		// write nothing
		break;
	case DISPLAY_BPM_SPECIFIED:
		if (out.m_fSpecifiedBPMMin == out.m_fSpecifiedBPMMax)
		{
			f.PutLine(ssprintf("#DISPLAYBPM:%.6f;", out.m_fSpecifiedBPMMin));
		}
		else
		{
			f.PutLine(ssprintf("#DISPLAYBPM:%.6f:%.6f;", out.m_fSpecifiedBPMMin, out.m_fSpecifiedBPMMax));
		}
		break;
	case DISPLAY_BPM_RANDOM:
		f.PutLine(ssprintf("#DISPLAYBPM:*;"));
		break;
	default:
		break;
	}

	WriteTimingTags(f, out.m_SongTiming);

	if (out.GetSpecifiedLastSecond() > 0)
	{
		f.PutLine(ssprintf("#LASTSECONDHINT:%.6f;", out.GetSpecifiedLastSecond()));
	}
	FOREACH_BackgroundLayer(b)
	{
		if (out.GetBackgroundChanges(b).empty())
		{
			continue;	// skip
		}
		if (b == 0)
		{
			f.Write("#BGCHANGES:");
		}
		else
		{
			f.Write(ssprintf("#BGCHANGES%d:", b + 1));
		}
		for (auto &bgc : out.GetBackgroundChanges(b))
		{
			f.PutLine(bgc.ToString() + ",");
		}

		/* If there's an animation plan at all, add a dummy "-nosongbg-" tag to
		* indicate that this file doesn't want a song BG entry added at the end.
		* See SSCLoader::TidyUpData. This tag will be removed on load. Add it
		* at a very high beat, so it won't cause problems if loaded in older versions. */
		if (b == 0 && !out.GetBackgroundChanges(b).empty())
		{
			f.PutLine("99999=-nosongbg-=1.000=0=0=0 // don't automatically add -songbackground-");
		}
		f.PutLine(";");
	}

	if (out.GetForegroundChanges().size())
	{
		f.Write("#FGCHANGES:");
		for (auto const &bgc : out.GetForegroundChanges())
		{
			f.PutLine(bgc.ToString() + ",");
		}
		f.PutLine(";");
	}

	if (!out.m_vsKeysoundFile.empty())
	{
		f.Write("#KEYSOUNDS:");
		for (unsigned i = 0; i < out.m_vsKeysoundFile.size(); i++)
		{
			// some keysound files has the first sound that starts with #,
			// which makes MsdFile fail parsing the whole declaration.
			// in this case, add a backslash at the front
			// (#KEYSOUNDS:\#bgm.wav,01.wav,02.wav,..) and handle that on load.
			if (i == 0 && out.m_vsKeysoundFile[i].size() > 0 && out.m_vsKeysoundFile[i][0] == '#')
			{
				f.Write("\\");
			}
			f.Write(out.m_vsKeysoundFile[i]);
			if (i != out.m_vsKeysoundFile.size() - 1)
			{
				f.Write(",");
			}
		}
		f.PutLine(";");
	}

	f.PutLine("");
}

static void emplace_back_tag(vector<RString>& lines,
	RString const& format, RString const& value)
{
	if (!value.empty())
	{
		lines.emplace_back(ssprintf(format, SmEscape(value).c_str()));
	}
}

/**
 * @brief Retrieve the individual batches of NoteData.
 * @param song the Song in question.
 * @param in the Steps in question.
 * @param bSavingCache a flag to see if we're saving certain cache data.
 * @return the NoteData in RString form. */
static RString GetSSCNoteData( const Song &song, const Steps &in, bool bSavingCache )
{
	vector<RString> lines;

	lines.emplace_back("");
	// Escape to prevent some clown from making a comment of "\r\n;"
	lines.emplace_back(ssprintf("//---------------%s - %s----------------",
		in.m_StepsTypeStr.c_str(), SmEscape(in.GetDescription()).c_str()));
	lines.emplace_back("#NOTEDATA:;"); // our new separator.
	emplace_back_tag(lines, "#CHARTNAME:%s;", in.GetChartName());
	emplace_back_tag(lines, "#STEPSTYPE:%s;", in.m_StepsTypeStr);
	emplace_back_tag(lines, "#DESCRIPTION:%s;", in.GetDescription());
	emplace_back_tag(lines, "#CHARTSTYLE:%s;", in.GetChartStyle());
	emplace_back_tag(lines, "#DIFFICULTY:%s;", DifficultyToString(in.GetDifficulty()));
	lines.emplace_back(ssprintf("#METER:%d;", in.GetMeter()));
	lines.emplace_back(ssprintf("#MSDVALUES:%s;", MSDToString(in.GetAllMSD()).c_str()));
	lines.emplace_back(ssprintf("#CHARTKEY:%s;", SmEscape(in.GetChartKey()).c_str()));

	emplace_back_tag(lines, "#MUSIC:%s;", in.GetMusicFile());

	vector<RString> asRadarValues;
	const RadarValues &rv = in.GetRadarValues();
	FOREACH_ENUM(RadarCategory, rc)
		asRadarValues.emplace_back(ssprintf("%i", rv[rc]));
	lines.emplace_back(ssprintf("#RADARVALUES:%s;", join(",", asRadarValues).c_str()));

	emplace_back_tag(lines, "#CREDIT:%s;", in.GetCredit());

	// If the Steps TimingData is not empty, then they have their own
	// timing.  Write out the corresponding tags.
	if (!in.m_Timing.empty())
	{
		lines.emplace_back(ssprintf("#OFFSET:%.6f;", in.m_Timing.m_fBeat0OffsetInSeconds));
		GetTimingTags(lines, in.m_Timing);
	}

	switch (in.GetDisplayBPM())
	{
	case DISPLAY_BPM_ACTUAL:
		// write nothing
		break;
	case DISPLAY_BPM_SPECIFIED:
	{
		float small = in.GetMinBPM();
		float big = in.GetMaxBPM();
		if (small == big)
			lines.emplace_back(ssprintf("#DISPLAYBPM:%.6f;", small));
		else
			lines.emplace_back(ssprintf("#DISPLAYBPM:%.6f:%.6f;", small, big));
		break;
	}
	case DISPLAY_BPM_RANDOM:
		lines.emplace_back(ssprintf("#DISPLAYBPM:*;"));
		break;
	default:
		break;
	}
	if (bSavingCache)
	{
		lines.emplace_back(ssprintf("#STEPFILENAME:%s;", in.GetFilename().c_str()));
	}
	else
	{
		RString sNoteData = "";

		/* hack to ensure notedata exists when changing offset from gameplay not sure what i/we could have done 
		to mess up the original flow but all the save/load functions should be rewritten anyway -mina */ 
		in.Decompress();

		in.GetSMNoteData(sNoteData);
		lines.emplace_back(song.m_vsKeysoundFile.empty() ? "#NOTES:" : "#NOTES2:");

		TrimLeft(sNoteData);
		vector<RString> splitData;
		split(sNoteData, "\n", splitData);
		lines.insert(lines.end(), std::make_move_iterator(splitData.begin()), std::make_move_iterator(splitData.end()));
		lines.emplace_back(";");

		// cause of the decompress call above- maybe unnecessary because who's going to resync a file and not play it? -mina
		in.Compress();
	}
	return JoinLineList(lines);
}

bool NotesWriterSSC::Write( RString &sPath, const Song &out, const vector<Steps*>& vpStepsToSave, bool bSavingCache )
{
	int flags = RageFile::WRITE;

	/* If we're not saving cache, we're saving real data, so enable SLOW_FLUSH
	 * to prevent data loss. If we're saving cache, this will slow things down
	 * too much. */
	if( !bSavingCache )
		flags |= RageFile::SLOW_FLUSH;

	RageFile f;
	if( !f.Open( sPath, flags ) )
	{
		LOG->UserLog( "Song file", sPath, "couldn't be opened for writing: %s", f.GetError().c_str() );
		return false;
	}

	WriteGlobalTags( f, out );
	
	if( bSavingCache )
	{
		f.PutLine( ssprintf( "// cache tags:" ) );
		f.PutLine( ssprintf( "#FIRSTSECOND:%.6f;", out.GetFirstSecond() ) );
		f.PutLine( ssprintf( "#LASTSECOND:%.6f;", out.GetLastSecond() ) );
		f.PutLine( ssprintf( "#SONGFILENAME:%s;", out.m_sSongFileName.c_str() ) );
		f.PutLine( ssprintf( "#HASMUSIC:%i;", out.m_bHasMusic ) );
		f.PutLine( ssprintf( "#HASBANNER:%i;", out.m_bHasBanner ) );
		f.PutLine( ssprintf( "#MUSICLENGTH:%.6f;", out.m_fMusicLengthSeconds ) );
		f.PutLine( ssprintf( "// end cache tags" ) );
	}

	// Save specified Steps to this file
	FOREACH_CONST(Steps*, vpStepsToSave, s)
	{
		const Steps* pSteps = *s;
		if (pSteps->GetChartKey() != "") {		// Avoid writing cache tags for invalid chartkey files(empty steps) -Mina
			RString sTag = GetSSCNoteData(out, *pSteps, bSavingCache);
			f.PutLine(sTag);
		}
		else
		{
			LOG->Info("Not caching empty difficulty in file %s", sPath.c_str());
		}
	}
	if( f.Flush() == -1 )
		return false;

	return true;
}

void NotesWriterSSC::GetEditFileContents( const Song *pSong, const Steps *pSteps, RString &sOut )
{
	sOut = "";
	RString sDir = pSong->GetSongDir();

	// "Songs/foo/bar"; strip off "Songs/".
	vector<RString> asParts;
	split( sDir, "/", asParts );
	if( asParts.size() )
		sDir = join( "/", asParts.begin()+1, asParts.end() );
	sOut += ssprintf( "#SONG:%s;\r\n", sDir.c_str() );
	sOut += GetSSCNoteData( *pSong, *pSteps, false );
}

RString NotesWriterSSC::GetEditFileName( const Song *pSong, const Steps *pSteps )
{
	/* Try to make a unique name. This isn't guaranteed. Edit descriptions are
	 * case-sensitive, filenames on disk are usually not, and we decimate certain
	 * characters for FAT filesystems. */
	RString sFile = pSong->GetTranslitFullTitle() + " - " + pSteps->GetDescription();

	// HACK:
	if( pSteps->m_StepsType == StepsType_dance_double )
		sFile += " (doubles)";

	sFile += ".edit";

	MakeValidFilename( sFile );
	return sFile;
}

static LocalizedString DESTINATION_ALREADY_EXISTS	("NotesWriterSSC", "Error renaming file.  Destination file '%s' already exists.");
static LocalizedString ERROR_WRITING_FILE		("NotesWriterSSC", "Error writing file '%s'.");

/*
 * (c) 2011 Jason Felds
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
