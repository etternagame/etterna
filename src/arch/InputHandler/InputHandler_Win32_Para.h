/* Initializes a USB Para controller so that it will function as a regular HID
 * joystick. */

#ifndef INPUT_HANDLER_WIN32_PARA_H
#define INPUT_HANDLER_WIN32_PARA_H

#include "InputHandler.h"

class InputHandler_Win32_Para : public InputHandler
{
  public:
	InputHandler_Win32_Para();
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut);
};

#endif
