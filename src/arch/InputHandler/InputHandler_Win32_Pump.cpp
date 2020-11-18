#include "Etterna/Globals/global.h"
#include "InputHandler_Win32_Pump.h"

#include "Etterna/Singletons/PrefsManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Misc/RageInputDevice.h"
#include "archutils/Win32/ErrorStrings.h"
#include "archutils/Win32/USB.h"

REGISTER_INPUT_HANDLER_CLASS2(Pump, Win32_Pump);

InputHandler_Win32_Pump::InputHandler_Win32_Pump()
{
	m_bShutdown = false;
	const int pump_usb_vid = 0x0d2f;
	const int pump_usb_pids[2] = {
		0x0001 /* older model */, 0x0003 /* shipped with Exceed */
	};

	m_pDevice = new USBDevice[NUM_PUMPS];

	int iNumFound = 0;
	// TODO: Use C++1x semantics to loop through the pids.
	// Hardcoding for now to keep this moving.
	for (int p = 0; p < 2; ++p) {
		const int pump_usb_pid = pump_usb_pids[p];
		for (int i = 0; i < NUM_PUMPS; ++i) {
			if (m_pDevice[i].Open(
				  pump_usb_vid, pump_usb_pid, sizeof(long), i, NULL)) {
				iNumFound++;
				Locator::getLogger()->info("Found Pump pad {}", iNumFound);
			}
		}
	}

	/* Don't start a thread if we have no pads. */
	if (iNumFound > 0 && PREFSMAN->m_bThreadedInput) {
		InputThread.SetName("Pump thread");
		InputThread.Create(InputThread_Start, this);
	}
}

InputHandler_Win32_Pump::~InputHandler_Win32_Pump()
{
	if (InputThread.IsCreated()) {
		m_bShutdown = true;
		if (PREFSMAN->m_verbose_log > 1)
			Locator::getLogger()->trace("Shutting down Pump thread ...");
		InputThread.Wait();
		if (PREFSMAN->m_verbose_log > 1)
			Locator::getLogger()->trace("Pump thread shut down.");
	}

	delete[] m_pDevice;
}

void
InputHandler_Win32_Pump::HandleInput(int iDevice, int iEvent)
{
	static const int bits[] = {
		/* P1 */ (1 << 9),	 (1 << 12), (1 << 13),
		(1 << 11),			 (1 << 10),
		/* ESC */ (1 << 16),
		/* P1 */ (1 << 17),	 (1 << 20), (1 << 21),
		(1 << 19),			 (1 << 18),
	};

	InputDevice id = InputDevice(DEVICE_PUMP1 + iDevice);

	for (int iButton = 0; iButton < ARRAYLEN(bits); ++iButton) {
		DeviceInput di(
		  id, enum_add2(JOY_BUTTON_1, iButton), !(iEvent & bits[iButton]));

		/* If we're in a thread, our timestamp is accurate. */
		if (InputThread.IsCreated())
			di.ts = std::chrono::steady_clock::now();

		ButtonPressed(di);
	}
}

std::string
InputHandler_Win32_Pump::GetDeviceSpecificInputString(const DeviceInput& di)
{
	switch (di.button) {
		case JOY_BUTTON_1:
			return "UL";
		case JOY_BUTTON_2:
			return "UR";
		case JOY_BUTTON_3:
			return "MID";
		case JOY_BUTTON_4:
			return "DL";
		case JOY_BUTTON_5:
			return "DR";
		case JOY_BUTTON_6:
			return "Esc";
		case JOY_BUTTON_7:
			return "P2 UL";
		case JOY_BUTTON_8:
			return "P2 UR";
		case JOY_BUTTON_9:
			return "P2 MID";
		case JOY_BUTTON_10:
			return "P2 DL";
		case JOY_BUTTON_11:
			return "P2 DR";
	}

	return InputHandler::GetDeviceSpecificInputString(di);
}

void
InputHandler_Win32_Pump::GetDevicesAndDescriptions(
  vector<InputDeviceInfo>& vDevicesOut)
{
	for (int i = 0; i < NUM_PUMPS; ++i) {
		if (m_pDevice[i].IsOpen()) {
			vDevicesOut.push_back(
			  InputDeviceInfo(InputDevice(DEVICE_PUMP1 + i), "Pump USB"));
		}
	}
}

int
InputHandler_Win32_Pump::InputThread_Start(void* p)
{
	((InputHandler_Win32_Pump*)p)->InputThreadMain();
	return 0;
}

void
InputHandler_Win32_Pump::InputThreadMain()
{
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
		Locator::getLogger()->warn(werr_ssprintf(GetLastError(), "Failed to set Pump thread priority"));

	/* Enable priority boosting. */
	SetThreadPriorityBoost(GetCurrentThread(), FALSE);

	vector<WindowsFileIO*> apSources;
	for (int i = 0; i < NUM_PUMPS; ++i) {
		if (m_pDevice[i].m_IO.IsOpen())
			apSources.push_back(&m_pDevice[i].m_IO);
	}

	while (!m_bShutdown) {
		int iActual = 0, iVal = 0;
		int iRet =
		  WindowsFileIO::read_several(apSources, &iVal, iActual, 0.100f);

		if (iRet <= 0)
			continue; /* no event */

		HandleInput(iActual, iVal);
		InputHandler::UpdateTimer();
	}
}

void
InputHandler_Win32_Pump::Update()
{
	if (!InputThread.IsCreated()) {
		for (int i = 0; i < NUM_PUMPS; ++i) {
			int iRet = m_pDevice[i].GetPadEvent();

			if (iRet == -1)
				continue; /* no event */

			HandleInput(i, iRet);
		}
		InputHandler::UpdateTimer();
	}
}

/*
 * (c) 2002-2004 Glenn Maynard
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
