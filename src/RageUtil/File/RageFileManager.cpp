#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageFile.h"
#include "RageFileDriver.h"
#include "RageFileManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Utils/RageUtil_FileDB.h"

#include <cerrno>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <paths.h>
#endif

RageFileManager* FILEMAN = nullptr;

/* Lock this before touching any of these globals (except FILEMAN itself). */
static RageEvent* g_Mutex;

std::string RageFileManagerUtil::sInitialWorkingDirectory;
std::string RageFileManagerUtil::sDirOfExecutable;

struct LoadedDriver
{
	/* A loaded driver may have a base path, which modifies the path we
	 * pass to the driver.  For example, if the base is "Songs/", and we
	 * want to send the path "Songs/Foo/Bar" to it, then we actually
	 * only send "Foo/Bar".  The path "Themes/Foo" is out of the scope
	 * of the driver, and GetPath returns false. */
	RageFileDriver* m_pDriver;
	std::string m_sType, m_sRoot, m_sMountPoint;

	int m_iRefs;

	LoadedDriver()
	{
		m_pDriver = nullptr;
		m_iRefs = 0;
	}
	std::string GetPath(const std::string& sPath) const;
};

static std::vector<LoadedDriver*> g_pDrivers;
static std::map<const RageFileBasic*, LoadedDriver*> g_mFileDriverMap;

static void
ReferenceAllDrivers(std::vector<LoadedDriver*>& apDriverList)
{
	g_Mutex->Lock();
	apDriverList = g_pDrivers;
	for (auto& i : apDriverList)
		++i->m_iRefs;
	g_Mutex->Unlock();
}

static void
UnreferenceAllDrivers(std::vector<LoadedDriver*>& apDriverList)
{
	g_Mutex->Lock();
	for (auto& i : apDriverList)
		--i->m_iRefs;
	g_Mutex->Broadcast();
	g_Mutex->Unlock();

	/* Clear the temporary list, to make it clear that the drivers may no longer
	 * be accessed. */
	apDriverList.clear();
}

RageFileDriver*
RageFileManager::GetFileDriver(std::string sMountpoint)
{
	FixSlashesInPlace(sMountpoint);
	ensure_slash_at_end(sMountpoint);

	g_Mutex->Lock();
	RageFileDriver* pRet = nullptr;
	for (auto& g_pDriver : g_pDrivers) {
		if (g_pDriver->m_sType == "mountpoints")
			continue;
		if (CompareNoCase(g_pDriver->m_sMountPoint, sMountpoint))
			continue;

		pRet = g_pDriver->m_pDriver;
		++g_pDriver->m_iRefs;
		break;
	}
	g_Mutex->Unlock();

	return pRet;
}

void
RageFileManager::ReleaseFileDriver(RageFileDriver* pDriver)
{
	ASSERT(pDriver != NULL);

	g_Mutex->Lock();
	unsigned i;
	for (i = 0; i < g_pDrivers.size(); ++i) {
		if (g_pDrivers[i]->m_pDriver == pDriver)
			break;
	}
	ASSERT(i != g_pDrivers.size());

	--g_pDrivers[i]->m_iRefs;

	g_Mutex->Broadcast();
	g_Mutex->Unlock();
}

/* Wait for the given driver to become unreferenced, and remove it from the list
 * to get exclusive access to it.  Returns false if the driver is no longer
 * available (somebody else got it first). */
#if 0
static bool GrabDriver( RageFileDriver *pDriver )
{
	g_Mutex->Lock();

	for(;;)
	{
		unsigned i;
		for( i = 0; i < g_pDrivers.size(); ++i )
		{
			if( g_pDrivers[i]->m_pDriver == pDriver )
			{
				break;
			}
		}
		if( i == g_pDrivers.size() )
		{
			g_Mutex->Unlock();
			return false;
		}

		if( g_pDrivers[i]->m_iRefs == 0 )
		{
			g_pDrivers.erase( g_pDrivers.begin()+i );
			return true;
		}

		/* The driver is in use.  Wait for somebody to release a driver, and
		 * try again. */
		g_Mutex->Wait();
	}
}
#endif

// Mountpoints as directories cause a problem.  If "Themes/default" is a
// mountpoint, and doesn't exist anywhere else, then GetDirListing("Themes/*")
// must return "default".  The driver containing "Themes/default" won't do this;
// its world view begins at "BGAnimations" (inside "Themes/default").  We need a
// dummy driver that handles mountpoints. */
class RageFileDriverMountpoints : public RageFileDriver
{
  public:
	RageFileDriverMountpoints()
	  : RageFileDriver(new FilenameDB)
	{
	}
	RageFileBasic* Open(const std::string& sPath, int iMode, int& iError)
	{
		iError =
		  (iMode == RageFile::WRITE) ? ERROR_WRITING_NOT_SUPPORTED : ENOENT;
		return nullptr;
	}
	/* Never flush FDB, except in LoadFromDrivers. */
	void FlushDirCache(const std::string& sPath) {}

	void LoadFromDrivers(const std::vector<LoadedDriver*>& apDrivers)
	{
		/* XXX: Even though these two operations lock on their own, lock around
		 * them, too.  That way, nothing can sneak in and get incorrect
		 * results between the flush and the re-population. */
		FDB->FlushDirCache();
		for (unsigned i = 0; i < apDrivers.size(); ++i)
			if (apDrivers[i]->m_sMountPoint != "/")
				FDB->AddFile(apDrivers[i]->m_sMountPoint, 0, 0);
	}
};
static RageFileDriverMountpoints* g_Mountpoints = nullptr;

static std::string
ExtractDirectory(std::string sPath)
{
	// return the directory containing sPath
	size_t n = sPath.find_last_of('/');
	if (n != sPath.npos)
		sPath.erase(n);
	else
		sPath.erase();
	return sPath;
}

#if defined(__unix__) || defined(__APPLE__)
static std::string
ReadlinkRecursive(std::string sPath)
{
	// unices support symbolic links; dereference them
	std::string dereferenced = sPath;
	do {
		sPath = dereferenced;
		char derefPath[512];
		ssize_t linkSize =
		  readlink(sPath.c_str(), derefPath, sizeof(derefPath));
		if (linkSize != -1 && linkSize != sizeof(derefPath)) {
			dereferenced = std::string(derefPath, linkSize);
			if (derefPath[0] != '/') {
				// relative link
				dereferenced =
				  std::string(ExtractDirectory(sPath) + "/" + dereferenced);
			}
		}
	} while (sPath != dereferenced);

	return sPath;
}
#endif

static std::string
GetDirOfExecutable(std::string argv0)
{
	// argv[0] can be wrong in most OS's; try to avoid using it.

	std::string sPath;
#ifdef _WIN32
	char szBuf[MAX_PATH];
	GetModuleFileName(nullptr, szBuf, sizeof(szBuf));
	sPath = szBuf;
#else
	sPath = argv0;
#endif

	s_replace(sPath, "\\", "/");

	bool bIsAbsolutePath = false;
	if (sPath.empty() || sPath[0] == '/')
		bIsAbsolutePath = true;
#ifdef _WIN32
	if (sPath.size() > 2 && sPath[1] == ':' && sPath[2] == '/')
		bIsAbsolutePath = true;
#endif

	// strip off executable name
	sPath = ExtractDirectory(sPath);

	if (!bIsAbsolutePath) {
#if defined(__unix__) || defined(__APPLE__)
		if (sPath.empty()) {
			// This is in our path so look for it.
			const char* path = getenv("PATH");

			if (!path)
				path = _PATH_DEFPATH;

			std::vector<std::string> vPath;
			split(path, ":", vPath);
			for (auto& i : vPath) {
				if (access((i + "/" + argv0).c_str(), X_OK | R_OK))
					continue;
				sPath = ExtractDirectory(ReadlinkRecursive(i + "/" + argv0));
				break;
			}
			if (sPath.empty())
				sPath = GetCwd();	  // What?
			else if (sPath[0] != '/') // For example, if . is in $PATH.
				sPath = GetCwd() + "/" + sPath;

		} else {
			sPath = ExtractDirectory(ReadlinkRecursive(GetCwd() + "/" + argv0));
		}
#else
		sPath = GetCwd() + "/" + sPath;
		s_replace(sPath, "\\", "/");
#endif
	}
	return sPath;
}

static void
ChangeToDirOfExecutable(const std::string& argv0)
{
	RageFileManagerUtil::sInitialWorkingDirectory = GetCwd();
	RageFileManagerUtil::sDirOfExecutable = GetDirOfExecutable(argv0);

	/* Set the CWD.  Any effects of this is platform-specific; most files are
	 * read and written through RageFile.  See also
	 * RageFileManager::RageFileManager. */
#ifdef _WIN32
	if (_chdir(
		  std::string(RageFileManagerUtil::sDirOfExecutable + "/..").c_str()))
#elif defined(__unix__)
	if (chdir((RageFileManagerUtil::sDirOfExecutable + "/").c_str()))
#elif defined(__APPLE__)
	/* If the basename is not MacOS, then we've likely been launched via the
	 * command line through a symlink. Assume this is the case and change to the
	 * dir of the symlink. */
	if (Basename(RageFileManagerUtil::sDirOfExecutable) == "MacOS")
		CollapsePath(RageFileManagerUtil::sDirOfExecutable += "/../../../");
	if (chdir(RageFileManagerUtil::sDirOfExecutable.c_str()))
#endif
	{
		Locator::getLogger()->warn("Can't set current working directory to {}",
				  RageFileManagerUtil::sDirOfExecutable.c_str());
		return;
	}
}

RageFileManager::RageFileManager(const std::string& argv0)
{
	Locator::getLogger()->trace("{}", argv0.c_str());
	ChangeToDirOfExecutable(argv0);

	g_Mutex = new RageEvent("RageFileManager");

	g_Mountpoints = new RageFileDriverMountpoints;
	auto* pLoadedDriver = new LoadedDriver;
	pLoadedDriver->m_pDriver = g_Mountpoints;
	pLoadedDriver->m_sMountPoint = "/";
	pLoadedDriver->m_sType = "mountpoints";
	g_pDrivers.push_back(pLoadedDriver);

	/* The mount path is unused, but must be nonempty. */
	RageFileManager::Mount("mem", "(cache)", "/@mem");

	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "FILEMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

RageFileManager::~RageFileManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("FILEMAN");

	/* Note that drivers can use previously-loaded drivers, eg. to load a ZIP
	 * from the FS.  Unload drivers in reverse order. */
	for (int i = g_pDrivers.size() - 1; i >= 0; --i) {
		delete g_pDrivers[i]->m_pDriver;
		delete g_pDrivers[i];
	}
	g_pDrivers.clear();

	//	delete g_Mountpoints; // g_Mountpoints was in g_pDrivers
	g_Mountpoints = nullptr;

	delete g_Mutex;
	g_Mutex = nullptr;
}

/* path must be normalized (FixSlashesInPlace, CollapsePath). */
std::string
LoadedDriver::GetPath(const std::string& sPath) const
{
	/* If the path begins with /@, only match mountpoints that begin with /@. */
	if (sPath.size() >= 2 && sPath[1] == '@') {
		if (m_sMountPoint.size() < 2 || m_sMountPoint[1] != '@')
			return std::string();
	}

	if (CompareNoCase(sPath.substr(0, m_sMountPoint.size()), m_sMountPoint))
		return std::string(); /* no match */

	/* Add one, so we don't cut off the leading slash. */
	std::string sRet = tail(sPath, sPath.size() - m_sMountPoint.size() + 1);
	return sRet;
}

static inline void
NormalizePath(std::string& sPath)
{
	FixSlashesInPlace(sPath);
	CollapsePath(sPath, true);
	if (sPath.empty()) {
		sPath = '/';
	} else if (sPath[0] != '/') {
		sPath = '/' + sPath;
	}
}

inline bool
ilt(const std::string& a, const std::string& b)
{
	return CompareNoCase(a, b) < 0;
}

inline bool
ieq(const std::string& a, const std::string& b)
{
	return CompareNoCase(a, b) == 0;
}

/*
 * Helper function to remove all objects from an STL container for which the
 * Predicate pred is true. If you want to remove all objects for which the
 * predicate returns false, wrap the predicate with not1().
 */
template<typename Container, typename Predicate>
void
RemoveIf(Container& c, Predicate p)
{
	c.erase(remove_if(c.begin(), c.end(), p), c.end());
}

// remove various version control-related files
static inline bool
CVSOrSVN(const std::string& s)
{
	return EqualsNoCase(tail(s, 3), "CVS") ||
		   EqualsNoCase(tail(s, 4), ".svn") || EqualsNoCase(tail(s, 3), ".hg");
}

inline void
StripCvsAndSvn(std::vector<std::string>& vs)
{
	RemoveIf(vs, CVSOrSVN);
}

static inline bool
MacResourceFork(const std::string& s)
{
	return EqualsNoCase(s.substr(0, 2), "._") && s != "._Pulse.sm";
}

inline void
StripMacResourceForks(std::vector<std::string>& vs)
{
	RemoveIf(vs, MacResourceFork);
}

void
RageFileManager::GetDirListing(const std::string& sPath_,
							   std::vector<std::string>& AddTo,
							   bool bOnlyDirs,
							   bool bReturnPathToo)
{
	std::string sPath = sPath_;
	NormalizePath(sPath);

	// NormalizePath() calls CollapsePath() which will remove "dir/.." pairs.
	// So if a "/.." is still present, they're trying to go below the root,
	// which isn't valid.
	if (sPath.find("/..") != std::string::npos)
		return;

	std::vector<LoadedDriver*> apDriverList;
	ReferenceAllDrivers(apDriverList);

	int iDriversThatReturnedFiles = 0;
	int iOldSize = AddTo.size();
	for (auto pLoadedDriver : apDriverList) {
		const std::string p = pLoadedDriver->GetPath(sPath);
		if (p.empty())
			continue;

		const unsigned OldStart = AddTo.size();

		pLoadedDriver->m_pDriver->GetDirListing(
		  p, AddTo, bOnlyDirs, bReturnPathToo);
		if (AddTo.size() != OldStart)
			++iDriversThatReturnedFiles;

		/* If returning the path, prepend the mountpoint name to the files this
		 * driver returned. */
		if (bReturnPathToo && !pLoadedDriver->m_sMountPoint.empty()) {
			std::string const& mountPoint = pLoadedDriver->m_sMountPoint;
			/* Skip the trailing slash on the mountpoint; there's already a
			 * slash there. */
			std::string const& trimPoint =
			  mountPoint.substr(0, mountPoint.size() - 1);
			for (unsigned j = OldStart; j < AddTo.size(); ++j) {
				AddTo[j] = trimPoint + AddTo[j];
			}
		}
	}

	UnreferenceAllDrivers(apDriverList);
	StripCvsAndSvn(AddTo);
	StripMacResourceForks(AddTo);

	if (iDriversThatReturnedFiles > 1) {
		/* More than one driver returned files.  Remove duplicates
		 * (case-insensitively). */
		sort(AddTo.begin() + iOldSize, AddTo.end(), ilt);
		std::vector<std::string>::iterator it =
		  unique(AddTo.begin() + iOldSize, AddTo.end(), ieq);
		AddTo.erase(it, AddTo.end());
	}
}

void
RageFileManager::GetDirListingWithMultipleExtensions(
  const std::string& sPath,
  std::vector<std::string> const& ExtensionList,
  std::vector<std::string>& AddTo,
  bool bOnlyDirs,
  bool bReturnPathToo)
{
	std::vector<std::string> ret;
	GetDirListing(sPath + "*", ret, bOnlyDirs, bReturnPathToo);
	for (auto&& item : ret) {
		std::string item_ext = GetExtension(item);
		for (auto&& check_ext : ExtensionList) {
			if (item_ext == check_ext) {
				AddTo.push_back(item);
			}
		}
	}
}

/* Files may only be moved within the same file driver. */
bool
RageFileManager::Move(const std::string& sOldPath_,
					  const std::string& sNewPath_)
{
	std::string sOldPath = sOldPath_;
	std::string sNewPath = sNewPath_;

	std::vector<LoadedDriver*> aDriverList;
	ReferenceAllDrivers(aDriverList);

	NormalizePath(sOldPath);
	NormalizePath(sNewPath);

	/* Multiple drivers may have the same file. */
	bool Deleted = false;
	for (auto& i : aDriverList) {
		const std::string sOldDriverPath = i->GetPath(sOldPath);
		const std::string sNewDriverPath = i->GetPath(sNewPath);
		if (sOldDriverPath.empty() || sNewDriverPath.empty())
			continue;

		bool ret = i->m_pDriver->Move(sOldDriverPath, sNewDriverPath);
		if (ret)
			Deleted = true;
	}

	UnreferenceAllDrivers(aDriverList);

	return Deleted;
}

bool
RageFileManager::Remove(const std::string& sPath_)
{
	std::string sPath = sPath_;

	std::vector<LoadedDriver*> apDriverList;
	ReferenceAllDrivers(apDriverList);

	NormalizePath(sPath);

	/* Multiple drivers may have the same file. */
	bool bDeleted = false;
	for (auto& i : apDriverList) {
		const std::string p = i->GetPath(sPath);
		if (p.empty())
			continue;

		bool ret = i->m_pDriver->Remove(p);
		if (ret)
			bDeleted = true;
	}

	UnreferenceAllDrivers(apDriverList);

	return bDeleted;
}

void
RageFileManager::CreateDir(const std::string& sDir)
{
	std::string sTempFile = sDir + "newdir.temp.newdir";
	RageFile f;
	if (!f.Open(sTempFile, RageFile::WRITE))
		Locator::getLogger()->warn("Creating temporary file '{}' failed: {}",
				   sTempFile.c_str(),
				   f.GetError().c_str());
	f.Close();

	Remove(sTempFile);
}

static void
AdjustMountpoint(std::string& sMountPoint)
{
	FixSlashesInPlace(sMountPoint);

	ASSERT_M(sMountPoint.front() == '/',
			 "Mountpoints must be absolute: " + sMountPoint);

	if (sMountPoint.size() && sMountPoint.back() != '/')
		sMountPoint += '/';

	if (sMountPoint.front() != '/')
		sMountPoint = "/" + sMountPoint;
}

static void
AddFilesystemDriver(LoadedDriver* pLoadedDriver, bool bAddToEnd)
{
	g_Mutex->Lock();
	g_pDrivers.insert(bAddToEnd ? g_pDrivers.end() : g_pDrivers.begin(),
					  pLoadedDriver);
	g_Mountpoints->LoadFromDrivers(g_pDrivers);
	g_Mutex->Unlock();
}

bool
RageFileManager::Mount(const std::string& sType,
					   const std::string& sRoot_,
					   const std::string& sMountPoint_,
					   bool bAddToEnd)
{
	std::string sRoot = sRoot_;
	std::string sMountPoint = sMountPoint_;

	FixSlashesInPlace(sRoot);
	AdjustMountpoint(sMountPoint);

	ASSERT(!sRoot.empty());

	const std::string& sPaths = ssprintf("\"%s\", \"%s\", \"%s\"",
										 sType.c_str(),
										 sRoot.c_str(),
										 sMountPoint.c_str());
	Locator::getLogger()->debug("Driver MOUNT: {}", sPaths.c_str());
#if defined(DEBUG)
	puts(sPaths);
#endif

	// Unmount anything that was previously mounted here.
	Unmount(sType, sRoot, sMountPoint);

	Locator::getLogger()->trace("About to make a driver with \"{}\", \"{}\"",
						  sType.c_str(),
						  sRoot.c_str());
	RageFileDriver* pDriver = MakeFileDriver(sType, sRoot);
	if (pDriver == nullptr) {
		Locator::getLogger()->warn("Can't mount unknown VFS type \"{}\", root \"{}\"",
					  sType.c_str(),
					  sRoot.c_str());

		return false;
	}

	Locator::getLogger()->debug("Driver %s successfully made.");

	auto* pLoadedDriver = new LoadedDriver;
	pLoadedDriver->m_pDriver = pDriver;
	pLoadedDriver->m_sType = sType;
	pLoadedDriver->m_sRoot = sRoot;
	pLoadedDriver->m_sMountPoint = sMountPoint;

	AddFilesystemDriver(pLoadedDriver, bAddToEnd);
	return true;
}

/* Mount a custom filesystem. */
void
RageFileManager::Mount(RageFileDriver* pDriver,
					   const std::string& sMountPoint_,
					   bool bAddToEnd)
{
	std::string sMountPoint = sMountPoint_;

	AdjustMountpoint(sMountPoint);

	auto* pLoadedDriver = new LoadedDriver;
	pLoadedDriver->m_pDriver = pDriver;
	pLoadedDriver->m_sType = "";
	pLoadedDriver->m_sRoot = "";
	pLoadedDriver->m_sMountPoint = sMountPoint;

	AddFilesystemDriver(pLoadedDriver, bAddToEnd);
}

void
RageFileManager::Unmount(const std::string& sType,
						 const std::string& sRoot_,
						 const std::string& sMountPoint_)
{
	std::string sRoot = sRoot_;
	std::string sMountPoint = sMountPoint_;

	FixSlashesInPlace(sRoot);
	FixSlashesInPlace(sMountPoint);

	if (sMountPoint.size() && sMountPoint.back() != '/')
		sMountPoint += '/';

	/* Find all drivers we want to delete.  Remove them from g_pDrivers, and
	 * move them into aDriverListToUnmount. */
	std::vector<LoadedDriver*> apDriverListToUnmount;
	g_Mutex->Lock();
	for (unsigned i = 0; i < g_pDrivers.size(); ++i) {
		if (!sType.empty() && CompareNoCase(g_pDrivers[i]->m_sType, sType))
			continue;
		if (!sRoot.empty() && CompareNoCase(g_pDrivers[i]->m_sRoot, sRoot))
			continue;
		if (!sMountPoint.empty() &&
			CompareNoCase(g_pDrivers[i]->m_sMountPoint, sMountPoint))
			continue;

		++g_pDrivers[i]->m_iRefs;
		apDriverListToUnmount.push_back(g_pDrivers[i]);
		g_pDrivers.erase(g_pDrivers.begin() + i);
		--i;
	}

	g_Mountpoints->LoadFromDrivers(g_pDrivers);

	g_Mutex->Unlock();

	/* Now we have a list of drivers to remove. */
	while (!apDriverListToUnmount.empty()) {
		/* If the driver has more than one reference, somebody other than us is
		 * using it; wait for that operation to complete. Note that two
		 * Unmount() calls that want to remove the same mountpoint will deadlock
		 * here. */
		g_Mutex->Lock();
		while (apDriverListToUnmount[0]->m_iRefs > 1)
			g_Mutex->Wait();
		g_Mutex->Unlock();

		delete apDriverListToUnmount[0]->m_pDriver;
		delete apDriverListToUnmount[0];
		apDriverListToUnmount.erase(apDriverListToUnmount.begin());
	}
}

void
RageFileManager::Remount(const std::string& sMountpoint,
						 const std::string& sPath)
{
	RageFileDriver* pDriver = GetFileDriver(sMountpoint);
	if (pDriver == nullptr) {
		Locator::getLogger()->warn("Remount({},{}): mountpoint not found",
					  sMountpoint.c_str(),
					  sPath.c_str());
		return;
	}

	if (!pDriver->Remount(sPath))
		Locator::getLogger()->warn("Remount({},{}): remount failed (does the driver support "
				  "remounting?)",
				  sMountpoint.c_str(),
				  sPath.c_str());
	else
		pDriver->FlushDirCache("");

	ReleaseFileDriver(pDriver);
}

bool
RageFileManager::IsMounted(const std::string& MountPoint)
{
	LockMut(*g_Mutex);

	for (auto& g_pDriver : g_pDrivers)
		if (!CompareNoCase(g_pDriver->m_sMountPoint, MountPoint))
			return true;

	return false;
}

void
RageFileManager::GetLoadedDrivers(std::vector<DriverLocation>& asMounts)
{
	LockMut(*g_Mutex);

	for (auto& g_pDriver : g_pDrivers) {
		DriverLocation l;
		l.MountPoint = g_pDriver->m_sMountPoint;
		l.Type = g_pDriver->m_sType;
		l.Root = g_pDriver->m_sRoot;
		asMounts.push_back(l);
	}
}

void
RageFileManager::FlushDirCache(const std::string& sPath_)
{
	std::string sPath = sPath_;

	LockMut(*g_Mutex);

	if (sPath.empty()) {
		for (auto& g_pDriver : g_pDrivers)
			g_pDriver->m_pDriver->FlushDirCache("");
		return;
	}

	/* Flush a specific path. */
	NormalizePath(sPath);
	for (auto& g_pDriver : g_pDrivers) {
		const std::string& path = g_pDriver->GetPath(sPath);
		if (path.empty())
			continue;
		g_pDriver->m_pDriver->FlushDirCache(path);
	}
}

RageFileManager::FileType
RageFileManager::GetFileType(const std::string& sPath_)
{
	std::string sPath = sPath_;

	NormalizePath(sPath);

	std::vector<LoadedDriver*> apDriverList;
	ReferenceAllDrivers(apDriverList);

	RageFileManager::FileType ret = TYPE_NONE;
	for (auto& i : apDriverList) {
		const std::string p = i->GetPath(sPath);
		if (p.empty())
			continue;
		ret = i->m_pDriver->GetFileType(p);
		if (ret != TYPE_NONE)
			break;
	}

	UnreferenceAllDrivers(apDriverList);

	return ret;
}

int
RageFileManager::GetFileSizeInBytes(const std::string& sPath_)
{
	std::string sPath = sPath_;

	NormalizePath(sPath);

	std::vector<LoadedDriver*> apDriverList;
	ReferenceAllDrivers(apDriverList);

	int iRet = -1;
	for (auto& i : apDriverList) {
		const std::string p = i->GetPath(sPath);
		if (p.empty())
			continue;
		iRet = i->m_pDriver->GetFileSizeInBytes(p);
		if (iRet != -1)
			break;
	}
	UnreferenceAllDrivers(apDriverList);

	return iRet;
}

int
RageFileManager::GetFileHash(const std::string& sPath_)
{
	std::string sPath = sPath_;

	NormalizePath(sPath);

	std::vector<LoadedDriver*> apDriverList;
	ReferenceAllDrivers(apDriverList);

	int iRet = -1;
	for (auto& i : apDriverList) {
		const std::string p = i->GetPath(sPath);
		if (p.empty())
			continue;
		iRet = i->m_pDriver->GetFileHash(p);
		if (iRet != -1)
			break;
	}
	UnreferenceAllDrivers(apDriverList);

	return iRet;
}

std::string
RageFileManager::ResolvePath(const std::string& path)
{
	std::string tmpPath = path;
	NormalizePath(tmpPath);

	std::string resolvedPath = tmpPath;

	std::vector<LoadedDriver*> apDriverList;
	ReferenceAllDrivers(apDriverList);

	for (auto pDriver : apDriverList) {
		const std::string driverPath = pDriver->GetPath(tmpPath);

		if (driverPath.empty() || pDriver->m_sRoot.empty())
			continue;

		if (pDriver->m_sType != "dir" && pDriver->m_sType != "dirro")
			continue;

		int iMountPointLen = pDriver->m_sMountPoint.length();
		if (tmpPath.substr(0, iMountPointLen) != pDriver->m_sMountPoint)
			continue;

		resolvedPath =
		  pDriver->m_sRoot + "/" + std::string(tmpPath.substr(iMountPointLen));
		break;
	}

	UnreferenceAllDrivers(apDriverList);

	NormalizePath(resolvedPath);

	return resolvedPath;
}

std::string
RageFileManager::ResolveSongFolder(const std::string& path, bool additionalSongs)
{
	std::string tmpPath = path;
	NormalizePath(tmpPath);

	std::string resolvedPath = tmpPath;

	std::vector<LoadedDriver*> apDriverList;
	ReferenceAllDrivers(apDriverList);

	for (auto pDriver : apDriverList) {
		const std::string driverPath = pDriver->GetPath(tmpPath);

		if (driverPath.empty() || pDriver->m_sRoot.empty())
			continue;

		if (pDriver->m_sType != "dir" && pDriver->m_sType != "dirro")
			continue;

		// skip the root game folder if song is located in AdditionalSongs
		if (additionalSongs && pDriver->m_sMountPoint == "/")
			continue;
		
		int iMountPointLen = pDriver->m_sMountPoint.length();
		if (tmpPath.substr(0, iMountPointLen) != pDriver->m_sMountPoint)
			continue;

		resolvedPath =
		  pDriver->m_sRoot + "/" + std::string(tmpPath.substr(iMountPointLen));
		break;
	}

	UnreferenceAllDrivers(apDriverList);

	NormalizePath(resolvedPath);

	// on windows, remove the beginning / to give an absolute path
#ifdef _WIN32
	if (resolvedPath.length() > 0)
		resolvedPath.erase(0, 1);
#endif
	
	return resolvedPath;
}

static bool
SortBySecond(const std::pair<int, int>& a, const std::pair<int, int>& b)
{
	return a.second < b.second;
}

/*
 * Return true if the given path should use slow, reliable writes.
 *
 * I haven't decided if it's better to do this here, or to specify SLOW_FLUSH
 * manually each place we want it.  This seems more reliable (we might forget
 * somewhere and not notice), and easier (don't have to pass flags down to
 * IniFile::Write, etc).
 */
static bool
PathUsesSlowFlush(const std::string& sPath)
{
	static const char* FlushPaths[] = { "/Save/", "Save/" };

	for (unsigned i = 0; i < ARRAYLEN(FlushPaths); ++i)
		if (!strncmp(sPath.c_str(), FlushPaths[i], strlen(FlushPaths[i])))
			return true;
	return false;
}

/* Used only by RageFile: */
RageFileBasic*
RageFileManager::Open(const std::string& sPath_, int mode, int& err)
{
	std::string sPath = sPath_;

	err = ENOENT;

	if ((mode & RageFile::WRITE) && PathUsesSlowFlush(sPath))
		mode |= RageFile::SLOW_FLUSH;

	NormalizePath(sPath);

	/* If writing, we need to do a heuristic to figure out which driver to write
	 * with--there may be several that will work. */
	if (mode & RageFile::WRITE)
		return OpenForWriting(sPath, mode, err);
	else
		return OpenForReading(sPath, mode, err);
}

void
RageFileManager::CacheFile(const RageFileBasic* fb, const std::string& sPath_)
{
	std::map<const RageFileBasic*, LoadedDriver*>::iterator it =
	  g_mFileDriverMap.find(fb);

	ASSERT_M(it != g_mFileDriverMap.end(),
			 ssprintf("No recorded driver for file: %s", sPath_.c_str()));

	std::string sPath = sPath_;
	NormalizePath(sPath);
	sPath = it->second->GetPath(sPath);
	it->second->m_pDriver->FDB->CacheFile(sPath);
	g_mFileDriverMap.erase(it);
}

RageFileBasic*
RageFileManager::OpenForReading(const std::string& sPath, int mode, int& err)
{
	std::vector<LoadedDriver*> apDriverList;
	ReferenceAllDrivers(apDriverList);

	for (unsigned i = 0; i < apDriverList.size(); ++i) {
		LoadedDriver& ld = *apDriverList[i];
		const std::string path = ld.GetPath(sPath);
		if (path.empty())
			continue;
		int error;
		RageFileBasic* ret = ld.m_pDriver->Open(path, mode, error);
		if (ret) {
			UnreferenceAllDrivers(apDriverList);
			return ret;
		}

		/* ENOENT (File not found) is low-priority: if some other error
		 was reported, return that instead. */
		if (error != ENOENT)
			err = error;
	}
	UnreferenceAllDrivers(apDriverList);

	return nullptr;
}

RageFileBasic*
RageFileManager::OpenForWriting(const std::string& sPath, int mode, int& iError)
{
	/*
	 * The value for a driver to open a file is the number of directories and/or
	 * files that would have to be created in order to write it, or 0 if the
	 * file already exists. For example, if we're opening "foo/bar/baz.txt", and
	 * only "foo/" exists in a driver, we'd have to create the "bar" directory
	 * and the "baz.txt" file, so the value is 2.  If "foo/bar/" exists, we'd
	 * only have to create the file, so the value is 1.  Create the file with
	 * the driver that returns the lowest value; in case of a tie,
	 * earliest-loaded driver wins.
	 *
	 * The purpose of this is to create files in the expected place.  For
	 * example, if we have both C:/games/StepMania and C:/games/DWI loaded, and
	 * we're writing "Songs/Music/Waltz/waltz.ssc", and the song was loaded out
	 * of "C:/games/DWI/Songs/Music/Waltz/waltz.dwi", we want to write the new
	 * SSC into the same directory (if possible).  Don't split up files in the
	 * same directory any more than we have to.
	 *
	 * If the given path can not be created, return -1.  This happens if a path
	 * that needs to be a directory is a file, or vice versa.
	 */
	std::vector<LoadedDriver*> apDriverList;
	ReferenceAllDrivers(apDriverList);

	std::vector<std::pair<int, int>> Values;
	for (unsigned i = 0; i < apDriverList.size(); ++i) {
		LoadedDriver& ld = *apDriverList[i];
		const std::string path = ld.GetPath(sPath);
		if (path.empty())
			continue;

		const int value = ld.m_pDriver->GetPathValue(path);
		if (value == -1)
			continue;

		Values.push_back(std::make_pair(i, value));
	}

	std::stable_sort(Values.begin(), Values.end(), SortBySecond);

	/* Only write files if they'll be read.  If a file exists in any driver,
	 * don't create or write files in any driver mounted after it, because when
	 * we later try to read it, we'll get that file and not the one we wrote. */
	int iMaximumDriver = apDriverList.size();
	if (!Values.empty() && Values[0].second == 0)
		iMaximumDriver = Values[0].first;

	iError = 0;
	for (auto& Value : Values) {
		const int iDriver = Value.first;
		if (iDriver > iMaximumDriver)
			continue;
		LoadedDriver& ld = *apDriverList[iDriver];
		const std::string sDriverPath = ld.GetPath(sPath);
		ASSERT(!sDriverPath.empty());

		int iThisError;
		RageFileBasic* pRet = ld.m_pDriver->Open(sDriverPath, mode, iThisError);
		if (pRet) {
			g_mFileDriverMap[pRet] = &ld;
			UnreferenceAllDrivers(apDriverList);
			return pRet;
		}

		/* The drivers are in order of priority; if they all return error,
		 * return the first.  Never return ERROR_WRITING_NOT_SUPPORTED. */
		if (!iError &&
			iThisError != RageFileDriver::ERROR_WRITING_NOT_SUPPORTED)
			iError = iThisError;
	}

	if (!iError)
		iError = EEXIST; /* no driver could write */

	UnreferenceAllDrivers(apDriverList);

	return nullptr;
}

bool
RageFileManager::IsAFile(const std::string& sPath)
{
	return GetFileType(sPath) == TYPE_FILE;
}
bool
RageFileManager::IsADirectory(const std::string& sPath)
{
	return GetFileType(sPath) == TYPE_DIR;
}
bool
RageFileManager::DoesFileExist(const std::string& sPath)
{
	return GetFileType(sPath) != TYPE_NONE;
}

bool
DoesFileExist(const std::string& sPath)
{
	return FILEMAN->DoesFileExist(sPath);
}

bool
IsAFile(const std::string& sPath)
{
	return FILEMAN->IsAFile(sPath);
}

bool
IsADirectory(const std::string& sPath)
{
	return FILEMAN->IsADirectory(sPath);
}

int
GetFileSizeInBytes(const std::string& sPath)
{
	return FILEMAN->GetFileSizeInBytes(sPath);
}

void
GetDirListing(const std::string& sPath,
			  std::vector<std::string>& AddTo,
			  bool bOnlyDirs,
			  bool bReturnPathToo)
{
	FILEMAN->GetDirListing(sPath, AddTo, bOnlyDirs, bReturnPathToo);
}

void
GetDirListingRecursive(const std::string& sDir,
					   const std::string& sMatch,
					   std::vector<std::string>& AddTo)
{
	ASSERT(sDir.back() == '/');
	std::vector<std::string> vsFiles;
	GetDirListing(sDir + sMatch, vsFiles, false, true);
	std::vector<std::string> vsDirs;
	GetDirListing(sDir + "*", vsDirs, true, true);
	for (int i = 0; i < static_cast<int>(vsDirs.size()); i++) {
		GetDirListing(vsDirs[i] + "/" + sMatch, vsFiles, false, true);
		GetDirListing(vsDirs[i] + "/*", vsDirs, true, true);
		vsDirs.erase(vsDirs.begin() + i);
		i--;
	}
	for (int i = vsFiles.size() - 1; i >= 0; i--) {
		if (!IsADirectory(vsFiles[i]))
			AddTo.push_back(vsFiles[i]);
	}
}

void
GetDirListingRecursive(RageFileDriver* prfd,
					   const std::string& sDir,
					   const std::string& sMatch,
					   std::vector<std::string>& AddTo)
{
	ASSERT(sDir.back() == '/');
	std::vector<std::string> vsFiles;
	prfd->GetDirListing(sDir + sMatch, vsFiles, false, true);
	std::vector<std::string> vsDirs;
	prfd->GetDirListing(sDir + "*", vsDirs, true, true);
	for (int i = 0; i < static_cast<int>(vsDirs.size()); i++) {
		prfd->GetDirListing(vsDirs[i] + "/" + sMatch, vsFiles, false, true);
		prfd->GetDirListing(vsDirs[i] + "/*", vsDirs, true, true);
		vsDirs.erase(vsDirs.begin() + i);
		i--;
	}
	for (int i = vsFiles.size() - 1; i >= 0; i--) {
		if (prfd->GetFileType(vsFiles[i]) != RageFileManager::TYPE_DIR)
			AddTo.push_back(vsFiles[i]);
	}
}

unsigned int
GetHashForFile(const std::string& sPath)
{
	return FILEMAN->GetFileHash(sPath);
}

unsigned int
GetHashForDirectory(const std::string& sDir)
{
	unsigned int hash = 0;

	hash += GetHashForString(sDir);

	std::vector<std::string> arrayFiles;
	GetDirListing(sDir + "*", arrayFiles, false);
	for (auto& arrayFile : arrayFiles) {
		const std::string sFilePath = sDir + arrayFile;
		hash += GetHashForFile(sFilePath);
	}

	return hash;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the RageFileManager. */
class LunaRageFileManager : public Luna<RageFileManager>
{
  public:
	static int DoesFileExist(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->DoesFileExist(SArg(1)));
		return 1;
	}
	static int GetFileSizeBytes(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetFileSizeInBytes(SArg(1)));
		return 1;
	}
	static int GetHashForFile(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetFileHash(SArg(1)));
		return 1;
	}
	static int GetDirListing(T* p, lua_State* L)
	{
		std::vector<std::string> vDirs;
		bool bOnlyDirs = false;
		bool bReturnPathToo = false;

		// the last two arguments of GetDirListing are optional;
		// let's reflect that in the Lua too. -aj
		if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
			bOnlyDirs = BArg(2);
			if (!lua_isnil(L, 3)) {
				bReturnPathToo = BArg(3);
			}
		}
		//( Path, addTo, OnlyDirs=false, ReturnPathToo=false );
		p->GetDirListing(SArg(1), vDirs, bOnlyDirs, bReturnPathToo);
		LuaHelpers::CreateTableFromArray(vDirs, L);
		return 1;
	}
	static int FlushDirCache(T* p, lua_State* L)
	{
		auto dir = SArg(1);
		p->FlushDirCache(dir);
		return 0;
	}

	LunaRageFileManager()
	{
		ADD_METHOD(DoesFileExist);
		ADD_METHOD(GetFileSizeBytes);
		ADD_METHOD(GetHashForFile);
		ADD_METHOD(GetDirListing);
		ADD_METHOD(FlushDirCache);
	}
};

LUA_REGISTER_CLASS(RageFileManager)
// lua end
