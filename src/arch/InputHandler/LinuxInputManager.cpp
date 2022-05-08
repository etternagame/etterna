#include "LinuxInputManager.h"

#include "InputHandler_Linux_Event.h"
#include "InputHandler_Linux_Joystick.h"

#include "RageUtil/Misc/RageInput.h" // g_sInputDrivers
#include "Core/Services/Locator.hpp"
#include "Etterna/Models/Misc/Foreach.h"

#include <string> // std::string::npos

#if defined(HAVE_DIRENT_H)
#include <dirent.h>
#endif

#include <errno.h>

std::string
getDevice(std::string inputDir, std::string type)
{
	std::string result = "";
	DIR* dir = opendir(inputDir.c_str());
	if (dir == NULL) {
        Locator::getLogger()->warn("LinuxInputManager: Couldn't open {}: {}.",
				  inputDir.c_str(), strerror(errno));
		return "";
	}

	struct dirent* d;
	while ((d = readdir(dir)) != NULL)
		if (strncmp(type.c_str(), d->d_name, type.size()) == 0) {
			result = std::string("/dev/input/") + d->d_name;
			break;
		}

	closedir(dir);
	return result;
}

LinuxInputManager::LinuxInputManager()
{
	m_bEventEnabled =
	  g_sInputDrivers.Get().find("LinuxEvent") != std::string::npos;
	m_bJoystickEnabled =
	  g_sInputDrivers.Get().find("LinuxJoystick") != std::string::npos;
	// HACK: If empty, assume both are enabled
	if (g_sInputDrivers.Get() == "") {
		m_bEventEnabled = true;
		m_bJoystickEnabled = true;
	}

	m_EventDriver = NULL;
	m_JoystickDriver = NULL;

	// XXX: Can I use RageFile for this?
	DIR* sysClassInput = opendir("/sys/class/input");
	if (sysClassInput == NULL) {
		// XXX: Probably should throw a Dialog. But Linux doesn't have a
		// DialogDriver yet so eh.
        Locator::getLogger()->warn("Couldn't open /sys/class/input: {}. Joysticks will not work!", strerror(errno));
		return;
	}

	struct dirent* d;
	while ((d = readdir(sysClassInput)) != NULL) {
		if (strncmp("input", d->d_name, 5) != 0)
			continue;

		std::string dName = std::string("/sys/class/input/") + d->d_name;

		bool bEventPresent = getDevice(dName, "event") != "";
		if (m_bEventEnabled && bEventPresent) {
			m_vsPendingEventDevices.push_back(dName);
			continue;
		}

		bool bJoystickPresent = getDevice(dName, "js") != "";
		if (m_bJoystickEnabled && bJoystickPresent) {
			m_vsPendingJoystickDevices.push_back(dName);
			continue;
		}

		if (!bEventPresent && !bJoystickPresent)
            Locator::getLogger()->info("LinuxInputManager: {} seems to have no eventNN or jsNN.",
					  dName.c_str());
	}
	closedir(sysClassInput);
}

void
LinuxInputManager::InitDriver(InputHandler_Linux_Event* driver)
{
	m_EventDriver = driver;

	FOREACH(std::string, m_vsPendingEventDevices, dev)
	{
		std::string devFile = getDevice(*dev, "event");
		ASSERT(devFile != "");

		if (!driver->TryDevice(devFile) && m_bJoystickEnabled &&
			getDevice(*dev, "js") != "")
			m_vsPendingJoystickDevices.push_back(*dev);
	}
	if (m_JoystickDriver != NULL)
		InitDriver(m_JoystickDriver);

	m_vsPendingEventDevices.clear();
}

void
LinuxInputManager::InitDriver(InputHandler_Linux_Joystick* driver)
{
	m_JoystickDriver = driver;

	FOREACH(std::string, m_vsPendingJoystickDevices, dev)
	{
		std::string devFile = getDevice(*dev, "js");
		ASSERT(devFile != "");

		driver->TryDevice(devFile);
	}

	m_vsPendingJoystickDevices.clear();
}

LinuxInputManager* LINUXINPUT =
  NULL; // global and accessible anywhere in our program

/*
 * (c) 2013 Ben "root" Anderson
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
