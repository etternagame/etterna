#include "PlogLogger.hpp"

#include <plog/Appenders/ColorConsoleAppender.h>
#include <fmt/chrono.h>

class EtternaFormatter {
public:
    // This method returns a header for a new file.
    static plog::util::nstring header() {
        return plog::util::nstring();
    }

    static plog::util::nstring format(const plog::Record &record) {
        tm time{};
        plog::util::localtime_s(&time, &record.getTime().time);

        // Log Format -> [YYYY-MM-DD HH:MM:SS][Severity]: Message
        plog::util::nostringstream ss;
        ss << fmt::format("[{:%F %T}]", time).c_str(); // Time
        ss << fmt::format("[{:<4}]", plog::severityToString(record.getSeverity())).c_str(); // Severity
		ss << ": " << record.getMessage() << "\n"; // Message
        return ss.str();
    }
};

PlogLogger::PlogLogger() {
    // Get current time for log file name. Format: YYYY_MM_DD-HH_MM_SS.log
    char timeString[20]; // Date and time portion only
    std::time_t t = std::time(nullptr);
    std::strftime(timeString, sizeof(timeString), "%Y_%m_%d-%H_%M_%S", std::localtime(&t));
    std::string logFileName = fmt::format("Logs/{}.log", timeString);

    // File Appender
    static plog::RollingFileAppender<EtternaFormatter, plog::UTF8Converter> rollingFileAppender{logFileName.c_str()};
    plog::init(plog::Severity::verbose, &rollingFileAppender);

    // Console Appender
    // Note: Windows is unable to print colors when using the /subsystem:windows compile flag
    static plog::ColorConsoleAppender<EtternaFormatter> consoleAppender;
    plog::init(plog::Severity::verbose, &consoleAppender);
}

void PlogLogger::log(Core::ILogger::Severity logLevel, const std::string_view message) {
    PLOG(PlogLogger::convertSeverity(logLevel)) << message;
}

plog::Severity PlogLogger::convertSeverity(ILogger::Severity logLevel) {
	switch (logLevel) {
		case Severity::TRACE:	return plog::Severity::verbose;
		case Severity::DEBUG:	return plog::Severity::debug;
		case Severity::INFO:	return plog::Severity::info;
		case Severity::WARN:	return plog::Severity::warning;
		case Severity::ERR:		return plog::Severity::error;
		case Severity::FATAL:	return plog::Severity::fatal;
		default:				return plog::Severity::info;
	}
}
