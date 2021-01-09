#ifndef THREADS_PTHREADS_H
#define THREADS_PTHREADS_H

#include "Threads.h"

#include <pthread.h>
#include <semaphore.h>

class ThreadImpl_Pthreads : public ThreadImpl
{
  public:
	pthread_t thread;

	/* Linux:
	 * Keep a list of child PIDs, so we can send them SIGKILL. This has
	 * an added bonus: if this is corrupted, we'll just send signals and
	 * they'll fail; we won't blow up (unless we're root). */
	uint64_t threadHandle;

	// These are only used during initialization.
	int (*m_pFunc)(void* pData);
	void* m_pData;
	uint64_t* m_piThreadID;
	SemaImpl* m_StartFinishedSem;

	void Halt(bool Kill);
	void Resume();
	uint64_t GetThreadId() const;
	int Wait();
};

class MutexImpl_Pthreads : public MutexImpl
{
	friend class EventImpl_Pthreads;

  public:
	MutexImpl_Pthreads(RageMutex* parent);
	~MutexImpl_Pthreads();

	bool Lock();
	bool TryLock();
	void Unlock();

  protected:
	pthread_mutex_t mutex;
};

class EventImpl_Pthreads : public EventImpl
{
  public:
	EventImpl_Pthreads(MutexImpl_Pthreads* pParent);
	~EventImpl_Pthreads();

	bool Wait(float timeout);
	void Signal();
	void Broadcast();
	bool WaitTimeoutSupported() const;

  private:
	MutexImpl_Pthreads* m_pParent;
	pthread_cond_t m_Cond;
};

#if 0
class SemaImpl_Pthreads: public SemaImpl
{
public:
	SemaImpl_Pthreads( int iInitialValue );
	~SemaImpl_Pthreads();
	int GetValue() const;
	void Post();
	bool Wait();
	bool TryWait();

private:
	sem_t sem;
};
#else
class SemaImpl_Pthreads : public SemaImpl
{
  public:
	SemaImpl_Pthreads(int iInitialValue);
	~SemaImpl_Pthreads();
	int GetValue() const { return m_iValue; }
	void Post();
	bool Wait();
	bool TryWait();

  private:
	pthread_cond_t m_Cond;
	pthread_mutex_t m_Mutex;
	unsigned m_iValue;
};

#endif

#endif
