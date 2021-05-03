#ifndef THREADS_H
#define THREADS_H

/* This is the low-level implementation; you probably want RageThreads. */
class RageMutex;
class RageTimer;

class ThreadImpl
{
  public:
	virtual ~ThreadImpl() = default;
	virtual void Halt(bool Kill) = 0;
	virtual void Resume() = 0;

	/* Get the identifier for this thread. The actual meaning of this is
	 * implementation-defined, except that each thread has exactly one ID
	 * and each ID corresponds to one thread. (This means that Win32
	 * thread handles are not acceptable as ThreadIds.) */
	virtual uint64_t GetThreadId() const = 0;

	virtual int Wait() = 0;
};

class MutexImpl
{
  public:
	RageMutex* m_Parent;

	explicit MutexImpl(RageMutex* pParent)
	  : m_Parent(pParent)
	{
	}
	virtual ~MutexImpl() = default;

	/* Lock the mutex. If mutex timeouts are implemented, and the mutex
	 * times out, return false and do not lock the mutex. No other failure
	 * return is allowed; all other errors should fail with an assertion. */
	virtual bool Lock() = 0;

	/* Non-blocking lock. If locking the mutex would block because the mutex
	 * is already locked by another thread, return false; otherwise
	 * return true and lock the mutex. */
	virtual bool TryLock() = 0;

	/* Unlock the mutex. This must only be called when the mutex is locked;
	 * implementations may fail with an assertion if the mutex is not locked. */
	virtual void Unlock() = 0;

  private:
	MutexImpl(const MutexImpl& rhs) = delete;
	MutexImpl& operator=(const MutexImpl& rhs) = delete;
};

class EventImpl
{
  public:
	virtual ~EventImpl() = default;
	virtual bool Wait(float timeout) = 0;
	virtual void Signal() = 0;
	virtual void Broadcast() = 0;
	virtual bool WaitTimeoutSupported() const = 0;
};

class SemaImpl
{
  public:
	virtual ~SemaImpl() = default;
	virtual int GetValue() const = 0;
	virtual void Post() = 0;
	virtual bool Wait() = 0;
	virtual bool TryWait() = 0;
};

// These functions must be implemented by the thread implementation.
ThreadImpl*
MakeThread(int (*fn)(void*), void* data, uint64_t* piThreadID);
ThreadImpl*
MakeThisThread();
MutexImpl*
MakeMutex(RageMutex* pParent);
EventImpl*
MakeEvent(MutexImpl* pMutex);
SemaImpl*
MakeSemaphore(int iInitialValue);
uint64_t
GetThisThreadId();

/* Since ThreadId is implementation-defined, we can't define a universal
 * invalid value. Return the invalid value for this implementation. */
uint64_t
GetInvalidThreadId();

#endif
