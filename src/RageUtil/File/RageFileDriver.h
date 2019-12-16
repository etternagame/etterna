/* RageFileDriver - File driver base classes. */

#ifndef RAGE_FILE_DRIVER_H
#define RAGE_FILE_DRIVER_H

#include "RageFileManager.h"

class RageFileBasic;
class FilenameDB;

class RageFileDriver
{
  public:
	RageFileDriver(FilenameDB* pDB) { FDB = pDB; }
	virtual ~RageFileDriver();
	virtual RageFileBasic* Open(const RString& sPath,
								int iMode,
								int& iError) = 0;
	virtual void GetDirListing(const RString& sPath,
							   vector<RString>& asAddTo,
							   bool bOnlyDirs,
							   bool bReturnPathToo);
	virtual RageFileManager::FileType GetFileType(const RString& sPath);
	virtual int GetFileSizeInBytes(const RString& sFilePath);
	virtual int GetFileHash(const RString& sPath);
	virtual int GetPathValue(const RString& sPath);
	virtual void FlushDirCache(const RString& sPath);
	virtual void CacheFile(const RString& /* sPath */) {}
	virtual bool Move(const RString& /* sOldPath */,
					  const RString& /* sNewPath */)
	{
		return false;
	}
	virtual bool Remove(const RString& /* sPath */) { return false; }

	/* Optional: Move to a different place, as if reconstructed with a different
	 * path. */
	virtual bool Remount(const RString& /* sPath */) { return false; }

	/* Possible error returns from Open, in addition to standard errno.h values:
	 */
	enum
	{
		ERROR_WRITING_NOT_SUPPORTED = -1
	};
	// protected:
	FilenameDB* FDB;
};

/* This is used to register the driver, so RageFileManager can see it. */
struct FileDriverEntry
{
	FileDriverEntry(const RString& sType);
	virtual ~FileDriverEntry();
	virtual RageFileDriver* Create(const RString& sRoot) const = 0;

	RString m_sType;
	const FileDriverEntry* m_pLink;
};
RageFileDriver*
MakeFileDriver(const RString& Type, const RString& Root);

#endif
