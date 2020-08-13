#ifndef ERROR_STRINGS_H
#define ERROR_STRINGS_H

std::string
werr_ssprintf(int err, const char* fmt, ...);
std::string
ConvertWstringToCodepage(const std::wstring& s, int iCodePage);
std::string
ConvertUTF8ToACP(const std::string& s);
std::wstring
ConvertCodepageToWString(const std::string& s, int iCodePage);
std::string
ConvertACPToUTF8(const std::string& s);

#endif
