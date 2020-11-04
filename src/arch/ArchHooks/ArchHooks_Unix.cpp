#include "Etterna/Globals/global.h"
#include "ArchHooks_Unix.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Misc/RageThreads.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "archutils/Unix/SignalHandler.h"
#include "archutils/Unix/GetSysInfo.h"
#include "archutils/Common/PthreadHelpers.h"
#include "archutils/Unix/EmergencyShutdown.h"
#include "archutils/Unix/AssertionHandler.h"
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef __linux__
#include <limits.h>
#endif

#if defined(HAVE_FFMPEG)
extern "C" {
#include <libavcodec/avcodec.h>
}
#endif

#if defined(HAVE_X11)
#include "archutils/Unix/X11Helper.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#include <chrono>

static bool
IsFatalSignal(int signal)
{
	switch (signal) {
		case SIGINT:
		case SIGTERM:
		case SIGHUP:
			return false;
		default:
			return true;
	}
}

static bool
DoCleanShutdown(int signal, siginfo_t* si, const ucontext_t* uc)
{
	if (IsFatalSignal(signal))
		return false;

	/* ^C. */
	ArchHooks::SetUserQuit();
	return true;
}

static bool
EmergencyShutdown(int signal, siginfo_t* si, const ucontext_t* uc)
{
	if (!IsFatalSignal(signal))
		return false;

	DoEmergencyShutdown();

	/* If we ran the crash handler, then die. */
	kill(getpid(), SIGKILL);

	/* We didn't run the crash handler.  Run the default handler, so we can dump
	 * core. */
	return false;
}

#if defined(HAVE_TLS)
static thread_local int g_iTestTLS = 0;

static int
TestTLSThread(void* p)
{
	g_iTestTLS = 2;
	return 0;
}

static void
TestTLS()
{
#ifdef __linux__
	/* TLS won't work on older threads libraries, and may crash. */
	if (!UsingNPTL())
		return;
#endif
	/* TLS won't work on older Linux kernels.  Do a simple check. */
	g_iTestTLS = 1;

	RageThread TestThread;
	TestThread.SetName("TestTLS");
	TestThread.Create(TestTLSThread, NULL);
	TestThread.Wait();

	if (g_iTestTLS == 1)
		RageThread::SetSupportsTLS(true);
}
#endif

#if 1
/* If librt is available, use CLOCK_MONOTONIC to implement
 * GetMicrosecondsSinceStart, if supported, so changes to the system clock don't
 * cause problems. */
namespace {
clockid_t g_Clock = CLOCK_REALTIME;

void
OpenGetTime()
{
	static bool bInitialized = false;
	if (bInitialized)
		return;
	bInitialized = true;

	/* Check whether the clock is actually supported. */
	timespec ts;
	if (clock_getres(CLOCK_MONOTONIC, &ts) == -1)
		return;

	/* If the resolution is worse than a millisecond, fall back on
	 * CLOCK_REALTIME. */
	if (ts.tv_sec > 0 || ts.tv_nsec > 1000000)
		return;

	g_Clock = CLOCK_MONOTONIC;
}
};

clockid_t
ArchHooks_Unix::GetClock()
{
	OpenGetTime();
	return g_Clock;
}

int64_t
ArchHooks::GetMicrosecondsSinceStart()
{
	OpenGetTime();

	timespec ts;
	clock_gettime(g_Clock, &ts);

	int64_t iRet = int64_t(ts.tv_sec) * 1000000 + int64_t(ts.tv_nsec) / 1000;
	if (g_Clock != CLOCK_MONOTONIC)
		iRet = ArchHooks::FixupTimeIfBackwards(iRet);
	return iRet;
}
std::chrono::microseconds
ArchHooks::GetChronoDurationSinceStart()
{
	return std::chrono::microseconds(GetMicrosecondsSinceStart());
}
#else
int64_t
ArchHooks::GetMicrosecondsSinceStart()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	int64_t iRet = int64_t(tv.tv_sec) * 1000000 + int64_t(tv.tv_usec);
	ret = FixupTimeIfBackwards(ret);
	return iRet;
}
#endif

void
ArchHooks_Unix::Init()
{
	/* First, handle non-fatal termination signals. */
	SignalHandler::OnClose(DoCleanShutdown);

	/* Set up EmergencyShutdown, to try to shut down the window if we crash.
	 * This might blow up, so be sure to do it after the crash handler. */
	SignalHandler::OnClose(EmergencyShutdown);

	InstallExceptionHandler();

#if defined(HAVE_TLS)
	TestTLS();
#endif
}

#ifndef _CS_GNU_LIBC_VERSION
#define _CS_GNU_LIBC_VERSION 2
#endif

static std::string
LibcVersion()
{
	char buf[1024] = "(error)";
	int ret = confstr(_CS_GNU_LIBC_VERSION, buf, sizeof(buf));
	if (ret == -1)
		return "(unknown)";

	return buf;
}

#include "RageUtil/File/RageFileManager.h"
#include <sys/stat.h>

static LocalizedString COULDNT_FIND_SONGS("ArchHooks_Unix",
										  "Couldn't find 'Songs'");
void
ArchHooks::MountInitialFilesystems(const std::string& sDirOfExecutable)
{
#ifdef __unix__
	/* Mount the root filesystem, so we can read files in /proc, /etc, and so
	 * on. This is /rootfs, not /root, to avoid confusion with root's home
	 * directory. */
	FILEMAN->Mount("dir", "/", "/rootfs");

	/* Mount /proc, so Alsa9Buf::GetSoundCardDebugInfo() and others can access
	 * it. (Deprecated; use rootfs.) */
	FILEMAN->Mount("dir", "/proc", "/proc");
#endif

	std::string Root;
	struct stat st;
	if (!stat((sDirOfExecutable + "/Packages").c_str(), &st) && st.st_mode & S_IFDIR)
		Root = sDirOfExecutable;
	else if (!stat((sDirOfExecutable + "/Songs").c_str(), &st) && st.st_mode & S_IFDIR)
		Root = sDirOfExecutable;
	else if (!stat((RageFileManagerUtil::sInitialWorkingDirectory + "/Songs").c_str(),
				   &st) &&
			 st.st_mode & S_IFDIR)
		Root = RageFileManagerUtil::sInitialWorkingDirectory;
	else
		RageException::Throw("%s", COULDNT_FIND_SONGS.GetValue().c_str());

	FILEMAN->Mount("dir", Root, "/");
}

void
ArchHooks::MountUserFilesystems(const std::string& sDirOfExecutable)
{
	/* Path to write general mutable user data when not Portable
	 * Lowercase the PRODUCT_ID; dotfiles and directories are almost always
	 * lowercase.
	 */
	const char* szHome = getenv("HOME");
	std::string sUserDataPath = ssprintf(
	  "%s/.%s", szHome ? szHome : ".", "stepmania-5.0"); // call an ambulance!
	FILEMAN->Mount("dir", sUserDataPath + "/Announcers", "/Announcers");
	FILEMAN->Mount("dir", sUserDataPath + "/BGAnimations", "/BGAnimations");
	FILEMAN->Mount(
	  "dir", sUserDataPath + "/BackgroundEffects", "/BackgroundEffects");
	FILEMAN->Mount("dir",
				   sUserDataPath + "/BackgroundTransitions",
				   "/BackgroundTransitions");
	FILEMAN->Mount("dir", sUserDataPath + "/Cache", "/Cache");
	FILEMAN->Mount("dir", sUserDataPath + "/CDTitles", "/CDTitles");
	FILEMAN->Mount("dir", sUserDataPath + "/Characters", "/Characters");
	FILEMAN->Mount("dir", sUserDataPath + "/Courses", "/Courses");
	FILEMAN->Mount("dir", sUserDataPath + "/Logs", "/Logs");
	FILEMAN->Mount("dir", sUserDataPath + "/NoteSkins", "/NoteSkins");
	FILEMAN->Mount("dir",
				   sUserDataPath + "/Packages",
				   "/" + SpecialFiles::USER_PACKAGES_DIR);
	FILEMAN->Mount("dir", sUserDataPath + "/Save", "/Save");
	FILEMAN->Mount("dir", sUserDataPath + "/Screenshots", "/Screenshots");
	FILEMAN->Mount("dir", sUserDataPath + "/Songs", "/Songs");
	FILEMAN->Mount("dir", sUserDataPath + "/RandomMovies", "/RandomMovies");
	FILEMAN->Mount("dir", sUserDataPath + "/Themes", "/Themes");
}
