#ifndef NOTES_LOADER_OSU_H
#define NOTES_LOADER_OSU_H

#include <set>

class Song;
class Steps;

namespace OsuLoader
{
	/**
	* @brief Retrieve the list of .dwi files.
	* @param sPath a const reference to the path on the hard drive to check.
	* @param out a vector of files found in the path.
	*/
	void GetApplicableFiles(const RString &sPath, vector<RString> &out);
	/**
	* @brief Attempt to load a song from a specified path.
	* @param sPath a const reference to the path on the hard drive to check.
	* @param out a reference to the Song that will retrieve the song information.
	* @param BlacklistedImages a set of images that aren't used.
	* @return its success or failure.
	*/
	bool LoadFromDir(const RString &sPath, Song &out);

	bool LoadNoteDataFromSimfile(const RString &path, Steps &out);
}

#endif
