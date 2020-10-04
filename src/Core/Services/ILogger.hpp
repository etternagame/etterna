#ifndef CORE_SERVICES_ILOGGER_HPP
#define CORE_SERVICES_ILOGGER_HPP

#include <string>
#include <fmt/format.h>

namespace Core {
/**
 * A logger class to be implemented around a separate logging library.
 * ILogger is what will be used throughout the game files.
 *
 * The public functions defined in the class are templates which use the
 * format library to insert the extra parameters into the final string.
 * To simplify operation for the backend, they receive the final
 * formatted string in the log parameter, at which point they can log
 * with it as is desired.
 */
class ILogger {

public:
    /**
     * Severity Enum - Middle man between each logging backed
     * and their own severity terminology.
     */
    enum class Severity {TRACE, DEBUG, INFO, WARN, ERR, FATAL};

    // Logging Specific
    template <typename... Args> void trace(const std::string_view log, const Args& ... args) {
        this->log(Severity::TRACE, fmt::format(log, args...));
    }
    template <typename... Args> void debug(const std::string_view log, const Args& ... args) {
        this->log(Severity::DEBUG, fmt::format(log, args...));
    }
    template <typename... Args> void info(const std::string_view log, const Args& ... args) {
        this->log(Severity::INFO, fmt::format(log, args...));
    }
    template <typename... Args> void warn(const std::string_view log, const Args& ... args) {
        this->log(Severity::WARN, fmt::format(log, args...));
    }
    template <typename... Args> void error(const std::string_view log, const Args& ... args){
        this->log(Severity::ERR, fmt::format(log, args...));
    }
    template <typename... Args> void fatal(const std::string_view log, const Args& ... args) {
        this->log(Severity::FATAL, fmt::format(log, args...));
    }

    /** @brief Enabled or disable a stdout prompt on Windows */
    bool setConsoleEnabled(bool enable);
    virtual void setLogLevel(ILogger::Severity logLevel) = 0;
	virtual ~ILogger() = default; 
protected:
    /**
     * Implementations only need to implement this log function to send logs
     * to file, console, network, etc. This function is kept protected as
     * for the majority of logs, the six severity functions should be sufficient
     * for most logging purposes.
     * @param logLevel Log Severity
     * @param message Formatted string to log
     */
    virtual void log(ILogger::Severity logLevel, const std::string_view message) = 0;
};
}

#endif    // CORE_SERVICES_ILOGGER_HPP
