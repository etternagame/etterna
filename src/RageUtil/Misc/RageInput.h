/* RageInput - Starts up InputHandlers, which generate InputEvents. */

#ifndef RAGEINPUT_H
#define RAGEINPUT_H

#include "Etterna/Models/Misc/Preference.h"
#include "RageInputDevice.h"

struct lua_State;
class InputHandler;

class RageInput
{
  public:
	RageInput();
	~RageInput();

	void LoadDrivers();
	void Update();
	bool DevicesChanged();
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vOut) const;
	void WindowReset();
	void AddHandler(InputHandler* pHandler);
	InputHandler* GetHandlerForDevice(InputDevice id);
	RString GetDeviceSpecificInputString(const DeviceInput& di);
	RString GetLocalizedInputString(const DeviceInput& di);
	wchar_t DeviceInputToChar(DeviceInput di, bool bUseCurrentKeyModifiers);
	InputDeviceState GetInputDeviceState(InputDevice id);
	RString GetDisplayDevicesString() const;

	// Lua
	void PushSelf(lua_State* L);
};

extern Preference<RString> g_sInputDrivers;

extern RageInput*
  INPUTMAN; // global and accessible from anywhere in our program

#endif
