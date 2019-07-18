#ifndef LOW_LEVEL_WINDOW_H
#define LOW_LEVEL_WINDOW_H

#include <set>

class DisplayResolution;
typedef set<DisplayResolution> DisplayResolutions;
class VideoModeParams;
class RenderTarget;
struct RenderTargetParam;
/** @brief Handle low-level operations that OGL 1.x doesn't give us. */
class LowLevelWindow
{
  public:
	static LowLevelWindow* Create();

	virtual ~LowLevelWindow() = default;

	virtual void* GetProcAddress(const RString& s) = 0;

	// Return "" if mode change was successful, otherwise an error message.
	// bNewDeviceOut is set true if a new device was created and textures
	// need to be reloaded.
	virtual RString TryVideoMode(const VideoModeParams& p,
								 bool& bNewDeviceOut) = 0;
	virtual void GetDisplayResolutions(DisplayResolutions& out) const = 0;

	virtual void LogDebugInformation() const {}
	virtual bool IsSoftwareRenderer(RString& /* sError */) { return false; }

	virtual void SwapBuffers() = 0;
	virtual void Update() {}

	virtual const VideoModeParams* GetActualVideoModeParams() const = 0;

	virtual bool SupportsRenderToTexture() const { return false; }
	virtual RenderTarget* CreateRenderTarget() { return NULL; }

	virtual bool SupportsThreadedRendering() { return false; }
	virtual void BeginConcurrentRenderingMainThread() {}
	virtual void EndConcurrentRenderingMainThread() {}
	virtual void BeginConcurrentRendering() {}
	virtual void EndConcurrentRendering() {}
};

#endif
