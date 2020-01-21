#ifndef INPUT_HANDLER_LINUX_PIUIO_H
#define INPUT_HANDLER_LINUX_PIUIO_H 1

#include "InputHandler.h"
#include "RageUtil/Misc/RageThreads.h"

class InputHandler_Linux_PIUIO : public InputHandler
{
  public:
	InputHandler_Linux_PIUIO();
	~InputHandler_Linux_PIUIO();
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut);

  private:
	static int InputThread_Start(void* p);
	void InputThread();

	int fd;
	unsigned char lastInputs[8];
	RageThread m_InputThread;
	bool m_bShutdown;
};

#endif
