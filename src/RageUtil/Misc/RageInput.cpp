#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Models/Misc/Preference.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageInput.h"
#include "Core/Services/Locator.hpp"

#include <map>

RageInput* INPUTMAN =
  nullptr; // global and accessible from anywhere in our program

Preference<std::string> g_sInputDrivers("InputDrivers",
										""); // "" == DEFAULT_INPUT_DRIVER_LIST

namespace {
struct LoadedInputHandler
{
	InputHandler* m_pDevice;
};
std::vector<LoadedInputHandler> m_InputHandlers;
std::map<InputDevice, InputHandler*> g_mapDeviceToHandler;
} // namespace

RageInput::RageInput()
{
	if (PREFSMAN->m_verbose_log > 1)
		Locator::getLogger()->trace("RageInput::RageInput()");

	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "INPUTMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}

}

RageInput::~RageInput()
{
	// Delete optional devices.
	for (auto& m_InputHandler : m_InputHandlers)
		delete m_InputHandler.m_pDevice;
	m_InputHandlers.clear();
	g_mapDeviceToHandler.clear();

	// Unregister with Lua.
	LUA->UnsetGlobal("INPUTMAN");
}

static LocalizedString NO_INPUT_DEVICES_LOADED("RageInput",
											   "No input devices were loaded.");


void
RageInput::GetDevicesAndDescriptions(std::vector<InputDeviceInfo>& vDevicesOut) const
{
//	for (auto& m_InputHandler : m_InputHandlers)
//		m_InputHandler.m_pDevice->GetDevicesAndDescriptions(vDevicesOut);
}


/** @brief Return the first InputDriver for the requested InputDevice. */
InputHandler*
RageInput::GetHandlerForDevice(const InputDevice id)
{
	auto it = g_mapDeviceToHandler.find(id);
	if (it == g_mapDeviceToHandler.end())
		return nullptr;
	return it->second;
}

std::string
RageInput::GetDeviceSpecificInputString(const DeviceInput& di)
{
	if (di.device == InputDevice_Invalid)
		return std::string();

	if (di.device == DEVICE_KEYBOARD) {
		wchar_t c = DeviceInputToChar(di, false);
		if (c && c != L' ') // Don't show "Key  " for space.
			return InputDeviceToString(di.device) + " " +
				   Capitalize(WStringToString(std::wstring() + c));
	}

	std::string s = DeviceButtonToString(di.button);
	if (di.device != DEVICE_KEYBOARD)
		s = InputDeviceToString(di.device) + " " + s;
	return s;
}

std::string
RageInput::GetLocalizedInputString(const DeviceInput& di)
{
   return Capitalize(DeviceButtonToString(di.button));
}

std::string
RageInput::GetDisplayDevicesString() const
{
	std::vector<InputDeviceInfo> vDevices;
	GetDevicesAndDescriptions(vDevices);

	std::vector<std::string> vs;
	for (auto& vDevice : vDevices) {
		const std::string& sDescription = vDevice.sDesc;
		InputDevice id = vDevice.id;
		vs.push_back(ssprintf(
		  "%s (%s)", sDescription.c_str(), InputDeviceToString(id).c_str()));
	}
	return join("\n", vs);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to RageInput. */
class LunaRageInput : public Luna<RageInput>
{
  public:
	static int GetDescriptions(T* p, lua_State* L)
	{
		std::vector<std::string> vsDescriptions{"Keyboard", "Mouse"};
		LuaHelpers::CreateTableFromArray(vsDescriptions, L);
		return 1;
	}

	LunaRageInput() { ADD_METHOD(GetDescriptions); }
};

LUA_REGISTER_CLASS(RageInput)
// lua end
