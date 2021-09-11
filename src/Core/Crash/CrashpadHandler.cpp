#include "CrashpadHandler.hpp"

#include "Core/Platform/Platform.hpp"
#include "Core/Services/Locator.hpp"
#include "Core/Misc/AppInfo.hpp"
#include "arch/Dialog/Dialog.h"

#include <ghc/filesystem.hpp>
#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/simulate_crash.h"
#include "client/settings.h"
#include <string>
#include <tuple>

std::tuple<base::FilePath, base::FilePath, bool> getCrashpadInfo(){
    auto exeDir = Core::Platform::getExecutableDirectory();
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
	const bool startHandlerFromBGThread = true;
#else
    base::FilePath handler(handlerPath);
    base::FilePath dataDir(dataPath);
	const bool startHandlerFromBGThread = false; // not available, assert crash if true
#endif

    return {handler, dataDir, startHandlerFromBGThread};
}

/**
 * Initialize the crashpad_handler watchdog, and configure the settings database.
 * The preprocessor defines are only needed since crashpad's StartHandler
 * function uses different parameter types for windows vs non-windows.
 *
 * @return True if successfully initialized, False if otherwise.
 */
bool Core::Crash::initCrashpad() {
    auto[handler, dataDir, startHandlerFromBGThread] = getCrashpadInfo();

    auto database = crashpad::CrashReportDatabase::Initialize(dataDir);
    if (database == nullptr) return false;

    auto *client = new crashpad::CrashpadClient();
    bool status = client->StartHandler(
        handler, // relative path to executable handler file
        dataDir, // the crashpad database
        dataDir, // an existing directory for metrics data

        // an upload server. A url is required, even if nothing is uploaded.
        "https://crash.etterna.dev/api/minidump/upload?api_key=0b03527eb91c4d4f8d7576cf4e4939c4",
        {}, // crash report metadata
        {"--no-upload-gzip"}, // any additional handler arguments
        true, // auto-restart if handler dies
        startHandlerFromBGThread // start handler from background thread (windows only)
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

/**
 * Allows for enabling or disabling of crashpad after initialization.
 * @param shouldUpload True if game should upload minidump on crash, false otherwise.
 */
void Core::Crash::setShouldUpload(bool shouldUpload) {
    auto[handler, dataDir, BGthread] = getCrashpadInfo();

    auto database = crashpad::CrashReportDatabase::InitializeWithoutCreating(dataDir);
    if (database != nullptr && database->GetSettings() != nullptr) {
        database->GetSettings()->SetUploadsEnabled(shouldUpload);
        Locator::getLogger()->info("Crash dumps {} be uploaded if the game crashes.", shouldUpload ? "WILL" : "WILL NOT");
    } else {
        Locator::getLogger()->info("Unable to enable/disable minidump upload.");
    }
}