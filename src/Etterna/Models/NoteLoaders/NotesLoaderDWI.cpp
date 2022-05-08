#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameInput.h"
#include "Etterna/FileTypes/MsdFile.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "NotesLoader.h"
#include "NotesLoaderDWI.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Utils/RageUtil_CharConversions.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"

#include <map>

Difficulty
DwiCompatibleStringToDifficulty(const std::string& sDC);

/** @brief The different types of core DWI arrows and pads. */
enum DanceNotes
{
	DANCE_NOTE_NONE = 0,
	DANCE_NOTE_PAD1_LEFT,
	DANCE_NOTE_PAD1_UPLEFT,
	DANCE_NOTE_PAD1_DOWN,
	DANCE_NOTE_PAD1_UP,
	DANCE_NOTE_PAD1_UPRIGHT,
	DANCE_NOTE_PAD1_RIGHT,
	DANCE_NOTE_PAD2_LEFT,
	DANCE_NOTE_PAD2_UPLEFT,
	DANCE_NOTE_PAD2_DOWN,
	DANCE_NOTE_PAD2_UP,
	DANCE_NOTE_PAD2_UPRIGHT,
	DANCE_NOTE_PAD2_RIGHT
};

/**
 * @brief Turn the individual character to the proper note.
 * @param c The character in question.
 * @param i The player.
 * @param note1Out The first result based on the character.
 * @param note2Out The second result based on the character.
 * @param sPath the path to the file.
 */
static void
DWIcharToNote(char c,
			  GameController i,
			  int& note1Out,
			  int& note2Out,
			  const std::string& sPath)
{
	switch (c) {
		case '0':
			note1Out = DANCE_NOTE_NONE;
			note2Out = DANCE_NOTE_NONE;
			break;
		case '1':
			note1Out = DANCE_NOTE_PAD1_DOWN;
			note2Out = DANCE_NOTE_PAD1_LEFT;
			break;
		case '2':
			note1Out = DANCE_NOTE_PAD1_DOWN;
			note2Out = DANCE_NOTE_NONE;
			break;
		case '3':
			note1Out = DANCE_NOTE_PAD1_DOWN;
			note2Out = DANCE_NOTE_PAD1_RIGHT;
			break;
		case '4':
			note1Out = DANCE_NOTE_PAD1_LEFT;
			note2Out = DANCE_NOTE_NONE;
			break;
		case '5':
			note1Out = DANCE_NOTE_NONE;
			note2Out = DANCE_NOTE_NONE;
			break;
		case '6':
			note1Out = DANCE_NOTE_PAD1_RIGHT;
			note2Out = DANCE_NOTE_NONE;
			break;
		case '7':
			note1Out = DANCE_NOTE_PAD1_UP;
			note2Out = DANCE_NOTE_PAD1_LEFT;
			break;
		case '8':
			note1Out = DANCE_NOTE_PAD1_UP;
			note2Out = DANCE_NOTE_NONE;
			break;
		case '9':
			note1Out = DANCE_NOTE_PAD1_UP;
			note2Out = DANCE_NOTE_PAD1_RIGHT;
			break;
		case 'A':
			note1Out = DANCE_NOTE_PAD1_UP;
			note2Out = DANCE_NOTE_PAD1_DOWN;
			break;
		case 'B':
			note1Out = DANCE_NOTE_PAD1_LEFT;
			note2Out = DANCE_NOTE_PAD1_RIGHT;
			break;
		case 'C':
			note1Out = DANCE_NOTE_PAD1_UPLEFT;
			note2Out = DANCE_NOTE_NONE;
			break;
		case 'D':
			note1Out = DANCE_NOTE_PAD1_UPRIGHT;
			note2Out = DANCE_NOTE_NONE;
			break;
		case 'E':
			note1Out = DANCE_NOTE_PAD1_LEFT;
			note2Out = DANCE_NOTE_PAD1_UPLEFT;
			break;
		case 'F':
			note1Out = DANCE_NOTE_PAD1_UPLEFT;
			note2Out = DANCE_NOTE_PAD1_DOWN;
			break;
		case 'G':
			note1Out = DANCE_NOTE_PAD1_UPLEFT;
			note2Out = DANCE_NOTE_PAD1_UP;
			break;
		case 'H':
			note1Out = DANCE_NOTE_PAD1_UPLEFT;
			note2Out = DANCE_NOTE_PAD1_RIGHT;
			break;
		case 'I':
			note1Out = DANCE_NOTE_PAD1_LEFT;
			note2Out = DANCE_NOTE_PAD1_UPRIGHT;
			break;
		case 'J':
			note1Out = DANCE_NOTE_PAD1_DOWN;
			note2Out = DANCE_NOTE_PAD1_UPRIGHT;
			break;
		case 'K':
			note1Out = DANCE_NOTE_PAD1_UP;
			note2Out = DANCE_NOTE_PAD1_UPRIGHT;
			break;
		case 'L':
			note1Out = DANCE_NOTE_PAD1_UPRIGHT;
			note2Out = DANCE_NOTE_PAD1_RIGHT;
			break;
		case 'M':
			note1Out = DANCE_NOTE_PAD1_UPLEFT;
			note2Out = DANCE_NOTE_PAD1_UPRIGHT;
			break;
		default:
			/*LOG->UserLog(
			  "Song file", sPath, "has an invalid DWI note character '%c'.", c);*/
			note1Out = DANCE_NOTE_NONE;
			note2Out = DANCE_NOTE_NONE;
			break;
	}

	switch (i) {
		case GameController_1:
			break;
		case GameController_2:
			if (note1Out != DANCE_NOTE_NONE)
				note1Out += 6;
			if (note2Out != DANCE_NOTE_NONE)
				note2Out += 6;
			break;
		default:
			FAIL_M(ssprintf("Invalid GameController: %i", i));
	}
}

/**
 * @brief Determine the note column[s] to place notes.
 * @param c The character in question.
 * @param i The player.
 * @param col1Out The first result based on the character.
 * @param col2Out The second result based on the character.
 * @param sPath the path to the file.
 * @param mapDanceNoteToColumn a map to pass to keep track of column info.
 */
static void
DWIcharToNoteCol(char c,
				 GameController i,
				 int& col1Out,
				 int& col2Out,
				 const std::string& sPath,
				 std::map<int, int>& mapDanceNoteToColumn)
{
	int note1, note2;
	DWIcharToNote(c, i, note1, note2, sPath);

	if (note1 != DANCE_NOTE_NONE)
		col1Out = mapDanceNoteToColumn[note1];
	else
		col1Out = -1;

	if (note2 != DANCE_NOTE_NONE)
		col2Out = mapDanceNoteToColumn[note2];
	else
		col2Out = -1;
}

/**
 * @brief Determine if the note in question is a 192nd note.
 *
 * DWI used to use <...> to indicate 1/192nd notes; at some
 * point, <...> was changed to indicate jumps, and `' was used for
 * 1/192nds.  So, we have to do a check to figure out what it really
 * means.  If it contains 0s, it's most likely 192nds; otherwise,
 * it's most likely a jump.  Search for a 0 before the next >:
 * @param sStepData the step data.
 * @param pos the position of the step data.
 * @return true if it's a 192nd note, false otherwise.
 */
static bool
Is192(const std::string& sStepData, size_t pos)
{
	while (pos < sStepData.size()) {
		if (sStepData[pos] == '>')
			return false;
		if (sStepData[pos] == '0')
			return true;
		++pos;
	}

	return false;
}
/** @brief All DWI files use 4 beats per measure. */
const int BEATS_PER_MEASURE = 4;

/* We prefer the normal names; recognize a number of others, too. (They'll get
 * normalized when written to SMs, etc.) */
Difficulty
DwiCompatibleStringToDifficulty(const std::string& sDC)
{
	std::string s2 = make_lower(sDC);
	if (s2 == "beginner")
		return Difficulty_Beginner;
	if (s2 == "easy")
		return Difficulty_Easy;
	if (s2 == "basic")
		return Difficulty_Easy;
	else if (s2 == "light")
		return Difficulty_Easy;
	else if (s2 == "medium")
		return Difficulty_Medium;
	else if (s2 == "another")
		return Difficulty_Medium;
	else if (s2 == "trick")
		return Difficulty_Medium;
	else if (s2 == "standard")
		return Difficulty_Medium;
	else if (s2 == "difficult")
		return Difficulty_Medium;
	else if (s2 == "hard")
		return Difficulty_Hard;
	else if (s2 == "ssr")
		return Difficulty_Hard;
	else if (s2 == "maniac")
		return Difficulty_Hard;
	else if (s2 == "heavy")
		return Difficulty_Hard;
	else if (s2 == "smaniac")
		return Difficulty_Challenge;
	else if (s2 == "challenge")
		return Difficulty_Challenge;
	else if (s2 == "expert")
		return Difficulty_Challenge;
	else if (s2 == "oni")
		return Difficulty_Challenge;
	else if (s2 == "edit")
		return Difficulty_Edit;
	else
		return Difficulty_Invalid;
}

static StepsType
GetTypeFromMode(const std::string& mode)
{
	if (mode == "SINGLE")
		return StepsType_dance_single;
	if (mode == "DOUBLE")
		return StepsType_dance_double;
	else if (mode == "SOLO")
		return StepsType_dance_solo;
	return StepsType_Invalid; // just in case.
}

static NoteData
ParseNoteData(std::string& step1,
			  std::string& step2,
			  Steps& out,
			  const std::string& path)
{
	std::map<int, int> g_mapDanceNoteToNoteDataColumn;

	switch (out.m_StepsType) {
		case StepsType_dance_single:
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 1;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 2;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 3;
			break;
		case StepsType_dance_double:
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 1;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 2;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 3;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_LEFT] = 4;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_DOWN] = 5;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_UP] = 6;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_RIGHT] = 7;
			break;
		case StepsType_dance_solo:
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UPLEFT] = 1;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 2;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 3;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UPRIGHT] = 4;
			g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 5;
			break;
			DEFAULT_FAIL(out.m_StepsType);
	}

	NoteData newNoteData;
	newNoteData.SetNumTracks(g_mapDanceNoteToNoteDataColumn.size());

	for (int pad = 0; pad < 2; pad++) // foreach pad
	{
		std::string sStepData;
		switch (pad) {
			case 0:
				sStepData = step1;
				break;
			case 1:
				if (step2 == "") // no data
					continue;	 // skip
				sStepData = step2;
				break;
				DEFAULT_FAIL(pad);
		}

		s_replace(sStepData, "\n", "");
		s_replace(sStepData, "\r", "");
		s_replace(sStepData, "\t", "");
		s_replace(sStepData, " ", "");

		double fCurrentBeat = 0;
		double fCurrentIncrementer = 1.0 / 8 * BEATS_PER_MEASURE;

		for (size_t i = 0; i < sStepData.size();) {
			char c = sStepData[i++];
			switch (c) {
					// begins a series
				case '(':
					fCurrentIncrementer = 1.0 / 16 * BEATS_PER_MEASURE;
					break;
				case '[':
					fCurrentIncrementer = 1.0 / 24 * BEATS_PER_MEASURE;
					break;
				case '{':
					fCurrentIncrementer = 1.0 / 64 * BEATS_PER_MEASURE;
					break;
				case '`':
					fCurrentIncrementer = 1.0 / 192 * BEATS_PER_MEASURE;
					break;

					// ends a series
				case ')':
				case ']':
				case '}':
				case '\'':
				case '>':
					fCurrentIncrementer = 1.0 / 8 * BEATS_PER_MEASURE;
					break;

				default
				  : // this is a note character
				{
					if (c == '!') {
						/*LOG->UserLog("Song file",
									 path,
									 "has an unexpected character: '!'.");*/
						continue;
					}

					bool jump = false;
					if (c == '<') {
						/* Arr.  Is this a jump or a 1/192 marker? */
						if (Is192(sStepData, i)) {
							fCurrentIncrementer = 1.0 / 192 * BEATS_PER_MEASURE;
							break;
						}

						/* It's a jump.
						 * We need to keep reading notes until we hit a >. */
						jump = true;
						i++;
					}

					const int iIndex =
					  BeatToNoteRow(static_cast<float>(fCurrentBeat));
					i--;
					do {
						c = sStepData[i++];

						if (jump && c == '>')
							break;

						int iCol1, iCol2;
						DWIcharToNoteCol(c,
										 (GameController)pad,
										 iCol1,
										 iCol2,
										 path,
										 g_mapDanceNoteToNoteDataColumn);

						if (iCol1 != -1)
							newNoteData.SetTapNote(
							  iCol1, iIndex, TAP_ORIGINAL_TAP);
						if (iCol2 != -1)
							newNoteData.SetTapNote(
							  iCol2, iIndex, TAP_ORIGINAL_TAP);

						if (i >= sStepData.length()) {
							break;
							// we ran out of data
							// while looking for the ending > mark
						}

						if (sStepData[i] == '!') {
							i++;
							const char holdChar = sStepData[i++];

							DWIcharToNoteCol(holdChar,
											 (GameController)pad,
											 iCol1,
											 iCol2,
											 path,
											 g_mapDanceNoteToNoteDataColumn);

							if (iCol1 != -1)
								newNoteData.SetTapNote(
								  iCol1, iIndex, TAP_ORIGINAL_HOLD_HEAD);
							if (iCol2 != -1)
								newNoteData.SetTapNote(
								  iCol2, iIndex, TAP_ORIGINAL_HOLD_HEAD);
						}
					} while (jump);
					fCurrentBeat += fCurrentIncrementer;
				} break;
			}
		}
	}

	/* Fill in iDuration. */
	for (int t = 0; t < newNoteData.GetNumTracks(); ++t) {
		FOREACH_NONEMPTY_ROW_IN_TRACK(newNoteData, t, iHeadRow)
		{
			TapNote tn = newNoteData.GetTapNote(t, iHeadRow);
			if (tn.type != TapNoteType_HoldHead)
				continue;

			int iTailRow = iHeadRow;
			bool bFound = false;
			while (!bFound &&
				   newNoteData.GetNextTapNoteRowForTrack(t, iTailRow)) {
				const TapNote& TailTap = newNoteData.GetTapNote(t, iTailRow);
				if (TailTap.type == TapNoteType_Empty)
					continue;

				newNoteData.SetTapNote(t, iTailRow, TAP_EMPTY);
				tn.iDuration = iTailRow - iHeadRow;
				newNoteData.SetTapNote(t, iHeadRow, tn);
				bFound = true;
			}

			if (!bFound) {
				/* The hold was never closed.  */
				/*LOG->UserLog(
				  "Song file",
				  path,
				  "failed to close a hold note in \"%s\" on track %i",
				  DifficultyToString(out.GetDifficulty()).c_str(),
				  t);*/

				newNoteData.SetTapNote(t, iHeadRow, TAP_EMPTY);
			}
		}
	}

	ASSERT(newNoteData.GetNumTracks() > 0);
	return newNoteData;
}

/**
 * @brief Look through the notes tag to extract the data.
 * @param sMode the steps type.
 * @param sDescription the difficulty.
 * @param sNumFeet the meter.
 * @param sStepData1 the guaranteed step data.
 * @param sStepData2 used if sMode is double or couple.
 * @param out the step data.
 * @param sPath the path to the file.
 * @return the success or failure of the operation.
 */
static bool
LoadFromDWITokens(std::string sMode,
				  std::string sDescription,
				  std::string sNumFeet,
				  std::string sStepData1,
				  std::string sStepData2,
				  Steps& out,
				  const std::string& sPath)
{
	out.m_StepsType = GetTypeFromMode(sMode);
	if (out.m_StepsType == StepsType_Invalid)
		return false;

	// if the meter is empty, force it to 1.
	if (sNumFeet.empty())
		sNumFeet = "1";

	out.SetMeter(StringToInt(sNumFeet));

	out.SetDifficulty(DwiCompatibleStringToDifficulty(sDescription));

	out.SetNoteData(ParseNoteData(sStepData1, sStepData2, out, sPath));

	out.TidyUpData();

	out.SetSavedToDisk(
	  true); // we're loading from disk, so this is by definintion already saved
	return true;
}

/**
 * @brief Turn the DWI style timestamp into a compatible time for our system.
 *
 * This value can be in either "HH:MM:SS.sssss", "MM:SS.sssss", "SSS.sssss"
 * or milliseconds.
 * @param arg1 Either hours, minutes, or seconds, depending on other args.
 * @param arg2 Either minutes or seconds, depending on other args.
 * @param arg3 Seconds if not empty.
 * @return the proper timestamp.
 */
static float
ParseBrokenDWITimestamp(const std::string& arg1,
						const std::string& arg2,
						const std::string& arg3)
{
	if (arg1.empty())
		return 0;

	/* 1+ args */
	if (arg2.empty()) {
		/* If the value contains a period, treat it as seconds; otherwise ms. */
		if (arg1.find_first_of('.') != arg1.npos)
			return StringToFloat(arg1);

		return StringToFloat(arg1) / 1000.f;
	}

	/* 2+ args */
	if (arg3.empty())
		return HHMMSSToSeconds(arg1 + ":" + arg2);

	/* 3+ args */
	return HHMMSSToSeconds(arg1 + ":" + arg2 + ":" + arg3);
}

void
DWILoader::GetApplicableFiles(const std::string& sPath,
							  std::vector<std::string>& out)
{
	GetDirListing(sPath + std::string("*.dwi"), out);
}

bool
DWILoader::LoadNoteDataFromSimfile(const std::string& path, Steps& out)
{
	MsdFile msd;
	if (!msd.ReadFile(path, false)) // don't unescape
	{
//		LOG->UserLog(
//		  "Song file", path, "couldn't be opened: %s", msd.GetError().c_str());
		return false;
	}

	for (unsigned i = 0; i < msd.GetNumValues(); i++) {
		int iNumParams = msd.GetNumParams(i);
		const MsdFile::value_t& params = msd.GetValue(i);
		std::string valueName = params[0];

		if (EqualsNoCase(valueName, "SINGLE") ||
			EqualsNoCase(valueName, "DOUBLE") ||
			EqualsNoCase(valueName, "COUPLE") ||
			EqualsNoCase(valueName, "SOLO")) {
			if (out.m_StepsType != GetTypeFromMode(valueName))
				continue;
			if (out.GetDifficulty() !=
				  DwiCompatibleStringToDifficulty(params[1]) &&
				out.GetDescription().find(
				  DifficultyToString(
					DwiCompatibleStringToDifficulty(params[1])) +
				  " Edit") == std::string::npos)
				continue;
			if (out.GetMeter() != StringToInt(params[2]))
				continue;
			std::string step1 = params[3];
			std::string step2 = (iNumParams == 5) ? params[4] : std::string("");
			out.SetNoteData(ParseNoteData(step1, step2, out, path));
			return true;
		}
	}
	return false;
}

bool
DWILoader::LoadFromDir(const std::string& sPath_,
					   Song& out,
					   std::set<std::string>& BlacklistedImages)
{
	std::vector<std::string> aFileNames;
	GetApplicableFiles(sPath_, aFileNames);

	if (aFileNames.size() > 1) {
//		LOG->UserLog("Song",
//					 sPath_,
//					 "has more than one DWI file. There should be only one!");
		return false;
	}

	/* We should have exactly one; if we had none, we shouldn't have been called
	 * to begin with. */
	ASSERT(aFileNames.size() == 1);
	const std::string sPath = sPath_ + aFileNames[0];

//	LOG->Trace("Song::LoadFromDWIFile(%s)", sPath.c_str());

	MsdFile msd;
	if (!msd.ReadFile(sPath, false)) // don't unescape
	{
//		LOG->UserLog(
//		  "Song file", sPath, "couldn't be opened: %s", msd.GetError().c_str());
		return false;
	}

	out.m_sSongFileName = sPath;

	for (unsigned i = 0; i < msd.GetNumValues(); i++) {
		int iNumParams = msd.GetNumParams(i);
		const MsdFile::value_t& sParams = msd.GetValue(i);
		std::string sValueName = sParams[0];

		if (iNumParams < 1) {
			/*LOG->UserLog("Song file",
						 sPath,
						 "has tag \"%s\" with no parameters.",
						 sValueName.c_str());*/
			continue;
		}

		// handle the data
		if (EqualsNoCase(sValueName, "FILE"))
			out.m_sMusicFile = sParams[1];

		else if (EqualsNoCase(sValueName, "TITLE")) {
			NotesLoader::GetMainAndSubTitlesFromFullTitle(
			  sParams[1], out.m_sMainTitle, out.m_sSubTitle);

			/* As far as I know, there's no spec on the encoding of this text.
			 * (I didn't look very hard, though.)  I've seen at least one file
			 * in ISO-8859-1. */
			ConvertString(out.m_sMainTitle, "utf-8,english");
			ConvertString(out.m_sSubTitle, "utf-8,english");
		}

		else if (EqualsNoCase(sValueName, "ARTIST")) {
			out.m_sArtist = sParams[1];
			ConvertString(out.m_sArtist, "utf-8,english");
		}

		else if (EqualsNoCase(sValueName, "GENRE")) {
			out.m_sGenre = sParams[1];
			ConvertString(out.m_sGenre, "utf-8,english");
		}

		else if (EqualsNoCase(sValueName, "CDTITLE"))
			out.m_sCDTitleFile = sParams[1];

		else if (EqualsNoCase(sValueName, "BPM")) {
			const float fBPM = StringToFloat(sParams[1]);

			if (unlikely(fBPM <= 0.0f)) {
				/*LOG->UserLog("Song file",
							 sPath,
							 "has an invalid BPM change at beat %f, BPM %f.",
							 0.0f,
							 fBPM);*/
			} else {
				out.m_SongTiming.AddSegment(BPMSegment(0, fBPM));
			}
		} else if (EqualsNoCase(sValueName, "DISPLAYBPM")) {
			// #DISPLAYBPM:[xxx..xxx]|[xxx]|[*];
			int iMin, iMax;
			/* We can't parse this as a float with sscanf, since '.' is a valid
			 * character in a float.  (We could do it with a regex, but it's not
			 * worth bothering with since we don't display fractional BPM
			 * anyway.) */
			if (sscanf(sParams[1].c_str(), "%i..%i", &iMin, &iMax) == 2) {
				out.m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
				out.m_fSpecifiedBPMMin = static_cast<float>(iMin);
				out.m_fSpecifiedBPMMax = static_cast<float>(iMax);
			} else if (sscanf(sParams[1].c_str(), "%i", &iMin) == 1) {
				out.m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
				out.m_fSpecifiedBPMMin = out.m_fSpecifiedBPMMax =
				  static_cast<float>(iMin);
			} else {
				out.m_DisplayBPMType = DISPLAY_BPM_RANDOM;
			}
		}

		else if (EqualsNoCase(sValueName, "GAP"))
			// the units of GAP is 1/1000 second
			out.m_SongTiming.m_fBeat0OffsetInSeconds =
			  -StringToInt(sParams[1]) / 1000.0f;

		else if (EqualsNoCase(sValueName, "SAMPLESTART"))
			out.m_fMusicSampleStartSeconds =
			  ParseBrokenDWITimestamp(sParams[1], sParams[2], sParams[3]);

		else if (EqualsNoCase(sValueName, "SAMPLELENGTH")) {
			float sampleLength =
			  ParseBrokenDWITimestamp(sParams[1], sParams[2], sParams[3]);
			if (sampleLength > 0 && sampleLength < 1) {
				// there were multiple versions of this tag allegedly: ensure a
				// decent length if requested.
				sampleLength *= 1000;
			}
			out.m_fMusicSampleLengthSeconds = sampleLength;

		}

		else if (EqualsNoCase(sValueName, "FREEZE")) {
			std::vector<std::string> arrayFreezeExpressions;
			split(sParams[1], ",", arrayFreezeExpressions);

			for (auto& arrayFreezeExpression : arrayFreezeExpressions) {
				std::vector<std::string> arrayFreezeValues;
				split(arrayFreezeExpression, "=", arrayFreezeValues);
				if (arrayFreezeValues.size() != 2) {
					/*LOG->UserLog("Song file",
								 sPath,
								 "has an invalid FREEZE: '%s'.",
								 arrayFreezeExpressions[f].c_str());*/
					continue;
				}
				int iFreezeRow =
				  BeatToNoteRow(StringToFloat(arrayFreezeValues[0]) / 4.0f);
				float fFreezeSeconds =
				  StringToFloat(arrayFreezeValues[1]) / 1000.0f;

				out.m_SongTiming.AddSegment(
				  StopSegment(iFreezeRow, fFreezeSeconds));
				//				LOG->Trace( "Adding a freeze segment: beat: %f,
				// seconds = %f", fFreezeBeat, fFreezeSeconds );
			}
		}

		else if (EqualsNoCase(sValueName, "CHANGEBPM") ||
				 EqualsNoCase(sValueName, "BPMCHANGE")) {
			std::vector<std::string> arrayBPMChangeExpressions;
			split(sParams[1], ",", arrayBPMChangeExpressions);

			for (auto& arrayBPMChangeExpression : arrayBPMChangeExpressions) {
				std::vector<std::string> arrayBPMChangeValues;
				split(arrayBPMChangeExpression, "=", arrayBPMChangeValues);
				if (arrayBPMChangeValues.size() != 2) {
					/*LOG->UserLog("Song file",
								 sPath,
								 "has an invalid CHANGEBPM: '%s'.",
								 arrayBPMChangeExpressions[b].c_str());*/
					continue;
				}

				int iStartIndex =
				  BeatToNoteRow(StringToFloat(arrayBPMChangeValues[0]) / 4.0f);
				float fBPM = StringToFloat(arrayBPMChangeValues[1]);
				if (fBPM > 0.0f)
					out.m_SongTiming.AddSegment(BPMSegment(iStartIndex, fBPM));
				else {
					//					LOG->UserLog(
					//					  "Song file",
					//					  sPath,
					//					  "has an invalid BPM change at beat %f, BPM %f.", 					  NoteRowToBeat(iStartIndex), 					  fBPM);
				}
			}
		}

		else if (EqualsNoCase(sValueName, "SINGLE") ||
				 EqualsNoCase(sValueName, "DOUBLE") ||
				 EqualsNoCase(sValueName, "COUPLE") ||
				 EqualsNoCase(sValueName, "SOLO")) {
			Steps* pNewNotes = out.CreateSteps();
			LoadFromDWITokens(sParams[0],
							  sParams[1],
							  sParams[2],
							  sParams[3],
							  (iNumParams == 5) ? sParams[4] : std::string(""),
							  *pNewNotes,
							  sPath);
			if (pNewNotes->m_StepsType != StepsType_Invalid) {
				pNewNotes->SetFilename(sPath);
				out.AddSteps(pNewNotes);
			} else
				delete pNewNotes;
		} else if (EqualsNoCase(sValueName, "DISPLAYTITLE") ||
				   EqualsNoCase(sValueName, "DISPLAYARTIST")) {
			/* We don't want to support these tags.  However, we don't want
			 * to pick up images used here as song images (eg. banners). */
			std::string param = sParams[1];
			/* "{foo} ... {foo2}" */
			size_t pos = 0;
			while (pos < std::string::npos) {

				size_t startpos = param.find('{', pos);
				if (startpos == std::string::npos)
					break;
				size_t endpos = param.find('}', startpos);
				if (endpos == std::string::npos)
					break;

				std::string sub =
				  param.substr(startpos + 1, endpos - startpos - 1);

				pos = endpos + 1;

				MakeLower(sub);
				BlacklistedImages.insert(sub);
			}
		} else {
			// do nothing.  We don't care about this value name
		}
	}

	return true;
}
