#include "CrashpadHandler.hpp"

#ifdef _WIN32
#include "RageUtil/Utils/RageUtil.h"
#endif

#include "RageUtil/File/RageFileManager.h"
#include "Core/Misc/AppInfo.hpp"

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
#ifdef _WIN32
	std::wstring exeDir = StringToWString(RageFileManagerUtil::sDirOfExecutable);
    base::FilePath handler(exeDir + L"./crashpad_handler.exe");
    base::FilePath dataDir(exeDir + L"./CrashData");
#else
    std::string exeDir = RageFileManagerUtil::sDirOfExecutable;
    base::FilePath handler(exeDir + "./crashpad_handler");
    base::FilePath dataDir(exeDir + "./CrashData");
#endif
    std::string url; // A url is required, even if nothing is uploaded.

    auto database = crashpad::CrashReportDatabase::Initialize(dataDir);
    if (database == nullptr) return false;

    auto *client = new crashpad::CrashpadClient();
    bool status = client->StartHandler(
        handler, // relative path to executable handler file
        dataDir, // the crashpad database
        dataDir, // an existing directory for metrics data
        url, // an upload server
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