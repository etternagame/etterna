#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Models/Misc/Preference.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageInput.h"
#include "RageLog.h"
#include "arch/InputHandler/InputHandler.h"

RageInput* INPUTMAN =
  NULL; // global and accessible from anywhere in our program

Preference<RString> g_sInputDrivers("InputDrivers",
									""); // "" == DEFAULT_INPUT_DRIVER_LIST

namespace {
struct LoadedInputHandler
{
	InputHandler* m_pDevice;
};
vector<LoadedInputHandler> m_InputHandlers;
map<InputDevice, InputHandler*> g_mapDeviceToHandler;
} // namespace

RageInput::RageInput()
{
	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("RageInput::RageInput()");

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
	for (unsigned i = 0; i < m_InputHandlers.size(); ++i)
		delete m_InputHandlers[i].m_pDevice;
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
	for (unsigned i = 0; i < m_InputHandlers.size(); ++i)
		delete m_InputHandlers[i].m_pDevice;
	m_InputHandlers.clear();
	g_mapDeviceToHandler.clear();

	// Init optional devices.
	vector<InputHandler*> apDevices;

	InputHandler::Create(g_sInputDrivers, apDevices);
	for (unsigned i = 0; i < apDevices.size(); ++i)
		AddHandler(apDevices[i]);

	// If no input devices are loaded, the user won't be able to input anything.
	if (apDevices.size() == 0)
		LOG->Warn("%s", NO_INPUT_DEVICES_LOADED.GetValue().c_str());
}

void
RageInput::Update()
{
	// Update optional devices.
	for (unsigned i = 0; i < m_InputHandlers.size(); ++i)
		m_InputHandlers[i].m_pDevice->Update();
}

bool
RageInput::DevicesChanged()
{
	// Update optional devices.
	for (unsigned i = 0; i < m_InputHandlers.size(); ++i) {
		if (m_InputHandlers[i].m_pDevice->DevicesChanged())
			return true;
	}
	return false;
}

void
RageInput::GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut) const
{
	for (unsigned i = 0; i < m_InputHandlers.size(); ++i)
		m_InputHandlers[i].m_pDevice->GetDevicesAndDescriptions(vDevicesOut);
}

void
RageInput::WindowReset()
{
	for (unsigned i = 0; i < m_InputHandlers.size(); ++i)
		m_InputHandlers[i].m_pDevice->WindowReset();
}

void
RageInput::AddHandler(InputHandler* pHandler)
{
	ASSERT(pHandler != NULL);

	LoadedInputHandler hand;
	hand.m_pDevice = pHandler;
	m_InputHandlers.push_back(hand);

	vector<InputDeviceInfo> aDeviceInfo;
	hand.m_pDevice->GetDevicesAndDescriptions(aDeviceInfo);
	FOREACH_CONST(InputDeviceInfo, aDeviceInfo, idi)
	g_mapDeviceToHandler[idi->id] = pHandler;
}

/** @brief Return the first InputDriver for the requested InputDevice. */
InputHandler*
RageInput::GetHandlerForDevice(const InputDevice id)
{
	map<InputDevice, InputHandler*>::iterator it =
	  g_mapDeviceToHandler.find(id);
	if (it == g_mapDeviceToHandler.end())
		return NULL;
	return it->second;
}

RString
RageInput::GetDeviceSpecificInputString(const DeviceInput& di)
{
	InputHandler* pDriver = GetHandlerForDevice(di.device);
	if (pDriver != NULL)
		return pDriver->GetDeviceSpecificInputString(di);
	else
		return di.ToString();
}

RString
RageInput::GetLocalizedInputString(const DeviceInput& di)
{
	InputHandler* pDriver = GetHandlerForDevice(di.device);
	if (pDriver != NULL)
		return pDriver->GetLocalizedInputString(di);
	else
		return Capitalize(DeviceButtonToString(di.button));
}

wchar_t
RageInput::DeviceInputToChar(DeviceInput di, bool bUseCurrentKeyModifiers)
{
	InputHandler* pDriver = GetHandlerForDevice(di.device);
	if (pDriver != NULL)
		return pDriver->DeviceButtonToChar(di.button, bUseCurrentKeyModifiers);

	return '\0';
}

InputDeviceState
RageInput::GetInputDeviceState(InputDevice id)
{
	InputHandler* pDriver = GetHandlerForDevice(id);
	if (pDriver != NULL)
		return pDriver->GetInputDeviceState(id);

	return InputDeviceState_NoInputHandler;
}

RString
RageInput::GetDisplayDevicesString() const
{
	vector<InputDeviceInfo> vDevices;
	GetDevicesAndDescriptions(vDevices);

	vector<RString> vs;
	for (unsigned i = 0; i < vDevices.size(); ++i) {
		const RString& sDescription = vDevices[i].sDesc;
		InputDevice id = vDevices[i].id;
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
		vector<RString> vsDescriptions;
		FOREACH_CONST(InputDeviceInfo, vDevices, idi)
		vsDescriptions.push_back(idi->sDesc);
		LuaHelpers::CreateTableFromArray(vsDescriptions, L);
		return 1;
	}

	LunaRageInput() { ADD_METHOD(GetDescriptions); }
};

LUA_REGISTER_CLASS(RageInput)
// lua end
