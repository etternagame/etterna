#include "Etterna/Globals/global.h"
#include "X11Helper.h"
#include "RageUtil/Misc/RageLog.h"
#include "Etterna/Globals/ProductInfo.h"
#include "Etterna/Models/Misc/Preference.h"
#include "Etterna/Singletons/PrefsManager.h" // XXX: only used for m_bShowMouseCursor -aj

#include <X11/extensions/dpms.h>

Display* X11Helper::Dpy = NULL;
Window X11Helper::Win = None;

static int
ErrorCallback(Display*, XErrorEvent*);
static int
FatalCallback(Display*);

static Preference<RString> g_XWMName("XWMName", PRODUCT_ID);

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
		LOG->Trace("DPMSQueryExtension returned true.  Stepmania will disable "
				   "power management, and restore the original state on exit.");
		CARD16 power_level;
		BOOL state;
		if (DPMSInfo(Dpy, &power_level, &state)) {
			dpms_state_at_startup = state;
			DPMSDisable(Dpy);
		} else {
			LOG->Trace("DPMSInfo returned false.  Stepmania will not be able "
					   "to disable power management.");
		}
	} else {
		LOG->Trace("DPMSQueryExtension returned false, which means this "
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
		LOG->Warn("Could not set class hint for X11 Window");
	} else {
		hint->res_name = (char*)g_XWMName.Get().c_str();
		hint->res_class = (char*)PRODUCT_FAMILY;
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

int
ErrorCallback(Display* d, XErrorEvent* err)
{
	char errText[512];
	XGetErrorText(d, err->error_code, errText, 512);
	LOG->Warn("X11 Protocol error %s (%d) has occurred, caused by request "
			  "%d,%d, resource ID %d",
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
