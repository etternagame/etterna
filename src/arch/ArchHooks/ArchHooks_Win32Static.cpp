#include "Etterna/Globals/global.h"
#include "ArchHooks.h"
#include "RageUtil/Utils/RageUtil.h"
#include "archutils/Win32/SpecialDirs.h"
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "Core/Services/Locator.hpp"
#include "Core/Misc/AppInfo.hpp"

#include <windows.h>
#include <mmsystem.h>
#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib")
#endif

#include <chrono>

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
	std::string sAppDataDir = SpecialDirs::GetAppDataDir() + Core::AppInfo::APP_TITLE;
	// std::string sCommonAppDataDir = SpecialDirs::GetCommonAppDataDir() +
	// PRODUCT_ID;  std::string sLocalAppDataDir =
	// SpecialDirs::GetLocalAppDataDir()
	// + PRODUCT_ID;  std::string sPicturesDir = SpecialDirs::GetPicturesDir() +
	// PRODUCT_ID;

	FILEMAN->Mount("dir", sAppDataDir + "/Announcers", "/Announcers");
	FILEMAN->Mount("dir", sAppDataDir + "/BGAnimations", "/BGAnimations");
	FILEMAN->Mount("dir", sAppDataDir + "/BackgroundEffects", "/BackgroundEffects");
	FILEMAN->Mount("dir", sAppDataDir + "/BackgroundTransitions", "/BackgroundTransitions");
	FILEMAN->Mount("dir", sAppDataDir + "/Cache", "/Cache");
	FILEMAN->Mount("dir", sAppDataDir + "/CDTitles", "/CDTitles");
	FILEMAN->Mount("dir", sAppDataDir + "/Courses", "/Courses");
	FILEMAN->Mount("dir", sAppDataDir + "/Logs", "/Logs");
	FILEMAN->Mount("dir", sAppDataDir + "/NoteSkins", "/NoteSkins");
	FILEMAN->Mount("dir", sAppDataDir + "/Packages", "/" + SpecialFiles::USER_PACKAGES_DIR);
	FILEMAN->Mount("dir", sAppDataDir + "/Save", "/Save");
	FILEMAN->Mount("dir", sAppDataDir + "/Screenshots", "/Screenshots");
	FILEMAN->Mount("dir", sAppDataDir + "/Songs", "/Songs");
	FILEMAN->Mount("dir", sAppDataDir + "/RandomMovies", "/RandomMovies");
	FILEMAN->Mount("dir", sAppDataDir + "/Themes", "/Themes");
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
