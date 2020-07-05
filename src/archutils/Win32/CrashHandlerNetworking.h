#ifndef CRASH_HANDLER_NETWORKING_H
#define CRASH_HANDLER_NETWORKING_H

#include "RageUtil/Misc/RageThreads.h"
#include <map>

class NetworkStream;

// Send a set of data over HTTP, as a POST form.
class NetworkPostData
{
  public:
	NetworkPostData();
	~NetworkPostData();

	void SetData(const std::string& sKey, const std::string& sData);

	// For simplicity, we don't parse URLs here.
	void Start(const std::string& sHost, int iPort, const std::string& sPath);

	// Cancel the running operation, and close the thread.
	void Cancel();

	// If the operation is unfinished, return false. Otherwise, close the thread
	// and return true.
	bool IsFinished();

	std::string GetStatus() const;
	float GetProgress() const;
	std::string GetError() const;
	std::string GetResult() const { return m_sResult; }

  private:
	static void CreateMimeData(
	  const map<std::string, std::string>& mapNameToData,
	  std::string& sOut,
	  std::string& sMimeBoundaryOut);
	void SetProgress(float fProgress);

	RageThread m_Thread;
	void HttpThread();
	static int HttpThread_Start(void* p)
	{
		((NetworkPostData*)p)->HttpThread();
		return 0;
	}

	mutable RageMutex m_Mutex;
	std::string m_sStatus;
	float m_fProgress;

	// When the thread exists, it owns the rest of the data, regardless of
	// m_Mutex.
	map<std::string, std::string> m_Data;

	bool m_bFinished;
	std::string m_sHost;
	int m_iPort;
	std::string m_sPath;
	std::string m_sResult;

	NetworkStream* m_pStream;
};

#endif
