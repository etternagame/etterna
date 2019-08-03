#include "Etterna/Globals/global.h"
#include "RageException.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "RageUtil/Utils/RageUtil.h"

#include <cstdarg>

#if defined(_WIN32) && defined(DEBUG)
#include <windows.h>
#elif defined(__APPLE__)
#include "archutils/Darwin/Crash.h"
using CrashHandler::DebugBreak;
using CrashHandler::IsDebuggerPresent;
#endif

static uint64_t g_HandlerThreadID = RageThread::GetInvalidThreadID();
static void (*g_CleanupHandler)(const RString& sError) = NULL;
void
RageException::SetCleanupHandler(void (*pHandler)(const RString& sError))
{
	g_HandlerThreadID = RageThread::GetCurrentThreadID();
	g_CleanupHandler = pHandler;
}

/* This is no longer actually implemented by throwing an exception, but it acts
 * the same way to code in practice. */
void
RageException::Throw(const char* sFmt, ...)
{
	va_list va;
	va_start(va, sFmt);
	RString error = vssprintf(sFmt, va);
	va_end(va);

	RString msg =
	  ssprintf("\n"
			   "//////////////////////////////////////////////////////\n"
			   "Exception: %s\n"
			   "//////////////////////////////////////////////////////\n",
			   error.c_str());
	if (LOG != nullptr) {
		LOG->Trace("%s", msg.c_str());
		LOG->Flush();
	} else {
		puts(msg);
		fflush(stdout);
	}

#if (defined(_WIN32) && defined(DEBUG)) || defined(_XDBG) || defined(__APPLE__)
	if (IsDebuggerPresent())
		DebugBreak();
#endif

	ASSERT_M(
	  g_HandlerThreadID == RageThread::GetInvalidThreadID() ||
		g_HandlerThreadID == RageThread::GetCurrentThreadID(),
	  ssprintf("RageException::Throw() on another thread: %s", error.c_str()));
	if (g_CleanupHandler != NULL)
		g_CleanupHandler(error);

	exit(1);
}
