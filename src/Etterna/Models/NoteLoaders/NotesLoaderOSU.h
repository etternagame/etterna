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

std::map<string, std::map<string, string>>
ParseFileString(string fileContents);

void
SeparateTagsAndContents(string fileContents,
						std::vector<string>& tagsOut,
						std::vector<std::vector<string>>& contentsOut);

void
SetMetadata(std::map<string, std::map<string, string>>, Song& out);
void
SetTimingData(std::map<string, std::map<string, string>>, Song& out);

void
GetApplicableFiles(const RString& sPath, std::vector<RString>& out);

bool
LoadFromDir(const RString& sPath, Song& out);

void
LoadNoteDataFromParsedData(Steps* out,
						   std::map<string, std::map<string, string>> parsedData);

bool
LoadNoteDataFromSimfile(const RString& path, Steps& out);

bool
LoadChartData(Song* song,
			  Steps* chart,
			  std::map<string, std::map<string, string>> parsedData);

int
MsToNoteRow(int ms, Song* song);
}

#endif
