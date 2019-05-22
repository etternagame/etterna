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
	bool TryDevice(RString dev);
	bool DevicesChanged() { return m_bDevicesChanged; }
	void GetDevicesAndDescriptions(std::vector<InputDeviceInfo>& vDevicesOut);

  private:
	void StartThread();
	void StopThread();
	static int InputThread_Start(void* p);
	void InputThread();

	int fds[NUM_JOYSTICKS];
	int m_iLastFd;
	RString m_sDescription[NUM_JOYSTICKS];
	RageThread m_InputThread;
	bool m_bShutdown, m_bDevicesChanged;
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
