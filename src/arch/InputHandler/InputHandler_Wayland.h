#ifndef INPUT_HANDLER_X11_H
#define INPUT_HANDLER_X11_H

#include "InputHandler.h"
#include <xkbcommon/xkbcommon.h>

// global boolean to control whether or not the wayland cursor is shown
static bool WLshouldShowCursor = true;

class InputHandler_Wayland : public InputHandler
{
  public:
	InputHandler_Wayland();
	~InputHandler_Wayland();

	void Update();
	void GetDevicesAndDescriptions(std::vector<InputDeviceInfo>& vDevicesOut);

	xkb_context* xkbContext;
	xkb_keymap* keymap;
	xkb_state* xkbState;

	// convert wayland timestamp to whatever the rage3d input system wants
	void RegisterKeyEvent(uint32_t timestamp,
						  bool keyDown,
						  InputDevice input,
						  DeviceButton button);
};

#endif