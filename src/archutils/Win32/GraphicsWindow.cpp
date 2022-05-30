#include "Etterna/Globals/global.h"
#include "GraphicsWindow.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "Etterna/Models/Misc/DisplaySpec.h"
#include "Etterna/Globals/GameLoop.h"
#include "Core/Services/Locator.hpp"
#include "Core/Misc/AppInfo.hpp"
#include "arch/InputHandler/InputHandler_DirectInput.h"
#include "archutils/Win32/AppInstance.h"
#include "archutils/Win32/ErrorStrings.h"
#include "archutils/Win32/WindowIcon.h"
#include "archutils/Win32/GetFileInformation.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/CommandLineActions.h"
#include "Etterna/Singletons/InputFilter.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "RageUtil/Misc/RageInput.h"

#include <set>
#include <dbt.h>

static const std::string g_sClassName = Core::AppInfo::APP_TITLE;

static HWND g_hWndMain;
static HDC g_HDC;
static VideoModeParams g_CurrentParams;
static ActualVideoModeParams g_ActualParams;
static bool g_bResolutionChanged = false;
static bool g_bHasFocus = true;
static HICON g_hIcon = nullptr;
static bool m_bWideWindowClass;
static bool g_bD3D = false;

// If we're fullscreen, this is the mode we set.
static DEVMODE g_FullScreenDevMode;
static bool g_bRecreatingVideoMode = false;

static UINT g_iQueryCancelAutoPlayMessage = 0;

static std::string
GetNewWindow()
{
	HWND h = GetForegroundWindow();
	if (h == nullptr)
		return "(NULL)";

	DWORD iProcessID;
	GetWindowThreadProcessId(h, &iProcessID);

	std::string sName;
	GetProcessFileName(iProcessID, sName);

	sName = Basename(sName);

	return sName;
}

static LRESULT CALLBACK
GraphicsWindow_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Suppress autorun.
	if (msg == g_iQueryCancelAutoPlayMessage)
		return true;

	switch (msg) {
		case WM_ACTIVATE: {
			const bool bInactive = (LOWORD(wParam) == WA_INACTIVE);
			const bool bMinimized = (HIWORD(wParam) != 0);
			const bool bHadFocus = g_bHasFocus;
			g_bHasFocus = !bInactive && !bMinimized;
			if (PREFSMAN != nullptr)
				Locator::getLogger()->debug("WM_ACTIVATE ({}, {}): {}",
						   bInactive,
						   bMinimized,
						   g_bHasFocus ? "has focus" : "doesn't have focus");
			if (!g_bHasFocus) {
				std::string sName = GetNewWindow();
				static std::set<std::string> sLostFocusTo;
				sLostFocusTo.insert(sName);
				std::string sStr;
				for (std::set<std::string>::const_iterator it =
					   sLostFocusTo.begin();
					 it != sLostFocusTo.end();
					 ++it)
					sStr += (!sStr.empty() ? ", " : "") + *it;

				if (PREFSMAN != nullptr)
                    Locator::getLogger()->debug("Lost focus to: {}", sStr);
			}

			if (!g_bD3D && !g_CurrentParams.windowed &&
				!g_bRecreatingVideoMode) {
				/* In OpenGL (not D3D), it's our job to unset and reset the
				 * full-screen video mode when we focus changes, and to hide
				 * and show the window. Hiding is done in WM_KILLFOCUS,
				 * because that's where most other apps seem to do it. */
				if (g_bHasFocus && !bHadFocus) {
					ChangeDisplaySettings(&g_FullScreenDevMode, CDS_FULLSCREEN);
					ShowWindow(g_hWndMain, SW_SHOWNORMAL);
				} else if (!g_bHasFocus && bHadFocus) {
					ChangeDisplaySettings(nullptr, 0);
				}
			}

			return 0;
		}
		case WM_KILLFOCUS:
			if (!g_bD3D && !g_CurrentParams.windowed && !g_bRecreatingVideoMode)
				ShowWindow(g_hWndMain, SW_SHOWMINNOACTIVE);
			break;

			/* Is there any reason we should care what size the user resizes
			 * the window to? (who? -aj)
			 * Short answer: yes. -aj */
			//	case WM_GETMINMAXINFO:

		case WM_SETCURSOR:
			if (!g_CurrentParams.windowed) {
				SetCursor(nullptr);
				return 1;
			}
			break;

		case WM_SYSCOMMAND:
			switch (wParam & 0xFFF0) {
				case SC_MONITORPOWER:
				case SC_SCREENSAVE:
					return 0;
			}
			break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;
		}

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_MOUSEWHEEL: // might want to use this for
							// GET_WHEEL_DELTA_WPARAM(wParam) -aj
			// We handle all input ourself, via DirectInput.
			return 0;

		case WM_CLOSE:
			Locator::getLogger()->info("WM_CLOSE: shutting down");
			GameLoop::setUserQuit();
			return 0;

		case WM_WINDOWPOSCHANGED: {
			/* If we're fullscreen and don't have focus, our window is hidden,
			 * so GetClientRect isn't meaningful. */
			if (!g_CurrentParams.windowed && !g_bHasFocus)
				break;

			RECT rect;
			GetClientRect(hWnd, &rect);

			int iWidth = rect.right - rect.left;
			int iHeight = rect.bottom - rect.top;
			if (g_CurrentParams.width != iWidth ||
				g_CurrentParams.height != iHeight) {
				g_CurrentParams.width = iWidth;
				g_CurrentParams.height = iHeight;
				g_bResolutionChanged = true;
			}
			g_ActualParams = ActualVideoModeParams(g_CurrentParams);
			break;
		}
		case WM_COPYDATA: {
			PCOPYDATASTRUCT pMyCDS = (PCOPYDATASTRUCT)lParam;
			std::string sCommandLine((char*)pMyCDS->lpData, pMyCDS->cbData);
			CommandLineActions::CommandLineArgs args;
			split(sCommandLine, "|", args.argv, false);
			CommandLineActions::ToProcess.push_back(args);
			break;
		}
		case WM_DEVICECHANGE: {
			switch (wParam) {
				case DBT_DEVICEARRIVAL: {
					DInput_ForceJoystickPollingInNextDevicesChangedCall();
					if (INPUTMAN->DevicesChanged()) {
						INPUTFILTER->Reset();
						INPUTMAN->LoadDrivers();
						std::string sMessage;
						if (INPUTMAPPER->CheckForChangedInputDevicesAndRemap(
							  sMessage))
							SCREENMAN->SystemMessage(sMessage);
					}
					break;
				}
			}
			break;
		}
	}

	if (m_bWideWindowClass)
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	else
		return DefWindowProcA(hWnd, msg, wParam, lParam);
}

static void
AdjustVideoModeParams(VideoModeParams& p)
{
	DEVMODE dm;
	ZERO(dm);
	dm.dmSize = sizeof(dm);
	if (!EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm)) {
		p.rate = 60;
		Locator::getLogger()->warn(werr_ssprintf(GetLastError(), "EnumDisplaySettings failed"));
		return;
	}

	/* On a nForce 2 IGP on Windows 98, dm.dmDisplayFrequency sometimes
	 * (but not always) is 0.
	 *
	 * MSDN: When you call the EnumDisplaySettings function, the
	 * dmDisplayFrequency member may return with the value 0 or 1.
	 * These values represent the display hardware's default refresh rate.
	 * This default rate is typically set by switches on a display card or
	 * computer motherboard, or by a configuration program that does not
	 * use Win32 display functions such as ChangeDisplaySettings. */
	if (!(dm.dmFields & DM_DISPLAYFREQUENCY) || dm.dmDisplayFrequency == 0 ||
		dm.dmDisplayFrequency == 1) {
		p.rate = 60;
		Locator::getLogger()->warn("EnumDisplaySettings doesn't know what the refresh rate is. {} {} {}",
		  dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel);
	} else {
		p.rate = dm.dmDisplayFrequency;
	}
}

/* Set the display mode to the given size, bit depth and refresh.
 * The refresh setting may be ignored. */
std::string
GraphicsWindow::SetScreenMode(const VideoModeParams& p)
{
	if (p.windowed) {
		// We're going windowed. If we were previously fullscreen, reset.
		ChangeDisplaySettings(nullptr, 0);

		return std::string();
	}

	DEVMODE DevMode;
	ZERO(DevMode);
	DevMode.dmSize = sizeof(DEVMODE);
	DevMode.dmPelsWidth = p.width;
	DevMode.dmPelsHeight = p.height;
	DevMode.dmBitsPerPel = p.bpp;
	DevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

	if (p.rate != REFRESH_DEFAULT) {
		DevMode.dmDisplayFrequency = p.rate;
		DevMode.dmFields |= DM_DISPLAYFREQUENCY;
	}
	ChangeDisplaySettings(nullptr, 0);

	int ret = ChangeDisplaySettings(&DevMode, CDS_FULLSCREEN);
	if (ret != DISP_CHANGE_SUCCESSFUL &&
		(DevMode.dmFields & DM_DISPLAYFREQUENCY)) {
		DevMode.dmFields &= ~DM_DISPLAYFREQUENCY;
		ret = ChangeDisplaySettings(&DevMode, CDS_FULLSCREEN);
	}

	// XXX: append error
	if (ret != DISP_CHANGE_SUCCESSFUL)
		return "Couldn't set screen mode";

	g_FullScreenDevMode = DevMode;
	return std::string();
}

static int
GetWindowStyle(bool bWindowed, bool bBorderless)
{
	if (bWindowed && !bBorderless)
		return WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	else
		return WS_POPUP;
}

/* Set the final window size, set the window text and icon, and then unhide the
 * window. */
void
GraphicsWindow::CreateGraphicsWindow(const VideoModeParams& p,
									 bool bForceRecreateWindow)
{
	g_CurrentParams = p;

	// Adjust g_CurrentParams to reflect the actual display settings.
	AdjustVideoModeParams(g_CurrentParams);

	if (g_hWndMain == nullptr || bForceRecreateWindow) {
		int iWindowStyle =
		  GetWindowStyle(p.windowed, p.bWindowIsFullscreenBorderless);

		AppInstance inst;
		HWND hWnd = CreateWindow(g_sClassName.c_str(),
								 "app",
								 iWindowStyle,
								 0,
								 0,
								 0,
								 0,
								 NULL,
								 NULL,
								 inst,
								 NULL);
		if (hWnd == nullptr)
			RageException::Throw(
			  "%s", werr_ssprintf(GetLastError(), "CreateWindow").c_str());

		/* If an old window exists, transfer focus to the new window before
		 * deleting it, or some other window may temporarily get focus, which
		 * can cause it to be resized. */
		if (g_hWndMain != nullptr) {
			// While we change to the new window, don't do ChangeDisplaySettings
			// in WM_ACTIVATE.
			g_bRecreatingVideoMode = true;
			SetForegroundWindow(hWnd);
			g_bRecreatingVideoMode = false;

			GraphicsWindow::DestroyGraphicsWindow();
		}

		g_hWndMain = hWnd;
//		CrashHandler::SetForegroundWindow(g_hWndMain);
		g_HDC = GetDC(g_hWndMain);
	}

	// Update the window title.
	do {
		if (m_bWideWindowClass) {
			if (SetWindowText(g_hWndMain,
							  ConvertUTF8ToACP(p.sWindowTitle).c_str()))
				break;
		}

		SetWindowTextA(g_hWndMain, ConvertUTF8ToACP(p.sWindowTitle).c_str());
	} while (0);

	// Update the window icon.
	if (g_hIcon != nullptr) {
#if _WIN64
		SetClassLongPtr(
		  g_hWndMain, GCLP_HICON, (LONG_PTR)LoadIcon(nullptr, IDI_APPLICATION));
#else
		SetClassLong(
		  g_hWndMain, GCL_HICON, (LONG)LoadIcon(NULL, IDI_APPLICATION));
#endif
		DestroyIcon(g_hIcon);
		g_hIcon = nullptr;
	}
	g_hIcon = IconFromFile(p.sIconFile);
	if (g_hIcon != nullptr)
#if _WIN64
		SetClassLongPtr(g_hWndMain, GCLP_HICON, (LONG_PTR)g_hIcon);
#else
		SetClassLong(g_hWndMain, GCL_HICON, (LONG)g_hIcon);
#endif

	/* The window style may change as a result of switching to or from
	 * fullscreen; apply it. Don't change the WS_VISIBLE bit. */
	int iWindowStyle =
	  GetWindowStyle(p.windowed, p.bWindowIsFullscreenBorderless);
	if (GetWindowLong(g_hWndMain, GWL_STYLE) & WS_VISIBLE)
		iWindowStyle |= WS_VISIBLE;
	SetWindowLong(g_hWndMain, GWL_STYLE, iWindowStyle);

	RECT WindowRect;
	SetRect(&WindowRect, 0, 0, p.width, p.height);
	AdjustWindowRect(&WindowRect, iWindowStyle, FALSE);

	// LOG->Warn( "w = %d, h = %d", p.width, p.height );

	const int iWidth = WindowRect.right - WindowRect.left;
	const int iHeight = WindowRect.bottom - WindowRect.top;

	// If windowed, center the window.
	int x = 0, y = 0;
	if (p.windowed) {
		x = GetSystemMetrics(SM_CXSCREEN) / 2 - iWidth / 2;
		y = GetSystemMetrics(SM_CYSCREEN) / 2 - iHeight / 2;
	}

	/* Move and resize the window. SWP_FRAMECHANGED causes the above
	 * SetWindowLong to take effect. */
	if (!SetWindowPos(g_hWndMain,
					  HWND_NOTOPMOST,
					  x,
					  y,
					  iWidth,
					  iHeight,
					  SWP_FRAMECHANGED | SWP_SHOWWINDOW))
		Locator::getLogger()->warn(werr_ssprintf(GetLastError(), "SetWindowPos"));

	SetForegroundWindow(g_hWndMain);

	/* Pump messages quickly, to make sure the window is completely set up.
	 * If we don't do this, then starting up in a D3D fullscreen window may
	 * cause all other windows on the system to be resized. */
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
		GetMessage(&msg, nullptr, 0, 0);
		DispatchMessage(&msg);
	}
	g_ActualParams = ActualVideoModeParams(g_CurrentParams);
}

/** @brief Shut down the window, but don't reset the video mode. */
void
GraphicsWindow::DestroyGraphicsWindow()
{
	if (g_HDC != nullptr) {
		ReleaseDC(g_hWndMain, g_HDC);
		g_HDC = nullptr;
	}
	if (g_hWndMain != nullptr) {
		DestroyWindow(g_hWndMain);
		g_hWndMain = nullptr;
//		CrashHandler::SetForegroundWindow(g_hWndMain);
	}

	if (g_hIcon != nullptr) {
		DestroyIcon(g_hIcon);
		g_hIcon = nullptr;
	}

	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
		GetMessage(&msg, nullptr, 0, 0);
		DispatchMessage(&msg);
	}

}

void
GraphicsWindow::Initialize(bool bD3D)
{
	// A few things need to be handled differently for D3D.
	g_bD3D = bD3D;

	AppInstance inst;
	do {
		const std::wstring wsClassName = StringToWString(g_sClassName);
		WNDCLASSW WindowClassW = {
			CS_OWNDC | CS_BYTEALIGNCLIENT,
			GraphicsWindow_WndProc,
			0,								/* cbClsExtra */
			0,								/* cbWndExtra */
			inst,							/* hInstance */
			nullptr,						/* set icon later */
			LoadCursor(nullptr, IDC_ARROW), /* default cursor */
			nullptr,						/* hbrBackground */
			nullptr,						/* lpszMenuName */
			wsClassName.c_str()				/* lpszClassName */
		};

		m_bWideWindowClass = true;
		if (RegisterClassW(&WindowClassW))
			break;

		WNDCLASS WindowClassA = {
			CS_OWNDC | CS_BYTEALIGNCLIENT,
			GraphicsWindow_WndProc,
			0,								/* cbClsExtra */
			0,								/* cbWndExtra */
			inst,							/* hInstance */
			nullptr,						/* set icon later */
			LoadCursor(nullptr, IDC_ARROW), /* default cursor */
			nullptr,						/* hbrBackground */
			nullptr,						/* lpszMenuName */
			g_sClassName.c_str()			/* lpszClassName */
		};

		m_bWideWindowClass = false;
		if (!RegisterClassA(&WindowClassA))
			RageException::Throw(
			  "%s", werr_ssprintf(GetLastError(), "RegisterClass").c_str());
	} while (0);

	g_iQueryCancelAutoPlayMessage =
	  RegisterWindowMessage("QueryCancelAutoPlay");
}

void
GraphicsWindow::Shutdown()
{
	DestroyGraphicsWindow();

	/* Return to the desktop resolution, if needed.
	 * It'd be nice to not do this: Windows will do it when we quit, and if
	 * we're shutting down OpenGL to try D3D, this will cause extra mode
	 * switches. However, we need to do this before displaying dialogs. */
	ChangeDisplaySettings(nullptr, 0);

	AppInstance inst;
	UnregisterClass(g_sClassName.c_str(), inst);
}

HDC
GraphicsWindow::GetHDC()
{
	ASSERT(g_HDC != NULL);
	return g_HDC;
}

ActualVideoModeParams*
GraphicsWindow::GetParams()
{
	return &g_ActualParams;
}

void
GraphicsWindow::Update()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
		GetMessage(&msg, nullptr, 0, 0);
		DispatchMessage(&msg);
	}

	GameLoop::setGameFocused(g_bHasFocus);

	if (g_bResolutionChanged && DISPLAY != nullptr) {
		// LOG->Warn( "Changing resolution" );

		/* Let DISPLAY know that our resolution has changed. (Note that
		 * ResolutionChanged() can come back here, so reset g_bResolutionChanged
		 * first.) */
		g_bResolutionChanged = false;
		DISPLAY->ResolutionChanged();
	}
}

HWND
GraphicsWindow::GetHwnd()
{
	return g_hWndMain;
}

void
GraphicsWindow::GetDisplaySpecs(DisplaySpecs& out)
{
	const size_t DM_DRIVER_EXTRA_BYTES = 4096;
	const size_t DMSIZE = sizeof(DEVMODE) + DM_DRIVER_EXTRA_BYTES;
	auto reset = [=](std::unique_ptr<DEVMODE>& p) {
		::memset(p.get(), 0, DMSIZE);
		p->dmSize = sizeof(DEVMODE);
		p->dmDriverExtra = static_cast<WORD>(DM_DRIVER_EXTRA_BYTES);
	};
	auto isvalid = [](std::unique_ptr<DEVMODE>& dm) {
		// Windows 8 and later don't support less than 32bpp, so don't even test
		// for them.
		return (dm->dmFields & DM_PELSWIDTH) &&
			   (dm->dmFields & DM_PELSHEIGHT) &&
			   (dm->dmFields & DM_DISPLAYFREQUENCY) &&
			   (dm->dmBitsPerPel >= 32 || !(dm->dmFields & DM_BITSPERPEL));
	};

	std::unique_ptr<DEVMODE> dm(static_cast<DEVMODE*>(operator new(DMSIZE)));
	reset(dm);

	int i = 0;
	std::set<DisplayMode> modes;
	while (EnumDisplaySettingsEx(nullptr, i++, dm.get(), 0)) {
		if (isvalid(dm) && ChangeDisplaySettingsEx(
							 nullptr, dm.get(), nullptr, CDS_TEST, nullptr) ==
							 DISP_CHANGE_SUCCESSFUL) {
			DisplayMode m = { dm->dmPelsWidth,
							  dm->dmPelsHeight,
							  static_cast<double>(dm->dmDisplayFrequency) };
			modes.insert(m);
		}
		reset(dm);
	}
	reset(dm);
	// Get the current display mode
	if (EnumDisplaySettingsEx(nullptr, ENUM_CURRENT_SETTINGS, dm.get(), 0) &&
		isvalid(dm)) {
		DisplayMode m = { dm->dmPelsWidth,
						  dm->dmPelsHeight,
						  static_cast<double>(dm->dmDisplayFrequency) };
		RectI bounds = {
			0, 0, static_cast<int>(m.width), static_cast<int>(m.height)
		};
		out.insert(DisplaySpec("", "Fullscreen", modes, m, bounds));
	} else if (!modes.empty()) {
		Locator::getLogger()->warn("Could not retrieve valid current display mode");
		out.insert(DisplaySpec("", "Fullscreen", *modes.begin()));
	} else {
		Locator::getLogger()->warn("Could not retrieve *any* DisplaySpecs!");
	}
}

BOOL
GraphicsWindow::PushWindow(int a, int b)
{
	HWND g = GetHwnd();

	if (!g_ActualParams.windowed)
		return 0;

	RECT r;
	GetWindowRect(g, &r);
	// The immediately below is for "Aero Glass"
	// DwmGetWindowAttribute(g, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(r));

	int x = r.left + a;
	int y = r.top + b;

	return SetWindowPos(
	  g, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/*
 * (c) 2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
