#ifndef RAGE_FILE_MANAGER_H
#define RAGE_FILE_MANAGER_H

#include <string>
#include <vector>

/** @brief Constants for working with the RageFileManager. */
namespace RageFileManagerUtil {
extern std::string sInitialWorkingDirectory;
extern std::string sDirOfExecutable;
}

class RageFileDriver;
class RageFileBasic;
struct lua_State;

auto
ilt(const std::string& a, const std::string& b) -> bool;
auto
ieq(const std::string& a, const std::string& b) -> bool;

enum DirListingReturnFilter
{
	ANY_TYPE = 0,
	ONLY_DIR = 1,
	ONLY_FILE = 2,
};

/** @brief File utilities and high-level manager for RageFile objects. */
class RageFileManager
{
  public:
	RageFileManager(const std::string& argv0);
	~RageFileManager();

	void GetDirListing(const std::string& sPath,
					   std::vector<std::string>& AddTo,
					   DirListingReturnFilter dirListingFilter = ANY_TYPE,
					   bool bReturnPathToo = false);

	void GetDirListingWithMultipleExtensions(
	  const std::string& sPath,
	  std::vector<std::string> const& ExtensionList,
	  std::vector<std::string>& AddTo,
	  DirListingReturnFilter dirListingFilter = ANY_TYPE,
	  bool bReturnPathToo = false);

	// compatibility ....
	void GetDirListing(const std::string& sPath,
					   std::vector<std::string>& AddTo,
					   bool dirOnly,
					   bool bReturnPathToo)
	{
		GetDirListing(
		  sPath, AddTo, dirOnly ? ONLY_DIR : ANY_TYPE, bReturnPathToo);
	}

	// compatibility ....
	void GetDirListingWithMultipleExtensions(
	  const std::string& sPath,
	  std::vector<std::string> const& ExtensionList,
	  std::vector<std::string>& AddTo,
	  bool dirOnly,
	  bool bReturnPathToo)
	{
		GetDirListingWithMultipleExtensions(sPath,
											ExtensionList,
											AddTo,
											dirOnly ? ONLY_DIR : ANY_TYPE,
											bReturnPathToo);
	}

	auto Move(const std::string& sOldPath, const std::string& sNewPath) -> bool;
	auto Remove(const std::string& sPath) -> bool;
	void CreateDir(const std::string& sDir);

	enum FileType
	{
		TYPE_FILE,
		TYPE_DIR,
		TYPE_NONE
	};
	auto GetFileType(const std::string& sPath) -> FileType;

	auto IsAFile(const std::string& sPath) -> bool;
	auto IsADirectory(const std::string& sPath) -> bool;
	auto DoesFileExist(const std::string& sPath) -> bool;

	auto GetFileSizeInBytes(const std::string& sPath) -> int;
	auto GetFileHash(const std::string& sPath) -> int;

	/**
	 * @brief Get the absolte path from the VPS.
	 * @param path the VPS path.
	 * @return the absolute path. */
	auto ResolvePath(const std::string& path) -> std::string;

	// This function fails if the path given is in AdditionalSongs
	// when using multiple AdditionalSongs mount points
	auto ResolveSongFolder(const std::string& path,
						   bool additionalSongs = false) -> std::string;

	auto Mount(const std::string& sType,
			   const std::string& sRealPath,
			   const std::string& sMountPoint,
			   bool bAddToEnd = true) -> bool;
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
	auto IsMounted(const std::string& MountPoint) -> bool;
	struct DriverLocation
	{
		std::string Type, Root, MountPoint;
	};
	void GetLoadedDrivers(std::vector<DriverLocation>& asMounts);

	void FlushDirCache(const std::string& sPath = std::string());

	/* Used only by RageFile: */
	auto Open(const std::string& sPath, int iMode, int& iError)
	  -> RageFileBasic*;
	void CacheFile(const RageFileBasic* fb, const std::string& sPath);

	/* Retrieve or release a reference to the low-level driver for a mountpoint.
	 */
	auto GetFileDriver(std::string sMountpoint) -> RageFileDriver*;
	void ReleaseFileDriver(RageFileDriver* pDriver);

	// Lua
	void PushSelf(lua_State* L);

  private:
	auto OpenForReading(const std::string& sPath, int iMode, int& iError)
	  -> RageFileBasic*;
	auto OpenForWriting(const std::string& sPath, int iMode, int& iError)
	  -> RageFileBasic*;
};

extern RageFileManager* FILEMAN;

#endif
