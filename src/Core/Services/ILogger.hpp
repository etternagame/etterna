#ifndef CORE_SERVICES_ILOGGER_HPP
#define CORE_SERVICES_ILOGGER_HPP

#include <string>

namespace Core {

/**
 * A logger class to be implemented around a separate logging library.
 * ILogger is what will be used throughout the game files.
 */
class ILogger {
protected:
    /** Log directory relative to the executable file. */
    static constexpr const char *const LOG_DIRECTORY = "./Logs";

public:
    virtual void trace(std::string message) = 0;
    virtual void debug(std::string message) = 0;
    virtual void info(std::string message) = 0;
    virtual void warn(std::string message) = 0;
    virtual void error(std::string message) = 0;
    virtual void fatal(std::string message) = 0;
};

}

#endif    // CORE_SERVICES_ILOGGER_HPP
