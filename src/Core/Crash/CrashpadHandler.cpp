#include "CrashpadHandler.hpp"

#ifdef _WIN32
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/File/RageFileManager.h"
#include "Core/Services/Locator.hpp"
#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#include <string>
#include <vector>
#endif

bool Core::Crash::initCrashpad() {
#ifdef _WIN32 // Only windows is ready
	std::wstring exeDir = StringToWString(RageFileManagerUtil::sDirOfExecutable);
    base::FilePath handler(exeDir + L"./crashpad_handler.exe");
    base::FilePath dataDir(exeDir + L"./CrashData");
    std::string url;

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
#else
    return false;
#endif
}