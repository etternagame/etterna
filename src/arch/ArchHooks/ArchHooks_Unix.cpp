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
#include "Etterna/Globals/GameLoop.h"

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
	GameLoop::setUserQuit();
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


void
ArchHooks_Unix::Init()
{
	/* First, handle non-fatal termination signals. */
	SignalHandler::OnClose(DoCleanShutdown);

	/* Set up EmergencyShutdown, to try to shut down the window if we crash.
	 * This might blow up, so be sure to do it after the crash handler. */
	SignalHandler::OnClose(EmergencyShutdown);

	InstallExceptionHandler();
}