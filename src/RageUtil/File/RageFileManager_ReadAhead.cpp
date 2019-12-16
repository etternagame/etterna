#include "Etterna/Globals/global.h"
#include "RageFileManager_ReadAhead.h"

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(HAVE_POSIX_FADVISE)
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

#else
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
}
void
RageFileManagerReadAhead::DiscardCache(RageFileBasic* pFile,
									   int iRelativePosition,
									   int iBytes)
{
}
#endif

void
RageFileManagerReadAhead::CacheHintStreaming(RageFileBasic* pFile)
{
#if defined(HAVE_POSIX_FADVISE)
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
#endif
}
