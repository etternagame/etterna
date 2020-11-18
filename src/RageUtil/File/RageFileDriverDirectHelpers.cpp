#include "Etterna/Globals/global.h"
#include "RageFileDriverDirectHelpers.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Core/Services/Locator.hpp"
#include <sys/stat.h>

#include <cerrno>
#include <set>

#if !defined(_WIN32)

#if defined(HAVE_DIRENT_H)
#include <dirent.h>
#endif

#else
#include <windows.h>
#endif

std::string
DoPathReplace(const std::string& sPath)
{
	std::string TempPath = sPath;
	return TempPath;
}

#ifdef _WIN32
static bool
WinMoveFileInternal(const std::string& sOldPath, const std::string& sNewPath)
{
	static bool Win9x = false;

	/* Windows botches rename: it returns error if the file exists. In NT,
	 * we can use MoveFileEx( new, old, MOVEFILE_REPLACE_EXISTING ) (though I
	 * don't know if it has similar atomicity guarantees to rename). In
	 * 9x, we're screwed, so just delete any existing file (we aren't going
	 * to be robust on 9x anyway). */
	if (!Win9x) {
		if (MoveFileEx(
			  sOldPath.c_str(), sNewPath.c_str(), MOVEFILE_REPLACE_EXISTING))
			return true;

		// On Win9x, MoveFileEx is expected to fail (returns
		// ERROR_CALL_NOT_IMPLEMENTED).
		DWORD err = GetLastError();
		if (err == ERROR_CALL_NOT_IMPLEMENTED)
			Win9x = true;
		else
			return false;
	}

	if (MoveFile(sOldPath.c_str(), sNewPath.c_str()))
		return true;

	if (GetLastError() != ERROR_ALREADY_EXISTS)
		return false;

	if (!DeleteFile(sNewPath.c_str()))
		return false;

	return !!MoveFile(sOldPath.c_str(), sNewPath.c_str());
}

bool
WinMoveFile(const std::string& sOldPath, const std::string& sNewPath)
{
	if (WinMoveFileInternal(DoPathReplace(sOldPath), DoPathReplace(sNewPath)))
		return true;
	if (GetLastError() != ERROR_ACCESS_DENIED)
		return false;
	/* Try turning off the read-only bit on the file we're overwriting. */
	SetFileAttributes(DoPathReplace(sNewPath).c_str(), FILE_ATTRIBUTE_NORMAL);

	return WinMoveFileInternal(DoPathReplace(sOldPath),
							   DoPathReplace(sNewPath));
}
#endif

/* mkdir -p.  Doesn't fail if Path already exists and is a directory. */
bool
CreateDirectories(const std::string& Path)
{
	// XXX: handle "//foo/bar" paths in Windows
	vector<std::string> parts;
	std::string curpath;

	// If Path is absolute, add the initial slash ("ignore empty" will remove
	// it).
	if (Path.front() == '/')
		curpath = "/";

	// Ignore empty, so eg. "/foo/bar//baz" doesn't try to create "/foo/bar"
	// twice.
	split(Path, "/", parts, true);

	for (unsigned i = 0; i < parts.size(); ++i) {
		if (i)
			curpath += "/";
		curpath += parts[i];

#ifdef _WIN32
		if (curpath.size() == 2 && curpath[1] == ':') /* C: */
		{
			/* Don't try to create the drive letter alone. */
			continue;
		}
#endif

		if (DoMkdir(curpath.c_str(), 0777) == 0)
			continue;

#ifdef _WIN32
		/* When creating a directory that already exists over Samba, Windows is
		 * returning ENOENT instead of EEXIST. */
		/* I can't reproduce this anymore.  If we get ENOENT, log it but keep
		 * going. */
		if (errno == ENOENT) {
			Locator::getLogger()->warn("Couldn't create {}: {}", curpath.c_str(), strerror(errno));
			errno = EEXIST;
		}
#endif

		if (errno == EEXIST) {
			/* Make sure it's a directory. */
			struct stat st;
			if (DoStat(curpath.c_str(), &st) != -1 && !(st.st_mode & S_IFDIR)) {
				Locator::getLogger()->warn("Couldn't create {}: path exists and is not a directory", curpath.c_str());
				return false;
			}

			continue; // we expect to see this error
		}

        Locator::getLogger()->warn("Couldn't create {}: {}", curpath.c_str(), strerror(errno));
		return false;
	}

	return true;
}

DirectFilenameDB::DirectFilenameDB(const std::string& root_)
{
	ExpireSeconds = 30;
	SetRoot(root_);
}

void
DirectFilenameDB::SetRoot(const std::string& root_)
{
	root = root_;

	// "\abcd\" -> "/abcd/":
	s_replace(root, "\\", "/");

	// "/abcd/" -> "/abcd":
	if (root.back() == '/')
		root.erase(root.size() - 1, 1);
}

void
DirectFilenameDB::CacheFile(const std::string& sPath)
{
	Locator::getLogger()->trace(std::string(root + sPath).c_str());
	std::string sDir = Dirname(sPath);
	FileSet* pFileSet = GetFileSet(sDir, false);
	if (pFileSet == nullptr) {
		// This directory isn't cached so do nothing.
		m_Mutex.Unlock(); // Locked by GetFileSet()
		return;
	}
	while (!pFileSet->m_bFilled)
		m_Mutex.Wait();

#ifdef _WIN32
	// There is almost surely a better way to do this
	WIN32_FIND_DATA fd;
	HANDLE hFind = DoFindFirstFile(std::string(root + sPath).c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE) {
		m_Mutex.Unlock(); // Locked by GetFileSet()
		return;
	}
	File f(fd.cFileName);
	f.dir = !!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	f.size = fd.nFileSizeLow;
	f.hash = fd.ftLastWriteTime.dwLowDateTime;

	pFileSet->files.insert(f);
	FindClose(hFind);
#else
	File f(Basename(sPath));

	struct stat st;
	if (DoStat((root + sPath).c_str(), &st) == -1) {
		int iError = errno;
		// If it's a broken symlink, ignore it.  Otherwise, warn.
		// Huh?
		Locator::getLogger()->warn("File '{}' is gone! ({})", sPath.c_str(), strerror(iError));
	} else {
		f.dir = (st.st_mode & S_IFDIR);
		f.size = (int)st.st_size;
		f.hash = st.st_mtime;
	}

	pFileSet->files.insert(f);
#endif
	m_Mutex.Unlock(); // Locked by GetFileSet()
}

void
DirectFilenameDB::PopulateFileSet(FileSet& fs, const std::string& path)
{
	std::string sPath = path;

	// Resolve path cases (path/Path -> PATH/path).
	ResolvePath(sPath);

	fs.age.GetDeltaTime(); // reset
	fs.files.clear();

#ifdef _WIN32
	WIN32_FIND_DATA fd;

	if (sPath.size() > 0 && sPath.back() == '/')
		sPath.erase(sPath.size() - 1);

	HANDLE hFind =
	  DoFindFirstFile(std::string(root + sPath + "/*").c_str(), &fd);
	// This crashes on multithreaded startup occasionally. -poco
	// CHECKPOINT_M(root + sPath + "/*");

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do {
		if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
			continue;

		File f;
		f.SetName(fd.cFileName);
		f.dir = !!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		f.size = fd.nFileSizeLow;
		f.hash = fd.ftLastWriteTime.dwLowDateTime;

		fs.files.insert(f);
	} while (FindNextFile(hFind, &fd));
	FindClose(hFind);
#else
	/* Ugly: POSIX threads are not guaranteed to have per-thread cwds, and only
	 * a few systems have openat() or equivalent; one or the other is needed
	 * to do efficient, thread-safe directory traversal.  Instead, we have to
	 * use absolute paths, which forces the system to re-parse the directory
	 * for each file.  This isn't a major issue, since most large directory
	 * scans are I/O-bound. */

	DIR* pDir = opendir((root + sPath).c_str());
	if (pDir == NULL)
		return;

	while (struct dirent* pEnt = readdir(pDir)) {
		if (!strcmp(pEnt->d_name, "."))
			continue;
		if (!strcmp(pEnt->d_name, ".."))
			continue;

		File f(pEnt->d_name);

		struct stat st;
		if (DoStat((root + sPath + "/" + pEnt->d_name).c_str(), &st) == -1) {
			int iError = errno;
			/* If it's a broken symlink, ignore it.  Otherwise, warn. */
			if (lstat((root + sPath + "/" + pEnt->d_name).c_str(), &st) == 0)
				continue;

			/* Huh? */
			Locator::getLogger()->warn("Got file '{}' in '{}' from list, but can't stat? ({})",
					   pEnt->d_name, sPath.c_str(), strerror(iError));
			continue;
		}

		f.dir = (st.st_mode & S_IFDIR);
		f.size = (int)st.st_size;
		f.hash = st.st_mtime;

		fs.files.insert(f);
	}

	closedir(pDir);
#endif

	/*
	 * Check for any ".ignore" markers.  If a marker exists, hide the marker and
	 * its corresponding file. For example, if "file.xml.ignore" exists, hide
	 * both it and "file.xml" by removing them from the file set. Ignore markers
	 * are used for convenience during build staging and are not used in
	 * performance-critical situations.  To avoid incurring some of the
	 * overheard due to ignore markers, delete the file instead instead of using
	 * an ignore marker.
	 */
	static const std::string IGNORE_MARKER_BEGINNING = "ignore-";

	vector<std::string> vsFilesToRemove;
	for (std::set<File>::iterator iter =
		   fs.files.lower_bound(IGNORE_MARKER_BEGINNING);
		 iter != fs.files.end();
		 ++iter) {
		if (!BeginsWith(iter->lname, IGNORE_MARKER_BEGINNING))
			break;
		std::string sFileLNameToIgnore =
		  tail(std::string(iter->lname),
			   iter->lname.length() - IGNORE_MARKER_BEGINNING.length());
		vsFilesToRemove.push_back(iter->name);
		vsFilesToRemove.push_back(sFileLNameToIgnore);
	}

	for (auto& iter : vsFilesToRemove) {
		// Erase the file corresponding to the ignore marker
		File fileToDelete;
		fileToDelete.SetName(iter);
		std::set<File>::iterator iter2 = fs.files.find(fileToDelete);
		if (iter2 != fs.files.end())
			fs.files.erase(iter2);
	}
}
