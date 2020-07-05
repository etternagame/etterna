/* InputHandler_Linux_Event - evdev-based input driver */

#ifndef INPUT_HANDLER_LINUX_EVENT_H
#define INPUT_HANDLER_LINUX_EVENT_H

#include "InputHandler.h"
#include "RageUtil/Misc/RageThreads.h"

class InputHandler_Linux_Event : public InputHandler
{
  public:
	InputHandler_Linux_Event();
	~InputHandler_Linux_Event();
	bool TryDevice(std::string devfile);
	bool DevicesChanged() { return m_bDevicesChanged; }
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut);

  private:
	void StartThread();
	void StopThread();
	static int InputThread_Start(void* p);
	void InputThread();

	RageThread m_InputThread;
	InputDevice m_NextDevice;
	bool m_bShutdown, m_bDevicesChanged;
};
#define USE_INPUT_HANDLER_LINUX_JOYSTICK

#endif
