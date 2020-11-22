#include "Etterna/Globals/global.h"
#include "Crash.h"
#include "Core/Services/Locator.hpp"
#include "Core/Platform/Platform.hpp"
#include "Core/Misc/AppInfo.hpp"
#include <CoreServices/CoreServices.h>
#include <sys/types.h>
#include <fmt/format.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/sysctl.h>

std::string
CrashHandler::GetLogsDirectory()
{
	FSRef fs;
	char dir[PATH_MAX];

	if (FSFindFolder(
		  kUserDomain, kDomainLibraryFolderType, kDontCreateFolder, &fs) ||
		FSRefMakePath(&fs, (UInt8*)dir, PATH_MAX)) {
		return "/tmp";
	}
	return fmt::format("{}/Logs/{}", dir, Core::AppInfo::APP_TITLE);
}

// XXX Can we use LocalizedString here instead?
#define LSTRING(b, x)                                                          \
	CFBundleCopyLocalizedString((b), CFStringCreateWithCString(kCFAllocatorDefault, x, kCFStringEncodingUTF8), NULL, CFSTR("Localizable"))

void
CrashHandler::InformUserOfCrash(const std::string& sPath)
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFStringRef sAlternate = LSTRING(bundle, fmt::format("Quit {}", Core::AppInfo::APP_TITLE).c_str());
	/* XXX Translate these and remove the redefine of LSTRING. Another way to do
	 * this would be to pass bundle's URL to CFUserNotificationDisplayAlert's
	 * localizationURL parameter and let it do it. This wouldn't work for sBody
	 * though. */
	CFStringRef sDefault = LSTRING(bundle, "File Bug Report");
	CFStringRef sOther = LSTRING(bundle, "Open crashinfo.txt");
	CFStringRef sTitle = LSTRING(bundle, fmt::format("{} has crashed", Core::AppInfo::APP_TITLE).c_str());
	CFStringRef sFormat = LSTRING(bundle, fmt::format("{} has crashed"
					 "Debugging information has been output to\n\n%s\n\n"
					 "Please file a bug report at\n\n%s", Core::AppInfo::APP_TITLE).c_str());
	CFStringRef sBody = CFStringCreateWithFormat(
	  kCFAllocatorDefault, NULL, sFormat, sPath.c_str(), Core::AppInfo::BUG_REPORT_URL);
	CFOptionFlags response = kCFUserNotificationCancelResponse;
	CFTimeInterval timeout = 0.0; // Should we ever time out?

	CFUserNotificationDisplayAlert(timeout,kCFUserNotificationStopAlertLevel,
								   NULL, NULL, NULL,
								   sTitle,
								   sBody,
								   sDefault,
								   sAlternate,
								   sOther,
								   &response);

	switch (response) {
		case kCFUserNotificationDefaultResponse:
			Core::Platform::openWebsite(Core::AppInfo::BUG_REPORT_URL);
			// Fall through.
		case kCFUserNotificationOtherResponse:
			// Open the file with the default application (probably TextEdit).
            Core::Platform::openWebsite("file://" + sPath);
			break;
	}
	CFRelease(sBody);
	CFRelease(sFormat);
	CFRelease(sTitle);
	CFRelease(sOther);
	CFRelease(sDefault);
	CFRelease(sAlternate);
}

/* IMPORTANT: Because the definition of the kinfo_proc structure (in
 * <sys/sysctl.h>) is conditionalized by __APPLE_API_UNSTABLE, you should
 * restrict use of the [below] code to the debug build of your program.
 * http://developer.apple.com/qa/qa2004/qa1361.html */
bool
CrashHandler::IsDebuggerPresent()
{
#ifdef DEBUG
	int ret;
	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
	struct kinfo_proc info;
	size_t size;

	// Initialize the flags so that, if sysctl fails for some bizarre
	// reason, we get a predictable result.

	info.kp_proc.p_flag = 0;

	// Call sysctl.
	size = sizeof(info);
	ret = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

	// We're being debugged if the P_TRACED flag is set.

	return ret == 0 && (info.kp_proc.p_flag & P_TRACED) != 0;
#else
	return false;
#endif
}

void
CrashHandler::DebugBreak()
{
	// TODO: Following command is depreciated.
	// DebugStr("\pDebugBreak()");
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
