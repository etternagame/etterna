#include "PlogLogger.hpp"

#include "Core/Platform/Platform.hpp"

#include <plog/Appenders/ColorConsoleAppender.h>
#include <fmt/chrono.h>

#ifdef _WIN32
#include <cstdio>
namespace plog {

    /**
     * A Windows Specific Appender
     *
     * Windows is very particular about how a message is printed out. The built in appenders
     * use a function that do not function well with stdout/stderr redirection as we do in ILogger.cpp
     * This appender is specifically for windows as it will work with redirection.
     */
    template<class Formatter>
    class WindowsAppender : public IAppender {
    public:
        virtual void write(const Record& record){
            util::nstring str = Formatter::format(record);  // Get the formatted print string
            fputws(str.c_str(), stdout);  // Print to standard out
        }
    };
}
#endif

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
    auto logDirectory = Core::Platform::getAppDirectory() / "Logs";

    // Ensure log directory exists before initializing logger.
    if(!ghc::filesystem::exists(logDirectory))
        ghc::filesystem::create_directory(logDirectory);

    auto logFilePath = logDirectory / fmt::format(FMT_STRING("{}.log"), timeString);

    // File Appender
    static plog::RollingFileAppender<EtternaFormatter, plog::UTF8Converter> rollingFileAppender{logFilePath.c_str()};
    plog::init(plog::Severity::info, &rollingFileAppender);

    // Console Appender. One for windows, and another for other operating systems.
    #ifdef _WIN32
        static plog::WindowsAppender<EtternaFormatter> consoleAppender;
    #else
        static plog::ColorConsoleAppender<EtternaFormatter> consoleAppender;
    #endif
    plog::init(plog::Severity::info, &consoleAppender);
}

void PlogLogger::log(Core::ILogger::Severity logLevel, const std::string_view message) {
    PLOG(PlogLogger::convertSeverity(logLevel)) << message;
}

void PlogLogger::setLogLevel(Core::ILogger::Severity logLevel) {
    plog::get()->setMaxSeverity(convertSeverity(logLevel));
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
