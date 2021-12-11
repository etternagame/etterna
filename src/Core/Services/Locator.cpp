#include "Locator.hpp"

// Static variable default values
std::unique_ptr<Core::ILogger> Locator::logger = nullptr;


Core::ILogger *Locator::getLogger() {
    return logger.get();
}

void Locator::provide(std::unique_ptr<Core::ILogger> log) {
    if(!Locator::logger)
        Locator::logger = std::move(log);
}
