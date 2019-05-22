﻿#ifndef NOTES_WRITER_SM_H
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
Write(const RString& sPath, Song& out, const std::vector<Steps*>& vpStepsToSave);
/**
 * @brief Get some contents about the edit file first.
 * @param pSong the Song in question.
 * @param pSteps the Steps in question.
 * @param sOut the start of the file contents.
 */
void
GetEditFileContents(const Song* pSong, const Steps* pSteps, RString& sOut);
/**
 * @brief Get the name of the edit file to use.
 * @param pSong the Song in question.
 * @param pSteps the Steps in question.
 * @return the name of the edit file. */
RString
GetEditFileName(const Song* pSong, const Steps* pSteps);
/**
 * @brief Write the edit file to the machine for future use.
 * @param pSong the Song in question.
 * @param pSteps the Steps in question.
 * @param sErrorOut any error messages that may have occurred.
 * @return its success or failure. */
bool
WriteEditFileToMachine(const Song* pSong, Steps* pSteps, RString& sErrorOut);
}

#endif

/**
 * @file
 * @author Chris Danford, Glenn Maynard (c) 2001-2004
 * @section LICENSE
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
