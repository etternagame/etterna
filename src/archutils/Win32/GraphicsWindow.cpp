#include "GraphicsWindow.h"
#include "RageUtil/Graphics/RageDisplay.h"

static HWND g_hWndMain;
static VideoModeParams g_CurrentParams;
static ActualVideoModeParams g_ActualParams;

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
