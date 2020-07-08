#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include <csignal>
#include <ucontext.h>

namespace CrashHandler {
void
CrashHandlerHandleArgs(int argc, char* argv[]);
void
InitializeCrashHandler();
void
CrashSignalHandler(int signal, siginfo_t* si, const ucontext_t* uc);
void
ForceCrash(const char* reason);
void
ForceDeadlock(std::string reason, uint64_t CrashHandle);
}

#endif
