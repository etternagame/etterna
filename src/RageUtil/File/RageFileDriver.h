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
	virtual RageFileBasic* Open(const std::string& sPath,
								int iMode,
								int& iError) = 0;
	virtual void GetDirListing(const std::string& sPath,
							   std::vector<std::string>& asAddTo,
							   DirListingReturnFilter returnFilter,
							   bool bReturnPathToo);
	virtual RageFileManager::FileType GetFileType(const std::string& sPath);
	virtual int GetFileSizeInBytes(const std::string& sFilePath);
	virtual int GetFileHash(const std::string& sPath);
	virtual int GetPathValue(const std::string& sPath);
	virtual void FlushDirCache(const std::string& sPath);
	virtual void CacheFile(const std::string& /* sPath */) {}
	virtual bool Move(const std::string& /* sOldPath */,
					  const std::string& /* sNewPath */)
	{
		return false;
	}
	virtual bool Remove(const std::string& /* sPath */) { return false; }

	/* Optional: Move to a different place, as if reconstructed with a different
	 * path. */
	virtual bool Remount(const std::string& /* sPath */) { return false; }

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
	FileDriverEntry(const std::string& sType);
	virtual ~FileDriverEntry();
	virtual RageFileDriver* Create(const std::string& sRoot) const = 0;

	std::string m_sType;
	const FileDriverEntry* m_pLink;
};
RageFileDriver*
MakeFileDriver(const std::string& Type, const std::string& Root);

#endif
