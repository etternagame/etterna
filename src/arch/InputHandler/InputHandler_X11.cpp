#include "Etterna/Globals/global.h"
#include "InputHandler_X11.h"

#include <array>

#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Graphics/Display/RageDisplay.h"
#include "Etterna/Singletons/InputFilter.h"
#include "archutils/Unix/X11Helper.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>

using namespace X11Helper;
using std::vector;

REGISTER_INPUT_HANDLER_CLASS(X11);

static DeviceButton
XSymToDeviceButton(int key)
{
#define KEY_INV DeviceButton_Invalid
	static std::array<DeviceButton, 128> const ASCIIKeySyms = {
		KEY_INV,		KEY_INV,	   KEY_INV,
		KEY_INV,		KEY_INV, /* 0 - 4 */
		KEY_INV,		KEY_INV,	   KEY_INV,
		KEY_INV,		KEY_INV, /* 5 - 9 */
		KEY_INV,		KEY_INV,	   KEY_INV,
		KEY_INV,		KEY_INV, /* 10 - 14 */
		KEY_INV,		KEY_INV,	   KEY_INV,
		KEY_INV,		KEY_INV, /* 15 - 19 */
		KEY_INV,		KEY_INV,	   KEY_INV,
		KEY_INV,		KEY_INV, /* 20 - 24 */
		KEY_INV,		KEY_INV,	   KEY_INV,
		KEY_INV,		KEY_INV, /* 25 - 29 */
		KEY_INV,		KEY_INV,	   KEY_SPACE,
		KEY_EXCL,		KEY_QUOTE, /* 30 - 34 */
		KEY_HASH,		KEY_DOLLAR,	KEY_PERCENT,
		KEY_AMPER,		KEY_SQUOTE, /* 35 - 39 */
		KEY_LPAREN,		KEY_RPAREN,	KEY_ASTERISK,
		KEY_PLUS,		KEY_COMMA, /* 40 - 44 */
		KEY_HYPHEN,		KEY_PERIOD,	KEY_SLASH,
		KEY_C0,			KEY_C1, /* 45 - 49 */
		KEY_C2,			KEY_C3,		   KEY_C4,
		KEY_C5,			KEY_C6, /* 50 - 54 */
		KEY_C7,			KEY_C8,		   KEY_C9,
		KEY_COLON,		KEY_SEMICOLON, /* 55 - 59 */
		KEY_LANGLE,		KEY_EQUAL,	 KEY_RANGLE,
		KEY_QUESTION,   KEY_AT, /* 60 - 64 */
		KEY_CA,			KEY_CB,		   KEY_CC,
		KEY_CD,			KEY_CE, /* 65 - 69 */
		KEY_CF,			KEY_CG,		   KEY_CH,
		KEY_CI,			KEY_CJ, /* 70 - 74 */
		KEY_CK,			KEY_CL,		   KEY_CM,
		KEY_CN,			KEY_CO, /* 75 - 79 */
		KEY_CP,			KEY_CQ,		   KEY_CR,
		KEY_CS,			KEY_CT, /* 80 - 84 */
		KEY_CU,			KEY_CV,		   KEY_CW,
		KEY_CX,			KEY_CY, /* 85 - 89 */
		KEY_CZ,			KEY_LBRACKET,  KEY_BACKSLASH,
		KEY_RBRACKET,   KEY_CARAT, /* 90 - 94 */
		KEY_UNDERSCORE, KEY_ACCENT,	KEY_Ca,
		KEY_Cb,			KEY_Cc, /* 95 - 99 */
		KEY_Cd,			KEY_Ce,		   KEY_Cf,
		KEY_Cg,			KEY_Ch, /* 100 - 104 */
		KEY_Ci,			KEY_Cj,		   KEY_Ck,
		KEY_Cl,			KEY_Cm, /* 105 - 109 */
		KEY_Cn,			KEY_Co,		   KEY_Cp,
		KEY_Cq,			KEY_Cr, /* 110 - 114 */
		KEY_Cs,			KEY_Ct,		   KEY_Cu,
		KEY_Cv,			KEY_Cw, /* 115 - 119 */
		KEY_Cx,			KEY_Cy,		   KEY_Cz,
		KEY_LBRACE,		KEY_PIPE,			   /* 120 - 124 */
		KEY_RBRACE,		KEY_INV,	   KEY_DEL /* 125 - 127 */
	};

	/* 32...127: */
	if (key < ASCIIKeySyms.size()) {
		return ASCIIKeySyms[key];
	}

	/* XK_KP_0 ... XK_KP_9 to KEY_KP_C0 ... KEY_KP_C9 */
	if (key >= XK_KP_0 && key <= XK_KP_9)
		return enum_add2(KEY_KP_C0, key - XK_KP_0);

	switch (key) {
		/* These are needed because of the way X registers the keypad. */
		case XK_BackSpace:
			return KEY_BACK;
		case XK_Tab:
			return KEY_TAB;
		case XK_Pause:
			return KEY_PAUSE;
		case XK_Escape:
			return KEY_ESC;
		case XK_KP_Insert:
			return KEY_KP_C0;
		case XK_KP_End:
			return KEY_KP_C1;
		case XK_KP_Down:
			return KEY_KP_C2;
		case XK_KP_Page_Down:
			return KEY_KP_C3;
		case XK_KP_Left:
			return KEY_KP_C4;
		case XK_KP_Begin:
			return KEY_KP_C5;
		case XK_KP_Right:
			return KEY_KP_C6;
		case XK_KP_Home:
			return KEY_KP_C7;
		case XK_KP_Up:
			return KEY_KP_C8;
		case XK_KP_Page_Up:
			return KEY_KP_C9;
		case XK_KP_Decimal:
			return KEY_KP_PERIOD;
		case XK_KP_Divide:
			return KEY_KP_SLASH;
		case XK_KP_Multiply:
			return KEY_KP_ASTERISK;
		case XK_KP_Subtract:
			return KEY_KP_HYPHEN;
		case XK_KP_Add:
			return KEY_KP_PLUS;
		case XK_KP_Equal:
			return KEY_KP_EQUAL;
		case XK_KP_Enter:
			return KEY_KP_ENTER;
		case XK_Up:
			return KEY_UP;
		case XK_Down:
			return KEY_DOWN;
		case XK_Right:
			return KEY_RIGHT;
		case XK_Left:
			return KEY_LEFT;
		case XK_Insert:
			return KEY_INSERT;
		case XK_Home:
			return KEY_HOME;
		case XK_Delete:
			return KEY_DEL;
		case XK_End:
			return KEY_END;
		case XK_Page_Up:
			return KEY_PGUP;
		case XK_Page_Down:
			return KEY_PGDN;
		case XK_F1:
			return KEY_F1;
		case XK_F2:
			return KEY_F2;
		case XK_F3:
			return KEY_F3;
		case XK_F4:
			return KEY_F4;
		case XK_F5:
			return KEY_F5;
		case XK_F6:
			return KEY_F6;
		case XK_F7:
			return KEY_F7;
		case XK_F8:
			return KEY_F8;
		case XK_F9:
			return KEY_F9;
		case XK_F10:
			return KEY_F10;
		case XK_F11:
			return KEY_F11;
		case XK_F12:
			return KEY_F12;
		case XK_F13:
			return KEY_F13;
		case XK_F14:
			return KEY_F14;
		case XK_F15:
			return KEY_F15;

		case XK_Num_Lock:
			return KEY_NUMLOCK;
		case XK_Caps_Lock:
			return KEY_CAPSLOCK;
		case XK_Scroll_Lock:
			return KEY_SCRLLOCK;
		case XK_Return:
			return KEY_ENTER;
		case XK_Sys_Req:
			return KEY_PRTSC;
		case XK_Print:
			return KEY_PRTSC;
		case XK_Shift_R:
			return KEY_RSHIFT;
		case XK_Shift_L:
			return KEY_LSHIFT;
		case XK_Control_R:
			return KEY_RCTRL;
		case XK_Control_L:
			return KEY_LCTRL;
		case XK_Alt_R:
			return KEY_RALT;
		case XK_Alt_L:
			return KEY_LALT;
		case XK_Meta_R:
			return KEY_RMETA;
		case XK_Meta_L:
			return KEY_LMETA;
		case XK_Super_L:
			return KEY_LSUPER;
		case XK_Super_R:
			return KEY_RSUPER;
		case XK_Menu:
			return KEY_MENU;

		// mouse
		case XK_Pointer_Button1:
			return MOUSE_LEFT;
		case XK_Pointer_Button2:
			return MOUSE_MIDDLE;
		case XK_Pointer_Button3:
			return MOUSE_RIGHT;
		case XK_Pointer_Button4:
			return MOUSE_WHEELUP;
		case XK_Pointer_Button5:
			return MOUSE_WHEELDOWN;
	}

	return DeviceButton_Invalid;
}

static DeviceButton
XMouseButtonToDeviceButton(int button)
{
	switch (button) {
		case Button1:
			return MOUSE_LEFT;
		case Button2:
			return MOUSE_MIDDLE;
		case Button3:
			return MOUSE_RIGHT;
		case Button4:
			return MOUSE_WHEELUP;
		case Button5:
			return MOUSE_WHEELDOWN;
	}
	return DeviceButton_Invalid;
}

InputHandler_X11::InputHandler_X11()
{
	if (Dpy == nullptr || Win == None)
		return;
	XWindowAttributes winAttrib;

	XGetWindowAttributes(Dpy, Win, &winAttrib);
	// todo: add ButtonMotionMask, Button(1-5)MotionMask,
	// (EnterWindowMask/LeaveWindowMask?) -aj
	XSelectInput(Dpy,
				 Win,
				 winAttrib.your_event_mask | KeyPressMask | KeyReleaseMask |
				   ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
				   FocusChangeMask);
}

InputHandler_X11::~InputHandler_X11()
{
	if (Dpy == nullptr || Win == None)
		return;
	// TODO: Determine if we even need to set this back (or is the window
	// destroyed just after this?)
	XWindowAttributes winAttrib;

	XGetWindowAttributes(Dpy, Win, &winAttrib);
	XSelectInput(Dpy,
				 Win,
				 winAttrib.your_event_mask &
				   ~(KeyPressMask | KeyReleaseMask | ButtonPressMask |
					 ButtonReleaseMask | PointerMotionMask | FocusChangeMask));
}

void
InputHandler_X11::Update()
{
	if (Dpy == nullptr || Win == None) {
		InputHandler::UpdateTimer();
		return;
	}

	XEvent event, last_release;
	last_release.type = 0;

	DeviceButton curr_db = DeviceButton_Invalid;
	DeviceButton release_db = DeviceButton_Invalid;
	InputDevice release_dv = InputDevice_Invalid;

	// We use XCheckWindowEvent instead of XNextEvent because
	// other things (most notably ArchHooks_Unix) may be looking for
	// events that we'd pick up and discard.
	// todo: add other masks? (like the ones for drag'n drop) -aj
	while (XCheckWindowEvent(Dpy,
							 Win,
							 KeyPressMask | KeyReleaseMask | ButtonPressMask |
							   ButtonReleaseMask | PointerMotionMask |
							   FocusChangeMask,
							 &event)) {
		// X's key repeat behavior:
		//   Press when the key is pressed.
		//   Release + Press pair for every repeat.
		//   Release when the key is released.
		// Stepmania's key repeat behavior:
		//   Press when the key is pressed.
		//   Repeat for every repeat.
		//   Release when the key is released.
		// So any release event from X needs extra handling to check for the
		// paired press.
		// Release + Press pairs are ignored, because Stepmania already has its
		// own repeat logic elsewhere.
		// -Kyz
		bool real_release = false;
		switch (last_release.type) {
			case KeyRelease:
				if (event.type == KeyPress &&
					event.xkey.keycode == last_release.xkey.keycode &&
					event.xkey.time == last_release.xkey.time) {
					// This is a repeat event so ignore it.
					last_release.type = 0;
					continue;
				}
				real_release = true;
				break;
			case ButtonRelease:
				if (event.type == ButtonPress &&
					event.xbutton.button == last_release.xbutton.button &&
					event.xbutton.time == last_release.xbutton.time) {
					// This is a repeat event so ignore it.
					last_release.type = 0;
					continue;
				}
				real_release = true;
				break;
			default:
				break;
		}
		if (real_release) {
			RegisterKeyEvent(event.xkey.time, false, release_dv, release_db);
		}
		switch (event.type) {
			case MotionNotify:
				INPUTFILTER->UpdateCursorLocation(event.xmotion.x,
												  event.xmotion.y);
				break;
			case KeyPress:
				curr_db = XSymToDeviceButton(XLookupKeysym(&event.xkey, 0));
				if (curr_db != DeviceButton_Invalid) {
					RegisterKeyEvent(
					  event.xkey.time, true, DEVICE_KEYBOARD, curr_db);
				}
				break;
			case KeyRelease:
				release_db = XSymToDeviceButton(XLookupKeysym(&event.xkey, 0));
				if (release_db != DeviceButton_Invalid) {
					last_release = event;
					release_dv = DEVICE_KEYBOARD;
				} else {
					last_release.type = 0;
				}
				break;
			case ButtonPress:
				curr_db = XMouseButtonToDeviceButton(event.xbutton.button);
				if (curr_db != DeviceButton_Invalid) {
					switch (curr_db) {
						case MOUSE_WHEELUP:
							INPUTFILTER->UpdateMouseWheel(
							  INPUTFILTER->GetMouseWheel() + 1);
							break;
						case MOUSE_WHEELDOWN:
							INPUTFILTER->UpdateMouseWheel(
							  INPUTFILTER->GetMouseWheel() - 1);
							break;
						default:
							break;
					}
					RegisterKeyEvent(
					  event.xkey.time, true, DEVICE_MOUSE, curr_db);
				}
				break;
			case ButtonRelease:
				release_db = XMouseButtonToDeviceButton(event.xbutton.button);
				if (release_db != DeviceButton_Invalid) {
					last_release = event;
					release_dv = DEVICE_MOUSE;
				} else {
					last_release.type = 0;
				}
				break;
			case FocusOut:
				// Release all buttons
				INPUTFILTER->Reset();
				break;
			default:
				break;
		}
	}
	if (last_release.type != 0) {
		RegisterKeyEvent(event.xkey.time, false, release_dv, release_db);
	}
	InputHandler::UpdateTimer();
}

void
InputHandler_X11::GetDevicesAndDescriptions(
  vector<InputDeviceInfo>& vDevicesOut)
{
	if (Dpy && Win) {
		vDevicesOut.push_back(InputDeviceInfo(DEVICE_KEYBOARD, "Keyboard"));
		vDevicesOut.push_back(InputDeviceInfo(DEVICE_MOUSE, "Mouse"));
	}
}

void
InputHandler_X11::RegisterKeyEvent(unsigned long timestamp,
								   bool keyDown,
								   InputDevice input,
								   DeviceButton button)
{
	// https://linux.die.net/man/3/xkeyevent
	// Event timestamp is in milliseconds

	std::chrono::steady_clock::time_point timer(
	  std::chrono::milliseconds((long)timestamp));

	DeviceInput di(input, button, keyDown ? 1 : 0, timer);

	ButtonPressed(di);
}
