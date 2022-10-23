#include "Platform.hpp"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Globals/global.h"

#include <fmt/format.h>

#include <string>
#include <fstream>

#include <X11/Xlib.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <cxxabi.h> // For UnexpectedExceptionHandler

// Translation unit specific functions
std::string getX11UTF8String(Display *dpy, Window w, Atom p) {
    Atom da, incr, type;
    int di;
    unsigned long size, dul;
    unsigned char *prop_ret = NULL;

    /* Dummy call to get type and size. */
    XGetWindowProperty(dpy, w, p, 0, 0, False, AnyPropertyType, &type, &di, &dul, &size, &prop_ret);
    XFree(prop_ret);

    incr = XInternAtom(dpy, "INCR", False);
    if (type == incr) return ""; //printf("Data too large and INCR mechanism not implemented\n");

    /* Read the data in one go. */
    XGetWindowProperty(dpy, w, p, 0, size, False, AnyPropertyType, &da, &di, &dul, &dul, &prop_ret);

    // Store in std::string before cleanup
    std::string res = fmt::format("{}", prop_ret);

    // Cleanup
    fflush(stdout);
    XFree(prop_ret);

    /* Signal the selection owner that we have successfully read the
     * data. */
    XDeleteProperty(dpy, w, p);
    return res;
}

/* Catch unhandled C++ exceptions.  Note that this works in g++ even with
 * -fno-exceptions, in which case it'll be called if any exceptions are thrown
 * at all. */
void UnexpectedExceptionHandler() {
	std::type_info* pException = abi::__cxa_current_exception_type();
	char const* pName = pException->name();
	int iStatus = -1;
	char* pDem = abi::__cxa_demangle(pName, 0, 0, &iStatus);

	const std::string error =
	  ssprintf("Unhandled exception: %s", iStatus ? pName : pDem);
	sm_crash(error.c_str());
}

/* We can define these symbol to catch failed assert() calls.  This is only used for library code that uses assert().
 * Internally we always use ASSERT, which does this for all platforms, not just glibc. */
extern "C" {
    void __assert_fail(const char* assertion, const char* file, unsigned int line, const char* function) throw() {
        const std::string error = ssprintf("Assertion failure: %s: %s", function, assertion);
        sm_crash(assertion);
    }

    void __assert_perror_fail(int errnum, const char* file, unsigned int line, const char* function) throw() {
        const std::string error = ssprintf("Assertion failure: %s: %s", function, strerror(errnum));
        sm_crash(strerror(errnum));
    }
}

namespace Core::Platform {

    void init(){
        std::set_terminate(UnexpectedExceptionHandler);
    }

    std::string getSystem(){
        // Get system info
        std::string line;

        // We're reading /etc/os-release since a lot of distributions are using this file.
        std::ifstream file{"/etc/os-release"};
        while(std::getline(file, line)){ // Read line of cpu info
            auto delimLoc = line.find('=');
            auto label = line.substr(0, delimLoc); // Get substring to only have key portion
            if(label.compare("PRETTY_NAME") != 0) continue; // If we have the wrong key, keep searching

            // Move past the quote and the space
            int startIndex = delimLoc + 2;

            // From the location after the first quote to before the last quote
            return line.substr(startIndex, line.find_last_of('"') - startIndex);
        }
        return "Unknown";
    }

    std::string getArchitecture(){
        // Get system info
        utsname info{};
        uname(&info);
        return info.machine;
    }

    std::string getKernel(){
        // Get system info
        utsname info{};
        uname(&info);
        return fmt::format("{} {}", info.sysname, info.release);
    }

    std::size_t getSystemMemory(){
        struct sysinfo info{};
        sysinfo(&info);
        return static_cast<std::size_t>(info.totalram);
    }

    std::string getSystemCPU(){
        // Read from /proc/cpuinfo
        std::string line;
        std::ifstream file{"/proc/cpuinfo"};
        while(std::getline(file, line)){ // Read line of cpu info
            auto delimLoc = line.find(':');
            auto label = line.substr(0, delimLoc - 1); // Get substring to only have key portion
            if(label.compare("model name") != 0) continue; // If we have the wrong key, keep searching
            return line.substr(delimLoc + 2, line.size() - 1);
        }
        return "Unknown";
    }

    std::string getSystemGPU(){
        return "Unknown";
    }

    Dimensions getScreenDimensions(){
        // Using X11 since it's currently a dependency, but is this the most portable?
        auto display = XOpenDisplay(nullptr);
        auto screen = XDefaultScreen(display);
        unsigned width = XDisplayWidth(display, screen);
        unsigned height = XDisplayHeight(display, screen);
        return {width, height};
    }

    Dimensions getWindowDimensions(){
        auto width = static_cast<unsigned>(PREFSMAN->m_iDisplayHeight * PREFSMAN->m_fDisplayAspectRatio);
        auto height = static_cast<unsigned>(PREFSMAN->m_iDisplayHeight);
        return {width, height};
    }

    std::string getLanguage(){
        // Since linux systems store language per user, and not the whole system,
        // we will get the user's preferred language.
        char* langRes = getenv("LANG");
        if (langRes == nullptr){
            // Return "en" if "LANG" doesn't exist.
            Locator::getLogger()->warn("Environment variable \"LANG\" not found. Using \"en\" for language.");
            return "en";
        }

        auto lang = std::string(langRes);
        auto res = lang.substr(0, 2); // First two characters should be a language
        return res;
    }

    bool openWebsite(const std::string& url){
        // Escape double quotes.
        std::string url_escaped = url;
        size_t index = 0;
        while ((index = url_escaped.find('"', index)) != std::string::npos) {
            url_escaped.replace(index, 1, "\\\""); // `1` is the length of the old `"`.
            index += 2; // `2` is the length of the new `\"`.
        }

        // xgd-open is the most portable command. Use in popular desktop environments
        int resCode = system(fmt::format("xdg-open \"{}\"", url_escaped).c_str());
        if(resCode != 0){
            Locator::getLogger()->warn("Unable to open url. xdg-open return code: {}. URL: {}", resCode, url);
            return false;
        }
        return true;
    }

    bool openFolder(const ghc::filesystem::path& path){
        return openWebsite(path.string());
    }

    std::string getClipboard(){
        // Final string to be returned to user
        std::string res;

        // Get reference to display then clipboard
        Display *display = XOpenDisplay(nullptr);
        if(display == nullptr){
            Locator::getLogger()->warn("Couldn't access clipboard. Can't open X Display.");
            return "";
        }

        Atom clipboard = XInternAtom(display, "CLIPBOARD", 0); // Get a refernece to clipboard selection
        Window owner = XGetSelectionOwner(display, clipboard); // Get refernece to clipboard selection owner
        if (owner == None) return ""; // If there is no owner, there is no clipboard. (None is X11 null constant)

        // Create a small window to paste clipboard into
        Window root = RootWindow(display, DefaultScreen(display));
        Window target_window = XCreateSimpleWindow(display, root, -10, -10, 1, 1, 0, 0, 0);
        Atom target_property = XInternAtom(display, "ETT_CLIPBOARD", 0);

        // Tell the owner we want a UTF8 string
        Atom utf8 = XInternAtom(display, "UTF8_STRING", 0);
        XConvertSelection(display, clipboard, utf8, target_property, target_window, CurrentTime);

        // Event loop to want and check to see if the other application has convereted the
        // stirng as requested. We get notified on the SelectionNotify event. If it is successful,
        // we run the function that calls gets the X11 property. If it is not, we return an empty
        // string.
        XEvent ev;
        XSelectionEvent *sev;
        while(true) {
            XNextEvent(display, &ev);
            switch (ev.type) {
                case SelectionNotify:
                    sev = (XSelectionEvent*)&ev.xselection;
                    if (sev->property != None) {
                        res = getX11UTF8String(display, target_window, target_property);
                        goto main; // Jump down to the outside of the infinite while loop.
                    } else {
                        return "";
                    }
            }
        }
        main: // Label to leave loop
        // At this point, we should have either returend an empty string, or retrieved the
        // properly convereted UTF-8 string.

        // Remove newline characters
        res.erase(std::remove(res.begin(), res.end(), '\n'), res.end()); // Remove newlines
		res.erase(std::remove(res.begin(), res.end(), '\r'), res.end()); // Remove carriage returns

        // Cleanup
        XDestroyWindow(display, target_window);
        XCloseDisplay(display);
        return res;
    }

    void setCursorVisible(bool value){
        Locator::getLogger()->warn("Core::Platform::setCursorVisible not implemented");
    }

    ghc::filesystem::path getExecutableDirectory(){
        char locationBuffer[FILENAME_MAX] = {0};
        int maxSize = sizeof(locationBuffer);
        int bytesRead = std::min<int>(readlink("/proc/self/exe", locationBuffer, maxSize), maxSize - 1);

        // Null-terminate (readlink doesn't do this)
        if(bytesRead > 0) locationBuffer[bytesRead] = '\0';

        // Remove last element of path
        char* lastForwardSlash = strrchr(&locationBuffer[0], '/');
        if (lastForwardSlash == NULL) return "";
        *lastForwardSlash = '\0';

        return locationBuffer;
    }

    ghc::filesystem::path getAppDirectory() {
	    return getExecutableDirectory();
	}

	bool isOtherInstanceRunning(int argc, char** argv){
        Locator::getLogger()->warn("Core::Platform::isOtherInstanceRunning not implemented");
        return false;
    }

    bool boostPriority()
    {
        Locator::getLogger()->warn("Core::Platform::boostPriority not implemented");
		return true;
    }

	bool unboostPriority()
    {
        Locator::getLogger()->warn("Core::Platform::unboostPriority not implemented");
		return true;
    }

    bool requestUserAttention()
    {
        Locator::getLogger()->warn("Core::Platform::flashWindow not implemented");
        return true;
    }
}
