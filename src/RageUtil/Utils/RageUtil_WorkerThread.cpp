#include "Etterna/Globals/global.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil.h"
#include "RageUtil_WorkerThread.h"

RageWorkerThread::RageWorkerThread(const std::string& sName)
  : m_WorkerEvent("\"" + sName + "\" worker event")
  , m_HeartbeatEvent("\"" + sName + "\" heartbeat event")
{
	m_sName = sName;
	m_Timeout = 0.F;
	m_iRequest = REQ_NONE;
	m_bTimedOut = false;
	m_fHeartbeat = -1;
	m_bRequestFinished = false;

	m_WorkerThread.SetName("Worker thread (" + sName + ")");
}

RageWorkerThread::~RageWorkerThread()
{
	/* The worker thread must be stopped by the derived class. */
	ASSERT(!m_WorkerThread.IsCreated());
}

void
RageWorkerThread::SetTimeout(float fSeconds)
{
	m_WorkerEvent.Lock();
	if (fSeconds < 0)
		m_Timeout = 0.F;
	else {
		m_Timeout = fSeconds;
	}
	m_WorkerEvent.Unlock();
}

void
RageWorkerThread::StartThread()
{
	ASSERT(!m_WorkerThread.IsCreated());

	m_WorkerThread.Create(StartWorkerMain, this);
}

void
RageWorkerThread::StopThread()
{
	/* If we're timed out, wait. */
	m_WorkerEvent.Lock();
	if (m_bTimedOut) {
		Locator::getLogger()->trace("Waiting for timed-out worker thread \"{}\" to complete ...",
				   m_sName.c_str());
		while (m_bTimedOut)
			m_WorkerEvent.Wait();
	}
	m_WorkerEvent.Unlock();

	/* Disable the timeout.  This will ensure that we really wait for the worker
	 * thread to shut down. */
	SetTimeout(-1);

	/* Shut down. */
	if (!DoRequest(REQ_SHUTDOWN))
		Locator::getLogger()->warn("May have failed to shut down worker thread \"{}\"",
				  m_sName.c_str());
	m_WorkerThread.Wait();
}

bool
RageWorkerThread::DoRequest(int iRequest)
{
	ASSERT(!m_bTimedOut);
	ASSERT(m_iRequest == REQ_NONE);

	if (m_Timeout <= 0.F && iRequest != REQ_SHUTDOWN)
		Locator::getLogger()->warn("Request made with timeout disabled ({}, iRequest = {})", m_sName.c_str(), iRequest);

	/* Set the request, and wake up the worker thread. */
	m_WorkerEvent.Lock();

	m_iRequest = iRequest;
	m_WorkerEvent.Broadcast();

	/* Wait for it to complete or time out. */
	while (!m_bRequestFinished) {
		bool bTimedOut = !m_WorkerEvent.Wait(m_Timeout);
		if (bTimedOut)
			break;
	}

	const bool bRequestFinished = m_bRequestFinished;
	if (m_bRequestFinished) {
		/* The request finished successfully.  It's the calling function's
		 * responsibility to clean up. */
		m_bRequestFinished = false;
	} else {
		/* The request hasn't finished yet.  Set m_bTimedOut true.  This tells
		 * the still-running request that it timed out, and so it's the thread's
		 * responsibility to clean up--we can't do it, since we'd collide with
		 * the request. */
		m_bTimedOut = true;
	}
	m_WorkerEvent.Unlock();

	return bRequestFinished;
}

void
RageWorkerThread::WorkerMain()
{
	for (;;) {
		bool bTimeToRunHeartbeat = false;
		m_WorkerEvent.Lock();
		while (m_iRequest == REQ_NONE && !bTimeToRunHeartbeat) {
			if (!m_WorkerEvent.Wait(m_fHeartbeat != -1 ? m_NextHeartbeat
													   : 0.F))
				bTimeToRunHeartbeat = true;
		}
		const int iRequest = m_iRequest;
		m_iRequest = REQ_NONE;

		m_WorkerEvent.Unlock();

		/* If it's time to run a heartbeat, do so. */
		if (bTimeToRunHeartbeat) {
			DoHeartbeat();

			/* Wake up anyone waiting for a heartbeat. */
			m_HeartbeatEvent.Lock();
			m_HeartbeatEvent.Broadcast();
			m_HeartbeatEvent.Unlock();

			/* Schedule the next heartbeat. */
			m_NextHeartbeat = m_fHeartbeat;
		}

		if (iRequest != REQ_NONE) {
			/* Handle the request. */
			if (iRequest != REQ_SHUTDOWN) {
				Locator::getLogger()->trace("HandleRequest({})", iRequest);
				HandleRequest(iRequest);
				Locator::getLogger()->trace("HandleRequest({}) done", iRequest);
			}

			/* Lock the mutex, to keep DoRequest where it is (if it's still
			 * running). */
			/* The request is finished.  If it timed out, clear the timeout flag
			 * and call RequestTimedOut, to allow cleaning up. */
			m_WorkerEvent.Lock();

			if (m_bTimedOut) {
				Locator::getLogger()->trace("Request {} timed out", iRequest);

				/* The calling thread timed out.  It's already gone and moved
				 * on, so it's our responsibility to clean up.  No new requests
				 * will come in until we clear m_bTimedOut, so we can safely
				 * unlock and clean up. */
				m_WorkerEvent.Unlock();

				RequestTimedOut();

				/* Clear the time-out flag, indicating that we can work again.
				 */
				m_bTimedOut = false;
			} else {
				Locator::getLogger()->trace("HandleRequest({}) OK", iRequest);

				m_bRequestFinished = true;

				/* We're finished.  Wake up the requester. */
				m_WorkerEvent.Broadcast();
				m_WorkerEvent.Unlock();
			}
		}

		if (iRequest == REQ_SHUTDOWN)
			break;
	}
}

bool
RageWorkerThread::WaitForOneHeartbeat()
{
	/* It doesn't make sense to wait for a heartbeat if there is no heartbeat.
	 */
	ASSERT(m_fHeartbeat != -1);

	m_HeartbeatEvent.Lock();
	bool bTimedOut = !m_HeartbeatEvent.Wait(m_Timeout);
	m_HeartbeatEvent.Unlock();

	return !bTimedOut;
}
