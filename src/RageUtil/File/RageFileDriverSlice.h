/* RageFileDriverSlice - Treat a portion of a file as a file. */

#ifndef RAGE_FILE_DRIVER_SLICE_H
#define RAGE_FILE_DRIVER_SLICE_H

#include "RageFileBasic.h"

class RageFileDriverSlice : public RageFileObj
{
  public:
	/* pFile will be freed if DeleteFileWhenFinished is called. */
	RageFileDriverSlice(RageFileBasic* pFile, int iOffset, int iFileSize);
	RageFileDriverSlice(const RageFileDriverSlice& cpy);
	~RageFileDriverSlice() override;
	RageFileDriverSlice* Copy() const override;

	void DeleteFileWhenFinished() { m_bFileOwned = true; }

	int ReadInternal(void* pBuffer, size_t iBytes) override;
	int WriteInternal(const void* /* pBuffer */, size_t /* iBytes */) override
	{
		SetError("Not implemented");
		return -1;
	}
	int SeekInternal(int iOffset) override;
	int GetFileSize() const override { return m_iFileSize; }
	int GetFD() override { return m_pFile->GetFD(); }

  private:
	RageFileBasic* m_pFile;
	int m_iFilePos;
	int m_iOffset, m_iFileSize;
	bool m_bFileOwned;
};

#endif
