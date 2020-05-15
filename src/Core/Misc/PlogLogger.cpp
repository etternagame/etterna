#include "PlogLogger.hpp"

#include <plog/Appenders/ColorConsoleAppender.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

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
