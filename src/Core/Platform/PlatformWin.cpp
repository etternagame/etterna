#include "Platform.hpp"

#include "Core/Services/Locator.hpp"
#include "Core/Misc/AppInfo.hpp"
#include "archutils/Win32/GraphicsWindow.h"
#include "Etterna/Globals/global.h"

#include <windows.h>
#include <GL/glew.h>
#include <string>
#include <nowide/convert.hpp>
#include <intsafe.h>
#include <limits>
#include <iostream>
#include <cwchar>

// Translation Unit Specific Functions
struct CallbackData { HWND hParent; HWND hResult; };

// TODO: Consider *any* possibility where we don't need to access the registry. Works, but is it ideal?
std::string RegistryGetString(HKEY hKey, const std::string& subKey, const std::string& value){
    LONG retCode; // Return code for registry functions
    DWORD bufferSize{};  // buffer for return value
    std::string result; // result for registry value

    // Get size of registry key
    retCode = ::RegGetValue(hKey, subKey.c_str(), value.c_str(), RRF_RT_REG_SZ, nullptr,nullptr, &bufferSize);

    // Notify if we can't read the key
    if (retCode != ERROR_SUCCESS) {
        Locator::getLogger()->error("Unable to read registry key: {}:{}", subKey, value);
        return "";
    }
    result.resize(bufferSize); // Create wide string with enough space

    // Call same function to get value
    retCode = ::RegGetValueA(hKey, subKey.c_str(), value.c_str(), RRF_RT_REG_SZ, nullptr, &result[0], &bufferSize);

    // Notify if we can't read the key
    if (retCode != ERROR_SUCCESS) {
        Locator::getLogger()->error("Unable to read registry key: {}:{}", subKey, value);
        return "";
    }

    // Remove last null bit
    DWORD stringLengthInWchars = bufferSize;
    stringLengthInWchars--; // Exclude the NUL written by the Win32 API
    result.resize(stringLengthInWchars);

    return result;
}

unsigned RegistryGetDWORD(HKEY hKey, const std::string& subKey, const std::string& value){
    LONG retCode; // Return code for registry functions
    DWORD result;  // result for registry value
    DWORD bufferSize(sizeof(result)); // size of DWORD

    // Get size of registry key
    retCode = ::RegGetValue(hKey, subKey.c_str(), value.c_str(), RRF_RT_REG_DWORD, nullptr, &result, &bufferSize);

    // Notify if we can't read the key
    if (retCode != ERROR_SUCCESS) {
        Locator::getLogger()->error("Unable to read registry key: {}:{}", subKey, value);
        return 0;
    }

    return static_cast<unsigned>(result);
}

static BOOL CALLBACK GetEnabledPopup(HWND hWnd, LPARAM lParam) {
	CallbackData* pData = (CallbackData*)lParam;
	if (GetParent(hWnd) != pData->hParent)
		return TRUE;
	if ((GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP) != WS_POPUP)
		return TRUE;
	if (!IsWindowEnabled(hWnd))
		return TRUE;

	pData->hResult = hWnd;
	return FALSE;
}

void InvalidParameterHandler(const wchar_t* szExpression, const wchar_t* szFunction, const wchar_t* szFile,
						unsigned int iLine, uintptr_t pReserved) {
	Locator::getLogger()->trace("Entered Invalid Parameter Handler");

	std::mbstate_t state = std::mbstate_t();
	int lenExpr = 1 + std::wcsrtombs(NULL, &szExpression, 0, &state);
	state = std::mbstate_t();
	int lenFunc = 1 + std::wcsrtombs(NULL, &szFunction, 0, &state);
	state = std::mbstate_t();
	int lenFile = 1 + std::wcsrtombs(NULL, &szFile, 0, &state);

	std::vector<char> strExpr(lenExpr);
	std::vector<char> strFunc(lenFunc);
	std::vector<char> strFile(lenFile);

	std::wcsrtombs(&strExpr[0], &szExpression, lenExpr, &state);
	std::wcsrtombs(&strFunc[0], &szFunction, lenFunc, &state);
	std::wcsrtombs(&strFile[0], &szFile, lenFile, &state);

	std::string expr(strExpr.begin(), strExpr.end());
	std::string func(strFunc.begin(), strFunc.end());
	std::string file(strFile.begin(), strFile.end());

	FAIL_M(ssprintf("Invalid Parameter In C Function %s\n File: %s Line %d\n Expression: %s",
	  func, file, iLine, expr));
}

namespace Core::Platform {

    void init(){
        /* Disable critical errors, and handle them internally.  We never want the
         * "drive not ready", etc. dialogs to pop up. */
        SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);

        _set_invalid_parameter_handler(InvalidParameterHandler);

        /* Windows boosts priority on keyboard input, among other things.  Disable
         * that for the main thread. */
        SetThreadPriorityBoost(GetCurrentThread(), TRUE);
    }

	std::string getSystem(){
	    // Registry subkey where values are located
        std::string versionKey = R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)";

        // Use registry to access each of the values
        // TODO: Verify it works as intended with other devs.
        std::string productName = RegistryGetString(HKEY_LOCAL_MACHINE, versionKey, "EditionId");
        unsigned major = RegistryGetDWORD(HKEY_LOCAL_MACHINE, versionKey, "CurrentMajorVersionNumber");
        unsigned minor = RegistryGetDWORD(HKEY_LOCAL_MACHINE, versionKey, "CurrentMinorVersionNumber");
        std::string build = RegistryGetString(HKEY_LOCAL_MACHINE, versionKey, "CurrentBuildNumber");

        return fmt::format("Windows {}.{} {} (Build #{})", major, minor, productName, build);
	}

	std::string getArchitecture(){
	    SYSTEM_INFO info; // Struct with computer/runtime info
	    GetNativeSystemInfo(&info);

	    // Use switch statements to determine processor type
	    switch(info.wProcessorArchitecture){
	        case PROCESSOR_ARCHITECTURE_AMD64: return "x64";
	        case PROCESSOR_ARCHITECTURE_INTEL: return "i386";
	        case PROCESSOR_ARCHITECTURE_UNKNOWN:
	        default: return "Unknown";
	    }
	}

    std::string getKernel(){
	    std::string versionKey = R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)";
	    unsigned major = RegistryGetDWORD(HKEY_LOCAL_MACHINE, versionKey, "CurrentMajorVersionNumber");
        unsigned minor = RegistryGetDWORD(HKEY_LOCAL_MACHINE, versionKey, "CurrentMinorVersionNumber");
		return fmt::format("Windows NT {}.{}", major, minor);
	}

    std::size_t getSystemMemory(){
	    // Note: This assumes the user has the correct bit OS installed. If the user has 16GB of ram, but instealled
	    // the 32bit operating system, the function will only report 4gb of ram.
        ULONGLONG numBytes{0};
        GetPhysicallyInstalledSystemMemory(&numBytes);
        numBytes *= 1000; // Convert from kilobytes to bytes
	    return static_cast<std::size_t>(numBytes);
	}

    std::string getSystemCPU(){
		// Registry and Result values
		std::string registryPath = R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0\)";
		std::string value = "ProcessorNameString";

		// Get from registry
        return RegistryGetString(HKEY_LOCAL_MACHINE, registryPath, value);
	}

    std::string getSystemGPU(){
	    // FIXME: Requires OpenGL backend to be initialized.
	    std::string vendor = (char *)glGetString(GL_VENDOR); // Returns the vendor
        std::string renderer = (char *)glGetString(GL_RENDERER); // Returns a hint to the model
		return fmt::format("{} {}", vendor, renderer);
	}

    Dimensions getScreenDimensions(){
        // Get screen information
	    RECT screen;
	    HWND hDesktop = GetDesktopWindow();
	    GetWindowRect(hDesktop, &screen);

	    // Convert into Dimensions struct
	    Dimensions dims{};
	    dims.width = static_cast<unsigned>(screen.right);
	    dims.height = static_cast<unsigned>(screen.bottom);

	    return dims;
	}

	Dimensions getWindowDimensions(){
	    // TODO: Should this be handled by the window class?
        // Get screen information
	    RECT window;
	    GetClientRect(GraphicsWindow::GetHwnd(), &window);

	    // Convert into Dimensions struct
	    Dimensions dims{};
	    dims.width = static_cast<unsigned>(window.right - window.left);
	    dims.height = static_cast<unsigned>(window.bottom - window.top);

	    return dims;
	}

    std::string getLanguage(){
	    // First, get the number of languages in the system.
	    ULONG numLanguages = 0;
	    DWORD languageBufferSize = 0;
		GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, nullptr, &languageBufferSize);

        // Then, put all those language codes in a buffer
		wchar_t languages(static_cast<int>(languageBufferSize));
        GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, &languages, &languageBufferSize);

        // Convert to std::string
        std::wstring ws(&languages);
        std::string str(ws.begin(), ws.end() - 1);
		return str;
	}

    bool openWebsite(const std::string& url){
	    // ShellExecuteA returns HINSTANCE for compatibility, and must be converted into int.
	    HINSTANCE ret = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        auto res = reinterpret_cast<ULONG_PTR>(ret);

        // A result value greater than 32 means the operating was successful.
	    return res > 32;
	}

	bool openFolder(const ghc::filesystem::path& path){
        if(!ghc::filesystem::is_directory(path)){
            Locator::getLogger()->warn("Could not open folder. Note a folder. Path: \"{}\"", path.string());
            return false;
        }
        ShellExecuteA(nullptr, nullptr, path.native().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return true;
    }

    std::string getClipboard(){
		// Attempt to open the clipboard, and prevent others from accessing
		if(!OpenClipboard(nullptr)) return "";

		// Get text handler on clipboard, and prevent other applications from accessing
		HANDLE hData = GetClipboardData(CF_TEXT);
  		if (hData == nullptr) return "";

		// Get contents by locking the handle
		char *clipboardContent = static_cast<char*>(GlobalLock(hData));
 	 	if (clipboardContent == nullptr) return "";

 	 	// Close clipboard handles
 	 	GlobalUnlock(hData);
		CloseClipboard();

		// Convert to std::string
		std::string res(clipboardContent);
		res.erase(std::remove(res.begin(), res.end(), '\n'), res.end()); // Remove newlines
		res.erase(std::remove(res.begin(), res.end(), '\r'), res.end()); // Remove carriage returns
		return res;
	}

	void setCursorVisible(bool value){
 		if (value){
		    while (ShowCursor(true) < 0);
		} else {
		    while (ShowCursor(false) >= 0);
		}
    }

    ghc::filesystem::path getExecutableDirectory(){
	    // Get a handle on the current executable.
        HMODULE hModule = GetModuleHandleW(nullptr);
        WCHAR path[MAX_PATH]; // A variable to store the path

        // Get the full path for the current executable
        DWORD retVal = GetModuleFileNameW(hModule, path, MAX_PATH);
        if (retVal == 0) return "";

        // Find the last backslash, and set it to a null character
        wchar_t *lastBackslash = wcsrchr(path, '\\');
        if (lastBackslash == nullptr) return "";
        *lastBackslash = 0;

        // Convert the UTF-16 into UTF-8
        return nowide::narrow(path);
	}

	ghc::filesystem::path getAppDirectory() {
	    auto dir = getExecutableDirectory();
	    return dir.parent_path();
	}

	bool isOtherInstanceRunning(int argc, char** argv){
        /* Search for the existing window.  Prefer to use the class name, which is
         * less likely to have a false match, and will match the gameplay window.
         * If that fails, try the window name, which should match the loading
         * window. */
        HWND hWnd = FindWindow(Core::AppInfo::APP_TITLE, nullptr);
        if (hWnd == nullptr)
            hWnd = FindWindow(nullptr, Core::AppInfo::APP_TITLE);

        // If after two find window attempts, the pointer is still null,
        // then no other game instance was found.
        if (hWnd == nullptr)
            return false;

        // If we reach this point in the function, another instance of the
        // game was found.

        /* If the application has a model dialog box open, we want to be sure to
         * give focus to it, not the main window. */
        CallbackData data;
        data.hParent = hWnd;
        data.hResult = nullptr;
        EnumWindows(GetEnabledPopup, (LPARAM)&data);

        if (data.hResult != nullptr){
            SetForegroundWindow(data.hResult);
        } else {
            SetForegroundWindow(hWnd);
        }

        // Send the command line to the existing window.
        std::vector<std::string> vsArgs;
        for (int i = 0; i < argc; i++)
            vsArgs.push_back(argv[i]);
        std::string sAllArgs = join("|", vsArgs);
        COPYDATASTRUCT cds;
        cds.dwData = 0;
        cds.cbData = sAllArgs.size();
        cds.lpData = (void*)sAllArgs.data();
        SendMessage((HWND)hWnd, // HWND hWnd = handle of destination window
            WM_COPYDATA,
            (WPARAM)NULL,	 // HANDLE OF SENDING WINDOW
            (LPARAM)&cds); // 2nd msg parameter = pointer to COPYDATASTRUCT

        return true; // Return true because the window exists
    }

	bool boostPriority()
    {
		return SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS) != 0;
    }

	bool unboostPriority()
    {
		return SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS) != 0;
    }

}
