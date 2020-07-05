#ifndef NOTES_WRITER_ETT_H
#define NOTES_WRITER_ETT_H

#include "Etterna/Globals/global.h"

class Song;
class Steps;

/** @brief Writes a Song to a .ETT file. */
namespace NotesWriterETT {
/**
 * @brief Write the song out to a file.
 * @param sPath the path to write the file.
 * @param out the Song to be written out.
 * @param vpStepsToSave the Steps to save.
 * @param bSavingCache a flag to see if we're saving certain cache data.
 * @return its success or failure. */
bool
Write(std::string& sPath, const Song& out, const vector<Steps*>& vpStepsToSave);
}

#endif
