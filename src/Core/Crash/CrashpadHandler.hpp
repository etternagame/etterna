#ifndef CORE_CRASH_CRASHPADHANDLER_HPP
#define CORE_CRASH_CRASHPADHANDLER_HPP

namespace Core::Crash {
    bool initCrashpad();
    void generateMinidump();
}

#endif //CORE_CRASH_CRASHPADHANDLER_HPP
