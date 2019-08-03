#ifndef PUMP_DEVICE_H
#define PUMP_DEVICE_H

#include "HIDDevice.h"

class PumpDevice : public HIDDevice
{
  private:
	InputDevice m_Id;

  protected:
	bool AddLogicalDevice(int usagePage, int usage) { return true; }
	void AddElement(int usagePage,
					int usage,
					IOHIDElementCookie cookie,
					const CFDictionaryRef properties)
	{
	}
	void Open();
	bool InitDevice(int vid, int pid) { return vid == 0x0d2f && pid == 0x0001; }

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
