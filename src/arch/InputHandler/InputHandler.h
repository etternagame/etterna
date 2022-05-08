#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

/* This is a simple class to handle special input devices. Update() will be
 * called during the input update; the derived class should send appropriate
 * events to InputHandler.
 * Note that, if the underlying device is capable of it, you're free to start
 * a blocking thread; just store inputs in your class and send them off in a
 * batch on the next Update. This gets much more accurate timestamps; we get
 * events more quickly and timestamp them, instead of having a rough timing
 * granularity due to the framerate.
 * Send input events for a specific type of device. Only one driver for a given
 * set of InputDevice types should be loaded for a given arch. For example,
 * any number of drivers may produce DEVICE_PUMPn events, but only one may be
 * loaded at a time. (This will be inconvenient if, for example, we have two
 * completely distinct methods of getting input for the same device; we have no
 * method to allocate device numbers. We don't need this now; I'll write it
 * if it becomes needed.) */
#include "RageUtil/Misc/RageInputDevice.h" // for InputDevice
#include "arch/RageDriver.h"
/** @brief A class designed to handle special input devices. */
class InputHandler : public RageDriver
{
  public:
	static void Create(const std::string& sDrivers,
					   std::vector<InputHandler*>& apAdd);
	static DriverList m_pDriverList;

	InputHandler()
	  : m_LastUpdate()
	{
	}
	~InputHandler() override = default;
	virtual void Update() {}
	virtual bool DevicesChanged() { return false; }
	virtual void GetDevicesAndDescriptions(
	  std::vector<InputDeviceInfo>& vDevicesOut) = 0;

	// Override to return a pretty string that's specific to the controller
	// type.
	virtual std::string GetDeviceSpecificInputString(const DeviceInput& di);
	static wchar_t ApplyKeyModifiers(wchar_t c);
	virtual std::string GetLocalizedInputString(const DeviceInput& di);
	virtual wchar_t DeviceButtonToChar(DeviceButton button,
									   bool bUseCurrentKeyModifiers);

	// Override to find out whether the controller is currently plugged in.
	// Not all InputHandlers will support this.  Not applicable to all
	// InputHandlers.
	virtual InputDeviceState GetInputDeviceState(InputDevice /* id */)
	{
		return InputDeviceState_Connected;
	}

	/* In Windows, some devices need to be recreated if we recreate our main
	 * window. Override this if you need to do that. */
	virtual void WindowReset() {}

  protected:
	/* Convenience function: Call this to queue a received event.
	 * This may be called in a thread.
	 *
	 * Important detail: If the timestamp, di.ts, is zero, then it is assumed
	 * that this is not a threaded event handler.  In that case, input is being
	 * polled, and the actual time the button was pressed may be any time since
	 * the last poll.  In this case, ButtonPressed will pretend the button was
	 * pressed at the midpoint since the last update, which will smooth out the
	 * error.
	 *
	 * Note that timestamps are set to the current time by default, so for this
	 * to happen, you need to explicitly call di.ts.SetZero().
	 *
	 * If the timestamp is set, it'll be left alone. */
	void ButtonPressed(DeviceInput di);

	/* Call this at the end of polling input. */
	void UpdateTimer();

  private:
	std::chrono::time_point<std::chrono::steady_clock> m_LastUpdate;
	int m_iInputsSinceUpdate{ 0 };
};

#define REGISTER_INPUT_HANDLER_CLASS2(name, x)                                 \
	static RegisterRageDriver register_##name(                                 \
	  &InputHandler::m_pDriverList,                                            \
	  #name,                                                                   \
	  CreateClass<InputHandler_##x, RageDriver>)
#define REGISTER_INPUT_HANDLER_CLASS(name)                                     \
	REGISTER_INPUT_HANDLER_CLASS2(name, name)

#endif
