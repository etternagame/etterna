#ifndef RAGE_FILE_MANAGER_H
#define RAGE_FILE_MANAGER_H

/** @brief Constants for working with the RageFileManager. */
namespace RageFileManagerUtil {
extern std::string sInitialWorkingDirectory;
extern std::string sDirOfExecutable;
}

class RageFileDriver;
class RageFileBasic;
struct lua_State;

bool
ilt(const std::string& a, const std::string& b);
bool
ieq(const std::string& a, const std::string& b);

/** @brief File utilities and high-level manager for RageFile objects. */
class RageFileManager
{
  public:
	RageFileManager(const std::string& argv0);
	~RageFileManager();
	void MountInitialFilesystems();
	void MountUserFilesystems();

	void GetDirListing(const std::string& sPath,
					   vector<std::string>& AddTo,
					   bool bOnlyDirs,
					   bool bReturnPathToo);

	void GetDirListingWithMultipleExtensions(
	  const std::string& sPath,
	  vector<std::string> const& ExtensionList,
	  vector<std::string>& AddTo,
	  bool bOnlyDirs = false,
	  bool bReturnPathToo = false);

	bool Move(const std::string& sOldPath, const std::string& sNewPath);
	bool Remove(const std::string& sPath);
	void CreateDir(const std::string& sDir);

	enum FileType
	{
		TYPE_FILE,
		TYPE_DIR,
		TYPE_NONE
	};
	FileType GetFileType(const std::string& sPath);

	bool IsAFile(const std::string& sPath);
	bool IsADirectory(const std::string& sPath);
	bool DoesFileExist(const std::string& sPath);

	int GetFileSizeInBytes(const std::string& sPath);
	int GetFileHash(const std::string& sPath);

	/**
	 * @brief Get the absolte path from the VPS.
	 * @param path the VPS path.
	 * @return the absolute path. */
	std::string ResolvePath(const std::string& path);

	bool Mount(const std::string& sType,
			   const std::string& sRealPath,
			   const std::string& sMountPoint,
			   bool bAddToEnd = true);
	void Mount(RageFileDriver* pDriver,
			   const std::string& sMountPoint,
			   bool bAddToEnd = true);
	void Unmount(const std::string& sType,
				 const std::string& sRoot,
				 const std::string& sMountPoint);

	/* Change the root of a filesystem.  Only a couple drivers support this;
	 * it's used to change memory card mountpoints without having to actually
	 * unmount the driver. */
	void Remount(const std::string& sMountpoint, const std::string& sPath);
	bool IsMounted(const std::string& MountPoint);
	struct DriverLocation
	{
		std::string Type, Root, MountPoint;
	};
	void GetLoadedDrivers(vector<DriverLocation>& asMounts);

	void FlushDirCache(const std::string& sPath = std::string());

	/* Used only by RageFile: */
	RageFileBasic* Open(const std::string& sPath, int iMode, int& iError);
	void CacheFile(const RageFileBasic* fb, const std::string& sPath);

	/* Retrieve or release a reference to the low-level driver for a mountpoint.
	 */
	RageFileDriver* GetFileDriver(std::string sMountpoint);
	void ReleaseFileDriver(RageFileDriver* pDriver);

	// Lua
	void PushSelf(lua_State* L);

  private:
	RageFileBasic* OpenForReading(const std::string& sPath,
								  int iMode,
								  int& iError);
	RageFileBasic* OpenForWriting(const std::string& sPath,
								  int iMode,
								  int& iError);
};

extern RageFileManager* FILEMAN;

#endif
