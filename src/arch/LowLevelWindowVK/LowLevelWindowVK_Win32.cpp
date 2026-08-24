#include "LowLevelWindowVK_Win32.h"
#include <archutils/Win32/GraphicsWindow.h>

LowLevelWindowVK_Win32::LowLevelWindowVK_Win32()
{
	GraphicsWindow::Initialize(false);
}

std::string
LowLevelWindowVK_Win32::TryVideoMode(const VideoModeParams& p,
									 bool& bNewDeviceOut)
{
	GraphicsWindow::CreateGraphicsWindow(p);
	return "";
}

void
LowLevelWindowVK_Win32::GetDisplaySpecs(DisplaySpecs& out) const
{
	GraphicsWindow::GetDisplaySpecs(out);
}

void
LowLevelWindowVK_Win32::Update()
{
	GraphicsWindow::Update();
}

const ActualVideoModeParams*
LowLevelWindowVK_Win32::GetActualVideoModeParams() const
{
	return static_cast<ActualVideoModeParams*>(GraphicsWindow::GetParams());
}

bool
LowLevelWindowVK_Win32::SupportsFullscreenBorderlessWindow() const
{
	return true;
}
