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

/*
 * (c) 2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
