#include "Etterna/Globals/global.h"
#include "RageFileDriver.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Utils/RageUtil_FileDB.h"

RageFileDriver::~RageFileDriver()
{
	delete FDB;
}

int
RageFileDriver::GetPathValue(const RString& sPath)
{
	vector<std::string> asParts;
	split(sPath, "/", asParts, true);

	RString sPartialPath;

	for (unsigned i = 0; i < asParts.size(); ++i) {
		sPartialPath += asParts[i];
		if (i + 1 < asParts.size())
			sPartialPath += "/";

		const RageFileManager::FileType Type = GetFileType(sPartialPath);
		switch (Type) {
			case RageFileManager::TYPE_NONE:
				return asParts.size() - i;

			/* If this is the last part (the whole path), it needs to be a file;
			 * otherwise a directory. */
			case RageFileManager::TYPE_FILE:
				if (i != asParts.size() - 1)
					return -1;
				break;
			case RageFileManager::TYPE_DIR:
				if (i == asParts.size() - 1)
					return -1;
				break;
		}
	}

	return 0;
}

void
RageFileDriver::GetDirListing(const RString& sPath,
							  vector<RString>& asAddTo,
							  bool bOnlyDirs,
							  bool bReturnPathToo)
{
	FDB->GetDirListing(sPath, asAddTo, bOnlyDirs, bReturnPathToo);
}
void
RageFileDriver::GetDirListing(const std::string& sPath,
							  vector<std::string>& asAddTo,
							  bool bOnlyDirs,
							  bool bReturnPathToo)
{
	FDB->GetDirListing(sPath, asAddTo, bOnlyDirs, bReturnPathToo);
}

RageFileManager::FileType
RageFileDriver::GetFileType(const RString& sPath)
{
	return FDB->GetFileType(sPath);
}

int
RageFileDriver::GetFileSizeInBytes(const RString& sPath)
{
	return FDB->GetFileSize(sPath);
}

int
RageFileDriver::GetFileHash(const RString& sPath)
{
	return FDB->GetFileHash(sPath);
}

void
RageFileDriver::FlushDirCache(const RString& sPath)
{
	FDB->FlushDirCache(sPath);
}

const struct FileDriverEntry* g_pFileDriverList = NULL;

FileDriverEntry::FileDriverEntry(const RString& sType)
{
	m_pLink = g_pFileDriverList;
	g_pFileDriverList = this;
	m_sType = sType;
}

FileDriverEntry::~FileDriverEntry()
{
	g_pFileDriverList = NULL; /* invalidate */
}

RageFileDriver*
MakeFileDriver(const RString& sType, const RString& sRoot)
{
	for (const FileDriverEntry* p = g_pFileDriverList; p != nullptr;
		 p = p->m_pLink)
		if (!p->m_sType.CompareNoCase(sType))
			return p->Create(sRoot);
	return NULL;
}
