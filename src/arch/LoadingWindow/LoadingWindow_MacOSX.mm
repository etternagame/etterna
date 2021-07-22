#import <Cocoa/Cocoa.h>
#import "LoadingWindow_MacOSX.h"
#import "Core/Platform/Window/GLFWWindowBackend.hpp"
#import "RageUtil/Utils/RageUtil.h"
#import "RageUtil/File/RageFile.h"
#import "Etterna/Singletons/ThemeManager.h"
#import "Core/Misc/AppInfo.hpp"
#import "Core/Services/Locator.hpp"

#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3.h>
#import <GLFW/glfw3native.h>

using namespace Core::Platform::Window;

@interface LoadingWindowHelper : NSObject  {
@public std::unique_ptr <GLFWWindowBackend> window;
@public NSTextView *m_Text;
@public NSProgressIndicator *m_ProgressIndicator;
}
- (void) setupWindow:(NSImage *)image;
@end

static LoadingWindowHelper *g_helper = nullptr;

@implementation LoadingWindowHelper

- (void) setupWindow:(NSImage *)image  {
    NSSize size = [image size];
    NSRect viewRect, windowRect;
    float height = 0.0f;
    float padding = 5.0f;

    NSRect progressIndicatorRect;
    progressIndicatorRect = NSMakeRect(padding, padding, size.width-padding*2.0f, 0);
    m_ProgressIndicator = [[NSProgressIndicator alloc] initWithFrame:progressIndicatorRect];
    [m_ProgressIndicator sizeToFit];
    [m_ProgressIndicator setIndeterminate:YES];
    [m_ProgressIndicator setMinValue:0];
    [m_ProgressIndicator setMaxValue:1];
    [m_ProgressIndicator setDoubleValue:0];
    progressIndicatorRect = [m_ProgressIndicator frame];
    auto progressHeight = static_cast<float>(progressIndicatorRect.size.height);

    NSFont *font = [NSFont systemFontOfSize:0.0f];
    NSRect textRect;
    // Just give it a size until it is created.
    textRect = NSMakeRect( 0, progressHeight + padding, size.width, size.height );
    m_Text = [[NSTextView alloc] initWithFrame:textRect];
    [m_Text setFont:font];
    height = static_cast<float>([[m_Text layoutManager] defaultLineHeightForFont:font] * 3 + 4);
    textRect = NSMakeRect( 0, progressHeight + padding, size.width, height );

    [m_Text setFrame:textRect];
    [m_Text setEditable:NO];
    [m_Text setSelectable:NO];
    [m_Text setDrawsBackground:NO];
    [m_Text setBackgroundColor:[NSColor lightGrayColor]];
    [m_Text setAlignment:NSTextAlignmentCenter];
    [m_Text setHorizontallyResizable:NO];
    [m_Text setVerticallyResizable:NO];
    [m_Text setString:@"Initializing Etterna..."];

    viewRect = NSMakeRect( 0, height + progressHeight + padding, size.width, size.height );
    NSImageView *iView = [[NSImageView alloc] initWithFrame:viewRect];
    [iView setImage:image];
    [iView setImageFrameStyle:NSImageFrameNone];
    windowRect = NSMakeRect( 0, 0, size.width, size.height + height + progressHeight + padding);

    // Prepare window
    Dimensions dims{
            static_cast<unsigned int>(windowRect.size.width),
            static_cast<unsigned int>(windowRect.size.height)
    };

    // Create window via GLFW
    window = std::make_unique<GLFWWindowBackend>(Core::AppInfo::APP_TITLE, dims);
    GLFWWindowBackend::setWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window->create();
    window->update();

    // Get window reference to customize view
    NSWindow *m_Window = glfwGetCocoaWindow((GLFWwindow*)window->getNativeWindow());
    NSView *view = [m_Window contentView];

    // Set some properties.
    [m_Window setReleasedWhenClosed:YES];
    [m_Window setExcludedFromWindowsMenu:YES];

    // Set subviews.
    [view addSubview:m_Text];
    [m_Text release];

    [view addSubview:iView];
    [iView release];

    [view addSubview:m_ProgressIndicator];

    window->update();
}

@end

LoadingWindow_MacOSX::LoadingWindow_MacOSX() {
    // The last file in the foundFiles list is the one we want.
    // The first one added will be the fallback.
    // The fall back should always exist
    // Find image location
    vector<std::string> foundFiles;
    GetDirListing("Data/splash*.png", foundFiles, false, true);

    if(THEME != nullptr)
        GetDirListing(THEME->GetPathG("Common", "splash"), foundFiles, false, true);

    if(foundFiles.empty()){
        Locator::getLogger()->warn("No loading window splash images found. No loading screen.");
        return;
    }

    // Load last found image
    RageFile imageFile;
    std::string imageData;
    if(!imageFile.Open(foundFiles[foundFiles.size() - 1])) // Open last file
        return;

    imageFile.Read(imageData);
    if(imageData.empty()){
        Locator::getLogger()->warn("Image file empty");
        return;
    }

    // Convert to NSImage
    NSData *nsImageData = [[[NSData alloc] initWithBytes:(imageData.data()) length:(imageData.length())] autorelease];
    NSImage *image = [[[NSImage alloc] initWithData:(nsImageData)] autorelease];
    if(!image) return;

    g_helper = [[[LoadingWindowHelper alloc] init] autorelease];
    [g_helper performSelectorOnMainThread:@selector(setupWindow:) withObject:image waitUntilDone:YES];
}

LoadingWindow_MacOSX::~LoadingWindow_MacOSX(){
    [g_helper release];
}

void LoadingWindow_MacOSX::SetText(const std::string& str) {
    NSString *s = [[NSString alloc] initWithUTF8String:(str.c_str())];
    [g_helper->m_Text setString:(s)];
    g_helper->window->update();
    [s release];
}

void LoadingWindow_MacOSX::SetProgress(const int progress) {
    [g_helper->m_ProgressIndicator setDoubleValue:[@((double) progress) doubleValue]];
    g_helper->window->update();

}

void LoadingWindow_MacOSX::SetTotalWork(const int totalWork) {
    [g_helper->m_ProgressIndicator setMaxValue:[@((double) totalWork) doubleValue]];
    g_helper->window->update();

}

void LoadingWindow_MacOSX::SetIndeterminate(bool indeterminate) {
    [g_helper->m_ProgressIndicator setIndeterminate:indeterminate];
    g_helper->window->update();
}
