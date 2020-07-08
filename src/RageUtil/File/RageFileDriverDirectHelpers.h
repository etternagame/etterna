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
std::string
DoPathReplace(const std::string& sPath);

#ifdef _WIN32
bool
WinMoveFile(const std::string& sOldPath, const std::string& sNewPath);
#endif

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

bool
CreateDirectories(const std::string& sPath);

#include "RageUtil/Utils/RageUtil_FileDB.h"
class DirectFilenameDB : public FilenameDB
{
  public:
	DirectFilenameDB(const std::string& root);
	void SetRoot(const std::string& root);
	void CacheFile(const std::string& sPath) override;

  protected:
	void PopulateFileSet(FileSet& fs, const std::string& sPath) override;
	std::string root;
};

#endif
