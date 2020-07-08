/* InputFilter - Checks RageInput and generates a list of InputEvents,
 * representing button presses, releases, and repeats. */

#ifndef INPUT_FILTER_H
#define INPUT_FILTER_H

#include "RageUtil/Misc/RageInputDevice.h"

enum InputEventType
{
	// The device was just pressed.
	IET_FIRST_PRESS,

	/* The device is auto-repeating. This event is guaranteed to be sent only
	 * between IET_FIRST_PRESS and IET_RELEASE pairs. */
	IET_REPEAT,

	/* The device is no longer pressed. Exactly one IET_RELEASE event will be
	 * sent for each IET_FIRST_PRESS. */
	IET_RELEASE,

	NUM_InputEventType,
	InputEventType_Invalid
};

const std::string&
InputEventTypeToString(InputEventType cat);
const std::string&
InputEventTypeToLocalizedString(InputEventType cat);
LuaDeclareType(InputEventType);

struct InputEvent
{
	InputEvent() = default;

	DeviceInput di;
	InputEventType type{ IET_FIRST_PRESS };

	// A list of all buttons that were pressed at the time of this event:
	DeviceInputList m_ButtonState;
};

struct MouseCoordinates
{
	float fX;
	float fY;
	float fZ;
};

class RageMutex;
struct ButtonState;
class InputFilter
{
  public:
	void ButtonPressed(const DeviceInput& di);
	void SetButtonComment(const DeviceInput& di,
						  const std::string& sComment = "");
	void ResetDevice(InputDevice dev);

	InputFilter();
	~InputFilter();
	void Reset();
	void Update(float fDeltaTime);

	void SetRepeatRate(float fRepeatRate);
	void SetRepeatDelay(float fDelay);
	void ResetRepeatRate();
	void ResetKeyRepeat(const DeviceInput& di);
	void RepeatStopKey(const DeviceInput& di);

	// If aButtonState is NULL, use the last reported state.
	bool IsBeingPressed(const DeviceInput& di,
						const DeviceInputList* pButtonState = nullptr) const;
	bool IsKBKeyPressed(DeviceButton k) const;
	bool IsControlPressed() const;
	bool IsShiftPressed() const;
	float GetSecsHeld(const DeviceInput& di,
					  const DeviceInputList* pButtonState = nullptr) const;
	float GetLevel(const DeviceInput& di,
				   const DeviceInputList* pButtonState = nullptr) const;
	std::string GetButtonComment(const DeviceInput& di) const;

	void GetInputEvents(std::vector<InputEvent>& aEventOut);
	void GetPressedButtons(std::vector<DeviceInput>& array) const;

	// cursor
	void UpdateCursorLocation(float _fX, float _fY);
	void UpdateMouseWheel(float _fZ);
	float GetCursorX() { return m_MouseCoords.fX; }
	float GetCursorY() { return m_MouseCoords.fY; }
	float GetMouseWheel() { return m_MouseCoords.fZ; }

	// Lua
	void PushSelf(lua_State* L);

  private:
	void CheckButtonChange(ButtonState& bs,
						   DeviceInput di,
						   const std::chrono::steady_clock::time_point& now);
	void ReportButtonChange(const DeviceInput& di, InputEventType t);
	void MakeButtonStateList(std::vector<DeviceInput>& aInputOut) const;

	std::vector<InputEvent> queue;
	RageMutex* queuemutex;
	MouseCoordinates m_MouseCoords;

	InputFilter(const InputFilter& rhs);
	InputFilter& operator=(const InputFilter& rhs);
};

extern InputFilter*
  INPUTFILTER; // global and accessible from anywhere in our program

#endif
