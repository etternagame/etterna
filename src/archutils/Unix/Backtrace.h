#ifndef BACKTRACE_H
#define BACKTRACE_H

/* This API works like backtrace_pointers(), to retrieve a stack trace. */

#if defined(CPU_PPC)
struct Frame;
#endif
/* This contains the information necessary to backtrace a thread. */
struct BacktraceContext
{
#if defined(__i386__) || defined(__x86_64__)
	const void *ip, *bp, *sp;
#endif

#ifdef __unix__
	pid_t pid;
#endif

#if defined(CPU_PPC)
	const Frame* FramePtr;
	const void* PC;
#endif
};

/**
 * @brief Initialize the backtrace.
 *
 * This is optional.  If not called explicitly, it will be called as necessary.
 * This may do things that are not safe to do in crash conditions. */
void
InitializeBacktrace();

/* Retrieve up to size-1 backtrace pointers in buf.  The array will be
 * null-terminated.  If ctx is NULL, retrieve the current backtrace; otherwise
 * retrieve a backtrace for the given context.  (Not all backtracers may
 * support contexts.) */
void
GetBacktrace(const void** buf, size_t size, const BacktraceContext* ctx = NULL);

/* Set up a BacktraceContext to get a backtrace for a thread.  ThreadID may
 * not be the current thread.  True is returned on success, false on failure. */
bool
GetThreadBacktraceContext(uint64_t ThreadID, BacktraceContext* ctx);

/* Set up a BacktraceContext to get a backtrace after receiving a signal, given
 * a ucontext_t (see sigaction(2)).  (This interface is UNIX-specific.) */
#if defined(__unix__) || defined(__APPLE__)
#include <ucontext.h>
void
GetSignalBacktraceContext(BacktraceContext* ctx, const ucontext_t* uc);
#endif

#define BACKTRACE_METHOD_NOT_AVAILABLE ((void*)-1)

#endif
