#ifndef GRAPHICS_WINDOW_H
#define GRAPHICS_WINDOW_H

#include "Etterna/Models/Misc/DisplayResolutions.h"
#include <windows.h>

class VideoModeParams;
class ActualVideoModeParams;

/** @brief Sets up a window for OpenGL/D3D. */
namespace GraphicsWindow {
/** @brief Set up, and create a hidden window.
 *
 * This only needs to be called once. */
void
Initialize(bool bD3D);

/** @brief Shut down completely. */
void
Shutdown();

/** @brief Set the display mode.
 *
 * p will not be second-guessed, except to try disabling the refresh rate
 * setting. */
RString
SetScreenMode(const VideoModeParams& p);

/** @brief Create the window.
 *
 * This also updates VideoModeParams (returned by GetParams). */
void
CreateGraphicsWindow(const VideoModeParams& p,
					 bool bForceRecreateWindow = false);
void
DestroyGraphicsWindow();

void
GetDisplayResolutions(DisplayResolutions& out);

ActualVideoModeParams*
GetParams();

HDC
GetHDC();
void
Update();

HWND
GetHwnd();
};

#endif
