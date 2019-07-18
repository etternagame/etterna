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

	void SetData(const RString& sKey, const RString& sData);

	// For simplicity, we don't parse URLs here.
	void Start(const RString& sHost, int iPort, const RString& sPath);

	// Cancel the running operation, and close the thread.
	void Cancel();

	// If the operation is unfinished, return false. Otherwise, close the thread
	// and return true.
	bool IsFinished();

	RString GetStatus() const;
	float GetProgress() const;
	RString GetError() const;
	RString GetResult() const { return m_sResult; }

  private:
	static void CreateMimeData(const map<RString, RString>& mapNameToData,
							   RString& sOut,
							   RString& sMimeBoundaryOut);
	void SetProgress(float fProgress);

	RageThread m_Thread;
	void HttpThread();
	static int HttpThread_Start(void* p)
	{
		((NetworkPostData*)p)->HttpThread();
		return 0;
	}

	mutable RageMutex m_Mutex;
	RString m_sStatus;
	float m_fProgress;

	// When the thread exists, it owns the rest of the data, regardless of
	// m_Mutex.
	map<RString, RString> m_Data;

	bool m_bFinished;
	RString m_sHost;
	int m_iPort;
	RString m_sPath;
	RString m_sResult;

	NetworkStream* m_pStream;
};

#endif
