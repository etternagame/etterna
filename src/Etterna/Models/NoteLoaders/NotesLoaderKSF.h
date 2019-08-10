#ifndef NOTES_LOADER_KSF_H
#define NOTES_LOADER_KSF_H

class Song;
class Steps;
/** @brief Reads a Song from a set of .KSF files. */
namespace KSFLoader {
void
GetApplicableFiles(const RString& sPath, vector<RString>& out);
bool
LoadFromDir(const RString& sDir, Song& out);
bool
LoadNoteDataFromSimfile(const RString& cachePath, Steps& out);
}

#endif
