#ifndef INPUTHANDLER_DIRECTINPUT_H
#define INPUTHANDLER_DIRECTINPUT_H

#include "InputHandler.h"
#include "RageUtil/Misc/RageThreads.h"
#include <chrono>

void
DInput_ForceJoystickPollingInNextDevicesChangedCall();

struct DIDevice;
class InputHandler_DInput : public InputHandler
{
  public:
	InputHandler_DInput();
	~InputHandler_DInput();
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut);
	wchar_t DeviceButtonToChar(DeviceButton button,
							   bool bUseCurrentKeyModifiers);
	void Update();
	bool DevicesChanged();
	void WindowReset();

  private:
	RageThread m_InputThread;
	bool m_bShutdown;

	int m_iLastSeenNumHidDevices; // This changes first on plug/unplug
	int m_iNumTimesLeftToPollForJoysticksChanged;
	int m_iLastSeenNumJoysticks; // This changes sometime after
								 // m_iLastSeenNumHidDevices

	void UpdatePolled(DIDevice& device,
					  const std::chrono::steady_clock::time_point& tm);
	void UpdateBuffered(DIDevice& device,
						const std::chrono::steady_clock::time_point& tm);
	void PollAndAcquireDevices(bool bBuffered);

	static int InputThread_Start(void* p)
	{
		((InputHandler_DInput*)p)->InputThreadMain();
		return 0;
	}
	void InputThreadMain();

	void StartThread();
	void ShutdownThread();
};

#endif
