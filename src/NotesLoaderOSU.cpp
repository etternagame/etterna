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
#include <algorithm>


vector<string> split(string str, string token) {
	vector<string>result;
	while (str.size()) {
		int index = str.find(token);
		if (index != string::npos) {
			result.push_back(str.substr(0, index));
			str = str.substr(index + token.size());
			if (str.size() == 0)result.push_back(str);
		}
		else {
			result.push_back(str);
			str = "";
		}
	}
	return result;
}

map<string, map<string, string>> OsuLoader::ParseFileString(string fileContents)
{
	vector<string> sections;
	vector<vector<string>> contents;

	SeparateTagsAndContents(fileContents, sections, contents);

	map<string, map<string, string>> parsedData;
	for (int i = 0; i < sections.size(); ++i)
	{
		for (auto& content : contents[i]) {
			auto& str = content;
			int pos = str.find_first_of(':');
			std::string value = str.substr(pos + 1),
				tag = str.substr(0, pos);
			parsedData[sections[i]][tag] = value;
		}
	}
	return parsedData;
}

void OsuLoader::SeparateTagsAndContents(string fileContents, vector<string> &tagsOut, vector<vector<string>> &contentsOut)
{
	char lastByte = '\0';
	bool isComment = false;
	bool isTag = false;
	bool isContent = false;
	string tag = "";
	string content = "";

	int tagIndex = 0;

	for (int i = 0; i < (int)fileContents.length(); ++i)
	{
		char currentByte = fileContents[i];

		if (isComment)
		{
			if (currentByte == '\n')
			{
				isComment = false;
			}
		}
		else if (currentByte == '/')
		{
			if (lastByte == '/')
			{
				isComment = true;
			}
		}
		else if (currentByte == '[')
		{
			tag = "";
			isTag = true;
		}
		else if (isTag)
		{
			if (currentByte == ']')
			{
				++tagIndex;
				tagsOut.emplace_back(tag);
				isTag = false;
				content = "";
				contentsOut.emplace_back(vector<string>());
				isContent = true;
			}
			else
			{
				tag = tag + currentByte;
			}
		}
		else if (isContent)
		{
			if (currentByte == '[' || i == (int)fileContents.length()-1)
			{
				contentsOut.back().emplace_back(content);
				content = "";
				isContent = false;
				tag = "";
				isTag = true;
			}
			else if (currentByte == '\n')
			{
				contentsOut.back().emplace_back(content);
				content = "";
			}
			else
			{
				content = content + currentByte;
			}
		}
		lastByte = currentByte;
	}
}

void OsuLoader::SetMetadata(map<string, map<string, string>> parsedData, Song &out)
{
	// set metadata values
	auto& metadata = parsedData["Metadata"];
	out.m_sMainTitle = metadata["Title"];
	out.m_sSubTitle = metadata["Version"];
	out.m_sArtist = metadata["Artist"];
	out.m_sGenre = "";

	// set other stuff (to-do)
	out.m_sCDTitleFile = "";
	auto it = parsedData["TimingPoints"].begin();
	it++;
	auto values = split(it->first, ",");
	out.m_SongTiming.AddSegment(BPMSegment(0, 60000/stof(values[1])));
	out.m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
	out.m_fSpecifiedBPMMin = 1;
	out.m_fSpecifiedBPMMax = 100;
	out.m_SongTiming.m_fBeat0OffsetInSeconds = -StringToInt("000") / 1000.0f;
	auto general = parsedData["General"];
	out.m_fMusicSampleStartSeconds = stoi(general["AudioLeadIn"]) / 1000;
	out.m_fMusicSampleLengthSeconds = stoi(general["PreviewTime"]) / 1000;

	ConvertString(out.m_sMainTitle, "utf-8,english");
	ConvertString(out.m_sSubTitle, "utf-8,english");
}

void LoadChartData(Song* song,Steps* chart, map<string, map<string, string>> parsedData) {
	switch (stoi(parsedData["Difficulty"]["CircleSize"]))
	{
	case(4):
	{
		chart->m_StepsType = StepsType_dance_single;
		break;
	}
	case(6):
	{
		chart->m_StepsType = StepsType_dance_solo;
		break;
	}
	case(8):
	{
		chart->m_StepsType = StepsType_dance_double;
		break;
	}
	default:
		chart->m_StepsType = StepsType_Invalid;
		break;
	} // needs more stepstypes

	
	chart->SetMeter(song->GetAllSteps().size());

	chart->SetDifficulty((Difficulty)(min(song->GetAllSteps().size(), (size_t)Difficulty_Edit)));


	NoteData nd;
	nd.Init();
	nd.SetNumTracks(4);
	nd.SetTapNote(0, 0, TAP_ORIGINAL_TAP);
	nd.SetTapNote(1, 20, TAP_ORIGINAL_TAP);
	nd.SetTapNote(2, 0, TAP_ORIGINAL_TAP);
	nd.SetTapNote(3, 600, TAP_ORIGINAL_TAP);

	chart->SetNoteData(nd);

	chart->TidyUpData();

	chart->SetSavedToDisk(true);

	chart->m_Timing.AddSegment(BPMSegment(0, 160.0));
}
void OsuLoader::GetApplicableFiles(const RString &sPath, vector<RString> &out)
{
	GetDirListing(sPath + RString("*.osu"), out);
}

bool OsuLoader::LoadNoteDataFromSimfile(const RString &path, Steps &out)
{
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

bool OsuLoader::LoadFromDir(const RString &sPath_, Song &out)
{
	vector<RString> aFileNames;
	GetApplicableFiles(sPath_, aFileNames);

	const RString sPath = sPath_ + aFileNames[0];

	LOG->Trace("Song::LoadFromDWIFile(%s)", sPath.c_str()); //osu

	RageFile f;
	if (!f.Open(sPath))
	{
		LOG->UserLog("Song file", sPath, "couldn't be opened: %s", f.GetError().c_str());
		return false;
	}

	RString fileStr;
	f.Read(fileStr, -1);

	out.m_sSongFileName = sPath;
	auto parsedData = ParseFileString(fileStr.c_str());

	SetMetadata(parsedData, out);


	for (auto&filename : aFileNames) {
		auto chart = out.CreateSteps();
		chart->SetFilename(sPath_ + filename);
		LoadChartData(&out, chart, parsedData);
		out.AddSteps(chart);
	}

	return true;
}
