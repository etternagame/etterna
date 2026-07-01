#ifndef LOW_LEVEL_WINDOW_VK_WIN32_H
#define LOW_LEVEL_WINDOW_VK_WIN32_H

#include "LowLevelWindowVK.h"

class LowLevelWindowVK_Win32 : public LowLevelWindowVK
{
  public:
	LowLevelWindowVK_Win32();
	~LowLevelWindowVK_Win32() {}

	// Return "" if mode change was successful, otherwise an error message.
	// bNewDeviceOut is set true if a new device was created and textures
	// need to be reloaded.
	std::string TryVideoMode(const VideoModeParams& p,
							 bool& bNewDeviceOut) override;
	void GetDisplaySpecs(DisplaySpecs& out) const override;
	void Update() override;

	const ActualVideoModeParams* GetActualVideoModeParams() const override;
	bool SupportsFullscreenBorderlessWindow() const override;
};

#endif
