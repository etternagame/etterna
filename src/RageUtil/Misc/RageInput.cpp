#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Models/Misc/Preference.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageInput.h"
#include "Core/Services/Locator.hpp"
#include "arch/InputHandler/InputHandler.h"

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
vector<LoadedInputHandler> m_InputHandlers;
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

	LoadDrivers();
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
RageInput::LoadDrivers()
{
	for (auto& m_InputHandler : m_InputHandlers)
		delete m_InputHandler.m_pDevice;
	m_InputHandlers.clear();
	g_mapDeviceToHandler.clear();

	// Init optional devices.
	vector<InputHandler*> apDevices;

	InputHandler::Create(g_sInputDrivers, apDevices);
	for (auto& apDevice : apDevices)
		AddHandler(apDevice);

	// If no input devices are loaded, the user won't be able to input anything.
	if (apDevices.size() == 0)
		Locator::getLogger()->warn(NO_INPUT_DEVICES_LOADED.GetValue().c_str());
}

void
RageInput::Update()
{
	// Update optional devices.
	for (auto& m_InputHandler : m_InputHandlers)
		m_InputHandler.m_pDevice->Update();
}

bool
RageInput::DevicesChanged()
{
	// Update optional devices.
	for (auto& m_InputHandler : m_InputHandlers) {
		if (m_InputHandler.m_pDevice->DevicesChanged())
			return true;
	}
	return false;
}

void
RageInput::GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut) const
{
	for (auto& m_InputHandler : m_InputHandlers)
		m_InputHandler.m_pDevice->GetDevicesAndDescriptions(vDevicesOut);
}

void
RageInput::WindowReset()
{
	for (auto& m_InputHandler : m_InputHandlers)
		m_InputHandler.m_pDevice->WindowReset();
}

void
RageInput::AddHandler(InputHandler* pHandler)
{
	ASSERT(pHandler != nullptr);

	LoadedInputHandler hand;
	hand.m_pDevice = pHandler;
	m_InputHandlers.push_back(hand);

	vector<InputDeviceInfo> aDeviceInfo;
	hand.m_pDevice->GetDevicesAndDescriptions(aDeviceInfo);
	for (auto& idi : aDeviceInfo)
		g_mapDeviceToHandler[idi.id] = pHandler;
}

/** @brief Return the first InputDriver for the requested InputDevice. */
InputHandler*
RageInput::GetHandlerForDevice(const InputDevice id)
{
	std::map<InputDevice, InputHandler*>::iterator it =
	  g_mapDeviceToHandler.find(id);
	if (it == g_mapDeviceToHandler.end())
		return nullptr;
	return it->second;
}

std::string
RageInput::GetDeviceSpecificInputString(const DeviceInput& di)
{
	InputHandler* pDriver = GetHandlerForDevice(di.device);
	if (pDriver != nullptr)
		return pDriver->GetDeviceSpecificInputString(di);
	else
		return di.ToString();
}

std::string
RageInput::GetLocalizedInputString(const DeviceInput& di)
{
	InputHandler* pDriver = GetHandlerForDevice(di.device);
	if (pDriver != nullptr)
		return pDriver->GetLocalizedInputString(di);
	else
		return Capitalize(DeviceButtonToString(di.button));
}

wchar_t
RageInput::DeviceInputToChar(DeviceInput di, bool bUseCurrentKeyModifiers)
{
	InputHandler* pDriver = GetHandlerForDevice(di.device);
	if (pDriver != nullptr)
		return pDriver->DeviceButtonToChar(di.button, bUseCurrentKeyModifiers);

	return '\0';
}

InputDeviceState
RageInput::GetInputDeviceState(InputDevice id)
{
	InputHandler* pDriver = GetHandlerForDevice(id);
	if (pDriver != nullptr)
		return pDriver->GetInputDeviceState(id);

	return InputDeviceState_NoInputHandler;
}

std::string
RageInput::GetDisplayDevicesString() const
{
	vector<InputDeviceInfo> vDevices;
	GetDevicesAndDescriptions(vDevices);

	vector<std::string> vs;
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
		vector<InputDeviceInfo> vDevices;
		p->GetDevicesAndDescriptions(vDevices);
		vector<std::string> vsDescriptions;
		for (auto& idi : vDevices)
			vsDescriptions.push_back(idi.sDesc);
		LuaHelpers::CreateTableFromArray(vsDescriptions, L);
		return 1;
	}

	LunaRageInput() { ADD_METHOD(GetDescriptions); }
};

LUA_REGISTER_CLASS(RageInput)
// lua end
