#ifndef CORE_MISC_PLOGLOGGER_HPP
#define CORE_MISC_PLOGLOGGER_HPP

#include "Core/Services/ILogger.hpp"
#include <plog/Log.h>

class PlogLogger : public Core::ILogger {
public:
    PlogLogger();
    void setLogLevel(ILogger::Severity logLevel) override;
protected:
    void log(ILogger::Severity logLevel, const std::string_view message) override;
private:
    static plog::Severity convertSeverity(Core::ILogger::Severity logLevel);
};

#endif //CORE_MISC_PLOGLOGGER_HPP
