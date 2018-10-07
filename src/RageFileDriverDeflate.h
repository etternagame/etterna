/* RageFileObjInflate - decompress streams compressed with "deflate"
 * compression. */

#ifndef RAGE_FILE_DRIVER_DEFLATE_H
#define RAGE_FILE_DRIVER_DEFLATE_H

#include "RageFileBasic.h"

using z_stream = struct z_stream_s;

class RageFileObjInflate : public RageFileObj
{
  public:
	/* By default, pFile will not be freed.  To implement GetFileSize(), the
	 * container format must store the file size. */
	RageFileObjInflate(RageFileBasic* pFile, int iUncompressedSize);
	RageFileObjInflate(const RageFileObjInflate& cpy);
	~RageFileObjInflate() override;
	int ReadInternal(void* pBuffer, size_t iBytes) override;
	int WriteInternal(const void* /* pBuffer */, size_t /* iBytes */) override
	{
		SetError("Not implemented");
		return -1;
	}
	int SeekInternal(int iOffset) override;
	int GetFileSize() const override { return m_iUncompressedSize; }
	int GetFD() override { return m_pFile->GetFD(); }
	RageFileObjInflate* Copy() const override;

	void DeleteFileWhenFinished() { m_bFileOwned = true; }

  private:
	int m_iUncompressedSize;
	RageFileBasic* m_pFile;
	int m_iFilePos;
	bool m_bFileOwned;

	z_stream* m_pInflate;
	enum
	{
		INBUFSIZE = 1024 * 4
	};
	char decomp_buf[INBUFSIZE], *decomp_buf_ptr;
	int decomp_buf_avail;
};

class RageFileObjDeflate : public RageFileObj
{
  public:
	/* By default, pFile will not be freed. */
	RageFileObjDeflate(RageFileBasic* pOutput);
	~RageFileObjDeflate() override;

	int GetFileSize() const override { return m_pFile->GetFileSize(); }
	void DeleteFileWhenFinished() { m_bFileOwned = true; }

  protected:
	int ReadInternal(void* /* pBuffer */, size_t /* iBytes */) override
	{
		SetError("Not implemented");
		return -1;
	}
	int WriteInternal(const void* pBuffer, size_t iBytes) override;
	int FlushInternal() override;

	RageFileBasic* m_pFile;
	z_stream* m_pDeflate;
	bool m_bFileOwned;
};

class RageFileObjGzip : public RageFileObjDeflate
{
  public:
	RageFileObjGzip(RageFileBasic* pFile);
	int Start();
	int Finish();

  private:
	int m_iDataStartOffset;
};

RageFileObjInflate*
GunzipFile(RageFileBasic* pFile, RString& sError, uint32_t* iCRC32);

/* Quick helpers: */
void
GzipString(const RString& sIn, RString& sOut);
bool
GunzipString(const RString& sIn,
			 RString& sOut,
			 RString& sError); // returns false on CRC, etc. error

#endif

/*
 * Copyright (c) 2003-2004 Glenn Maynard
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
