#include "Etterna/Globals/global.h"
#include "archutils/Win32/AppInstance.h"
#include "archutils/Win32/DirectXHelpers.h"
#include "archutils/Win32/ErrorStrings.h"
#include "archutils/Win32/GraphicsWindow.h"
#include "archutils/Win32/RegistryAccess.h"
#include "Etterna/Models/Misc/GamePreferences.h" //needed for Axis Fix
#include "Etterna/Singletons/InputFilter.h"
#include "InputHandler_DirectInput.h"
#include "InputHandler_DirectInputHelper.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"

#include <algorithm>

REGISTER_INPUT_HANDLER_CLASS2(DirectInput, DInput);

static std::vector<DIDevice> Devices;

// Number of joysticks found:
static int g_iNumJoysticks;
static bool g_forceJoystickPolling = false;
void
DInput_ForceJoystickPollingInNextDevicesChangedCall()
{
	g_forceJoystickPolling = true;
}

static BOOL CALLBACK
EnumDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, void* pContext)
{
	DIDevice device;

	Locator::getLogger()->info("DInput: Enumerating device - Type: {0:#x} Instance Name: \"{1}\" "
		"Product Name: \"{2}\"",
		pdidInstance->dwDevType, pdidInstance->tszInstanceName, pdidInstance->tszProductName);

	switch (GET_DIDEVICE_TYPE(pdidInstance->dwDevType)) {
		case DI8DEVTYPE_JOYSTICK:
		case DI8DEVTYPE_GAMEPAD:
		case DI8DEVTYPE_DRIVING:
		case DI8DEVTYPE_FLIGHT:
		case DI8DEVTYPE_1STPERSON:
		case DI8DEVTYPE_DEVICECTRL:
		case DI8DEVTYPE_SCREENPOINTER:
		case DI8DEVTYPE_REMOTE:
		case DI8DEVTYPE_SUPPLEMENTAL: {
			device.type = device.JOYSTICK;
			break;
		}

		case DI8DEVTYPE_KEYBOARD:
			device.type = device.KEYBOARD;
			break;
		case DI8DEVTYPE_MOUSE:
			device.type = device.MOUSE;
			break;
		default:
			Locator::getLogger()->info("DInput: Unrecognized device ignored.");
			return DIENUM_CONTINUE;
	}

	device.JoystickInst = *pdidInstance;

	switch (device.type) {
		case DIDevice::JOYSTICK:
			if (g_iNumJoysticks == NUM_JOYSTICKS)
				return DIENUM_CONTINUE;

			device.dev = enum_add2(DEVICE_JOY1, g_iNumJoysticks);
			g_iNumJoysticks++;
			break;

		case DIDevice::KEYBOARD:
			device.dev = DEVICE_KEYBOARD;
			break;

		case DIDevice::MOUSE:
			device.dev = DEVICE_MOUSE;
			break;
	}

	Devices.push_back(device);

	return DIENUM_CONTINUE;
}

static void
CheckForDirectInputDebugMode()
{
	int iVal;
	if (RegistryAccess::GetRegValue(
		  "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\DirectInput",
		  "emulation",
		  iVal)) {
		if (iVal & 0x8)
			Locator::getLogger()->warn("DirectInput keyboard debug mode appears to be enabled. "
					  "This reduces\n"
					  "input timing accuracy significantly. Disabling this is "
					  "strongly recommended.");
	}
}

static BOOL CALLBACK
CountDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, void* pContext)
{
	(*(int*)pContext)++;
	return DIENUM_CONTINUE;
}

static int
GetNumHidDevices()
{
	int i = 0;
	RegistryAccess::GetRegValue(
	  "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\HidUsb\\Enum",
	  "Count",
	  i,
	  false); // don't warn on error
	return i;
}

static int
GetNumJoysticksSlow()
{
	int iCount = 0;
	HRESULT hr = g_dinput->EnumDevices(
	  DI8DEVCLASS_GAMECTRL, CountDevicesCallback, &iCount, DIEDFL_ATTACHEDONLY);
	if (hr != DI_OK) {
		Locator::getLogger()->warn(hr_ssprintf(hr, "g_dinput->EnumDevices"));
	}
	return iCount;
}

InputHandler_DInput::InputHandler_DInput()
{
	Locator::getLogger()->debug("InputHandler_DInput::InputHandler_DInput()");

	CheckForDirectInputDebugMode();

	m_bShutdown = false;
	g_iNumJoysticks = 0;

	AppInstance inst;
	HRESULT hr = DirectInput8Create(inst.Get(),
									DIRECTINPUT_VERSION,
									IID_IDirectInput8,
									(LPVOID*)&g_dinput,
									nullptr);
	if (hr != DI_OK)
		RageException::Throw(
		  hr_ssprintf(hr, "InputHandler_DInput: DirectInputCreate").c_str());

	Locator::getLogger()->trace("InputHandler_DInput: IDirectInput::EnumDevices(DIDEVTYPE_KEYBOARD)");
	hr = g_dinput->EnumDevices(
	  DI8DEVCLASS_KEYBOARD, EnumDevicesCallback, nullptr, DIEDFL_ATTACHEDONLY);
	if (hr != DI_OK)
		RageException::Throw(
		  hr_ssprintf(hr, "InputHandler_DInput: IDirectInput::EnumDevices")
			.c_str());

	Locator::getLogger()->trace("InputHandler_DInput: IDirectInput::EnumDevices(DIDEVTYPE_JOYSTICK)");
	hr = g_dinput->EnumDevices(
	  DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, nullptr, DIEDFL_ATTACHEDONLY);
	if (hr != DI_OK)
		RageException::Throw(
		  hr_ssprintf(hr, "InputHandler_DInput: IDirectInput::EnumDevices")
			.c_str());

	// mouse
	Locator::getLogger()->trace("InputHandler_DInput: IDirectInput::EnumDevices(DIDEVTYPE_MOUSE)");
	hr = g_dinput->EnumDevices(
	  DI8DEVCLASS_POINTER, EnumDevicesCallback, nullptr, DIEDFL_ATTACHEDONLY);
	if (hr != DI_OK)
		RageException::Throw(
		  hr_ssprintf(hr, "InputHandler_DInput: IDirectInput::EnumDevices")
			.c_str());

	for (unsigned i = 0; i < Devices.size(); ++i) {
		if (Devices[i].Open())
			continue;

		Devices.erase(Devices.begin() + i);
		i--;
		continue;
	}

	Locator::getLogger()->info("Found {} DirectInput devices:", Devices.size());
	for (unsigned i = 0; i < Devices.size(); ++i) {
		Locator::getLogger()->info("   {}: '{}' axes: {}, hats: {}, buttons: {} ({})",
					i, Devices[i].m_sName.c_str(), Devices[i].axes, Devices[i].hats,
					Devices[i].buttons, Devices[i].buffered ? "buffered" : "unbuffered");
	}

	m_iLastSeenNumHidDevices = GetNumHidDevices();
	m_iNumTimesLeftToPollForJoysticksChanged = 0;
	m_iLastSeenNumJoysticks = GetNumJoysticksSlow();

	StartThread();
}

void
InputHandler_DInput::StartThread()
{
	ASSERT(!m_InputThread.IsCreated());
	if (PREFSMAN->m_bThreadedInput) {
		m_InputThread.SetName("DirectInput thread");
		m_InputThread.Create(InputThread_Start, this);
	}
}

void
InputHandler_DInput::ShutdownThread()
{
	m_bShutdown = true;
	if (m_InputThread.IsCreated()) {
		Locator::getLogger()->info("Shutting down DirectInput thread ...");
		m_InputThread.Wait();
		Locator::getLogger()->info("DirectInput thread shut down.");
	}
	m_bShutdown = false;
}

InputHandler_DInput::~InputHandler_DInput()
{
	ShutdownThread();

	for (unsigned i = 0; i < Devices.size(); ++i)
		Devices[i].Close();

	Devices.clear();
	g_dinput->Release();
	g_dinput = nullptr;
}

void
InputHandler_DInput::WindowReset()
{
	// We need to reopen keyboards.
	ShutdownThread();

	for (unsigned i = 0; i < Devices.size(); ++i) {
		if (Devices[i].type != Devices[i].KEYBOARD)
			continue;

		Devices[i].Close();

		// We lose buffered inputs here, so we need to clear all pressed keys.
		INPUTFILTER->ResetDevice(Devices[i].dev);

		bool ret = Devices[i].Open();

		// Reopening it should succeed.
		ASSERT(ret);
	}

	StartThread();
}

#define HAT_UP_MASK 1
#define HAT_DOWN_MASK 2
#define HAT_LEFT_MASK 4
#define HAT_RIGHT_MASK 8

static int
TranslatePOV(DWORD value)
{
	const int HAT_VALS[] = { HAT_UP_MASK,	 HAT_UP_MASK | HAT_RIGHT_MASK,
							 HAT_RIGHT_MASK, HAT_DOWN_MASK | HAT_RIGHT_MASK,
							 HAT_DOWN_MASK,	 HAT_DOWN_MASK | HAT_LEFT_MASK,
							 HAT_LEFT_MASK,	 HAT_UP_MASK | HAT_LEFT_MASK };

	if (LOWORD(value) == 0xFFFF)
		return 0;

	// Round the value up:
	value += 4500 / 2;
	value %= 36000;
	value /= 4500;

	if (value >= 8)
		return 0; // shouldn't happen

	return HAT_VALS[value];
}

static HRESULT
GetDeviceState(LPDIRECTINPUTDEVICE8 dev, int size, void* ptr)
{
	HRESULT hr = dev->GetDeviceState(size, ptr);
	if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
		hr = dev->Acquire();
		if (hr != DI_OK) {
			Locator::getLogger()->trace(hr_ssprintf(hr, "?"));
			return hr;
		}

		hr = dev->GetDeviceState(size, ptr);
	}

	return hr;
}

/* This doesn't take a timestamp; instead, we let InputHandler::ButtonPressed
 * figure it out. Be sure to call InputHandler::Update() between each poll. */
void
InputHandler_DInput::UpdatePolled(
  DIDevice& device,
  const std::chrono::steady_clock::time_point& tm)
{
	switch (device.type) {
		default:
			FAIL_M(ssprintf("Unsupported DI device type: %i", device.type));
		case DIDevice::KEYBOARD: {
			unsigned char keys[256];

			HRESULT hr = GetDeviceState(device.Device, 256, keys);
			if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
				return;

			if (hr != DI_OK) {
                Locator::getLogger()->debug(hr_ssprintf(hr, "Failures on polled keyboard update"));
				return;
			}

			for (int k = 0; k < 256; ++k) {
				const DeviceButton key = (DeviceButton)device.Inputs[k].num;
				ButtonPressed(DeviceInput(device.dev, key, !!(keys[k] & 0x80)));
			}
		} break;
		case DIDevice::JOYSTICK: {
			DIJOYSTATE state;

			HRESULT hr = GetDeviceState(device.Device, sizeof(state), &state);
			if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
				return;

			// Set each known axis, button and POV.
			for (unsigned i = 0; i < device.Inputs.size(); ++i) {
				const input_t& in = device.Inputs[i];
				const InputDevice dev = device.dev;

				switch (in.type) {
					case input_t::BUTTON: {
						DeviceInput di(
						  dev,
						  enum_add2(JOY_BUTTON_1, in.num),
						  !!state.rgbButtons[in.ofs - DIJOFS_BUTTON0],
						  tm);
						ButtonPressed(di);
						break;
					}

					case input_t::AXIS: {
						DeviceButton neg = DeviceButton_Invalid,
									 pos = DeviceButton_Invalid;
						int val = 0;
						if (in.ofs == DIJOFS_X) {
							neg = JOY_LEFT;
							pos = JOY_RIGHT;
							val = state.lX;
						} else if (in.ofs == DIJOFS_Y) {
							neg = JOY_UP;
							pos = JOY_DOWN;
							val = state.lY;
						} else if (in.ofs == DIJOFS_Z) {
							neg = JOY_Z_UP;
							pos = JOY_Z_DOWN;
							val = state.lZ;
						} else if (in.ofs == DIJOFS_RX) {
							neg = JOY_ROT_LEFT;
							pos = JOY_ROT_RIGHT;
							val = state.lRx;
						} else if (in.ofs == DIJOFS_RY) {
							neg = JOY_ROT_UP;
							pos = JOY_ROT_DOWN;
							val = state.lRy;
						} else if (in.ofs == DIJOFS_RZ) {
							neg = JOY_ROT_Z_UP;
							pos = JOY_ROT_Z_DOWN;
							val = state.lRz;
						} else if (in.ofs == DIJOFS_SLIDER(0)) {
							neg = JOY_AUX_1;
							pos = JOY_AUX_2;
							val = state.rglSlider[0];
						} else if (in.ofs == DIJOFS_SLIDER(1)) {
							neg = JOY_AUX_3;
							pos = JOY_AUX_4;
							val = state.rglSlider[1];
						} else
							Locator::getLogger()->warn("Controller '{}' is returning an unknown joystick offset, {}",
										device.m_sName.c_str(), in.ofs);

						if (neg != DeviceButton_Invalid) {
							float l = SCALE(
							  static_cast<int>(val), 0.0f, 100.0f, 0.0f, 1.0f);
							ButtonPressed(
							  DeviceInput(dev, neg, std::max(-l, 0.F), tm));
							ButtonPressed(
							  DeviceInput(dev, pos, std::max(+l, 0.F), tm));
						}

						break;
					}

					case input_t::HAT:
						if (in.num == 0) {
							const int pos = TranslatePOV(
							  state.rgdwPOV[in.ofs - DIJOFS_POV(0)]);
							ButtonPressed(DeviceInput(
							  dev, JOY_HAT_UP, !!(pos & HAT_UP_MASK), tm));
							ButtonPressed(DeviceInput(
							  dev, JOY_HAT_DOWN, !!(pos & HAT_DOWN_MASK), tm));
							ButtonPressed(DeviceInput(
							  dev, JOY_HAT_LEFT, !!(pos & HAT_LEFT_MASK), tm));
							ButtonPressed(DeviceInput(dev,
													  JOY_HAT_RIGHT,
													  !!(pos & HAT_RIGHT_MASK),
													  tm));
						}

						break;

					case input_t::KEY: break;
				}
			}
		} break;
		case DIDevice::MOUSE:
			// Not 100% perfect (or tested much) yet. -aj
			DIMOUSESTATE state;

			HRESULT hr = GetDeviceState(device.Device, sizeof(state), &state);
			if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
				return;

			// reset mousewheel
			ButtonPressed(DeviceInput(device.dev, MOUSE_WHEELUP, 0, tm));
			ButtonPressed(DeviceInput(device.dev, MOUSE_WHEELDOWN, 0, tm));

			for (unsigned i = 0; i < device.Inputs.size(); ++i) {
				const input_t& in = device.Inputs[i];
				const InputDevice dev = device.dev;

				switch (in.type) {
					case input_t::BUTTON: {
						// force mouse xy positions to update before click
						// inputs are sent -mina
						POINT cursorPos;
						GetCursorPos(&cursorPos);
						ScreenToClient(GraphicsWindow::GetHwnd(), &cursorPos);
						INPUTFILTER->UpdateCursorLocation(
						  static_cast<float>(cursorPos.x),
						  static_cast<float>(cursorPos.y));

						DeviceInput di(
						  dev,
						  enum_add2(MOUSE_LEFT, in.num),
						  !!state.rgbButtons[in.ofs - DIMOFS_BUTTON0],
						  tm);
						ButtonPressed(di);
						break;
					}
					case input_t::AXIS: {
						DeviceButton neg = DeviceButton_Invalid,
									 pos = DeviceButton_Invalid;
						int val = 0;
						if (in.ofs == DIMOFS_X) {
							neg = MOUSE_X_LEFT;
							pos = MOUSE_X_RIGHT;
							val = state.lX;
						} else if (in.ofs == DIMOFS_Y) {
							neg = MOUSE_Y_UP;
							pos = MOUSE_Y_DOWN;
							val = state.lY;
						} else if (in.ofs == DIMOFS_Z) {
							neg = MOUSE_WHEELDOWN;
							pos = MOUSE_WHEELUP;
							val = state.lZ;
							// Locator::getLogger()->trace("MouseWheel polled: %i",val);
							INPUTFILTER->UpdateMouseWheel(
							  static_cast<float>(val));
							if (val == 0) {
								// release all
								ButtonPressed(DeviceInput(dev, pos, 0, tm));
								ButtonPressed(DeviceInput(dev, neg, 0, tm));
							} else if (val > 0) {
								// positive values: WheelUp
								ButtonPressed(DeviceInput(dev, pos, 1, tm));
								ButtonPressed(DeviceInput(dev, neg, 0, tm));
							} else if (val < 0) {
								// negative values: WheelDown
								ButtonPressed(DeviceInput(dev, neg, 1, tm));
								ButtonPressed(DeviceInput(dev, pos, 0, tm));
							}
						} else
							Locator::getLogger()->warn("Mouse '{}' is returning an unknown mouse offset, {}",
										device.m_sName.c_str(), in.ofs);
						break;
					}
					case input_t::KEY:
					case input_t::HAT:
						break;
				}
			}
			break;
	}
}

void
InputHandler_DInput::UpdateBuffered(
  DIDevice& device,
  const std::chrono::steady_clock::time_point& tm)
{
	DWORD numevents;
	DIDEVICEOBJECTDATA evtbuf[INPUT_QSIZE];

	numevents = INPUT_QSIZE;
	HRESULT hr = device.Device->GetDeviceData(
	  sizeof(DIDEVICEOBJECTDATA), evtbuf, &numevents, 0);
	if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
		INPUTFILTER->ResetDevice(device.dev);
		return;
	}

	if (hr != DI_OK) {
		Locator::getLogger()->trace("{}", hr_ssprintf(
		  hr, "UpdateBuffered: IDirectInputDevice2_GetDeviceData"));
		return;
	}

	if (GetForegroundWindow() != GraphicsWindow::GetHwnd()) {
		// Discard input when not focused, and release all keys.
		INPUTFILTER->ResetDevice(device.dev);
		return;
	}

	// reset mousewheel
	DeviceInput diUp = DeviceInput(device.dev, MOUSE_WHEELUP, 0.0f, tm);
	ButtonPressed(diUp);
	DeviceInput diDown = DeviceInput(device.dev, MOUSE_WHEELDOWN, 0.0f, tm);
	ButtonPressed(diDown);

	for (int i = 0; i < static_cast<int>(numevents); ++i) {
		for (unsigned j = 0; j < device.Inputs.size(); ++j) {
			const input_t& in = device.Inputs[j];
			const InputDevice dev = device.dev;

			if (evtbuf[i].dwOfs != in.ofs)
				continue;

			switch (in.type) {
				case input_t::KEY:
					ButtonPressed(DeviceInput(dev,
											  (DeviceButton)in.num,
											  !!(evtbuf[i].dwData & 0x80),
											  tm));
					break;

				case input_t::BUTTON:
					if (dev == DEVICE_MOUSE) {
						// force mouse xy positions to update before click
						// inputs are sent -mina
						POINT cursorPos;
						GetCursorPos(&cursorPos);
						ScreenToClient(GraphicsWindow::GetHwnd(), &cursorPos);
						INPUTFILTER->UpdateCursorLocation(
						  static_cast<float>(cursorPos.x),
						  static_cast<float>(cursorPos.y));

						DeviceButton mouseInput = DeviceButton_Invalid;

						if (in.ofs == DIMOFS_BUTTON0)
							mouseInput = MOUSE_LEFT;
						else if (in.ofs == DIMOFS_BUTTON1)
							mouseInput = MOUSE_RIGHT;
						else if (in.ofs == DIMOFS_BUTTON2)
							mouseInput = MOUSE_MIDDLE;
						else
							Locator::getLogger()->warn("Mouse '{}' is returning an unknown mouse offset [button], {}",
										device.m_sName.c_str(),in.ofs);
						ButtonPressed(
						  DeviceInput(dev, mouseInput, !!evtbuf[i].dwData, tm));
					} else
						ButtonPressed(
						  DeviceInput(dev,
									  enum_add2(JOY_BUTTON_1, in.num),
									  !!evtbuf[i].dwData,
									  tm));
					break;

				case input_t::AXIS: {
					DeviceButton up = DeviceButton_Invalid,
								 down = DeviceButton_Invalid;
					if (dev == DEVICE_MOUSE) {
						float l = static_cast<float>(
						  static_cast<int>(evtbuf[i].dwData));
						POINT cursorPos;
						GetCursorPos(&cursorPos);
						// convert screen coordinates to client
						ScreenToClient(GraphicsWindow::GetHwnd(), &cursorPos);

						if (in.ofs == DIMOFS_X)
							INPUTFILTER->UpdateCursorLocation(
							  static_cast<float>(cursorPos.x),
							  static_cast<float>(cursorPos.y));
						else if (in.ofs == DIMOFS_Y)
							INPUTFILTER->UpdateCursorLocation(
							  static_cast<float>(cursorPos.x),
							  static_cast<float>(cursorPos.y));
						else if (in.ofs == DIMOFS_Z) {
							// positive values: WheelUp
							// negative values: WheelDown
							INPUTFILTER->UpdateMouseWheel(l);
							{
								up = MOUSE_WHEELUP;
								down = MOUSE_WHEELDOWN;
								float fWheelDelta = l;
								// l = SCALE(
								// static_cast<int>(evtbuf[i].dwData),
								// -WHEEL_DELTA, WHEEL_DELTA, 1.0f, -1.0f );
								if (l > 0) {
									DeviceInput diUp =
									  DeviceInput(dev, up, 1.0f, tm);
									DeviceInput diDown =
									  DeviceInput(dev, down, 0.0f, tm);
									// This if statement used to be a while
									// loop.  But Kevin reported that scrolling
									// the mouse wheel locked up input. I assume
									// that fWheelDelta was some absurdly large
									// value, causing an infinite loop.
									// Changing the while loop to an if fixed
									// the input lockup.  Anything that wants to
									// imitate multiple presses when the wheel
									// is scrolled will have to do it manually.
									// -Kyz
									if (fWheelDelta >= WHEEL_DELTA) {
										ButtonPressed(diUp);
										ButtonPressed(diDown);
										INPUTFILTER->UpdateMouseWheel(
										  fWheelDelta);
										fWheelDelta -= WHEEL_DELTA;
									}
								} else if (l < 0) {
									DeviceInput diDown =
									  DeviceInput(dev, down, 1.0f, tm);
									DeviceInput diUp =
									  DeviceInput(dev, up, 0.0f, tm);
									// See comment for the l > 0 case. -Kyz
									if (fWheelDelta <= -WHEEL_DELTA) {
										ButtonPressed(diDown);
										ButtonPressed(diUp);
										INPUTFILTER->UpdateMouseWheel(
										  fWheelDelta);
										fWheelDelta += WHEEL_DELTA;
									}
								} else {
									DeviceInput diUp =
									  DeviceInput(dev, up, 0.0f, tm);
									ButtonPressed(diUp);
									DeviceInput diDown =
									  DeviceInput(dev, down, 0.0f, tm);
									ButtonPressed(diDown);
								}
							}
						} else
							Locator::getLogger()->warn("Mouse '{}' is returning an unknown mouse offset [axis], {}",
										device.m_sName.c_str(), in.ofs);
					} else {
						// joystick
						if (in.ofs == DIJOFS_X) {
							up = JOY_LEFT;
							down = JOY_RIGHT;
						} else if (in.ofs == DIJOFS_Y) {
							up = JOY_UP;
							down = JOY_DOWN;
						} else if (in.ofs == DIJOFS_Z) {
							up = JOY_Z_UP;
							down = JOY_Z_DOWN;
						} else if (in.ofs == DIJOFS_RX) {
							up = JOY_ROT_UP;
							down = JOY_ROT_DOWN;
						} else if (in.ofs == DIJOFS_RY) {
							up = JOY_ROT_LEFT;
							down = JOY_ROT_RIGHT;
						} else if (in.ofs == DIJOFS_RZ) {
							up = JOY_ROT_Z_UP;
							down = JOY_ROT_Z_DOWN;
						} else if (in.ofs == DIJOFS_SLIDER(0)) {
							up = JOY_AUX_1;
							down = JOY_AUX_2;
						} else if (in.ofs == DIJOFS_SLIDER(1)) {
							up = JOY_AUX_3;
							down = JOY_AUX_4;
						} else
							Locator::getLogger()->warn("Controller '{}' is returning an unknown joystick offset, {}",
										device.m_sName.c_str(), in.ofs);

						float l = SCALE(static_cast<int>(evtbuf[i].dwData),
										0.0f,
										100.0f,
										0.0f,
										1.0f);
						if (GamePreferences::m_AxisFix) {
							ButtonPressed(
							  DeviceInput(dev, up, (l == 0) || (l == -1), tm));
							ButtonPressed(
							  DeviceInput(dev, down, (l == 0) || (l == 1), tm));

						} else {
							ButtonPressed(
							  DeviceInput(dev, up, std::max(-l, 0.F), tm));
							ButtonPressed(
							  DeviceInput(dev, down, std::max(+l, 0.F), tm));
						}
					}
					break;
				}

				case input_t::HAT: {
					const int pos = TranslatePOV(evtbuf[i].dwData);
					ButtonPressed(
					  DeviceInput(dev, JOY_HAT_UP, !!(pos & HAT_UP_MASK), tm));
					ButtonPressed(DeviceInput(
					  dev, JOY_HAT_DOWN, !!(pos & HAT_DOWN_MASK), tm));
					ButtonPressed(DeviceInput(
					  dev, JOY_HAT_LEFT, !!(pos & HAT_LEFT_MASK), tm));
					ButtonPressed(DeviceInput(
					  dev, JOY_HAT_RIGHT, !!(pos & HAT_RIGHT_MASK), tm));
				}
			}
		}
	}
}

void
InputHandler_DInput::PollAndAcquireDevices(bool bBuffered)
{
	for (unsigned i = 0; i < Devices.size(); ++i) {
		if (Devices[i].buffered != bBuffered)
			continue;

		HRESULT hr = Devices[i].Device->Poll();
		if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
			INPUTFILTER->ResetDevice(Devices[i].dev);

			/* This will fail with "access denied" on the keyboard if we don't
			 * have focus. */
			hr = Devices[i].Device->Acquire();
			if (hr != DI_OK)
				continue;

			Devices[i].Device->Poll();
		}
	}
}

void
InputHandler_DInput::Update()
{
	// Handle polled devices. Handle buffered, too, if there's no input thread
	// to do it.
	PollAndAcquireDevices(false);
	if (!m_InputThread.IsCreated())
		PollAndAcquireDevices(true);

	for (unsigned i = 0; i < Devices.size(); ++i) {
		if (!Devices[i].buffered) {
			UpdatePolled(Devices[i], std::chrono::steady_clock::now());
		} else if (!m_InputThread.IsCreated()) {
			// If we have an input thread, it'll handle buffered devices.
			UpdateBuffered(Devices[i], std::chrono::steady_clock::now());
		}
	}

	InputHandler::UpdateTimer();
}

bool
InputHandler_DInput::DevicesChanged()
{
	/* Basically, check if devices are changed, especially joysticks.
	This should only be called when appropriate (when Windows says devices
	changed) The reason we don't just keep polling is because that's slow.
	GetNumHidDevices() should be fast enough to not be slow, then just use logic
	and modern magic.
	*/

	int iOldNumHidDevices = m_iLastSeenNumHidDevices;
	m_iLastSeenNumHidDevices = GetNumHidDevices();
	if (iOldNumHidDevices != m_iLastSeenNumHidDevices ||
		g_forceJoystickPolling) {
		g_forceJoystickPolling = false;
		Locator::getLogger()->warn("Caught HID Changes. Checking for Joystick Changes.");
		int iOldNumJoysticks = m_iLastSeenNumJoysticks;
		m_iLastSeenNumJoysticks = GetNumJoysticksSlow();
		if (iOldNumJoysticks != m_iLastSeenNumJoysticks) {
			Locator::getLogger()->warn("Caught Joystick Changes.");
			m_iNumTimesLeftToPollForJoysticksChanged = 0;
			return true;
		}
	}

	return false;
}

void
InputHandler_DInput::InputThreadMain()
{
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
		Locator::getLogger()->warn("{}", werr_ssprintf(
		  GetLastError(), "Failed to set DirectInput thread priority"));

	// Enable priority boosting.
	SetThreadPriorityBoost(GetCurrentThread(), FALSE);

	std::vector<DIDevice*> BufferedDevices;
	HANDLE Handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	for (unsigned i = 0; i < Devices.size(); ++i) {
		if (!Devices[i].buffered)
			continue;

		BufferedDevices.push_back(&Devices[i]);

		Devices[i].Device->Unacquire();
		HRESULT hr = Devices[i].Device->SetEventNotification(Handle);
		if (FAILED(hr))
			Locator::getLogger()->warn("IDirectInputDevice2_SetEventNotification failed on {}", i);
		Devices[i].Device->Acquire();
	}

	while (!m_bShutdown) {
		if (BufferedDevices.size()) {
			// Update buffered devices.
			PollAndAcquireDevices(true);

			int ret = WaitForSingleObjectEx(Handle, 50, true);
			if (ret == -1) {
				Locator::getLogger()->trace("{}", werr_ssprintf(
				  GetLastError(), "WaitForSingleObjectEx failed"));
				continue;
			}

			/* Update devices even if no event was triggered, since this also
			 * checks for focus loss. */
			auto now = std::chrono::steady_clock::now();
			for (unsigned i = 0; i < BufferedDevices.size(); ++i)
				UpdateBuffered(*BufferedDevices[i], now);
		}

		// If we have no buffered devices, we didn't delay at
		// WaitForMultipleObjectsEx.
		if (BufferedDevices.size() == 0)
			usleep(50000);
	}

	for (unsigned i = 0; i < Devices.size(); ++i) {
		if (!Devices[i].buffered)
			continue;

		Devices[i].Device->Unacquire();
		Devices[i].Device->SetEventNotification(nullptr);
	}

	CloseHandle(Handle);
}

void
InputHandler_DInput::GetDevicesAndDescriptions(
  std::vector<InputDeviceInfo>& vDevicesOut)
{
	for (unsigned i = 0; i < Devices.size(); ++i)
		vDevicesOut.push_back(
		  InputDeviceInfo(Devices[i].dev, Devices[i].m_sName));
}

static wchar_t
ScancodeAndKeysToChar(DWORD scancode, unsigned char keys[256])
{
	static HKL layout = GetKeyboardLayout(0); // 0 == current thread
	UINT vk = MapVirtualKeyEx(scancode, 1, layout);

	static bool bInitialized = false;

	typedef int(WINAPI TOUNICODEEX)(IN UINT wVirtKey,
									IN UINT wScanCode,
									IN CONST BYTE * lpKeyState,
									OUT LPWSTR pwszBuff,
									IN int cchBuff,
									IN UINT wFlags,
									IN HKL dwhkl);
	static TOUNICODEEX* pToUnicodeEx;

	if (!bInitialized) {
		bInitialized = true;
		HMODULE hModule = GetModuleHandle("user32.dll");
		pToUnicodeEx = (TOUNICODEEX*)GetProcAddress(hModule, "ToUnicodeEx");
	}

	unsigned short result[2]; // ToAscii writes a max of 2 chars
	ZERO(result);

	if (pToUnicodeEx != nullptr) {
		int iNum =
		  pToUnicodeEx(vk, scancode, keys, (LPWSTR)result, 2, 0, layout);
		if (iNum == 1)
			return result[0];
	} else {
		int iNum = ToAsciiEx(vk, scancode, keys, result, 0, layout);
		// iNum == 2 will happen only for dead keys. See MSDN for ToAsciiEx.
		if (iNum == 1) {
			std::string s = std::string() + (char)result[0];
			return ConvertCodepageToWString(s, CP_ACP)[0];
		}
	}

	return '\0';
}

wchar_t
InputHandler_DInput::DeviceButtonToChar(DeviceButton button,
										bool bUseCurrentKeyModifiers)
{
	// ToAsciiEx maps these keys to a character. They shouldn't be mapped to any
	// character.
	switch (button) {
		case KEY_ESC:
		case KEY_TAB:
		case KEY_ENTER:
		case KEY_BACK:
			return '\0';
		default: break;
	}

	for (auto& d : Devices) {
		if (d.type != DIDevice::KEYBOARD)
			continue;

		for (auto& i : d.Inputs) {
			if (button != i.num)
				continue;

			unsigned char keys[256];
			ZERO(keys);
			if (bUseCurrentKeyModifiers)
				GetKeyboardState(keys);
			// todo: handle Caps Lock -freem
			wchar_t c = ScancodeAndKeysToChar(i.ofs, keys);
			if (c)
				return c;
		}
	}

	return InputHandler::DeviceButtonToChar(button, bUseCurrentKeyModifiers);
}

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
