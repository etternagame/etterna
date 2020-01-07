#include "Etterna/Globals/global.h"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <cerrno>
#include <limits.h>
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
#include <csignal>

#include "RageUtil/Misc/RageLog.h" /* for RageLog::GetAdditionalLog, etc, only */
#include "RageUtil/Misc/RageThreads.h"
#include "Backtrace.h"

#include <sys/types.h>
#include <sys/wait.h>

#include "CrashHandler.h"
#include "CrashHandlerInternal.h"

extern uint64_t
GetInvalidThreadId();
extern const char* g_pCrashHandlerArgv0;

static void
safe_print(int fd, ...)
{
	va_list ap;
	va_start(ap, fd);

	for (;;) {
		const char* p = va_arg(ap, const char*);
		if (p == NULL) {
			break;
		}
		size_t len = strlen(p);
		while (len) {
			ssize_t result = write(fd, p, strlen(p));
			if (result == -1) {
				_exit(1);
			}
			len -= result;
			p += result;
		}
	}
	va_end(ap);
}

#ifdef __linux__
static void
GetExecutableName(char* buf, int bufsize)
{
	/* Reading /proc/self/exe always gives the running binary, even if it no
	 * longer exists. */
	strncpy(buf, "/proc/self/exe", bufsize);
	buf[bufsize - 1] = 0;
}
#else
static void
GetExecutableName(char* buf, int bufsize)
{
	strncpy(buf, g_pCrashHandlerArgv0, bufsize);
	buf[bufsize - 1] = 0;
}
#endif

static void NORETURN
spawn_child_process(int from_parent)
{
	/* We need to re-exec ourself, to get a clean process.  Close all
	 * FDs except for 0-2 and to_child, and then assign to_child to 3. */
	for (int fd = 3; fd < 1024; ++fd)
		if (fd != from_parent)
			close(fd);

	if (from_parent != 3) {
		dup2(from_parent, 3);
		close(from_parent);
	}

	char path[1024];
	char magic[32];
	GetExecutableName(path, sizeof(path));
	strncpy(magic, CHILD_MAGIC_PARAMETER, sizeof(magic));

	/* Use execve; it's the lowest-level of the exec calls.  The others may
	 * allocate. */
	char* argv[3] = { path, magic, NULL };
	char* envp[1] = { NULL };
	execve(path, argv, envp);

	/* If we got here, the exec failed.  We can't call strerror. */
	// safe_print(fileno(stderr), "Crash handler execl(", path, ") failed: ",
	// strerror(errno), "\n", NULL);
	safe_print(fileno(stderr),
			   "Crash handler execl(",
			   path,
			   ") failed: ",
			   itoa(errno),
			   "\n",
			   NULL);
	_exit(1);
}

/* write(), but retry a couple times on EINTR. */
static int
retried_write(int fd, const void* buf, size_t count)
{
	int tries = 3, ret;
	do {
		ret = write(fd, buf, count);
	} while (ret == -1 && errno == EINTR && tries--);

	return ret;
}

static bool
parent_write(int to_child, const void* p, size_t size)
{
	int ret = retried_write(to_child, p, size);
	if (ret == -1) {
		safe_print(fileno(stderr),
				   "Unexpected write() result (",
				   strerror(errno),
				   ")\n",
				   NULL);
		return false;
	}

	if (size_t(ret) != size) {
		safe_print(fileno(stderr),
				   "Unexpected write() result (",
				   itoa(ret),
				   ")\n",
				   NULL);
		return false;
	}

	return true;
}

static void
parent_process(int to_child, const CrashData* crash)
{
	/* 1. Write the CrashData. */
	if (!parent_write(to_child, crash, sizeof(CrashData)))
		return;

	/* 2. Write info. */
	const char* p = RageLog::GetInfo();
	int size = strlen(p) + 1;
	if (!parent_write(to_child, &size, sizeof(size)))
		return;
	if (!parent_write(to_child, p, size))
		return;

	/* 3. Write AdditionalLog. */
	p = RageLog::GetAdditionalLog();
	size = strlen(p) + 1;
	if (!parent_write(to_child, &size, sizeof(size)))
		return;
	if (!parent_write(to_child, p, size))
		return;

	/* 4. Write RecentLogs. */
	int cnt = 0;
	const char* ps[1024];
	while (cnt < 1024 && (ps[cnt] = RageLog::GetRecentLog(cnt)) != NULL)
		++cnt;

	parent_write(to_child, &cnt, sizeof(cnt));
	for (int i = 0; i < cnt; ++i) {
		size = strlen(ps[i]) + 1;
		if (!parent_write(to_child, &size, sizeof(size)))
			return;
		if (!parent_write(to_child, ps[i], size))
			return;
	}

	/* 5. Write CHECKPOINTs. */
	static char buf[1024 * 32];
	Checkpoints::GetLogs(buf, sizeof(buf), "\n");
	size = strlen(buf) + 1;
	if (!parent_write(to_child, &size, sizeof(size)))
		return;
	if (!parent_write(to_child, buf, size))
		return;

	/* 6. Write the crashed thread's name. */
	p = RageThread::GetCurrentThreadName();
	size = strlen(p) + 1;
	if (!parent_write(to_child, &size, sizeof(size)))
		return;
	if (!parent_write(to_child, p, size))
		return;
}

/* The parent process is the crashed process.  It'll send data to the
 * child, who will do stuff with it.  The parent then waits for the
 * child to quit, and exits.
 *
 * We can do whatever fancy things we want in the child process.  However,
 * let's not open any windows until we at least try to shut down OpenGL,
 * since it may cause problems.  We don't want to try to shut down OpenGL
 * until we've sent all of our data, since it might explode.
 *
 * So, first fork off the error reporting child, send data to it, shut down
 * OpenGL, close the socket and wait for the child to shut down.
 *
 * The child reads the data from the parent, waits for the socket to close
 * (EOF), and it's then free to open windows and stuff.
 *
 * XXX: make sure the parent dying doesn't take out the child
 */

/* The x86 backtrace() in glibc doesn't make any effort at all to decode
 * signal trampolines.  The result is that it doesn't properly show the
 * function that actually caused the signal--which is the most important
 * one!  So, we have to do it all ourself. */

static void
RunCrashHandler(const CrashData* crash)
{
	if (g_pCrashHandlerArgv0 == NULL) {
		safe_print(
		  fileno(stderr),
		  "Crash handler failed: CrashHandlerHandleArgs was not called\n",
		  NULL);
		_exit(1);
	}

	/* Block SIGPIPE, so we get EPIPE. */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL) != 0) {
		safe_print(
		  fileno(stderr), "sigaction() failed: %s", strerror(errno), NULL);
		/* non-fatal */
	}

	static int active = 0;

	if (active) {
		/* We've received a second signal.  This may mean that another thread
		 * crashed before we stopped it, or it may mean that the crash handler
		 * crashed. */
		switch (crash->type) {
			case CrashData::SIGNAL:
				safe_print(fileno(stderr),
						   "Fatal signal (",
						   SignalName(crash->signal),
						   ")",
						   NULL);
				break;

			case CrashData::FORCE_CRASH:
				safe_print(fileno(stderr),
						   "Crash handler failed: \"",
						   crash->reason,
						   "\"",
						   NULL);
				break;

			default:
				safe_print(fileno(stderr),
						   "Unexpected RunCrashHandler call (",
						   itoa(crash->type),
						   ")",
						   NULL);
				break;
		}

		if (active == 1)
			safe_print(
			  fileno(stderr), " while still in the crash handler\n", NULL);
		else if (active == 2)
			safe_print(
			  fileno(stderr), " while in the crash handler child\n", NULL);

		_exit(1);
	}
	active = 1;

	/* Stop other threads.  XXX: This prints a spurious ptrace error if any
	 * threads are already suspended, which happens in
	 * ForceCrashHandlerDeadlock(). */
	RageThread::HaltAllThreads();

	/* We need to be very careful, since we're under crash conditions.  Let's
	 * fork a process and exec ourself to get a clean environment to work in. */
	int fds[2];
	if (pipe(fds) != 0) {
		safe_print(fileno(stderr),
				   "Crash handler pipe() failed: ",
				   strerror(errno),
				   "\n",
				   NULL);
		exit(1);
	}

	pid_t childpid = fork();
	if (childpid == -1) {
		safe_print(fileno(stderr),
				   "Crash handler fork() failed: ",
				   strerror(errno),
				   "\n",
				   NULL);
		_exit(1);
	}

	if (childpid == 0) {
		active = 2;
		close(fds[1]);
		spawn_child_process(fds[0]);
	} else {
		close(fds[0]);
		parent_process(fds[1], crash);
		close(fds[1]);

		int status = 0;
#if !defined(__APPLE__)
		waitpid(childpid, &status, 0);
#endif

		/* We need to resume threads before continuing, or we may deadlock on
		 * _exit(). */
		/* XXX: race condition:  If two threads are deadlocked on a pair of
		 * mutexes, there's a chance that they'll both try to lock the other's
		 * mutex at the same time.  If that happens, then they'll both time out
		 * the lock at about the same time.  One will trigger the deadlock crash
		 * first, and the other will be suspended.  However, once we resume it
		 * here, it'll continue, and * trigger the deadlock crash again. It can
		 * result in unrecoverable deadlocks. */
		RageThread::ResumeAllThreads();

		if (WIFSIGNALED(status))
			safe_print(fileno(stderr),
					   "Crash handler child exited with signal ",
					   itoa(WTERMSIG(status)),
					   "\n",
					   NULL);
	}
}

static void
BacktraceAllThreads(CrashData& crash)
{
	int iCnt = 1;
	uint64_t iID;

	for (int i = 0; RageThread::EnumThreadIDs(i, iID); ++i) {
		if (iID == GetInvalidThreadId() ||
			iID == RageThread::GetCurrentThreadID())
			continue;

		BacktraceContext ctx;
		if (GetThreadBacktraceContext(iID, &ctx))
			GetBacktrace(
			  crash.BacktracePointers[iCnt], BACKTRACE_MAX_SIZE, &ctx);
		strncpy(crash.m_ThreadName[iCnt],
				RageThread::GetThreadNameByID(iID),
				sizeof(crash.m_ThreadName[0]) - 1);

		++iCnt;

		if (iCnt == CrashData::MAX_BACKTRACE_THREADS)
			break;
	}
}

void
CrashHandler::ForceCrash(const char* reason)
{
	CrashData crash;
	memset(&crash, 0, sizeof(crash));

	crash.type = CrashData::FORCE_CRASH;
	strncpy(crash.reason, reason, sizeof(crash.reason));
	crash.reason[sizeof(crash.reason) - 1] = 0;

	GetBacktrace(crash.BacktracePointers[0], BACKTRACE_MAX_SIZE, NULL);

	RunCrashHandler(&crash);
}

void
CrashHandler::ForceDeadlock(RString reason, uint64_t iID)
{
	CrashData crash;
	memset(&crash, 0, sizeof(crash));

	crash.type = CrashData::FORCE_CRASH;

	GetBacktrace(crash.BacktracePointers[0], BACKTRACE_MAX_SIZE, NULL);

	if (iID == GetInvalidThreadId()) {
		/* Backtrace all threads. */
		BacktraceAllThreads(crash);
	} else {
		BacktraceContext ctx;
		if (!GetThreadBacktraceContext(iID, &ctx))
			reason += "; GetThreadBacktraceContext failed";
		else
			GetBacktrace(crash.BacktracePointers[1], BACKTRACE_MAX_SIZE, &ctx);
		strncpy(crash.m_ThreadName[1],
				RageThread::GetThreadNameByID(iID),
				sizeof(crash.m_ThreadName[0]) - 1);
	}

	strncpy(crash.m_ThreadName[0],
			RageThread::GetCurrentThreadName(),
			sizeof(crash.m_ThreadName[0]) - 1);

	strncpy(
	  crash.reason, reason, std::min(sizeof(crash.reason) - 1, reason.length()));
	crash.reason[sizeof(crash.reason) - 1] = 0;

	RunCrashHandler(&crash);

	_exit(1);
}

void
CrashHandler::CrashSignalHandler(int signal,
								 siginfo_t* si,
								 const ucontext_t* uc)
{
	static volatile bool bInCrashSignalHandler = false;
	if (bInCrashSignalHandler) {
		safe_print(
		  2, "Fatal: crash from within the crash signal handler\n", NULL);
		_exit(1);
	}

	bInCrashSignalHandler = true;

	/* Work around a gcc bug: it reorders the above assignment past other work
	 * down here, even if we declare it volatile.  This makes us not catch
	 * recursive crashes. */
	asm volatile("nop");

	CrashData crash;
	memset(&crash, 0, sizeof(crash));

	crash.type = CrashData::SIGNAL;
	crash.signal = signal;
	crash.si = *si;

	BacktraceContext ctx;
	GetSignalBacktraceContext(&ctx, uc);
	GetBacktrace(crash.BacktracePointers[0], BACKTRACE_MAX_SIZE, &ctx);
#if defined(HAVE_DECL_SIGUSR1) && HAVE_DECL_SIGUSR1
	if (signal == SIGUSR1)
		BacktraceAllThreads(crash);
#endif

	strncpy(crash.m_ThreadName[0],
			RageThread::GetCurrentThreadName(),
			sizeof(crash.m_ThreadName[0]) - 1);

	bInCrashSignalHandler = false;

	/* RunCrashHandler handles any recursive crashes of its own by itself. */
	RunCrashHandler(&crash);
}

void
CrashHandler::InitializeCrashHandler()
{
	InitializeBacktrace();
}
