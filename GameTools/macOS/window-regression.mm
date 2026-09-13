// Native integration test for Etterna's macOS OpenGL window.
// Build and run only in an isolated game directory; see Docs/macOSGraphics.md.
// It resizes the window, captures the game's own framebuffer, and exits.
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <ImageIO/ImageIO.h>
#include <vector>
#include <string>
#include <atomic>
#include <cstdio>
#include <cstdlib>

static std::atomic<int> viewportWidth{0}, viewportHeight{0}, swapInterval{-1};
static std::atomic<unsigned long> frames{0};
static FILE *report;
static bool failed;
static int expectedVsync = 1;
static std::atomic<int> capturePhase{-1};

static void saveFrame(int phase, int width, int height)
{
    std::vector<unsigned char> source(size_t(width) * height * 4), pixels(source.size());
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, source.data());
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
            size_t a = (size_t(y) * width + x) * 4;
            size_t b = (size_t(height - y - 1) * width + x) * 4;
            for (int c = 0; c < 3; c++) pixels[a + c] = source[b + c];
            pixels[a + 3] = 255;
        }
    CGDataProviderRef provider = CGDataProviderCreateWithData(nullptr, pixels.data(), pixels.size(), nullptr);
    CGColorSpaceRef color = CGColorSpaceCreateDeviceRGB();
    CGImageRef image = CGImageCreate(width, height, 8, 32, width * 4, color,
        kCGImageAlphaLast, provider, nullptr, false, kCGRenderingIntentDefault);
    std::string path = std::string(std::getenv("ETTERNA_WINDOW_TEST_LOG")) + "-" + std::to_string(phase) + ".png";
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(nullptr,
        (const UInt8 *)path.data(), path.size(), false);
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr);
    if (dest) {
        CGImageDestinationAddImage(dest, image, nullptr);
        CGImageDestinationFinalize(dest);
        CFRelease(dest);
    }
    CFRelease(url);
    CGImageRelease(image);
    CGColorSpaceRelease(color);
    CGDataProviderRelease(provider);
}

static CGLError recordPresent(CGLContextObj context)
{
    GLint viewport[4] = {}, swap = -1;
    glGetIntegerv(GL_VIEWPORT, viewport);
    CGLGetParameter(context, kCGLCPSwapInterval, &swap);
    viewportWidth.store(viewport[2]);
    viewportHeight.store(viewport[3]);
    swapInterval.store(swap);
    frames.fetch_add(1);
    int phase = capturePhase.exchange(-1);
    if (phase >= 0) saveFrame(phase, viewport[2], viewport[3]);
    return CGLFlushDrawable(context);
}

__attribute__((used)) static struct {
    const void *replacement;
    const void *original;
} interpose __attribute__((section("__DATA,__interpose"))) = {
    (const void *)recordPresent, (const void *)CGLFlushDrawable
};

static NSWindow *gameWindow()
{
    for (NSWindow *window in [NSApp windows])
        if (([window styleMask] & NSWindowStyleMaskResizable) && [window isVisible])
            return window;
    return nil;
}

static const char *names[] = {
    "window-800x600", "window-1280x720", "maximize", "restore",
    "native-fullscreen", "exit-fullscreen", "window-854x480"
};

static void step(unsigned phase)
{
    NSWindow *window = gameWindow();
    if (!window) {
        fprintf(report, "FAIL: game window disappeared\n");
        fflush(report);
        std::_Exit(1);
    }
    switch (phase) {
        case 0: [window setContentSize:NSMakeSize(800, 600)]; break;
        case 1: [window setContentSize:NSMakeSize(1280, 720)]; break;
        case 2: case 3: [window zoom:nil]; break;
        case 4: case 5: [window toggleFullScreen:nil]; break;
        case 6: [window setContentSize:NSMakeSize(854, 480)]; break;
    }
    unsigned long before = frames.load();
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        NSView *view = [window contentView];
        NSRect bounds = [view bounds];
        NSSize pixels = [view wantsBestResolutionOpenGLSurface] ?
            [view convertRectToBacking:bounds].size : bounds.size;
        int width = viewportWidth.load(), height = viewportHeight.load();
        bool fullscreen = ([window styleMask] & NSWindowStyleMaskFullScreen) != 0;
        unsigned long count = frames.load() - before;
        bool ok = width == int(pixels.width) && height == int(pixels.height) &&
            swapInterval.load() == expectedVsync && count > 5 && fullscreen == (phase == 4);
        failed |= !ok;
        fprintf(report, "%s %s: view=%.0fx%.0f viewport=%dx%d vsync=%d frames=%lu fullscreen=%d\n",
                ok ? "PASS" : "FAIL", names[phase], pixels.width, pixels.height,
                width, height, swapInterval.load(), count, fullscreen);
        fflush(report);
        capturePhase.store(phase);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if (phase < 6)
            step(phase + 1);
        else {
            fprintf(report, "%s: native window/viewport/presentation regression\n", failed ? "FAILED" : "PASSED");
            fflush(report);
            std::_Exit(failed ? 1 : 0);
        }
        });
    });
}

static void awaitWindow(unsigned attempts)
{
    if (gameWindow() && frames.load() > 120) {
        step(0);
        return;
    }
    if (attempts == 60) {
        fprintf(report, "FAIL: renderer did not start within 60 seconds\n");
        fflush(report);
        std::_Exit(1);
    }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        awaitWindow(attempts + 1);
    });
}

__attribute__((constructor)) static void startRegression()
{
    const char *path = std::getenv("ETTERNA_WINDOW_TEST_LOG");
    if (!path) return;
    if (FILE *prefs = std::fopen("Save/Preferences.ini", "r")) {
        char line[1024];
        while (std::fgets(line, sizeof(line), prefs))
            std::sscanf(line, "Vsync=%d", &expectedVsync);
        std::fclose(prefs);
    }
    report = std::fopen(path, "w");
    if (!report) std::_Exit(2);
    dispatch_async(dispatch_get_main_queue(), ^{ awaitWindow(0); });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 90 * NSEC_PER_SEC),
                   dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
        fprintf(report, "FAIL: timeout (possible window/render deadlock)\n");
        fflush(report);
        std::_Exit(1);
    });
}
