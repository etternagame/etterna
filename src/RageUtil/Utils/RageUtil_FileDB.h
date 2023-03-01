#ifndef RAGE_UTIL_FILEDB
#define RAGE_UTIL_FILEDB

#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Misc/RageTimer.h"
#include "RageUtil/Utils/RageUtil.h"

#include <map>
#include <set>

struct FileSet;
struct File
{
	std::string name;
	std::string lname;

	void SetName(const std::string& fn)
	{
		name = fn;
		lname = name;
		MakeLower(lname);
	}

	bool dir;
	int size;
	/* Modification time of the file.  The contents of this is undefined, except
	 * that when the file has been modified, this value will change. */
	int hash;

	/* Private data, for RageFileDrivers. */
	void* priv;

	/* If this is non-NULL, and dir is true, this is a pointer to the FileSet
	 * containing the directory contents.  (This is a cache; it isn't always
	 * set.) */
	const FileSet* dirp;

	File()
	{
		dir = false;
		dirp = nullptr;
		size = -1;
		hash = -1;
		priv = nullptr;
	}
	File(const std::string& fn)
	{
		SetName(fn);
		dir = false;
		size = -1;
		hash = -1;
		priv = nullptr;
		dirp = nullptr;
	}

	bool operator<(const File& rhs) const { return lname < rhs.lname; }

	bool equal(const File& rhs) const { return lname == rhs.lname; }
	bool equal(const std::string& rhs) const
	{
		std::string l = make_lower(rhs);
		return lname == l;
	}
};

inline bool
operator==(File const& lhs, File const& rhs)
{
	return lhs.lname == rhs.lname;
}
inline bool
operator!=(File const& lhs, File const& rhs)
{
	return !operator==(lhs, rhs);
}

/** @brief This represents a directory. */
struct FileSet
{
	std::set<File> files;
	RageTimer age;

	/*
	 * If m_bFilled is false, this FileSet hasn't completed being filled in yet;
	 * it's owned by the thread filling it in.  Wait on FilenameDB::m_Mutex and
	 * retry until it becomes true.
	 */
	bool m_bFilled;

	FileSet() { m_bFilled = true; }

	void GetFilesMatching(const std::string& sBeginning,
						  const std::string& sContaining,
						  const std::string& sEnding,
						  std::vector<std::string>& asOut,
						  DirListingReturnFilter returnFilter) const;
	void GetFilesEqualTo(const std::string& pat,
						 std::vector<std::string>& out,
						 DirListingReturnFilter returnFilter) const;

	RageFileManager::FileType GetFileType(const std::string& sPath) const;
	int GetFileSize(const std::string& sPath) const;
	int GetFileHash(const std::string& sPath) const;
};
/** @brief A container for a file listing. */
class FilenameDB
{
  public:
	FilenameDB()
	  : m_Mutex("FilenameDB")
	{
	}
	virtual ~FilenameDB() { FlushDirCache(); }

	void AddFile(const std::string& sPath,
				 int iSize,
				 int iHash,
				 void* pPriv = nullptr);
	void DelFile(const std::string& sPath);
	void* GetFilePriv(const std::string& sPath);

	/* This handles at most two * wildcards.  If we need anything more
	 * complicated, we'll need to use fnmatch or regex. */
	void GetFilesSimpleMatch(const std::string& sDir,
							 const std::string& sFile,
							 std::vector<std::string>& asOut,
							 DirListingReturnFilter returnFilter);

	/* Search for "path" case-insensitively and replace it with the correct
	 * case.  If only a portion of the path exists, resolve as much as possible.
	 * Return true if the entire path was matched. */
	bool ResolvePath(std::string& sPath);

	RageFileManager::FileType GetFileType(const std::string& sPath);
	int GetFileSize(const std::string& sPath);
	int GetFileHash(const std::string& sFilePath);
	void GetDirListing(const std::string& sPath,
					   std::vector<std::string>& asAddTo,
					   DirListingReturnFilter returnFilter,
					   bool bReturnPathToo);

	void FlushDirCache(const std::string& sDir = std::string());

	void GetFileSetCopy(const std::string& dir, FileSet& out);
	/* Probably slow, so override it. */
	virtual void CacheFile(const std::string& sPath);

  protected:
	RageEvent m_Mutex;

	const File* GetFile(const std::string& sPath);
	FileSet* GetFileSet(const std::string& sDir, bool create = true);

	/* Directories we have cached: */
	std::map<std::string, FileSet*> dirs;

	int ExpireSeconds{ -1 };

	void GetFilesEqualTo(const std::string& sDir,
						 const std::string& sName,
						 std::vector<std::string>& asOut,
						 DirListingReturnFilter returnFilter);

	void GetFilesMatching(const std::string& sDir,
						  const std::string& sBeginning,
						  const std::string& sContaining,
						  const std::string& sEnding,
						  std::vector<std::string>& asOut,
						  DirListingReturnFilter returnFilter);

	void DelFileSet(std::map<std::string, FileSet*>::iterator dir);

	/* The given path wasn't cached.  Cache it. */
	virtual void PopulateFileSet(FileSet& /* fs */,
								 const std::string& /* sPath */)
	{
	}
};

/* This FilenameDB must be populated in advance. */
class NullFilenameDB : public FilenameDB
{
  public:
	NullFilenameDB() { ExpireSeconds = -1; }
	void CacheFile(const std::string& /* sPath */) override {}
};

#endif
