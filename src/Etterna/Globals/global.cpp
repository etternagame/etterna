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

#if (defined(__unix__) || defined(__APPLE__))
#include "archutils/Unix/CrashHandler.h"
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

	CrashHandler::ForceCrash(reason);

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

/*
 * (c) 2004 Glenn Maynard
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
