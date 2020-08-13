/* RageFileDriverReadAhead - Read-ahead hinting for seamless rewinding. */

#ifndef RAGE_FILE_DRIVER_READ_AHEAD_H
#define RAGE_FILE_DRIVER_READ_AHEAD_H

#include "RageFileBasic.h"

class RageFileDriverReadAhead : public RageFileObj
{
  public:
	/* This filter can only be used on supported files; test before using. */
	static bool FileSupported(RageFileBasic* pFile);

	/* pFile will be freed if DeleteFileWhenFinished is called. */
	RageFileDriverReadAhead(RageFileBasic* pFile,
							int iCacheBytes,
							int iPostBufferReadAhead = -1);
	RageFileDriverReadAhead(const RageFileDriverReadAhead& cpy);
	~RageFileDriverReadAhead() override;
	RageFileDriverReadAhead* Copy() const override;

	void DeleteFileWhenFinished() { m_bFileOwned = true; }

	std::string GetError() const override { return m_pFile->GetError(); }
	void ClearError() override { return m_pFile->ClearError(); }

	int ReadInternal(void* pBuffer, size_t iBytes) override;
	int WriteInternal(const void* pBuffer, size_t iBytes) override
	{
		return m_pFile->Write(pBuffer, iBytes);
	}
	int SeekInternal(int iOffset) override;
	int GetFileSize() const override { return m_pFile->GetFileSize(); }
	int GetFD() override { return m_pFile->GetFD(); }
	int Tell() const override { return m_iFilePos; }

  private:
	void FillBuffer(int iBytes);

	RageFileBasic* m_pFile;
	int m_iFilePos;
	bool m_bFileOwned;
	std::string m_sBuffer;
	int m_iPostBufferReadAhead;
	bool m_bReadAheadNeeded = false;
};

#endif
