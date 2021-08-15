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
    /** Virtual destructor to ensure derived objects are guaranteed to have destructor */
    virtual ~ILogger() = default;

    /**
     * Severity Enum - Middle man between each logging backed
     * and their own severity terminology.
     */
    enum class Severity {TRACE, DEBUG, INFO, WARN, ERR, FATAL};

    // Logging Specific
    template <typename... Args> void trace(const std::string_view log, const Args& ... args) {
		this->log(Severity::TRACE, safe_format(log, args...));
    }
    template <typename... Args> void debug(const std::string_view log, const Args& ... args) {
        this->log(Severity::DEBUG, safe_format(log, args...));
    }
    template <typename... Args> void info(const std::string_view log, const Args& ... args) {
        this->log(Severity::INFO, safe_format(log, args...));
    }
    template <typename... Args> void warn(const std::string_view log, const Args& ... args) {
        this->log(Severity::WARN, safe_format(log, args...));
    }
    template <typename... Args> void error(const std::string_view log, const Args& ... args) {
        this->log(Severity::ERR, safe_format(log, args...));
    }
    template <typename... Args> void fatal(const std::string_view log, const Args& ... args) {
        this->log(Severity::FATAL, safe_format(log, args...));
    }

    virtual void setLogLevel(ILogger::Severity logLevel) = 0;

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

private:
	template <typename... Args> inline std::string safe_format(const std::string_view log, const Args& ... args) {
		try {
			return fmt::format(log, args...);
		} catch (fmt::v7::format_error& e) {
			std::string msg("There was an error formatting the next log "
							"message - Report to developers: ");
			msg.append(e.what());
			this->log(Severity::ERR, msg);
			return std::string(log);
		}
	}
};
}

#endif    // CORE_SERVICES_ILOGGER_HPP
