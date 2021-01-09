#ifndef THREADS_WIN32_H
#define THREADS_WIN32_H

#include "Threads.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <windef.h>
#endif

class ThreadImpl_Win32 : public ThreadImpl
{
  public:
	HANDLE ThreadHandle;
	DWORD ThreadId;

	int (*m_pFunc)(void* pData);
	void* m_pData;

	void Halt(bool Kill);
	void Resume();
	uint64_t GetThreadId() const;
	int Wait();
};

HANDLE
Win32ThreadIdToHandle(uint64_t iID);

class MutexImpl_Win32 : public MutexImpl
{
	friend class EventImpl_Win32;

  public:
	MutexImpl_Win32(RageMutex* parent);
	~MutexImpl_Win32();

	bool Lock();
	bool TryLock();
	void Unlock();

  private:
	HANDLE mutex;
};

class EventImpl_Win32 : public EventImpl
{
  public:
	EventImpl_Win32(MutexImpl_Win32* pParent);
	~EventImpl_Win32();

	bool Wait(float timeout);
	void Signal();
	void Broadcast();
	bool WaitTimeoutSupported() const { return true; }

  private:
	MutexImpl_Win32* m_pParent;

	int m_iNumWaiting;
	CRITICAL_SECTION m_iNumWaitingLock;
	HANDLE m_WakeupSema;
	HANDLE m_WaitersDone;
};

class SemaImpl_Win32 : public SemaImpl
{
  public:
	SemaImpl_Win32(int iInitialValue);
	~SemaImpl_Win32();
	int GetValue() const { return m_iCounter; }
	void Post();
	bool Wait();
	bool TryWait();

  private:
	HANDLE sem;

	// We have to track the count ourself, since Windows gives no way to query
	// it.
	int m_iCounter;
};

#endif
