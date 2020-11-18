#include "Etterna/Globals/global.h"
#include "ArchHooks.h"
#include "RageUtil/Utils/RageUtil.h"
#include "archutils/Win32/SpecialDirs.h"
#include "Etterna/Globals/ProductInfo.h"
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "Core/Services/Locator.hpp"

#include <windows.h>
#include <mmsystem.h>
#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib")
#endif

#include <chrono>

static bool g_bTimerInitialized;

static void
InitTimer()
{
	if (g_bTimerInitialized)
		return;
	g_bTimerInitialized = true;

	// Set Windows clock resolution to 1ms
	timeBeginPeriod(1);
}

int64_t
ArchHooks::GetMicrosecondsSinceStart()
{
	if (!g_bTimerInitialized)
		InitTimer();

	int64_t ret = timeGetTime() * int64_t(1000);
	ret = FixupTimeIfLooped(ret);
	ret = FixupTimeIfBackwards(ret);

	return ret;
}

std::chrono::microseconds
ArchHooks::GetChronoDurationSinceStart()
{
	if (!g_bTimerInitialized)
		InitTimer();

	// You may be thinking "why dont we use ::now() compared with a duration and
	// return that?" well, that didnt work, is all i can say right now. maybe
	// rewriting it later will work
	return std::chrono::microseconds(GetMicrosecondsSinceStart());
}

static std::string
GetMountDir(const std::string& sDirOfExecutable)
{
	/* All Windows data goes in the directory one level above the executable. */
	Locator::getLogger()->trace("DOE \"{}\"", sDirOfExecutable);
	vector<std::string> asParts;
	split(sDirOfExecutable, "/", asParts);
	Locator::getLogger()->trace("... {} asParts", asParts.size());
	ASSERT_M(
	  asParts.size() > 1,
	  ssprintf("Strange sDirOfExecutable: %s", sDirOfExecutable.c_str()));
	std::string sDir = join("/", asParts.begin(), asParts.end() - 1);
	return sDir;
}

void
ArchHooks::MountInitialFilesystems(const std::string& sDirOfExecutable)
{
	std::string sDir = GetMountDir(sDirOfExecutable);

	FILEMAN->Mount("dir", sDir, "/");
}

void
ArchHooks::MountUserFilesystems(const std::string& sDirOfExecutable)
{
	/*
	 * Look, I know what you're thinking: "Hey, let's put all this stuff into
	 * their respective 'proper' places on the filesystem!" Stop. Now.
	 * This was done before and it was the most ungodly confusing thing to ever
	 * happen. Just don't do it, seriously. Keep them in one place.
	 * - Colby
	 */
	std::string sAppDataDir = SpecialDirs::GetAppDataDir() + PRODUCT_ID;
	// std::string sCommonAppDataDir = SpecialDirs::GetCommonAppDataDir() +
	// PRODUCT_ID;  std::string sLocalAppDataDir =
	// SpecialDirs::GetLocalAppDataDir()
	// + PRODUCT_ID;  std::string sPicturesDir = SpecialDirs::GetPicturesDir() +
	// PRODUCT_ID;

	FILEMAN->Mount("dir", sAppDataDir + "/Announcers", "/Announcers");
	FILEMAN->Mount("dir", sAppDataDir + "/BGAnimations", "/BGAnimations");
	FILEMAN->Mount(
	  "dir", sAppDataDir + "/BackgroundEffects", "/BackgroundEffects");
	FILEMAN->Mount(
	  "dir", sAppDataDir + "/BackgroundTransitions", "/BackgroundTransitions");
	FILEMAN->Mount("dir", sAppDataDir + "/Cache", "/Cache");
	FILEMAN->Mount("dir", sAppDataDir + "/CDTitles", "/CDTitles");
	FILEMAN->Mount("dir", sAppDataDir + "/Courses", "/Courses");
	FILEMAN->Mount("dir", sAppDataDir + "/Logs", "/Logs");
	FILEMAN->Mount("dir", sAppDataDir + "/NoteSkins", "/NoteSkins");
	FILEMAN->Mount(
	  "dir", sAppDataDir + "/Packages", "/" + SpecialFiles::USER_PACKAGES_DIR);
	FILEMAN->Mount("dir", sAppDataDir + "/Save", "/Save");
	FILEMAN->Mount("dir", sAppDataDir + "/Screenshots", "/Screenshots");
	FILEMAN->Mount("dir", sAppDataDir + "/Songs", "/Songs");
	FILEMAN->Mount("dir", sAppDataDir + "/RandomMovies", "/RandomMovies");
	FILEMAN->Mount("dir", sAppDataDir + "/Themes", "/Themes");
}

static std::string
LangIdToString(LANGID l)
{
	switch (PRIMARYLANGID(l)) {
		case LANG_ARABIC:
			return "ar";
		case LANG_BULGARIAN:
			return "bg";
		case LANG_CATALAN:
			return "ca";
		case LANG_CHINESE:
			return "zh";
		case LANG_CZECH:
			return "cs";
		case LANG_DANISH:
			return "da";
		case LANG_GERMAN:
			return "de";
		case LANG_GREEK:
			return "el";
		case LANG_SPANISH:
			return "es";
		case LANG_FINNISH:
			return "fi";
		case LANG_FRENCH:
			return "fr";
		case LANG_HEBREW:
			return "iw";
		case LANG_HUNGARIAN:
			return "hu";
		case LANG_ICELANDIC:
			return "is";
		case LANG_ITALIAN:
			return "it";
		case LANG_JAPANESE:
			return "ja";
		case LANG_KOREAN:
			return "ko";
		case LANG_DUTCH:
			return "nl";
		case LANG_NORWEGIAN:
			return "no";
		case LANG_POLISH:
			return "pl";
		case LANG_PORTUGUESE:
			return "pt";
		case LANG_ROMANIAN:
			return "ro";
		case LANG_RUSSIAN:
			return "ru";
		case LANG_CROATIAN:
			return "hr";
		// case LANG_SERBIAN: return "sr"; // same as LANG_CROATIAN?
		case LANG_SLOVAK:
			return "sk";
		case LANG_ALBANIAN:
			return "sq";
		case LANG_SWEDISH:
			return "sv";
		case LANG_THAI:
			return "th";
		case LANG_TURKISH:
			return "tr";
		case LANG_URDU:
			return "ur";
		case LANG_INDONESIAN:
			return "in";
		case LANG_UKRAINIAN:
			return "uk";
		case LANG_SLOVENIAN:
			return "sl";
		case LANG_ESTONIAN:
			return "et";
		case LANG_LATVIAN:
			return "lv";
		case LANG_LITHUANIAN:
			return "lt";
		case LANG_VIETNAMESE:
			return "vi";
		case LANG_ARMENIAN:
			return "hy";
		case LANG_BASQUE:
			return "eu";
		case LANG_MACEDONIAN:
			return "mk";
		case LANG_AFRIKAANS:
			return "af";
		case LANG_GEORGIAN:
			return "ka";
		case LANG_FAEROESE:
			return "fo";
		case LANG_HINDI:
			return "hi";
		case LANG_MALAY:
			return "ms";
		case LANG_KAZAK:
			return "kk";
		case LANG_SWAHILI:
			return "sw";
		case LANG_UZBEK:
			return "uz";
		case LANG_TATAR:
			return "tt";
		case LANG_PUNJABI:
			return "pa";
		case LANG_GUJARATI:
			return "gu";
		case LANG_TAMIL:
			return "ta";
		case LANG_KANNADA:
			return "kn";
		case LANG_MARATHI:
			return "mr";
		case LANG_SANSKRIT:
			return "sa";
		// These aren't present in the VC6 headers. We'll never have
		// translations to these languages anyway. -C
		// case LANG_MONGOLIAN: return "mn";
		// case LANG_GALICIAN: return "gl";
		default:
		case LANG_ENGLISH:
			return "en";
	}
}

static LANGID
GetLanguageID()
{
	HINSTANCE hDLL = LoadLibrary("kernel32.dll");
	if (hDLL) {
		typedef LANGID(GET_USER_DEFAULT_UI_LANGUAGE)(void);

		GET_USER_DEFAULT_UI_LANGUAGE* pGetUserDefaultUILanguage =
		  (GET_USER_DEFAULT_UI_LANGUAGE*)GetProcAddress(
			hDLL, "GetUserDefaultUILanguage");
		if (pGetUserDefaultUILanguage) {
			LANGID ret = pGetUserDefaultUILanguage();
			FreeLibrary(hDLL);
			return ret;
		}
		FreeLibrary(hDLL);
	}

	return GetUserDefaultLangID();
}

std::string
ArchHooks::GetPreferredLanguage()
{
	return LangIdToString(GetLanguageID());
}

/*
 * (c) 2003-2004 Chris Danford
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
