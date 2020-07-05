#ifndef PID_THREAD_HELPERS_H
#define PID_THREAD_HELPERS_H

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif
#include "Etterna/Globals/global.h"

std::string
ThreadsVersion();

/* Get the current thread's ThreadID. */
uint64_t
GetCurrentThreadId();

/* Return true if NPTL libraries are in use, false if linuxthreads. */
bool
UsingNPTL();

int
SuspendThread(uint64_t ThreadID);
int
ResumeThread(uint64_t ThreadID);

struct BacktraceContext;
int
GetThreadContext(uint64_t ThreadID, BacktraceContext* ctx);

#endif
