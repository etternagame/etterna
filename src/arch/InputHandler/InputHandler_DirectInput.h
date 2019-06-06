#ifndef INPUTHANDLER_DIRECTINPUT_H
#define INPUTHANDLER_DIRECTINPUT_H

#include "InputHandler.h"
#include "RageUtil/Misc/RageThreads.h"
#include <chrono>

void DInput_ForceJoystickPollingInNextDevicesChangedCall();

struct DIDevice;
class InputHandler_DInput : public InputHandler
{
  public:
	InputHandler_DInput();
	~InputHandler_DInput();
	void GetDevicesAndDescriptions(std::vector<InputDeviceInfo>& vDevicesOut);
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

/*
 * (c) 2003-2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
