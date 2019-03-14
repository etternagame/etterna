#include "Etterna/Globals/global.h"
#include "RageFileManager_ReadAhead.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

void
RageFileManagerReadAhead::Init()
{
}
void
RageFileManagerReadAhead::Shutdown()
{
}
void
RageFileManagerReadAhead::ReadAhead(RageFileBasic* pFile, int iBytes)
{
	int iFD = pFile->GetFD();
	if (iFD == -1)
		return;

	int iStart = lseek(iFD, 0, SEEK_CUR);
	posix_fadvise(iFD, iStart, iBytes, POSIX_FADV_WILLNEED);
}

void
RageFileManagerReadAhead::DiscardCache(RageFileBasic* pFile,
									   int iRelativePosition,
									   int iBytes)
{
	int iFD = pFile->GetFD();
	if (iFD == -1)
		return;

	int iStart = lseek(iFD, 0, SEEK_CUR);
	iStart += iRelativePosition;
	if (iStart < 0) {
		iBytes -= iStart;
		iStart = 0;
	}

	if (iBytes == 0)
		return;

	posix_fadvise(iFD, iStart, iBytes, POSIX_FADV_DONTNEED);
}


void
RageFileManagerReadAhead::CacheHintStreaming(RageFileBasic* pFile)
{
	/* This guesses at the actual size of the file on disk, which may be smaller
	 * if this file is compressed. Since this is usually used on music and video
	 * files, it generally shouldn't be. */
	int iFD = pFile->GetFD();
	if (iFD == -1)
		return;
	int iPos = pFile->Tell();
	int iFrom = lseek(iFD, 0, SEEK_CUR);
	int iBytes = pFile->GetFileSize() - iPos;
	posix_fadvise(iFD, iFrom, iBytes, POSIX_FADV_SEQUENTIAL);
}

/*
 * Copyright (c) 2010 Glenn Maynard
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
