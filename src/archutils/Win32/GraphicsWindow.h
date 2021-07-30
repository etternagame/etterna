#ifndef GRAPHICS_WINDOW_H
#define GRAPHICS_WINDOW_H

#include "Etterna/Models/Misc/DisplaySpec.h"
#include <windows.h>

class VideoModeParams;
class ActualVideoModeParams;

/** @brief Sets up a window for OpenGL/D3D. */
namespace GraphicsWindow {
	void Initialize(bool bD3D); /** @brief Set up, and create a hidden window. This only needs to be called once. */
	void Shutdown(); /** @brief Shut down completely. */

	/** @brief Create the window. This also updates VideoModeParams (returned by GetParams). */
	void CreateGraphicsWindow(const VideoModeParams& p, bool bForceRecreateWindow = false);
	void DestroyGraphicsWindow();

	void GetDisplaySpecs(DisplaySpecs& out);
	auto PushWindow(int a, int b) -> BOOL;
	auto GetParams() -> ActualVideoModeParams*;

	void Update();
	auto GetHwnd() -> HWND;
} // namespace GraphicsWindow;

#endif
