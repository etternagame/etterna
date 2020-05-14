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
        ss << fmt::format("[{:%F %T}]", time); // Time
        ss << fmt::format("[{:<5}]", plog::severityToString(record.getSeverity())); // Severity
        ss << fmt::format(": {}\n", record.getMessage()); // Message

        return ss.str();
    }
};

PlogLogger::PlogLogger() {
    static plog::ColorConsoleAppender<EtternaFormatter> consoleAppender;
    plog::init(plog::Severity::info, &consoleAppender);
}

void PlogLogger::log(Core::ILogger::Severity logLevel, const std::string_view message) {
    PLOG(PlogLogger::convertSeverity(logLevel)) << message;
}

plog::Severity PlogLogger::convertSeverity(ILogger::Severity logLevel) {
    if(logLevel == Severity::TRACE) return plog::Severity::verbose;
    if(logLevel == Severity::DEBUG) return plog::Severity::debug;
    if(logLevel == Severity::INFO) return plog::Severity::info;
    if(logLevel == Severity::WARN) return plog::Severity::warning;
    if(logLevel == Severity::ERROR) return plog::Severity::error;
    if(logLevel == Severity::FATAL) return plog::Severity::fatal;
}