/* LoadingWindow_Win32 - Loading window using a Windows dialog box. */

#ifndef LOADING_WINDOW_WIN32_H
#define LOADING_WINDOW_WIN32_H

#include "LoadingWindow.h"
#include <windows.h>
#include "archutils/Win32/AppInstance.h"

class LoadingWindow_Win32: public LoadingWindow
{
public:
	LoadingWindow_Win32();
	~LoadingWindow_Win32();

	void Paint();
	void SetText( const RString &sText );
	void SetTextInternal();
	void SetIcon( const RageSurface *pIcon );
	void SetSplash( const RageSurface *pSplash );
	void SetProgress( const int progress );
	void SetTotalWork( const int totalWork );
	void SetIndeterminate( bool indeterminate );

private:
	AppInstance handle;
	HWND hwnd;
	HICON m_hIcon;
	string progress;
	RString lastText;

	static BOOL CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
};
#define USE_LOADING_WINDOW_WIN32

#endif

/*
 * (c) 2002-2004 Glenn Maynard
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

