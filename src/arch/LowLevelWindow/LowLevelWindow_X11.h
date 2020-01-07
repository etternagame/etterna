/* LowLevelWindow_X11 - OpenGL GLX window driver. */

#ifndef LOW_LEVEL_WINDOW_X11_H
#define LOW_LEVEL_WINDOW_X11_H

#include "RageUtil/Graphics/RageDisplay.h" // VideoModeParams
#include "LowLevelWindow.h"

class LowLevelWindow_X11 : public LowLevelWindow
{
  public:
	LowLevelWindow_X11();
	~LowLevelWindow_X11();

	void* GetProcAddress(const RString& s);
	RString TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut);
	void LogDebugInformation() const;
	bool IsSoftwareRenderer(RString& sError);
	void SwapBuffers();

	const VideoModeParams* GetActualVideoModeParams() const
	{
		return &CurrentParams;
	}

	void GetDisplayResolutions(DisplayResolutions& out) const;

	bool SupportsRenderToTexture() const;
	RenderTarget* CreateRenderTarget();

	bool SupportsThreadedRendering();
	void BeginConcurrentRenderingMainThread();
	void EndConcurrentRenderingMainThread();
	void BeginConcurrentRendering();
	void EndConcurrentRendering();

  private:
	bool m_bWasWindowed;
	VideoModeParams CurrentParams;
};

#ifdef ARCH_LOW_LEVEL_WINDOW
#error "More than one LowLevelWindow selected!"
#endif
#define ARCH_LOW_LEVEL_WINDOW LowLevelWindow_X11

#endif
