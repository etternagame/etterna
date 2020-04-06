/* Manages our X connection and window. */
#ifndef X11_HELPER_H
#define X11_HELPER_H

#include <X11/Xlib.h> // Window
#include <X11/Xutil.h>
#include "Etterna/Models/Misc/DisplaySpec.h"
namespace X11Helper {
// All functions in here that return a bool return true on success, and
// false on failure.

// Create the connection.
bool
OpenXConnection();

// Destroy the connection.
void
CloseXConnection();

// The current Display (connection). Initialized by the first call to
// OpenXConnection().
extern Display* Dpy;

// The Window used by LowLevelWindow_X11 as the main window.
extern Window Win;

// (Re)create the Window win.
bool
MakeWindow(Window& win,
		   int screenNum,
		   int depth,
		   Visual* visual,
		   int width,
		   int height,
		   bool overrideRedirect);
void
SetWMState(const Window& root,
		   const Window& win,
		   const long action,
		   const Atom atom);

#ifdef HAVE_XINERAMA
bool
SetWMFullscreenMonitors(const DisplaySpec& target);
#endif
};

#endif
