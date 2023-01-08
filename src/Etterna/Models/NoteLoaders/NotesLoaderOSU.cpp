#include "Etterna/Globals/global.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "NotesLoaderOSU.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Utils/RageUtil_CharConversions.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Singletons/PrefsManager.h"

#include <cmath>
#include <string>
#include <map>
#include <algorithm>

std::vector<std::string>
split(std::string str, const std::string& token)
{
	std::vector<std::string> result;
	while (!str.empty()) {
		auto index = str.find(token);
		if (index != std::string::npos) {
			result.push_back(str.substr(0, index));
			str = str.substr(index + token.size());
			if (str.empty())
				result.push_back(str);
		} else {
			result.push_back(str);
			str = "";
		}
	}
	return result;
}

std::map<std::string, std::map<std::string, std::string>>
OsuLoader::ParseFileString(const std::string& fileContents)
{
	std::vector<std::string> sections;
	std::vector<std::vector<std::string>> contents;

	SeparateTagsAndContents(fileContents, sections, contents);

	std::map<std::string, std::map<std::string, std::string>> parsedData;
	bool colurz = (sections.size() == 8 &&
				   std::count(sections.begin(), sections.end(), "Colours"));
	if (sections.size() == 7 || colurz) {
		for (int i = 0; i < (int)sections.size(); ++i) {
			for (auto& content : contents[i]) {
				auto& str = content;
				int pos = str.find_first_of(':');
				std::string value = str.substr(pos + 1);
				std::string tag = str.substr(0, pos);
				parsedData[sections[i]][tag] = value;
			}
		}
	}
	return parsedData;
}

void
OsuLoader::SeparateTagsAndContents(
  std::string fileContents,
  std::vector<std::string>& tagsOut,
  std::vector<std::vector<std::string>>& contentsOut)
{
	char lastByte = '\0';
	bool isComment = false;
	bool isTag = false;
	bool isContent = false;
	std::string tag;
	std::string content;

	int tagIndex = 0;

	for (int i = 0; i < static_cast<int>(fileContents.length()); ++i) {
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
				contentsOut.emplace_back(std::vector<std::string>());
				isContent = true;
			} else {
				tag = tag + currentByte;
			}
		} else if (isContent) {
			if ((currentByte == '[' && lastByte == '\n') ||
				i == static_cast<int>(fileContents.length()) - 1) {
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
OsuLoader::SetMetadata(
  std::map<std::string, std::map<std::string, std::string>> parsedData,
  Song& out)
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
OsuLoader::SetTimingData(
  std::map<std::string, std::map<std::string, std::string>> parsedData,
  Song& out)
{
	std::vector<std::pair<int, float>> tp;

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

	std::vector<std::pair<int, float>> bpms;
	float lastpositivebpm = 0;
	int offset = 0;
	int lastoffset = -9999;
	for (auto x : tp) {
		float bpm = 0.f;
		offset = std::max(0, x.first);
		if (x.second > 0) {
			bpm = 60000 / x.second;
			lastpositivebpm = bpm;
		} else // inherited timing points; these might be able to be ignored
		{
			bpm = lastpositivebpm * std::abs(x.second / 100);
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
	if (!bpms.empty()) {
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
OsuLoader::LoadChartData(
  Song* song,
  Steps* chart,
  std::map<std::string, std::map<std::string, std::string>> parsedData)
{
	if (stoi(parsedData["General"]["Mode"]) != 3 ||
		parsedData.find("HitObjects") ==
		  parsedData.end()) // if the mode isn't mania or notedata is empty
	{
		return false;
	}

	switch (stoi(parsedData["Difficulty"]["CircleSize"])) {
		case (3): {
			chart->m_StepsType = StepsType_dance_threepanel;
			break;
		}
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
		case (9): {
			chart->m_StepsType = StepsType_popn_nine;
			break;
		}
		case (10): {
			chart->m_StepsType = StepsType_pump_double;
			break;
		}
		default:
			chart->m_StepsType = StepsType_Invalid;
			return false;
	}

	chart->SetMeter(static_cast<int>(song->GetAllSteps().size()));

	chart->SetDifficulty(static_cast<Difficulty>(
	  std::min(song->GetAllSteps().size(), static_cast<size_t>(Difficulty_Edit))));

	chart->TidyUpData();

	chart->SetSavedToDisk(true);

	return true;
}

void
OsuLoader::GetApplicableFiles(const std::string& sPath,
							  std::vector<std::string>& out)
{
	GetDirListing(sPath + std::string("*.osu"), out);
}

int
OsuLoader::MsToNoteRow(int ms, Song* song)
{
	float tempOffset = song->m_SongTiming.m_fBeat0OffsetInSeconds;
	song->m_SongTiming.m_fBeat0OffsetInSeconds = 0;

	int row = static_cast<int>(std::round(std::abs(
	  song->m_SongTiming.GetBeatFromElapsedTimeNoOffset(static_cast<float>(ms) / 1000.f) * 48)));

	song->m_SongTiming.m_fBeat0OffsetInSeconds = tempOffset;

	return row;
	// wow
	// this is fucking hideous
	// but it works
}

void
OsuLoader::LoadNoteDataFromParsedData(
  Steps* out,
  std::map<std::string, std::map<std::string, std::string>> parsedData)
{
	const auto keymode = stoi(parsedData["Difficulty"]["CircleSize"]);
	auto lane = [&keymode](int lane) { return lane / (512 / keymode); };

	NoteData newNoteData;
	newNoteData.SetNumTracks(keymode);

	std::vector<OsuNote> taps;
	std::vector<OsuHold> holds;
	bool useLifts = PREFSMAN->LiftsOnOsuHolds;
	for (auto& it : parsedData["HitObjects"]) {
		auto line = it.first;
		auto values = split(line, ",");
		unsigned int type = stoi(values[3]);
		if (type == 128)
			holds.emplace_back(
			  OsuHold(stoi(values[2]), stoi(values[5]), stoi(values[0])));
		else if ((type & 1u) == 1)
			taps.emplace_back(OsuNote(stoi(values[2]), stoi(values[0])));
	}

	sort(taps.begin(), taps.end(), [](OsuNote a, OsuNote b) {
		return a.ms < b.ms;
	});
	sort(holds.begin(), holds.end(), [](OsuHold a, OsuHold b) {
		return a.msStart < b.msStart;
	});

	int firstTap = 0;
	if (!taps.empty() && !holds.empty()) {
		firstTap = std::min(taps[0].ms, holds[0].msStart);
	} else if (!taps.empty()) {
		firstTap = taps[0].ms;
	} else {
		firstTap = holds[0].msStart;
	}

	for (auto& tap : taps) {
		newNoteData.SetTapNote(lane(tap.lane),
							   MsToNoteRow(tap.ms - firstTap, out->m_pSong),
							   TAP_ORIGINAL_TAP);
	}
	for (auto& hold : holds) {
		int start = MsToNoteRow(hold.msStart - firstTap, out->m_pSong);
		int end = MsToNoteRow(hold.msEnd - firstTap, out->m_pSong);
		if (end - start > 0 && useLifts) {
			end = end - 1;
		}
		if (end <= start) {
			// if the hold is a 0 or negative length hold, it is a tap
			newNoteData.SetTapNote(lane(hold.lane), start, TAP_ORIGINAL_TAP);
			continue;
		}
		newNoteData.AddHoldNote(
		  lane(hold.lane), start, end, TAP_ORIGINAL_HOLD_HEAD);
		if (useLifts)
			newNoteData.SetTapNote(lane(hold.lane), end + 1, TAP_ORIGINAL_LIFT);
	}

	out->m_pSong->m_SongTiming.m_fBeat0OffsetInSeconds = -firstTap / 1000.0f;

	out->SetNoteData(newNoteData);
}

bool
OsuLoader::LoadNoteDataFromSimfile(const std::string& path, Steps& out)
{
	RageFile f;
	if (!f.Open(path)) {
		return false;
	}

	std::string fileRStr;
	fileRStr.reserve(f.GetFileSize());
	f.Read(fileRStr, -1);

	std::string fileStr = fileRStr;
	auto parsedData = ParseFileString(fileStr);
	LoadNoteDataFromParsedData(&out, parsedData);

	return !out.IsNoteDataEmpty();
}

bool
OsuLoader::LoadFromDir(const std::string& sPath_, Song& out)
{
	std::vector<std::string> aFileNames;
	GetApplicableFiles(sPath_, aFileNames);

	RageFile f;
	std::map<std::string, std::map<std::string, std::string>> parsedData;

	for (auto& filename : aFileNames) {
		auto p = sPath_ + filename;

		if (!f.Open(p)) {
			continue;
		}
		std::string fileContents;
		f.Read(fileContents, -1);
		parsedData = ParseFileString(fileContents);
		if (parsedData.empty()) {
			continue;
		}
		if (out.m_SongTiming.empty()) {
			SetMetadata(parsedData, out);
			SetTimingData(parsedData, out);
		}
		auto* chart = out.CreateSteps();
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

	// this would force the game to auto convert all .osu to .ssc
	// out.Save(false);

	return true;
}
