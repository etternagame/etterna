#ifndef INPUT_HANDLER_MACOSX_HID_H
#define INPUT_HANDLER_MACOSX_HID_H

#include <vector>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include "InputHandler.h"
#include "RageUtil/Misc/RageThreads.h"

class HIDDevice;

class InputHandler_MacOSX_HID : public InputHandler
{
  private:
	vector<HIDDevice*> m_vDevices;
	RageThread m_InputThread;
	RageSemaphore m_Sem;
	CFRunLoopRef m_LoopRef;
	CFRunLoopSourceRef m_SourceRef;
	vector<io_iterator_t> m_vIters; // We don't really care about these but they
									// need to stick around
	IONotificationPortRef m_NotifyPort;
	RageMutex m_ChangeLock;
	bool m_bChanged;

	static int Run(void* data);
	static void DeviceAdded(void* refCon, io_iterator_t iter);
	static void DeviceChanged(void* refCon,
							  io_service_t service,
							  natural_t messageType,
							  void* arg);
	void StartDevices();
	void AddDevices(int usagePage, int usage, InputDevice& id);

  public:
	InputHandler_MacOSX_HID();
	~InputHandler_MacOSX_HID();

	bool DevicesChanged()
	{
		LockMut(m_ChangeLock);
		return m_bChanged;
	}
	void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut);
	std::string GetDeviceSpecificInputString(const DeviceInput& di);
	wchar_t DeviceButtonToChar(DeviceButton button,
							   bool bUseCurrentKeyModifiers);
	static void QueueCallback(void* target,
							  int result,
							  void* refcon,
							  void* sender);
};

#endif
