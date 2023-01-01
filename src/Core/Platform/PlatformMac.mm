#include "Platform.hpp"
#include <Core/Services/Locator.hpp>

#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <Metal/Metal.h>
#include <mach-o/dyld.h>
#include <sys/sysctl.h>
#include <vector>

#include <fmt/format.h>
#include <stdio.h>
#import <dlfcn.h>


// Translation Unit Specific Functions
static std::string getSysctlName(const char* name) {
    char buffer[200]; // Buffer to store result in
    size_t sz = sizeof(buffer);  // sysctlbyname will only take a reference to a size
    sysctlbyname(name, &buffer, &sz, nullptr, 0);
    return buffer;
}

// Apple API Translocation variables
// Apple introduced app translocation in macOS Sierra v10.12, which prevents apps from accessing files outside
// their own .app directory (via a relative reference). This is a problem for Etterna since all files are accessed with
// relative references to where the .app directory is located. Apple moves the app to a secure location, then
// attempts to run the program. Etterna fails right away as it requires those local files. The code in init()
// will load Apple's private security API, load the functions, and call on them to untranslocate by removing
// the quarantine restriction on the original binary location. Apple will not accept the app into their app store
// unless this code is removed.
void (*mySecTranslocateIsTranslocatedURL)(CFURLRef path, bool *isTranslocated, CFErrorRef* __nullable error);
CFURLRef __nullable (*mySecTranslocateCreateOriginalPathForURL)(CFURLRef translocatedPath, CFErrorRef * __nullable error);

namespace Core::Platform {
    void init(){
        // Load Apple private security API
        void *handle = dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_LAZY);
		if(!handle){
			Locator::getLogger()->info("Unable to load security framework!\nSkipping translocation check");
			return;
		}
        mySecTranslocateIsTranslocatedURL = reinterpret_cast<void (*)(CFURLRef, bool *, CFErrorRef *)>(dlsym(handle, "SecTranslocateIsTranslocatedURL"));
        mySecTranslocateCreateOriginalPathForURL = reinterpret_cast<CFURLRef __nullable (*)(CFURLRef, CFErrorRef * __nullable)>(dlsym(handle, "SecTranslocateCreateOriginalPathForURL"));
		if(!mySecTranslocateIsTranslocatedURL || !mySecTranslocateCreateOriginalPathForURL){
			Locator::getLogger()->info("Unable to find security framework translocation functions!\nSkipping translocation check");
			return;
		}

        // Check if we are translocated
        bool isTranslocated = false;
        mySecTranslocateIsTranslocatedURL((__bridge CFURLRef)[NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]], &isTranslocated, nullptr);

        if(isTranslocated) {
            Locator::getLogger()->info("App is translocated! Removing com.apple.quarantine extended attribute");

            // Get original app location
            auto untranslocatedURL = (NSURL *)mySecTranslocateCreateOriginalPathForURL(static_cast<CFURLRef>(NSBundle.mainBundle.bundleURL), nullptr);

            // Remove quarantine flag
            std::system(fmt::format("xattr -rd com.apple.quarantine {}", untranslocatedURL.path.UTF8String).c_str());

            // Reopen ignoring other instances
            std::system(fmt::format("open -n -a {}", untranslocatedURL.path.UTF8String).c_str());

            // Close this instance
            std::exit(0);
        }
    }

    std::string getSystem(){
        return fmt::format("macOS {}", NSProcessInfo.processInfo.operatingSystemVersionString.UTF8String);
    }

    std::string getArchitecture(){
        return getSysctlName("hw.machine");
    }

    std::string getKernel(){
        auto type = getSysctlName("kern.ostype");
        auto release = getSysctlName("kern.osrelease");
        return fmt::format("{} Version {}", type, release);
    }

    std::size_t getSystemMemory(){
        return static_cast<std::size_t>(NSProcessInfo.processInfo.physicalMemory);
    }

    std::string getSystemCPU(){
        return getSysctlName("machdep.cpu.brand_string");
    }

    std::string getSystemGPU(){
        // MTLCreateSystemDefaultDevice() returns the system preferred GPU device
        return [MTLCreateSystemDefaultDevice() name].UTF8String;
    }

    Dimensions getScreenDimensions(){
        auto dims = NSScreen.mainScreen.frame;
        Dimensions res{
            static_cast<unsigned int>(dims.size.width),
            static_cast<unsigned int>(dims.size.height)};
        return res;
    }

    Dimensions getWindowDimensions(){
        auto dims = NSApplication.sharedApplication.mainWindow.frame;
        Dimensions res{
            static_cast<unsigned int>(dims.size.width),
            static_cast<unsigned int>(dims.size.height)};
        return res;
    }

    std::string getLanguage(){
		return [[NSLocale.currentLocale objectForKey:NSLocaleLanguageCode] UTF8String];
    }

    bool openWebsite(const std::string& url){
        // Allocate String and attempt to open
        NSURL *nsurl = [[NSURL alloc] initWithString:@(url.c_str())];
        bool successful = [NSWorkspace.sharedWorkspace openURL:(nsurl)];

        // Deallocate string and return success value.
        [nsurl release];
        return successful;
    }

    bool openFolder(const ghc::filesystem::path& path){
        if(!ghc::filesystem::is_directory(path)){
            Locator::getLogger()->warn("Could not open folder. Note a folder. Path: \"{}\"", path.string());
            return false;
        }
		NSURL* directory = [NSURL fileURLWithPath:@(path.c_str())];
		bool succeeded = [[NSWorkspace sharedWorkspace]openURL:directory];
		if(!succeeded){
            Locator::getLogger()->warn("Unable to open folder: {}", path.string());
            return false;
        }
        return true;
    }

    std::string getClipboard(){
        // Get all clipboard items
        NSArray<NSPasteboardItem *> *items = NSPasteboard.generalPasteboard.pasteboardItems;

        // Return empty string if there is nothing in pasteboard.
        if (items.count == 0)
            return "";

        // Get first element in pasteboard, and return converted string.
        NSPasteboardItem *item = NSPasteboard.generalPasteboard.pasteboardItems[0];
        return [item stringForType:(NSPasteboardTypeString)].UTF8String;
    }

    void setCursorVisible(bool value){
		static bool cursor_visible = true;
		/*NSCursor hide/unhide keeps a reference count; each hide
		 * must be matched by an unhide. We don't want this behaviour, so
		 * use a state variable
		 */
		if(value && !cursor_visible){
			[NSCursor unhide];
		}
		if(!value && cursor_visible){
			[NSCursor hide];
		}
		cursor_visible = value;
    }

    ghc::filesystem::path getExecutableDirectory(){
        return NSBundle.mainBundle.bundleURL.URLByDeletingLastPathComponent.path.UTF8String;
    }

    ghc::filesystem::path getAppDirectory() {
	    return getExecutableDirectory();
	}

    bool isOtherInstanceRunning(int argc, char** argv){
		FILE* fd = popen("pgrep Etterna", "r");
		char buf[128]; //Random value. No-one is going to have a PID size > 128
		bool found_other_process = false;
		if(fgets(buf, sizeof(buf), fd)!=nullptr){
			//Quick and dirty way to check "Is there another process open?"
			//If pgrep has any output, there is another process open right now.
			// (Pgrep excludes the calling process from its output by default)
			found_other_process = true;
		}
		pclose(fd);
		return found_other_process;
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
		[NSApp requestUserAttention:NSInformationalRequest];
        return true;
    }
}
