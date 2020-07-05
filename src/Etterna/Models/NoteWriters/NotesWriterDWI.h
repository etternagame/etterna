#ifndef NOTES_WRITER_DWI_H
#define NOTES_WRITER_DWI_H

class Song;

/** @brief Writes a Song to a .DWI file. */
namespace NotesWriterDWI {
/**
 * @brief Write the song out to a file.
 * @param sPath the path to write the file.
 * @param out the Song to be written out.
 * @return its success or failure. */
bool
Write(const std::string& sPath, const Song& out);
}

#endif
