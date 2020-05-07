#ifndef CORE_SERVICES_SERVICELOCATOR_HPP
#define CORE_SERVICES_SERVICELOCATOR_HPP

#include "Core/Services/IFileManager.hpp"
#include "arch/ArchHooks/ArchHooks.h"

#include <memory>

/**
 * This class is a Service Locator to be used to isolate the globals this program has.
 * Locator has flaws of it's own, thought could help isolate the number of singletons
 * etterna currently has.
 */
class Locator {
public:
    // Getters
    static IFileManager* getFileManager();
    static ArchHooks* getArchHooks();

    // Providers
    static void provide(IFileManager* manager);
    static void provide(ArchHooks* hooks);

private:
    static std::unique_ptr<IFileManager> fileManager;
    static std::unique_ptr<ArchHooks> archHooks;
};
#endif //CORE_SERVICES_SERVICELOCATOR_HPP
