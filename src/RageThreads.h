#ifndef RAGE_THREADS_H
#define RAGE_THREADS_H

struct ThreadSlot;
class RageTimer;
/** @brief Thread, mutex, semaphore, and event classes. */
class RageThread
{
public:
	RageThread();
	RageThread( const RageThread &cpy );
	~RageThread();

	void SetName( const RString &n ) { m_sName = n; }
	RString GetName() const { return m_sName; }
	void Create( int (*fn)(void *), void *data );

	void Halt( bool Kill=false);
	void Resume();

	/* For crash handlers: kill or suspend all threads (except for
	 * the running one) immediately. */ 
	static void HaltAllThreads( bool Kill=false );

	/* If HaltAllThreads was called (with Kill==false), resume. */
	static void ResumeAllThreads();

	static uint64_t GetCurrentThreadID();

	static const char *GetCurrentThreadName();
	static const char *GetThreadNameByID( uint64_t iID );
	static bool EnumThreadIDs( int n, uint64_t &iID );
	int Wait();
	bool IsCreated() const { return m_pSlot != nullptr; }

	/* A system can define HAVE_TLS, indicating that it can compile thread_local
	 * code, but an individual environment may not actually have functional TLS.
	 * If this returns false, thread_local variables are considered undefined. */
	static bool GetSupportsTLS() { return s_bSystemSupportsTLS; }
	static void SetSupportsTLS( bool b ) { s_bSystemSupportsTLS = b; }

	static bool GetIsShowingDialog() { return s_bIsShowingDialog; }
	static void SetIsShowingDialog( bool b ) { s_bIsShowingDialog = b; }
	static uint64_t GetInvalidThreadID();

private:
	ThreadSlot *m_pSlot;
	RString m_sName;

	static bool s_bSystemSupportsTLS;
	static bool s_bIsShowingDialog;
	
	// Swallow up warnings. If they must be used, define them.
	RageThread& operator=(const RageThread& rhs);
};

/**
 * @brief Register a thread created outside of RageThread.
 * 
 * This gives it a name for RageThread::GetCurrentThreadName,
 * and allocates a slot for checkpoints. */
class RageThreadRegister
{
public:
	RageThreadRegister( const RString &sName );
	~RageThreadRegister();

private:
	ThreadSlot *m_pSlot;
	// Swallow up warnings. If they must be used, define them.
	RageThreadRegister& operator=(const RageThreadRegister& rhs) = delete;
	RageThreadRegister(const RageThreadRegister& rhs) = delete;
};

namespace Checkpoints
{
	void LogCheckpoints( bool yes=true );
	void SetCheckpoint( const char *file, int line, const char *message );
	void GetLogs( char *pBuf, int iSize, const char *delim );
};

#define CHECKPOINT_M(m) (Checkpoints::SetCheckpoint(__FILE__, __LINE__, m))

/* Mutex class that follows the behavior of Windows mutexes: if the same
 * thread locks the same mutex twice, we just increase a refcount; a mutex
 * is considered unlocked when the refcount reaches zero.  This is more
 * convenient, though much slower on some archs.  (We don't have any tightly-
 * coupled threads, so that's OK.) */
class MutexImpl;
class RageMutex
{
public:
	RString GetName() const { return m_sName; }
	void SetName( const RString &s ) { m_sName = s; }
	virtual void Lock();
	virtual bool TryLock();
	virtual void Unlock();
	virtual bool IsLockedByThisThread() const;

	RageMutex( const RString &name );
	virtual ~RageMutex();

protected:
	MutexImpl *m_pMutex;
	RString m_sName;

	int m_UniqueID;
	
	uint64_t m_LockedBy;
	int m_LockCnt;

	void MarkLockedMutex();
private:
	// Swallow up warnings. If they must be used, define them.
	RageMutex& operator=(const RageMutex& rhs);
	RageMutex(const RageMutex& rhs);
};

/**
 * @brief Lock a mutex on construction, unlock it on destruction.
 *
 * Helps for functions with more than one return path. */
class LockMutex
{
	RageMutex &mutex;

	const char *file;
	int line;
	float locked_at;
	bool locked;

public:
	LockMutex(RageMutex &mut, const char *file, int line);
	LockMutex(RageMutex &mut): mutex(mut), file(nullptr), line(-1), locked_at(-1), locked(true) { mutex.Lock(); }
	~LockMutex();
	LockMutex(LockMutex &cpy): mutex(cpy.mutex), file(nullptr), line(-1), locked_at(cpy.locked_at), locked(true) { mutex.Lock(); }

	/**
	 * @brief Unlock the mutex (before this would normally go out of scope).
	 *
	 * This can only be called once. */
	void Unlock();
private:
	// Swallow up warnings. If they must be used, define them.
	LockMutex& operator=(const LockMutex& rhs) = delete;
};

#define LockMut(m) LockMutex SM_UNIQUE_NAME(LocalLock) (m, __FILE__, __LINE__)

class EventImpl;
class RageEvent: public RageMutex
{
public:
	RageEvent( const RString &name );
	~RageEvent() override;

	/*
	 * If pTimeout is non-NULL, the event will be automatically signalled at the given
	 * time.  Note that implementing this timeout is optional; not all archs support it. 
	 * If false is returned, the wait timed out (and the mutex is locked, as if the
	 * event had been signalled).
	 */
	bool Wait( RageTimer *pTimeout = nullptr );
	void Signal();
	void Broadcast();
	bool WaitTimeoutSupported() const;
	// Swallow up warnings. If they must be used, define them.
	RageEvent& operator=(const RageEvent& rhs);
	RageEvent(const RageEvent& rhs);

private:
	EventImpl *m_pEvent;
};

class SemaImpl;
class RageSemaphore
{
public:
	RageSemaphore( const RString &sName, int iInitialValue = 0 );
	~RageSemaphore();

	RString GetName() const { return m_sName; }
	int GetValue() const;
	void Post();
	void Wait( bool bFailOnTimeout=true );
	bool TryWait();

private:
	SemaImpl *m_pSema;
	RString m_sName;
	
	// Swallow up warnings. If they must be used, define them.
	RageSemaphore& operator=(const RageSemaphore& rhs) = delete;
	RageSemaphore(const RageSemaphore& rhs) = delete;
};

#endif

/**
 * @file
 * @author Glenn Maynard (c) 2001-2004
 * @section LICENSE
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
