#ifndef NOTES_LOADER_OSU_H
#define NOTES_LOADER_OSU_H

#include <set>

class Song;
class Steps;

namespace OsuLoader
{
	void ParseFileString(string fileContents, Song &out);

	void SeparateTagsAndContents(string fileContents, vector<string> &tagsOut, vector<vector<string>> &contentsOut);

	void SetMetadata(map<string, vector<string>> file, Song &out);

	void GetApplicableFiles(const RString &sPath, vector<RString> &out);

	bool LoadFromDir(const RString &sPath, Song &out);

	bool LoadNoteDataFromSimfile(const RString &path, Steps &out);
}

#endif
