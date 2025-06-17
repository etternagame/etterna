#ifndef LOW_LEVEL_WINDOW_WAYLAND_H
#define LOW_LEVEL_WINDOW_WAYLAND_H

#include "RageUtil/Graphics/RageDisplay.h" // VideoModeParams'
#include <wayland-egl.h>
#include <EGL/egl.h>
#include "LowLevelWindow.h"

class LowLevelWindow_Wayland : public LowLevelWindow
{
  public:
	LowLevelWindow_Wayland();
	~LowLevelWindow_Wayland();

	void* GetProcAddress(const std::string& s);
	std::string TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut);
	void SwapBuffers();
	void Update();

	const ActualVideoModeParams* GetActualVideoModeParams() const
	{
		return &CurrentParams;
	}

	void GetDisplaySpecs(DisplaySpecs& out) const;

	bool SupportsRenderToTexture() const;

  private:
	ActualVideoModeParams CurrentParams;
	EGLDisplay edisplay;

	wl_egl_window* main_window;
	EGLSurface esurface;
	EGLContext ectx;
};

#ifdef ARCH_LOW_LEVEL_WINDOW
#if ARCH_LOW_LEVEL_WINDOW != LowLevelWindow_Linux
#error "More than one LowLevelWindow selected!"
#endif
#else
#define ARCH_LOW_LEVEL_WINDOW LowLevelWindow_Wayland
#endif

#endif