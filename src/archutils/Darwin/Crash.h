#ifndef DARWIN_CRASH_H
#define DARWIN_CRASH_H

namespace CrashHandler {
RString
GetLogsDirectory();
void
InformUserOfCrash(const RString& sPath);
bool
IsDebuggerPresent();
void
DebugBreak();
}

#endif /* DARWIN_CRASH_H */
