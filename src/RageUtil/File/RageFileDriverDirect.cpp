#include "Etterna/Globals/global.h"
#include "RageFile.h"
#include "RageFileDriverDirect.h"
#include "RageFileDriverDirectHelpers.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Utils/RageUtil_FileDB.h"
#include "Core/Services/Locator.hpp"

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
#include <cerrno>

#if !defined(_WIN32)

#if defined(HAVE_DIRENT_H)
#include <dirent.h>
#endif

#else
#include "archutils/Win32/ErrorStrings.h"
#include <windows.h>
#include <io.h>
#endif // !defined(_WIN32)

/* Direct filesystem access: */
static struct FileDriverEntry_DIR : public FileDriverEntry
{
	FileDriverEntry_DIR()
	  : FileDriverEntry("DIR")
	{
	}
	RageFileDriver* Create(const std::string& sRoot) const override
	{
		return new RageFileDriverDirect(sRoot);
	}
} const g_RegisterDriver;

/* Direct read-only filesystem access: */
static struct FileDriverEntry_DIRRO : public FileDriverEntry
{
	FileDriverEntry_DIRRO()
	  : FileDriverEntry("DIRRO")
	{
	}
	RageFileDriver* Create(const std::string& sRoot) const override
	{
		return new RageFileDriverDirectReadOnly(sRoot);
	}
} const g_RegisterDriver2;

RageFileDriverDirect::RageFileDriverDirect(const std::string& sRoot)
  : RageFileDriver(new DirectFilenameDB(sRoot))
{
	Remount(sRoot);
}

static std::string
MakeTempFilename(const std::string& sPath)
{
	/* "Foo/bar/baz" -> "Foo/bar/new.baz.new".  Both prepend and append: we
	 * don't want a wildcard search for the filename to match (foo.txt.new
	 * matches foo.txt*), and we don't want to have the same extension (so
	 * "new.foo.sm" doesn't show up in *.sm). */
	return Dirname(sPath) + "new." + Basename(sPath) + ".new";
}

static RageFileObjDirect*
MakeFileObjDirect(std::string sPath, int iMode, int& iError)
{
	int iFD;
	if ((iMode & RageFile::READ) != 0) {
		iFD = open(sPath.c_str(), O_BINARY | O_RDONLY, 0666);

		/* XXX: Windows returns EACCES if we try to open a file on a CDROM that
		 * isn't ready, instead of something like ENODEV.  We want to return
		 * that case as ENOENT, but we can't distinguish it from file permission
		 * errors. */
	} else {
		std::string sOut;
		if (iMode & RageFile::STREAMED)
			sOut = sPath;
		else
			sOut = MakeTempFilename(sPath);

		/* Open a temporary file for writing. */
		iFD = open(sOut.c_str(), O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, 0666);
	}

	if (iFD == -1) {
		iError = errno;
		return nullptr;
	}

#ifdef __unix__
	struct stat st;
	if (fstat(iFD, &st) != -1 && (st.st_mode & S_IFDIR)) {
		iError = EISDIR;
		close(iFD);
		return NULL;
	}
#endif

	return new RageFileObjDirect(sPath, iFD, iMode);
}

RageFileBasic*
RageFileDriverDirect::Open(const std::string& sPath_, int iMode, int& iError)
{
	std::string sPath = sPath_;
	ASSERT(sPath.size() && sPath[0] == '/');

	/* This partially resolves.  For example, if "abc/def" exists, and we're
	 * opening "ABC/DEF/GHI/jkl/mno", this will resolve it to
	 * "abc/def/GHI/jkl/mno"; we'll create the missing parts below. */
	FDB->ResolvePath(sPath);

	if ((iMode & RageFile::WRITE) != 0) {
		const std::string dir = Dirname(sPath);
		if (this->GetFileType(dir) != RageFileManager::TYPE_DIR)
			CreateDirectories(m_sRoot + dir);
	}

	return MakeFileObjDirect(m_sRoot + sPath, iMode, iError);
}

bool
RageFileDriverDirect::Move(const std::string& sOldPath_,
						   const std::string& sNewPath_)
{
	std::string sOldPath = sOldPath_;
	std::string sNewPath = sNewPath_;
	FDB->ResolvePath(sOldPath);
	FDB->ResolvePath(sNewPath);

	if (this->GetFileType(sOldPath) == RageFileManager::TYPE_NONE)
		return false;

	{
		const std::string sDir = Dirname(sNewPath);
		CreateDirectories(m_sRoot + sDir);
	}
	int size = FDB->GetFileSize(sOldPath);
	int hash = FDB->GetFileHash(sOldPath);
	Locator::getLogger()->trace("rename \"{}\" -> \"{}\"",
				   (m_sRoot + sOldPath).c_str(),
				   (m_sRoot + sNewPath).c_str());
	if (DoRename(std::string(m_sRoot + sOldPath).c_str(),
				 std::string(m_sRoot + sNewPath).c_str()) == -1) {
		Locator::getLogger()->warn("rename({},{}) failed: {}",
					  (m_sRoot + sOldPath).c_str(),
					  (m_sRoot + sNewPath).c_str(),
					  strerror(errno));
		return false;
	}

	FDB->DelFile(sOldPath);
	FDB->AddFile(sNewPath, size, hash, nullptr);
	return true;
}

bool
RageFileDriverDirect::Remove(const std::string& sPath_)
{
	std::string sPath = sPath_;
	FDB->ResolvePath(sPath);
	RageFileManager::FileType type = this->GetFileType(sPath);
	switch (type) {
		case RageFileManager::TYPE_FILE:
			Locator::getLogger()->trace("remove '{}'", (m_sRoot + sPath).c_str());
			if (DoRemove(std::string(m_sRoot + sPath).c_str()) == -1) {
				Locator::getLogger()->warn("remove({}) failed: {}",
							  (m_sRoot + sPath).c_str(),
							  strerror(errno));
				return false;
			}
			FDB->DelFile(sPath);
			return true;

		case RageFileManager::TYPE_DIR:
			Locator::getLogger()->trace("rmdir '{}'", (m_sRoot + sPath).c_str());
			if (rmdir(std::string(m_sRoot + sPath).c_str()) == -1) {
				Locator::getLogger()->warn("rmdir({}) failed: {}",
							  (m_sRoot + sPath).c_str(), strerror(errno));
				return false;
			}
			FDB->DelFile(sPath);
			return true;

		case RageFileManager::TYPE_NONE:
			return false;

		default:
			FAIL_M(ssprintf("Invalid FileType: %i", type));
	}
}

RageFileObjDirect*
RageFileObjDirect::Copy() const
{
	int iErr;
	RageFileObjDirect* ret = MakeFileObjDirect(m_sPath, m_iMode, iErr);

	if (ret == nullptr)
		RageException::Throw(
		  "Couldn't reopen \"%s\": %s", m_sPath.c_str(), strerror(iErr));

	ret->Seek(static_cast<int>(lseek(m_iFD, 0, SEEK_CUR)));

	return ret;
}

bool
RageFileDriverDirect::Remount(const std::string& sPath)
{
	m_sRoot = sPath;
	((DirectFilenameDB*)FDB)->SetRoot(sPath);

	/* If the root path doesn't exist, create it. */
	CreateDirectories(m_sRoot);

	return true;
}

/* The DIRRO driver is just like DIR, except writes are disallowed. */
RageFileDriverDirectReadOnly::RageFileDriverDirectReadOnly(
  const std::string& sRoot)
  : RageFileDriverDirect(sRoot)
{
}
RageFileBasic*
RageFileDriverDirectReadOnly::Open(const std::string& sPath,
								   int iMode,
								   int& iError)
{
	if ((iMode & RageFile::WRITE) != 0) {
		iError = EROFS;
		return nullptr;
	}

	return RageFileDriverDirect::Open(sPath, iMode, iError);
}
bool
RageFileDriverDirectReadOnly::Move(const std::string& /* sOldPath */,
								   const std::string& /* sNewPath */)
{
	return false;
}
bool
RageFileDriverDirectReadOnly::Remove(const std::string& /* sPath */)
{
	return false;
}

static const unsigned int BUFSIZE = 1024 * 64;
RageFileObjDirect::RageFileObjDirect(const std::string& sPath,
									 int iFD,
									 int iMode)
{
	m_sPath = sPath;
	m_iFD = iFD;
	m_bWriteFailed = false;
	m_iMode = iMode;
	ASSERT(m_iFD != -1);

	if (m_iMode & RageFile::WRITE)
		this->EnableWriteBuffering(BUFSIZE);
}

namespace {
#if !defined(_WIN32)
bool
FlushDir(std::string sPath, std::string& sError)
{
	/* Wait for the directory to be flushed. */
	int dirfd = open(sPath.c_str(), O_RDONLY);
	if (dirfd == -1) {
		sError = strerror(errno);
		return false;
	}

	if (fsync(dirfd) == -1) {
		sError = strerror(errno);
		close(dirfd);
		return false;
	}

	close(dirfd);
	return true;
}
#else
bool
FlushDir(std::string /* sPath */, std::string& /* sError */)
{
	return true;
}
#endif
} // namespace

bool
RageFileObjDirect::FinalFlush()
{
	if ((m_iMode & RageFile::WRITE) == 0)
		return true;

	/* Flush the output buffer. */
	if (Flush() == -1)
		return false;

	/* Only do the rest of the flushes if SLOW_FLUSH is enabled. */
	if ((m_iMode & RageFile::SLOW_FLUSH) == 0)
		return true;

	/* Force a kernel buffer flush. */
	if (fsync(m_iFD) == -1) {
		Locator::getLogger()->warn("Error synchronizing {}: {}", this->m_sPath.c_str(), strerror(errno));
		SetError(strerror(errno));
		return false;
	}

	std::string sError;
	if (!FlushDir(Dirname(m_sPath), sError)) {
		Locator::getLogger()->warn("Error synchronizing fsync({} dir): {}",
					  this->m_sPath.c_str(), sError.c_str());
		SetError(sError);
		return false;
	}

	return true;
}

RageFileObjDirect::~RageFileObjDirect()
{
	bool bFailed = !FinalFlush();

	if (m_iFD != -1) {
		if (close(m_iFD) == -1) {
			Locator::getLogger()->warn("Error closing {}: {}", this->m_sPath.c_str(), strerror(errno));
			SetError(strerror(errno));
			bFailed = true;
		}
	}

	if (((m_iMode & RageFile::WRITE) == 0) ||
		((m_iMode & RageFile::STREAMED) != 0))
		return;

	/* We now have path written to MakeTempFilename(m_sPath).
	 * Rename the temporary file over the real path. */

	do {
		if (bFailed || WriteFailed())
			break;

		/* We now have path written to MakeTempFilename(m_sPath). Rename the
		 * temporary file over the real path. This should be an atomic operation
		 * with a journalling filesystem. That is, there should be no
		 * intermediate state a JFS might restore the file we're writing (in the
		 * case of a crash/powerdown) to an empty or partial file. */

		std::string sOldPath = MakeTempFilename(m_sPath);
		std::string sNewPath = m_sPath;

#ifdef _WIN32
		if (WinMoveFile(DoPathReplace(sOldPath), DoPathReplace(sNewPath)))
			return;

		/* We failed. */
		int err = GetLastError();
		const std::string error = werr_ssprintf(err, "Error renaming \"%s\" to \"%s\"",
						sOldPath.c_str(),
						sNewPath.c_str());
		Locator::getLogger()->warn("{}", error);
		SetError(error);
		break;
#else
		if (rename(sOldPath.c_str(), sNewPath.c_str()) == -1) {
			Locator::getLogger()->warn("Error renaming \"{}\" to \"{}\": {}",
						  sOldPath.c_str(),
						  sNewPath.c_str(),
						  strerror(errno));
			SetError(strerror(errno));
			break;
		}

		if ((m_iMode & RageFile::SLOW_FLUSH) != 0) {
			std::string sError;
			if (!FlushDir(Dirname(m_sPath), sError)) {
				Locator::getLogger()->warn("Error synchronizing fsync({} dir): {}",
							  this->m_sPath.c_str(),
							  sError.c_str());
				SetError(sError);
			}
		}

		// Success.
		return;
#endif
	} while (false);

	// The write or the rename failed. Delete the incomplete temporary file.
	int err = DoRemove(MakeTempFilename(m_sPath).c_str());
	if (err != 0)
		Locator::getLogger()->warn("On writing or renaming file, deleting temporary file failed with error {}", err);
}

int
RageFileObjDirect::ReadInternal(void* pBuf, size_t iBytes)
{
	int iRet = read(m_iFD, pBuf, iBytes);
	if (iRet == -1) {
		SetError(strerror(errno));
		return -1;
	}

	return iRet;
}

// write(), but retry a couple times on EINTR.
static int
RetriedWrite(int iFD, const void* pBuf, size_t iCount)
{
	int iTries = 3, iRet;
	do {
		iRet = write(iFD, pBuf, iCount);
	} while (iRet == -1 && errno == EINTR && iTries--);

	return iRet;
}

int
RageFileObjDirect::FlushInternal()
{
	if (WriteFailed()) {
		SetError("previous write failed");
		return -1;
	}

	return 0;
}

int
RageFileObjDirect::WriteInternal(const void* pBuf, size_t iBytes)
{
	if (WriteFailed()) {
		SetError("previous write failed");
		return -1;
	}

	/* The buffer is cleared. If we still don't have space, it's bigger than
	 * the buffer size, so just write it directly. */
	int iRet = RetriedWrite(m_iFD, pBuf, iBytes);
	if (iRet == -1) {
		SetError(strerror(errno));
		m_bWriteFailed = true;
		return -1;
	}
	return iBytes;
}

int
RageFileObjDirect::SeekInternal(int iOffset)
{
	return static_cast<int>(lseek(m_iFD, iOffset, SEEK_SET));
}

int
RageFileObjDirect::GetFileSize() const
{
	const long iOldPos = lseek(m_iFD, 0, SEEK_CUR);
	ASSERT_M(iOldPos != -1,
			 ssprintf("\"%s\": %s", m_sPath.c_str(), strerror(errno)));
	const int iRet = static_cast<int>(lseek(m_iFD, 0, SEEK_END));
	// causes crashes when trying to unzip files >2gb, switch for large packs is
	// handled theme-side in TD, so comment this out for now for other themes
	// ASSERT_M(iRet != -1,
	//		 ssprintf("\"%s\": %s", m_sPath.c_str(), strerror(errno)));
	const int iRet2 = lseek(m_iFD, iOldPos, SEEK_SET);
	if (iRet2 == -1)
        Locator::getLogger()->warn("Undoing seek for filesize getter may have failed.");

	return iRet;
}

int
RageFileObjDirect::GetFD()
{
	return m_iFD;
}
