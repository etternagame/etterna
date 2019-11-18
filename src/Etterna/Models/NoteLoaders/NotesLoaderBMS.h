#ifndef NOTES_LOADER_BMS_H
#define NOTES_LOADER_BMS_H

class Song;
class Steps;
/** @brief Reads a Song from a set of .BMS files. */
namespace BMSLoader {
void
GetApplicableFiles(const RString& sPath, vector<RString>& out);
bool
LoadFromDir(const RString& sDir, Song& out);
bool
LoadNoteDataFromSimfile(const RString& cachePath, Steps& out);
}

#endif
