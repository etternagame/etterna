#ifndef RAGE_FILE_MANAGER_H
#define RAGE_FILE_MANAGER_H

#include <vector>

/** @brief Constants for working with the RageFileManager. */
namespace RageFileManagerUtil {
extern RString sInitialWorkingDirectory;
extern RString sDirOfExecutable;
}

class RageFileDriver;
class RageFileBasic;
struct lua_State;

bool
ilt(const RString& a, const RString& b);
bool
ieq(const RString& a, const RString& b);

/** @brief File utilities and high-level manager for RageFile objects. */
class RageFileManager
{
  public:
	RageFileManager(const RString& argv0);
	~RageFileManager();
	void MountInitialFilesystems();
	void MountUserFilesystems();

	void GetDirListing(const RString& sPath,
					   std::vector<RString>& AddTo,
					   bool bOnlyDirs,
					   bool bReturnPathToo);
	void GetDirListingWithMultipleExtensions(
	  const RString& sPath,
	  std::vector<RString> const& ExtensionList,
	  std::vector<RString>& AddTo,
	  bool bOnlyDirs = false,
	  bool bReturnPathToo = false);
	bool Move(const RString& sOldPath, const RString& sNewPath);
	bool Remove(const RString& sPath);
	bool DeleteRecursive(const RString& sPath);
	void CreateDir(const RString& sDir);

	enum FileType
	{
		TYPE_FILE,
		TYPE_DIR,
		TYPE_NONE
	};
	FileType GetFileType(const RString& sPath);

	bool IsAFile(const RString& sPath);
	bool IsADirectory(const RString& sPath);
	bool DoesFileExist(const RString& sPath);

	int GetFileSizeInBytes(const RString& sPath);
	int GetFileHash(const RString& sPath);

	/**
	 * @brief Get the absolte path from the VPS.
	 * @param path the VPS path.
	 * @return the absolute path. */
	RString ResolvePath(const RString& path);

	bool Mount(const RString& sType,
			   const RString& sRealPath,
			   const RString& sMountPoint,
			   bool bAddToEnd = true);
	void Mount(RageFileDriver* pDriver,
			   const RString& sMountPoint,
			   bool bAddToEnd = true);
	void Unmount(const RString& sType,
				 const RString& sRoot,
				 const RString& sMountPoint);

	/* Change the root of a filesystem.  Only a couple drivers support this;
	 * it's used to change memory card mountpoints without having to actually
	 * unmount the driver. */
	void Remount(const RString& sMountpoint, const RString& sPath);
	bool IsMounted(const RString& MountPoint);
	struct DriverLocation
	{
		RString Type, Root, MountPoint;
	};
	void GetLoadedDrivers(std::vector<DriverLocation>& asMounts);

	void FlushDirCache(const RString& sPath = RString());

	/* Used only by RageFile: */
	RageFileBasic* Open(const RString& sPath, int iMode, int& iError);
	void CacheFile(const RageFileBasic* fb, const RString& sPath);

	/* Retrieve or release a reference to the low-level driver for a mountpoint.
	 */
	RageFileDriver* GetFileDriver(RString sMountpoint);
	void ReleaseFileDriver(RageFileDriver* pDriver);

	// Lua
	void PushSelf(lua_State* L);

  private:
	RageFileBasic* OpenForReading(const RString& sPath, int iMode, int& iError);
	RageFileBasic* OpenForWriting(const RString& sPath, int iMode, int& iError);
};

extern RageFileManager* FILEMAN;

#endif
