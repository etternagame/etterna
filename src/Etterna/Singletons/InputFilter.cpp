#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Models/Misc/GameInput.h"
#include "InputFilter.h"
#include "InputMapper.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Misc/Preference.h"
#include "RageUtil/Misc/RageInput.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Core/Services/Locator.hpp"
#include "Core/Platform/Platform.hpp"

#include <map>
#include <set>
#include <algorithm>

using std::map;
using std::set;

static const char* InputEventTypeNames[] = { "FirstPress",
											 "Repeat",
											 "Release" };

XToString(InputEventType);
XToLocalizedString(InputEventType);
LuaXType(InputEventType);

struct ButtonState
{
	ButtonState();
	bool m_BeingHeld;		  // actual current state
	bool m_bLastReportedHeld; // last state reported by Update()
	std::string m_sComment;
	float m_fSecsHeld;
	DeviceInput m_DeviceInput;

	// Timestamp of m_BeingHeld changing.
	std::chrono::steady_clock::time_point m_BeingHeldTime;

	// The time that we actually reported the last event (used for debouncing).
	std::chrono::steady_clock::time_point m_LastReportTime;

	// The timestamp of the last reported change. Unlike m_BeingHeldTime, this
	// value is debounced along with the input state. (This is the same as
	// m_fSecsHeld, except this isn't affected by Update scaling.)
	std::chrono::steady_clock::time_point m_LastInputTime;
};

struct DeviceButtonPair
{
	InputDevice device;
	DeviceButton button;
	DeviceButtonPair(InputDevice d, DeviceButton b)
	  : device(d)
	  , button(b)
	{
	}
};

inline bool
operator<(DeviceButtonPair const& lhs, DeviceButtonPair const& rhs)
{
	if (lhs.device != rhs.device) {
		return lhs.device < rhs.device;
	}
	return lhs.button < rhs.button;
}
inline bool
operator>(DeviceButtonPair const& lhs, DeviceButtonPair const& rhs)
{
	return operator<(rhs, lhs);
}
inline bool
operator<=(DeviceButtonPair const& lhs, DeviceButtonPair const& rhs)
{
	return !operator<(rhs, lhs);
}
inline bool
operator>=(DeviceButtonPair const& lhs, DeviceButtonPair const& rhs)
{
	return !operator<(lhs, rhs);
}

namespace {
/* Maintain a set of all interesting buttons: buttons which are being held
 * down, or which were held down and need a RELEASE event. We use this to
 * optimize InputFilter::Update, so we don't have to process every button
 * we know about when most of them aren't in use. This set is protected
 * by queuemutex. */
typedef std::map<DeviceButtonPair, ButtonState> ButtonStateMap;
ButtonStateMap g_ButtonStates;
ButtonState&
GetButtonState(const DeviceInput& di)
{
	DeviceButtonPair db(di.device, di.button);
	ButtonState& bs = g_ButtonStates[db];
	bs.m_DeviceInput.button = di.button;
	bs.m_DeviceInput.device = di.device;
	return bs;
}

DeviceInputList g_CurrentState;
set<DeviceInput> g_DisableRepeat;
} // namespace

/* Some input devices require debouncing. Do this on both press and release.
 * After reporting a change in state, don't report another for the debounce
 * period. If a button is reported pressed, report it. If the button is
 * immediately reported released, wait a period before reporting it; if the
 * button is repressed during that time, the release is never reported.
 * The detail is important: if a button is pressed for 1ms and released, we must
 * always report it, even if the debounce period is 10ms, since it might be a
 * coin counter with a very short signal. The only time we discard events is if
 * a button is pressed, released and then pressed again quickly.
 *
 * This delay in events is ordinarily not noticable, because we report initial
 * presses and releases immediately.  However, if a real press is ever delayed,
 * this won't cause timing problems, because the event timestamp is preserved.
 */
static Preference<float> g_fInputDebounceTime("InputDebounceTime", 0);

InputFilter* INPUTFILTER =
  nullptr; // global and accessible from anywhere in our program

static const float TIME_BEFORE_REPEATS = 0.375f;

static const float REPEATS_PER_SEC = 8;

static float g_fTimeBeforeRepeats, g_fTimeBetweenRepeats;

InputFilter::InputFilter()
{
	queuemutex = new RageMutex("InputFilter");

	Reset();
	ResetRepeatRate();

	m_MouseCoords.fX = 0;
	m_MouseCoords.fY = 0;
	m_MouseCoords.fZ = 0;

	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "INPUTFILTER");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

InputFilter::~InputFilter()
{
	delete queuemutex;
	g_ButtonStates.clear();
	// Unregister with Lua.
	LUA->UnsetGlobal("INPUTFILTER");
}

void
InputFilter::Reset()
{
	FOREACH_InputDevice(i) ResetDevice(InputDevice(i));
}

void
InputFilter::SetRepeatRate(float fRepeatRate)
{
	g_fTimeBetweenRepeats = 1 / fRepeatRate;
}

void
InputFilter::SetRepeatDelay(float fDelay)
{
	g_fTimeBeforeRepeats = fDelay;
}

void
InputFilter::ResetRepeatRate()
{
	SetRepeatRate(REPEATS_PER_SEC);
	SetRepeatDelay(TIME_BEFORE_REPEATS);
}

ButtonState::ButtonState()
  : m_BeingHeldTime(std::chrono::microseconds{ 0 })
  , m_LastReportTime(std::chrono::microseconds{ 0 })
{
	m_BeingHeld = false;
	m_bLastReportedHeld = false;
	m_fSecsHeld = 0;
}

void
InputFilter::ButtonPressed(const DeviceInput& di)
{
	LockMut(*queuemutex);

	if (di.ts == std::chrono::time_point<std::chrono::steady_clock>())
		Locator::getLogger()->warn(
		  "InputFilter::ButtonPressed: zero timestamp is invalid");

	// Filter out input that is beyond the range of the current system.
	if (di.device >= NUM_InputDevice) {
		Locator::getLogger()->warn(
		  "InputFilter::ButtonPressed: Invalid device {}", di.device);
		return;
	}
	if (di.button >= NUM_DeviceButton) {
		Locator::getLogger()->warn(
		  "InputFilter::ButtonPressed: Invalid button {}", di.button);
		return;
	}

	ButtonState& bs = GetButtonState(di);

	// Flush any delayed input, like Update() (in case Update() isn't being
	// called).
	auto now = std::chrono::steady_clock::now();
	CheckButtonChange(bs, di, now);

	bs.m_DeviceInput = di;

	bool Down = di.bDown;
	if (bs.m_BeingHeld != Down) {
		bs.m_BeingHeld = Down;
		bs.m_BeingHeldTime = di.ts;
	}

	// Try to report presses immediately.
	MakeButtonStateList(g_CurrentState);
	CheckButtonChange(bs, di, now);

	if (di.button == KEY_CAPSLOCK && di.bDown) {
		// This might not be the right place to do this
		capsLockEnabled = !capsLockEnabled;
	}
}

void
InputFilter::SetButtonComment(const DeviceInput& di,
							  const std::string& sComment)
{
	LockMut(*queuemutex);
	ButtonState& bs = GetButtonState(di);
	bs.m_sComment = sComment;
}

/** @brief Release all buttons on the given device. */
void
InputFilter::ResetDevice(InputDevice device)
{
	LockMut(*queuemutex);

	const ButtonStateMap ButtonStates(g_ButtonStates);
	for (auto& b : ButtonStates) {
		const DeviceButtonPair& db = b.first;
		if (db.device == device)
			ButtonPressed(DeviceInput(
			  device, db.button, 0, std::chrono::steady_clock::now()));
	}
}

/** @brief Check for reportable presses. */
void
InputFilter::CheckButtonChange(ButtonState& bs,
							   DeviceInput di,
							   const std::chrono::steady_clock::time_point& now)
{
	if (bs.m_BeingHeld == bs.m_bLastReportedHeld)
		return;

	// Possibly apply debounce
	std::chrono::duration<float> timeDelta = now - bs.m_LastReportTime;
	float delta = timeDelta.count();
	if (di.button != MOUSE_WHEELDOWN && di.button != MOUSE_WHEELUP) {
		/* If the last IET_FIRST_PRESS or IET_RELEASE event was sent too
		 * recently, wait a while before sending it. */
		// note: a debounce time longer than the min IET_REPEAT time
		// will cause repeat events to fire here
		if (delta < g_fInputDebounceTime) {
			return;
		}
	}

	bs.m_LastReportTime = now;
	bs.m_bLastReportedHeld = bs.m_BeingHeld;
	bs.m_fSecsHeld = 0;
	bs.m_LastInputTime = bs.m_BeingHeldTime;

	di.ts = bs.m_BeingHeldTime;
	if (!bs.m_bLastReportedHeld)
		di.level = 0;

	MakeButtonStateList(g_CurrentState);
	ReportButtonChange(di,
					   bs.m_bLastReportedHeld ? IET_FIRST_PRESS : IET_RELEASE);

	if (!bs.m_bLastReportedHeld)
		g_DisableRepeat.erase(di);
}

void
InputFilter::ReportButtonChange(const DeviceInput& di, InputEventType t)
{
	queue.push_back(InputEvent());
	InputEvent& ie = queue.back();
	ie.type = t;
	ie.di = di;

	/* Include a list of all buttons that were pressed at the time of this
	 * event. We can create this efficiently using g_ButtonStates. Use a vector
	 * and not a map, for efficiency; most code will not use this information.
	 * Iterating over g_ButtonStates will be in DeviceInput order, so users can
	 * binary search this list (eg. std::lower_bound). */
	ie.m_ButtonState = g_CurrentState;
}

void
InputFilter::MakeButtonStateList(std::vector<DeviceInput>& aInputOut) const
{
	aInputOut.clear();
	aInputOut.reserve(g_ButtonStates.size());
	for (ButtonStateMap::const_iterator it = g_ButtonStates.begin();
		 it != g_ButtonStates.end();
		 ++it) {
		const ButtonState& bs = it->second;
		aInputOut.push_back(bs.m_DeviceInput);
		aInputOut.back().ts = bs.m_LastInputTime;
		aInputOut.back().bDown = bs.m_bLastReportedHeld;
	}
}

void
InputFilter::Update(float fDeltaTime)
{
	auto now = std::chrono::steady_clock::now();

	INPUTMAN->Update();

	/* Make sure that nothing gets inserted while we do this, to prevent things
	 * like "key pressed, key release, key repeat". */
	LockMut(*queuemutex);

	DeviceInput di(InputDevice_Invalid, DeviceButton_Invalid, 1.0f, now);

	MakeButtonStateList(g_CurrentState);

	std::vector<ButtonStateMap::iterator> ButtonsToErase;

	FOREACHM(DeviceButtonPair, ButtonState, g_ButtonStates, b)
	{
		di.device = b->first.device;
		di.button = b->first.button;
		ButtonState& bs = b->second;

		// Generate IET_FIRST_PRESS and IET_RELEASE events that were delayed.
		CheckButtonChange(bs, di, now);

		// Generate IET_REPEAT events.
		if (!bs.m_bLastReportedHeld) {
			// If the key isn't pressed, and hasn't been pressed for a while
			// (so debouncing isn't interested in it), purge the entry.
			std::chrono::duration<float> timeAgo = now - bs.m_LastReportTime;
			float lastReportTime = timeAgo.count();

			if (lastReportTime > g_fInputDebounceTime &&
				bs.m_DeviceInput.level == 0.0f)
				ButtonsToErase.push_back(b);
			continue;
		}

		// If repeats are disabled for this button, skip.
		if (g_DisableRepeat.find(di) != g_DisableRepeat.end())
			continue;

		const float fOldHoldTime = bs.m_fSecsHeld;
		bs.m_fSecsHeld += fDeltaTime;
		const float fNewHoldTime = bs.m_fSecsHeld;

		if (fNewHoldTime <= g_fTimeBeforeRepeats)
			continue;

		float fRepeatTime;
		if (fOldHoldTime < g_fTimeBeforeRepeats) {
			fRepeatTime = g_fTimeBeforeRepeats;
		} else {
			float fAdjustedOldHoldTime = fOldHoldTime - g_fTimeBeforeRepeats;
			float fAdjustedNewHoldTime = fNewHoldTime - g_fTimeBeforeRepeats;
			if (static_cast<int>(fAdjustedOldHoldTime /
								 g_fTimeBetweenRepeats) ==
				static_cast<int>(fAdjustedNewHoldTime / g_fTimeBetweenRepeats))
				continue;
			fRepeatTime = ftruncf(fNewHoldTime, g_fTimeBetweenRepeats);
		}

		/* Set the timestamp to the exact time of the repeat. This way, as long
		 * as tab/` aren't being used, the timestamp will always increase
		 * steadily during repeats. */
		di.ts = std::chrono::steady_clock::time_point(
		  bs.m_LastInputTime +
		  std::chrono::microseconds(static_cast<int>(fRepeatTime * 1000000)));

		ReportButtonChange(di, IET_REPEAT);
	}

	FOREACH(ButtonStateMap::iterator, ButtonsToErase, it)
	g_ButtonStates.erase(*it);
}

template<typename T, typename IT>
const T*
FindItemBinarySearch(IT begin, IT end, const T& i)
{
	IT it = lower_bound(begin, end, i);
	if (it == end || *it != i)
		return nullptr;

	return &*it;
}

bool
InputFilter::IsBeingPressed(const DeviceInput& di,
							const DeviceInputList* pButtonState) const
{
	LockMut(*queuemutex);
	if (pButtonState == nullptr)
		pButtonState = &g_CurrentState;
	const DeviceInput* pDI =
	  FindItemBinarySearch(pButtonState->begin(), pButtonState->end(), di);
	return pDI != nullptr && pDI->bDown;
}

float
InputFilter::GetSecsHeld(const DeviceInput& di,
						 const DeviceInputList* pButtonState) const
{
	LockMut(*queuemutex);
	if (pButtonState == nullptr)
		pButtonState = &g_CurrentState;
	const DeviceInput* pDI =
	  FindItemBinarySearch(pButtonState->begin(), pButtonState->end(), di);
	if (pDI == nullptr)
		return 0;

	std::chrono::duration<float> inputLength =
	  std::chrono::steady_clock::now() - pDI->ts;
	return inputLength.count();
}

float
InputFilter::GetLevel(const DeviceInput& di,
					  const DeviceInputList* pButtonState) const
{
	LockMut(*queuemutex);
	if (pButtonState == nullptr)
		pButtonState = &g_CurrentState;
	const DeviceInput* pDI =
	  FindItemBinarySearch(pButtonState->begin(), pButtonState->end(), di);
	if (pDI == nullptr)
		return 0.0f;
	return pDI->level;
}

std::string
InputFilter::GetButtonComment(const DeviceInput& di) const
{
	LockMut(*queuemutex);
	return GetButtonState(di).m_sComment;
}

void
InputFilter::ResetKeyRepeat(const DeviceInput& di)
{
	LockMut(*queuemutex);
	GetButtonState(di).m_fSecsHeld = 0;
}

/** @brief Stop repeating the specified key until released. */
void
InputFilter::RepeatStopKey(const DeviceInput& di)
{
	LockMut(*queuemutex);

	// If the button is up, do nothing.
	ButtonState& bs = GetButtonState(di);
	if (!bs.m_bLastReportedHeld)
		return;

	g_DisableRepeat.insert(di);
}

void
InputFilter::GetInputEvents(std::vector<InputEvent>& array)
{
	array.clear();
	LockMut(*queuemutex);
	array.swap(queue);
}

void
InputFilter::GetPressedButtons(std::vector<DeviceInput>& array) const
{
	LockMut(*queuemutex);
	array = g_CurrentState;
}

void
InputFilter::UpdateCursorLocation(float _fX, float _fY)
{
	m_MouseCoords.fX = _fX;

	m_MouseCoords.fY = _fY;
}

void
InputFilter::UpdateMouseWheel(float _fZ)
{
	m_MouseCoords.fZ = _fZ;
}

bool
InputFilter::IsKBKeyPressed(DeviceButton k) const
{
	return INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, k));
}

bool
InputFilter::IsControlPressed() const
{
	return IsKBKeyPressed(KEY_LCTRL) || IsKBKeyPressed(KEY_RCTRL);
}

bool
InputFilter::IsShiftPressed() const
{
	return IsKBKeyPressed(KEY_LSHIFT) || IsKBKeyPressed(KEY_RSHIFT);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to InputFilter. */
class LunaInputFilter : public Luna<InputFilter>
{
  public:
	static int GetMouseX(T* p, lua_State* L)
	{
		float fX = p->GetCursorX();
		fX = SCALE(fX,
				   0,
				   Core::Platform::getWindowDimensions().width,
				   SCREEN_LEFT,
				   SCREEN_RIGHT);
		lua_pushnumber(L, fX);
		return 1;
	}
	static int GetMouseY(T* p, lua_State* L)
	{
		float fY = p->GetCursorY();
		fY = SCALE(fY,
				   0,
				   Core::Platform::getWindowDimensions().height,
				   SCREEN_TOP,
				   SCREEN_BOTTOM);
		lua_pushnumber(L, fY);
		return 1;
	}
	static int GetMouseWheel(T* p, lua_State* L)
	{
		float fZ = p->GetMouseWheel();
		lua_pushnumber(L, fZ);
		return 1;
	}
	static int IsBeingPressed(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) {
			return luaL_error(L,
							  "IsBeingPressed(button, inputDevice=keyboard) "
							  "expects at least one parameter");
		}
		DeviceButton button = StringToDeviceButton(SArg(1));
		InputDevice device = DEVICE_KEYBOARD;
		if (!(lua_isnil(L, 2)) && lua_gettop(L) > 1) {
			device = StringToInputDevice(SArg(2));
		}
		lua_pushboolean(
		  L, INPUTFILTER->IsBeingPressed(DeviceInput(device, button)));
		return 1;
	}
	DEFINE_METHOD(IsShiftPressed, IsShiftPressed());
	DEFINE_METHOD(IsControlPressed, IsControlPressed());
	LunaInputFilter()
	{
		ADD_METHOD(GetMouseX);
		ADD_METHOD(GetMouseY);
		ADD_METHOD(GetMouseWheel);
		ADD_METHOD(IsBeingPressed);
		ADD_METHOD(IsShiftPressed);
		ADD_METHOD(IsControlPressed);
	}
};

LUA_REGISTER_CLASS(InputFilter)
// lua end
