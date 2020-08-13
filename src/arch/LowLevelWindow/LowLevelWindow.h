#ifndef LOW_LEVEL_WINDOW_H
#define LOW_LEVEL_WINDOW_H

#include <set>

class DisplaySpec;
typedef std::set<DisplaySpec> DisplaySpecs;
class VideoModeParams;
class ActualVideoModeParams;
class RenderTarget;
struct RenderTargetParam;
/** @brief Handle low-level operations that OGL 1.x doesn't give us. */
class LowLevelWindow
{
  public:
	static LowLevelWindow* Create();

	virtual ~LowLevelWindow() = default;

	virtual void* GetProcAddress(const std::string& s) = 0;

	// Return "" if mode change was successful, otherwise an error message.
	// bNewDeviceOut is set true if a new device was created and textures
	// need to be reloaded.
	virtual std::string TryVideoMode(const VideoModeParams& p,
									 bool& bNewDeviceOut) = 0;
	virtual void GetDisplaySpecs(DisplaySpecs& out) const = 0;

	virtual void LogDebugInformation() const {}
	virtual bool IsSoftwareRenderer(std::string& /* sError */) { return false; }

	virtual void SwapBuffers() = 0;
	virtual void Update() {}

	virtual const ActualVideoModeParams* GetActualVideoModeParams() const = 0;

	virtual bool SupportsRenderToTexture() const { return false; }
	virtual RenderTarget* CreateRenderTarget() { return nullptr; }

	virtual bool SupportsFullscreenBorderlessWindow() const { return false; };

	virtual bool SupportsThreadedRendering() { return false; }
	virtual void BeginConcurrentRenderingMainThread() {}
	virtual void EndConcurrentRenderingMainThread() {}
	virtual void BeginConcurrentRendering() {}
	virtual void EndConcurrentRendering() {}
};

#endif
