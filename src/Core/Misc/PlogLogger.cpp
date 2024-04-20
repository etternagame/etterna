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
    // Console Appender. One for windows, and another for other operating systems.
    #ifdef _WIN32
        static plog::WindowsAppender<EtternaFormatter> consoleAppender;
    #else
        static plog::ColorConsoleAppender<EtternaFormatter> consoleAppender;
    #endif
    plog::init(plog::Severity::info, &consoleAppender);


    // Get current time for log file name. Format: YYYY_MM_DD-HH_MM_SS.log
    char timeString[20]; // Date and time portion only
    std::time_t t = std::time(nullptr);
    std::strftime(timeString, sizeof(timeString), "%Y_%m_%d-%H_%M_%S", std::localtime(&t));
    auto logDirectory = Core::Platform::getAppDirectory() / "Logs";

    namespace fs = ghc::filesystem;
    // Ensure log directory exists and is writable before initializing logger.
    auto appDirPerms = fs::status(Core::Platform::getAppDirectory()).permissions();
    auto writable = (appDirPerms & (fs::perms::owner_write )) != fs::perms::none;

    // If not writable, only output to console.
    if(!writable)
        return;

    if(!fs::exists(logDirectory))
        fs::create_directory(logDirectory);

    auto logFilePath = logDirectory / fmt::format(FMT_STRING("{}.log"), timeString);

    // File Appender
    static plog::RollingFileAppender<EtternaFormatter, plog::UTF8Converter> rollingFileAppender{logFilePath.c_str()};
	currentLogFile = absolute(logFilePath);
    plog::init(plog::Severity::info, &rollingFileAppender);
}

void PlogLogger::log(Core::ILogger::Severity logLevel, const std::string_view message) {
	/* Note: If multiple PlogLogger objects exist in the same thread, this last-message log suppression
	 * will persist across calls to log between different instances. This may or may not be desired,
	 * if this hypothetical scenario ever happens, consider :)
	 */
	static thread_local std::string last_message = "";
	static thread_local Core::ILogger::Severity last_log_level;
	static thread_local unsigned num_occurrences = 0;
	
	if(message == last_message && logLevel == last_log_level){
		num_occurrences += 1;
		//Duplicate message; suppress log output
	}else{
		//This is a novel message.
		
		if(num_occurrences > 0){
			PLOG(PlogLogger::convertSeverity(last_log_level)) << last_message << " (Repeated " << num_occurrences << " times)";
		}
		last_message = message;
		last_log_level = logLevel;
		num_occurrences = 0;
		
		PLOG(PlogLogger::convertSeverity(logLevel)) << message;
	}
}

void PlogLogger::setLogLevel(Core::ILogger::Severity logLevel) {
    plog::get()->setMaxSeverity(convertSeverity(logLevel));
}

std::string PlogLogger::getLogFile(){
	return currentLogFile;
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
