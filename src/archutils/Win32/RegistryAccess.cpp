#include "Etterna/Globals/global.h"
#include "RegistryAccess.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "archutils/Win32/ErrorStrings.h"

#include <windows.h>

/* Given "HKEY_LOCAL_MACHINE\hardware\foo", return "hardware\foo", and place
 * the HKEY_LOCAL_MACHINE constant in key. */
static bool
GetRegKeyType(const std::string& sIn, std::string& sOut, HKEY& key)
{
	size_t iBackslash = sIn.find('\\');
	if (iBackslash == sIn.npos) {
		Locator::getLogger()->warn("Invalid registry key: \"{}\" ", sIn.c_str());
		return false;
	}

	std::string sType = sIn.substr(0, iBackslash);

	if (!CompareNoCase(sType, "HKEY_CLASSES_ROOT"))
		key = HKEY_CLASSES_ROOT;
	else if (!CompareNoCase(sType, "HKEY_CURRENT_CONFIG"))
		key = HKEY_CURRENT_CONFIG;
	else if (!CompareNoCase(sType, "HKEY_CURRENT_USER"))
		key = HKEY_CURRENT_USER;
	else if (!CompareNoCase(sType, "HKEY_LOCAL_MACHINE"))
		key = HKEY_LOCAL_MACHINE;
	else if (!CompareNoCase(sType, "HKEY_USERS"))
		key = HKEY_USERS;
	else {
		Locator::getLogger()->warn("Invalid registry key: \"{}\" ", sIn.c_str());
		return false;
	}

	sOut = sIn.substr(iBackslash + 1);

	return true;
}

/* Given a full key, eg. "HKEY_LOCAL_MACHINE\hardware\foo", open it and return
 * it. On error, return NULL. */
enum RegKeyMode
{
	READ,
	WRITE
};
static HKEY
OpenRegKey(const std::string& sKey, RegKeyMode mode, bool bWarnOnError = true)
{
	std::string sSubkey;
	HKEY hType;
	if (!GetRegKeyType(sKey, sSubkey, hType))
		return nullptr;

	HKEY hRetKey;
	LONG retval = RegOpenKeyEx(hType,
							   sSubkey.c_str(),
							   0,
							   (mode == READ) ? KEY_READ : KEY_WRITE,
							   &hRetKey);
	if (retval != ERROR_SUCCESS) {
		if (bWarnOnError)
			Locator::getLogger()->warn(werr_ssprintf(retval, "RegOpenKeyEx(%x,%s) error", hType, sSubkey.c_str()));
		return nullptr;
	}

	return hRetKey;
}

bool
RegistryAccess::GetRegValue(const std::string& sKey,
							const std::string& sName,
							std::string& sVal)
{
	HKEY hKey = OpenRegKey(sKey, READ);
	if (hKey == nullptr)
		return false;

	char sBuffer[MAX_PATH];
	DWORD iSize = sizeof(sBuffer);
	DWORD iType;
	LONG iRet = RegQueryValueEx(
	  hKey, sName.c_str(), nullptr, &iType, (LPBYTE)sBuffer, &iSize);
	RegCloseKey(hKey);
	if (iRet != ERROR_SUCCESS)
		return false;

	/* Actually, CStrings are 8-bit clean, so we can accept any type of data.
	 * Remove this if that becomes useful. */
	if (iType != REG_SZ && iType != REG_MULTI_SZ && iType != REG_EXPAND_SZ &&
		iType != REG_BINARY)
		return false; /* type mismatch */

	if (iSize &&
		(iType == REG_SZ || iType == REG_MULTI_SZ || iType == REG_EXPAND_SZ))
		--iSize; /* remove nul terminator */

	sVal = std::string(sBuffer, iSize);
	return true;
}

bool
RegistryAccess::GetRegValue(const std::string& sKey,
							const std::string& sName,
							int& iVal,
							bool bWarnOnError)
{
	HKEY hKey = OpenRegKey(sKey, READ, bWarnOnError);
	if (hKey == nullptr)
		return false;

	DWORD iValue;
	DWORD iSize = sizeof(iValue);
	DWORD iType;
	LONG iRet = RegQueryValueEx(
	  hKey, sName.c_str(), nullptr, &iType, (LPBYTE)&iValue, &iSize);
	RegCloseKey(hKey);
	if (iRet != ERROR_SUCCESS)
		return false;

	if (iType != REG_DWORD)
		return false; /* type mismatch */

	iVal = iValue;
	return true;
}

bool
RegistryAccess::GetRegValue(const std::string& sKey,
							const std::string& sName,
							bool& bVal)
{
	int iVal;
	bool b = GetRegValue(sKey, sName, iVal);
	bVal = !!iVal;
	return b;
}

bool
RegistryAccess::GetRegSubKeys(const std::string& sKey,
							  std::vector<std::string>& lst,
							  const std::string& regex,
							  bool bReturnPathToo)
{
	HKEY hKey = OpenRegKey(sKey, READ);
	if (hKey == nullptr)
		return false;

	Regex re(regex);

	bool bError = false;
	for (int index = 0;; ++index) {
		FILETIME ft;
		char szBuffer[MAX_PATH];
		DWORD iSize = sizeof(szBuffer);
		LONG iRet = RegEnumKeyEx(
		  hKey, index, szBuffer, &iSize, nullptr, nullptr, nullptr, &ft);
		if (iRet == ERROR_NO_MORE_ITEMS)
			break;

		if (iRet != ERROR_SUCCESS) {
			Locator::getLogger()->warn(werr_ssprintf(iRet, "GetRegSubKeys(%p,%i) error", hKey, index));
			bError = true;
			break;
		}

		std::string sStr(szBuffer, iSize);

		if (re.Compare(sStr)) {
			if (bReturnPathToo)
				sStr = sKey + "\\" + sStr;
			lst.push_back(sStr);
		}
	}

	RegCloseKey(hKey);

	return !bError;
}

bool
RegistryAccess::SetRegValue(const std::string& sKey,
							const std::string& sName,
							const std::string& sVal)
{
	HKEY hKey = OpenRegKey(sKey, WRITE);
	if (hKey == nullptr)
		return false;

	bool bSuccess = true;
	TCHAR sz[255];

	if (sVal.size() > 254)
		return false;

	strcpy(sz, sVal.c_str());

	LONG lResult = ::RegSetValueEx(
	  hKey, LPCTSTR(sName.c_str()), 0, REG_SZ, (LPBYTE)sz, strlen(sz) + 1);
	if (lResult != ERROR_SUCCESS)
		bSuccess = false;

	::RegCloseKey(hKey);
	return bSuccess;
}

bool
RegistryAccess::SetRegValue(const std::string& sKey,
							const std::string& sName,
							bool bVal)
{
	HKEY hKey = OpenRegKey(sKey, WRITE);
	if (hKey == nullptr)
		return false;

	bool bSuccess = true;

	if (::RegSetValueEx(hKey,
						LPCTSTR(sName.c_str()),
						0,
						REG_BINARY,
						(LPBYTE)&bVal,
						sizeof(bVal)) != ERROR_SUCCESS)
		bSuccess = false;

	::RegCloseKey(hKey);
	return bSuccess;
}

bool
RegistryAccess::CreateKey(const std::string& sKey)
{
	std::string sSubkey;
	HKEY hType;
	if (!GetRegKeyType(sKey, sSubkey, hType))
		return NULL;

	HKEY hKey;
	DWORD dwDisposition = 0;
	if (::RegCreateKeyEx(hType,
						 sSubkey.c_str(),
						 0,
						 nullptr,
						 REG_OPTION_NON_VOLATILE,
						 KEY_ALL_ACCESS,
						 nullptr,
						 &hKey,
						 &dwDisposition) != ERROR_SUCCESS) {
		return false;
	}
	::RegCloseKey(hKey);
	return true;
}

/*
 * (c) 2004 Glenn Maynard
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
