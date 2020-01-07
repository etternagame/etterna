#ifndef LOW_LEVEL_WINDOW_MACOSX_H
#define LOW_LEVEL_WINDOW_MACOSX_H

#include "LowLevelWindow.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include <objc/objc.h>

typedef const struct __CFDictionary* CFDictionaryRef;
typedef uint32_t CGDirectDisplayID;

class LowLevelWindow_MacOSX : public LowLevelWindow
{
	VideoModeParams m_CurrentParams;
	id m_WindowDelegate;
	id m_Context;
	id m_BGContext;
	CFDictionaryRef m_CurrentDisplayMode;
	CGDirectDisplayID m_DisplayID;

  public:
	LowLevelWindow_MacOSX();
	~LowLevelWindow_MacOSX();
	void* GetProcAddress(const RString& s);
	RString TryVideoMode(const VideoModeParams& p, bool& newDeviceOut);
	void GetDisplayResolutions(DisplayResolutions& dr) const;

	void SwapBuffers();
	void Update();

	const VideoModeParams* GetActualVideoModeParams() const
	{
		return &m_CurrentParams;
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
