#include "Etterna/Globals/global.h"
#include "RageFileDriver.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Utils/RageUtil_FileDB.h"

RageFileDriver::~RageFileDriver()
{
	delete FDB;
}

int
RageFileDriver::GetPathValue(const std::string& sPath)
{
	std::vector<std::string> asParts;
	split(sPath, "/", asParts, true);

	std::string sPartialPath;

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
RageFileDriver::GetDirListing(const std::string& sPath,
							  std::vector<std::string>& asAddTo,
							  DirListingReturnFilter returnFilter,
							  bool bReturnPathToo)
{
	FDB->GetDirListing(sPath, asAddTo, returnFilter, bReturnPathToo);
}

RageFileManager::FileType
RageFileDriver::GetFileType(const std::string& sPath)
{
	return FDB->GetFileType(sPath);
}

int
RageFileDriver::GetFileSizeInBytes(const std::string& sPath)
{
	return FDB->GetFileSize(sPath);
}

int
RageFileDriver::GetFileHash(const std::string& sPath)
{
	return FDB->GetFileHash(sPath);
}

void
RageFileDriver::FlushDirCache(const std::string& sPath)
{
	FDB->FlushDirCache(sPath);
}

const struct FileDriverEntry* g_pFileDriverList = nullptr;

FileDriverEntry::FileDriverEntry(const std::string& sType)
{
	m_pLink = g_pFileDriverList;
	g_pFileDriverList = this;
	m_sType = sType;
}

FileDriverEntry::~FileDriverEntry()
{
	g_pFileDriverList = nullptr; /* invalidate */
}

RageFileDriver*
MakeFileDriver(const std::string& sType, const std::string& sRoot)
{
	for (const FileDriverEntry* p = g_pFileDriverList; p != nullptr;
		 p = p->m_pLink)
		if (!CompareNoCase(p->m_sType, sType))
			return p->Create(sRoot);
	return nullptr;
}
