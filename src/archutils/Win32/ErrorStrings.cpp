#include "Etterna/Globals/global.h"
#include "ErrorStrings.h"
#include "RageUtil/Utils/RageUtil.h"

#include <windows.h>

RString
werr_ssprintf(int err, const char* fmt, ...)
{
	char buf[1024] = "";
	FormatMessage(
	  FORMAT_MESSAGE_FROM_SYSTEM, 0, err, 0, buf, sizeof(buf), NULL);

	// Why is FormatMessage returning text ending with \r\n? (who? -aj)
	// Perhaps it's because you're on Windows, where newlines are \r\n. -aj
	RString text = buf;
	text.Replace("\n", "");
	text.Replace("\r", " "); // foo\r\nbar -> foo bar
	TrimRight(text);		 // "foo\r\n" -> "foo"

	va_list va;
	va_start(va, fmt);
	RString s = vssprintf(fmt, va);
	va_end(va);

	return s += ssprintf(" (%s)", text.c_str());
}

RString
ConvertWstringToCodepage(const std::wstring& s, int iCodePage)
{
	if (s.empty())
		return RString();

	int iBytes = WideCharToMultiByte(
	  iCodePage, 0, s.data(), (int)s.size(), NULL, 0, NULL, FALSE);
	ASSERT_M(iBytes > 0,
			 werr_ssprintf(GetLastError(), "WideCharToMultiByte").c_str());

	char* buf = new char[iBytes + 1];
	std::fill(buf, buf + iBytes + 1, '\0');
	WideCharToMultiByte(
	  CP_ACP, 0, s.data(), (int)s.size(), buf, iBytes, NULL, FALSE);
	RString ret(buf);
	delete[] buf;
	return ret;
}

RString
ConvertUTF8ToACP(const RString& s)
{
	return ConvertWstringToCodepage(RStringToWstring(s), CP_ACP);
}

std::wstring
ConvertCodepageToWString(const RString& s, int iCodePage)
{
	if (s.empty())
		return std::wstring();

	int iBytes =
	  MultiByteToWideChar(iCodePage, 0, s.data(), (int)s.size(), NULL, 0);
	ASSERT_M(iBytes > 0,
			 werr_ssprintf(GetLastError(), "MultiByteToWideChar").c_str());

	wchar_t* pTemp = new wchar_t[iBytes];
	MultiByteToWideChar(iCodePage, 0, s.data(), (int)s.size(), pTemp, iBytes);
	std::wstring sRet(pTemp, iBytes);
	delete[] pTemp;

	return sRet;
}

RString
ConvertACPToUTF8(const RString& s)
{
	return WStringToRString(ConvertCodepageToWString(s, CP_ACP));
}

/*
 * Copyright (c) 2001-2005 Chris Danford, Glenn Maynard
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
