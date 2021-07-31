#include "GraphicsWindow.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "Core/Misc/AppInfo.hpp"
#include "arch/InputHandler/InputHandler_DirectInput.h"
#include "archutils/Win32/AppInstance.h"

static const std::string g_sClassName = Core::AppInfo::APP_TITLE;

static HWND g_hWndMain;
static VideoModeParams g_CurrentParams;
static ActualVideoModeParams g_ActualParams;
static HICON g_hIcon = nullptr;

HWND GraphicsWindow::GetHwnd() { return g_hWndMain; }
void GraphicsWindow::SetHwnd(HWND hwnd){ g_hWndMain = hwnd; }

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
