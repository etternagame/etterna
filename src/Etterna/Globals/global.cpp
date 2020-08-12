#include "global.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#ifdef _WIN32
#define _WIN32_WINDOWS 0x0410 // include Win98 stuff
#include "windows.h"
#include "archutils/Win32/Crash.h"
#elif defined(__APPLE__)
#include "archutils/Darwin/Crash.h"
#include <stdlib.h>
using CrashHandler::DebugBreak;
using CrashHandler::IsDebuggerPresent;
#endif

void NORETURN
sm_crash(const char* reason)
{
#if defined(_WIN32) || defined(__APPLE__) || defined(_XDBG)
	/* If we're being debugged, throw a debug break so it'll suspend the
	 * process. */
	if (IsDebuggerPresent()) {
		DebugBreak();
		for (;;)
			; /* don't return */
	}
#endif

//	CrashHandler::ForceCrash(reason);

#ifdef _WIN32
	/* Do something after the above, so the call/return isn't optimized to a
	 * jmp; that way, this function will appear in backtrace stack traces. */
#if defined(_MSC_VER)
	//__nop();
#elif defined(__GNUC__) // MinGW or similar
	asm("nop");
#endif
#else
	_exit(1);
#endif
}
