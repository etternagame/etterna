/* RageThreads helpers for threads in Linux, which are based on PIDs and TIDs.
 */
#include "PthreadHelpers.h"

#include "Etterna/Globals/global.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Misc/RageThreads.h"
#include "archutils/Unix/Backtrace.h" // HACK: This should be platform-agnosticized
#ifdef __unix__
#include "archutils/Unix/RunningUnderValgrind.h"
#endif

#ifdef __linux__
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <stdlib.h>
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#define _LINUX_PTRACE_H // hack to prevent broken linux/ptrace.h from
						// conflicting with sys/ptrace.h
#include <sys/user.h>

/* In Linux, we might be using PID-based or TID-based threads.  With PID-based
 * threads, getpid() returns a unique value for each thread; each thread is a
 * separate process.  Newer kernels using NPTL return the same PID for all
 * threads; these systems support a gettid() call to get a unique TID for each
 * thread. */

static bool g_bUsingNPTL = false;

#define gettid() syscall(SYS_gettid)

#ifndef _CS_GNU_LIBPTHREAD_VERSION
#define _CS_GNU_LIBPTHREAD_VERSION 3
#endif

RString
ThreadsVersion()
{
	char buf[1024] = "(error)";
	int ret = confstr(_CS_GNU_LIBPTHREAD_VERSION, buf, sizeof(buf));
	if (ret == -1)
		return "(unknown)";

	return buf;
}

/* Crash-conditions-safe: */
bool
UsingNPTL()
{
	char buf[1024] = "";
	int ret = confstr(_CS_GNU_LIBPTHREAD_VERSION, buf, sizeof(buf));
	if (ret == -1)
		return false;

	return !strncmp(buf, "NPTL", 4);
}
/* Crash-conditions-safe: */
void
InitializePidThreadHelpers()
{
	static bool bInitialized = false;
	if (bInitialized)
		return;
	bInitialized = true;

	g_bUsingNPTL = UsingNPTL();
}

/* waitpid(); ThreadID can be a PID or (in NPTL) a TID; doesn't care if the ID
 * is a clone() or not. */
static int
waittid(int ThreadID, int* status, int options)
{
	static bool bSupportsWall = true;

	if (bSupportsWall) {
		int ret = waitpid(ThreadID, status, options | __WALL);
		if (ret != -1 || errno != EINVAL)
			return ret;
		bSupportsWall = false;
	}

	/* XXX: on 2.2, we need to use __WCLONE only if ThreadID isn't the main
	 * thread; perhaps wait and retry without it if errno == ECHILD? */
	int ret;
	ret = waitpid(ThreadID, status, options);

	return ret;
}

/* Attempt to PTRACE_ATTACH to a thread, and wait for the SIGSTOP. */
static int
PtraceAttach(int ThreadID)
{
	int ret;
	ret = ptrace(PTRACE_ATTACH, ThreadID, NULL, NULL);
	if (ret == -1) {
		printf("ptrace failed: %s\n", strerror(errno));
		return -1;
	}

	/* Wait for the SIGSTOP from the ptrace call. */
	int status;
	ret = waittid(ThreadID, &status, 0);
	if (ret == -1)
		return -1;

	//	printf( "ret %i, exited %i, signalled %i, sig %i, stopped %i, stopsig
	//%i\n", ret, WIFEXITED(status), 			WIFSIGNALED(status),
	// WTERMSIG(status), WIFSTOPPED(status), WSTOPSIG(status));
	return 0;
}

static int
PtraceDetach(int ThreadID)
{
	return ptrace(PTRACE_DETACH, ThreadID, NULL, NULL);
}

/* Get this thread's ID (this may be a TID or a PID). */
static uint64_t
GetCurrentThreadIdInternal()
{
	/* If we're under Valgrind, neither the PID nor the TID is associated with
	 * the thread.  Return the pthread ID.  This can't be used to kill threads,
	 * etc., but that only happens under error conditions anyway.  If we don't
	 * return a usable, unique ID, then mutexes won't work. */
	if (RunningUnderValgrind())
		return (int)pthread_self();

	InitializePidThreadHelpers(); // for g_bUsingNPTL

	/* Don't keep calling gettid() if it's not supported; it'll make valgrind
	 * spam us. */
	static bool GetTidUnsupported = 0;
	if (!GetTidUnsupported) {
		pid_t ret = gettid();

		/* If this fails with ENOSYS, we're on a kernel before gettid, or we're
		 * under valgrind.  If we don't have NPTL, then just use getpid().  If
		 * we're on NPTL and don't have gettid(), something's wrong. */
		if (ret != -1)
			return ret;

		ASSERT(!g_bUsingNPTL);
		GetTidUnsupported = true;
	}

	return getpid();
}

uint64_t
GetCurrentThreadId()
{
#if defined(HAVE_TLS)
	/* This is called each time we lock a mutex, and gettid() is a little slow,
	 * so cache the result if we support TLS. */
	if (RageThread::GetSupportsTLS()) {
		static thread_local uint64_t cached_tid = 0;
		static thread_local bool cached = false;
		if (!cached) {
			cached_tid = GetCurrentThreadIdInternal();
			cached = true;
		}
		return cached_tid;
	}
#endif

	return GetCurrentThreadIdInternal();
}

int
SuspendThread(uint64_t ThreadID)
{
	/*
	 * Linux: We can't simply kill(SIGSTOP) (or tkill), since that will stop all
	 * processes (grr).  We can ptrace(PTRACE_ATTACH) the process to stop it,
	 * and PTRACE_DETACH to restart it.
	 */
	return PtraceAttach(int(ThreadID));
	// kill( ThreadID, SIGSTOP );
}

int
ResumeThread(uint64_t ThreadID)
{
	return PtraceDetach(int(ThreadID));
	// kill( ThreadID, SIGSTOP );
}

/* Get a BacktraceContext for a thread.  ThreadID must not be the current
 * thread.
 *
 * tid() is a PID (from getpid) or a TID (from gettid).  Note that this may have
 * kernel compatibility problems, because NPTL is new and its interactions with
 * ptrace() aren't well-defined. If we're on a non-NPTL system, tid is a regular
 * PID.
 *
 * This call leaves the given thread suspended, so the returned context doesn't
 * become invalid. ResumeThread() can be used to resume a thread after this
 * call. */
bool
GetThreadBacktraceContext(uint64_t ThreadID, BacktraceContext* ctx)
{
	/* Can't GetThreadBacktraceContext the current thread. */
	ASSERT(ThreadID != GetCurrentThreadId());

	/* Attach to the thread.  This may fail with EPERM.  This can happen for at
	 * least two common reasons: the process might be in a debugger already, or
	 * *we* might already have attached to it via SuspendThread.
	 *
	 * If it's in a debugger, we won't be able to ptrace(PTRACE_GETREGS). If
	 * it's us that attached, we will. */
	if (PtraceAttach(int(ThreadID)) == -1 && errno != EPERM) {
		CHECKPOINT_M(ssprintf("%s (pid %i tid %i locking tid %i)",
							  strerror(errno),
							  getpid(),
							  (int)GetCurrentThreadId(),
							  int(ThreadID)));
		return false;
	}

#if defined(__x86_64__) || defined(__i386__)
	user_regs_struct regs;
	if (ptrace(PTRACE_GETREGS, pid_t(ThreadID), NULL, &regs) == -1)
		return false;

	ctx->pid = pid_t(ThreadID);
#if defined(__x86_64__)
	ctx->ip = (void*)regs.rip;
	ctx->bp = (void*)regs.rbp;
	ctx->sp = (void*)regs.rsp;
#else
	ctx->ip = (void*)regs.eip;
	ctx->bp = (void*)regs.ebp;
	ctx->sp = (void*)regs.esp;
#endif
#elif defined(CPU_PPC)
	errno = 0;
	ctx->FramePtr = (const Frame*)ptrace(
	  PTRACE_PEEKUSER, pid_t(ThreadID), (void*)(PT_R1 << 2), 0);
	if (errno)
		return false;
	ctx->PC =
	  (void*)ptrace(PTRACE_PEEKUSER, pid_t(ThreadID), (void*)(PT_NIP << 2), 0);
	if (errno)
		return false;
#else
#error GetThreadBacktraceContext: which arch?
#endif

	return true;
}

#elif defined(__unix__)
#include <pthread.h>
#include <signal.h>

RString
ThreadsVersion()
{
	return "(unknown)";
}

uint64_t
GetCurrentThreadId()
{
	return uint64_t(pthread_self());
}

int
SuspendThread(uint64_t id)
{
	return pthread_kill(pthread_t(id), SIGSTOP);
}

int
ResumeThread(uint64_t id)
{
	return pthread_kill(pthread_t(id), SIGCONT);
}
#endif
