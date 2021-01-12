#include "Platform.hpp"
#include <Core/Services/Locator.hpp>

#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <Metal/Metal.h>
#include <mach-o/dyld.h>
#include <sys/sysctl.h>
#include <vector>

#include <fmt/format.h>


// Translation Unit Specific Functions
static std::string getSysctlName(const char* name) {
    char buffer[200]; // Buffer to store result in
    size_t sz = sizeof(buffer);  // sysctlbyname will only take a reference to a size
    sysctlbyname(name, &buffer, &sz, nullptr, 0);
    return buffer;
}

namespace Core::Platform {

    void init(){
        Locator::getLogger()->info("macOS has not platform specific initialization");
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
        return NSLocale.currentLocale.languageCode.UTF8String;
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
        Locator::getLogger()->warn("Core::Platform::setCursorVisible not implemented");
    }

    ghc::filesystem::path getExecutableDirectory(){
        return NSBundle.mainBundle.bundleURL.URLByDeletingLastPathComponent.path.UTF8String;
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
}
