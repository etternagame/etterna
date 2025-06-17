#include "Etterna/Globals/global.h"
#include "InputHandler_Wayland.h"

#include "archutils/Unix/WaylandHelper.h"

#include "Etterna/Singletons/InputFilter.h"
#include "Core/Services/Locator.hpp"

#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <sys/mman.h>

using namespace WaylandHelper;

REGISTER_INPUT_HANDLER_CLASS(Wayland);

static DeviceButton
MapKeys(xkb_keysym_t sym)
{
	if (sym < 128) {
		return static_cast<DeviceButton>(sym);
	}

	switch (sym) {
		/* These are needed because of the way X registers the keypad. */
		case XKB_KEY_BackSpace:
			return KEY_BACK;
		case XKB_KEY_Tab:
			return KEY_TAB;
		case XKB_KEY_Pause:
			return KEY_PAUSE;
		case XKB_KEY_Escape:
			return KEY_ESC;
		case XKB_KEY_KP_Insert:
			return KEY_KP_C0;
		case XKB_KEY_KP_End:
			return KEY_KP_C1;
		case XKB_KEY_KP_Down:
			return KEY_KP_C2;
		case XKB_KEY_KP_Page_Down:
			return KEY_KP_C3;
		case XKB_KEY_KP_Left:
			return KEY_KP_C4;
		case XKB_KEY_KP_Begin:
			return KEY_KP_C5;
		case XKB_KEY_KP_Right:
			return KEY_KP_C6;
		case XKB_KEY_KP_Home:
			return KEY_KP_C7;
		case XKB_KEY_KP_Up:
			return KEY_KP_C8;
		case XKB_KEY_KP_Page_Up:
			return KEY_KP_C9;
		case XKB_KEY_KP_Decimal:
			return KEY_KP_PERIOD;
		case XKB_KEY_KP_Divide:
			return KEY_KP_SLASH;
		case XKB_KEY_KP_Multiply:
			return KEY_KP_ASTERISK;
		case XKB_KEY_KP_Subtract:
			return KEY_KP_HYPHEN;
		case XKB_KEY_KP_Add:
			return KEY_KP_PLUS;
		case XKB_KEY_KP_Equal:
			return KEY_KP_EQUAL;
		case XKB_KEY_KP_Enter:
			return KEY_KP_ENTER;
		case XKB_KEY_Up:
			return KEY_UP;
		case XKB_KEY_Down:
			return KEY_DOWN;
		case XKB_KEY_Right:
			return KEY_RIGHT;
		case XKB_KEY_Left:
			return KEY_LEFT;
		case XKB_KEY_Insert:
			return KEY_INSERT;
		case XKB_KEY_Home:
			return KEY_HOME;
		case XKB_KEY_Delete:
			return KEY_DEL;
		case XKB_KEY_End:
			return KEY_END;
		case XKB_KEY_Page_Up:
			return KEY_PGUP;
		case XKB_KEY_Page_Down:
			return KEY_PGDN;
		case XKB_KEY_F1:
			return KEY_F1;
		case XKB_KEY_F2:
			return KEY_F2;
		case XKB_KEY_F3:
			return KEY_F3;
		case XKB_KEY_F4:
			return KEY_F4;
		case XKB_KEY_F5:
			return KEY_F5;
		case XKB_KEY_F6:
			return KEY_F6;
		case XKB_KEY_F7:
			return KEY_F7;
		case XKB_KEY_F8:
			return KEY_F8;
		case XKB_KEY_F9:
			return KEY_F9;
		case XKB_KEY_F10:
			return KEY_F10;
		case XKB_KEY_F11:
			return KEY_F11;
		case XKB_KEY_F12:
			return KEY_F12;
		case XKB_KEY_F13:
			return KEY_F13;
		case XKB_KEY_F14:
			return KEY_F14;
		case XKB_KEY_F15:
			return KEY_F15;

		case XKB_KEY_Num_Lock:
			return KEY_NUMLOCK;
		case XKB_KEY_Caps_Lock:
			return KEY_CAPSLOCK;
		case XKB_KEY_Scroll_Lock:
			return KEY_SCRLLOCK;
		case XKB_KEY_Return:
			return KEY_ENTER;
		case XKB_KEY_Sys_Req:
			return KEY_PRTSC;
		case XKB_KEY_Print:
			return KEY_PRTSC;
		case XKB_KEY_Shift_R:
			return KEY_RSHIFT;
		case XKB_KEY_Shift_L:
			return KEY_LSHIFT;
		case XKB_KEY_Control_R:
			return KEY_RCTRL;
		case XKB_KEY_Control_L:
			return KEY_LCTRL;
		case XKB_KEY_Alt_R:
			return KEY_RALT;
		case XKB_KEY_Alt_L:
			return KEY_LALT;
		case XKB_KEY_Meta_R:
			return KEY_RMETA;
		case XKB_KEY_Meta_L:
			return KEY_LMETA;
		case XKB_KEY_Super_L:
			return KEY_LSUPER;
		case XKB_KEY_Super_R:
			return KEY_RSUPER;
		case XKB_KEY_Menu:
			return KEY_MENU;

		// mouse
		case XKB_KEY_Pointer_Button1:
			return MOUSE_LEFT;
		case XKB_KEY_Pointer_Button2:
			return MOUSE_MIDDLE;
		case XKB_KEY_Pointer_Button3:
			return MOUSE_RIGHT;
		case XKB_KEY_Pointer_Button4:
			return MOUSE_WHEELUP;
		case XKB_KEY_Pointer_Button5:
			return MOUSE_WHEELDOWN;
	}

	Locator::getLogger()->warn("Unknown key: {}", sym);

	return DeviceButton_Invalid;
}

static void
wl_keyboard_keymap(void* data,
				   struct wl_keyboard* keyboard,
				   uint32_t format,
				   int fd,
				   uint32_t size)
{
	InputHandler_Wayland* pthis = static_cast<InputHandler_Wayland*>(data);
	Locator::getLogger()->info("Keymap format: {}", format);
	ASSERT(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

	char* map_shm = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	ASSERT(map_shm != MAP_FAILED);

	// Create a new xkb keymap from the shared memory
	// TODO: load us layout instead of the user layout since that is what the
	// engine wants?
	struct xkb_keymap* keymap =
	  xkb_keymap_new_from_string(pthis->xkbContext,
								 map_shm,
								 XKB_KEYMAP_FORMAT_TEXT_V1,
								 XKB_KEYMAP_COMPILE_NO_FLAGS);

	if (keymap == nullptr) {
		Locator::getLogger()->error("Failed to create xkb keymap");
		munmap(map_shm, size);
		close(fd);
		return;
	}

	pthis->keymap = keymap;

	pthis->xkbState = xkb_state_new(pthis->keymap);
	munmap(map_shm, size);
	close(fd);
}

static void
wl_keyboard_enter(void* data,
				  struct wl_keyboard* keyboard,
				  uint32_t serial,
				  struct wl_surface* surface,
				  struct wl_array* keys)
{
	// ignored event
}

static void
wl_keyboard_leave(void* data,
				  struct wl_keyboard* keyboard,
				  uint32_t serial,
				  struct wl_surface* surface)
{
	// ignored event
}

static void
wl_keyboard_key(void* data,
				struct wl_keyboard* keyboard,
				uint32_t serial,
				uint32_t time,
				uint32_t key,
				uint32_t state)
{
	InputHandler_Wayland* pthis = static_cast<InputHandler_Wayland*>(data);

	// Map the scancode to a key symbol
	// this mostly works, however some keys are not mapped correctly on non us
	// layouts and end up being mapped to DeviceButton_Invalid
	xkb_keysym_t sym = xkb_state_key_get_one_sym(pthis->xkbState, key + 8);

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		pthis->RegisterKeyEvent(time, true, DEVICE_KEYBOARD, MapKeys(sym));
	} else {
		pthis->RegisterKeyEvent(time, false, DEVICE_KEYBOARD, MapKeys(sym));
	}
}

static void
wl_keyboard_modifiers(void* data,
					  struct wl_keyboard* keyboard,
					  uint32_t serial,
					  uint32_t mods_depressed,
					  uint32_t mods_latched,
					  uint32_t mods_locked,
					  uint32_t group)
{
	// ignored event
}

static void
wl_keyboard_repeat_info(void* data,
						struct wl_keyboard* keyboard,
						int32_t rate,
						int32_t delay)
{
	// ignored event
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
	.keymap = wl_keyboard_keymap,
	.enter = wl_keyboard_enter,
	.leave = wl_keyboard_leave,
	.key = wl_keyboard_key,
	.modifiers = wl_keyboard_modifiers,
	.repeat_info = wl_keyboard_repeat_info,
};

static void
wl_pointer_enter(void* data,
				 struct wl_pointer* pointer,
				 uint32_t serial,
				 struct wl_surface* surface,
				 wl_fixed_t surface_x,
				 wl_fixed_t surface_y)
{
	Locator::getLogger()->info("Pointer entered surface");

	// set the cursor used by the pointer
	WaylandHelper::UpdateCursor(
	  WaylandHelper::pointer, serial, WLshouldShowCursor);
}

static void
wl_pointer_leave(void* data,
				 struct wl_pointer* pointer,
				 uint32_t serial,
				 struct wl_surface* surface)
{
	Locator::getLogger()->info("Pointer left surface");
	INPUTFILTER->Reset();
}

static void
wl_pointer_motion(void* data,
				  struct wl_pointer* pointer,
				  uint32_t time,
				  wl_fixed_t surface_x,
				  wl_fixed_t surface_y)
{
	float x = (float)wl_fixed_to_double(surface_x);
	float y = (float)wl_fixed_to_double(surface_y);

	INPUTFILTER->UpdateCursorLocation(x, y);
}

static void
wl_pointer_button(void* data,
				  struct wl_pointer* pointer,
				  uint32_t serial,
				  uint32_t time,
				  uint32_t button,
				  uint32_t state)
{
	InputHandler_Wayland* pthis = static_cast<InputHandler_Wayland*>(data);

	DeviceButton db = DeviceButton_Invalid;

	switch (button) {
		case 272:
			db = MOUSE_LEFT;
			break;
		case 273:
			db = MOUSE_RIGHT;
			break;
		case 274:
			db = MOUSE_MIDDLE;
			break;
	}

	pthis->RegisterKeyEvent(
	  time, state == WL_POINTER_BUTTON_STATE_PRESSED, DEVICE_MOUSE, db);
}

static void
wl_pointer_axis(void* data,
				struct wl_pointer* pointer,
				uint32_t time,
				uint32_t axis,
				wl_fixed_t value)
{
	InputHandler_Wayland* pthis = static_cast<InputHandler_Wayland*>(data);

	// scroll up = negative
	// scroll down = positive

	Locator::getLogger()->info("Pointer axis {} value {}", axis, value);

	if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
		if (value < 0) {
			INPUTFILTER->UpdateMouseWheel(INPUTFILTER->GetMouseWheel() + 1);

			// currently this sends two events to simulate a "keystroke" as that
			// is how the engine handles it. Wayland doesnt seem to have a
			// reliable event that could be used for the "keyup" so just send 2
			// events
			pthis->RegisterKeyEvent(time, true, DEVICE_MOUSE, MOUSE_WHEELUP);
			pthis->RegisterKeyEvent(time, false, DEVICE_MOUSE, MOUSE_WHEELUP);
		} else {
			INPUTFILTER->UpdateMouseWheel(INPUTFILTER->GetMouseWheel() - 1);
			pthis->RegisterKeyEvent(time, true, DEVICE_MOUSE, MOUSE_WHEELDOWN);
			pthis->RegisterKeyEvent(time, false, DEVICE_MOUSE, MOUSE_WHEELDOWN);
		}
	}
}

static void
wl_pointer_frame(void* data, struct wl_pointer* pointer)
{
	// ignored event
}

static void
wl_pointer_axis_source(void* data, struct wl_pointer* pointer, uint32_t source)
{
	// ignored event
}

static void
wl_pointer_axis_stop(void* data,
					 struct wl_pointer* pointer,
					 uint32_t time,
					 uint32_t axis)
{
	// This event may seem useful for the scroll stop, however it is only fired
	// for specific "kinetic scrolling" so something like a touchpad which makes
	// it useless to us
}

static void
wl_pointer_axis_discrete(void* data,
						 struct wl_pointer* pointer,
						 uint32_t axis,
						 int32_t discrete)
{
	// ignored event
}

static const struct wl_pointer_listener wl_pointer_listener = {
	.enter = wl_pointer_enter,
	.leave = wl_pointer_leave,
	.motion = wl_pointer_motion,
	.button = wl_pointer_button,
	.axis = wl_pointer_axis,
	.frame = wl_pointer_frame,
	.axis_source = wl_pointer_axis_source,
	.axis_stop = wl_pointer_axis_stop,
	.axis_discrete = wl_pointer_axis_discrete,
};

InputHandler_Wayland::InputHandler_Wayland()
{
	xkbContext = nullptr;
	keymap = nullptr;
	xkbState = nullptr;

	// check that we have a wayland seat if not we should immediately return
	if (seat == nullptr) {
		return;
	}

	// create a new xkb context to handle keymaps
	xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	keymap = nullptr;

	// check for a poiner and keyboard then register the listeners
	if (pointer) {
		wl_pointer_add_listener(pointer, &wl_pointer_listener, this);
	} else {
		Locator::getLogger()->warn("No pointer found");
	}

	if (keyboard) {
		wl_keyboard_add_listener(keyboard, &wl_keyboard_listener, this);
	} else {
		Locator::getLogger()->warn("No keyboard found");
	}

	Locator::getLogger()->info("Wayland seat created");
}

InputHandler_Wayland::~InputHandler_Wayland()
{
	// release the xkb state and keymap

	if (xkbState) {
		xkb_state_unref(xkbState);
	}
	if (keymap) {
		xkb_keymap_unref(keymap);
	}
	if (xkbContext) {
		xkb_context_unref(xkbContext);
	}
}

void
InputHandler_Wayland::Update()
{
	if (seat == nullptr) {
		return;
	}

	// tell wayland to send events
	wl_display_dispatch_pending(display);
}

void
InputHandler_Wayland::GetDevicesAndDescriptions(
  std::vector<InputDeviceInfo>& vDevicesOut)
{
	if (seat) {
		if (pointer) {
			vDevicesOut.push_back(InputDeviceInfo(DEVICE_MOUSE, "Mouse"));
		}
		if (keyboard) {
			vDevicesOut.push_back(InputDeviceInfo(DEVICE_KEYBOARD, "Keyboard"));
		}
	}
}

void
InputHandler_Wayland::RegisterKeyEvent(uint32_t timestamp,
									   bool keyDown,
									   InputDevice input,
									   DeviceButton button)
{
	// convert and pass in the timestamp from wayland so we can have it for note
	// timing

	std::chrono::steady_clock::time_point timer(
	  std::chrono::milliseconds((long)timestamp));

	DeviceInput di(input, button, keyDown ? 1 : 0, timer);

	ButtonPressed(di);
}
