#ifndef INPUTHANDLER_DIRECTINPUT_HELPER_H
#define INPUTHANDLER_DIRECTINPUT_HELPER_H

#include "Etterna/Singletons/InputFilter.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
extern LPDIRECTINPUT8 g_dinput;

#define INPUT_QSIZE 32

struct input_t
{
	// DirectInput offset for this input type:
	DWORD ofs;

	// Button, axis or hat:
	enum Type
	{
		KEY,
		BUTTON,
		AXIS,
		HAT
	} type;

	int num;

	// Comparitor for finding the input_t with the matching ofs member in std
	// containers.
	class Compare
	{
	  public:
		Compare(DWORD _ofs)
		  : ofs(_ofs)
		{
		}

		bool operator()(const input_t& input) const { return input.ofs == ofs; }

	  private:
		DWORD ofs;
	};
};

struct DIDevice
{
	DIDEVICEINSTANCE JoystickInst;
	LPDIRECTINPUTDEVICE8 Device;
	std::string m_sName;

	enum
	{
		KEYBOARD,
		JOYSTICK,
		MOUSE
	} type;

	bool buffered;
	int buttons, axes, hats;
	vector<input_t> Inputs;
	InputDevice dev;

	DIDevice();

	bool Open();
	void Close();
};

#endif
