#include "Locator.hpp"

// Static variable default values
std::unique_ptr<ArchHooks> Locator::archHooks = nullptr;
std::unique_ptr<Core::ILogger> Locator::logger = nullptr;

ArchHooks *Locator::getArchHooks() {
    return archHooks.get();
}

Core::ILogger *Locator::getLogger() {
    return logger.get();
}

void Locator::provide(ArchHooks *hooks) {
    if(!Locator::archHooks)
        Locator::archHooks = std::unique_ptr<ArchHooks>(hooks);
}

void Locator::provide(std::unique_ptr<Core::ILogger> log) {
    if(!Locator::logger)
        Locator::logger = std::move(log);
}
