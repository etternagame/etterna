/* RageFileDriverTimeOut - manipulate files with a forced timeout. */

#ifndef RAGE_FILE_DRIVER_TIMEOUT_H
#define RAGE_FILE_DRIVER_TIMEOUT_H

#include "RageFileDriver.h"

class ThreadedFileWorker;

class RageFileDriverTimeout : public RageFileDriver
{
  public:
	explicit RageFileDriverTimeout(const RString& path);
	~RageFileDriverTimeout() override;

	RageFileBasic* Open(const RString& path, int mode, int& err) override;
	void FlushDirCache(const RString& sPath) override;
	bool Move(const RString& sOldPath, const RString& sNewPath) override;
	bool Remove(const RString& sPath) override;

	static void SetTimeout(float fSeconds);
	static void ResetTimeout() { SetTimeout(-1); }

  private:
	ThreadedFileWorker* m_pWorker;
};

#endif
