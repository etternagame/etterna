#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "Etterna/Models/Misc/NoteTypes.h"
#include "NotesLoaderSM.h" // may need this.
#include "NotesLoaderSMA.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"

void
SMALoader::ProcessMultipliers(TimingData& out,
							  const int iRowsPerBeat,
							  const std::string& sParam)
{
	vector<std::string> arrayMultiplierExpressions;
	split(sParam, ",", arrayMultiplierExpressions);

	for (auto& arrayMultiplierExpression : arrayMultiplierExpressions) {
		vector<std::string> arrayMultiplierValues;
		split(arrayMultiplierExpression, "=", arrayMultiplierValues);
		unsigned size = arrayMultiplierValues.size();
		if (size < 2) {
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has an invalid #MULTIPLIER value \"%s\" (must have "
//						 "at least one '='), ignored.",
//						 arrayMultiplierExpression.c_str());
			continue;
		}
		const auto fComboBeat =
		  RowToBeat(arrayMultiplierValues[0], iRowsPerBeat);
		const auto iCombos =
		  StringToInt(arrayMultiplierValues[1]); // always true.
		// hoping I'm right here: SMA files can use 6 values after the row/beat.
		const auto iMisses =
		  (size == 2 || size == 4 ? iCombos
								  : StringToInt(arrayMultiplierValues[2]));
		out.AddSegment(
		  ComboSegment(BeatToNoteRow(fComboBeat), iCombos, iMisses));
	}
}

void
SMALoader::ProcessBeatsPerMeasure(TimingData& out, const std::string& sParam)
{
	vector<std::string> vs1;
	split(sParam, ",", vs1);

	for (auto& s1 : vs1) {
		vector<std::string> vs2;
		split(s1, "=", vs2);

		if (vs2.size() < 2) {
//			LOG->UserLog(
//			  "Song file",
//			  this->GetSongTitle(),
//			  "has an invalid beats per measure change with %i values.",
//			  static_cast<int>(vs2.size()));
			continue;
		}
		const auto fBeat = StringToFloat(vs2[0]);
		const auto iNumerator = StringToInt(vs2[1]);

		if (fBeat < 0) {
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has an invalid time signature change with beat %f.",
//						 fBeat);
			continue;
		}
		if (iNumerator < 1) {
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has an invalid time signature change with beat %f, "
//						 "iNumerator %i.",
//						 fBeat,
//						 iNumerator);
			continue;
		}

		out.AddSegment(TimeSignatureSegment(BeatToNoteRow(fBeat), iNumerator));
	}
}

void
SMALoader::ProcessSpeeds(TimingData& out,
						 const std::string& line,
						 const int rowsPerBeat)
{
	vector<std::string> vs1;
	split(line, ",", vs1);

	for (auto& s1 : vs1) {
		vector<std::string> vs2;
		vs2.clear(); // trying something.
		auto loopTmp = s1;
		Trim(loopTmp);
		split(loopTmp, "=", vs2);

		if (vs2.size() == 2) // First one always seems to have 2.
		{
			// Aldo_MX: 4 is the default value in SMA, although SM5 requires 0
			// for the first segment :/
			// dunno what this is doing or if this is right (used to be implicit
			// conversion of string compared to vs1.begin()
			vs2.push_back(s1 == vs1[0] ? "0" : "4");
		}

		if (vs2.size() < 3) {
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has an speed change with %i values.",
//						 static_cast<int>(vs2.size()));
			continue;
		}

		const auto fBeat = RowToBeat(vs2[0], rowsPerBeat);

		auto backup = vs2[2];
		Trim(vs2[2], "s");
		Trim(vs2[2], "S");

		const auto fRatio = StringToFloat(vs2[1]);
		const auto fDelay = StringToFloat(vs2[2]);

		auto unit = ((backup != vs2[2]) ? SpeedSegment::UNIT_SECONDS
										: SpeedSegment::UNIT_BEATS);

		if (fBeat < 0) {
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has an speed change with beat %f.",
//						 fBeat);
			continue;
		}

		if (fDelay < 0) {
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has an speed change with beat %f, length %f.",
//						 fBeat,
//						 fDelay);
			continue;
		}

		out.AddSegment(
		  SpeedSegment(BeatToNoteRow(fBeat), fRatio, fDelay, unit));
	}
}

bool
SMALoader::LoadFromSimfile(const std::string& sPath, Song& out, bool bFromCache)
{
//	LOG->Trace("Song::LoadFromSMAFile(%s)", sPath.c_str());

	MsdFile msd;
	if (!msd.ReadFile(sPath, true)) // unescape
	{
//		LOG->UserLog(
//		  "Song file", sPath, "couldn't be opened: %s", msd.GetError().c_str());
		return false;
	}

	out.m_SongTiming.m_sFile = sPath; // songs still have their fallback timing.
	out.m_sSongFileName = sPath;

	Steps* pNewNotes = nullptr;
	auto iRowsPerBeat = -1; // Start with an invalid value: needed for checking.
	std::vector<std::pair<float, float>> vBPMChanges, vStops;

	for (unsigned i = 0; i < msd.GetNumValues(); i++) {
		int iNumParams = msd.GetNumParams(i);
		const auto& sParams = msd.GetValue(i);
		auto sValueName = make_upper(sParams[0]);

		// handle the data
		/* Don't use GetMainAndSubTitlesFromFullTitle; that's only for
		 * heuristically splitting other formats that *don't* natively support
		 * #SUBTITLE. */
		if (sValueName == "TITLE") {
			out.m_sMainTitle = sParams[1];
			this->SetSongTitle(sParams[1]);
		}

		else if (sValueName == "SUBTITLE")
			out.m_sSubTitle = sParams[1];

		else if (sValueName == "ARTIST")
			out.m_sArtist = sParams[1];

		else if (sValueName == "TITLETRANSLIT")
			out.m_sMainTitleTranslit = sParams[1];

		else if (sValueName == "SUBTITLETRANSLIT")
			out.m_sSubTitleTranslit = sParams[1];

		else if (sValueName == "ARTISTTRANSLIT")
			out.m_sArtistTranslit = sParams[1];

		else if (sValueName == "GENRE")
			out.m_sGenre = sParams[1];

		else if (sValueName == "CREDIT")
			out.m_sCredit = sParams[1];

		else if (sValueName == "BANNER")
			out.m_sBannerFile = sParams[1];

		else if (sValueName == "BACKGROUND")
			out.m_sBackgroundFile = sParams[1];

		else if (sValueName == "PREVIEW")
			out.m_sPreviewVidFile = sParams[1];

		// Save "#LYRICS" for later, so we can add an internal lyrics tag.
		else if (sValueName == "LYRICSPATH")
			out.m_sLyricsFile = sParams[1];

		else if (sValueName == "CDTITLE")
			out.m_sCDTitleFile = sParams[1];

		else if (sValueName == "MUSIC")
			out.m_sMusicFile = sParams[1];

		else if (sValueName == "INSTRUMENTTRACK") {
			SMLoader::ProcessInstrumentTracks(out, sParams[1]);
		}

		else if (sValueName == "MUSICLENGTH") {
			continue;
		}

		else if (sValueName == "LASTBEATHINT") {
			// can't identify at this position: ignore.
		} else if (sValueName == "MUSICBYTES")
			; /* ignore */

		// Cache tags: ignore.
		else if (sValueName == "FIRSTBEAT" || sValueName == "LASTBEAT" ||
				 sValueName == "SONGFILENAME" || sValueName == "HASMUSIC" ||
				 sValueName == "HASBANNER") {
			;
		}

		else if (sValueName == "SAMPLESTART")
			out.m_fMusicSampleStartSeconds = HHMMSSToSeconds(sParams[1]);

		else if (sValueName == "SAMPLELENGTH")
			out.m_fMusicSampleLengthSeconds = HHMMSSToSeconds(sParams[1]);

		// SamplePath is used when the song has a separate preview clip. -aj
		// else if( sValueName=="SAMPLEPATH" )
		// out.m_sMusicSamplePath = sParams[1];

		else if (sValueName == "LISTSORT") {
			;
		}

		else if (sValueName == "DISPLAYBPM") {
			// #DISPLAYBPM:[xxx][xxx:xxx]|[*];
			if (sParams[1] == "*")
				out.m_DisplayBPMType = DISPLAY_BPM_RANDOM;
			else {
				out.m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
				out.m_fSpecifiedBPMMin = StringToFloat(sParams[1]);
				if (sParams[2].empty())
					out.m_fSpecifiedBPMMax = out.m_fSpecifiedBPMMin;
				else
					out.m_fSpecifiedBPMMax = StringToFloat(sParams[2]);
			}
		}

		else if (sValueName == "SMAVERSION") {
			; // ignore it.
		}

		else if (sValueName == "ROWSPERBEAT") {
			/* This value is used to help translate the timings
			 * the SMA format uses. Starting with the second
			 * appearance, it delimits NoteData. Right now, this
			 * value doesn't seem to be editable in SMA. When it
			 * becomes so, make adjustments to this code. */
			if (iRowsPerBeat < 0) {
				vector<std::string> arrayBeatChangeExpressions;
				split(sParams[1], ",", arrayBeatChangeExpressions);

				vector<std::string> arrayBeatChangeValues;
				split(
				  arrayBeatChangeExpressions[0], "=", arrayBeatChangeValues);
				iRowsPerBeat = StringToInt(arrayBeatChangeValues[1]);
			} else {
				// This should generally return song timing
				auto& timing =
				  (pNewNotes ? pNewNotes->m_Timing : out.m_SongTiming);
				ProcessBPMsAndStops(timing, vBPMChanges, vStops);
			}
		}

		else if (sValueName == "BEATSPERMEASURE") {
			auto& timing = (pNewNotes ? pNewNotes->m_Timing : out.m_SongTiming);
			ProcessBeatsPerMeasure(timing, sParams[1]);
		}

		else if (sValueName == "SELECTABLE") {
			if (EqualsNoCase(sParams[1], "YES"))
				out.m_SelectionDisplay = out.SHOW_ALWAYS;
			else if (EqualsNoCase(sParams[1], "NO"))
				out.m_SelectionDisplay = out.SHOW_NEVER;
			// ROULETTE from 3.9. It was removed since UnlockManager can serve
			// the same purpose somehow. This, of course, assumes you're using
			// unlocks. -aj
			else if (EqualsNoCase(sParams[1], "ROULETTE"))
				out.m_SelectionDisplay = out.SHOW_ALWAYS;
			/* The following two cases are just fixes to make sure simfiles that
			 * used 3.9+ features are not excluded here */
			else if (EqualsNoCase(sParams[1], "ES") ||
					 EqualsNoCase(sParams[1], "OMES"))
				out.m_SelectionDisplay = out.SHOW_ALWAYS;
			else if (StringToInt(sParams[1]) > 0)
				out.m_SelectionDisplay = out.SHOW_ALWAYS;
			else {
                //				LOG->UserLog(
//				  "Song file",
//				  sPath,
//				  "has an unknown #SELECTABLE value, \"%s\"; ignored.",
//				  sParams[1].c_str());
			}
		}

		else if (head(sValueName, 9) == "BGCHANGES" ||
				 sValueName == "ANIMATIONS") {
			SMLoader::ProcessBGChanges(out, sValueName, sPath, sParams[1]);
		}

		else if (sValueName == "FGCHANGES") {
			vector<std::string> aFGChangeExpressions;
			split(sParams[1], ",", aFGChangeExpressions);

			for (auto& aFGChangeExpression : aFGChangeExpressions) {
				BackgroundChange change;
				if (LoadFromBGChangesString(change, aFGChangeExpression))
					out.AddForegroundChange(change);
			}
		}

		else if (sValueName == "OFFSET") {
			auto& timing = (pNewNotes ? pNewNotes->m_Timing : out.m_SongTiming);
			timing.m_fBeat0OffsetInSeconds = StringToFloat(sParams[1]);
		}

		else if (sValueName == "BPMS") {
			vBPMChanges.clear();
			ParseBPMs(vBPMChanges, sParams[1], iRowsPerBeat);
		}

		else if (sValueName == "STOPS" || sValueName == "FREEZES") {
			vStops.clear();
			ParseStops(vStops, sParams[1], iRowsPerBeat);
		}

		else if (sValueName == "DELAYS") {
			auto& timing = (pNewNotes ? pNewNotes->m_Timing : out.m_SongTiming);
			ProcessDelays(timing, sParams[1], iRowsPerBeat);
		}

		else if (sValueName == "TICKCOUNT") {
			auto& timing = (pNewNotes ? pNewNotes->m_Timing : out.m_SongTiming);
			ProcessTickcounts(timing, sParams[1], iRowsPerBeat);
		}

		else if (sValueName == "SPEED") {
			auto& timing = (pNewNotes ? pNewNotes->m_Timing : out.m_SongTiming);
			std::string tmp = sParams[1];
			Trim(tmp);
			ProcessSpeeds(timing, tmp, iRowsPerBeat);
		}

		else if (sValueName == "MULTIPLIER") {
			auto& timing = (pNewNotes ? pNewNotes->m_Timing : out.m_SongTiming);
			ProcessMultipliers(timing, iRowsPerBeat, sParams[1]);
		}

		else if (sValueName == "FAKES") {
			auto& timing = (pNewNotes ? pNewNotes->m_Timing : out.m_SongTiming);
			ProcessFakes(timing, sParams[1], iRowsPerBeat);
		}

		else if (sValueName == "METERTYPE") {
			; // We don't use this...yet.
		}

		else if (sValueName == "KEYSOUNDS") {
			split(sParams[1], ",", out.m_vsKeysoundFile);
		}

		else if (sValueName == "NOTES" || sValueName == "NOTES2") {
			if (iNumParams < 7) {
//				LOG->UserLog(
//				  "Song file",
//				  sPath,
//				  "has %d fields in a #NOTES tag, but should have at least 7.",
//				  iNumParams);
				continue;
			}

			pNewNotes = new Steps(&out);
			if (pNewNotes) {
				LoadFromTokens(sParams[1],
							   sParams[2],
							   sParams[3],
							   sParams[4],
							   /*sParams[5],*/ // radar values
							   sParams[6],
							   *pNewNotes);
				pNewNotes->SetFilename(sPath);
				out.AddSteps(pNewNotes);
				// Handle timing changes and convert negative bpms/stops
				auto& timing = pNewNotes->m_Timing;
				ProcessBPMsAndStops(timing, vBPMChanges, vStops);
			} else {
				auto& timing = out.m_SongTiming;
				ProcessBPMsAndStops(timing, vBPMChanges, vStops);
			}
		} else if (sValueName == "TIMESIGNATURES" || sValueName == "LEADTRACK")
			;
		else {
//			LOG->UserLog("Song file",
//						 sPath,
//						 "has an unexpected value named \"%s\".",
//						 sValueName.c_str());
		}

	}
	TidyUpData(out, false);
	return true;
}
