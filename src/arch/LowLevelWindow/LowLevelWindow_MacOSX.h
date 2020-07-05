#ifndef LOW_LEVEL_WINDOW_MACOSX_H
#define LOW_LEVEL_WINDOW_MACOSX_H

#include "LowLevelWindow.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include <objc/objc.h>

typedef const struct __CFDictionary* CFDictionaryRef;
typedef uint32_t CGDirectDisplayID;

class LowLevelWindow_MacOSX : public LowLevelWindow
{
	ActualVideoModeParams m_ActualParams;
	VideoModeParams m_CurrentParams;
	id m_WindowDelegate;
	id m_Context;
	id m_BGContext;
	CFDictionaryRef m_CurrentDisplayMode;
	CGDirectDisplayID m_DisplayID;

  public:
	LowLevelWindow_MacOSX();
	~LowLevelWindow_MacOSX();
	void* GetProcAddress(const std::string& s);
	std::string TryVideoMode(const VideoModeParams& p, bool& newDeviceOut);
	void GetDisplaySpecs(DisplaySpecs& dr) const;

	void SwapBuffers();
	void Update();

	const ActualVideoModeParams* GetActualVideoModeParams() const
	{
		return &m_ActualParams;
	}

	bool SupportsRenderToTexture() const { return true; }
	RenderTarget* CreateRenderTarget();

	bool SupportsThreadedRendering() { return m_BGContext; }
	void BeginConcurrentRendering();

  private:
	void ShutDownFullScreen();
	int ChangeDisplayMode(const VideoModeParams& p);
	void SetActualParamsFromMode(CFDictionaryRef mode);
};

#ifdef ARCH_LOW_LEVEL_WINDOW
#error "More than one LowLevelWindow selected!"
#endif
#define ARCH_LOW_LEVEL_WINDOW LowLevelWindow_MacOSX

#endif
