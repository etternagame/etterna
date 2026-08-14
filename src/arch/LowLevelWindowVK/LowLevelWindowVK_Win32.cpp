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

static BOOL CALLBACK
EnumerateMonitors(HMONITOR monitor,
				  HDC deviceContextHandle,
				  LPRECT monitorRect,
				  LPARAM userData)
{
	auto* out = reinterpret_cast<DisplaySpecs*>(userData);

	MONITORINFOEXW monitorInfo = {};
	monitorInfo.cbSize = sizeof(monitorInfo);
	if (!GetMonitorInfoW(monitor, &monitorInfo)) {
		return TRUE;
	}

	std::set<DisplayMode> modes;
	DEVMODEW deviceMode = {};
	deviceMode.dmSize = sizeof(deviceMode);
	deviceMode.dmDriverExtra = 0;
	DWORD modeIndex = 0;
	while (EnumDisplaySettingsW(monitorInfo.szDevice, modeIndex, &deviceMode)) {
		modes.insert({ deviceMode.dmPelsWidth,
					   deviceMode.dmPelsHeight,
					   static_cast<double>(deviceMode.dmDisplayFrequency) });
		modeIndex++;
	}

	DisplayMode active = { 0, 0, 0.0 };
	if (EnumDisplaySettingsW(
		  monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &deviceMode)) {
		active.width = deviceMode.dmPelsWidth;
		active.height = deviceMode.dmPelsHeight;
		active.refreshRate = static_cast<double>(deviceMode.dmDisplayFrequency);
	} else if (!modes.empty()) {
		active = *modes.begin();
	}

	RectI bounds(monitorInfo.rcMonitor.left,
				 monitorInfo.rcMonitor.top,
				 monitorInfo.rcMonitor.right,
				 monitorInfo.rcMonitor.bottom);

	out->insert(DisplaySpec("", "Fullscreen", modes, active, bounds));
	return TRUE;
}

void
LowLevelWindowVK_Win32::GetDisplaySpecs(DisplaySpecs& out) const
{
	EnumDisplayMonitors(
	  nullptr, nullptr, EnumerateMonitors, reinterpret_cast<LPARAM>(&out));
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
