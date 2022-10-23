/*
 * If you're going to use threads, remember this:
 *
 * Threads suck.
 *
 * If there's any way to avoid them, take it!  Threaded code an order of
 * magnitude more complicated, harder to debug and harder to make robust.
 *
 * That said, here are a few helpers for when they're unavoidable.
 */

#include "Etterna/Globals/global.h"

#include "Etterna/Singletons/PrefsManager.h"
#include "Core/Services/Locator.hpp"
#include "RageThreads.h"
#include "RageTimer.h"
#include "RageUtil/Utils/RageUtil.h"

#include <atomic>
#include <thread>
#include <algorithm>
#include <set>

#include "arch/Threads/Threads.h"

/* Assume TLS doesn't work until told otherwise.  It's ArchHooks's job to set
 * this. */
bool RageThread::s_bSystemSupportsTLS = false;
bool RageThread::s_bIsShowingDialog = false;

#define MAX_THREADS 128
// static std::vector<RageMutex*> *g_MutexList = NULL; /* watch out for static
// initialization order problems */

constexpr size_t MAX_THREAD_NAME_LEN = 1024;

struct ThreadSlot
{
	mutable char m_szName[MAX_THREAD_NAME_LEN]; /* mutable so we can force nul-termination */

	/* Format this beforehand, since it's easier to do that than to do it under
	 * crash conditions. */
	char m_szThreadFormattedOutput[MAX_THREAD_NAME_LEN];

	bool m_bUsed{ false };
	uint64_t m_iID;

	ThreadImpl* m_pImpl;

#undef CHECKPOINT_COUNT
#define CHECKPOINT_COUNT 5
	struct ThreadCheckpoint
	{
		const char *m_szFile, *m_szMessage;
		int m_iLine;
		char m_szFormattedBuf[1024];

		ThreadCheckpoint() { Set(NULL, 0, NULL); }
		void Set(const char* szFile, int iLine, const char* szMessage = NULL);
		const char* GetFormattedCheckpoint();
	};
	ThreadCheckpoint m_Checkpoints[CHECKPOINT_COUNT];
	int m_iCurCheckpoint{ 0 }, m_iNumCheckpoints{ 0 };
	const char* GetFormattedCheckpoint(int lineno);

	ThreadSlot()
	  : m_iID(GetInvalidThreadId())
	  , m_pImpl(NULL){};
	void Init()
	{
		m_iID = GetInvalidThreadId();
		m_iCurCheckpoint = m_iNumCheckpoints = 0;
		m_pImpl = NULL;

		/* Reset used last; otherwise, a thread creation might pick up the slot.
		 */
		m_bUsed = false;
	}

	void Release()
	{
		SAFE_DELETE(m_pImpl);
		Init();
	}

	const char* GetThreadName() const;
};

void
ThreadSlot::ThreadCheckpoint::Set(const char* szFile,
								  int iLine,
								  const char* szMessage)
{
	m_szFile = szFile;
	m_iLine = iLine;
	m_szMessage = szMessage;

	/* Skip any path components. */
	if (m_szFile != NULL) {
		const char* p = strrchr(m_szFile, '/');
		if (p == NULL)
			p = strrchr(m_szFile, '\\');
		if (p != NULL && p[1] != '\0')
			m_szFile = p + 1;
	}

	snprintf(m_szFormattedBuf,
			 sizeof(m_szFormattedBuf),
			 "        %s:%i %s",
			 m_szFile,
			 m_iLine,
			 m_szMessage ? m_szMessage : "");
}

const char*
ThreadSlot::ThreadCheckpoint::GetFormattedCheckpoint()
{
	if (m_szFile == NULL)
		return NULL;

	/* Make sure it's terminated: */
	m_szFormattedBuf[sizeof(m_szFormattedBuf) - 1] = 0;

	return m_szFormattedBuf;
}

const char*
ThreadSlot::GetFormattedCheckpoint(int lineno)
{
	if (lineno >= CHECKPOINT_COUNT || lineno >= m_iNumCheckpoints)
		return NULL;

	if (m_iNumCheckpoints == CHECKPOINT_COUNT) {
		lineno += m_iCurCheckpoint;
		lineno %= CHECKPOINT_COUNT;
	}

	return m_Checkpoints[lineno].GetFormattedCheckpoint();
}

static ThreadSlot g_ThreadSlots[MAX_THREADS];
struct ThreadSlot* g_pUnknownThreadSlot = NULL;

/* Lock this mutex before using or modifying m_pImpl.  Other values are just
 * identifiers, so possibly racing over them is harmless (simply using a stale
 * thread ID, etc). */
static RageMutex&
GetThreadSlotsLock()
{
	static RageMutex* pLock = new RageMutex("ThreadSlots");
	return *pLock;
}

static int
FindEmptyThreadSlot()
{
	LockMut(GetThreadSlotsLock());
	for (int entry = 0; entry < MAX_THREADS; ++entry) {
		if (g_ThreadSlots[entry].m_bUsed)
			continue;

		g_ThreadSlots[entry].m_bUsed = true;
		return entry;
	}

	RageException::Throw("Out of thread slots!");
}

static void
InitThreads()
{
	/* We don't have to worry about two threads calling this at once, since it's
	 * called when we create a thread. */
	static bool bInitialized = false;
	if (bInitialized)
		return;

	LockMut(GetThreadSlotsLock());

	/* Libraries might start threads on their own, which might call user
	 * callbacks, which could come back here.  Make sure we don't accidentally
	 * initialize twice. */
	if (bInitialized)
		return;

	bInitialized = true;

	/* Register the "unknown thread" slot. */
	const int slot = FindEmptyThreadSlot();
	strcpy(g_ThreadSlots[slot].m_szName, "Unknown thread");
	g_ThreadSlots[slot].m_iID = GetInvalidThreadId();
	sprintf(g_ThreadSlots[slot].m_szThreadFormattedOutput, "Unknown thread");
	g_pUnknownThreadSlot = &g_ThreadSlots[slot];
}

static ThreadSlot*
GetThreadSlotFromID(uint64_t iID)
{
	InitThreads();

	for (auto& g_ThreadSlot : g_ThreadSlots) {
		if (!g_ThreadSlot.m_bUsed)
			continue;
		if (g_ThreadSlot.m_iID == iID)
			return &g_ThreadSlot;
	}
	return NULL;
}

RageThread::RageThread()
  : m_pSlot(NULL)
  , m_sName("unnamed")
{
}

/* Copying a thread does not start the copy. */
RageThread::RageThread(const RageThread& cpy)
  : m_pSlot(NULL)
  , m_sName(cpy.m_sName)
{
}

RageThread::~RageThread()
{
	if (m_pSlot != NULL)
		Wait();
}

const char*
ThreadSlot::GetThreadName() const
{
	/* This function may be called in crash conditions, so guarantee the string
	 * is null-terminated. */
	m_szName[sizeof(m_szName) - 1] = 0;

	return m_szName;
}

void
RageThread::Create(int (*fn)(void*), void* data)
{
	/* Don't create a thread that's already running: */
	ASSERT(m_pSlot == NULL);

	InitThreads();

	/* Lock unused slots, so nothing else uses our slot before we mark it used.
	 */
	LockMut(GetThreadSlotsLock());

	const int slotno = FindEmptyThreadSlot();
	m_pSlot = &g_ThreadSlots[slotno];

	strncpy(m_pSlot->m_szName, m_sName.c_str(), MAX_THREAD_NAME_LEN - 1);
	m_pSlot->m_szName[MAX_THREAD_NAME_LEN - 1] = '\0';

	Locator::getLogger()->info("Starting thread: {}", m_sName.c_str());
	snprintf(m_pSlot->m_szThreadFormattedOutput, MAX_THREAD_NAME_LEN - 1, "Thread: %s", m_sName.c_str());
	m_pSlot->m_szThreadFormattedOutput[MAX_THREAD_NAME_LEN - 1] = '\0';

	/* Start a thread using our own startup function.  We pass the id to fill
	 * in, to make sure it's set before the thread actually starts.  (Otherwise,
	 * early checkpoints might not have a completely set-up thread slot.) */
	m_pSlot->m_pImpl = MakeThread(fn, data, &m_pSlot->m_iID);
}

RageThreadRegister::RageThreadRegister(const std::string& sName)
{
	InitThreads();
	LockMut(GetThreadSlotsLock());

	const int iSlot = FindEmptyThreadSlot();

	m_pSlot = &g_ThreadSlots[iSlot];

	strcpy(m_pSlot->m_szName, sName.c_str());
	sprintf(m_pSlot->m_szThreadFormattedOutput, "Thread: %s", sName.c_str());

	m_pSlot->m_iID = GetThisThreadId();
	m_pSlot->m_pImpl = MakeThisThread();
}

RageThreadRegister::~RageThreadRegister()
{
	LockMut(GetThreadSlotsLock());

	m_pSlot->Release();
	m_pSlot = NULL;
}

const char*
RageThread::GetCurrentThreadName()
{
	return GetThreadNameByID(GetCurrentThreadID());
}

const char*
RageThread::GetThreadNameByID(uint64_t iID)
{
	ThreadSlot* slot = GetThreadSlotFromID(iID);
	if (slot == NULL)
		return "???";

	return slot->GetThreadName();
}

bool
RageThread::EnumThreadIDs(int n, uint64_t& iID)
{
	if (n >= MAX_THREADS)
		return false;

	LockMut(GetThreadSlotsLock());
	const ThreadSlot* slot = &g_ThreadSlots[n];

	if (slot->m_bUsed)
		iID = slot->m_iID;
	else
		iID = GetInvalidThreadId();

	return true;
}

int
RageThread::Wait()
{
	ASSERT(m_pSlot != NULL);
	ASSERT(m_pSlot->m_pImpl != NULL);
	const int ret = m_pSlot->m_pImpl->Wait();

	LockMut(GetThreadSlotsLock());

	m_pSlot->Release();
	m_pSlot = NULL;

	return ret;
}

void
RageThread::Halt(bool Kill)
{
	ASSERT(m_pSlot != NULL);
	ASSERT(m_pSlot->m_pImpl != NULL);
	m_pSlot->m_pImpl->Halt(Kill);
}

void
RageThread::Resume()
{
	ASSERT(m_pSlot != NULL);
	ASSERT(m_pSlot->m_pImpl != NULL);
	m_pSlot->m_pImpl->Resume();
}

void
RageThread::HaltAllThreads(bool Kill)
{
	const uint64_t ThisThreadID = GetThisThreadId();
	for (auto& g_ThreadSlot : g_ThreadSlots) {
		if (!g_ThreadSlot.m_bUsed)
			continue;
		if (ThisThreadID == g_ThreadSlot.m_iID || g_ThreadSlot.m_pImpl == NULL)
			continue;
		g_ThreadSlot.m_pImpl->Halt(Kill);
	}
}

void
RageThread::ResumeAllThreads()
{
	const uint64_t ThisThreadID = GetThisThreadId();
	for (auto& g_ThreadSlot : g_ThreadSlots) {
		if (!g_ThreadSlot.m_bUsed)
			continue;
		if (ThisThreadID == g_ThreadSlot.m_iID || g_ThreadSlot.m_pImpl == NULL)
			continue;

		g_ThreadSlot.m_pImpl->Resume();
	}
}

uint64_t
RageThread::GetCurrentThreadID()
{
	return GetThisThreadId();
}
uint64_t
RageThread::GetInvalidThreadID()
{
	return GetInvalidThreadId();
}

/*
 * "Safe" mutexes: locking the same mutex more than once from the same
 * thread is refcounted and does not deadlock.
 *
 * Only actually lock the mutex once; when we do so, remember which thread
 * locked it. Then, when we lock in the future, only increment a counter,
 * with no locks.
 *
 * We must be holding the real mutex to write to LockedBy and LockCnt.
 * However, we can look at LockedBy to see if it's us that owns it (in which
 * case, we already hold the mutex).
 *
 * In Windows, this helps smooth out performance: for some reason, Windows
 * likes to yank the scheduler away from a thread that locks a mutex that it
 * already owns.
 */

RageMutex::RageMutex(const std::string& name)
  : m_pMutex(MakeMutex(this))
  , m_sName(name)
  , m_LockedBy(GetInvalidThreadId())
  , m_LockCnt(0)
{
}

RageMutex::~RageMutex()
{
	delete m_pMutex;
}

void
RageMutex::Lock()
{
	const uint64_t iThisThreadId = GetThisThreadId();
	if (m_LockedBy == iThisThreadId) {
		++m_LockCnt;
		return;
	}

	if (!m_pMutex->Lock()) {
		const ThreadSlot* ThisSlot = GetThreadSlotFromID(GetThisThreadId());
		const ThreadSlot* OtherSlot = GetThreadSlotFromID(m_LockedBy);

		std::string ThisSlotName = "(???"
								   ")"; // stupid trigraph warnings
		std::string OtherSlotName = "(???"
									")"; // stupid trigraph warnings
		if (ThisSlot)
			ThisSlotName = ssprintf("%s (%i)",
									ThisSlot->GetThreadName(),
									static_cast<int>(ThisSlot->m_iID));
		if (OtherSlot)
			OtherSlotName = ssprintf("%s (%i)",
									 OtherSlot->GetThreadName(),
									 static_cast<int>(OtherSlot->m_iID));

		/* Don't leave GetThreadSlotsLock() locked when we call
		 * ForceCrashHandlerDeadlock. */
		GetThreadSlotsLock().Lock();
		const uint64_t CrashHandle = OtherSlot ? OtherSlot->m_iID : 0;
		GetThreadSlotsLock().Unlock();

		const std::string sReason =
		  ssprintf("Thread deadlock on mutex %s between %s and %s CrashHandle %d",
				   GetName().c_str(),
				   ThisSlotName.c_str(),
				   OtherSlotName.c_str(),
				   CrashHandle);

		sm_crash(sReason.c_str());
	}

	m_LockedBy = iThisThreadId;

	/* This has internal thread safety issues itself (eg. one thread may delete
	 * a mutex while another locks one); disable for now. */
	//	MarkLockedMutex();
}

bool
RageMutex::TryLock()
{
	if (m_LockedBy == GetThisThreadId()) {
		++m_LockCnt;
		return true;
	}

	if (!m_pMutex->TryLock())
		return false;

	m_LockedBy = GetThisThreadId();

	return true;
}

void
RageMutex::Unlock()
{
	if (m_LockCnt) {
		--m_LockCnt;
		return;
	}

	m_LockedBy = GetInvalidThreadId();

	m_pMutex->Unlock();
}

bool
RageMutex::IsLockedByThisThread() const
{
	return m_LockedBy == GetThisThreadId();
}

LockMutex::LockMutex(RageMutex& pMutex, const char* file_, int line_)
  : mutex(pMutex)
  , file(file_)
  , line(line_)
  , locked_at(RageTimer::GetTimeSinceStart())
  , locked(false) // ensure it gets locked inside.
{
	mutex.Lock();
	locked = true;
}

LockMutex::~LockMutex()
{
	if (locked)
		mutex.Unlock();
}

void
LockMutex::Unlock()
{
	ASSERT(locked);
	locked = false;

	mutex.Unlock();

	if (file && locked_at != -1) {
		const float dur = RageTimer::GetTimeSinceStart() - locked_at;
		if (dur > 0.015f)
			Locator::getLogger()->trace("Lock at {}:{} took {}", file, line, dur);
	}
}

RageEvent::RageEvent(const std::string& name)
  : RageMutex(name)
  , m_pEvent(MakeEvent(m_pMutex))
{
}

RageEvent::~RageEvent()
{
	delete m_pEvent;
}

/* For each of these calls, the mutex must be locked, and must not be locked
 * recursively. */
bool
RageEvent::Wait(float timeout)
{
	ASSERT(IsLockedByThisThread());
	ASSERT(m_LockCnt == 0);

	const bool bRet = m_pEvent->Wait(timeout);

	m_LockedBy = GetThisThreadId();
	return bRet;
}

void
RageEvent::Signal()
{
	ASSERT(IsLockedByThisThread());
	ASSERT(m_LockCnt == 0);
	m_pEvent->Signal();
}

void
RageEvent::Broadcast()
{
	ASSERT(IsLockedByThisThread());
	ASSERT(m_LockCnt == 0);
	m_pEvent->Broadcast();
}

bool
RageEvent::WaitTimeoutSupported() const
{
	return m_pEvent->WaitTimeoutSupported();
}

RageSemaphore::RageSemaphore(const std::string& sName, int iInitialValue)
  : m_pSema(MakeSemaphore(iInitialValue))
  , m_sName(sName)
{
}

RageSemaphore::~RageSemaphore()
{
	delete m_pSema;
}

int
RageSemaphore::GetValue() const
{
	return m_pSema->GetValue();
}

void
RageSemaphore::Post()
{
	m_pSema->Post();
}

void
RageSemaphore::Wait(bool bFailOnTimeout)
{
	do {
		if (m_pSema->Wait())
			return;
	} while (!bFailOnTimeout || RageThread::GetIsShowingDialog());

	/* We waited too long.  We're probably deadlocked, though unlike mutexes, we
	 * can't tell which thread we're stuck on. */
	const ThreadSlot* ThisSlot = GetThreadSlotFromID(GetThisThreadId());
	const std::string sReason =
	  ssprintf("Semaphore timeout on mutex %s on thread %s",
			   GetName().c_str(),
			   ThisSlot ? ThisSlot->GetThreadName()
						: "(???"
						  ")"); // stupid trigraph warnings
//	CrashHandler::ForceDeadlock(sReason, GetInvalidThreadId());
}

bool
RageSemaphore::TryWait()
{
	return m_pSema->TryWait();
}
