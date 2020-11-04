#include "Etterna/Globals/global.h"
#include "ArchHooks_MacOSX.h"
#include "Core/Services/Locator.hpp"
#include "Core/Misc/AppInfo.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "archutils/Unix/SignalHandler.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "Etterna/Globals/ProductInfo.h"
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

static bool IsFatalSignal( int signal )
{
	switch( signal )
	{
	case SIGINT:
	case SIGTERM:
	case SIGHUP:
		return false;
	default:
		return true;
	}
}

static bool DoCleanShutdown( int signal, siginfo_t *si, const ucontext_t *uc )
{
	if( IsFatalSignal(signal) )
		return false;

	// ^C.
	ArchHooks::SetUserQuit();
	return true;
}

static bool DoEmergencyShutdown( int signal, siginfo_t *si, const ucontext_t *us )
{
	if( IsFatalSignal(signal) )
		_exit( 1 ); // We ran the crash handler already
	return false;
}

void ArchHooks_MacOSX::Init()
{
	// First, handle non-fatal termination signals.
	SignalHandler::OnClose( DoCleanShutdown );
	SignalHandler::OnClose( DoEmergencyShutdown );

	// Now that the crash handler is set up, disable crash reporter.
	// Breaks gdb
	// task_set_exception_ports( mach_task_self(), EXC_MASK_ALL, MACH_PORT_NULL, EXCEPTION_DEFAULT, 0 );

	// CF*Copy* functions' return values need to be released, CF*Get* functions' do not.
	CFStringRef key = CFSTR( "ApplicationBundlePath" );

	CFBundleRef bundle = CFBundleGetMainBundle();
	CFStringRef appID = CFBundleGetIdentifier( bundle );
	if( appID == NULL )
	{
		// We were probably launched through a symlink. Don't bother hunting down the real path.
		return;
	}
	CFStringRef version = CFStringRef( CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleVersionKey) );
	CFPropertyListRef old = CFPreferencesCopyAppValue( key, appID );
	CFURLRef path = CFBundleCopyBundleURL( bundle );
	CFPropertyListRef value = CFURLCopyFileSystemPath( path, kCFURLPOSIXPathStyle );
	CFMutableDictionaryRef newDict = NULL;

	if( old && CFGetTypeID(old) != CFDictionaryGetTypeID() )
	{
		CFRelease( old );
		old = NULL;
	}

	if( !old )
	{
		newDict = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
						     &kCFTypeDictionaryValueCallBacks );
		CFDictionaryAddValue( newDict, version, value );
	}
	else
	{
		CFTypeRef oldValue;
		CFDictionaryRef dict = CFDictionaryRef( old );

		if( !CFDictionaryGetValueIfPresent(dict, version, &oldValue) || !CFEqual(oldValue, value) )
		{
			// The value is either not present or it is but it is different
			newDict = CFDictionaryCreateMutableCopy( kCFAllocatorDefault, 0, dict );
			CFDictionarySetValue( newDict, version, value );
		}
		CFRelease( old );
	}

	if( newDict )
	{
		CFPreferencesSetAppValue( key, newDict, appID );
		if( !CFPreferencesAppSynchronize(appID) )
			Locator::getLogger()->warn( "Failed to record the run path." );
		CFRelease( newDict );
	}
	CFRelease( value );
	CFRelease( path );
}

int64_t ArchHooks::GetMicrosecondsSinceStart()
{
	// http://developer.apple.com/qa/qa2004/qa1398.html
	static double factor = 0.0;

	if( unlikely(factor == 0.0) )
	{
		mach_timebase_info_data_t timeBase;

		mach_timebase_info( &timeBase );
		factor = timeBase.numer / ( 1000.0 * timeBase.denom );
	}
	return int64_t( mach_absolute_time() * factor );
}

std::chrono::microseconds ArchHooks::GetChronoDurationSinceStart()
{
	return std::chrono::microseconds(GetMicrosecondsSinceStart());
}

#include "RageUtil/File/RageFileManager.h"

static void PathForFolderType( char dir[PATH_MAX], OSType folderType )
{
	FSRef fs;

	if( FSFindFolder(kUserDomain, folderType, kDontCreateFolder, &fs) )
		FAIL_M( ssprintf("FSFindFolder(%lu) failed.", folderType) );
	if( FSRefMakePath(&fs, (UInt8 *)dir, PATH_MAX) )
		FAIL_M( "FSRefMakePath() failed." );
}

void ArchHooks::MountInitialFilesystems( const std::string &sDirOfExecutable )
{
	char dir[PATH_MAX];
	CFURLRef dataUrl = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR("StepMania"), CFSTR("smzip"), NULL );

	FILEMAN->Mount( "dir", sDirOfExecutable, "/" );

	if( dataUrl )
	{
		CFStringRef dataPath = CFURLCopyFileSystemPath( dataUrl, kCFURLPOSIXPathStyle );
		CFStringGetCString( dataPath, dir, PATH_MAX, kCFStringEncodingUTF8 );

		if( strncmp(sDirOfExecutable.c_str(), dir, sDirOfExecutable.length()) == 0 )
			FILEMAN->Mount( "zip", dir + sDirOfExecutable.length(), "/" );
		CFRelease( dataPath );
		CFRelease( dataUrl );
	}
}

void ArchHooks::MountUserFilesystems( const std::string &sDirOfExecutable )
{
	char dir[PATH_MAX];

	// /Save -> ~/Library/Preferences/PRODUCT_ID
	PathForFolderType( dir, kPreferencesFolderType );
	FILEMAN->Mount( "dir", ssprintf("%s/%s", dir, Core::AppInfo::APP_TITLE), "/Save" );

	// Other stuff -> ~/Library/Application Support/PRODUCT_ID/*
	PathForFolderType( dir, kApplicationSupportFolderType );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/Announcers", dir, Core::AppInfo::APP_TITLE), "/Announcers" );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/BGAnimations", dir, Core::AppInfo::APP_TITLE), "/BGAnimations" );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/BackgroundEffects", dir, Core::AppInfo::APP_TITLE), "/BackgroundEffects" );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/BackgroundTransitions", dir, Core::AppInfo::APP_TITLE), "/BackgroundTransitions" );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/CDTitles", dir, Core::AppInfo::APP_TITLE), "/CDTitles" );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/Characters", dir, Core::AppInfo::APP_TITLE), "/Characters" );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/Courses", dir, Core::AppInfo::APP_TITLE), "/Courses" );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/NoteSkins", dir, Core::AppInfo::APP_TITLE), "/NoteSkins" );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/Packages", dir, Core::AppInfo::APP_TITLE), "/" + SpecialFiles::USER_PACKAGES_DIR );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/Songs", dir, Core::AppInfo::APP_TITLE), "/Songs" );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/RandomMovies", dir, Core::AppInfo::APP_TITLE), "/RandomMovies" );
	FILEMAN->Mount( "dir", ssprintf("%s/%s/Themes", dir, Core::AppInfo::APP_TITLE), "/Themes" );

	// /Screenshots -> ~/Pictures/PRODUCT_ID Screenshots
	PathForFolderType( dir, kPictureDocumentsFolderType );
	FILEMAN->Mount( "dir", ssprintf("%s/%s Screenshots", dir, Core::AppInfo::APP_TITLE), "/Screenshots" );

	// /Cache -> ~/Library/Caches/PRODUCT_ID
	PathForFolderType( dir, kCachedDataFolderType );
	FILEMAN->Mount( "dir", ssprintf("%s/%s", dir, Core::AppInfo::APP_TITLE), "/Cache" );

	// /Logs -> ~/Library/Logs/PRODUCT_ID
	PathForFolderType( dir, kDomainLibraryFolderType );
	FILEMAN->Mount( "dir", ssprintf("%s/Logs/%s", dir, Core::AppInfo::APP_TITLE), "/Logs" );

	// /Desktop -> /Users/<user>/Desktop/PRODUCT_ID
	// PathForFolderType( dir, kDesktopFolderType );
	// FILEMAN->Mount( "dir", ssprintf("%s/" PRODUCT_ID, dir), "/Desktop" );
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
