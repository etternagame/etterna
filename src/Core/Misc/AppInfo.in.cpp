/**
 * Core::AppInfo contains metadata that is populated by the build system.
 */
namespace Core::AppInfo {
    const char *APP_TITLE = "@PROJECT_NAME@";
    const char *APP_VERSION = "@PROJECT_VERSION@";
    const char *GIT_HASH = "@PROJECT_GIT_HASH@";
    const char *CRASHPAD_HANDLER_EXE = "@CRASHPAD_HANDLER_EXE@";
    const char *BUG_REPORT_URL = "https://github.com/etternagame/etterna/issues";
}