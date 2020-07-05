/* RageFileDriverTimeOut - manipulate files with a forced timeout. */

#ifndef RAGE_FILE_DRIVER_TIMEOUT_H
#define RAGE_FILE_DRIVER_TIMEOUT_H

#include "RageFileDriver.h"

class ThreadedFileWorker;

class RageFileDriverTimeout : public RageFileDriver
{
  public:
	explicit RageFileDriverTimeout(const std::string& path);
	~RageFileDriverTimeout() override;

	RageFileBasic* Open(const std::string& path, int mode, int& err) override;
	void FlushDirCache(const std::string& sPath) override;
	bool Move(const std::string& sOldPath,
			  const std::string& sNewPath) override;
	bool Remove(const std::string& sPath) override;

	static void SetTimeout(float fSeconds);
	static void ResetTimeout() { SetTimeout(-1); }

  private:
	ThreadedFileWorker* m_pWorker;
};

#endif
