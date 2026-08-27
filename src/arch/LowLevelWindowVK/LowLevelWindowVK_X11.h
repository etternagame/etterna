#ifndef LOW_LEVEL_WINDOW_VK_X11_H
#define LOW_LEVEL_WINDOW_VK_X11_H

#include "LowLevelWindowVK.h"
#include <X11/extensions/Xrandr.h>
#include <memory>

class LowLevelWindowVK_X11 : public LowLevelWindowVK
{
  public:
	LowLevelWindowVK_X11();
	~LowLevelWindowVK_X11();

	// Return "" if mode change was successful, otherwise an error message.
	// bNewDeviceOut is set true if a new device was created and textures
	// need to be reloaded.
	std::string TryVideoMode(const VideoModeParams& p,
							 bool& bNewDeviceOut) override;
	void GetDisplaySpecs(DisplaySpecs& out) const override;
	void Update() override;

	const ActualVideoModeParams* GetActualVideoModeParams() const override;
	bool SupportsFullscreenBorderlessWindow() const override;

  private:
	bool NetWMSupported(Display* Dpy, Atom feature) const;
	void RestoreOutputConfig();

	Atom wmDeleteMessage = None;
	std::unique_ptr<ActualVideoModeParams> CurrentParams;
	bool m_bWasWindowed = true;

	bool m_bUseXRandR12 = false;
	int m_iRandRVerMajor = 0;
	int m_iRandRVerMinor = 0;
	RROutput m_usedCrtc = None;
	RRMode m_originalRandRMode = None;
	bool m_bChangedScreenSize = false;
	SizeID m_iOldSize = None;
	Rotation m_OldRotation = RR_Rotate_0;
	XRRScreenConfiguration* m_pScreenConfig = nullptr;
};

#endif