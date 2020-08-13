#ifndef INPUT_HANDLER_LINUX_JOYSTICK_H
#define INPUT_HANDLER_LINUX_JOYSTICK_H 1

#include "InputHandler.h"
#include "RageUtil/Misc/RageThreads.h"

class InputHandler_Linux_Joystick : public InputHandler
{
  public:
	enum
	{
		NUM_JOYSTICKS = 4
	};
	InputHandler_Linux_Joystick();
	~InputHandler_Linux_Joystick();
	bool TryDevice(std::string dev);
	bool DevicesChanged() { return m_bDevicesChanged; }
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut);

  private:
	void StartThread();
	void StopThread();
	static int InputThread_Start(void* p);
	void InputThread();

	int fds[NUM_JOYSTICKS];
	int m_iLastFd;
	std::string m_sDescription[NUM_JOYSTICKS];
	RageThread m_InputThread;
	bool m_bShutdown, m_bDevicesChanged;
};

#endif
