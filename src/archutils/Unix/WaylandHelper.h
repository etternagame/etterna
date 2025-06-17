#ifndef WAYLAND_HELPER_H
#define WAYLAND_HELPER_H

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>
#include <map>
#include "archutils/Unix/WaylandProtocols/xdg-shell-client-protocol.h"

namespace WaylandHelper {

// The main connection to the wayland server
extern wl_display* display;
// The registry object used to get the compositor and other objects
extern wl_registry* registry;
// The compositor object used to create surfaces
extern wl_compositor* compositor;
// The shared memory object used to create a buffer for the mouse cursor
extern wl_shm* shm;

// The surface object used for the main window
extern wl_surface* surface;

// The xdg shell object used to create the xdg surface and toplevel
extern xdg_wm_base* wm_base;
// The xdg surface object used to create the window
extern xdg_surface* xdgsurface;
// The xdg toplevel object used to create the window
extern xdg_toplevel* xdgtoplevel;

// The seat object used to get the input devices
extern wl_seat* seat;
// The pointer and keyboard objects used to get the input events
extern wl_pointer* pointer;
// The keyboard object used to get the input events
extern wl_keyboard* keyboard;

// The surface containing the cursor image
extern wl_surface* cursor_surface;
// The cursor theme object used to load the cursor image
extern wl_cursor_theme* cursor_theme;
// The buffer actually containing the cursor image
extern wl_cursor_image* cursor_image;
// Empty surface for the purpose of hiding the cursor
extern wl_surface* cursor_none_surface;

/// Class for holding output information
class OutputInfo
{
  public:
	OutputInfo()
	  : name(nullptr)
	  , description(nullptr)
	  , width(0)
	  , height(0)
	  , rate(0)
	  , x(0)
	  , y(0)
	{
	}
	std::string* name;
	std::string* description;
	int32_t width;
	int32_t height;
	int32_t rate;
	int32_t x;
	int32_t y;
};

// store the known outputs
extern std::map<wl_output*, OutputInfo> outputs;

bool
OpenWaylandConnection();

bool
CloseWaylandConnection();

bool
MakeWindow(wl_egl_window*& win, int width, int height);

bool
DestroyWindow(wl_egl_window* win);

void
UpdateCursor(wl_pointer* pointer, uint32_t serial, bool show);
}

#endif