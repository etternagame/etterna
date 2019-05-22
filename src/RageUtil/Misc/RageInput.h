﻿/* RageInput - Starts up InputHandlers, which generate InputEvents. */

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
	void GetDevicesAndDescriptions(std::vector<InputDeviceInfo>& vOut) const;
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

/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard
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
