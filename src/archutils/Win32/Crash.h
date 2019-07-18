#ifndef CRASH_H
#define CRASH_H
#include <windows.h>
/** @brief Win32 crash handling. */
namespace CrashHandler {
extern long __stdcall ExceptionHandler(
  struct _EXCEPTION_POINTERS* ExceptionInfo);

void
do_backtrace(const void** buf,
			 size_t size,
			 HANDLE hProcess,
			 HANDLE hThread,
			 const CONTEXT* pContext);
void
SymLookup(const void* ptr, char* buf);
void
ForceCrash(const char* reason);
void
ForceDeadlock(const RString& reason, uint64_t iID);

/* Inform the crash handler of a foreground window that may be fullscreen.
 * If set, the crash handler will attempt to hide the window or reset the
 * video mode. */
void
SetForegroundWindow(HWND hWnd);

void
CrashHandlerHandleArgs(int argc, char* argv[]);
};

#endif
