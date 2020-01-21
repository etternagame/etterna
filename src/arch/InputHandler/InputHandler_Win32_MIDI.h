#ifndef INPUT_HANDLER_WIN32_MIDI
#define INPUT_HANDLER_WIN32_MIDI

#include "InputHandler.h"
#include "RageUtil/Misc/RageInputDevice.h"

class InputHandler_Win32_MIDI : public InputHandler
{
  public:
	InputHandler_Win32_MIDI();
	~InputHandler_Win32_MIDI();

	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut);

	void SetDev(DeviceInput key) { ButtonPressed(key); }

  private:
	bool m_bFoundDevice;
};

#endif
