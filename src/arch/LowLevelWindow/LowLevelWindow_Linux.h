#ifndef LOW_LEVEL_WINDOW_LINUX_H
#define LOW_LEVEL_WINDOW_LINUX_H

#include "RageUtil/Graphics/RageDisplay.h"
#include "LowLevelWindow.h"

// LowLevelWindow_Linux exists to select between the X11 and Wayland
// LowLevelWindow implementations at runtime. When it is constructed,
// it checks which display server is available and forwards all calls
// to the appropriate implementation.
class LowLevelWindow_Linux : public LowLevelWindow
{
  public:
	LowLevelWindow_Linux();
	~LowLevelWindow_Linux();

	void* GetProcAddress(const std::string& s);
	std::string TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut);
	void LogDebugInformation() const;
	bool IsSoftwareRenderer(std::string& sError);
	void SwapBuffers();
	void Update();

	const ActualVideoModeParams* GetActualVideoModeParams() const;

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
	LowLevelWindow* actualLowLevelWindow;
};

#ifdef ARCH_LOW_LEVEL_WINDOW
#error "More than one LowLevelWindow selected!"
#endif
#define ARCH_LOW_LEVEL_WINDOW LowLevelWindow_Linux

#endif