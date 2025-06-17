#include "Core/Services/Locator.hpp"
#include "Etterna/Globals/GameLoop.h"

#include "WaylandHelper.h"
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>

#include <EGL/egl.h>

// variables used for the wayland connection
wl_display* WaylandHelper::display = nullptr;
wl_registry* WaylandHelper::registry = nullptr;
wl_compositor* WaylandHelper::compositor = nullptr;
wl_shm* WaylandHelper::shm = nullptr;

// variables used to represent the window
wl_surface* WaylandHelper::surface = nullptr;

// varaibles used by the xdg shell
xdg_wm_base* WaylandHelper::wm_base = nullptr;
xdg_surface* WaylandHelper::xdgsurface = nullptr;
xdg_toplevel* WaylandHelper::xdgtoplevel = nullptr;

// variables used for input
wl_seat* WaylandHelper::seat = nullptr;
wl_pointer* WaylandHelper::pointer = nullptr;
wl_keyboard* WaylandHelper::keyboard = nullptr;

// buffer used to store the cursor image
wl_surface* WaylandHelper::cursor_surface = nullptr;
wl_cursor_theme* WaylandHelper::cursor_theme = nullptr;
wl_cursor_image* WaylandHelper::cursor_image = nullptr;
wl_surface* WaylandHelper::cursor_none_surface = nullptr;

// store the known outputs
std::map<wl_output*, WaylandHelper::OutputInfo> WaylandHelper::outputs = {};

// Listen for the wayland seat devices and capture the pointer and keyboard
static void
wl_seat_handle_capabilities(void* data, struct wl_seat* seat, uint32_t caps)
{
	if (caps & WL_SEAT_CAPABILITY_POINTER &&
		WaylandHelper::pointer == nullptr) {
		Locator::getLogger()->info("Wayland seat has pointer capability");
		WaylandHelper::pointer = (wl_pointer*)wl_seat_get_pointer(seat);

		// set the cursor to the default image
		WaylandHelper::UpdateCursor(WaylandHelper::pointer, 0, true);
	}
	if (caps & WL_SEAT_CAPABILITY_KEYBOARD &&
		WaylandHelper::keyboard == nullptr) {
		Locator::getLogger()->info("Wayland seat has keyboard capability");
		WaylandHelper::keyboard = (wl_keyboard*)wl_seat_get_keyboard(seat);
	}
}

static void
wl_seat_name(void* data, struct wl_seat* seat, const char* name)
{
	// ignored event
}

// seat event listener
static const struct wl_seat_listener wl_seat_listener = {
	.capabilities = wl_seat_handle_capabilities,
	.name = wl_seat_name,
};

// This gives us the output geometry
// This mainly gives us the position in compositor space
static void
wl_output_geometry(void* data,
				   struct wl_output* output,
				   int32_t x,
				   int32_t y,
				   int32_t physical_width,
				   int32_t physical_height,
				   int32_t subpixel,
				   const char* make,
				   const char* model,
				   int32_t transform)
{
	Locator::getLogger()->info("Wayland output geometry: {} {} {} {}",
							   x,
							   y,
							   physical_width,
							   physical_height);

	WaylandHelper::OutputInfo* poif = (WaylandHelper::OutputInfo*)data;

	poif->x = x;
	poif->y = y;
}

// This gives us the output mode
// Should fill in the width, height and refresh rate
static void
wl_output_mode(void* data,
			   struct wl_output* output,
			   uint32_t flags,
			   int32_t width,
			   int32_t height,
			   int32_t refresh)
{
	WaylandHelper::OutputInfo* poif = (WaylandHelper::OutputInfo*)data;

	poif->width = width;
	poif->height = height;
	poif->rate = refresh;
}

// This is called when the compositor has finished broadcasting the output
// information and saves the finished output information into the map
static void
wl_output_done(void* data, struct wl_output* output)
{
	WaylandHelper::OutputInfo* poif = (WaylandHelper::OutputInfo*)data;

	Locator::getLogger()->info(
	  "Wayland output done: {} {} {}", poif->width, poif->height, poif->rate);

	WaylandHelper::outputs[output] = *poif;
}

static void
wl_output_scale(void* data, struct wl_output* output, int32_t factor)
{
	// ignored event
}

// This gives the output name eg. "DP-1"
static void
wl_output_name(void* data, struct wl_output* output, const char* name)
{
	WaylandHelper::OutputInfo* poif = (WaylandHelper::OutputInfo*)data;

	poif->name = new std::string(name);
}

// This give the output description eg. "Acme Inc. 27 inch monitor"
static void
wl_output_description(void* data,
					  struct wl_output* output,
					  const char* description)
{
	WaylandHelper::OutputInfo* poif = (WaylandHelper::OutputInfo*)data;

	poif->description = new std::string(description);
}

// Listen for the wayland output events
// this is used to get the display information for fullscreen outputs
static const struct wl_output_listener wl_output_listener = {
	.geometry = wl_output_geometry,
	.mode = wl_output_mode,
	.done = wl_output_done,
	.scale = wl_output_scale,
	.name = wl_output_name,
	.description = wl_output_description,
};

// The global registry handler listens for registry events
static void
global_registry_handler(void* data,
						struct wl_registry* registry,
						uint32_t name,
						const char* interface,
						uint32_t version)
{
	Locator::getLogger()->info("Global object: {} id: {}", interface, name);

	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		// The compositor object is used to create surfaces
		WaylandHelper::compositor = (wl_compositor*)wl_registry_bind(
		  registry, name, &wl_compositor_interface, 5);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		// The xdg_wm_base gives access to the xdg shell, which is used to tell
		// the shell details about the window
		WaylandHelper::wm_base = (xdg_wm_base*)wl_registry_bind(
		  registry, name, &xdg_wm_base_interface, 6);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		// The seat interface handles input devices
		WaylandHelper::seat =
		  (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 7);

		// We will also listen for the seat events
		wl_seat_add_listener(WaylandHelper::seat, &wl_seat_listener, nullptr);
	} else if (strcmp(interface, wl_output_interface.name) == 0) {
		// Outputs are used to get the display information
		wl_output* output =
		  (wl_output*)wl_registry_bind(registry, name, &wl_output_interface, 4);

		// Create a new output info object to store the information
		WaylandHelper::OutputInfo* info = new WaylandHelper::OutputInfo();

		// Add a listener to populate the output info
		wl_output_add_listener(output, &wl_output_listener, info);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		// The shared memory object is used to create a buffer for the mouse
		// cursor
		WaylandHelper::shm =
		  (wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}
}

// this is required to tell WMs that we are alive otherwise they will consider
// the window to be "not responding"
static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = [](void* data,
			   struct xdg_wm_base* xdg_wm_base,
			   uint32_t serial) { xdg_wm_base_pong(xdg_wm_base, serial); },
};

const struct wl_registry_listener registry_listener = {
	.global = global_registry_handler,
	.global_remove =
	  [](void* data, struct wl_registry* registry, uint32_t name) {
		  // ignored event
	  },
};

// According to the xdg shell protocol, we must ack configure events
// not much is done with these events
static const struct xdg_surface_listener xdg_surface_listener = {
	.configure =
	  [](void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
		  xdg_surface_ack_configure(xdg_surface, serial);
		  wl_surface_commit(WaylandHelper::surface);
	  },
};

// Listen for xdg toplevel events
// This is mainly used to listen for the close event
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	// we are not interested in being configured by the compositor so ignore the
	// event by doing nothing
	.configure = [](void* data,
					struct xdg_toplevel* xdg_toplevel,
					int32_t width,
					int32_t height,
					struct wl_array* states) {},

	.close = [](void* data,
				struct xdg_toplevel* xdg_toplevel) { GameLoop::setUserQuit(); },

	// ignore the rest of the events as we aren't interested in them
	.configure_bounds = [](void* data,
						   struct xdg_toplevel* xdg_toplevel,
						   int32_t width,
						   int32_t height) {},
	.wm_capabilities = [](void* data,
						  struct xdg_toplevel* xdg_toplevel,
						  wl_array* capabilities) {},
};

/// @brief  Open a connection to the Wayland display server
/// @return Whether the connection was successful or not
bool
WaylandHelper::OpenWaylandConnection()
{
	// get the wayland display
	display = wl_display_connect(nullptr);
	if (display == nullptr) {
		Locator::getLogger()->error("Failed to connect to Wayland display");
		return false;
	}

	// get the wayland registry
	registry = wl_display_get_registry(display);
	if (registry == nullptr) {
		Locator::getLogger()->error("Failed to get Wayland registry");
		wl_display_disconnect(display);
		return false;
	}

	// add the registry listener
	wl_registry_add_listener(registry, &registry_listener, nullptr);

	// roundtrip to synchronize
	wl_display_roundtrip(display);

	// we should have the compositor now
	if (compositor == nullptr) {
		Locator::getLogger()->error("Failed to get Wayland compositor");
		wl_display_disconnect(display);
		return false;
	}

	// we should also have the xdg shell
	if (wm_base == nullptr) {
		Locator::getLogger()->error("Failed to get Wayland xdg shell");
		wl_display_disconnect(display);
		return false;
	}

	// register the xdg shell listener
	// this is required to tell WMs that we are alive otherwise they will
	// consider the window to be "not responding"
	xdg_wm_base_add_listener(wm_base, &xdg_wm_base_listener, nullptr);

	// load the cursor image from the user's cursor theme
	cursor_theme = wl_cursor_theme_load(nullptr, 24, shm);
	struct wl_cursor* cursor =
	  wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");
	cursor_image = cursor->images[0];
	struct wl_buffer* cursor_buffer = wl_cursor_image_get_buffer(cursor_image);

	cursor_surface = wl_compositor_create_surface(compositor);
	wl_surface_attach(cursor_surface, cursor_buffer, 0, 0);
	wl_surface_commit(cursor_surface);

	cursor_none_surface = wl_compositor_create_surface(compositor);

	// second roundtrip to get the seat
	wl_display_roundtrip(display);

	// we should also have the seat
	if (seat == nullptr) {
		Locator::getLogger()->error("Failed to get Wayland seat");
		wl_display_disconnect(display);
		return false;
	}

	// create a surface
	surface = wl_compositor_create_surface(compositor);
	if (surface == nullptr) {
		Locator::getLogger()->error("Failed to create Wayland surface");
		wl_display_disconnect(display);
		return false;
	}

	// create the xdg surface
	xdgsurface = xdg_wm_base_get_xdg_surface(wm_base, surface);

	// listen for xdg surface events so we can ack the configure event
	xdg_surface_add_listener(xdgsurface, &xdg_surface_listener, nullptr);

	// get the xdg toplevel
	xdgtoplevel = xdg_surface_get_toplevel(xdgsurface);

	// add the toplevel listener to handle the close event
	xdg_toplevel_add_listener(xdgtoplevel, &xdg_toplevel_listener, nullptr);

	wl_surface_commit(surface);

	return true;
}

/// @brief  Close the Wayland connection
/// @return Whether the connection was closed successfully or not
bool
WaylandHelper::CloseWaylandConnection()
{
	// free the output info
	for (auto& output : WaylandHelper::outputs) {
		wl_output_destroy(output.first);

		delete output.second.name;
		delete output.second.description;
	}

	// free the cursor related objects
	if (cursor_surface != nullptr) {
		wl_surface_destroy(cursor_surface);
		cursor_surface = nullptr;
	}
	if (cursor_none_surface != nullptr) {
		wl_surface_destroy(cursor_none_surface);
		cursor_none_surface = nullptr;
	}
	if (cursor_theme != nullptr) {
		wl_cursor_theme_destroy(cursor_theme);
		cursor_theme = nullptr;
		cursor_image = nullptr;
	}

	// input
	if (pointer != nullptr) {
		wl_pointer_release(pointer);
		pointer = nullptr;
	}
	if (keyboard != nullptr) {
		wl_keyboard_release(keyboard);
		keyboard = nullptr;
	}
	if (seat != nullptr) {
		wl_seat_release(seat);
		seat = nullptr;
	}

	// xdg
	if (xdgtoplevel != nullptr) {
		xdg_toplevel_destroy(xdgtoplevel);
		xdgtoplevel = nullptr;
	}
	if (xdgsurface != nullptr) {
		xdg_surface_destroy(xdgsurface);
		xdgsurface = nullptr;
	}
	if (wm_base != nullptr) {
		xdg_wm_base_destroy(wm_base);
		wm_base = nullptr;
	}

	// surface
	if (surface != nullptr) {
		wl_surface_destroy(surface);
		surface = nullptr;
	}

	// compositor
	if (compositor != nullptr) {
		wl_compositor_destroy(compositor);
		compositor = nullptr;
	}

	// shm
	if (shm != nullptr) {
		wl_shm_destroy(shm);
		shm = nullptr;
	}

	// registry
	if (registry != nullptr) {
		wl_registry_destroy(registry);
		registry = nullptr;
	}

	// finally disconnect from the display
	if (display != nullptr) {
		wl_display_disconnect(display);
		display = nullptr;
	}

	return true;
}

bool
WaylandHelper::MakeWindow(wl_egl_window*& win, int width, int height)
{
	wl_region* region = wl_compositor_create_region(compositor);

	wl_region_add(region, 0, 0, width, height);
	wl_surface_set_opaque_region(surface, region);

	win = wl_egl_window_create(surface, width, height);

	wl_region_destroy(region);

	if (win == EGL_NO_SURFACE) {
		Locator::getLogger()->error("Failed to create EGL window");
		return false;
	}

	wl_surface_commit(surface);

	return true;
}

bool
WaylandHelper::DestroyWindow(wl_egl_window* win)
{
	if (win != nullptr) {

		wl_egl_window_destroy(win);
		return true;
	}

	return false;
}

void
WaylandHelper::UpdateCursor(wl_pointer* pointer, uint32_t serial, bool show)
{
	if (show) {
		wl_pointer_set_cursor(pointer,
							  serial,
							  cursor_surface,
							  cursor_image->hotspot_x,
							  cursor_image->hotspot_y);
	} else {
		wl_pointer_set_cursor(pointer, serial, cursor_none_surface, 0, 0);
	}
}