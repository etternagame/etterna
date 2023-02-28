#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/FileTypes/MsdFile.h"
#include "Etterna/Models/Misc/NoteTypes.h"
#include "NotesLoaderSM.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Singletons/SongManager.h"

#include <algorithm>

using std::pair;
using std::string;

// Everything from this line to the creation of sm_parser_helper exists to
// speed up parsing by allowing the use of std::map.  All these functions
// are put into a map of function pointers which is used when loading.
// -Kyz
/****************************************************************/
struct SMSongTagInfo
{
	SMLoader* loader;
	Song* song;
	const MsdFile::value_t* params;
	const std::string& path;
	std::vector<pair<float, float>> BPMChanges, Stops;
	SMSongTagInfo(SMLoader* l, Song* s, const std::string& p)
	  : loader(l)
	  , song(s)
	  , path(p)
	{
		params = nullptr;
	}
};

using song_tag_func_t = void (*)(SMSongTagInfo&);

// Functions for song tags go below this line. -Kyz
/****************************************************************/
void
SMSetTitle(SMSongTagInfo& info)
{
	info.song->m_sMainTitle = (*info.params)[1];
	info.loader->SetSongTitle((*info.params)[1]);
}
void
SMSetSubtitle(SMSongTagInfo& info)
{
	info.song->m_sSubTitle = (*info.params)[1];
}
void
SMSetArtist(SMSongTagInfo& info)
{
	info.song->m_sArtist = (*info.params)[1];
}
void
SMSetTitleTranslit(SMSongTagInfo& info)
{
	info.song->m_sMainTitleTranslit = (*info.params)[1];
}
void
SMSetSubtitleTranslit(SMSongTagInfo& info)
{
	info.song->m_sSubTitleTranslit = (*info.params)[1];
}
void
SMSetArtistTranslit(SMSongTagInfo& info)
{
	info.song->m_sArtistTranslit = (*info.params)[1];
}
void
SMSetGenre(SMSongTagInfo& info)
{
	info.song->m_sGenre = (*info.params)[1];
}
void
SMSetCredit(SMSongTagInfo& info)
{
	info.song->m_sCredit = (*info.params)[1];
}
void
SMSetBanner(SMSongTagInfo& info)
{
	info.song->m_sBannerFile = (*info.params)[1];
}
void
SMSetBackground(SMSongTagInfo& info)
{
	info.song->m_sBackgroundFile = (*info.params)[1];
}
void
SMSetLyricsPath(SMSongTagInfo& info)
{
	info.song->m_sLyricsFile = (*info.params)[1];
}
void
SMSetCDTitle(SMSongTagInfo& info)
{
	info.song->m_sCDTitleFile = (*info.params)[1];
}
void
SMSetMusic(SMSongTagInfo& info)
{
	info.song->m_sMusicFile = (*info.params)[1];
}
void
SMSetOffset(SMSongTagInfo& info)
{
	info.song->m_SongTiming.m_fBeat0OffsetInSeconds =
	  StringToFloat((*info.params)[1]);
}
void
SMSetBPMs(SMSongTagInfo& info)
{
	info.BPMChanges.clear();
	info.loader->ParseBPMs(info.BPMChanges, (*info.params)[1]);
}
void
SMSetStops(SMSongTagInfo& info)
{
	info.Stops.clear();
	info.loader->ParseStops(info.Stops, (*info.params)[1]);
}
void
SMSetDelays(SMSongTagInfo& info)
{
	info.loader->ProcessDelays(info.song->m_SongTiming, (*info.params)[1]);
}
void
SMSetTimeSignatures(SMSongTagInfo& info)
{
	info.loader->ProcessTimeSignatures(info.song->m_SongTiming,
									   (*info.params)[1]);
}
void
SMSetTickCounts(SMSongTagInfo& info)
{
	info.loader->ProcessTickcounts(info.song->m_SongTiming, (*info.params)[1]);
}
void
SMSetInstrumentTrack(SMSongTagInfo& info)
{
	info.loader->ProcessInstrumentTracks(*info.song, (*info.params)[1]);
}
void
SMSetSampleStart(SMSongTagInfo& info)
{
	info.song->m_fMusicSampleStartSeconds = HHMMSSToSeconds((*info.params)[1]);
}
void
SMSetSampleLength(SMSongTagInfo& info)
{
	info.song->m_fMusicSampleLengthSeconds = HHMMSSToSeconds((*info.params)[1]);
}
void
SMSetDisplayBPM(SMSongTagInfo& info)
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
SMSetSelectable(SMSongTagInfo& info)
{
	if (EqualsNoCase((*info.params)[1], "YES")) {
		info.song->m_SelectionDisplay = info.song->SHOW_ALWAYS;
	} else if (EqualsNoCase((*info.params)[1], "NO")) {
		info.song->m_SelectionDisplay = info.song->SHOW_NEVER;
	}
	// ROULETTE from 3.9. It was removed since UnlockManager can serve
	// the same purpose somehow. This, of course, assumes you're using
	// unlocks. -aj
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
SMSetBGChanges(SMSongTagInfo& info)
{
	info.loader->ProcessBGChanges(
	  *info.song, (*info.params)[0], info.path, (*info.params)[1]);
}
void
SMSetFGChanges(SMSongTagInfo& info)
{
	std::vector<std::string> aFGChangeExpressions;
	split((*info.params)[1], ",", aFGChangeExpressions);

	for (auto& aFGChangeExpression : aFGChangeExpressions) {
		BackgroundChange change;
		if (info.loader->LoadFromBGChangesString(change, aFGChangeExpression))
			info.song->AddForegroundChange(change);
	}
}
void
SMSetKeysounds(SMSongTagInfo& info)
{
	split((*info.params)[1], ",", info.song->m_vsKeysoundFile);
}
typedef std::map<std::string, song_tag_func_t> song_handler_map_t;

struct sm_parser_helper_t
{
	song_handler_map_t song_tag_handlers;
	// Unless signed, the comments in this tag list are not by me.  They were
	// moved here when converting from the else if chain. -Kyz
	sm_parser_helper_t()
	{
		song_tag_handlers["TITLE"] = &SMSetTitle;
		song_tag_handlers["SUBTITLE"] = &SMSetSubtitle;
		song_tag_handlers["ARTIST"] = &SMSetArtist;
		song_tag_handlers["TITLETRANSLIT"] = &SMSetTitleTranslit;
		song_tag_handlers["SUBTITLETRANSLIT"] = &SMSetSubtitleTranslit;
		song_tag_handlers["ARTISTTRANSLIT"] = &SMSetArtistTranslit;
		song_tag_handlers["GENRE"] = &SMSetGenre;
		song_tag_handlers["CREDIT"] = &SMSetCredit;
		song_tag_handlers["BANNER"] = &SMSetBanner;
		song_tag_handlers["BACKGROUND"] = &SMSetBackground;
		// Save "#LYRICS" for later, so we can add an internal lyrics tag.
		song_tag_handlers["LYRICSPATH"] = &SMSetLyricsPath;
		song_tag_handlers["CDTITLE"] = &SMSetCDTitle;
		song_tag_handlers["MUSIC"] = &SMSetMusic;
		song_tag_handlers["OFFSET"] = &SMSetOffset;
		song_tag_handlers["BPMS"] = &SMSetBPMs;
		song_tag_handlers["STOPS"] = &SMSetStops;
		song_tag_handlers["FREEZES"] = &SMSetStops;
		song_tag_handlers["DELAYS"] = &SMSetDelays;
		song_tag_handlers["TIMESIGNATURES"] = &SMSetTimeSignatures;
		song_tag_handlers["TICKCOUNTS"] = &SMSetTickCounts;
		song_tag_handlers["INSTRUMENTTRACK"] = &SMSetInstrumentTrack;
		song_tag_handlers["SAMPLESTART"] = &SMSetSampleStart;
		song_tag_handlers["SAMPLELENGTH"] = &SMSetSampleLength;
		song_tag_handlers["DISPLAYBPM"] = &SMSetDisplayBPM;
		song_tag_handlers["SELECTABLE"] = &SMSetSelectable;
		// It's a bit odd to have the tag that exists for backwards
		// compatibility in this list and not the replacement, but the BGCHANGES
		// tag has a number on the end, allowing up to NUM_BackgroundLayer tags,
		// so it can't fit in the map. -Kyz
		song_tag_handlers["ANIMATIONS"] = &SMSetBGChanges;
		song_tag_handlers["FGCHANGES"] = &SMSetFGChanges;
		song_tag_handlers["KEYSOUNDS"] = &SMSetKeysounds;
		// Attacks loaded from file
		/* Tags that no longer exist, listed for posterity.  May their names
		 * never be forgotten for their service to Stepmania. -Kyz
		 * LASTBEATHINT: // unable to identify at this point: ignore
		 * MUSICBYTES: // ignore
		 * FIRSTBEAT: // cache tags from older SM files: ignore.
		 * LASTBEAT: // cache tags from older SM files: ignore.
		 * SONGFILENAME: // cache tags from older SM files: ignore.
		 * HASMUSIC: // cache tags from older SM files: ignore.
		 * HASBANNER: // cache tags from older SM files: ignore.
		 * SAMPLEPATH: // SamplePath was used when the song has a separate
		 * preview clip. -aj LEADTRACK: // XXX: Does anyone know what LEADTRACK
		 * is for? -Wolfman2000 MUSICLENGTH: // Loaded from the cache now. -Kyz
		 * ATTACKS: // Stupid. -mina
		 */
	}
};
sm_parser_helper_t sm_parser_helper;
// End sm_parser_helper related functions. -Kyz
/****************************************************************/

void
SMLoader::SetSongTitle(const std::string& title)
{
	this->songTitle = title;
}

std::string
SMLoader::GetSongTitle() const
{
	return this->songTitle;
}

bool
SMLoader::LoadFromDir(const std::string& sPath, Song& out)
{
	std::vector<std::string> aFileNames;
	GetApplicableFiles(sPath, aFileNames);
	return LoadFromSimfile(sPath + aFileNames[0], out);
}

float
SMLoader::RowToBeat(const std::string& line, const int rowsPerBeat)
{
	auto trimmed = line;
	Trim(trimmed, "r");
	Trim(trimmed, "R");
	if (trimmed != line) {
		return StringToFloat(trimmed) / rowsPerBeat;
	} else {
		return StringToFloat(trimmed);
	}
}

void
SMLoader::LoadFromTokens(std::string sStepsType,
						 std::string sDescription,
						 std::string sDifficulty,
						 std::string sMeter,
						 std::string sNoteData,
						 Steps& out)
{
	// we're loading from disk, so this is by definition already saved:
	out.SetSavedToDisk(true);

	Trim(sStepsType);
	Trim(sDescription);
	Trim(sDifficulty);
	Trim(sNoteData);

	// LOG->Trace( "Steps::LoadFromTokens(), %s", sStepsType.c_str() );

	// backwards compatibility hacks:
	// HACK: We eliminated "ez2-single-hard", but we should still handle it.
	if (sStepsType == "ez2-single-hard")
		sStepsType = "ez2-single";

	// HACK: replace para and para-single with pump (both are 5 keys)
	if (sStepsType == "para" || sStepsType == "para-single")
		sStepsType = "pump-single";

	// what could go wrong with doing this? (a lot of things)
	// we removed couple but those charts probably work better as 8k charts
	// chartkey resolving for loading notedata should make this work
	if (sStepsType == "dance-couple")
		sStepsType = "dance-double";

	out.m_StepsType = GAMEMAN->StringToStepsType(sStepsType);
	out.m_StepsTypeStr = sStepsType;
	out.SetDescription(sDescription);
	out.SetCredit(sDescription);	// this is often used for both.
	out.SetChartName(sDescription); // yeah, one more for good measure.
	out.SetDifficulty(OldStyleStringToDifficulty(sDifficulty));

	// Handle hacks that originated back when StepMania didn't have
	// Difficulty_Challenge. (At least v1.64, possibly v3.0 final...)
	if (out.GetDifficulty() == Difficulty_Hard) {
		// HACK: SMANIAC used to be Difficulty_Hard with a special description.
		if (CompareNoCase(sDescription, "smaniac") == 0)
			out.SetDifficulty(Difficulty_Challenge);

		// HACK: CHALLENGE used to be Difficulty_Hard with a special
		// description.
		if (CompareNoCase(sDescription, "challenge") == 0)
			out.SetDifficulty(Difficulty_Challenge);
	}

	if (sMeter.empty()) {
		// some simfiles (e.g. X-SPECIALs from Zenius-I-Vanisher) don't
		// have a meter on certain steps. Make the meter 1 in these instances.
		sMeter = "1";
	}
	out.SetMeter(StringToInt(sMeter));

	out.SetSMNoteData(sNoteData);

	out.TidyUpData();
}

void
SMLoader::ProcessBGChanges(Song& out,
						   const std::string& sValueName,
						   const std::string& sPath,
						   const std::string& sParam)
{
	auto iLayer = BACKGROUND_LAYER_1;
	if (sscanf(
		  sValueName.c_str(), "BGCHANGES%d", &*ConvertValue<int>(&iLayer)) == 1)
		enum_add(iLayer, -1); // #BGCHANGES2 = BACKGROUND_LAYER_2

	auto bValid = iLayer >= 0 && iLayer < NUM_BackgroundLayer;
	if (!bValid) {
//		LOG->UserLog("Song file",
//					 sPath,
//					 "has a #BGCHANGES tag \"%s\" that is out of range.",
//					 sValueName.c_str());
	} else {
		std::vector<std::string> aBGChangeExpressions;
		split(sParam, ",", aBGChangeExpressions);

		for (auto& aBGChangeExpression : aBGChangeExpressions) {
			BackgroundChange change;
			if (LoadFromBGChangesString(change, aBGChangeExpression))
				out.AddBackgroundChange(iLayer, change);
		}
	}
}

void
SMLoader::ProcessInstrumentTracks(Song& out, const std::string& sParam)
{
	std::vector<std::string> vs1;
	split(sParam, ",", vs1);
	for (auto& s : vs1) {
		std::vector<std::string> vs2;
		split(s, "=", vs2);
		if (vs2.size() >= 2) {
			auto it = StringToInstrumentTrack(vs2[0]);
			if (it != InstrumentTrack_Invalid)
				out.m_sInstrumentTrackFile[it] = vs2[1];
		}
	}
}

void
SMLoader::ParseBPMs(std::vector<pair<float, float>>& out,
					const std::string& line,
					const int rowsPerBeat)
{
	std::vector<std::string> arrayBPMChangeExpressions;
	split(line, ",", arrayBPMChangeExpressions);

	for (auto& arrayBPMChangeExpression : arrayBPMChangeExpressions) {
		std::vector<std::string> arrayBPMChangeValues;
		split(arrayBPMChangeExpression, "=", arrayBPMChangeValues);
		if (arrayBPMChangeValues.size() != 2) {
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has an invalid #BPMs value \"%s\" (must have exactly "
//						 "one '='), ignored.",
//						 arrayBPMChangeExpression.c_str());
			continue;
		}

		const auto fBeat = RowToBeat(arrayBPMChangeValues[0], rowsPerBeat);
		const auto fNewBPM = StringToFloat(arrayBPMChangeValues[1]);
		if (fNewBPM == 0) {
//			LOG->UserLog(
//			  "Song file", this->GetSongTitle(), "has a zero BPM; ignored.");
//			continue;
		}

		out.emplace_back(std::make_pair(fBeat, fNewBPM));
	}
}

void
SMLoader::ParseStops(std::vector<pair<float, float>>& out,
					 const std::string& line,
					 const int rowsPerBeat)
{
	std::vector<std::string> arrayFreezeExpressions;
	split(line, ",", arrayFreezeExpressions);

	for (auto& arrayFreezeExpression : arrayFreezeExpressions) {
		std::vector<std::string> arrayFreezeValues;
		split(arrayFreezeExpression, "=", arrayFreezeValues);
		if (arrayFreezeValues.size() != 2) {
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has an invalid #STOPS value \"%s\" (must have "
//						 "exactly one '='), ignored.",
//						 arrayFreezeExpression.c_str());
			continue;
		}

		const auto fFreezeBeat = RowToBeat(arrayFreezeValues[0], rowsPerBeat);
		const auto fFreezeSeconds = StringToFloat(arrayFreezeValues[1]);
		if (fFreezeSeconds == 0) {
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has a zero-length stop; ignored.");
			continue;
		}

		out.emplace_back(std::make_pair(fFreezeBeat, fFreezeSeconds));
	}
}

// Utility function for sorting timing change data
namespace {
bool
compare_first(pair<float, float> a, pair<float, float> b)
{
	return a.first < b.first;
}
} // namespace

// Precondition: no BPM change or stop has 0 for its value (change.second).
//     (The ParseBPMs and ParseStops functions make sure of this.)
// Postcondition: all BPM changes, stops, and warps are added to the out
//     parameter, already sorted by beat.
void
SMLoader::ProcessBPMsAndStops(TimingData& out,
							  std::vector<pair<float, float>>& vBPMs,
							  std::vector<pair<float, float>>& vStops)
{
	std::vector<pair<float, float>>::const_iterator ibpm, ibpmend;
	std::vector<pair<float, float>>::const_iterator istop, istopend;

	// Current BPM (positive or negative)
	float bpm = 0;
	// Beat at which the previous timing change occurred
	float prevbeat = 0;
	// Start/end of current warp (-1 if not currently warping)
	float warpstart = -1;
	float warpend = -1;
	// BPM prior to current warp, to detect if it has changed
	float prewarpbpm = 0;
	// How far off we have gotten due to negative changes
	float timeofs = 0;

	// Sort BPM changes and stops by beat.  Order matters.
	// TODO: Make sorted lists a precondition rather than sorting them here.
	// The caller may know that the lists are sorted already (e.g. if
	// loaded from cache).
	stable_sort(vBPMs.begin(), vBPMs.end(), compare_first);
	stable_sort(vStops.begin(), vStops.end(), compare_first);

	// Convert stops that come before beat 0.  All these really do is affect
	// where the arrows are with respect to the music, i.e. the song offset.
	// Positive stops subtract from the offset, and negative add to it.
	istop = vStops.begin();
	istopend = vStops.end();
	for (/* istop */; istop != istopend && istop->first < 0; istop++) {
		out.m_fBeat0OffsetInSeconds -= istop->second;
	}

	// Get rid of BPM changes that come before beat 0.  Positive BPMs before
	// the chart don't really do anything, so we just ignore them.  Negative
	// BPMs cause unpredictable behavior, so ignore them as well and issue a
	// warning.
	ibpm = vBPMs.begin();
	ibpmend = vBPMs.end();
	for (/* ibpm */; ibpm != ibpmend && ibpm->first <= 0; ibpm++) {
		bpm = ibpm->second;
		if (bpm < 0 && ibpm->first < 0) {
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has a negative BPM prior to beat 0.  "
//						 "These cause problems; ignoring.");
		}
	}

	// It's beat 0.  Do you know where your BPMs are?
	if (bpm == 0) {
		// Nope.  Can we just use the next BPM value?
		if (ibpm == ibpmend) {
			// Nope.
			bpm = 60;
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "has no valid BPMs.  Defaulting to 60.");
		} else {
			// Yep.  Get the next BPM.
			ibpm++;
			bpm = ibpm->second;
//			LOG->UserLog("Song file",
//						 this->GetSongTitle(),
//						 "does not establish a BPM before beat 0.  "
//						 "Using the value from the next BPM change.");
		}
	}
	// We always want to have an initial BPM.  If we start out warping, this
	// BPM will be added later.  If we start with a regular BPM, add it now.
	if (bpm > 0 && bpm <= FAST_BPM_WARP) {
		out.AddSegment(BPMSegment(BeatToNoteRow(0), bpm));
	}

	// Iterate over all BPMs and stops in tandem
	while (ibpm != ibpmend || istop != istopend) {
		// Get the next change in order, with BPMs taking precedence
		// when they fall on the same beat.
		auto changeIsBpm =
		  istop == istopend || (ibpm != ibpmend && ibpm->first <= istop->first);
		const auto& change = changeIsBpm ? *ibpm : *istop;

		// Calculate the effects of time at the current BPM.  "Infinite"
		// BPMs (SM4 warps) imply that zero time passes, so skip this
		// step in that case.
		if (bpm <= FAST_BPM_WARP) {
			timeofs += (change.first - prevbeat) * 60 / bpm;

			// If we were in a warp and it finished during this
			// timeframe, create the warp segment.
			if (warpstart >= 0 && bpm > 0 && timeofs > 0) {
				// timeofs represents how far past the end we are
				warpend = change.first - (timeofs * bpm / 60);
				out.AddSegment(
				  WarpSegment(BeatToNoteRow(warpstart), warpend - warpstart));

				// If the BPM changed during the warp, put that
				// change at the beginning of the warp.
				if (bpm != prewarpbpm) {
					out.AddSegment(BPMSegment(BeatToNoteRow(warpstart), bpm));
				}
				// No longer warping
				warpstart = -1;
			}
		}

		// Save the current beat for the next round of calculations
		prevbeat = change.first;

		// Now handle the timing changes themselves
		if (changeIsBpm) {
			// Does this BPM change start a new warp?
			if (warpstart < 0 &&
				(change.second < 0 || change.second > FAST_BPM_WARP)) {
				// Yes.
				warpstart = change.first;
				prewarpbpm = bpm;
				timeofs = 0;
			} else if (warpstart < 0) {
				// No, and we aren't currently warping either.
				// Just a normal BPM change.
				out.AddSegment(
				  BPMSegment(BeatToNoteRow(change.first), change.second));
			}
			bpm = change.second;
			ibpm++;
		} else {
			// Does this stop start a new warp?
			if (warpstart < 0 && change.second < 0) {
				// Yes.
				warpstart = change.first;
				prewarpbpm = bpm;
				timeofs = change.second;
			} else if (warpstart < 0) {
				// No, and we aren't currently warping either.
				// Just a normal stop.
				out.AddSegment(
				  StopSegment(BeatToNoteRow(change.first), change.second));
			} else {
				// We're warping already.  Stops affect the time
				// offset directly.
				timeofs += change.second;

				// If a stop overcompensates for the time
				// deficit, the warp ends and we stop for the
				// amount it goes over.
				if (change.second > 0 && timeofs > 0) {
					warpend = change.first;
					out.AddSegment(WarpSegment(BeatToNoteRow(warpstart),
											   warpend - warpstart));
					out.AddSegment(
					  StopSegment(BeatToNoteRow(change.first), timeofs));

					// Now, are we still warping because of
					// the BPM value?
					if (bpm < 0 || bpm > FAST_BPM_WARP) {
						// Yep.
						warpstart = change.first;
						// prewarpbpm remains the same
						timeofs = 0;
					} else {
						// Nope, warp is done.  Add any
						// BPM change that happened in
						// the meantime.
						if (bpm != prewarpbpm) {
							out.AddSegment(
							  BPMSegment(BeatToNoteRow(warpstart), bpm));
						}
						warpstart = -1;
					}
				}
			}
			istop++;
		}
	}

	// If we are still warping, we now have to consider the time remaining
	// after the last timing change.
	if (warpstart >= 0) {
		// Will this warp ever end?
		if (bpm < 0 || bpm > FAST_BPM_WARP) {
			// No, so it ends the entire chart immediately.
			// XXX There must be a less hacky and more accurate way
			// to do this.
			warpend = 99999999.0f;
		} else {
			// Yes.  Figure out when it will end.
			warpend = prevbeat - (timeofs * bpm / 60);
		}
		out.AddSegment(
		  WarpSegment(BeatToNoteRow(warpstart), warpend - warpstart));

		// As usual, record any BPM change that happened during the warp
		if (bpm != prewarpbpm) {
			out.AddSegment(BPMSegment(BeatToNoteRow(warpstart), bpm));
		}
	}
}
void
SMLoader::ProcessDelays(TimingData& out,
						const std::string& line,
						const int rowsPerBeat)
{
	ProcessDelays(out, line, this->GetSongTitle(), rowsPerBeat);
}
void
SMLoader::ProcessDelays(TimingData& out,
						const std::string& line,
						const string& songname,
						const int rowsPerBeat)
{
	std::vector<std::string> arrayDelayExpressions;
	split(line, ",", arrayDelayExpressions);

	for (auto& arrayDelayExpression : arrayDelayExpressions) {
		std::vector<std::string> arrayDelayValues;
		split(arrayDelayExpression, "=", arrayDelayValues);
		if (arrayDelayValues.size() != 2) {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an invalid #DELAYS value \"%s\" (must have "
//						 "exactly one '='), ignored.",
//						 arrayDelayExpression.c_str());
			continue;
		}
		const auto fFreezeBeat = RowToBeat(arrayDelayValues[0], rowsPerBeat);
		const auto fFreezeSeconds = StringToFloat(arrayDelayValues[1]);
		// LOG->Trace( "Adding a delay segment: beat: %f, seconds = %f",
		// new_seg.m_fStartBeat, new_seg.m_fStopSeconds );

		if (fFreezeSeconds > 0.0f)
			out.AddSegment(
			  DelaySegment(BeatToNoteRow(fFreezeBeat), fFreezeSeconds));
		else {


//			LOG->UserLog("Song file",
//						 songname,
//						 "has an invalid delay at beat %f, length %f.",
//						 fFreezeBeat,
//						 fFreezeSeconds);
		}
	}
}

void
SMLoader::ProcessTimeSignatures(TimingData& out,
								const std::string& line,
								const int rowsPerBeat)
{
	ProcessTimeSignatures(out, line, this->GetSongTitle(), rowsPerBeat);
}
void
SMLoader::ProcessTimeSignatures(TimingData& out,
								const std::string& line,
								const string& songname,
								const int rowsPerBeat)
{
	std::vector<std::string> vs1;
	split(line, ",", vs1);

	for (auto& s1 : vs1) {
		std::vector<std::string> vs2;
		split(s1, "=", vs2);

		if (vs2.size() < 3) {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an invalid time signature change with %i values.",
//						 static_cast<int>(vs2.size()));
			continue;
		}

		const auto fBeat = RowToBeat(vs2[0], rowsPerBeat);
		const auto iNumerator = StringToInt(vs2[1]);
		const auto iDenominator = StringToInt(vs2[2]);

		if (fBeat < 0) {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an invalid time signature change with beat %f.",
//						 fBeat);
			continue;
		}

		if (iNumerator < 1) {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an invalid time signature change with beat %f, "
//						 "iNumerator %i.",
//						 fBeat,
//						 iNumerator);
			continue;
		}

		if (iDenominator < 1) {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an invalid time signature change with beat %f, "
//						 "iDenominator %i.",
//						 fBeat,
//						 iDenominator);
			continue;
		}

		out.AddSegment(
		  TimeSignatureSegment(BeatToNoteRow(fBeat), iNumerator, iDenominator));
	}
}

void
SMLoader::ProcessTickcounts(TimingData& out,
							const std::string& line,
							const int rowsPerBeat)
{
	ProcessTickcounts(out, line, this->GetSongTitle(), rowsPerBeat);
}
void
SMLoader::ProcessTickcounts(TimingData& out,
							const std::string& line,
							const string& songname,
							const int rowsPerBeat)
{
	std::vector<std::string> arrayTickcountExpressions;
	split(line, ",", arrayTickcountExpressions);

	for (auto& arrayTickcountExpression : arrayTickcountExpressions) {
		std::vector<std::string> arrayTickcountValues;
		split(arrayTickcountExpression, "=", arrayTickcountValues);
		if (arrayTickcountValues.size() != 2) {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an invalid #TICKCOUNTS value \"%s\" (must have "
//						 "exactly one '='), ignored.",
//						 arrayTickcountExpression.c_str());
			continue;
		}

		const auto fTickcountBeat =
		  RowToBeat(arrayTickcountValues[0], rowsPerBeat);
		auto iTicks =
		  std::clamp(atoi(arrayTickcountValues[1].c_str()), 0, ROWS_PER_BEAT);

		out.AddSegment(TickcountSegment(BeatToNoteRow(fTickcountBeat), iTicks));
	}
}

void
SMLoader::ProcessSpeeds(TimingData& out,
						const std::string& line,
						const int rowsPerBeat)
{
	ProcessSpeeds(out, line, this->GetSongTitle(), rowsPerBeat);
}
void
SMLoader::ProcessSpeeds(TimingData& out,
						const std::string& line,
						const string& songname,
						const int rowsPerBeat)
{
	std::vector<std::string> vs1;
	split(line, ",", vs1);

	for (auto& s1 : vs1) {
		std::vector<std::string> vs2;
		split(s1, "=", vs2);

		if (vs2[0].c_str() == nullptr &&
			vs2.size() == 2) // First one always seems to have 2.
		{
			vs2.emplace_back("0");
		}

		if (vs2.size() == 3) // use beats by default.
		{
			vs2.emplace_back("0");
		}

		if (vs2.size() < 4) {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an speed change with %i values.",
//						 static_cast<int>(vs2.size()));
			continue;
		}

		const auto fBeat = RowToBeat(vs2[0], rowsPerBeat);
		const auto fRatio = StringToFloat(vs2[1]);
		const auto fDelay = StringToFloat(vs2[2]);

		// XXX: ugly...
		auto iUnit = StringToInt(vs2[3]);
		auto unit =
		  (iUnit == 0) ? SpeedSegment::UNIT_BEATS : SpeedSegment::UNIT_SECONDS;

		if (fBeat < 0) {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an speed change with beat %f.",
//						 fBeat);
			continue;
		}

		if (fDelay < 0) {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an speed change with beat %f, length %f.",
//						 fBeat,
//						 fDelay);
			continue;
		}

		out.AddSegment(
		  SpeedSegment(BeatToNoteRow(fBeat), fRatio, fDelay, unit));
	}
}

void
SMLoader::ProcessFakes(TimingData& out,
					   const std::string& line,
					   const int rowsPerBeat)
{
	ProcessFakes(out, line, this->GetSongTitle(), rowsPerBeat);
}
void
SMLoader::ProcessFakes(TimingData& out,
					   const std::string& line,
					   const string& songname,
					   const int rowsPerBeat)
{
	std::vector<std::string> arrayFakeExpressions;
	split(line, ",", arrayFakeExpressions);

	for (auto& arrayFakeExpression : arrayFakeExpressions) {
		std::vector<std::string> arrayFakeValues;
		split(arrayFakeExpression, "=", arrayFakeValues);
		if (arrayFakeValues.size() != 2) {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an invalid #FAKES value \"%s\" (must have "
//						 "exactly one '='), ignored.",
//						 arrayFakeExpression.c_str());
			continue;
		}

		const auto fBeat = RowToBeat(arrayFakeValues[0], rowsPerBeat);
		const auto fSkippedBeats = StringToFloat(arrayFakeValues[1]);

		if (fSkippedBeats > 0)
			out.AddSegment(FakeSegment(BeatToNoteRow(fBeat), fSkippedBeats));
		else {
//			LOG->UserLog("Song file",
//						 songname,
//						 "has an invalid Fake at beat %f, beats to skip %f.",
//						 fBeat,
//						 fSkippedBeats);
		}
	}
}

bool
SMLoader::LoadFromBGChangesString(BackgroundChange& change,
								  const std::string& sBGChangeExpression)
{
	std::vector<std::string> aBGChangeValues;
	split(sBGChangeExpression, "=", aBGChangeValues, false);

	aBGChangeValues.resize(
	  std::min(static_cast<int>(aBGChangeValues.size()), 11));

	std::string aaa;
	switch (aBGChangeValues.size()) {
		case 11:
			change.m_def.m_sColor2 = aBGChangeValues[10];
			aaa = change.m_def.m_sColor2;
			s_replace(aaa, "^", ",");
			change.m_def.m_sColor2 = aaa;
			change.m_def.m_sColor2 =
			  RageColor::NormalizeColorString(change.m_def.m_sColor2);
			// fall through
		case 10:
			change.m_def.m_sColor1 = aBGChangeValues[9];
			aaa = change.m_def.m_sColor1;
			s_replace(aaa, "^", ",");
			change.m_def.m_sColor1 = aaa;
			change.m_def.m_sColor1 =
			  RageColor::NormalizeColorString(change.m_def.m_sColor1);
			// fall through
		case 9:
			change.m_sTransition = aBGChangeValues[8];
			// fall through
		case 8: {
			auto tmp = make_lower(aBGChangeValues[7]);

			if ((tmp.find(".ini") != string::npos ||
				 tmp.find(".xml") != string::npos)) {
				return false;
			}
			change.m_def.m_sFile2 = aBGChangeValues[7];
			// fall through
		}
		case 7:
			change.m_def.m_sEffect = aBGChangeValues[6];
			// fall through
		case 6:
			// param 7 overrides this.
			// Backward compatibility:
			if (change.m_def.m_sEffect.empty()) {
				auto bLoop = StringToInt(aBGChangeValues[5]) != 0;
				if (!bLoop)
					change.m_def.m_sEffect = SBE_StretchNoLoop;
			}
			// fall through
		case 5:
			// param 7 overrides this.
			// Backward compatibility:
			if (change.m_def.m_sEffect.empty()) {
				auto bRewindMovie = StringToInt(aBGChangeValues[4]) != 0;
				if (bRewindMovie)
					change.m_def.m_sEffect = SBE_StretchRewind;
			}
			// fall through
		case 4:
			// param 9 overrides this.
			// Backward compatibility:
			if (change.m_sTransition.empty())
				change.m_sTransition =
				  (StringToInt(aBGChangeValues[3]) != 0) ? "CrossFade" : "";
			// fall through
		case 3:
			change.m_fRate = StringToFloat(aBGChangeValues[2]);
			// fall through
		case 2: {
			auto tmp = make_lower(aBGChangeValues[1]);
			if ((tmp.find(".ini") != string::npos ||
				 tmp.find(".xml") != string::npos)) {
				return false;
			}
			change.m_def.m_sFile1 = aBGChangeValues[1];
			// fall through
		}
		case 1:
			change.m_fStartBeat = StringToFloat(aBGChangeValues[0]);
			// fall through
	}

	return aBGChangeValues.size() >= 2;
}

bool
SMLoader::LoadNoteDataFromSimfile(const std::string& path, Steps& out)
{
	MsdFile msd;
	if (!msd.ReadFile(path, true)) // unescape
	{
//		LOG->UserLog(
//		  "Song file", path, "couldn't be opened: %s", msd.GetError().c_str());
		return false;
	}
	for (unsigned i = 0; i < msd.GetNumValues(); i++) {
		int iNumParams = msd.GetNumParams(i);
		const auto& sParams = msd.GetValue(i);
		std::string sValueName = make_upper(sParams[0]);

		// The only tag we care about is the #NOTES tag.
		if (sValueName == "NOTES" || sValueName == "NOTES2") {
			if (iNumParams < 7) {
//				LOG->UserLog(
//				  "Song file",
//				  path,
//				  "has %d fields in a #NOTES tag, but should have at least 7.",
//				  iNumParams);
				continue;
			}

			std::string stepsType = sParams[1];
			std::string description = sParams[2];
			std::string difficulty = sParams[3];

			// HACK?: If this is a .edit fudge the edit difficulty
			if (CompareNoCase(tail(path, 5), ".edit") == 0)
				difficulty = "edit";

			Trim(stepsType);
			Trim(description);
			Trim(difficulty);
			// Remember our old versions.
			if (CompareNoCase(difficulty, "smaniac") == 0) {
				difficulty = "Challenge";
			}

			/* Handle hacks that originated back when StepMania didn't have
			 * Difficulty_Challenge. TODO: Remove the need for said hacks. */
			if (CompareNoCase(difficulty, "hard") == 0) {
				/* HACK: Both SMANIAC and CHALLENGE used to be Difficulty_Hard.
				 * They were differentiated via aspecial description.
				 * Account for the rogue charts that do this. */
				// HACK: SMANIAC used to be Difficulty_Hard with a special
				// description.
				if (CompareNoCase(description, "smaniac") == 0 ||
					CompareNoCase(description, "challenge") == 0)
					difficulty = "Challenge";
			}

			if (!(out.m_StepsType == GAMEMAN->StringToStepsType(stepsType) &&
				  out.GetDescription() == description &&
				  (out.GetDifficulty() == StringToDifficulty(difficulty) ||
				   out.GetDifficulty() ==
					 OldStyleStringToDifficulty(difficulty)))) {
				if (out.IsDupeDiff()) {
					// for duplicate difficulties, check by chartkey.
					// not aware of a case where chartkey is not filled here.
					std::string noteData = sParams[6];
					Trim(noteData);
					Steps tmp(out.m_pSong);
					tmp.m_Timing = out.m_Timing;
					tmp.m_StepsType = out.m_StepsType;
					tmp.SetSMNoteData(noteData);
					auto tnd = tmp.GetNoteData();
					tnd.LogNonEmptyRows(&tmp.m_Timing);

					auto ck = tmp.GenerateChartKey(tnd, tmp.GetTimingData());
					if (ck != out.GetChartKey())
						continue;
				} else
					continue;
			}

			std::string noteData = sParams[6];
			Trim(noteData);
			out.SetSMNoteData(noteData);
			out.TidyUpData();
			return true;
		}
	}
	return false;
}

bool
SMLoader::LoadFromSimfile(const std::string& sPath, Song& out, bool bFromCache)
{
	// LOG->Trace( "Song::LoadFromSMFile(%s)", sPath.c_str() );

	MsdFile msd;
	if (!msd.ReadFile(sPath, true)) // unescape
	{
//		LOG->UserLog(
//		  "Song file", sPath, "couldn't be opened: %s", msd.GetError().c_str());
		return false;
	}

	out.m_SongTiming.m_sFile = sPath;
	out.m_sSongFileName = sPath;

	SMSongTagInfo reused_song_info(&*this, &out, sPath);

	for (unsigned i = 0; i < msd.GetNumValues(); i++) {
		int iNumParams = msd.GetNumParams(i);
		const auto& sParams = msd.GetValue(i);
		std::string sValueName = make_upper(sParams[0]);

		reused_song_info.params = &sParams;
		auto handler = sm_parser_helper.song_tag_handlers.find(sValueName);
		if (handler != sm_parser_helper.song_tag_handlers.end()) {
			/* Don't use GetMainAndSubTitlesFromFullTitle; that's only for
			 * heuristically splitting other formats that *don't* natively
			 * support #SUBTITLE. */
			handler->second(reused_song_info);
		} else if (head(sValueName, 9) == "BGCHANGES") {
			SMSetBGChanges(reused_song_info);
		} else if (sValueName == "NOTES" || sValueName == "NOTES2") {
			if (iNumParams < 7) {
//				LOG->UserLog(
//				  "Song file",
//				  sPath,
//				  "has %d fields in a #NOTES tag, but should have at least 7.",
//				  iNumParams);
				continue;
			}

			auto pNewNotes = out.CreateSteps();
			LoadFromTokens(sParams[1],
						   sParams[2],
						   sParams[3],
						   sParams[4],
						   /*sParams[5],*/ // radar values
						   sParams[6],
						   *pNewNotes);

			pNewNotes->SetFilename(sPath);
			out.AddSteps(pNewNotes);
		} else {
//			LOG->UserLog("Song file",
//						 sPath,
//						 "has an unexpected value named \"%s\".",
//						 sValueName.c_str());
		}
	}

	// Turn negative time changes into warps
	ProcessBPMsAndStops(
	  out.m_SongTiming, reused_song_info.BPMChanges, reused_song_info.Stops);

	TidyUpData(out, bFromCache);
	return true;
}

void
SMLoader::GetApplicableFiles(const std::string& sPath, std::vector<std::string>& out)
{
	FILEMAN->GetDirListing(
	  sPath + std::string("*" + this->GetFileExtension()), out, ONLY_FILE);
}

void
SMLoader::TidyUpData(Song& song, bool bFromCache)
{
	/*
	 * Hack: if the song has any changes at all (so it won't use a random BGA)
	 * and doesn't end with "-nosongbg-", add a song background BGC.  Remove
	 * "-nosongbg-" if it exists.
	 *
	 * This way, songs that were created earlier, when we added the song BG
	 * at the end by default, will still behave as expected; all new songs will
	 * have to add an explicit song BG tag if they want it.  This is really a
	 * formatting hack only; nothing outside of SMLoader ever sees "-nosongbg-".
	 */
	auto& bg = song.GetBackgroundChanges(BACKGROUND_LAYER_1);
	if (!bg.empty()) {
		/* BGChanges have been sorted. On the odd chance that a BGChange exists
		 * with a very high beat, search the whole list. */
		auto bHasNoSongBgTag = false;

		for (unsigned i = 0; !bHasNoSongBgTag && i < bg.size(); ++i) {
			if (!CompareNoCase(bg[i].m_def.m_sFile1, NO_SONG_BG_FILE)) {
				bg.erase(bg.begin() + i);
				bHasNoSongBgTag = true;
			}
		}

		// If there's no -nosongbg- tag, add the song BG.
		if (!bHasNoSongBgTag)
			do {
				/* If we're loading cache, -nosongbg- should always be in there.
				 * We must not call IsAFile(song.GetBackgroundPath()) when
				 * loading cache. */
				if (bFromCache)
					break;

				auto lastBeat = song.GetLastBeat();
				/* If BGChanges already exist after the last beat, don't add the
				 * background in the middle. */
				if (!bg.empty() && bg.back().m_fStartBeat - 0.0001f >= lastBeat)
					break;

				// If the last BGA is already the song BGA, don't add a
				// duplicate.
				if (!bg.empty() && !ssicmp(bg.back().m_def.m_sFile1.c_str(),
										   song.m_sBackgroundFile.c_str()))
					break;

				if (!IsAFile(song.GetBackgroundPath()))
					break;

				bg.emplace_back(
				  BackgroundChange(lastBeat, song.m_sBackgroundFile));
			} while (0);
	}
	if (bFromCache) {
		song.TidyUpData(bFromCache, true);
	}
}
