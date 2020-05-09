#include "Locator.hpp"

// Static variable default values
std::unique_ptr<IFileManager> Locator::fileManager = nullptr;
std::unique_ptr<ArchHooks> Locator::archHooks = nullptr;
std::unique_ptr<Core::ILogger> Locator::logger = nullptr;

IFileManager *Locator::getFileManager() {
    return fileManager.get();
}

ArchHooks *Locator::getArchHooks() {
    return archHooks.get();
}

Core::ILogger *Locator::getLogger() {
    return logger.get();
}

void Locator::provide(IFileManager *manager) {
    if(!Locator::fileManager)
        Locator::fileManager = std::unique_ptr<IFileManager>(manager);
}

void Locator::provide(ArchHooks *hooks) {
    if(!Locator::archHooks)
        Locator::archHooks = std::unique_ptr<ArchHooks>(hooks);
}

void Locator::provide(Core::ILogger *logger) {
    if(!Locator::logger)
        Locator::logger = std::unique_ptr<Core::ILogger>(logger);
}
