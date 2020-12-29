#include "Etterna/Globals/global.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "NotesLoaderOSU.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Utils/RageUtil_CharConversions.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Singletons/PrefsManager.h"

#include <string>
#include <map>
#include <algorithm>

using std::map;
using std::string;

vector<string>
split(string str, string token)
{
	vector<string> result;
	while (str.size()) {
		int index = str.find(token);
		if (index != string::npos) {
			result.push_back(str.substr(0, index));
			str = str.substr(index + token.size());
			if (str.size() == 0)
				result.push_back(str);
		} else {
			result.push_back(str);
			str = "";
		}
	}
	return result;
}

map<string, map<string, string>>
OsuLoader::ParseFileString(string fileContents)
{
	vector<string> sections;
	vector<vector<string>> contents;

	SeparateTagsAndContents(fileContents, sections, contents);

	map<string, map<string, string>> parsedData;
	bool colurz = (sections.size() == 8 &&
				   std::count(sections.begin(), sections.end(), "Colours"));
	if (sections.size() == 7 || colurz) {
		for (int i = 0; i < (int)sections.size(); ++i) {
			for (auto& content : contents[i]) {
				auto& str = content;
				int pos = str.find_first_of(':');
				string value = str.substr(pos + 1), tag = str.substr(0, pos);
				parsedData[sections[i]][tag] = value;
			}
		}
	}
	return parsedData;
}

void
OsuLoader::SeparateTagsAndContents(string fileContents,
								   vector<string>& tagsOut,
								   vector<vector<string>>& contentsOut)
{
	char lastByte = '\0';
	bool isComment = false;
	bool isTag = false;
	bool isContent = false;
	string tag = "";
	string content = "";

	int tagIndex = 0;

	for (int i = 0; i < (int)fileContents.length(); ++i) {
		char currentByte = fileContents[i];

		if (isComment || currentByte == '\r') // ignore carriage return
		{
			if (currentByte == '\n') {
				isComment = false;
			}
		} else if (currentByte == '/') {
			if (lastByte == '/') {
				isComment = true;
			}
		} else if (isTag) {
			if (currentByte == ']') {
				++tagIndex;
				tagsOut.emplace_back(tag);
				isTag = false;
				content = "";
				contentsOut.emplace_back(vector<string>());
				isContent = true;
			} else {
				tag = tag + currentByte;
			}
		} else if (isContent) {
			if ((currentByte == '[' && lastByte == '\n') ||
				i == (int)fileContents.length() - 1) {
				if (!content.empty()) // we don't want empty values on our
									  // content vectors
					contentsOut.back().emplace_back(content);
				content = "";
				isContent = false;
				tag = "";
				isTag = true;
			} else if (currentByte == '\n') {
				if (!content.empty()) // we don't want empty values on our
									  // content vectors
					contentsOut.back().emplace_back(content);
				content = "";
			} else {
				content = content + currentByte;
			}
		} else if (currentByte == '[') {
			tag = "";
			isTag = true;
		}
		lastByte = currentByte;
	}
}

void
OsuLoader::SetMetadata(map<string, map<string, string>> parsedData, Song& out)
{
	// set metadata values
	auto& metadata = parsedData["Metadata"];
	out.m_sSongFileName = metadata["Title"];
	out.m_sMainTitle = metadata["Title"];
	out.m_sSubTitle = metadata["Version"];
	out.m_sArtist = metadata["Artist"];
	out.m_sGenre = "";

	ConvertString(out.m_sMainTitle, "utf-8,english");
	ConvertString(out.m_sSubTitle, "utf-8,english");

	out.m_sMusicFile = parsedData["General"]["AudioFilename"];
	// out.m_sBackgroundFile = "";
	// out.m_sCDTitleFile = "";
}

void
OsuLoader::SetTimingData(map<string, map<string, string>> parsedData, Song& out)
{
	vector<std::pair<int, float>> tp;

	for (auto& it : parsedData["TimingPoints"]) {
		auto line = it.first;
		auto values = split(line, ",");

		tp.emplace_back(
		  std::pair<int, float>(stoi(values[0]), stod(values[1])));
	}
	sort(tp.begin(),
		 tp.end(),
		 [](std::pair<int, float> a, std::pair<int, float> b) {
			 return a.first < b.first;
		 });

	vector<std::pair<int, float>> bpms;
	float lastpositivebpm = 0;
	int offset = 0;
	int lastoffset = -9999;
	for (auto x : tp) {
		float bpm;
		offset = std::max(0, x.first);
		if (x.second > 0) {
			bpm = 60000 / x.second;
			lastpositivebpm = bpm;
		} else // inherited timing points; these might be able to be ignored
		{
			bpm = lastpositivebpm * abs(x.second / 100);
		}
		if (offset == lastoffset) {
			bpms[bpms.size() - 1] = std::pair<int, float>(
			  offset, bpm); // this because of dumb stuff like
							// in 4k Luminal dan (not robust,
							// but works for most files)
		} else {
			bpms.emplace_back(std::pair<int, float>(offset, bpm));
		}
		lastoffset = offset;
	}
	if (bpms.size() > 0) {
		out.m_SongTiming.AddSegment(
		  BPMSegment(0, bpms[0].second)); // set bpm at beat 0 (osu files don't
										  // make this required)
	} else {
		out.m_SongTiming.AddSegment(BPMSegment(0, 120)); // set bpm to 120 if
														 // there are no
														 // specified bpms in
														 // the file (there
														 // should be)
	}
	for (auto& bpm : bpms) {
		int row = MsToNoteRow(bpm.first, &out);
		if (row != 0) {
			out.m_SongTiming.AddSegment(BPMSegment(row, bpm.second));
		}
	}

	out.m_DisplayBPMType = DISPLAY_BPM_ACTUAL;

	auto general = parsedData["General"];
	out.m_fMusicSampleStartSeconds = stof(general["AudioLeadIn"]) / 1000.0f;
	out.m_fMusicSampleLengthSeconds =
	  stof(general["PreviewTime"]) / 1000.0f; // these probably aren't right
}

bool
OsuLoader::LoadChartData(Song* song,
						 Steps* chart,
						 map<string, map<string, string>> parsedData)
{
	if (stoi(parsedData["General"]["Mode"]) != 3 ||
		parsedData.find("HitObjects") ==
		  parsedData.end()) // if the mode isn't mania or notedata is empty
	{
		return false;
	}

	switch (stoi(parsedData["Difficulty"]["CircleSize"])) {
		case (4): {
			chart->m_StepsType = StepsType_dance_single;
			break;
		}
		case (5): {
			chart->m_StepsType = StepsType_pump_single;
			break;
		}
		case (6): {
			chart->m_StepsType = StepsType_dance_solo;
			break;
		}
		case (7): {
			chart->m_StepsType = StepsType_kb7_single;
			break;
		}
		case (8): {
			chart->m_StepsType = StepsType_dance_double;
			break;
		}
		default:
			chart->m_StepsType = StepsType_Invalid;
			return false;
	} // needs more stepstypes?

	chart->SetMeter(song->GetAllSteps().size());

	chart->SetDifficulty((Difficulty)(
	  std::min(song->GetAllSteps().size(), (size_t)Difficulty_Edit)));

	chart->TidyUpData();

	chart->SetSavedToDisk(true);

	return true;
}

void
OsuLoader::GetApplicableFiles(const std::string& sPath,
							  vector<std::string>& out)
{
	GetDirListing(sPath + std::string("*.osu"), out);
}

int
OsuLoader::MsToNoteRow(int ms, Song* song)
{
	float tempOffset = song->m_SongTiming.m_fBeat0OffsetInSeconds;
	song->m_SongTiming.m_fBeat0OffsetInSeconds = 0;

	int row = (int)round(abs(
	  song->m_SongTiming.GetBeatFromElapsedTimeNoOffset(ms / 1000.0f) * 48));

	song->m_SongTiming.m_fBeat0OffsetInSeconds = tempOffset;

	return row;
	// wow
	// this is fucking hideous
	// but it works
}

void
OsuLoader::LoadNoteDataFromParsedData(
  Steps* out,
  map<std::string, map<std::string, std::string>> parsedData)
{
	NoteData newNoteData;
	newNoteData.SetNumTracks(stoi(parsedData["Difficulty"]["CircleSize"]));

	vector<OsuNote> taps;
	vector<OsuHold> holds;
	bool useLifts = PREFSMAN->LiftsOnOsuHolds;
	for (auto& it : parsedData["HitObjects"]) {
		auto line = it.first;
		auto values = split(line, ",");
		int type = stoi(values[3]);
		if (type == 128)
			holds.emplace_back(
			  OsuHold(stoi(values[2]), stoi(values[5]), stoi(values[0])));
		else if ((type & 1) == 1)
			taps.emplace_back(OsuNote(stoi(values[2]), stoi(values[0])));
	}

	sort(taps.begin(), taps.end(), [](OsuNote a, OsuNote b) {
		return a.ms < b.ms;
	});
	sort(holds.begin(), holds.end(), [](OsuHold a, OsuHold b) {
		return a.msStart < b.msStart;
	});

	int firstTap = 0;
	if (taps.size() > 0 && holds.size() > 0) {
		firstTap = std::min(taps[0].ms, holds[0].msStart);
	} else if (taps.size() > 0) {
		firstTap = taps[0].ms;
	} else {
		firstTap = holds[0].msStart;
	}

	for (auto& tap : taps) {
		newNoteData.SetTapNote(
		  tap.lane / (512 / stoi(parsedData["Difficulty"]["CircleSize"])),
		  MsToNoteRow(tap.ms - firstTap, out->m_pSong),
		  TAP_ORIGINAL_TAP);
	}
	for (auto& hold : holds) {
		int start = MsToNoteRow(hold.msStart - firstTap, out->m_pSong);
		int end = MsToNoteRow(hold.msEnd - firstTap, out->m_pSong);
		if (end - start > 0 && useLifts) {
			end = end - 1;
		}
		newNoteData.AddHoldNote(
		  hold.lane / (512 / stoi(parsedData["Difficulty"]["CircleSize"])),
		  start,
		  end,
		  TAP_ORIGINAL_HOLD_HEAD);
		if (useLifts)
			newNoteData.SetTapNote(
			  hold.lane / (512 / stoi(parsedData["Difficulty"]["CircleSize"])),
			  end + 1,
			  TAP_ORIGINAL_LIFT);
	}

	out->m_pSong->m_SongTiming.m_fBeat0OffsetInSeconds = -firstTap / 1000.0f;

	out->SetNoteData(newNoteData);
}

bool
OsuLoader::LoadNoteDataFromSimfile(const std::string& path, Steps& out)
{
	RageFile f;
	if (!f.Open(path)) {
		//		LOG->UserLog(
		//		  "Song file", path, "couldn't be opened: %s",
		// f.GetError().c_str());
		return false;
	}

	std::string fileRStr;
	fileRStr.reserve(f.GetFileSize());
	f.Read(fileRStr, -1);

	string fileStr = fileRStr.c_str();
	auto parsedData = ParseFileString(fileStr.c_str());
	LoadNoteDataFromParsedData(&out, parsedData);

	return !out.IsNoteDataEmpty();
}

bool
OsuLoader::LoadFromDir(const std::string& sPath_, Song& out)
{
	vector<std::string> aFileNames;
	GetApplicableFiles(sPath_, aFileNames);

	// const std::string sPath = sPath_ + aFileNames[0];

	// LOG->Trace("Song::LoadFromDWIFile(%s)", sPath.c_str()); //osu

	RageFile f;
	map<std::string, map<std::string, std::string>> parsedData;

	for (auto& filename : aFileNames) {
		auto p = sPath_ + filename;

		if (!f.Open(p)) {
			continue;
		}
		std::string fileContents;
		f.Read(fileContents, -1);
		parsedData = ParseFileString(fileContents.c_str());
		if (parsedData.size() == 0) {
			continue;
		}
		if (out.m_SongTiming.empty()) {
			SetMetadata(parsedData, out);
			SetTimingData(parsedData, out);
		}
		auto chart = out.CreateSteps();
		chart->SetFilename(p);
		if (!LoadChartData(&out, chart, parsedData)) {
			SAFE_DELETE(chart);
			continue;
		}

		LoadNoteDataFromParsedData(chart, parsedData);
		out.AddSteps(chart);
	}

	// the metadata portion saves the filename/path wrong, this corrects it
	if (!out.m_sSongFileName.empty())
		out.m_sSongFileName = sPath_ + out.m_sSongFileName;

	// out.Save(false);

	return true;
}
