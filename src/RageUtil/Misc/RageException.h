#ifndef RAGE_EXCEPTION_H
#define RAGE_EXCEPTION_H

#include "config.hpp"
#include <string>

/**
 * @brief Namespace for throwing fatal errors.
 *
 * The original documentation stated this was a class for some reason. */
namespace RageException {
void NORETURN
Throw(const char* fmt, ...) PRINTF(1, 2);
void
SetCleanupHandler(void (*pHandler)(const std::string& sError));
}

#endif
