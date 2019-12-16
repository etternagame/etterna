#ifndef RAGE_FILE_MANAGER_READAHEAD_H
#define RAGE_FILE_MANAGER_READAHEAD_H

#include "RageFileBasic.h"

/** @brief Utilities for reading the RageFiles. */
namespace RageFileManagerReadAhead {
void
Init();
void
Shutdown();

// Nonblockingly read ahead iBytes in pFile, starting at the current file
// position.
void
ReadAhead(RageFileBasic* pFile, int iBytes);

/* Discard iBytes of kernel cache, starting at the current file position plus
 * iRelativePosition (which may be negative). */
void
DiscardCache(RageFileBasic* pFile, int iRelativePosition, int iBytes);

void
CacheHintStreaming(RageFileBasic* pFile);
};

#endif
