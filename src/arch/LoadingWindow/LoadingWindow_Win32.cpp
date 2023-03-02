#include "Etterna/Globals/global.h"
#include "RageUtil/Utils/RageUtil.h"
#include "LoadingWindow_Win32.h"
#include "RageUtil/File/RageFileManager.h"
#include "archutils/Win32/WindowsResources.h"
#include "archutils/Win32/WindowIcon.h"
#include "archutils/Win32/ErrorStrings.h"
#include <windows.h>
#include <wchar.h>
#include <Commdlg.h>
#include <tchar.h>
#include "Dwmapi.h"
#include "RageUtil/Graphics/RageSurface_Load.h"
#include "RageUtil/Graphics/RageSurface.h"
#include "RageUtil/Graphics/RageSurfaceUtils.h"
#include "RageUtil/Graphics/RageSurfaceUtils_Zoom.h"
#include "Core/Misc/AppInfo.hpp"

#pragma comment(lib, "Dwmapi.lib")

static HBITMAP g_hBitmap = nullptr;

std::string text[3];
const float FONT_HEIGHT = 12;
const std::string FONT_FILE = "Data/Roboto-Light.ttf";
const std::string FONT_NAME = "Roboto Light";
const auto FONT_COLOR = RGB(240, 240, 240);
const int FONT_Y = 98;
const int FONT_X = 20;
LoadingWindow_Win32* w;

/* Load a RageSurface into a GDI surface. */
static HBITMAP
LoadWin32Surface(const RageSurface* pSplash, HWND hWnd)
{
	RageSurface* s = CreateSurface(
	  pSplash->w, pSplash->h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0);
	RageSurfaceUtils::Blit(pSplash, s, -1, -1);
	RECT wrect;
	GetWindowRect(hWnd, &wrect);
	DwmGetWindowAttribute(
	  hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &wrect, sizeof(wrect));
	RECT rOld;
	GetClientRect(hWnd, &rOld);
	rOld.left = (rOld.right / 2) - (pSplash->w / 2);
	rOld.top = (rOld.bottom / 2) - (pSplash->h / 2);
	wrect.left += ((wrect.right - wrect.left) - s->w) / 2;
	wrect.top += ((wrect.bottom - wrect.top) - s->h) / 2;
	SetWindowPos(hWnd, nullptr, wrect.left, wrect.top, s->w, s->h, 0);

	/* Resize the splash image to fit the dialog.  Stretch to fit horizontally,
	 * maintaining aspect ratio. */
	{
		RECT r;
		GetClientRect(hWnd, &r);

		int iWidth = r.right;
		float fRatio = static_cast<float>(iWidth) / s->w;
		int iHeight = lround(s->h * fRatio);

		RageSurfaceUtils::Zoom(s, iWidth, iHeight);
	}

	HDC hScreen = GetDC(nullptr);
	ASSERT_M(hScreen != NULL, werr_ssprintf(GetLastError(), "hScreen"));

	HBITMAP bitmap = CreateCompatibleBitmap(hScreen, s->w, s->h);
	ASSERT_M(bitmap != NULL,
			 werr_ssprintf(GetLastError(), "CreateCompatibleBitmap"));

	HDC BitmapDC = CreateCompatibleDC(hScreen);
	SelectObject(BitmapDC, bitmap);

	/* This is silly, but simple.  We only do this once, on a small image. */
	for (int y = 0; y < s->h; ++y) {
		unsigned const char* line =
		  ((unsigned char*)s->pixels) + (y * s->pitch);
		for (int x = 0; x < s->w; ++x) {
			unsigned const char* data = line + (x * s->fmt.BytesPerPixel);

			SetPixelV(BitmapDC, x, y, RGB(data[3], data[2], data[1]));
		}
	}

	SelectObject(BitmapDC, nullptr);
	DeleteObject(BitmapDC);

	ReleaseDC(nullptr, hScreen);

	delete s;
	return bitmap;
}

static HBITMAP
LoadWin32Surface(std::string sFile, HWND hWnd)
{
	std::string error;
	RageSurface* pSurface = RageSurfaceUtils::LoadFile(sFile, error);
	if (pSurface == nullptr)
		return nullptr;

	HBITMAP ret = LoadWin32Surface(pSurface, hWnd);
	delete pSurface;
	return ret;
}

INT_PTR CALLBACK
LoadingWindow_Win32::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG: {
			std::vector<std::string> vs;
			FILEMAN->GetDirListing("Data/splash*.png", vs, ONLY_FILE, true);
			if (!vs.empty())
				g_hBitmap = LoadWin32Surface(vs[0], hWnd);
		}
			if (g_hBitmap == nullptr)
				g_hBitmap = LoadWin32Surface("Data/splash.bmp", hWnd);
			SendMessage(GetDlgItem(hWnd, IDC_SPLASH),
						STM_SETIMAGE,
						(WPARAM)IMAGE_BITMAP,
						(LPARAM)(HANDLE)g_hBitmap);
			SetWindowTextA(hWnd, Core::AppInfo::APP_TITLE);
			break;

		case WM_DESTROY:
			DeleteObject(g_hBitmap);
			g_hBitmap = nullptr;
			break;
		case WM_PAINT:
			w->InternalPaint();
			break;
	}
	return FALSE;
}

void
LoadingWindow_Win32::SetIcon(const RageSurface* pIcon)
{
	if (m_hIcon != nullptr)
		DestroyIcon(m_hIcon);

	m_hIcon = IconFromSurface(pIcon);
	if (m_hIcon != nullptr)
	// XXX: GCL_HICON isn't available on x86-64 Windows
#if _WIN64
		SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)m_hIcon);
#else
		SetClassLong(hwnd, GCL_HICON, (LONG)m_hIcon);
#endif
}

void
LoadingWindow_Win32::SetSplash(const RageSurface* pSplash)
{
	if (g_hBitmap != nullptr) {
		DeleteObject(g_hBitmap);
		g_hBitmap = nullptr;
	}

	g_hBitmap = LoadWin32Surface(pSplash, hwnd);
	if (g_hBitmap != nullptr) {
		SendDlgItemMessage(hwnd,
						   IDC_SPLASH,
						   STM_SETIMAGE,
						   (WPARAM)IMAGE_BITMAP,
						   (LPARAM)(HANDLE)g_hBitmap);
	}
}

LoadingWindow_Win32::LoadingWindow_Win32()
{
	w = this;

	// Load a provided font for clarity in the load window
	// if this font fails to load or is missing, the only
	// negative is just the wrong font being in the window.
	// Setting the FR_PRIVATE flag for the font will unload the font at program
	// termination.
	std::string szFontFile =
	  RageFileManagerUtil::sDirOfExecutable.substr(
		0, RageFileManagerUtil::sDirOfExecutable.length() - 7) +
	  FONT_FILE;
	AddFontResourceEx(szFontFile.c_str(), // font file name
									 FR_PRIVATE,		 // font flags
									 nullptr);

	m_hIcon = nullptr;
	hwnd = CreateDialog(
	  handle.Get(), MAKEINTRESOURCE(IDD_LOADING_DIALOG), NULL, WndProc);
	ASSERT(hwnd != NULL);
	for (unsigned i = 0; i < 3; ++i)
		text[i] = "ABC"; /* always set on first call */
	HDC wdc = GetWindowDC(hwnd);
	SetTextColor(wdc, FONT_COLOR);
	SetBkMode(wdc, TRANSPARENT);
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight = -MulDiv(
	  static_cast<int>(FONT_HEIGHT), GetDeviceCaps(wdc, LOGPIXELSY), 72);
	lf.lfQuality = NONANTIALIASED_QUALITY;
	lf.lfClipPrecision = CLIP_TT_ALWAYS;
	_tcscpy(lf.lfFaceName, FONT_NAME.c_str()); // we must include tchar.h
	f = CreateFontIndirect(&lf);
	SendMessage(hwnd, WM_SETFONT, (WPARAM)f, MAKELPARAM(FALSE, 0));
	SelectObject(wdc, f);
	RECT rect;
	GetClientRect(hwnd, &rect);
	hdcBG = CreateCompatibleDC(wdc);
	bitMapBG = CreateCompatibleBitmap(wdc, rect.right, rect.bottom);
	SelectObject(hdcBG, bitMapBG);
	PrintWindow(hwnd, hdcBG, 0);
	DeleteDC(wdc);
	SetText("");
	// Do graphical paint
	Paint();
}

LoadingWindow_Win32::~LoadingWindow_Win32()
{
	if (hwnd)
		DestroyWindow(hwnd);
	if (m_hIcon != nullptr)
		DestroyIcon(m_hIcon);
	if (f)
		DeleteObject(f);
}

void
LoadingWindow_Win32::InternalPaint()
{
	static int asd = 0;
	asd++;
	RECT rect;
	HDC hDC = GetDC(hwnd);
	GetClientRect(hwnd, &rect);
	HDC hDCMem = CreateCompatibleDC(hDC);
	HBITMAP hBitmap =
	  (HBITMAP)CopyImage(bitMapBG, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
	// HBITMAP hBitmap = CreateCompatibleBitmap(hDC, rect.right, rect.bottom);
	SelectObject(hDCMem, hBitmap);
	PrintWindow(hwnd, hDCMem, 0);
	RECT textRect;
	GetClientRect(hwnd, &textRect);
	textRect.left = FONT_X;
	SetTextColor(hDCMem, FONT_COLOR);
	SetBkMode(hDCMem, TRANSPARENT);
	auto oldF = SelectObject(hDCMem, f);
	for (unsigned i = 0; i < 3; ++i) {
		textRect.top = static_cast<long>(FONT_Y + (FONT_HEIGHT + 3) * i);
		TextOut(hDCMem,
				textRect.left,
				textRect.top,
				text[i].c_str(),
				text[i].length());
		//::SetWindowText( hwndItem, ConvertUTF8ToACP(asMessageLines[i]).c_str()
		//);
	}
	BitBlt(hDC, 0, 0, rect.right, rect.bottom, hDCMem, 0, 0, SRCCOPY);
	SelectObject(hDCMem, oldF);
	DeleteObject(hBitmap);
	DeleteDC(hDCMem);
	ReleaseDC(hwnd, hDC);
}

void
LoadingWindow_Win32::Paint()
{
	/* Process all queued messages since the last paint.  This allows the window
	 * to come back if it loses focus during load. */
	MSG msg;
	while (PeekMessage(&msg, hwnd, 0, 0, PM_NOREMOVE)) {
		GetMessage(&msg, hwnd, 0, 0);
		DispatchMessage(&msg);
	}
	InternalPaint();
}

void
LoadingWindow_Win32::SetText(const std::string& sText)
{
	lastText = sText;
	SetTextInternal();
}

void
LoadingWindow_Win32::SetTextInternal()
{
	if (m_indeterminate)
		progress = "";
	else {
		int percent =
		  m_totalWork != 0 ? 100 * m_progress / m_totalWork : m_progress;
		progress = " (" + std::to_string(percent) + "%)";
	}
	std::string& sText = lastText;

	std::vector<std::string> asMessageLines;
	split(sText, "\n", asMessageLines, false);
	while (asMessageLines.size() < 3)
		asMessageLines.push_back("");

	for (unsigned i = 0; i < 3; ++i) {
		if (text[i] == asMessageLines[i])
			continue;
		text[i] = asMessageLines[i];
	}
	for (unsigned i = 0; i < 3; ++i)
		if (text[i] != "") {
			text[i] += progress;
			break;
		}
	Paint();
}

void
LoadingWindow_Win32::SetProgress(const int progress)
{
	m_progress = progress;
	// HWND hwndItem = ::GetDlgItem( hwnd, IDC_PROGRESS );
	//::SendMessage(hwndItem,PBM_SETPOS,progress,0);
	SetTextInternal();
}

void
LoadingWindow_Win32::SetTotalWork(const int totalWork)
{
	m_totalWork = totalWork;
	// HWND hwndItem = ::GetDlgItem( hwnd, IDC_PROGRESS );
	// SendMessage(hwndItem,PBM_SETRANGE32,0,totalWork);
	SetTextInternal();
}

void
LoadingWindow_Win32::SetIndeterminate(bool indeterminate)
{
	m_indeterminate = indeterminate;
	SetTextInternal();

	/*
	HWND hwndItem = ::GetDlgItem( hwnd, IDC_PROGRESS );
	if(indeterminate) {
	SetWindowLong(hwndItem,GWL_STYLE, PBS_MARQUEE |
	GetWindowLong(hwndItem,GWL_STYLE));
	SendMessage(hwndItem,PBM_SETMARQUEE,1,0);
	} else {
	SendMessage(hwndItem,PBM_SETMARQUEE,0,0);
	SetWindowLong(hwndItem,GWL_STYLE, (~PBS_MARQUEE) &
	GetWindowLong(hwndItem,GWL_STYLE));
	}
	*/
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
