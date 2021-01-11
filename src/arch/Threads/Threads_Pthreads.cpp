#include "Etterna/Globals/global.h"
#include "Threads_Pthreads.h"
#include "RageUtil/Misc/RageTimer.h"
#include "RageUtil/Utils/RageUtil.h"
#include <sys/time.h>
#include <errno.h>
#include <chrono>
#include <cmath>

#ifdef __unix__
#include "archutils/Unix/RunningUnderValgrind.h"
#endif

#ifdef __APPLE__
#include "archutils/Darwin/DarwinThreadHelpers.h"
#else
#include "archutils/Common/PthreadHelpers.h"
#endif

void
ThreadImpl_Pthreads::Halt(bool Kill)
{
	/* Linux:
	 * Send a SIGSTOP to the thread. If we send a SIGKILL, pthreads will
	 * "helpfully" propagate it to the other threads, and we'll get killed, too.
	 * This isn't ideal, since it can cause the process to background as far as
	 * the shell is concerned, so the shell prompt can display before the crash
	 * handler actually displays a message. */
	SuspendThread(threadHandle);
}

void
ThreadImpl_Pthreads::Resume()
{
	// Linux: Send a SIGCONT to the thread.
	ResumeThread(threadHandle);
}

uint64_t
ThreadImpl_Pthreads::GetThreadId() const
{
	return threadHandle;
}

int
ThreadImpl_Pthreads::Wait()
{
	int* val;
	int ret = pthread_join(thread, (void**)&val);
	ASSERT_M(ret == 0, ssprintf("pthread_join: %s", strerror(ret)));

	int iRet = *val;
	delete val;
	return iRet;
}

ThreadImpl*
MakeThisThread()
{
	ThreadImpl_Pthreads* thread = new ThreadImpl_Pthreads;

	thread->thread = pthread_self();
	thread->threadHandle = GetCurrentThreadId();

	return thread;
}

static void*
StartThread(void* pData)
{
	ThreadImpl_Pthreads* pThis = (ThreadImpl_Pthreads*)pData;

	pThis->threadHandle = GetCurrentThreadId();
	*pThis->m_piThreadID = pThis->threadHandle;

	// Tell MakeThread that we've set m_piThreadID, so it's safe to return.
	pThis->m_StartFinishedSem->Post();

	int iRet = pThis->m_pFunc(pThis->m_pData);

	return new int(iRet);
}

ThreadImpl*
MakeThread(int (*pFunc)(void* pData), void* pData, uint64_t* piThreadID)
{
	ThreadImpl_Pthreads* thread = new ThreadImpl_Pthreads;
	thread->m_pFunc = pFunc;
	thread->m_pData = pData;
	thread->m_piThreadID = piThreadID;

	thread->m_StartFinishedSem = new SemaImpl_Pthreads(0);

	int ret = pthread_create(&thread->thread, NULL, StartThread, thread);
	ASSERT_M(ret == 0,
			 ssprintf("MakeThread: pthread_create: %s", strerror(errno)));

	// Don't return until StartThread sets m_piThreadID.
	thread->m_StartFinishedSem->Wait();
	delete thread->m_StartFinishedSem;

	return thread;
}

MutexImpl_Pthreads::MutexImpl_Pthreads(RageMutex* pParent)
  : MutexImpl(pParent)
{
	pthread_mutex_init(&mutex, NULL);
}

MutexImpl_Pthreads::~MutexImpl_Pthreads()
{
	int ret = pthread_mutex_destroy(&mutex) == -1;
	ASSERT_M(ret == 0, ssprintf("Error deleting mutex: %s", strerror(errno)));
}

#if defined(HAVE_PTHREAD_MUTEX_TIMEDLOCK) ||                                   \
  defined(HAVE_PTHREAD_COND_TIMEDWAIT)
static bool
UseTimedlock()
{
#ifdef __linux__
	// Valgrind crashes and burns on pthread_mutex_timedlock.
	if (RunningUnderValgrind())
		return false;
#endif

	return true;
}
#endif

bool
MutexImpl_Pthreads::Lock()
{
#if defined(HAVE_PTHREAD_MUTEX_TIMEDLOCK)
	if (UseTimedlock()) {
		int len = 10; // seconds
		int tries = 2;

		while (tries--) {
			/* Wait for ten seconds. If it takes longer than that, we're
			 * probably deadlocked. */
			timeval tv;
			gettimeofday(&tv, NULL);

			timespec ts;
			ts.tv_sec = tv.tv_sec + len;
			ts.tv_nsec = tv.tv_usec * 1000;
			int ret = pthread_mutex_timedlock(&mutex, &ts);
			switch (ret) {
				case 0:
					return true;

				case EINTR:
					/* Ignore it. */
					++tries;
					continue;

				case ETIMEDOUT:
					/* Timed out. Probably deadlocked. Try again one more time,
					 * with a smaller timeout, just in case we're debugging
					 * and happened to stop while waiting on the mutex. */
					len = 1;
					break;

				default:
					FAIL_M(
					  ssprintf("pthread_mutex_timedlock: %s", strerror(errno)));
			}
		}

		return false;
	}
#endif

	int ret = pthread_mutex_lock(&mutex);

	ASSERT_M(ret == 0, ssprintf("pthread_mutex_lock: %s", strerror(errno)));

	return true;
}

bool
MutexImpl_Pthreads::TryLock()
{
	int ret = pthread_mutex_trylock(&mutex);
	if (ret == EBUSY)
		return false;
	ASSERT_M(ret == 0,
			 ssprintf("pthread_mutex_trylock failed: %s", strerror(errno)));
	return true;
}

void
MutexImpl_Pthreads::Unlock()
{
	pthread_mutex_unlock(&mutex);
}

uint64_t
GetThisThreadId()
{
	return GetCurrentThreadId();
}

uint64_t
GetInvalidThreadId()
{
	return 0;
}

MutexImpl*
MakeMutex(RageMutex* pParent)
{
	return new MutexImpl_Pthreads(pParent);
}

/* Check if condattr_setclock is supported, and supports the clock that
 * RageTimer selected. */
#ifdef __unix__
#include <dlfcn.h>
#endif // On MinGW clockid_t is defined in pthread.h
namespace {
typedef int (*CONDATTR_SET_CLOCK)(pthread_condattr_t* attr, clockid_t clock_id);
CONDATTR_SET_CLOCK g_CondattrSetclock = NULL;
bool bInitialized = false;

#ifdef __unix__
clockid_t
GetClock()
{
	return CLOCK_MONOTONIC;
}

void
InitMonotonic()
{
	if (bInitialized)
		return;
	bInitialized = true;

	void* pLib = NULL;

	do {
		{
			pLib = dlopen(NULL, RTLD_LAZY);
			if (pLib == NULL)
				break;

			g_CondattrSetclock =
			  (CONDATTR_SET_CLOCK)dlsym(pLib, "pthread_condattr_setclock");

			if (g_CondattrSetclock == NULL)
				break;
		}

		// Make sure that we can set up the clock attribute.
		pthread_condattr_t condattr;
		pthread_condattr_init(&condattr);

		if (g_CondattrSetclock(&condattr, GetClock()) != 0) {
			printf("pthread_condattr_setclock failed\n");
			pthread_condattr_destroy(&condattr);
			break;
		}
		pthread_condattr_destroy(&condattr);

		/* Everything seems to work. */
		if (pLib != NULL)
			dlclose(pLib);
		return;
	} while (0);

	g_CondattrSetclock = NULL;
	if (pLib != NULL)
		dlclose(pLib);
	pLib = NULL;
}
#elif defined(__APPLE__)
void
InitMonotonic()
{
	bInitialized = true;
}
clockid_t
GetClock()
{
	return CLOCK_MONOTONIC;
}
#else
void
InitMonotonic()
{
	bInitialized = true;
}

clockid_t
GetClock()
{
	return CLOCK_REALTIME;
}
#endif
};

EventImpl_Pthreads::EventImpl_Pthreads(MutexImpl_Pthreads* pParent)
{
	m_pParent = pParent;

	InitMonotonic();

	pthread_condattr_t condattr;
	pthread_condattr_init(&condattr);

	if (g_CondattrSetclock != NULL)
		g_CondattrSetclock(&condattr, GetClock());

	pthread_cond_init(&m_Cond, &condattr);
	pthread_condattr_destroy(&condattr);
}

EventImpl_Pthreads::~EventImpl_Pthreads()
{
	pthread_cond_destroy(&m_Cond);
}

#if defined(HAVE_PTHREAD_COND_TIMEDWAIT)
bool
EventImpl_Pthreads::Wait(float timeout)
{
	if (timeout <= 0.F) {
		pthread_cond_wait(&m_Cond, &m_pParent->mutex);
		return true;
	}

	/* If the clock is not CLOCK_MONOTONIC, or we can't change the wait clock
	 * (no condattr_setclock), pthread_cond_timedwait has an inherent race
	 * condition: the system clock may change before we call it. */
	timespec abstime;
	if (g_CondattrSetclock != NULL || GetClock() == CLOCK_REALTIME) {
		/* If we support condattr_setclock, we'll set the condition to use
		 * the same clock as RageTimer and can use it directly. If the
		 * clock is CLOCK_REALTIME, that's the default anyway. */

#ifdef __APPLE__
		float isec;
#else
		int isec;
#endif
		auto fnsec = modf(timeout, &isec);
		long nsec = static_cast<long>(fnsec * 1000000000.0);
		time_t sec = static_cast<time_t>(isec);
		abstime.tv_sec = sec;
		abstime.tv_nsec = nsec;
	} else {
		// The RageTimer clock is different than the wait clock; convert it.
		timeval tv;
		gettimeofday(&tv, NULL);
		
		RageTimer timeofday(tv.tv_sec, tv.tv_usec);
		float fSecondsInFuture = timeout;
		timeofday += fSecondsInFuture;

		auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(timeofday.c_dur);
		auto sec = std::chrono::duration_cast<std::chrono::seconds>(timeofday.c_dur);
		nsec -= sec;
		abstime.tv_sec = sec.count();
		abstime.tv_nsec = nsec.count();
	}
	int iRet = pthread_cond_timedwait(&m_Cond, &m_pParent->mutex, &abstime);
	return iRet != ETIMEDOUT;
}

bool
EventImpl_Pthreads::WaitTimeoutSupported() const
{
	return true;
}
#else
bool
EventImpl_Pthreads::Wait(float timeout)
{
	pthread_cond_wait(&m_Cond, &m_pParent->mutex);
	return true;
}

bool
EventImpl_Pthreads::WaitTimeoutSupported() const
{
	return false;
}
#endif

void
EventImpl_Pthreads::Signal()
{
	pthread_cond_signal(&m_Cond);
}

void
EventImpl_Pthreads::Broadcast()
{
	pthread_cond_broadcast(&m_Cond);
}

EventImpl*
MakeEvent(MutexImpl* pMutex)
{
	MutexImpl_Pthreads* pPthreadsMutex = (MutexImpl_Pthreads*)pMutex;

	return new EventImpl_Pthreads(pPthreadsMutex);
}

#if 0
SemaImpl_Pthreads::SemaImpl_Pthreads( int iInitialValue )
{
	sem_init( &sem, 0, iInitialValue );
}

SemaImpl_Pthreads::~SemaImpl_Pthreads()
{
	sem_destroy( &sem );
}

int SemaImpl_Pthreads::GetValue() const
{
	int ret;
	sem_getvalue( const_cast<sem_t *>(&sem), &ret );
	return ret;
}

void SemaImpl_Pthreads::Post()
{
	sem_post( &sem );
}

bool SemaImpl_Pthreads::Wait()
{
	int ret;
	do
	{
		ret = sem_wait( &sem );
	}
	while( ret == -1 && errno == EINTR );

	ASSERT_M( ret == 0, ssprintf("Wait: sem_wait: %s", strerror(errno)) );

	return true;
}

bool SemaImpl_Pthreads::TryWait()
{
	int ret = sem_trywait( &sem );
	if( ret == -1 && errno == EAGAIN )
		return false;
	
	ASSERT_M( ret == 0, ssprintf("TryWait: sem_trywait failed: %s", strerror(errno)) );

	return true;
}
#else
// Use conditions, to work around OS X "forgetting" to implement semaphores.
SemaImpl_Pthreads::SemaImpl_Pthreads(int iInitialValue)
{
	int ret = pthread_cond_init(&m_Cond, NULL);
	ASSERT_M(
	  ret == 0,
	  ssprintf("SemaImpl_Pthreads: pthread_cond_init: %s", strerror(errno)));
	ret = pthread_mutex_init(&m_Mutex, NULL);
	ASSERT_M(
	  ret == 0,
	  ssprintf("SemaImpl_Pthreads: pthread_mutex_init: %s", strerror(errno)));

	m_iValue = iInitialValue;
}

SemaImpl_Pthreads::~SemaImpl_Pthreads()
{
	pthread_cond_destroy(&m_Cond);
	pthread_mutex_destroy(&m_Mutex);
}

void
SemaImpl_Pthreads::Post()
{
	pthread_mutex_lock(&m_Mutex);
	++m_iValue;
	if (m_iValue == 1)
		pthread_cond_signal(&m_Cond);
	pthread_mutex_unlock(&m_Mutex);
}

bool
SemaImpl_Pthreads::Wait()
{
#if defined(HAVE_PTHREAD_COND_TIMEDWAIT)
	if (UseTimedlock()) {
		timeval tv;
		gettimeofday(&tv, NULL);

		/* Wait for ten seconds.  If it takes longer than that, we're probably
		 * deadlocked. */
		timespec ts;
		ts.tv_sec = tv.tv_sec + 10;
		ts.tv_nsec = tv.tv_usec * 1000;

		pthread_mutex_lock(&m_Mutex);

		int tries = 2;
		while (!m_iValue && tries) {
			int ret = pthread_cond_timedwait(&m_Cond, &m_Mutex, &ts);

			switch (ret) {
				case 0:
				case EINTR:
					break;

				case ETIMEDOUT:
					/* Timed out. Probably deadlocked. Try again one more time,
					 * with a smaller timeout, just in case we're debugging and
					 * happened to stop while waiting on the mutex. */
					++ts.tv_sec;
					tries--;
					break;

				default:
					FAIL_M(
					  ssprintf("pthread_mutex_timedlock: %s", strerror(errno)));
			}
		}

		if (!m_iValue) {
			/* Timed out. */
			pthread_mutex_unlock(&m_Mutex);
			return false;
		} else {
			--m_iValue;
			pthread_mutex_unlock(&m_Mutex);
			return true;
		}
	}
#endif

	pthread_mutex_lock(&m_Mutex);
	while (!m_iValue)
		pthread_cond_wait(&m_Cond, &m_Mutex);

	--m_iValue;
	pthread_mutex_unlock(&m_Mutex);

	return true;
}

bool
SemaImpl_Pthreads::TryWait()
{
	pthread_mutex_lock(&m_Mutex);
	if (!m_iValue) {
		pthread_mutex_unlock(&m_Mutex);
		return false;
	}

	--m_iValue;
	pthread_mutex_unlock(&m_Mutex);

	return true;
}

#endif

SemaImpl*
MakeSemaphore(int iInitialValue)
{
	return new SemaImpl_Pthreads(iInitialValue);
}
