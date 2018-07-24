#include "global.h"
#include "RageUtil.h"

#include "LoadingWindow_Win32.h"
#include "RageFileManager.h"
#include "archutils/win32/WindowsResources.h"
#include "archutils/win32/WindowIcon.h"
#include "archutils/win32/ErrorStrings.h"
#include "arch/ArchHooks/ArchHooks.h"
#include <windows.h>
#include <Commdlg.h>
#include <tchar.h>
#include "CommCtrl.h"
#include "RageSurface_Load.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageLog.h"
#include <wchar.h>
#include "ProductInfo.h"
#include "LocalizedString.h"

#include "RageSurfaceUtils_Zoom.h"
static HBITMAP g_hBitmap = NULL;

RString text[3];
const float FONT_HEIGHT = 12;
const string FONT_FILE = "Data/Roboto-Light.ttf";
const string FONT_NAME = "Roboto Light";
const auto FONT_COLOR = RGB(240, 240, 240);
const int FONT_Y = 98;
const int FONT_X = 20;

/* Load a RageSurface into a GDI surface. */
static HBITMAP LoadWin32Surface( const RageSurface *pSplash, HWND hWnd )
{
	RageSurface *s = CreateSurface( pSplash->w, pSplash->h, 32,
		0xFF000000, 0x00FF0000, 0x0000FF00, 0 );
	RageSurfaceUtils::Blit( pSplash, s , -1, -1 );
	RECT rOld;
	GetClientRect(hWnd, &rOld);
	SetWindowPos(hWnd, 0, rOld.left, rOld.top, s->w, s->h, SWP_NOMOVE);

	/* Resize the splash image to fit the dialog.  Stretch to fit horizontally,
	 * maintaining aspect ratio. */
	{
		RECT r;
		GetClientRect( hWnd, &r );

		int iWidth = r.right;
		float fRatio = (float) iWidth / s->w;
		int iHeight = lround( s->h * fRatio );

		RageSurfaceUtils::Zoom( s, iWidth, iHeight );
	}

	HDC hScreen = GetDC(NULL);
	ASSERT_M( hScreen != NULL, werr_ssprintf(GetLastError(), "hScreen") );

	HBITMAP bitmap = CreateCompatibleBitmap( hScreen, s->w, s->h );
	ASSERT_M( bitmap != NULL, werr_ssprintf(GetLastError(), "CreateCompatibleBitmap") );

	HDC BitmapDC = CreateCompatibleDC( hScreen );
	SelectObject( BitmapDC, bitmap );

	/* This is silly, but simple.  We only do this once, on a small image. */
	for( int y = 0; y < s->h; ++y )
	{
		unsigned const char *line = ((unsigned char *) s->pixels) + (y * s->pitch);
		for( int x = 0; x < s->w; ++x )
		{
			unsigned const char *data = line + (x*s->fmt.BytesPerPixel);
			
			SetPixelV( BitmapDC, x, y, RGB( data[3], data[2], data[1] ) );
		}
	}

	SelectObject( BitmapDC, NULL );
	DeleteObject( BitmapDC );

	ReleaseDC( NULL, hScreen );

	delete s;
	return bitmap;
}

static HBITMAP LoadWin32Surface( RString sFile, HWND hWnd )
{
        RString error;
        RageSurface *pSurface = RageSurfaceUtils::LoadFile( sFile, error );
        if( pSurface == NULL )
                return NULL;

        HBITMAP ret = LoadWin32Surface( pSurface, hWnd );
        delete pSurface;
        return ret;
}

BOOL CALLBACK LoadingWindow_Win32::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	case WM_INITDIALOG:
		{
			vector<RString> vs;
			GetDirListing( "Data/splash*.png", vs, false, true );
			if( !vs.empty() )
				g_hBitmap = LoadWin32Surface( vs[0], hWnd );
		}
		if( g_hBitmap == NULL )
			g_hBitmap = LoadWin32Surface( "Data/splash.bmp", hWnd );
		SendMessage( 
			GetDlgItem(hWnd,IDC_SPLASH), 
			STM_SETIMAGE, 
			(WPARAM) IMAGE_BITMAP, 
			(LPARAM) (HANDLE) g_hBitmap );
		SetWindowTextA( hWnd, PRODUCT_ID );
		break;

	case WM_DESTROY:
		DeleteObject( g_hBitmap );
		g_hBitmap = NULL;
		break;
    case WM_PAINT:
		for (unsigned i = 0; i < 3; ++i)
		{
			//Do graphical paint
			RECT rect;
			HDC wdc = GetWindowDC(hWnd);
			GetClientRect(hWnd, &rect);
			SetTextColor(wdc, FONT_COLOR);
			SetBkMode(wdc, TRANSPARENT);
			rect.left = FONT_X;
			rect.top = FONT_Y + (FONT_HEIGHT+3) * i;

			LOGFONT lf;
			memset(&lf, 0, sizeof(lf));
			lf.lfHeight = -MulDiv(FONT_HEIGHT, GetDeviceCaps(wdc, LOGPIXELSY), 72);
			_tcscpy(lf.lfFaceName, FONT_NAME.c_str()); //we must include tchar.h
			auto f = CreateFontIndirect(&lf);
			SendMessage(hWnd, WM_SETFONT, (WPARAM)f, MAKELPARAM(FALSE, 0));
			SelectObject(wdc, f);

			DrawText(wdc, text[i].c_str(), -1, &rect, DT_SINGLELINE | DT_NOCLIP);
			DeleteDC(wdc);
			//::SetWindowText( hwndItem, ConvertUTF8ToACP(asMessageLines[i]).c_str() );
		}
	}

	return FALSE;
}

void LoadingWindow_Win32::SetIcon( const RageSurface *pIcon )
{
	if( m_hIcon != NULL )
		DestroyIcon( m_hIcon );

	m_hIcon = IconFromSurface( pIcon );
	if( m_hIcon != NULL )
		// XXX: GCL_HICON isn't available on x86-64 Windows
		SetClassLong( hwnd, GCL_HICON, (LONG) m_hIcon );
}

void LoadingWindow_Win32::SetSplash( const RageSurface *pSplash )
{
	if( g_hBitmap != NULL )
	{
		DeleteObject( g_hBitmap );
		g_hBitmap = NULL;
	}

	g_hBitmap = LoadWin32Surface( pSplash, hwnd );
	if( g_hBitmap != NULL )
	{
		SendDlgItemMessage(
			hwnd, IDC_SPLASH,
			STM_SETIMAGE,
			(WPARAM) IMAGE_BITMAP,
			(LPARAM) (HANDLE) g_hBitmap
		);
	}
}

LoadingWindow_Win32::LoadingWindow_Win32()
{
	string szFontFile = RageFileManagerUtil::sDirOfExecutable.substr(0, RageFileManagerUtil::sDirOfExecutable.length()-7) + FONT_FILE;

	int nResults = AddFontResourceEx(
		szFontFile.c_str(), 		// font file name
		FR_PRIVATE,    	// font characteristics
		NULL);

	m_hIcon = NULL;
	hwnd = CreateDialog( handle.Get(), MAKEINTRESOURCE(IDD_LOADING_DIALOG), NULL, WndProc );
	ASSERT( hwnd != NULL );
	for( unsigned i = 0; i < 3; ++i )
		text[i] = "ABC"; /* always set on first call */
	SetText( "" );
	Paint();
}

LoadingWindow_Win32::~LoadingWindow_Win32()
{
	if( hwnd )
		DestroyWindow( hwnd );
	if( m_hIcon != NULL )
		DestroyIcon( m_hIcon );
}

void LoadingWindow_Win32::Paint()
{
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
	SendMessage(hwnd, WM_PAINT, 0, 0);

	/* Process all queued messages since the last paint.  This allows the window to
	 * come back if it loses focus during load. */
	MSG msg;
	while( PeekMessage( &msg, hwnd, 0, 0, PM_NOREMOVE ) )
	{
		GetMessage(&msg, hwnd, 0, 0 );
		DispatchMessage( &msg );
	}
}

void LoadingWindow_Win32::SetText(const RString &sText)
{
	lastText = sText;
	SetTextInternal();
	Paint();
}

void LoadingWindow_Win32::SetTextInternal()
{
	if (m_indeterminate)
		progress = "";
	else {
		int percent = m_totalWork != 0 ? ((float)m_progress) / ((float)m_totalWork) * 100 : m_progress;
		progress = " ("+to_string(percent) + "%)";
	}
	RString& sText = lastText;

	vector<RString> asMessageLines;
	split( sText, "\n", asMessageLines, false );
	while( asMessageLines.size() < 3 )
		asMessageLines.push_back( "" );

	const int msgid[] = { IDC_STATIC_MESSAGE1, IDC_STATIC_MESSAGE2, IDC_STATIC_MESSAGE3 };
	for( unsigned i = 0; i < 3; ++i )
	{
		if( text[i] == asMessageLines[i] )
			continue;
		text[i] = asMessageLines[i];
	}
	for (unsigned i = 0; i < 3; ++i)
		if (text[i] != "") {
			text[i] += progress;
			break;
		}
}

void LoadingWindow_Win32::SetProgress(const int progress)
{
	m_progress=progress;
	//HWND hwndItem = ::GetDlgItem( hwnd, IDC_PROGRESS );
	//::SendMessage(hwndItem,PBM_SETPOS,progress,0);
	SetTextInternal();
	Paint();
}

void LoadingWindow_Win32::SetTotalWork(const int totalWork)
{
	m_totalWork=totalWork;
	//HWND hwndItem = ::GetDlgItem( hwnd, IDC_PROGRESS );
	//SendMessage(hwndItem,PBM_SETRANGE32,0,totalWork);
	SetTextInternal();
	Paint();
}

void LoadingWindow_Win32::SetIndeterminate(bool indeterminate)
{
	m_indeterminate=indeterminate;
	SetTextInternal();

	/*
	HWND hwndItem = ::GetDlgItem( hwnd, IDC_PROGRESS );

	if(indeterminate) {
		SetWindowLong(hwndItem,GWL_STYLE, PBS_MARQUEE | GetWindowLong(hwndItem,GWL_STYLE));
		SendMessage(hwndItem,PBM_SETMARQUEE,1,0);
	} else {
		SendMessage(hwndItem,PBM_SETMARQUEE,0,0);
		SetWindowLong(hwndItem,GWL_STYLE, (~PBS_MARQUEE) & GetWindowLong(hwndItem,GWL_STYLE));
	}
	*/
	Paint();
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
