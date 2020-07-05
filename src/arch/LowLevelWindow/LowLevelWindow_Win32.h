#ifndef LOW_LEVEL_WINDOW_WIN32_H
#define LOW_LEVEL_WINDOW_WIN32_H

#include "LowLevelWindow.h"

class LowLevelWindow_Win32 : public LowLevelWindow
{
  public:
	LowLevelWindow_Win32();
	~LowLevelWindow_Win32();
	void* GetProcAddress(const std::string& s);
	std::string TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut);
	void GetDisplaySpecs(DisplaySpecs& out) const;
	bool IsSoftwareRenderer(std::string& sError);
	void SwapBuffers();
	void Update();
	bool SupportsThreadedRendering();
	void BeginConcurrentRendering();
	void EndConcurrentRendering();
	virtual bool SupportsRenderToTexture() const { return true; }
	virtual RenderTarget* CreateRenderTarget();

	bool SupportsFullscreenBorderlessWindow() const { return true; }

	const ActualVideoModeParams* GetActualVideoModeParams() const;
};

#ifdef ARCH_LOW_LEVEL_WINDOW
#error "More than one LowLevelWindow selected!"
#endif
#define ARCH_LOW_LEVEL_WINDOW LowLevelWindow_Win32

#endif
