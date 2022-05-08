#ifndef CORE_CRASH_CRASHPADHANDLER_HPP
#define CORE_CRASH_CRASHPADHANDLER_HPP

namespace Core::Crash {
    bool initCrashpad();
    void generateMinidump();
    void setShouldUpload(bool shouldUpload);
}

#endif //CORE_CRASH_CRASHPADHANDLER_HPP
