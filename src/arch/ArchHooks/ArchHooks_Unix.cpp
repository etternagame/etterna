#include "Etterna/Globals/global.h"
#include "ArchHooks_Unix.h"
#include "Etterna/Globals/ProductInfo.h"
#include "RageUtil/Misc/RageLog.h"
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

#include "archutils/Unix/CrashHandler.h"
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
DoCrashSignalHandler(int signal, siginfo_t* si, const ucontext_t* uc)
{
	/* Don't dump a debug file if the user just hit ^C. */
	if (!IsFatalSignal(signal))
		return true;

	CrashHandler::CrashSignalHandler(signal, si, uc);
	return false;
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

RString
ArchHooks::GetPreferredLanguage()
{
	RString locale;

	if (getenv("LANG")) {
		locale = getenv("LANG");
		locale = locale.substr(0, 2);
	} else
		locale = "en";

	return locale;
}

void
ArchHooks_Unix::Init()
{
	/* First, handle non-fatal termination signals. */
	SignalHandler::OnClose(DoCleanShutdown);

	CrashHandler::CrashHandlerHandleArgs(g_argc, g_argv);
	CrashHandler::InitializeCrashHandler();
	SignalHandler::OnClose(DoCrashSignalHandler);

	/* Set up EmergencyShutdown, to try to shut down the window if we crash.
	 * This might blow up, so be sure to do it after the crash handler. */
	SignalHandler::OnClose(EmergencyShutdown);

	InstallExceptionHandler();

#if defined(HAVE_TLS)
	TestTLS();
#endif
}

bool
ArchHooks_Unix::GoToURL(const RString& sUrl)
{
	int status;
	pid_t p = fork();
	if (p == -1) {
		// Call to fork failed
		return false;
	} else if (p == 0) {
		// Child
		const char* const argv[] = { "xdg-open", sUrl.c_str(), NULL };
		execv("/usr/bin/xdg-open", const_cast<char* const*>(argv));
		// If we reach here, the call to execvp failed
		exit(1);
	} else {
		// Parent
		waitpid(p, &status, 0);
		return WEXITSTATUS(status) == 0;
	}
}

#ifndef _CS_GNU_LIBC_VERSION
#define _CS_GNU_LIBC_VERSION 2
#endif

static RString
LibcVersion()
{
	char buf[1024] = "(error)";
	int ret = confstr(_CS_GNU_LIBC_VERSION, buf, sizeof(buf));
	if (ret == -1)
		return "(unknown)";

	return buf;
}

void
ArchHooks_Unix::DumpDebugInfo()
{
	RString sys;
	int vers;
	GetKernel(sys, vers);
	LOG->Info("OS: %s ver %06i", sys.c_str(), vers);

	LOG->Info("Crash backtrace component: %s", BACKTRACE_METHOD_TEXT);
	LOG->Info("Crash lookup component: %s", BACKTRACE_LOOKUP_METHOD_TEXT);
#if defined(BACKTRACE_DEMANGLE_METHOD_TEXT)
	LOG->Info("Crash demangle component: %s", BACKTRACE_DEMANGLE_METHOD_TEXT);
#endif

	LOG->Info("Runtime library: %s", LibcVersion().c_str());
	LOG->Info("Threads library: %s", ThreadsVersion().c_str());
#if defined(HAVE_FFMPEG)
	LOG->Info("libavcodec: %#x (%u)", avcodec_version(), avcodec_version());
#endif
}

void
ArchHooks_Unix::SetTime(tm newtime)
{
	RString sCommand = ssprintf("date %02d%02d%02d%02d%04d.%02d",
								newtime.tm_mon + 1,
								newtime.tm_mday,
								newtime.tm_hour,
								newtime.tm_min,
								newtime.tm_year + 1900,
								newtime.tm_sec);

	LOG->Trace("executing '%s'", sCommand.c_str());
	int ret = system(sCommand);
	if (ret == -1 || ret == 127 || !WIFEXITED(ret) || WEXITSTATUS(ret))
		LOG->Trace("'%s' failed", sCommand.c_str());

	ret = system("hwclock --systohc");
	if (ret == -1 || ret == 127 || !WIFEXITED(ret) || WEXITSTATUS(ret))
		LOG->Trace("'hwclock --systohc' failed");
}

RString
ArchHooks_Unix::GetClipboard()
{
#ifdef HAVE_X11
	using namespace X11Helper;
	// Why isn't this defined by Xlib headers?
	Atom XA_CLIPBOARD = XInternAtom(Dpy, "CLIPBOARD", 0);
	Atom pstType;
	RString ret;
	unsigned char* paste;
	unsigned long remainder;
	int ck;
	Window selOwner;

	ASSERT(Win != None);
	// X11 is weird: instead of the clipboard being stored by the server
	// as you'd expect, it's actually a form of IPC, where we send
	// a request to the window we're pasting text from, and it sends
	// the paste data back.
	// XXX: X11 has TWO clipboards, the traditional Ctrl+C Ctrl+V,
	// and a second independent clipboard called the "primary selection",
	// set simply by selecting a span of text, and pasted with a
	// middle click. For now we just use the former, but this causes
	// problems in some cases because some (generally older) applications
	// only implement the latter.

	// Make sure someone has the clipboard first.
	selOwner = XGetSelectionOwner(Dpy, XA_CLIPBOARD);
	if (selOwner == None)
		// There is no clipboard right now.
		return "";

	// Tell the clipboard owner to give us the goods.
	// protip: XConvertSelection() puts the result of the request in a
	// property on YOUR window.
	XConvertSelection(
	  Dpy, XA_CLIPBOARD, XA_STRING, XA_PRIMARY, Win, CurrentTime);
	// XXX: This seems to always return 1 even when it works. (Success == 0)

	// Now we must wait for the clipboard owner to cough it up.
	// HACK: What we SHOULD do is XSelectInput() for SelectionNotify before
	// calling XConvertSelection and then block on XWindowEvent(), but that
	// would require significant cooperation with InputHandler_X11 through
	// X11Helper.
	// TODO: Said cooperation would be useful as then LowLevelWindow_X11
	// could listen for XFocusInEvent / XFocusOutEvent and send those.
	XSync(Dpy, false);
	usleep(10000);

	// Now retrieve the paste from our primary selection property.
	// Delete our primary selection so we'll just have null next time.
	int unused2; // XXX: "actual format of the property"? What?
	unsigned long unused3;
	ck = XGetWindowProperty(Dpy,
							Win,
							XA_PRIMARY,
							0,
							LONG_MAX,
							true,
							XA_STRING,
							&pstType,
							&unused2,
							&unused3,
							&remainder,
							&paste);
	if (ck != Success)
		// Selection doesn't exist
		return "";
	if (pstType != XA_STRING) {
		// Selection isn't stringable
		XFree(paste);
		return "";
	}

	ret = RString((char*)paste);
	XFree(paste);
	return ret;
#else
	LOG->Warn("ArchHooks_Unix: GetClipboard(): Compiled without any supported "
			  "clipboard source!");
	return "";
#endif
}

#include "RageUtil/File/RageFileManager.h"
#include <sys/stat.h>

static LocalizedString COULDNT_FIND_SONGS("ArchHooks_Unix",
										  "Couldn't find 'Songs'");
void
ArchHooks::MountInitialFilesystems(const RString& sDirOfExecutable)
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

	RString Root;
	struct stat st;
	if (!stat(sDirOfExecutable + "/Packages", &st) && st.st_mode & S_IFDIR)
		Root = sDirOfExecutable;
	else if (!stat(sDirOfExecutable + "/Songs", &st) && st.st_mode & S_IFDIR)
		Root = sDirOfExecutable;
	else if (!stat(RageFileManagerUtil::sInitialWorkingDirectory + "/Songs",
				   &st) &&
			 st.st_mode & S_IFDIR)
		Root = RageFileManagerUtil::sInitialWorkingDirectory;
	else
		RageException::Throw("%s", COULDNT_FIND_SONGS.GetValue().c_str());

	FILEMAN->Mount("dir", Root, "/");
}

void
ArchHooks::MountUserFilesystems(const RString& sDirOfExecutable)
{
	/* Path to write general mutable user data when not Portable
	 * Lowercase the PRODUCT_ID; dotfiles and directories are almost always
	 * lowercase.
	 */
	const char* szHome = getenv("HOME");
	RString sUserDataPath = ssprintf(
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
