#include "Etterna/Globals/global.h"
#include "Core/Services/Locator.hpp"
#include "Core/Misc/AppInfo.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "archutils/Unix/SignalHandler.h"
#include "Etterna/Globals/SpecialFiles.h"
#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <chrono>
extern "C" {
#include <mach/mach_time.h>
#include <IOKit/graphics/IOGraphicsLib.h>
}
#include <IOKit/IOKitLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#include <IOKit/network/IOEthernetController.h>

#import <Foundation/Foundation.h>


#include "RageUtil/File/RageFileManager.h"

static void PathForFolderType( char dir[PATH_MAX], OSType folderType )
{
	FSRef fs;

	if( FSFindFolder(kUserDomain, folderType, kDontCreateFolder, &fs) )
		FAIL_M( ssprintf("FSFindFolder(%lu) failed.", folderType) );
	if( FSRefMakePath(&fs, (UInt8 *)dir, PATH_MAX) )
		FAIL_M( "FSRefMakePath() failed." );
}

static inline int GetIntValue( CFTypeRef r )
{
	int ret;

	if( !r || CFGetTypeID(r) != CFNumberGetTypeID() || !CFNumberGetValue(CFNumberRef(r), kCFNumberIntType, &ret) )
		return 0;
	return ret;
}

/*
 * (c) 2003-2006 Steve Checkoway
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
