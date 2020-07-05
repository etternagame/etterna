#ifndef INPUT_HANDLER_WIN32_PUMP_H
#define INPUT_HANDLER_WIN32_PUMP_H

#include "InputHandler.h"
#include "RageUtil/Misc/RageThreads.h"

class USBDevice;
class InputHandler_Win32_Pump : public InputHandler
{
  public:
	void Update();
	InputHandler_Win32_Pump();
	~InputHandler_Win32_Pump();
	std::string GetDeviceSpecificInputString(const DeviceInput& di);
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut);

  private:
	USBDevice* m_pDevice;
	RageThread InputThread;
	bool m_bShutdown;

	static int InputThread_Start(void* p);
	void InputThreadMain();
	void HandleInput(int devno, int event);
};

#endif
