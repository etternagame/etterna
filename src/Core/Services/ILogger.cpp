#include "ILogger.hpp"
#include <Core/Services/Locator.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * Allocate and forward standard output to a command prompt on windows.
 *
 * This function only takes action on Windows. Unix based operating systems always
 * have a console associated with them, even if the console is not visible, and to get the console output,
 * the program usually only needs to be run from the command line. On linux it's the same as running
 * any program that is not in the PATH variable, and on MacOS, it's accessing the executable binary
 * that is within the .app package.
 *
 * Note: This function should only be executed after a logger instance has been initialized.
 *
 * @return True if operation sucessfully completed enabled. False if otherwise.
 */
bool Core::ILogger::setConsoleEnabled(bool enable) {
#ifdef _WIN32
    // Disable the console
    if (!enable){
        FreeConsole();
        return true;
    }

    // If we reach this point in the code, attempt to enable the console.
    // Return value of zero means failure to allocate console
    if(AllocConsole() == 0){
        Locator::getLogger()->error("Console window failed to initialize.");
        return false;
    }

    // Usually freopen_s would be used to reassign a file pointer to a new or different file.
    // Since out "file" is standard out, and we only want to redirect it, we can give
    // a dummy value for the file pointer. It can't be null as it is required for the
    // operation to occur, but we don't need to hold onto it afterwards.
    // The following functions should return zero after executing sucessfully. We don't check it here.
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
#endif
    return true;
}
