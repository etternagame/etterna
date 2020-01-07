#ifndef ERROR_STRINGS_H
#define ERROR_STRINGS_H

RString
werr_ssprintf(int err, const char* fmt, ...);
RString
ConvertWstringToCodepage(const std::wstring& s, int iCodePage);
RString
ConvertUTF8ToACP(const RString& s);
std::wstring
ConvertCodepageToWString(const RString& s, int iCodePage);
RString
ConvertACPToUTF8(const RString& s);

#endif
