/* RageWorkerThread - a worker thread for operations that are allowed to time
 * out. */

#ifndef RAGE_UTIL_WORKER_THREAD_H
#define RAGE_UTIL_WORKER_THREAD_H

#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Misc/RageTimer.h"

class RageWorkerThread
{
  public:
	RageWorkerThread(const std::string& sName);
	virtual ~RageWorkerThread();

	/* Call SetTimeout(10) to start a timeout period of 10 seconds.  This is not
	 * a per-request timeout; you have 10 seconds to do your work, at which
	 * point all requests time out until SetTimeout is called again. */
	void SetTimeout(float fSeconds);
	bool TimeoutEnabled() const { return m_Timeout > 0.F; }

	/* Return true if the last operation has timed out and has not yet
	 * recovered. */
	bool IsTimedOut() const { return m_bTimedOut; }

	/* Pause until the next heartbeat completes.  Returns false if timed out.
	 * This triggers no actions, so no cleanup is run and IsTimedOut() is not
	 * affected. */
	bool WaitForOneHeartbeat();

  protected:
	/* Call this in the derived class to start and stop the thread. */
	void StartThread();
	void StopThread();

	/* Run the given request.  Return true if the operation completed, false on
	 * timeout. Always call IsTimedOut() first; if true is returned, the thread
	 * is currently timed out and DoRequest() must not be called. */
	bool DoRequest(int iRequest);

	/* Overload this in the derived class to handle requests. */
	virtual void HandleRequest(int iRequest) = 0;

	/* If DoRequest times out, this will be called in the thread after
	 * completion. Clean up.  No new requests will be allowed until this
	 * completes. */
	virtual void RequestTimedOut() {}

	/* Enable a heartbeat.  DoHeartbeat will be called every fSeconds while
	 * idle. DoHeartbeat may safely time out; if DoRequest tries to start a
	 * request in the main thread, it'll simply time out. */
	void SetHeartbeat(float fSeconds)
	{
		m_fHeartbeat = fSeconds;
		m_NextHeartbeat = 0.F;
	}
	virtual void DoHeartbeat() {}

  private:
	static int StartWorkerMain(void* pThis)
	{
		(reinterpret_cast<RageWorkerThread*>(pThis))->WorkerMain();
		return 0;
	}
	void WorkerMain();

	enum
	{
		REQ_SHUTDOWN = -1,
		REQ_NONE = -2
	};
	RageThread m_WorkerThread;
	RageEvent m_WorkerEvent;
	std::string m_sName;
	int m_iRequest;
	bool m_bRequestFinished;
	bool m_bTimedOut;
	float m_Timeout;

	float m_fHeartbeat;
	float m_NextHeartbeat;
	RageEvent m_HeartbeatEvent;
};

#endif
