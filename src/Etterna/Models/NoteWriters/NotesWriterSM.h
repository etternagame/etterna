#ifndef NOTES_WRITER_SM_H
#define NOTES_WRITER_SM_H

class Song;
class Steps;

/** @brief Writes a Song to an .SM file. */
namespace NotesWriterSM {
/**
 * @brief Write the song out to a file.
 * @param sPath the path to write the file.
 * @param out the Song to be written out.
 * @return its success or failure. */
bool
Write(const std::string& sPath, Song& out, const vector<Steps*>& vpStepsToSave);
/**
 * @brief Get some contents about the edit file first.
 * @param pSong the Song in question.
 * @param pSteps the Steps in question.
 * @param sOut the start of the file contents.
 */
void
GetEditFileContents(const Song* pSong, const Steps* pSteps, std::string& sOut);
/**
 * @brief Get the name of the edit file to use.
 * @param pSong the Song in question.
 * @param pSteps the Steps in question.
 * @return the name of the edit file. */
std::string
GetEditFileName(const Song* pSong, const Steps* pSteps);
/**
 * @brief Write the edit file to the machine for future use.
 * @param pSong the Song in question.
 * @param pSteps the Steps in question.
 * @param sErrorOut any error messages that may have occurred.
 * @return its success or failure. */
bool
WriteEditFileToMachine(const Song* pSong,
					   Steps* pSteps,
					   std::string& sErrorOut);
}

#endif
