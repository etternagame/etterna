/** @brief DWILoader - reads a Song from a .DWI file. */

#ifndef NOTES_LOADER_DWI_H
#define NOTES_LOADER_DWI_H

#include <set>

class Song;
class Steps;

/** @brief The DWILoader handles parsing the .dwi file. */
namespace DWILoader {
/**
 * @brief Retrieve the list of .dwi files.
 * @param sPath a const reference to the path on the hard drive to check.
 * @param out a vector of files found in the path.
 */
void
GetApplicableFiles(const std::string& sPath, vector<std::string>& out);
/**
 * @brief Attempt to load a song from a specified path.
 * @param sPath a const reference to the path on the hard drive to check.
 * @param out a reference to the Song that will retrieve the song information.
 * @param BlacklistedImages a set of images that aren't used.
 * @return its success or failure.
 */
bool
LoadFromDir(const std::string& sPath,
			Song& out,
			std::set<std::string>& BlacklistedImages);

bool
LoadNoteDataFromSimfile(const std::string& path, Steps& out);
}

#endif
