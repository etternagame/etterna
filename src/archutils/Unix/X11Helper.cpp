#include "Etterna/Globals/global.h"
#include "X11Helper.h"
#include "Core/Services/Locator.hpp"
#include "Core/Misc/AppInfo.hpp"
#include "Etterna/Models/Misc/Preference.h"
#include "Etterna/Singletons/PrefsManager.h" // XXX: only used for m_bShowMouseCursor -aj

#include <X11/extensions/dpms.h>

#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <cstdlib>

#include <csignal>

void
printCallStack()
{
	pid_t myPid = getpid();
	std::string pstackCommand = "pstack ";
	std::stringstream ss;
	ss << myPid;
	pstackCommand += ss.str();
	system(pstackCommand.c_str());
}

Display* X11Helper::Dpy = NULL;
Window X11Helper::Win = None;

static int
ErrorCallback(Display*, XErrorEvent*);
static int
FatalCallback(Display*);

static Preference<std::string> g_XWMName("XWMName", Core::AppInfo::APP_TITLE);

static bool display_supports_dpms_extension = false;
static bool dpms_state_at_startup = false;

bool
X11Helper::OpenXConnection()
{
	DEBUG_ASSERT(Dpy == NULL && Win == None);

	int res = XInitThreads();
	if (res == 0)
		return false;

	Dpy = XOpenDisplay(0);
	if (Dpy == NULL)
		return false;

	XSetIOErrorHandler(FatalCallback);
	XSetErrorHandler(ErrorCallback);
	int event_base, error_base;
	display_supports_dpms_extension =
	  DPMSQueryExtension(Dpy, &event_base, &error_base);
	if (display_supports_dpms_extension) {
		Locator::getLogger()->trace("DPMSQueryExtension returned true.  Stepmania will disable "
				   "power management, and restore the original state on exit.");
		CARD16 power_level;
		BOOL state;
		if (DPMSInfo(Dpy, &power_level, &state)) {
			dpms_state_at_startup = state;
			DPMSDisable(Dpy);
		} else {
			Locator::getLogger()->trace("DPMSInfo returned false.  Stepmania will not be able "
					   "to disable power management.");
		}
	} else {
		Locator::getLogger()->trace("DPMSQueryExtension returned false, which means this "
				   "display does not support the DPMS extension.  Stepmania "
				   "will not be able to disable power management.");
	}
	return true;
}

void
X11Helper::CloseXConnection()
{
	if (display_supports_dpms_extension) {
		if (dpms_state_at_startup) {
			DPMSEnable(Dpy);
		} else {
			DPMSDisable(Dpy);
		}
	}
	// The window should have been shut down
	DEBUG_ASSERT(Dpy != NULL);
	DEBUG_ASSERT(Win == None);
	XCloseDisplay(Dpy);
	Dpy = NULL;
}

bool
X11Helper::MakeWindow(Window& win,
					  int screenNum,
					  int depth,
					  Visual* visual,
					  int width,
					  int height,
					  bool overrideRedirect)
{
	if (!Dpy)
		return false;

	XSetWindowAttributes winAttribs;
	winAttribs.border_pixel = 0;
	winAttribs.event_mask = 0L;

	if (win) {
		// Preserve the event mask.
		XWindowAttributes attribs;
		XGetWindowAttributes(Dpy, win, &attribs);
		winAttribs.event_mask = attribs.your_event_mask;
		XDestroyWindow(Dpy, win);
	}
	// XXX: Error catching/handling?
	winAttribs.colormap =
	  XCreateColormap(Dpy, RootWindow(Dpy, screenNum), visual, AllocNone);
	unsigned long mask = CWBorderPixel | CWColormap | CWEventMask;

	if (overrideRedirect) {
		winAttribs.override_redirect = True;
		mask |= CWOverrideRedirect;
	}
	win = XCreateWindow(Dpy,
						RootWindow(Dpy, screenNum),
						0,
						0,
						width,
						height,
						0,
						depth,
						InputOutput,
						visual,
						mask,
						&winAttribs);
	if (win == None)
		return false;

	XClassHint* hint = XAllocClassHint();
	if (hint == NULL) {
		Locator::getLogger()->warn("Could not set class hint for X11 Window");
	} else {
		hint->res_name = (char*)g_XWMName.Get().c_str();
		hint->res_class = (char*)Core::AppInfo::APP_TITLE;
		XSetClassHint(Dpy, win, hint);
		XFree(hint);
	}

	// Hide the mouse cursor in certain situations.
	if (!PREFSMAN->m_bShowMouseCursor) {
		const char pBlank[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		Pixmap BlankBitmap = XCreateBitmapFromData(Dpy, win, pBlank, 8, 8);

		XColor black = { 0, 0, 0, 0, 0, 0 };
		Cursor pBlankPointer = XCreatePixmapCursor(
		  Dpy, BlankBitmap, BlankBitmap, &black, &black, 0, 0);
		XFreePixmap(Dpy, BlankBitmap);

		XDefineCursor(Dpy, win, pBlankPointer);
		XFreeCursor(Dpy, pBlankPointer);
	}

	return true;
}

void
X11Helper::SetWMState(const Window& root,
					  const Window& win,
					  const long action,
					  const Atom atom)
{
	if (!Dpy)
		return;
	Atom wm_state = XInternAtom(Dpy, "_NET_WM_STATE", False);
	XEvent xev;
	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = Win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = action; // 0 = Remove, 1 = Add, 2 = Toggle
	xev.xclient.data.l[1] = atom;
	xev.xclient.data.l[2] = 0; // end list of Atoms
	XSendEvent(Dpy,
			   root,
			   False,
			   SubstructureRedirectMask | SubstructureNotifyMask,
			   &xev);
}

int
ErrorCallback(Display* d, XErrorEvent* err)
{
	char errText[512];
	XGetErrorText(d, err->error_code, errText, 512);
	Locator::getLogger()->warn("X11 Protocol error {} ({}) has occurred, caused by request "
			  "{},{}, resource ID {}",
			  errText,
			  err->error_code,
			  err->request_code,
			  err->minor_code,
			  (int)err->resourceid);

	return 0; // Xlib ignores our return value
}

int
FatalCallback(Display* d)
{
	RageException::Throw("Fatal I/O error communicating with X server.");
}

#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>

bool
X11Helper::SetWMFullscreenMonitors(const DisplaySpec& target)
{
	int num_screens = 0;
	XineramaScreenInfo* screens = XineramaQueryScreens(Dpy, &num_screens);
	if (screens == nullptr) {
		return false;
	}

	XineramaScreenInfo* end = screens + num_screens;
	RectI monitors{};
	bool found_bounds = false;

	if (target.isVirtual()) {
		auto topmost = std::min_element(
		  screens, end, [](XineramaScreenInfo& a, XineramaScreenInfo& b) {
			  return a.y_org < b.y_org;
		  });
		monitors.top = topmost->screen_number;

		auto bottommost = std::max_element(
		  screens, end, [](XineramaScreenInfo& a, XineramaScreenInfo& b) {
			  return a.y_org < b.y_org;
		  });
		monitors.bottom = bottommost->screen_number;

		auto leftmost = std::min_element(
		  screens, end, [](XineramaScreenInfo& a, XineramaScreenInfo& b) {
			  return a.x_org < b.x_org;
		  });
		monitors.left = leftmost->screen_number;

		auto rightmost = std::max_element(
		  screens, end, [](XineramaScreenInfo& a, XineramaScreenInfo& b) {
			  return a.x_org < b.x_org;
		  });
		monitors.right = rightmost->screen_number;
		found_bounds = true;
	} else if (target.currentMode() != nullptr) {
		auto mon = std::find_if(screens, end, [&](XineramaScreenInfo& screen) {
			return screen.x_org == target.currentBounds().left &&
				   screen.y_org == target.currentBounds().top &&
				   screen.width == target.currentMode()->width &&
				   screen.height == target.currentMode()->height;
		});
		if (mon != end) {
			monitors.left = monitors.right = monitors.top = monitors.bottom =
			  mon->screen_number;
			found_bounds = true;
		}
	}

	XFree(screens);
	XWindowAttributes attr = { 0 };
	if (!found_bounds || !XGetWindowAttributes(Dpy, Win, &attr)) {
		return false;
	}

	SetWMState(
	  attr.root, Win, 1, XInternAtom(Dpy, "_NET_WM_STATE_FULLSCREEN", False));

	XClientMessageEvent xclient = { 0 };
	xclient.type = ClientMessage;
	xclient.window = Win;
	xclient.message_type =
	  XInternAtom(Dpy, "_NET_WM_FULLSCREEN_MONITORS", False);
	xclient.format = 32;
	xclient.data.l[0] = monitors.top;
	xclient.data.l[1] = monitors.bottom;
	xclient.data.l[2] = monitors.left;
	xclient.data.l[3] = monitors.right;
	xclient.data.l[4] = 1;
	XSendEvent(Dpy,
			   attr.root,
			   False,
			   SubstructureRedirectMask | SubstructureNotifyMask,
			   reinterpret_cast<XEvent*>(&xclient));
	XFlush(Dpy);

	return true;
}
#endif
