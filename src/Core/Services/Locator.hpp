#ifndef CORE_SERVICES_SERVICELOCATOR_HPP
#define CORE_SERVICES_SERVICELOCATOR_HPP

#include "Core/Services/IFileManager.hpp"

/**
 * This class is a Service Locator to be used to isolate the globals this program has.
 * Locator has flaws of it's own, thought could help isolate the number of singletons
 * etterna currently has.
 */
class Locator {
public:
    // Getters
    static IFileManager& getFileManager();

    // Providers
    static void provide(IFileManager* manager);

private:
    static IFileManager* fileManager;
};
#endif //CORE_SERVICES_SERVICELOCATOR_HPP
