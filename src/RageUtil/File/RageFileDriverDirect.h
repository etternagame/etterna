#ifndef RAGE_FILE_DRIVER_DIRECT_H
#define RAGE_FILE_DRIVER_DIRECT_H

#include "RageFile.h"
#include "RageFileDriver.h"

/** @brief File driver for accessing a regular filesystem. */
class RageFileDriverDirect : public RageFileDriver
{
  public:
	explicit RageFileDriverDirect(const std::string& sRoot);

	RageFileBasic* Open(const std::string& sPath,
						int iMode,
						int& iError) override;
	bool Move(const std::string& sOldPath,
			  const std::string& sNewPath) override;
	bool Remove(const std::string& sPath) override;
	bool Remount(const std::string& sPath) override;

  private:
	std::string m_sRoot;
};

class RageFileDriverDirectReadOnly : public RageFileDriverDirect
{
  public:
	explicit RageFileDriverDirectReadOnly(const std::string& sRoot);
	RageFileBasic* Open(const std::string& sPath,
						int iMode,
						int& iError) override;
	bool Move(const std::string& sOldPath,
			  const std::string& sNewPath) override;
	bool Remove(const std::string& sPath) override;
};

/** @brief This driver handles direct file access. */

class RageFileObjDirect : public RageFileObj
{
  public:
	RageFileObjDirect(const std::string& sPath, int iFD, int iMode);
	~RageFileObjDirect() override;
	int ReadInternal(void* pBuffer, size_t iBytes) override;
	int WriteInternal(const void* pBuffer, size_t iBytes) override;
	int FlushInternal() override;
	int SeekInternal(int offset) override;
	RageFileObjDirect* Copy() const override;
	std::string GetDisplayPath() const override { return m_sPath; }
	int GetFileSize() const override;
	int GetFD() override;

  private:
	bool FinalFlush();

	int m_iFD;
	int m_iMode;
	std::string m_sPath; /* for Copy */

	/*
	 * When not streaming to disk, we write to a temporary file, and rename to
	 * the real file on completion.  If any write, this is aborted.  When
	 * streaming to disk, allow recovering from errors.
	 */
	bool m_bWriteFailed;
	bool WriteFailed() const
	{
		return ((m_iMode & RageFile::STREAMED) == 0) && m_bWriteFailed;
	}

	// unused
	RageFileObjDirect& operator=(const RageFileObjDirect& rhs) = delete;
	RageFileObjDirect(const RageFileObjDirect& rhs) = delete;
};

#endif
