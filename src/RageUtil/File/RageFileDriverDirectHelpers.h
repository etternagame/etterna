/* RageFileDriverDirectHelpers - Internal helpers for RageFileDriverDirect. */

#ifndef RAGE_FILE_DRIVER_DIRECT_HELPERS_H
#define RAGE_FILE_DRIVER_DIRECT_HELPERS_H

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif

#define DoStat stat
#define DoMkdir mkdir
#define DoFindFirstFile FindFirstFile
#define DoRename rename
#define DoRemove remove
RString
DoPathReplace(const RString& sPath);

#ifdef _WIN32
bool
WinMoveFile(const RString& sOldPath, const RString& sNewPath);
#endif

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

bool
CreateDirectories(const RString& sPath);

#include "RageUtil/Utils/RageUtil_FileDB.h"
class DirectFilenameDB : public FilenameDB
{
  public:
	DirectFilenameDB(const RString& root);
	void SetRoot(const RString& root);
	void CacheFile(const RString& sPath) override;

  protected:
	void PopulateFileSet(FileSet& fs, const RString& sPath) override;
	RString root;
};

#endif
