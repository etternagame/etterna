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

	void* GetProcAddress(const std::string& s);
	std::string TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut);
	void LogDebugInformation() const;
	bool IsSoftwareRenderer(std::string& sError);
	void SwapBuffers();
	void Update();

	const ActualVideoModeParams* GetActualVideoModeParams() const
	{
		return &CurrentParams;
	}

	void GetDisplaySpecs(DisplaySpecs& out) const;

	bool SupportsRenderToTexture() const;
	RenderTarget* CreateRenderTarget();

	bool SupportsFullscreenBorderlessWindow() const;

	bool SupportsThreadedRendering();
	void BeginConcurrentRenderingMainThread();
	void EndConcurrentRenderingMainThread();
	void BeginConcurrentRendering();
	void EndConcurrentRendering();

  private:
	void RestoreOutputConfig();

	bool m_bWasWindowed;
	ActualVideoModeParams CurrentParams;
};

#ifdef ARCH_LOW_LEVEL_WINDOW
#error "More than one LowLevelWindow selected!"
#endif
#define ARCH_LOW_LEVEL_WINDOW LowLevelWindow_X11

#endif
