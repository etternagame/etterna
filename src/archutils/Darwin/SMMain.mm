#include "Etterna/Globals/global.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Misc/RageThreads.h"
#include "Etterna/Singletons/CommandLineActions.h"

#include <Cocoa/Cocoa.h>
#import <Etterna/Globals/StepMania.h>
#include "archutils/Darwin/MouseDevice.h"
#include "Core/Services/Locator.hpp"
#include "Core/Misc/AppInfo.hpp"
#include "Etterna/Globals/GameLoop.h"

CGFloat scrolled;

float MACMouseScroll()  {
    CGFloat scrolledTmp = scrolled;
    scrolled = 0;
    return scrolledTmp;
}

float MACMouseX()  {
    NSRect frame = [[[NSApplication sharedApplication] mainWindow] frame];
    NSPoint mouseLoc = [NSEvent mouseLocation];
    return mouseLoc.x - frame.origin.x - 3; //Needed for padding the release animation
}
float MACMouseY()  {
    NSRect frame = [[[NSApplication sharedApplication] mainWindow] frame];
    NSPoint mouseLoc = [NSEvent mouseLocation];
    return frame.size.height - (mouseLoc.y - frame.origin.y - 1) - 10; //Appears to compensate for titlebar
    // This padding should be replaced in the future to use Cocoa calls to content
}

