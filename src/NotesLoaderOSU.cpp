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


vector<string> split(string str, string token)
{
	vector<string>result;
	while (str.size())
	{
		int index = str.find(token);
		if (index != string::npos)
		{
			result.push_back(str.substr(0, index));
			str = str.substr(index + token.size());
			if (str.size() == 0)result.push_back(str);
		}
		else
		{
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
		for (auto& content : contents[i])
		{
			auto& str = content;
			int pos = str.find_first_of(':');
			string value = str.substr(pos + 1),
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

	ConvertString(out.m_sMainTitle, "utf-8,english");
	ConvertString(out.m_sSubTitle, "utf-8,english");

	out.m_sCDTitleFile = "";
}

void OsuLoader::SetTimingData(map<string, map<string, string>> parsedData, Song &out)
{
	auto it = parsedData["TimingPoints"].begin();


	vector<pair<float, float>> bpms;
	++it;
	auto values = split(it->first, ",");
	bpms.emplace_back(pair<float, float>(stof(values[0]), 60000 / stof(values[1])));
	//out.m_SongTiming.AddSegment(BPMSegment(0, 999));
	out.m_SongTiming.AddSegment(BPMSegment(0, 60000 / stof(values[1])));

	float bpm = 0;
	while (++it != parsedData["TimingPoints"].end())
	{
		auto line = it->first;
		auto values = split(line, ",");

		float offset = stof(values[0]);
		if (stof(values[1]) > 0)
		{
			bpm = 60000 / stof(values[1]);
		}
		else
		{
			bpm *= abs(stof(values[1]))/100;
		}
		bpms.emplace_back(pair<float, float>(offset, bpm));
	}

	sort(bpms.begin(), bpms.end(), [](pair<float, float> a, pair<float, float> b)
	{
		return a.first < b.first;
	});
	for (int i = 0; i < bpms.size(); ++i)
	{
		int row = MsToNoteRow(bpms[i].first, &out);
		out.m_SongTiming.AddSegment(BPMSegment(row, bpms[i].second));
	}

	out.m_DisplayBPMType = DISPLAY_BPM_ACTUAL;

	auto general = parsedData["General"];
	out.m_fMusicSampleStartSeconds = stof(general["AudioLeadIn"]) / 1000.0f;
	out.m_fMusicSampleLengthSeconds = stof(general["PreviewTime"]) / 1000.0f;
}

void OsuLoader::LoadChartData(Song* song, Steps* chart, map<string, map<string, string>> parsedData)
{
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

	chart->TidyUpData();

	chart->SetSavedToDisk(true);
}

void OsuLoader::GetApplicableFiles(const RString &sPath, vector<RString> &out)
{
	GetDirListing(sPath + RString("*.osu"), out);
}

int OsuLoader::MsToNoteRow(int ms, Song* song)
{
	return (int)abs(song->m_SongTiming.GetBeatFromElapsedTime(ms / 1000.0f)*48);
	//return (int)abs(song->m_SongTiming.GetBeatFromElapsedTime((ms + song->m_SongTiming.m_fBeat0OffsetInSeconds) / 1000.0f)*48);
}

void OsuLoader::LoadNoteDataFromParsedData(Steps* out, map<string, map<string, string>> parsedData)
{
	NoteData newNoteData;
	newNoteData.SetNumTracks(stoi(parsedData["Difficulty"]["CircleSize"]));
	auto it = parsedData["HitObjects"].begin();
	vector<OsuNote> taps;
	vector<OsuHold> holds;
	while (++it != parsedData["HitObjects"].end()) {
		auto line = it->first;
		auto values = split(line, ",");
		int type = stoi(values[3]);
		if (type == 128)
			holds.emplace_back(OsuHold(stoi(values[2]), stoi(values[5]), stoi(values[0])));
		else if(type & 1 == 1)
			taps.emplace_back(OsuNote(stoi(values[2]), stoi(values[0])));
	}

	sort(taps.begin(), taps.end(), [](OsuNote a, OsuNote b)
	{
		return a.ms < b.ms;
	});

	//out->m_pSong->m_SongTiming.m_fBeat0OffsetInSeconds = 3;//taps[0].ms / 1000.0f;

	for (int i = 0; i < taps.size(); ++i)
	{
		newNoteData.SetTapNote(
			taps[i].lane / (512 / stoi(parsedData["Difficulty"]["CircleSize"])),
			MsToNoteRow( taps[i].ms - taps[0].ms, out->m_pSong ),
			TAP_ORIGINAL_TAP
		);
	}
	for (int i = 0; i < holds.size(); ++i)
	{
		newNoteData.AddHoldNote(
			holds[i].lane / (512 / stoi(parsedData["Difficulty"]["CircleSize"])),
			MsToNoteRow( holds[i].msStart, out->m_pSong ),
			MsToNoteRow( holds[i].msEnd, out->m_pSong ),
			TAP_ORIGINAL_HOLD_HEAD
		);
	}

	out->SetNoteData(newNoteData);
}

bool OsuLoader::LoadNoteDataFromSimfile(const RString &path, Steps &out)
{
	RageFile f;
	if (!f.Open(path))
	{
		LOG->UserLog("Song file", path, "couldn't be opened: %s", f.GetError().c_str());
		return false;
	}

	RString fileRStr;
	fileRStr.reserve(f.GetFileSize());
	f.Read(fileRStr, -1);

	string fileStr = fileRStr.c_str();
	auto parsedData = ParseFileString(fileStr.c_str());
	LoadNoteDataFromParsedData(&out, parsedData);

	return false;
}

bool OsuLoader::LoadFromDir(const RString &sPath_, Song &out)
{
	vector<RString> aFileNames;
	GetApplicableFiles(sPath_, aFileNames);

	//const RString sPath = sPath_ + aFileNames[0];

	//LOG->Trace("Song::LoadFromDWIFile(%s)", sPath.c_str()); //osu

	RageFile f;
	map<string, map<string, string>> parsedData;


	for (auto&filename : aFileNames) {
		auto p = sPath_ + filename;

		if (!f.Open(p))
		{
			continue;
		}
		RString fileContents;
		f.Read(fileContents, -1);
		parsedData = ParseFileString(fileContents.c_str());
		if (filename == aFileNames[0])
		{
			SetMetadata(parsedData, out);
			SetTimingData(parsedData, out);
		}
		auto chart = out.CreateSteps();
		chart->SetFilename(p);
		LoadChartData(&out, chart, parsedData);

		LoadNoteDataFromParsedData(chart, parsedData);
		out.AddSteps(chart);
	}

	//out.Save(false);

	return true;
}
