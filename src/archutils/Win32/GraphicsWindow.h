#ifndef GRAPHICS_WINDOW_H
#define GRAPHICS_WINDOW_H

#include <windows.h>

/** @brief Sets up a window for OpenGL/D3D. */
namespace GraphicsWindow {
	void SetIsFullscreen(bool value);
	auto GetHwnd() -> HWND;
	void SetHwnd(HWND hwnd);

	auto PushWindow(int a, int b) -> BOOL;
} // namespace GraphicsWindow;

#endif
