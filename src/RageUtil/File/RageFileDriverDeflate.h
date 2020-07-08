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
GunzipFile(RageFileBasic* pFile, std::string& sError, uint32_t* iCRC32);

/* Quick helpers: */
void
GzipString(const std::string& sIn, std::string& sOut);
bool
GunzipString(const std::string& sIn,
			 std::string& sOut,
			 std::string& sError); // returns false on CRC, etc. error

#endif
