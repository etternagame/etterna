#ifndef LINUX_INPUT_MANAGER
#define LINUX_INPUT_MANAGER 1

#include <vector>
using namespace std;

#include "Etterna/Globals/global.h"
class InputHandler_Linux_Joystick;
class InputHandler_Linux_Event;

// Enumerates the input devices on the system and dispatches them to
// IH_Linux_Event and IH_Linux_Joystick as appropriate.

class LinuxInputManager
{
  public:
	LinuxInputManager();
	void InitDriver(InputHandler_Linux_Joystick* drv);
	void InitDriver(InputHandler_Linux_Event* drv);
	~LinuxInputManager();

  private:
	bool m_bEventEnabled;
	InputHandler_Linux_Event* m_EventDriver;
	vector<std::string> m_vsPendingEventDevices;

	bool m_bJoystickEnabled;
	InputHandler_Linux_Joystick* m_JoystickDriver;
	vector<std::string> m_vsPendingJoystickDevices;
};

extern LinuxInputManager*
  LINUXINPUT; // global and accessible from anywhere in our program

#endif // LINUX_INPUT_MANAGER
