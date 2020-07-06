#ifndef CORE_SERVICES_SERVICELOCATOR_HPP
#define CORE_SERVICES_SERVICELOCATOR_HPP

#include "arch/ArchHooks/ArchHooks.h"
#include "Core/Services/ILogger.hpp"

#include <memory>

/**
 * This class is a Service Locator to be used to isolate the globals this program has.
 * Locator has flaws of it's own, thought could help isolate the number of singletons
 * etterna currently has.
 */
class Locator {
public:
    // Getters
    static ArchHooks* getArchHooks();
    static Core::ILogger* getLogger();

    // Providers
    static void provide(ArchHooks* hooks);
    static void provide(Core::ILogger* logger);

private:
    static std::unique_ptr<ArchHooks> archHooks;
    static std::unique_ptr<Core::ILogger> logger;
};
#endif //CORE_SERVICES_SERVICELOCATOR_HPP
