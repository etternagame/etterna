#include "Etterna/Globals/global.h"
#include "ArchHooks_Win32.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Misc/RageThreads.h"
#include "Etterna/Globals/ProductInfo.h"
#include "archutils/win32/AppInstance.h"
#include "archutils/win32/crash.h"
#include "archutils/win32/DebugInfoHunt.h"
#include "archutils/win32/ErrorStrings.h"
#include "archutils/win32/RestartProgram.h"
#include "archutils/win32/GotoURL.h"
#include "archutils/Win32/GraphicsWindow.h"

static HANDLE g_hInstanceMutex;
static bool g_bIsMultipleInstance = false;

#if _MSC_VER >= 1400 // VC8
#include <cwchar>
void
InvalidParameterHandler(const wchar_t* szExpression,
						const wchar_t* szFunction,
						const wchar_t* szFile,
						unsigned int iLine,
						uintptr_t pReserved)
{
	CHECKPOINT_M("Entered Invalid Parameter Handler");

	std::mbstate_t state = std::mbstate_t();
	int lenExpr = 1 + std::wcsrtombs(NULL, &szExpression, 0, &state);
	state = std::mbstate_t();
	int lenFunc = 1 + std::wcsrtombs(NULL, &szFunction, 0, &state);
	state = std::mbstate_t();
	int lenFile = 1 + std::wcsrtombs(NULL, &szFile, 0, &state);

	std::vector<char> strExpr(lenExpr);
	std::vector<char> strFunc(lenFunc);
	std::vector<char> strFile(lenFile);

	std::wcsrtombs(&strExpr[0], &szExpression, lenExpr, &state);
	std::wcsrtombs(&strFunc[0], &szFunction, lenFunc, &state);
	std::wcsrtombs(&strFile[0], &szFile, lenFile, &state);

	std::string expr(strExpr.begin(), strExpr.end());
	std::string func(strFunc.begin(), strFunc.end());
	std::string file(strFile.begin(), strFile.end());

	FAIL_M(ssprintf(
	  "Invalid Parameter In C Function %s\n File: %s Line %d\n Expression: %s",
	  func,
	  file,
	  iLine,
	  expr));
}
#endif

ArchHooks_Win32::ArchHooks_Win32()
{
	HOOKS = this;

	/* Disable critical errors, and handle them internally.  We never want the
	 * "drive not ready", etc. dialogs to pop up. */
	SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);

	CrashHandler::CrashHandlerHandleArgs(g_argc, g_argv);
	SetUnhandledExceptionFilter(CrashHandler::ExceptionHandler);

#if _MSC_VER >= 1400 // VC8
	_set_invalid_parameter_handler(InvalidParameterHandler);
#endif

	/* Windows boosts priority on keyboard input, among other things.  Disable
	 * that for the main thread. */
	SetThreadPriorityBoost(GetCurrentThread(), TRUE);

	g_hInstanceMutex = CreateMutex(NULL, TRUE, PRODUCT_ID);

	g_bIsMultipleInstance = false;
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		g_bIsMultipleInstance = true;
}

ArchHooks_Win32::~ArchHooks_Win32()
{
	CloseHandle(g_hInstanceMutex);
}

void
ArchHooks_Win32::DumpDebugInfo()
{
	/* This is a good time to do the debug search: before we actually
	 * start OpenGL (in case something goes wrong). */
	SearchForDebugInfo();
}

struct CallbackData
{
	HWND hParent;
	HWND hResult;
};

// Like GW_ENABLEDPOPUP:
static BOOL CALLBACK
GetEnabledPopup(HWND hWnd, LPARAM lParam)
{
	CallbackData* pData = (CallbackData*)lParam;
	if (GetParent(hWnd) != pData->hParent)
		return TRUE;
	if ((GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP) != WS_POPUP)
		return TRUE;
	if (!IsWindowEnabled(hWnd))
		return TRUE;

	pData->hResult = hWnd;
	return FALSE;
}

bool
ArchHooks_Win32::CheckForMultipleInstances(int argc, char* argv[])
{
	if (!g_bIsMultipleInstance)
		return false;

	/* Search for the existing window.  Prefer to use the class name, which is
	 * less likely to have a false match, and will match the gameplay window.
	 * If that fails, try the window name, which should match the loading
	 * window. */
	HWND hWnd = FindWindow(PRODUCT_ID, NULL);
	if (hWnd == NULL)
		hWnd = FindWindow(NULL, PRODUCT_ID);

	if (hWnd != NULL) {
		/* If the application has a model dialog box open, we want to be sure to
		 * give focus to it, not the main window. */
		CallbackData data;
		data.hParent = hWnd;
		data.hResult = NULL;
		EnumWindows(GetEnabledPopup, (LPARAM)&data);

		if (data.hResult != NULL)
			SetForegroundWindow(data.hResult);
		else
			SetForegroundWindow(hWnd);

		// Send the command line to the existing window.
		vector<std::string> vsArgs;
		for (int i = 0; i < argc; i++)
			vsArgs.push_back(argv[i]);
		std::string sAllArgs = join("|", vsArgs);
		COPYDATASTRUCT cds;
		cds.dwData = 0;
		cds.cbData = sAllArgs.size();
		cds.lpData = (void*)sAllArgs.data();
		SendMessage(
		  (HWND)hWnd, // HWND hWnd = handle of destination window
		  WM_COPYDATA,
		  (WPARAM)NULL,	 // HANDLE OF SENDING WINDOW
		  (LPARAM)&cds); // 2nd msg parameter = pointer to COPYDATASTRUCT
	}

	return true;
}

void
ArchHooks_Win32::RestartProgram()
{
	Win32RestartProgram();
}

void
ArchHooks_Win32::SetTime(tm newtime)
{
	SYSTEMTIME st;
	ZERO(st);
	st.wYear = (WORD)newtime.tm_year + 1900;
	st.wMonth = (WORD)newtime.tm_mon + 1;
	st.wDay = (WORD)newtime.tm_mday;
	st.wHour = (WORD)newtime.tm_hour;
	st.wMinute = (WORD)newtime.tm_min;
	st.wSecond = (WORD)newtime.tm_sec;
	st.wMilliseconds = 0;
	SetLocalTime(&st);
}

void
ArchHooks_Win32::BoostPriority()
{
	/* We just want a slight boost, so we don't skip needlessly if something
	 * happens in the background.  We don't really want to be
	 * high-priority--above normal should be enough.  However,
	 * ABOVE_NORMAL_PRIORITY_CLASS is only supported in Win2000 and later. */
	OSVERSIONINFO version;
	version.dwOSVersionInfoSize = sizeof(version);
	if (!GetVersionEx(&version)) {
		LOG->Warn(werr_ssprintf(GetLastError(), "GetVersionEx failed").c_str());
		return;
	}

#ifndef ABOVE_NORMAL_PRIORITY_CLASS
#define ABOVE_NORMAL_PRIORITY_CLASS 0x00008000
#endif

	DWORD pri = HIGH_PRIORITY_CLASS;
	if (version.dwMajorVersion >= 5)
		pri = ABOVE_NORMAL_PRIORITY_CLASS;

	/* Be sure to boost the app, not the thread, to make sure the
	 * sound thread stays higher priority than the main thread. */
	SetPriorityClass(GetCurrentProcess(), pri);
}

void
ArchHooks_Win32::UnBoostPriority()
{
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
}

void
ArchHooks_Win32::SetupConcurrentRenderingThread()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
}

bool
ArchHooks_Win32::GoToURL(const std::string& sUrl)
{
	return ::GotoURL(sUrl);
}

int
ArchHooks_Win32::GetWindowWidth()
{
	RECT rect;
	GetClientRect(GraphicsWindow::GetHwnd(), &rect);
	return rect.right - rect.left;
}

int
ArchHooks_Win32::GetWindowHeight()
{
	RECT rect;
	GetClientRect(GraphicsWindow::GetHwnd(), &rect);
	return rect.bottom - rect.top;
}

float
ArchHooks_Win32::GetDisplayAspectRatio()
{
	DEVMODE dm;
	ZERO(dm);
	dm.dmSize = sizeof(dm);
	BOOL bResult = EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &dm);
	ASSERT(bResult != 0);
	return dm.dmPelsWidth / static_cast<float>(dm.dmPelsHeight);
}

std::string
ArchHooks_Win32::GetClipboard()
{
	HGLOBAL hgl;
	LPTSTR lpstr;
	std::string ret;

	// First make sure that the clipboard actually contains a string
	// (or something stringifiable)
	if (unlikely(!IsClipboardFormatAvailable(CF_TEXT)))
		return "";

	// Yes. All this mess just to gain access to the string stored by the
	// clipboard. I'm having flashbacks to Berkeley sockets.
	if (unlikely(!OpenClipboard(NULL))) {
		LOG->Warn(
		  werr_ssprintf(GetLastError(),
						"InputHandler_DirectInput: OpenClipboard() failed")
			.c_str());
		return "";
	}

	hgl = GetClipboardData(CF_TEXT);
	if (unlikely(hgl == NULL)) {
		LOG->Warn(
		  werr_ssprintf(GetLastError(),
						"InputHandler_DirectInput: GetClipboardData() failed")
			.c_str());
		CloseClipboard();
		return "";
	}

	lpstr = (LPTSTR)GlobalLock(hgl);
	if (unlikely(lpstr == NULL)) {
		LOG->Warn(werr_ssprintf(GetLastError(),
								"InputHandler_DirectInput: GlobalLock() failed")
					.c_str());
		CloseClipboard();
		return "";
	}

	// And finally, we have a char (or wchar_t) array of the clipboard
	// contents, pointed to by sToPaste. (Hopefully.)

#ifdef UNICODE
	ret = WStringToString(wstring() + *lpstr);
#else
	ret = std::string(lpstr);
#endif

	// And now we clean up.
	GlobalUnlock(hgl);
	CloseClipboard();

	return ret;
}

/*
 * (c) 2003-2004 Glenn Maynard, Chris Danford
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
