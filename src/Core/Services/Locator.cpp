#include "Locator.hpp"
IFileManager* Locator::fileManager = nullptr;

IFileManager& Locator::getFileManager() {
    return *fileManager;
}

void Locator::provide(IFileManager* manager) {
    if(fileManager == nullptr)
        Locator::fileManager = manager;
}