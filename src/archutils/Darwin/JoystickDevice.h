#ifndef JOYSTICK_DEVICE_H
#define JOYSTICK_DEVICE_H

#include "HIDDevice.h"

struct Joystick
{
	InputDevice id;
	// map cookie to button
	__gnu_cxx::hash_map<IOHIDElementCookie, DeviceButton> mapping;
	IOHIDElementCookie x_axis, y_axis, z_axis, x_rot, y_rot, z_rot, hat;
	int x_min, x_max;
	int y_min, y_max;
	int z_min, z_max;
	int rx_min, rx_max;
	int ry_min, ry_max;
	int rz_min, rz_max;
	int hat_min, hat_max;

	Joystick();
};

class JoystickDevice : public HIDDevice
{
  private:
	vector<Joystick> m_vSticks;

  protected:
	bool AddLogicalDevice(int usagePage, int usage);
	void AddElement(int usagePage,
					int usage,
					IOHIDElementCookie cookie,
					const CFDictionaryRef properties);
	void Open();
	bool InitDevice(int vid, int pid);

  public:
	void GetButtonPresses(
	  vector<DeviceInput>& vPresses,
	  IOHIDElementCookie cookie,
	  int value,
	  const std::chrono::time_point<std::chrono::steady_clock>& now) const;
	int AssignIDs(InputDevice startID);
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevices) const;
};

#endif
