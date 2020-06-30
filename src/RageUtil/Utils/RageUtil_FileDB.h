#ifndef RAGE_UTIL_FILEDB
#define RAGE_UTIL_FILEDB

#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Misc/RageTimer.h"
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
		dirp = NULL;
		size = -1;
		hash = -1;
		priv = NULL;
	}
	File(const RString& fn)
	{
		SetName(fn);
		dir = false;
		size = -1;
		hash = -1;
		priv = NULL;
		dirp = NULL;
	}

	bool operator<(const File& rhs) const { return lname < rhs.lname; }

	bool equal(const File& rhs) const { return lname == rhs.lname; }
	bool equal(const RString& rhs) const
	{
		RString l = rhs;
		l.MakeLower();
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
	set<File> files;
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
						  vector<std::string>& asOut,
						  bool bOnlyDirs) const;
	void GetFilesMatching(const RString& sBeginning,
						  const RString& sContaining,
						  const RString& sEnding,
						  vector<RString>& asOut,
						  bool bOnlyDirs) const;
	void GetFilesEqualTo(const std::string& pat,
						 vector<std::string>& out,
						 bool bOnlyDirs) const;
	void GetFilesEqualTo(const RString& pat,
						 vector<RString>& out,
						 bool bOnlyDirs) const;

	RageFileManager::FileType GetFileType(const RString& sPath) const;
	int GetFileSize(const RString& sPath) const;
	int GetFileHash(const RString& sPath) const;
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

	void AddFile(const RString& sPath,
				 int iSize,
				 int iHash,
				 void* pPriv = NULL);
	void DelFile(const RString& sPath);
	void* GetFilePriv(const RString& sPath);

	/* This handles at most two * wildcards.  If we need anything more
	 * complicated, we'll need to use fnmatch or regex. */
	void GetFilesSimpleMatch(const RString& sDir,
							 const RString& sFile,
							 vector<RString>& asOut,
							 bool bOnlyDirs);

	void GetFilesSimpleMatch(const std::string& sDir,
							 const std::string& sFile,
							 vector<std::string>& asOut,
							 bool bOnlyDirs);

	/* Search for "path" case-insensitively and replace it with the correct
	 * case.  If only a portion of the path exists, resolve as much as possible.
	 * Return true if the entire path was matched. */
	bool ResolvePath(RString& sPath);

	RageFileManager::FileType GetFileType(const RString& sPath);
	int GetFileSize(const RString& sPath);
	int GetFileHash(const RString& sFilePath);
	void GetDirListing(const RString& sPath,
					   vector<RString>& asAddTo,
					   bool bOnlyDirs,
					   bool bReturnPathToo);
	void GetDirListing(const std::string& sPath,
					   vector<std::string>& asAddTo,
					   bool bOnlyDirs,
					   bool bReturnPathToo);

	void FlushDirCache(const RString& sDir = RString());

	void GetFileSetCopy(const RString& dir, FileSet& out);
	/* Probably slow, so override it. */
	virtual void CacheFile(const RString& sPath);

  protected:
	RageEvent m_Mutex;

	const File* GetFile(const RString& sPath);
	FileSet* GetFileSet(const RString& sDir, bool create = true);

	/* Directories we have cached: */
	map<RString, FileSet*> dirs;

	int ExpireSeconds{ -1 };

	void GetFilesEqualTo(const std::string& sDir,
						 const std::string& sName,
						 vector<std::string>& asOut,
						 bool bOnlyDirs);
	void GetFilesEqualTo(const RString& sDir,
						 const RString& sName,
						 vector<RString>& asOut,
						 bool bOnlyDirs);
	void GetFilesMatching(const std::string& sDir,
						  const std::string& sBeginning,
						  const std::string& sContaining,
						  const std::string& sEnding,
						  vector<std::string>& asOut,
						  bool bOnlyDirs);
	void GetFilesMatching(const RString& sDir,
						  const RString& sBeginning,
						  const RString& sContaining,
						  const RString& sEnding,
						  vector<RString>& asOut,
						  bool bOnlyDirs);
	void DelFileSet(map<RString, FileSet*>::iterator dir);

	/* The given path wasn't cached.  Cache it. */
	virtual void PopulateFileSet(FileSet& /* fs */, const RString& /* sPath */)
	{
	}
};

/* This FilenameDB must be populated in advance. */
class NullFilenameDB : public FilenameDB
{
  public:
	NullFilenameDB() { ExpireSeconds = -1; }
	void CacheFile(const RString& /* sPath */) override {}
};

#endif
