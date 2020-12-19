#ifndef MOUSE_DEVICE_H
#define MOUSE_DEVICE_H

#include "HIDDevice.h"
#include "Etterna/Singletons/InputFilter.h"

struct Mouse
{
	float x{ 0 };
	float y{ 0 };
	InputDevice id;
	IOHIDElementCookie x_axis, y_axis;
	int x_min, x_max;
	int y_min, y_max;
	vector<int> z_min, z_max;

	vector<IOHIDElementCookie> z_axis;

	Mouse();
};
float
MACMouseX();
float
MACMouseY();
float
MACMouseScroll();

class MouseDevice : public HIDDevice
{
  private:
	__gnu_cxx::hash_map<IOHIDElementCookie, DeviceButton> m_Mapping;
	Mouse m_Mouse;

  protected:
	bool AddLogicalDevice(int usagePage, int usage);
	void AddElement(int usagePage,
					int usage,
					IOHIDElementCookie cookie,
					const CFDictionaryRef properties);
	void Open();

	// just in case -aj
	Mouse GetMouse() { return m_Mouse; }

  public:
	void GetButtonPresses(
	  vector<DeviceInput>& vPresses,
	  IOHIDElementCookie cookie,
	  int value,
	  const std::chrono::time_point<std::chrono::steady_clock>& now) const;
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevices) const;
};

#endif
