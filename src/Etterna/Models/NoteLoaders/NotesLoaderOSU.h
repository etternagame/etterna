#ifndef NOTES_LOADER_OSU_H
#define NOTES_LOADER_OSU_H

class Song;
class Steps;

struct OsuHold
{
	int msStart;
	int msEnd;
	int lane;
	OsuHold(int msStart_, int msEnd_, int lane_)
	{
		msStart = msStart_;
		msEnd = msEnd_;
		lane = lane_;
	}
};
struct OsuNote
{
	int ms;
	int lane;
	OsuNote(int ms_, int lane_)
	{
		ms = ms_;
		lane = lane_;
	}
};

namespace OsuLoader {

// these are not organised AT ALL

std::map<std::string, std::map<std::string, std::string>>
ParseFileString(std::string fileContents);

void
SeparateTagsAndContents(std::string fileContents,
						vector<std::string>& tagsOut,
						vector<vector<std::string>>& contentsOut);

void
SetMetadata(std::map<std::string, std::map<std::string, std::string>>,
			Song& out);
void
SetTimingData(std::map<std::string, std::map<std::string, std::string>>,
			  Song& out);

void
GetApplicableFiles(const std::string& sPath, vector<std::string>& out);

bool
LoadFromDir(const std::string& sPath, Song& out);

void
LoadNoteDataFromParsedData(
  Steps* out,
  std::map<std::string, std::map<std::string, std::string>> parsedData);

bool
LoadNoteDataFromSimfile(const std::string& path, Steps& out);

bool
LoadChartData(
  Song* song,
  Steps* chart,
  std::map<std::string, std::map<std::string, std::string>> parsedData);

int
MsToNoteRow(int ms, Song* song);
}

#endif
