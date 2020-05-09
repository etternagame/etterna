#include "PlogLogger.hpp"

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>

#include <chrono>


class EtternaFormatter {
public:
    // This method returns a header for a new file.
    static plog::util::nstring header() {
        return plog::util::nstring();
    }

    static plog::util::nstring format(const plog::Record &record) {
        plog::util::nostringstream ss;
        tm time;
        plog::util::localtime_s(&time, &record.getTime().time);

        // Log Format -> [YYYY-MM-DD,HH:MM:SS.MIL][Severity]: Message
        // Time Section
        ss << "[" << (time.tm_year + 1900) << "-" << (time.tm_mon + 1) << "-" << time.tm_mday << ",";
        ss << time.tm_hour << ":" << time.tm_min << ":" << time.tm_sec << "." << record.getTime().millitm << "]";

        // Severity Section
        ss << "[" << std::setw(5) << plog::severityToString(record.getSeverity()) << "]";

        // Log Message
        ss << ": " << record.getMessage() << "\n";

        return ss.str();
    }
};

PlogLogger::PlogLogger() {
    static plog::ColorConsoleAppender<EtternaFormatter> consoleAppender;
    plog::init(plog::Severity::info, &consoleAppender);
}

void PlogLogger::trace(std::string message) {
    PLOG(plog::verbose) << message;
}

void PlogLogger::debug(std::string message) {
    PLOG(plog::debug) << message;
}

void PlogLogger::info(std::string message) {
    PLOG(plog::info) << message;
}

void PlogLogger::warn(std::string message) {
    PLOG(plog::warning) << message;
}

void PlogLogger::error(std::string message) {
    PLOG(plog::error) << message;
}

void PlogLogger::fatal(std::string message) {
    PLOG(plog::fatal) << message;
}
