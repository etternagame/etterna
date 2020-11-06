#include "CrashpadHandler.hpp"

#include "Core/Arch/Arch.hpp"
#include "Core/Misc/AppInfo.hpp"

#include <ghc/filesystem.hpp>
#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/simulate_crash.h"
#include "client/settings.h"
#include <string>

/**
 * Initialize the crashpad_handler watchdog, and configure the settings database.
 * The preprocessor defines are only needed since crashpad's StartHandler
 * function uses different parameter types for windows vs non-windows.
 *
 * @return True if successfully initialized, False if otherwise.
 */
bool Core::Crash::initCrashpad() {
    auto exeDir = Core::Arch::getExecutableDirectory();
    auto dataPath = exeDir / "CrashData";

    /* Core::AppInfo::CRASHPAD_HANDLER_EXE may either be a full path, or only an executable name.
     * - If it is an absolute path, it is assumed to be running on a development machine, and will pass
     * in that path for the handler location.
     *
     * - If it is not an absolute path, it is assumed to be the executable name, for the exe file
     * place in the same directory as this binary.
     */
    auto handlerPath = ghc::filesystem::path{Core::AppInfo::CRASHPAD_HANDLER_EXE};
    if (!handlerPath.is_absolute())
        handlerPath = exeDir / handlerPath;

#ifdef _WIN32
    base::FilePath handler(handlerPath.generic_wstring());
    base::FilePath dataDir(dataPath.generic_wstring());
#else
    base::FilePath handler(handlerPath);
    base::FilePath dataDir(dataPath);
#endif

    auto database = crashpad::CrashReportDatabase::Initialize(dataDir);
    if (database == nullptr) return false;

    auto *client = new crashpad::CrashpadClient();
    bool status = client->StartHandler(
        handler, // relative path to executable handler file
        dataDir, // the crashpad database
        dataDir, // an existing directory for metrics data
        "", // an upload server. A url is required, even if nothing is uploaded.
        {}, // crash report metadata
        {}, // any additional handler arguments
        true, // auto-restart if handler dies
        true // start handler from background thread (currently windows only)
    );
    return status;
}

/**
 * Force a minidump file to be generated of the stack at the location where this function is called.
 * It does not actually crash the program, and only generates the minidump.
 */
void Core::Crash::generateMinidump() {
    CRASHPAD_SIMULATE_CRASH();
}