#include "global.h"
#include "Difficulty.h"
#include "GameInput.h"
#include "MsdFile.h"
#include "NoteData.h"
#include "NotesLoader.h"
#include "NotesLoaderOSU.h"
#include "PrefsManager.h"
#include "RageFile.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageUtil_CharConversions.h"
#include "Song.h"
#include "Steps.h"

#include <map>



void OsuLoader::GetApplicableFiles(const RString &sPath, vector<RString> &out)
{
	GetDirListing(sPath + RString("*.osu"), out);
}

bool OsuLoader::LoadNoteDataFromSimfile(const RString &path, Steps &out)
{
	throw std::exception();

	RageFile f;
	if (!f.Open(path))
	{
		LOG->UserLog("Song file", path, "couldn't be opened: %s", f.GetError().c_str());
		return false;
	}

	RString FileRStr;
	FileRStr.reserve(f.GetFileSize());

	int iBytesRead = f.Read(FileRStr);
	string FileStr = FileRStr.c_str();







	return false;
}

bool OsuLoader::LoadFromDir(const RString &sPath_, Song &out, set<RString> &BlacklistedImages)
{
	vector<RString> aFileNames;
	GetApplicableFiles(sPath_, aFileNames);

	if (aFileNames.size() > 1)
	{
		LOG->UserLog("Song", sPath_, "has more than one OSZ file. There should be only one!");
		return false;
	}

	/* We should have exactly one; if we had none, we shouldn't have been called to begin with. */
	ASSERT(aFileNames.size() == 1);
	const RString sPath = sPath_ + aFileNames[0];

	LOG->Trace("Song::LoadFromDWIFile(%s)", sPath.c_str());

	RageFile f;
	if (!f.Open(sPath))
	{
		LOG->UserLog("Song file", sPath, "couldn't be opened: %s", f.GetError().c_str());
		return false;
	}

	out.m_sSongFileName = sPath;



	out.m_sMusicFile = "asdf";

	NotesLoader::GetMainAndSubTitlesFromFullTitle("asdf", out.m_sMainTitle, out.m_sSubTitle);

	ConvertString(out.m_sMainTitle, "utf-8,english");
	ConvertString(out.m_sSubTitle, "utf-8,english");

	//else if (sValueName.EqualsNoCase("ARTIST"))
	{
		out.m_sArtist = "artest";
		ConvertString(out.m_sArtist, "utf-8,english");
	}

	//else if (sValueName.EqualsNoCase("GENRE"))
	{
		out.m_sGenre = "genretest";
		ConvertString(out.m_sGenre, "utf-8,english");
	}

	//else if (sValueName.EqualsNoCase("CDTITLE"))
		out.m_sCDTitleFile = "cdtitest";

	//else if (sValueName.EqualsNoCase("BPM"))
	{
		const float fBPM = StringToFloat("420");

		if (unlikely(fBPM <= 0.0f && !PREFSMAN->m_bQuirksMode))
		{
			LOG->UserLog("Song file", sPath, "has an invalid BPM change at beat %f, BPM %f.",
				0.0f, fBPM);
		}
		else
		{
			out.m_SongTiming.AddSegment(BPMSegment(0, fBPM));
		}
	}
	//else if (sValueName.EqualsNoCase("DISPLAYBPM"))
	{
		// #DISPLAYBPM:[xxx..xxx]|[xxx]|[*];
		int iMin, iMax;
		/* We can't parse this as a float with sscanf, since '.' is a valid
		* character in a float.  (We could do it with a regex, but it's not
		* worth bothering with since we don't display fractional BPM anyway.) */
		if (sscanf("666", "%i..%i", &iMin, &iMax) == 2)
		{
			out.m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
			out.m_fSpecifiedBPMMin = static_cast<float>(iMin);
			out.m_fSpecifiedBPMMax = static_cast<float>(iMax);
		}
		else if (sscanf("9", "%i", &iMin) == 1)
		{
			out.m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
			out.m_fSpecifiedBPMMin = out.m_fSpecifiedBPMMax = static_cast<float>(iMin);
		}
		else
		{
			out.m_DisplayBPMType = DISPLAY_BPM_RANDOM;
		}
	}

	//else if (sValueName.EqualsNoCase("GAP"))
		// the units of GAP is 1/1000 second
		out.m_SongTiming.m_fBeat0OffsetInSeconds = -StringToInt("000") / 1000.0f;

	//else if (sValueName.EqualsNoCase("SAMPLESTART"))
		out.m_fMusicSampleStartSeconds = 1.0;//ParseBrokenDWITimestamp(sParams[1], sParams[2], sParams[3]);

	//else if (sValueName.EqualsNoCase("SAMPLELENGTH"))
	{
		float sampleLength = 10;//ParseBrokenDWITimestamp(sParams[1], sParams[2], sParams[3]);
		if (sampleLength > 0 && sampleLength < 1) {
			// there were multiple versions of this tag allegedly: ensure a decent length if requested.
			sampleLength *= 1000;
		}
		out.m_fMusicSampleLengthSeconds = sampleLength;

	}

	//else if (sValueName.EqualsNoCase("FREEZE"))
	{
		vector<RString> arrayFreezeExpressions;
		split("4,2,222,33,3", ",", arrayFreezeExpressions);

		for (unsigned f = 0; f<arrayFreezeExpressions.size(); f++)
		{
			vector<RString> arrayFreezeValues;
			split(arrayFreezeExpressions[f], "=", arrayFreezeValues);
			if (arrayFreezeValues.size() != 2)
			{
				LOG->UserLog("Song file", sPath, "has an invalid FREEZE: '%s'.", arrayFreezeExpressions[f].c_str());
				continue;
			}
			int iFreezeRow = BeatToNoteRow(StringToFloat(arrayFreezeValues[0]) / 4.0f);
			float fFreezeSeconds = StringToFloat(arrayFreezeValues[1]) / 1000.0f;

			out.m_SongTiming.AddSegment(StopSegment(iFreezeRow, fFreezeSeconds));
			//				LOG->Trace( "Adding a freeze segment: beat: %f, seconds = %f", fFreezeBeat, fFreezeSeconds );
		}
	}

	//else if (sValueName.EqualsNoCase("CHANGEBPM") || sValueName.EqualsNoCase("BPMCHANGE"))
	{
		vector<RString> arrayBPMChangeExpressions;
		split("234,2342,34", ",", arrayBPMChangeExpressions);

		for (unsigned b = 0; b<arrayBPMChangeExpressions.size(); b++)
		{
			vector<RString> arrayBPMChangeValues;
			split(arrayBPMChangeExpressions[b], "=", arrayBPMChangeValues);
			if (arrayBPMChangeValues.size() != 2)
			{
				LOG->UserLog("Song file", sPath, "has an invalid CHANGEBPM: '%s'.", arrayBPMChangeExpressions[b].c_str());
				continue;
			}

			int iStartIndex = BeatToNoteRow(StringToFloat(arrayBPMChangeValues[0]) / 4.0f);
			float fBPM = StringToFloat(arrayBPMChangeValues[1]);
			if (fBPM > 0.0f)
				out.m_SongTiming.AddSegment(BPMSegment(iStartIndex, fBPM));
			else
				LOG->UserLog("Song file", sPath, "has an invalid BPM change at beat %f, BPM %f.",
					NoteRowToBeat(iStartIndex), fBPM);
		}
	}

	/*else if (sValueName.EqualsNoCase("SINGLE") ||
		sValueName.EqualsNoCase("DOUBLE") ||
		sValueName.EqualsNoCase("COUPLE") ||
		sValueName.EqualsNoCase("SOLO"))*/
	{
		/*Steps* pNewNotes = out.CreateSteps();
		LoadFromDWITokens(
			sParams[0],
			sParams[1],
			sParams[2],
			sParams[3],
			(iNumParams == 5) ? sParams[4] : RString(""),
			*pNewNotes,
			sPath
		);
		if (pNewNotes->m_StepsType != StepsType_Invalid)
		{
			pNewNotes->SetFilename(sPath);
			out.AddSteps(pNewNotes);
		}
		else
			delete pNewNotes;*/

		auto chart = *out.CreateSteps();

		chart.m_StepsType = StepsType_dance_single;//GetTypeFromMode(sMode);

		chart.SetMeter(StringToInt("69"));

		//chart.SetDifficulty(DwiCompatibleStringToDifficulty(sDescription));
		chart.SetDifficulty(Difficulty_Beginner);

		//chart.SetNoteData(ParseNoteData(sStepData1, sStepData2, out, sPath));

		NoteData nd;
		nd.SetTapNote(0, 0, TAP_ORIGINAL_TAP);
		nd.SetTapNote(0, 20, TAP_ORIGINAL_TAP);
		chart.SetNoteData(nd);

		chart.TidyUpData();

		chart.SetSavedToDisk(true);

		out.AddSteps(&chart);
	}
	//else if (sValueName.EqualsNoCase("DISPLAYTITLE") ||
		//sValueName.EqualsNoCase("DISPLAYARTIST"))
	{
		/* We don't want to support these tags.  However, we don't want
		* to pick up images used here as song images (eg. banners). */
		RString param = "fda";
		/* "{foo} ... {foo2}" */
		size_t pos = 0;
		while (pos < RString::npos)
		{

			size_t startpos = param.find('{', pos);
			if (startpos == RString::npos)
				break;
			size_t endpos = param.find('}', startpos);
			if (endpos == RString::npos)
				break;

			RString sub = param.substr(startpos + 1, endpos - startpos - 1);

			pos = endpos + 1;

			sub.MakeLower();
			BlacklistedImages.insert(sub);
		}
	}





	

	return true;
}
