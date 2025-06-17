#include "LowLevelWindow_Linux.h"
#include "LowLevelWindow_X11.h"
#include "LowLevelWindow_Wayland.h"

LowLevelWindow_Linux::LowLevelWindow_Linux()
{
	// check if WAYLAND_DISPLAY is set, if so assume that wayland is available
	const char* wayland_display = getenv("WAYLAND_DISPLAY");

	if (wayland_display != nullptr && *wayland_display != '\0') {
		actualLowLevelWindow = new LowLevelWindow_Wayland();
		return;
	}

	// otherwise assume X11 is available
	actualLowLevelWindow = new LowLevelWindow_X11();
}

LowLevelWindow_Linux::~LowLevelWindow_Linux()
{
	if (actualLowLevelWindow) {
		delete actualLowLevelWindow;
	}
}

void*
LowLevelWindow_Linux::GetProcAddress(const std::string& s)
{
	return actualLowLevelWindow->GetProcAddress(s);
}

std::string
LowLevelWindow_Linux::TryVideoMode(const VideoModeParams& p,
								   bool& bNewDeviceOut)
{
	return actualLowLevelWindow->TryVideoMode(p, bNewDeviceOut);
}

void
LowLevelWindow_Linux::LogDebugInformation() const
{
	actualLowLevelWindow->LogDebugInformation();
}

bool
LowLevelWindow_Linux::IsSoftwareRenderer(std::string& sError)
{
	return actualLowLevelWindow->IsSoftwareRenderer(sError);
}

void
LowLevelWindow_Linux::SwapBuffers()
{
	actualLowLevelWindow->SwapBuffers();
}

void
LowLevelWindow_Linux::Update()
{
	actualLowLevelWindow->Update();
}

const ActualVideoModeParams*
LowLevelWindow_Linux::GetActualVideoModeParams() const
{
	return actualLowLevelWindow->GetActualVideoModeParams();
}

void
LowLevelWindow_Linux::GetDisplaySpecs(DisplaySpecs& out) const
{
	actualLowLevelWindow->GetDisplaySpecs(out);
}

bool
LowLevelWindow_Linux::SupportsRenderToTexture() const
{
	return actualLowLevelWindow->SupportsRenderToTexture();
}

RenderTarget*
LowLevelWindow_Linux::CreateRenderTarget()
{
	return actualLowLevelWindow->CreateRenderTarget();
}

bool
LowLevelWindow_Linux::SupportsFullscreenBorderlessWindow() const
{
	return actualLowLevelWindow->SupportsFullscreenBorderlessWindow();
}

bool
LowLevelWindow_Linux::SupportsThreadedRendering()
{
	return actualLowLevelWindow->SupportsThreadedRendering();
}

void
LowLevelWindow_Linux::BeginConcurrentRenderingMainThread()
{
	actualLowLevelWindow->BeginConcurrentRenderingMainThread();
}

void
LowLevelWindow_Linux::EndConcurrentRenderingMainThread()
{
	actualLowLevelWindow->EndConcurrentRenderingMainThread();
}

void
LowLevelWindow_Linux::BeginConcurrentRendering()
{
	actualLowLevelWindow->BeginConcurrentRendering();
}

void
LowLevelWindow_Linux::EndConcurrentRendering()
{
	actualLowLevelWindow->EndConcurrentRendering();
}
