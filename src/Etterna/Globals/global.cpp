#include "global.h"

#include "Core/Crash/CrashpadHandler.hpp"
#include "Core/Services/Locator.hpp"
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#ifdef _WIN32
#define _WIN32_WINDOWS 0x0410 // include Win98 stuff
#include "windows.h"
#include <fmt/format.h>
#elif defined(__APPLE__)
#include "archutils/Darwin/Crash.h"
#include <stdlib.h>
using CrashHandler::DebugBreak;
using CrashHandler::IsDebuggerPresent;
#endif

#ifdef _WIN32
void showCrashDialog(const char* message){
    std::string error_message =
            "Etterna has crashed.\n\n"
            "A crash report was created in the \"CrashData\" folder.\n\n"
            "Please send that file to the developers, and they can find out what happened!\n\n"
            "Crash Reason: {}";

    MessageBox(nullptr, fmt::format(error_message, message).c_str(), "Crash Message", MB_OK | MB_ICONERROR | MB_TASKMODAL);
}

#endif

void NORETURN sm_crash(const char* reason) {
#if defined(_WIN32) || defined(__APPLE__) || defined(_XDBG)
	/* If we're being debugged, throw a debug break so it'll suspend the
	 * process. */
	if (IsDebuggerPresent()) {
		DebugBreak();
		for (;;)
			; /* don't return */
	}
#endif
#ifdef __APPLE__
    CrashHandler::InformUserOfCrash(reason);
#endif

#ifdef _WIN32
    showCrashDialog(reason);
#endif
	Locator::getLogger()->fatal(reason);
    Core::Crash::generateMinidump();

    _exit(1);
}
