/* RageFileDriverMemory: Simple memory-based "filesystem". */

#ifndef RAGE_FILE_DRIVER_MEMORY_H
#define RAGE_FILE_DRIVER_MEMORY_H

#include "RageFileBasic.h"
#include "RageFileDriver.h"
#include "RageUtil/Misc/RageThreads.h"

struct RageFileObjMemFile;

class RageFileObjMem : public RageFileObj
{
  public:
	RageFileObjMem(RageFileObjMemFile* pFile = NULL);
	RageFileObjMem(const RageFileObjMem& cpy);
	~RageFileObjMem() override;

	int ReadInternal(void* buffer, size_t bytes) override;
	int WriteInternal(const void* buffer, size_t bytes) override;
	int SeekInternal(int offset) override;
	int GetFileSize() const override;
	RageFileObjMem* Copy() const override;

	/* Retrieve the contents of this file. */
	const RString& GetString() const;
	void PutString(const RString& sBuf);

  private:
	RageFileObjMemFile* m_pFile;
	int m_iFilePos;
};

class RageFileDriverMem : public RageFileDriver
{
  public:
	RageFileDriverMem();
	~RageFileDriverMem() override;

	RageFileBasic* Open(const RString& sPath, int mode, int& err) override;
	void FlushDirCache(const RString& /* sPath */) override {}

	bool Remove(const RString& sPath) override;

  private:
	RageMutex m_Mutex;
	std::vector<RageFileObjMemFile*> m_Files;
};

#endif
