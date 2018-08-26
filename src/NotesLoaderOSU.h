#ifndef NOTES_LOADER_OSU_H
#define NOTES_LOADER_OSU_H

#include <set>

class Song;
class Steps;

namespace OsuLoader
{
	map<string, map<string, string>> ParseFileString(string fileContents);

	void SeparateTagsAndContents(string fileContents, vector<string> &tagsOut, vector<vector<string>> &contentsOut);

	void SetMetadata(map<string, map<string, string>>, Song &out);

	void GetApplicableFiles(const RString &sPath, vector<RString> &out);

	bool LoadFromDir(const RString &sPath, Song &out);

	bool LoadNoteDataFromSimfile(const RString &path, Steps &out);
}

#endif
