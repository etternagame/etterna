#ifndef LOW_LEVEL_WINDOW_VK_H
#define LOW_LEVEL_WINDOW_VK_H

#include <set>
#include <string>

class DisplaySpec;
typedef std::set<DisplaySpec> DisplaySpecs;
class VideoModeParams;
class ActualVideoModeParams;
class RenderTarget;
struct RenderTargetParam;
class LowLevelWindowVK
{
  public:
	static LowLevelWindowVK* Create();

	virtual ~LowLevelWindowVK() = default;

	// Return "" if mode change was successful, otherwise an error message.
	// bNewDeviceOut is set true if a new device was created and textures
	// need to be reloaded.
	virtual std::string TryVideoMode(const VideoModeParams& p,
									 bool& bNewDeviceOut) = 0;
	virtual void GetDisplaySpecs(DisplaySpecs& out) const = 0;
	virtual void Update() {}

	virtual const ActualVideoModeParams* GetActualVideoModeParams() const = 0;
	virtual bool SupportsFullscreenBorderlessWindow() const { return false; };
};

#endif