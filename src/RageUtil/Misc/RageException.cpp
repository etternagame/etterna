#include "Etterna/Globals/global.h"
#include "RageException.h"
#include "Core/Services/Locator.hpp"
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
static void (*g_CleanupHandler)(const std::string& sError) = NULL;
void
RageException::SetCleanupHandler(void (*pHandler)(const std::string& sError))
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
	std::string error = vssprintf(sFmt, va);
	va_end(va);

	std::string msg =
	  ssprintf("\n"
			   "//////////////////////////////////////////////////////\n"
			   "Exception: %s\n"
			   "//////////////////////////////////////////////////////\n",
			   error.c_str());
    Locator::getLogger()->trace(msg.c_str());

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
