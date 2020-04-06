#ifndef KEYBOARD_DEVICE_H
#define KEYBOARD_DEVICE_H

#include "HIDDevice.h"

class KeyboardDevice : public HIDDevice
{
  private:
	__gnu_cxx::hash_map<IOHIDElementCookie, DeviceButton> m_Mapping;

  protected:
	bool AddLogicalDevice(int usagePage, int usage);
	void AddElement(int usagePage,
					int usage,
					IOHIDElementCookie cookie,
					const CFDictionaryRef properties);
	void Open();

  public:
	void GetButtonPresses(
	  vector<DeviceInput>& vPresses,
	  IOHIDElementCookie cookie,
	  int value,
	  const std::chrono::time_point<std::chrono::steady_clock>& now) const;
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevices) const;

	static bool DeviceButtonToMacVirtualKey(DeviceButton button,
											UInt8& iMacVKOut);
};

#endif
