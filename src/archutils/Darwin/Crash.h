#ifndef DARWIN_CRASH_H
#define DARWIN_CRASH_H

namespace CrashHandler {
std::string
GetLogsDirectory();
void
InformUserOfCrash(const std::string& sPath);
bool
IsDebuggerPresent();
void
DebugBreak();
}

#endif /* DARWIN_CRASH_H */
