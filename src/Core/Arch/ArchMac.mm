#include "Arch.hpp"

#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <Metal/Metal.h>
#include <mach-o/dyld.h>
#include <sys/sysctl.h>

#include <fmt/format.h>

#include <vector>

// Translation Unit Specific Functions
static std::string getSysctlName(const char* name) {
    char buffer[200]; // Buffer to store result in
    size_t sz = sizeof(buffer);  // sysctlbyname will only take a reference to a size
    sysctlbyname(name, &buffer, &sz, nullptr, 0);
    return buffer;
}

namespace Core::Arch {

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

    ScreenDimensions getScreenDimensions(){
        auto dims = NSScreen.mainScreen.frame;
        ScreenDimensions res{
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

    ghc::filesystem::path getExecutableDirectory(){
        return NSBundle.mainBundle.bundleURL.URLByDeletingLastPathComponent.path.UTF8String;
    }
}