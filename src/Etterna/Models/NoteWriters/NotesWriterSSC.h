#ifndef NOTES_WRITER_SSC_H
#define NOTES_WRITER_SSC_H

#include "Etterna/Models/StepsAndStyles/Steps.h"

class Song;
class Steps;
/** @brief Writes a Song to an .SSC file. */
namespace NotesWriterSSC {
/**
 * @brief Write the song out to a file.
 * @param sPath the path to write the file.
 * @param out the Song to be written out.
 * @param vpStepsToSave the Steps to save.
 * @param bSavingCache a flag to see if we're saving certain cache data.
 * @return its success or failure. */
bool
Write(std::string& sPath,
	  const Song& out,
	  const vector<Steps*>& vpStepsToSave,
	  bool bSavingCache);
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
std::string
MSDToString(const std::vector<std::vector<float>>& x);

std::string
MSDsAtRateToString(const std::vector<float>& x);
}

#endif
