#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/InputFilter.h"
#include "RageUtil/Utils/RageUtil.h"
#include "InputHandler.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "arch/arch_default.h"
#include "Etterna/Models/Misc/Foreach.h"

void
InputHandler::UpdateTimer()
{
	m_LastUpdate = std::chrono::steady_clock::now();
	m_iInputsSinceUpdate = 0;
}

void
InputHandler::ButtonPressed(DeviceInput di)
{
	if (di.ts == std::chrono::time_point<std::chrono::steady_clock>()) {
		auto lastUpdateDelta = std::chrono::steady_clock::now() - m_LastUpdate;
		di.ts = std::chrono::time_point<std::chrono::steady_clock>(
		  m_LastUpdate + lastUpdateDelta / 2);
		++m_iInputsSinceUpdate;
	}

	INPUTFILTER->ButtonPressed(di);

	if (m_iInputsSinceUpdate >= 1000) {
		/*
		 * We haven't received an update in a long time, so warn about it.  We
		 * expect to receive input events before the first UpdateTimer call only
		 * on the first update.  Leave m_iInputsSinceUpdate where it is, so we
		 * only warn once.  Only updates that didn't provide a timestamp are
		 * counted; if the driver provides its own timestamps, UpdateTimer is
		 * optional.
		 */
		Locator::getLogger()->warn("InputHandler::ButtonPressed: Driver sent many updates without calling UpdateTimer");
		FAIL_M("x");
	}
}

wchar_t
InputHandler::ApplyKeyModifiers(wchar_t c)
{
	bool bHoldingShift = INPUTFILTER->IsShiftPressed();

	bool bHoldingCtrl = INPUTFILTER->IsControlPressed();

	// todo: handle Caps Lock -freem
	if (bHoldingShift && !bHoldingCtrl) {
		MakeUpper(&c, 1);

		switch (c) {
			case L'`':
				c = L'~';
				break;
			case L'1':
				c = L'!';
				break;
			case L'2':
				c = L'@';
				break;
			/*case L'§':
				c = L'±';
				break;*/    // what the fuck is this?
			case L'3':
				c = L'#';
				break;
			case L'4':
				c = L'$';
				break;
			case L'5':
				c = L'%';
				break;
			case L'6':
				c = L'^';
				break;
			case L'7':
				c = L'&';
				break;
			case L'8':
				c = L'*';
				break;
			case L'9':
				c = L'(';
				break;
			case L'0':
				c = L')';
				break;
			case L'-':
				c = L'_';
				break;
			case L'=':
				c = L'+';
				break;
			case L'[':
				c = L'{';
				break;
			case L']':
				c = L'}';
				break;
			case L'\'':
				c = L'"';
				break;
			case L'\\':
				c = L'|';
				break;
			case L';':
				c = L':';
				break;
			case L',':
				c = L'<';
				break;
			case L'.':
				c = L'>';
				break;
			case L'/':
				c = L'?';
				break;
		}
	}

	return c;
}
wchar_t
InputHandler::DeviceButtonToChar(DeviceButton button,
								 bool bUseCurrentKeyModifiers)
{
	wchar_t c = L'\0';
	switch (button) {
		default:
			if (button < 127)
				c = (wchar_t)button;
			else if (button >= KEY_KP_C0 && button <= KEY_KP_C9)
				c = (wchar_t)(button - KEY_KP_C0) + '0';
			break;
		case KEY_KP_SLASH:
			c = L'/';
			break;
		case KEY_KP_ASTERISK:
			c = L'*';
			break;
		case KEY_KP_HYPHEN:
			c = L'-';
			break;
		case KEY_KP_PLUS:
			c = L'+';
			break;
		case KEY_KP_PERIOD:
			c = L'.';
			break;
		case KEY_KP_EQUAL:
			c = L'=';
			break;
	}

	// Handle some default US keyboard modifiers for derived InputHandlers that
	// don't implement DeviceButtonToChar.
	if (bUseCurrentKeyModifiers) {
		c = ApplyKeyModifiers(c);
	}

	return c;
}

static LocalizedString HOME("DeviceButton", "Home");
static LocalizedString END("DeviceButton", "End");
static LocalizedString UP("DeviceButton", "Up");
static LocalizedString DOWN("DeviceButton", "Down");
static LocalizedString SPACE("DeviceButton", "Space");
static LocalizedString SHIFT("DeviceButton", "Shift");
static LocalizedString CTRL("DeviceButton", "Ctrl");
static LocalizedString ALT("DeviceButton", "Alt");
static LocalizedString INSERT("DeviceButton", "Insert");
static LocalizedString DEL("DeviceButton", "Delete");
static LocalizedString PGUP("DeviceButton", "PgUp");
static LocalizedString PGDN("DeviceButton", "PgDn");
static LocalizedString BACKSLASH("DeviceButton", "Backslash");

std::string
InputHandler::GetDeviceSpecificInputString(const DeviceInput& di)
{
	if (di.device == InputDevice_Invalid)
		return std::string();

	if (di.device == DEVICE_KEYBOARD) {
		wchar_t c = DeviceButtonToChar(di.button, false);
		if (c && c != L' ') // Don't show "Key  " for space.
			return InputDeviceToString(di.device) + " " +
				   Capitalize(WStringToString(std::wstring() + c));
	}

	std::string s = DeviceButtonToString(di.button);
	if (di.device != DEVICE_KEYBOARD)
		s = InputDeviceToString(di.device) + " " + s;
	return s;
}

std::string
InputHandler::GetLocalizedInputString(const DeviceInput& di)
{
	switch (di.button) {
		case KEY_HOME:
			return HOME.GetValue();
		case KEY_END:
			return END.GetValue();
		case KEY_UP:
			return UP.GetValue();
		case KEY_DOWN:
			return DOWN.GetValue();
		case KEY_SPACE:
			return SPACE.GetValue();
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			return SHIFT.GetValue();
		case KEY_LCTRL:
		case KEY_RCTRL:
			return CTRL.GetValue();
		case KEY_LALT:
		case KEY_RALT:
			return ALT.GetValue();
		case KEY_INSERT:
			return INSERT.GetValue();
		case KEY_DEL:
			return DEL.GetValue();
		case KEY_PGUP:
			return PGUP.GetValue();
		case KEY_PGDN:
			return PGDN.GetValue();
		case KEY_BACKSLASH:
			return BACKSLASH.GetValue();
		default:
			wchar_t c = DeviceButtonToChar(di.button, false);
			if (c && c != L' ') // Don't show "Key  " for space.
				return Capitalize(WStringToString(std::wstring() + c));

			return DeviceButtonToString(di.button);
	}
}

DriverList InputHandler::m_pDriverList;

static LocalizedString INPUT_HANDLERS_EMPTY("Arch",
											"Input Handlers cannot be empty.");
void
InputHandler::Create(const std::string& drivers_, vector<InputHandler*>& Add)
{
	const std::string drivers = drivers_.empty()
								  ? std::string(DEFAULT_INPUT_DRIVER_LIST)
								  : drivers_.c_str();
	vector<std::string> DriversToTry;
	split(drivers, ",", DriversToTry, true);

	if (DriversToTry.empty())
		RageException::Throw("%s", INPUT_HANDLERS_EMPTY.GetValue().c_str());

	FOREACH_CONST(std::string, DriversToTry, s)
	{
		RageDriver* pDriver = InputHandler::m_pDriverList.Create(*s);
		if (pDriver == NULL) {
			Locator::getLogger()->trace("Unknown Input Handler name: {}", s->c_str());
			continue;
		}

		InputHandler* ret = dynamic_cast<InputHandler*>(pDriver);
		DEBUG_ASSERT(ret);
		Add.push_back(ret);
	}
}
