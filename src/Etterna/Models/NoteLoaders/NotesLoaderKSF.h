#ifndef NOTES_LOADER_KSF_H
#define NOTES_LOADER_KSF_H

class Song;
class Steps;
/** @brief Reads a Song from a set of .KSF files. */
namespace KSFLoader {
void
GetApplicableFiles(const std::string& sPath, vector<std::string>& out);
bool
LoadFromDir(const std::string& sDir, Song& out);
bool
LoadNoteDataFromSimfile(const std::string& cachePath, Steps& out);
}

#endif
