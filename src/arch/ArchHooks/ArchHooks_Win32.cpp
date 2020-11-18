#include "Etterna/Globals/global.h"
#include "ArchHooks_Win32.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Core/Services/Locator.hpp"
#include "Core/Misc/AppInfo.hpp"
#include "RageUtil/Misc/RageThreads.h"
#include "archutils/win32/AppInstance.h"
#include "archutils/win32/ErrorStrings.h"
#include "archutils/win32/RestartProgram.h"
#include "archutils/Win32/GraphicsWindow.h"

#if _MSC_VER >= 1400 // VC8
#include <cwchar>
void
InvalidParameterHandler(const wchar_t* szExpression,
						const wchar_t* szFunction,
						const wchar_t* szFile,
						unsigned int iLine,
						uintptr_t pReserved)
{
	Locator::getLogger()->trace("Entered Invalid Parameter Handler");

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

	/* Disable critical errors, and handle them internally.  We never want the
	 * "drive not ready", etc. dialogs to pop up. */
	SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);

//	CrashHandler::CrashHandlerHandleArgs(g_argc, g_argv);
//	SetUnhandledExceptionFilter(CrashHandler::ExceptionHandler);

#if _MSC_VER >= 1400 // VC8
	_set_invalid_parameter_handler(InvalidParameterHandler);
#endif

	/* Windows boosts priority on keyboard input, among other things.  Disable
	 * that for the main thread. */
	SetThreadPriorityBoost(GetCurrentThread(), TRUE);
}

void
ArchHooks_Win32::RestartProgram()
{
	Win32RestartProgram();
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
