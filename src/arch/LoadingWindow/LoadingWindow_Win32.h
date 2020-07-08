/* LoadingWindow_Win32 - Loading window using a Windows dialog box. */

#ifndef LOADING_WINDOW_WIN32_H
#define LOADING_WINDOW_WIN32_H

#include "LoadingWindow.h"
#include <windows.h>
#include "archutils/Win32/AppInstance.h"

class LoadingWindow_Win32 : public LoadingWindow
{
  public:
	LoadingWindow_Win32();
	~LoadingWindow_Win32();

	void Paint();
	void InternalPaint();
	void SetText(const std::string& sText);
	void SetTextInternal();
	void SetIcon(const RageSurface* pIcon);
	void SetSplash(const RageSurface* pSplash);
	void SetProgress(const int progress);
	void SetTotalWork(const int totalWork);
	void SetIndeterminate(bool indeterminate);

  private:
	AppInstance handle;
	HWND hwnd;
	HICON m_hIcon;
	HFONT f;
	LOGFONT lf;
	std::string progress;
	std::string lastText;

	HGDIOBJ bitMapBG;
	HDC hdcBG;

	static INT_PTR CALLBACK WndProc(HWND hWnd,
									UINT msg,
									WPARAM wParam,
									LPARAM lParam);
};
#define USE_LOADING_WINDOW_WIN32

#endif
