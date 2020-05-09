#ifndef CORE_MISC_PLOGLOGGER_HPP
#define CORE_MISC_PLOGLOGGER_HPP

#include "Core/Services/ILogger.hpp"

class PlogLogger : public Core::ILogger {
public:
    PlogLogger();

private:
    // Overridden Functions
    void trace(std::string message) override;
    void debug(std::string message) override;
    void info(std::string message) override;
    void warn(std::string message) override;
    void error(std::string message) override;
    void fatal(std::string message) override;

    // Class Specific
};

#endif //CORE_MISC_PLOGLOGGER_HPP
