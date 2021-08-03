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

	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vOut) const;
	static InputHandler* GetHandlerForDevice(InputDevice id);
	static std::string GetDeviceSpecificInputString(const DeviceInput& di);
	static std::string GetLocalizedInputString(const DeviceInput& di);
	static wchar_t DeviceInputToChar(DeviceInput di, bool bUseCurrentKeyModifiers);
	static InputDeviceState GetInputDeviceState(InputDevice id);
	std::string GetDisplayDevicesString() const;

	// Lua
	void PushSelf(lua_State* L);
};

extern Preference<std::string> g_sInputDrivers;

extern RageInput*
  INPUTMAN; // global and accessible from anywhere in our program

#endif
