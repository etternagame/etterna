#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/FileTypes/IniFile.h"
#include "Etterna/Singletons/InputFilter.h"
#include "InputMapper.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/MessageManager.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Misc/RageInput.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "arch/Dialog/Dialog.h"

#include <map>
#include <algorithm>

#define AUTOMAPPINGS_DIR "/Data/AutoMappings/"

static Preference<std::string> g_sLastSeenInputDevices("LastSeenInputDevices",
													   "");
static Preference<bool> g_bAutoMapOnJoyChange("AutoMapOnJoyChange", true);

namespace {
// lookup for efficiency from a DeviceInput to a GameInput
// This is repopulated every time m_PItoDI changes by calling
// UpdateTempDItoPI().
std::map<DeviceInput, GameInput> g_tempDItoGI;

PlayerNumber g_JoinControllers;
} // namespace;

InputMapper* INPUTMAPPER =
  nullptr; // global and accessible from anywhere in our program

InputMapper::InputMapper()
{
	// Register with Lua.
	{
		auto L = LUA->Get();
		lua_pushstring(L, "INPUTMAPPER");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}

	g_JoinControllers = PLAYER_INVALID;
	m_pInputScheme = nullptr;
}

InputMapper::~InputMapper()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("INPUTMAPPER");

	SaveMappingsToDisk();
	g_tempDItoGI.clear();
}

void
InputMapper::ClearAllMappings()
{
	m_mappings.Clear();

	UpdateTempDItoGI();
}

static const AutoMappings g_DefaultKeyMappings = AutoMappings(
	"",
	"",
	"",
	AutoMappingEntry( 0, KEY_LEFT,	GAME_BUTTON_MENULEFT,	false ),
	AutoMappingEntry( 0, KEY_RIGHT,	GAME_BUTTON_MENURIGHT,	false ),
	AutoMappingEntry( 0, KEY_UP,	GAME_BUTTON_MENUUP,	false ),
	AutoMappingEntry( 0, KEY_DOWN,	GAME_BUTTON_MENUDOWN,	false ),
	AutoMappingEntry( 0, KEY_ENTER,	GAME_BUTTON_START,	false ),
	AutoMappingEntry( 0, KEY_SLASH,	GAME_BUTTON_SELECT,	false ),
	AutoMappingEntry( 0, KEY_ESC,	GAME_BUTTON_BACK,	false ),
	/*AutoMappingEntry( 0, KEY_KP_C4,	GAME_BUTTON_MENULEFT,	true ),
	AutoMappingEntry( 0, KEY_KP_C6,	GAME_BUTTON_MENURIGHT,	true ),
	AutoMappingEntry( 0, KEY_KP_C8,	GAME_BUTTON_MENUUP,	true ),
	AutoMappingEntry( 0, KEY_KP_C2,	GAME_BUTTON_MENUDOWN,	true ),
	AutoMappingEntry( 0, KEY_KP_ENTER,	GAME_BUTTON_START,	true ),
	AutoMappingEntry( 0, KEY_KP_C0,	GAME_BUTTON_SELECT,	true ),
	AutoMappingEntry( 0, KEY_HYPHEN,	GAME_BUTTON_BACK,	true ), */// laptop keyboards.
	AutoMappingEntry( 0, KEY_F1,	GAME_BUTTON_COIN,	false ),
	AutoMappingEntry( 0, KEY_SCRLLOCK,	GAME_BUTTON_OPERATOR,	false ),
	AutoMappingEntry( 0, KEY_ACCENT, GAME_BUTTON_RESTART, false)
);

void
InputMapper::AddDefaultMappingsForCurrentGameIfUnmapped()
{
	std::vector<AutoMappingEntry> aMaps;
	aMaps.reserve(32);

	FOREACH_CONST(AutoMappingEntry, g_DefaultKeyMappings.m_vMaps, iter)
	aMaps.push_back(*iter);
	FOREACH_CONST(
	  AutoMappingEntry, m_pInputScheme->m_pAutoMappings->m_vMaps, iter)
	aMaps.push_back(*iter);

	/* There may be duplicate GAME_BUTTON maps.  Process the list backwards,
	 * so game-specific mappings override g_DefaultKeyMappings. */
	std::reverse(aMaps.begin(), aMaps.end());

	FOREACH(AutoMappingEntry, aMaps, m)
	{
		DeviceButton key = m->m_deviceButton;
		DeviceInput DeviceI(DEVICE_KEYBOARD, key);
		GameInput GameI(m->m_bSecondController ? GameController_2
											   : GameController_1,
						m->m_gb);
		// dont remap a button that is already being used
		if (!IsMapped(DeviceI))
		{
			if (!GameI.IsValid())
				ClearFromInputMap(DeviceI);
			else {
				// dont remap a default column binding
				if (!m_mappings.m_GItoDI[GameI.controller][GameI.button][2].IsValid())
					SetInputMap(DeviceI, GameI, 2);
			}
		}
	}
}

static const AutoMappings g_AutoMappings[] = {
	AutoMappings("dance",
				 "GIC USB Joystick",
				 "Boom USB convertor (black/gray)",
				 AutoMappingEntry(0, JOY_BUTTON_16, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_BUTTON_14, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_BUTTON_13, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_BUTTON_15, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_12, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "4 axis 16 button joystick",
				 "EMS USB2",
				 AutoMappingEntry(0, JOY_BUTTON_16, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_BUTTON_14, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_BUTTON_13, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_BUTTON_15, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "GamePad Pro USB ", // yes, there is a space at the end
				 "GamePad Pro USB",
				 AutoMappingEntry(0, JOY_LEFT, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_RIGHT, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_UP, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_DOWN, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "SideWinder Game Pad USB version 1.0",
				 "SideWinder Game Pad USB",
				 AutoMappingEntry(0, JOY_LEFT, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_RIGHT, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_UP, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_DOWN, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_5, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "4 axis 12 button joystick with hat switch",
				 "Super Joy Box 5",
				 AutoMappingEntry(0, JOY_LEFT, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_RIGHT, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_UP, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_DOWN, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "MP-8866 Dual USB Joypad",
				 "Super Dual Box (from DDRGame.com, Feb 2008)",
				 // NEEDS_DANCE_PAD_MAPPING_CODE,
				 AutoMappingEntry(0, JOY_BUTTON_3, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_BUTTON_2, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_1, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_BUTTON_4, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "NTPAD",
				 "NTPAD",
				 AutoMappingEntry(0, JOY_BUTTON_13, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_BUTTON_15, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_BUTTON_16, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_BUTTON_14, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "Psx Gamepad",
				 "PSXPAD",
				 AutoMappingEntry(0, JOY_LEFT, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_RIGHT, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_UP, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_DOWN, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_START, false)),
	AutoMappings(
	  "dance",
	  "XBOX Gamepad Plugin V0.01",
	  "X-Box gamepad",
	  AutoMappingEntry(0, JOY_LEFT, DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_RIGHT, DANCE_BUTTON_RIGHT, false),
	  AutoMappingEntry(0, JOY_UP, DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_DOWN, DANCE_BUTTON_DOWN, false),
	  AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_DOWN, false),	// A
	  AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_RIGHT, false), // B
	  AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_LEFT, false),	// X
	  AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_UP, false),	// Y
	  AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_START, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_BACK, false)),
	AutoMappings("dance",
				 "GamePad Pro USB ", // yes, there is a space at the end
				 "GamePad Pro USB",
				 AutoMappingEntry(0, JOY_LEFT, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_RIGHT, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_UP, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_DOWN, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "SideWinder Game Pad USB version 1.0",
				 "SideWinder Game Pad USB",
				 AutoMappingEntry(0, JOY_LEFT, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_RIGHT, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_UP, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_DOWN, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_5, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "4 axis 12 button joystick with hat switch",
				 "Super Joy Box 5",
				 AutoMappingEntry(0, JOY_LEFT, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_RIGHT, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_UP, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_DOWN, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "MP-8866 Dual USB Joypad",
				 "Super Dual Box (from DDRGame.com, Feb 2008)",
				 // NEEDS_DANCE_PAD_MAPPING_CODE,
				 AutoMappingEntry(0, JOY_BUTTON_3, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_BUTTON_2, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_1, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_BUTTON_4, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "NTPAD",
				 "NTPAD",
				 AutoMappingEntry(0, JOY_BUTTON_13, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_BUTTON_15, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_BUTTON_16, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_BUTTON_14, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings("dance",
				 "Psx Gamepad",
				 "PSXPAD",
				 AutoMappingEntry(0, JOY_LEFT, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(0, JOY_RIGHT, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(0, JOY_UP, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(0, JOY_DOWN, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_LEFT, false),
				 AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_RIGHT, false),
				 AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_UP, false),
				 AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_DOWN, false),
				 AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_BACK, false),
				 AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_START, false)),
	AutoMappings(
	  "dance",
	  "XBOX Gamepad Plugin V0.01",
	  "X-Box gamepad",
	  AutoMappingEntry(0, JOY_LEFT, DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_RIGHT, DANCE_BUTTON_RIGHT, false),
	  AutoMappingEntry(0, JOY_UP, DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_DOWN, DANCE_BUTTON_DOWN, false),
	  AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_DOWN, false),	// A
	  AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_RIGHT, false), // B
	  AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_LEFT, false),	// X
	  AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_UP, false),	// Y
	  AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_START, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_BACK, false)),
	AutoMappings(
	  "dance",
	  "0b43:0003", // The EMS USB2 doesn't provide a model string, so Linux
				   // just gives us the VendorID and ModelID in hex.
	  "EMS USB2",
	  // Player 1.
	  AutoMappingEntry(0, JOY_BUTTON_16, DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_14, DANCE_BUTTON_RIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_13, DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_BUTTON_15, DANCE_BUTTON_DOWN, false),
	  AutoMappingEntry(1, JOY_BUTTON_4, DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(1, JOY_BUTTON_2, DANCE_BUTTON_RIGHT, false),
	  AutoMappingEntry(1, JOY_BUTTON_1, DANCE_BUTTON_UP, false),
	  AutoMappingEntry(1, JOY_BUTTON_3, DANCE_BUTTON_DOWN, false),
	  AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_SELECT, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false),
	  AutoMappingEntry(0, JOY_BUTTON_5, GAME_BUTTON_BACK, false),
	  AutoMappingEntry(0, JOY_BUTTON_6, GAME_BUTTON_COIN, false)),
	AutoMappings(
	  "dance",
	  "Dance ",					   // Notice extra space at end
	  "LevelSix USB Pad (DDR638)", // "DDR638" is the model number of the pad
	  AutoMappingEntry(0, JOY_BUTTON_1, DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_BUTTON_2, DANCE_BUTTON_DOWN, false),
	  AutoMappingEntry(0, JOY_BUTTON_3, DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_4, DANCE_BUTTON_RIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings(
	  "dance",
	  "SmartJoy PLUS Adapter",
	  "SmartJoy PLUS Adapter",
	  AutoMappingEntry(0, JOY_LEFT, /* dpad L */ DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_RIGHT, /* dpad R */ DANCE_BUTTON_RIGHT, false),
	  AutoMappingEntry(0, JOY_UP, /* dpad U */ DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_DOWN, /* dpad D */ DANCE_BUTTON_DOWN, false),
	  AutoMappingEntry(1, JOY_BUTTON_4, /* Square */ DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(1, JOY_BUTTON_2, /* Circle */ DANCE_BUTTON_RIGHT, false),
	  AutoMappingEntry(1, JOY_BUTTON_1, /* Tri */ DANCE_BUTTON_UP, false),
	  AutoMappingEntry(1, JOY_BUTTON_3, /* X */ DANCE_BUTTON_DOWN, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, /* Select */ GAME_BUTTON_BACK, false),
	  AutoMappingEntry(0,
					   JOY_BUTTON_9,
					   /* Start	*/ GAME_BUTTON_START,
					   false),
	  AutoMappingEntry(0, JOY_BUTTON_5, /* R1 */ GAME_BUTTON_SELECT, false),
	  AutoMappingEntry(0, JOY_BUTTON_6, /* R2 */ GAME_BUTTON_COIN, false)),
	AutoMappings(
	  "dance",
	  "RedOctane USB Pad|XBOX DDR", // "RedOctane USB Pad" is Ignition 3s and
									// newer Afterburners.  "XBOX DDR" is older
									// Afterburners.
	  "RedOctane Ignition 3 or Afterburner",
	  AutoMappingEntry(0, JOY_BUTTON_1, /* dpad L */ DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_4, /* dpad R */ DANCE_BUTTON_RIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_3, /* dpad U */ DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_BUTTON_2, /* dpad D */ DANCE_BUTTON_DOWN, false),
	  // AutoMappingEntry{ 0, JOY_BUTTON_5,    /* Tri */       GAME_BUTTON_BACK,
	  // false ), AutoMappingEntry{ 0, JOY_BUTTON_6,    /* Square */
	  // GAME_BUTTON_BACK,       false ), AutoMappingEntry{ 1, JOY_BUTTON_7, /*
	  // X
	  // */	       GAME_BUTTON_START,      false ),
	  AutoMappingEntry(0, JOY_BUTTON_8, /* O */ GAME_BUTTON_START, false),
	  AutoMappingEntry(1,
					   JOY_BUTTON_9,
					   /* Start	*/ GAME_BUTTON_START,
					   false),
	  AutoMappingEntry(0, JOY_BUTTON_10, /* Sel */ GAME_BUTTON_BACK, false)),
	AutoMappings(
	  "dance",
	  "Joypad to USB converter",
	  "EMS Trio Linker",
	  AutoMappingEntry(0, JOY_BUTTON_16, /* dpad L */ DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0,
					   JOY_BUTTON_14,
					   /* dpad R */ DANCE_BUTTON_RIGHT,
					   false),
	  AutoMappingEntry(0, JOY_BUTTON_13, /* dpad U */ DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_BUTTON_15, /* dpad D */ DANCE_BUTTON_DOWN, false),
	  // AutoMappingEntry{ 0, JOY_BUTTON_5,    /* Tri */       GAME_BUTTON_BACK,
	  // false ), AutoMappingEntry{ 0, JOY_BUTTON_6,    /* Square */
	  // GAME_BUTTON_BACK,       false ), AutoMappingEntry{ 1, JOY_BUTTON_7, /*
	  // X
	  // */	       GAME_BUTTON_START,      false ),
	  AutoMappingEntry(0, JOY_BUTTON_2, /* O */ GAME_BUTTON_START, false),
	  AutoMappingEntry(1,
					   JOY_BUTTON_10,
					   /* Start	*/ GAME_BUTTON_START,
					   false),
	  AutoMappingEntry(0, JOY_BUTTON_9, /* Sel */ GAME_BUTTON_BACK, false)),
	AutoMappings(
	  "dance",
	  "Positive Gaming Impact USB pad",
	  "Positive Gaming Impact USB pad",
	  AutoMappingEntry(0, JOY_BUTTON_1, /* dpad L */ DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_4, /* dpad R */ DANCE_BUTTON_RIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_3, /* dpad U */ DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_BUTTON_2, /* dpad D */ DANCE_BUTTON_DOWN, false),
	  // AutoMappingEntry{ 0, JOY_BUTTON_5,    /* Tri */       GAME_BUTTON_BACK,
	  // false ), AutoMappingEntry{ 0, JOY_BUTTON_6,    /* Square */
	  // GAME_BUTTON_BACK,       false ),
	  AutoMappingEntry(1,
					   JOY_BUTTON_9,
					   /* Start	*/ GAME_BUTTON_START,
					   false),
	  AutoMappingEntry(0, JOY_BUTTON_10, /* Sel */ GAME_BUTTON_BACK, false)),
	AutoMappings(
	  "dance",
	  "USB Dance Pad",
	  "DDRGame Energy Dance Pad",
	  AutoMappingEntry(0, JOY_BUTTON_13, DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_BUTTON_15, DANCE_BUTTON_DOWN, false),
	  AutoMappingEntry(0, JOY_BUTTON_16, DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_14, DANCE_BUTTON_RIGHT, false),
	  // AutoMappingEntry{ 0, JOY_BUTTON_1,    DANCE_BUTTON_DOWNLEFT,  false ),
	  // AutoMappingEntry{ 0, JOY_BUTTON_4,    DANCE_BUTTON_DOWNRIGHT, false ),
	  AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings(
	  "dance",
	  "USB DancePad",
	  "D-Force Dance Pad",
	  AutoMappingEntry(0, JOY_BUTTON_1, DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_BUTTON_2, DANCE_BUTTON_DOWN, false),
	  AutoMappingEntry(0, JOY_BUTTON_3, DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_4, DANCE_BUTTON_RIGHT, false),
	  // AutoMappingEntry{ 0, JOY_BUTTON_1,    DANCE_BUTTON_DOWNLEFT,  false ),
	  // AutoMappingEntry{ 0, JOY_BUTTON_4,    DANCE_BUTTON_DOWNRIGHT, false ),
	  AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings(
	  "dance",
	  "Dual USB Vibration Joystick",
	  "PC Multi Hub Double Power Box 4",
	  AutoMappingEntry(0, JOY_BUTTON_13, DANCE_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_BUTTON_15, DANCE_BUTTON_DOWN, false),
	  AutoMappingEntry(0, JOY_BUTTON_16, DANCE_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_14, DANCE_BUTTON_RIGHT, false),
	  // AutoMappingEntry{ 0, JOY_BUTTON_1,    DANCE_BUTTON_DOWNLEFT,  false ),
	  // AutoMappingEntry{ 0, JOY_BUTTON_4,    DANCE_BUTTON_DOWNRIGHT, false ),
	  AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings(
	  "dance",
	  "Controller \\(Harmonix Drum Kit for Xbox 360\\)",
	  "Rock Band drum controller (Xbox 360, Windows driver)",
	  AutoMappingEntry(0, JOY_BUTTON_3, DANCE_BUTTON_UP, false), // blue drum
	  AutoMappingEntry(0,
					   JOY_BUTTON_4,
					   DANCE_BUTTON_DOWN,
					   false), // yellow drum
	  AutoMappingEntry(0, JOY_BUTTON_2, DANCE_BUTTON_LEFT, false), // red drum
	  AutoMappingEntry(0,
					   JOY_BUTTON_1,
					   DANCE_BUTTON_RIGHT,
					   false), // green	drum
	  AutoMappingEntry(0,
					   JOY_HAT_LEFT,
					   GAME_BUTTON_MENULEFT,
					   false), // d-pad	left
	  AutoMappingEntry(0,
					   JOY_HAT_RIGHT,
					   GAME_BUTTON_MENURIGHT,
					   false),									  // d-pad	right
	  AutoMappingEntry(0, JOY_HAT_UP, GAME_BUTTON_MENUUP, false), // d-pad	up
	  AutoMappingEntry(0,
					   JOY_HAT_DOWN,
					   GAME_BUTTON_MENUDOWN,
					   false), // d-pad	down
	  AutoMappingEntry(0,
					   JOY_BUTTON_8,
					   GAME_BUTTON_START,
					   false),									 // start	button
	  AutoMappingEntry(0, JOY_BUTTON_7, GAME_BUTTON_BACK, false) // back button
	  ),
	AutoMappings(
	  "pump",
	  "Pump USB",
	  "Pump USB pad",
	  AutoMappingEntry(0, JOY_BUTTON_1, PUMP_BUTTON_UPLEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_2, PUMP_BUTTON_UPRIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_3, PUMP_BUTTON_CENTER, false),
	  AutoMappingEntry(0, JOY_BUTTON_4, PUMP_BUTTON_DOWNLEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_5, PUMP_BUTTON_DOWNRIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_6, GAME_BUTTON_BACK, false)
	  /*AutoMappingEntry( 0, JOY_BUTTON_7,      PUMP_BUTTON_UPLEFT,     true ),
	  AutoMappingEntry( 0, JOY_BUTTON_8,      PUMP_BUTTON_UPRIGHT,    true ),
	  AutoMappingEntry( 0, JOY_BUTTON_9,      PUMP_BUTTON_CENTER,     true ),
	  AutoMappingEntry( 0, JOY_BUTTON_10,     PUMP_BUTTON_DOWNLEFT,   true ),
	  AutoMappingEntry( 0, JOY_BUTTON_11,     PUMP_BUTTON_DOWNRIGHT,  true )*/
	  ),
	AutoMappings(
	  "pump",
	  "GamePad Pro USB ", // yes, there is a space at the end
	  "GamePad Pro USB",
	  AutoMappingEntry(0, JOY_BUTTON_5, PUMP_BUTTON_UPLEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_6, PUMP_BUTTON_UPRIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_7, PUMP_BUTTON_DOWNLEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_8, PUMP_BUTTON_DOWNRIGHT, false),
	  AutoMappingEntry(0, JOY_LEFT, GAME_BUTTON_MENULEFT, false),
	  AutoMappingEntry(0, JOY_RIGHT, GAME_BUTTON_MENURIGHT, false),
	  AutoMappingEntry(0, JOY_UP, GAME_BUTTON_MENUUP, false),
	  AutoMappingEntry(0, JOY_DOWN, GAME_BUTTON_MENUDOWN, false),
	  AutoMappingEntry(1, JOY_BUTTON_1, PUMP_BUTTON_CENTER, false),
	  AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
	AutoMappings(
	  "pump",
	  "Controller \\(Harmonix Drum Kit for Xbox 360\\)",
	  "Rock Band drum controller (Xbox 360, Windows driver)",
	  AutoMappingEntry(0,
					   JOY_BUTTON_5,
					   PUMP_BUTTON_CENTER,
					   false), // bass pedal
	  AutoMappingEntry(0,
					   JOY_BUTTON_3,
					   PUMP_BUTTON_UPRIGHT,
					   false), // blue drum
	  AutoMappingEntry(0,
					   JOY_BUTTON_4,
					   PUMP_BUTTON_UPLEFT,
					   false), // yellow drum
	  AutoMappingEntry(0,
					   JOY_BUTTON_2,
					   PUMP_BUTTON_DOWNLEFT,
					   false), // red drum
	  AutoMappingEntry(0,
					   JOY_BUTTON_1,
					   PUMP_BUTTON_DOWNRIGHT,
					   false), // green	drum
	  AutoMappingEntry(0,
					   JOY_HAT_LEFT,
					   GAME_BUTTON_MENULEFT,
					   false), // d-pad	left
	  AutoMappingEntry(0,
					   JOY_HAT_RIGHT,
					   GAME_BUTTON_MENURIGHT,
					   false),									  // d-pad	right
	  AutoMappingEntry(0, JOY_HAT_UP, GAME_BUTTON_MENUUP, false), // d-pad	up
	  AutoMappingEntry(0,
					   JOY_HAT_DOWN,
					   GAME_BUTTON_MENUDOWN,
					   false), // d-pad	down
	  AutoMappingEntry(0,
					   JOY_BUTTON_8,
					   GAME_BUTTON_START,
					   false),									 // start	button
	  AutoMappingEntry(0, JOY_BUTTON_7, GAME_BUTTON_BACK, false) // back button
	  ),
	AutoMappings(
	  "para",
	  "ParaParaParadise Controller",
	  "ParaParaParadise Controller",
	  AutoMappingEntry(0, JOY_BUTTON_5, PARA_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_4, PARA_BUTTON_UPLEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_3, PARA_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_BUTTON_2, PARA_BUTTON_UPRIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_1, PARA_BUTTON_RIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false),
	  AutoMappingEntry(0, JOY_BUTTON_11, GAME_BUTTON_BACK, false),
	  AutoMappingEntry(0, JOY_BUTTON_12, GAME_BUTTON_MENULEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_MENURIGHT, false)),
	AutoMappings(
	  "techno",
	  "Dance ",					   // Notice the extra space at end
	  "LevelSix USB Pad (DDR638)", // "DDR638" is the model number of the pad
	  AutoMappingEntry(0, JOY_BUTTON_1, TECHNO_BUTTON_UP, false),
	  AutoMappingEntry(0, JOY_BUTTON_2, TECHNO_BUTTON_DOWN, false),
	  AutoMappingEntry(0, JOY_BUTTON_3, TECHNO_BUTTON_LEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_4, TECHNO_BUTTON_RIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_5, TECHNO_BUTTON_DOWNRIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_6, TECHNO_BUTTON_DOWNLEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_7, TECHNO_BUTTON_UPRIGHT, false),
	  AutoMappingEntry(0, JOY_BUTTON_8, TECHNO_BUTTON_UPLEFT, false),
	  AutoMappingEntry(0, JOY_BUTTON_9, GAME_BUTTON_BACK, false),
	  AutoMappingEntry(0, JOY_BUTTON_10, GAME_BUTTON_START, false)),
};

void
InputMapper::Unmap(InputDevice id)
{
	m_mappings.Unmap(id);

	UpdateTempDItoGI();
}

void
InputMapper::ApplyMapping(const std::vector<AutoMappingEntry>& vMmaps,
						  GameController gc,
						  InputDevice id)
{
	std::map<GameInput, int> MappedButtons;
	FOREACH_CONST(AutoMappingEntry, vMmaps, iter)
	{
		GameController map_gc = gc;
		if (iter->m_bSecondController) {
			map_gc = static_cast<GameController>(map_gc + 1);

			/* If that pushed it over, then it's a second controller for a
			 * joystick that's already a second controller, so we'll just ignore
			 * it.  (This can happen if eg. two primary Pump pads are
			 * connected.) */
			if (map_gc >= NUM_GameController)
				continue;
		}

		DeviceInput di(id, iter->m_deviceButton);
		GameInput gi(map_gc, iter->m_gb);
		int iSlot = MappedButtons[gi];
		++MappedButtons[gi];
		SetInputMap(di, gi, iSlot); // maps[k].iSlotIndex );
	}
}

void
InputMapper::AutoMapJoysticksForCurrentGame()
{
	std::vector<InputDeviceInfo> vDevices;
	INPUTMAN->GetDevicesAndDescriptions(vDevices);

	// fill vector with all auto mappings
	std::vector<AutoMappings> vAutoMappings;
	{
		// file automaps - Add these first so that they can match before the
		// hard-coded mappings
		std::vector<std::string> vs;
		FILEMAN->GetDirListing(AUTOMAPPINGS_DIR "*.ini", vs, ONLY_FILE, true);
		FOREACH_CONST(std::string, vs, sFilePath)
		{
			InputMappings km;
			km.ReadMappings(m_pInputScheme, *sFilePath, true);

			AutoMappings mapping(
			  m_pInputScheme->m_szName, km.m_sDeviceRegex, km.m_sDescription);

			FOREACH_ENUM(GameController, c)
			{
				FOREACH_ENUM(GameButton, b)
				{
					for (int s = 0; s < NUM_GAME_TO_DEVICE_SLOTS; s++) {
						const DeviceInput& DeviceI = km.m_GItoDI[c][b][s];
						if (!DeviceI.IsValid())
							continue;

						AutoMappingEntry im(
						  s, DeviceI.button, b, c > GameController_1);
						mapping.m_vMaps.push_back(im);
					}
				}
			}

			vAutoMappings.push_back(mapping);
		}

		// hard-coded automaps
		for (unsigned j = 0; j < ARRAYLEN(g_AutoMappings); j++) {
			const AutoMappings& mapping = g_AutoMappings[j];
			if (EqualsNoCase(mapping.m_sGame, m_pInputScheme->m_szName))
				vAutoMappings.push_back(mapping);
		}
	}

	// apply auto mappings
	int iNumJoysticksMapped = 0;
	FOREACH_CONST(InputDeviceInfo, vDevices, device)
	{
		InputDevice id = device->id;
		const std::string& sDescription = device->sDesc;
		FOREACH_CONST(AutoMappings, vAutoMappings, mapping)
		{
			Regex regex(mapping->m_sDriverRegex);
			if (!regex.Compare(sDescription))
				continue; // driver names don't match

			// We have a mapping for this joystick
			GameController gc =
			  static_cast<GameController>(iNumJoysticksMapped);
			if (gc >= NUM_GameController)
				break; // stop mapping.  We already mapped one device for each
					   // game controller.

			Locator::getLogger()->info("Applying default joystick mapping #{} for device '{}' ({})",
			  iNumJoysticksMapped + 1,
			  mapping->m_sDriverRegex.c_str(),
			  mapping->m_sControllerName.c_str());

			Unmap(id);
			ApplyMapping(mapping->m_vMaps, gc, id);

			iNumJoysticksMapped++;
		}
	}
}

void
InputMapper::SetInputScheme(const InputScheme* pInputScheme)
{
	m_pInputScheme = pInputScheme;

	ReadMappingsFromDisk();
}

const InputScheme*
InputMapper::GetInputScheme() const
{
	return m_pInputScheme;
}

// this isn't used in any key names
const std::string DEVICE_INPUT_SEPARATOR = ":";

void
InputMapper::ReadMappingsFromDisk()
{
	m_mappings.ReadMappings(m_pInputScheme, SpecialFiles::KEYMAPS_PATH, false);
	UpdateTempDItoGI();

	AddDefaultMappingsForCurrentGameIfUnmapped();
}

void
InputMapper::SaveMappingsToDisk()
{
	m_mappings.WriteMappings(m_pInputScheme, SpecialFiles::KEYMAPS_PATH);
}

void
InputMapper::ResetMappingsToDefault()
{
	m_mappings.Clear();
	UpdateTempDItoGI();
	AddDefaultMappingsForCurrentGameIfUnmapped();
}

void
InputMapper::CheckButtonAndAddToReason(GameButton menu,
									   std::vector<std::string>& full_reason,
									   std::string const& sub_reason)
{
	std::vector<GameInput> inputs;
	bool exists = false;
	// Only player 1 is checked because the player 2 buttons are rarely
	// unmapped and do not exist on some keyboard models. -Kyz
	GetInputScheme()->MenuButtonToGameInputs(menu, PLAYER_1, inputs);
	if (!inputs.empty()) {
		std::vector<DeviceInput> device_inputs;
		FOREACH(GameInput, inputs, inp)
		{
			for (int slot = 0; slot < NUM_GAME_TO_DEVICE_SLOTS; ++slot) {
				device_inputs.push_back(
				  m_mappings.m_GItoDI[inp->controller][inp->button][slot]);
			}
		}
		FOREACH(DeviceInput, device_inputs, inp)
		{
			if (!inp->IsValid()) {
				continue;
			}
			int use_count = 0;
			FOREACH_ENUM(GameController, cont)
			{
				FOREACH_GameButtonInScheme(GetInputScheme(), gb)
				{
					for (int slot = 0; slot < NUM_GAME_TO_DEVICE_SLOTS;
						 ++slot) {
						use_count +=
						  ((*inp) == m_mappings.m_GItoDI[cont][gb][slot]);
					}
				}
			}
			// If the device input is used more than once, it's a case where a
			// default mapped key was remapped to some other game button. -Kyz
			if (use_count == 1) {
				exists = true;
			}
		}
	}
	if (!exists) {
		full_reason.push_back(sub_reason);
	}
}

void
InputMapper::SanityCheckMappings(std::vector<std::string>& reason)
{
	// This is just to check whether the current mapping has the minimum
	// necessary to navigate the menus so the user can reach the config screen.
	// For this purpose, only the following keys are needed:
	// MenuLeft, MenuRight, Start, Operator
	// The InputScheme handles OnlyDedicatedMenuButtons logic. -Kyz
	CheckButtonAndAddToReason(GAME_BUTTON_MENULEFT, reason, "MenuLeftMissing");
	CheckButtonAndAddToReason(
	  GAME_BUTTON_MENURIGHT, reason, "MenuRightMissing");
	CheckButtonAndAddToReason(GAME_BUTTON_START, reason, "StartMissing");
	CheckButtonAndAddToReason(GAME_BUTTON_OPERATOR, reason, "OperatorMissing");
}

static LocalizedString CONNECTED("InputMapper", "Connected");
static LocalizedString DISCONNECTED("InputMapper", "Disconnected");
static LocalizedString AUTOMAPPING_ALL_JOYSTICKS("InputMapper",
												 "Auto-mapping all joysticks.");
bool
InputMapper::CheckForChangedInputDevicesAndRemap(std::string& sMessageOut)
{
	// Only check for changes in joysticks since that's all we know how to
	// remap.

	// update last seen joysticks
	std::vector<InputDeviceInfo> vDevices;
	INPUTMAN->GetDevicesAndDescriptions(vDevices);

	// Strip non-joysticks.
	std::vector<std::string> vsLastSeenJoysticks;
	// Don't use "," since some vendors have a name like "company Ltd., etc".
	// For now, use a pipe character. -aj, fix from Mordae.
	split(g_sLastSeenInputDevices, "|", vsLastSeenJoysticks);

	std::vector<std::string> vsCurrent;
	std::vector<std::string> vsCurrentJoysticks;
	for (int i = vDevices.size() - 1; i >= 0; i--) {
		vsCurrent.push_back(vDevices[i].sDesc);
		if (IsJoystick(vDevices[i].id)) {
			vsCurrentJoysticks.push_back(vDevices[i].sDesc);
		} else {
			std::vector<std::string>::iterator iter =
			  find(vsLastSeenJoysticks.begin(),
				   vsLastSeenJoysticks.end(),
				   vDevices[i].sDesc);
			if (iter != vsLastSeenJoysticks.end())
				vsLastSeenJoysticks.erase(iter);
		}
	}

	bool bJoysticksChanged = vsCurrentJoysticks != vsLastSeenJoysticks;
	if (!bJoysticksChanged)
		return false;

	std::vector<std::string> vsConnects, vsDisconnects;
	GetConnectsDisconnects(
	  vsLastSeenJoysticks, vsCurrentJoysticks, vsDisconnects, vsConnects);

	sMessageOut = std::string();
	if (!vsConnects.empty())
		sMessageOut +=
		  CONNECTED.GetValue() + ": " + join("\n", vsConnects) + "\n";
	if (!vsDisconnects.empty())
		sMessageOut +=
		  DISCONNECTED.GetValue() + ": " + join("\n", vsDisconnects) + "\n";

	if (g_bAutoMapOnJoyChange) {
		sMessageOut += AUTOMAPPING_ALL_JOYSTICKS.GetValue();
		AutoMapJoysticksForCurrentGame();
		SaveMappingsToDisk();
		MESSAGEMAN->Broadcast(Message_AutoJoyMappingApplied);
	}

	Locator::getLogger()->info("{}", sMessageOut.c_str());

	// see above comment about not using ",". -aj
	g_sLastSeenInputDevices.Set(join("|", vsCurrent));
	PREFSMAN->SavePrefsToDisk();

	return true;
}

void
InputMapper::SetInputMap(const DeviceInput& DeviceI,
						 const GameInput& GameI,
						 int iSlotIndex)
{
	m_mappings.SetInputMap(DeviceI, GameI, iSlotIndex);

	UpdateTempDItoGI();
}

void
InputMapper::ClearFromInputMap(const DeviceInput& DeviceI)
{
	m_mappings.ClearFromInputMap(DeviceI);

	UpdateTempDItoGI();
}

bool
InputMapper::ClearFromInputMap(const GameInput& GameI, int iSlotIndex)
{
	if (!m_mappings.ClearFromInputMap(GameI, iSlotIndex))
		return false;

	UpdateTempDItoGI();
	return true;
}

bool
InputMapper::IsMapped(const DeviceInput& DeviceI) const
{
	return g_tempDItoGI.find(DeviceI) != g_tempDItoGI.end();
}

void
InputMapper::UpdateTempDItoGI()
{
	// repopulate g_tempDItoGI
	g_tempDItoGI.clear();
	FOREACH_ENUM(GameController, n)
	{
		FOREACH_ENUM(GameButton, b)
		{
			for (int s = 0; s < NUM_GAME_TO_DEVICE_SLOTS; s++) {
				const DeviceInput& DeviceI = m_mappings.m_GItoDI[n][b][s];
				if (!DeviceI.IsValid())
					continue;

				g_tempDItoGI[DeviceI] = GameInput(n, b);
			}
		}
	}
}

// return true if there is a mapping from device to pad
bool
InputMapper::DeviceToGame(const DeviceInput& DeviceI, GameInput& GameI) const
{
	GameI = g_tempDItoGI[DeviceI];
	return GameI.controller != GameController_Invalid;
}

// return true if there is a mapping from pad to device
bool
InputMapper::GameToDevice(const GameInput& GameI,
						  int iSlotNum,
						  DeviceInput& DeviceI) const
{
	DeviceI = m_mappings.m_GItoDI[GameI.controller][GameI.button][iSlotNum];
	return DeviceI.device != InputDevice_Invalid;
}

PlayerNumber
InputMapper::ControllerToPlayerNumber(GameController controller) const
{
	if (controller == GameController_Invalid)
		return PLAYER_INVALID;
	if (g_JoinControllers != PLAYER_INVALID)
		return g_JoinControllers;

	return static_cast<PlayerNumber>(controller);
}

GameButton
InputMapper::GameButtonToMenuButton(GameButton gb) const
{
	return m_pInputScheme->GameButtonToMenuButton(gb);
}

/* If set (not PLAYER_INVALID), inputs from both GameControllers will be mapped
 * to the specified player. If PLAYER_INVALID, GameControllers will be mapped
 * individually. */
void
InputMapper::SetJoinControllers(PlayerNumber pn)
{
	g_JoinControllers = pn;
}

void
InputMapper::MenuToGame(GameButton MenuI,
						PlayerNumber pn,
						std::vector<GameInput>& GameIout) const
{
	if (g_JoinControllers != PLAYER_INVALID)
		pn = PLAYER_INVALID;

	m_pInputScheme->MenuButtonToGameInputs(MenuI, pn, GameIout);
}

bool
InputMapper::IsBeingPressed(const GameInput& GameI,
							MultiPlayer mp,
							const DeviceInputList* pButtonState) const
{
	if (GameI.button == GameButton_Invalid) {
		return false;
	}
	for (int i = 0; i < NUM_GAME_TO_DEVICE_SLOTS; i++) {
		DeviceInput DeviceI;

		if (GameToDevice(GameI, i, DeviceI)) {
			// if (mp != MultiPlayer_Invalid)
			//	DeviceI.device = MultiPlayerToInputDevice(mp);
			if (INPUTFILTER->IsBeingPressed(DeviceI, pButtonState))
				return true;
		}
	}

	return false;
}

bool
InputMapper::IsBeingPressed(GameButton MenuI, PlayerNumber pn) const
{
	if (MenuI == GameButton_Invalid) {
		return false;
	}
	std::vector<GameInput> GameI;
	MenuToGame(MenuI, pn, GameI);
	for (size_t i = 0; i < GameI.size(); i++)
		if (IsBeingPressed(GameI[i]))
			return true;

	return false;
}

bool
InputMapper::IsBeingPressed(const std::vector<GameInput>& GameI,
							MultiPlayer mp,
							const DeviceInputList* pButtonState) const
{
	bool pressed = false;
	for (size_t i = 0; i < GameI.size(); ++i) {
		pressed |= IsBeingPressed(GameI[i], mp, pButtonState);
	}
	return pressed;
}

void
InputMapper::RepeatStopKey(const GameInput& GameI)
{
	if (GameI.button == GameButton_Invalid) {
		return;
	}
	for (int i = 0; i < NUM_GAME_TO_DEVICE_SLOTS; i++) {
		DeviceInput DeviceI;

		if (GameToDevice(GameI, i, DeviceI))
			INPUTFILTER->RepeatStopKey(DeviceI);
	}
}

void
InputMapper::RepeatStopKey(GameButton MenuI, PlayerNumber pn)
{
	if (MenuI == GameButton_Invalid) {
		return;
	}
	std::vector<GameInput> GameI;
	MenuToGame(MenuI, pn, GameI);
	for (size_t i = 0; i < GameI.size(); i++)
		RepeatStopKey(GameI[i]);
}

float
InputMapper::GetSecsHeld(const GameInput& GameI, MultiPlayer mp) const
{
	if (GameI.button == GameButton_Invalid) {
		return 0.f;
	}
	float fMaxSecsHeld = 0;

	for (int i = 0; i < NUM_GAME_TO_DEVICE_SLOTS; i++) {
		DeviceInput DeviceI;
		if (GameToDevice(GameI, i, DeviceI)) {
			if (mp != MultiPlayer_Invalid)
				DeviceI.device = MultiPlayerToInputDevice(mp);
			fMaxSecsHeld =
			  std::max(fMaxSecsHeld, INPUTFILTER->GetSecsHeld(DeviceI));
		}
	}

	return fMaxSecsHeld;
}

float
InputMapper::GetSecsHeld(GameButton MenuI, PlayerNumber pn) const
{
	if (MenuI == GameButton_Invalid) {
		return 0.f;
	}
	float fMaxSecsHeld = 0;

	std::vector<GameInput> GameI;
	MenuToGame(MenuI, pn, GameI);
	for (size_t i = 0; i < GameI.size(); i++)
		fMaxSecsHeld = std::max(fMaxSecsHeld, GetSecsHeld(GameI[i]));

	return fMaxSecsHeld;
}

void
InputMapper::ResetKeyRepeat(const GameInput& GameI)
{
	if (GameI.button == GameButton_Invalid) {
		return;
	}
	for (int i = 0; i < NUM_GAME_TO_DEVICE_SLOTS; i++) {
		DeviceInput DeviceI;
		if (GameToDevice(GameI, i, DeviceI))
			INPUTFILTER->ResetKeyRepeat(DeviceI);
	}
}

void
InputMapper::ResetKeyRepeat(GameButton MenuI, PlayerNumber pn)
{
	if (MenuI == GameButton_Invalid) {
		return;
	}
	std::vector<GameInput> GameI;
	MenuToGame(MenuI, pn, GameI);
	for (size_t i = 0; i < GameI.size(); i++)
		ResetKeyRepeat(GameI[i]);
}

float
InputMapper::GetLevel(const GameInput& GameI) const
{
	if (GameI.button == GameButton_Invalid) {
		return 0.f;
	}
	float fLevel = 0;
	for (int i = 0; i < NUM_GAME_TO_DEVICE_SLOTS; i++) {
		DeviceInput DeviceI;

		if (GameToDevice(GameI, i, DeviceI))
			fLevel = std::max(fLevel, INPUTFILTER->GetLevel(DeviceI));
	}
	return fLevel;
}

float
InputMapper::GetLevel(GameButton MenuI, PlayerNumber pn) const
{
	if (MenuI == GameButton_Invalid) {
		return 0.f;
	}
	std::vector<GameInput> GameI;
	MenuToGame(MenuI, pn, GameI);

	float fLevel = 0;
	for (size_t i = 0; i < GameI.size(); i++)
		fLevel = std::max(fLevel, GetLevel(GameI[i]));

	return fLevel;
}

InputDevice
InputMapper::MultiPlayerToInputDevice(MultiPlayer mp)
{
	if (mp == MultiPlayer_Invalid)
		return InputDevice_Invalid;
	return enum_add2(DEVICE_JOY1, mp);
}

MultiPlayer
InputMapper::InputDeviceToMultiPlayer(InputDevice id)
{
	if (id == InputDevice_Invalid)
		return MultiPlayer_Invalid;
	return enum_add2(MultiPlayer_P1, id - DEVICE_JOY1);
}

GameButton
InputScheme::ButtonNameToIndex(const std::string& sButtonName) const
{
	for (auto gb = static_cast<GameButton>(0); gb < m_iButtonsPerController;
		 gb = static_cast<GameButton>(gb + 1))
		if (strcasecmp(GetGameButtonName(gb), sButtonName.c_str()) == 0)
			return gb;

	return GameButton_Invalid;
}

void
InputScheme::MenuButtonToGameInputs(GameButton MenuI,
									PlayerNumber pn,
									std::vector<GameInput>& GameIout) const
{
	ASSERT(MenuI != GameButton_Invalid);

	std::vector<GameButton> aGameButtons;
	MenuButtonToGameButtons(MenuI, aGameButtons);
	FOREACH(GameButton, aGameButtons, gb)
	{
		if (pn == PLAYER_INVALID) {
			GameIout.push_back(GameInput(GameController_1, *gb));
			GameIout.push_back(GameInput(GameController_2, *gb));
		} else {
			GameIout.push_back(GameInput(static_cast<GameController>(pn), *gb));
		}
	}
}

void
InputScheme::MenuButtonToGameButtons(GameButton MenuI,
									 std::vector<GameButton>& aGameButtons) const
{
	ASSERT(MenuI != GameButton_Invalid);

	if (MenuI == GameButton_Invalid)
		return;

	FOREACH_ENUM(GameButton, gb)
	{
		if (PREFSMAN->m_bOnlyDedicatedMenuButtons && gb >= GAME_BUTTON_NEXT)
			break;

		const GameButtonInfo* pGameButtonInfo = GetGameButtonInfo(gb);
		if (pGameButtonInfo->m_SecondaryMenuButton != MenuI)
			continue;
		aGameButtons.push_back(gb);
	}
}

GameButton
InputScheme::GameButtonToMenuButton(GameButton gb) const
{
	if (gb == GameButton_Invalid)
		return GameButton_Invalid;
	if (gb >= GAME_BUTTON_NEXT && PREFSMAN->m_bOnlyDedicatedMenuButtons)
		return GameButton_Invalid;
	return GetGameButtonInfo(gb)->m_SecondaryMenuButton;
}

static const InputScheme::GameButtonInfo g_CommonGameButtonInfo[] = {
	{ "MenuLeft", GAME_BUTTON_MENULEFT },
	{ "MenuRight", GAME_BUTTON_MENURIGHT },
	{ "MenuUp", GAME_BUTTON_MENUUP },
	{ "MenuDown", GAME_BUTTON_MENUDOWN },
	{ "Start", GAME_BUTTON_START },
	{ "Select", GAME_BUTTON_SELECT },
	{ "Back", GAME_BUTTON_BACK },
	{ "Coin", GAME_BUTTON_COIN },
	{ "Operator", GAME_BUTTON_OPERATOR },
	{ "EffectUp", GAME_BUTTON_EFFECT_UP },
	{ "EffectDown", GAME_BUTTON_EFFECT_DOWN },
	{ "RestartGameplay", GAME_BUTTON_RESTART },
};

const InputScheme::GameButtonInfo*
InputScheme::GetGameButtonInfo(GameButton gb) const
{
	COMPILE_ASSERT(GAME_BUTTON_NEXT == ARRAYLEN(g_CommonGameButtonInfo));
	if (gb < GAME_BUTTON_NEXT)
		return &g_CommonGameButtonInfo[gb];

	return &m_GameButtonInfo[gb - GAME_BUTTON_NEXT];
}

const char*
InputScheme::GetGameButtonName(GameButton gb) const
{
	if (gb == GameButton_Invalid)
		return "";
	auto e = GetGameButtonInfo(gb)->m_szName;

	// this happens if your keymapping ini has an unknown mapping
	if (e == nullptr)
		return "";

	return e;
}

void
InputMappings::Clear()
{
	FOREACH_ENUM(GameController, i)
	FOREACH_ENUM(GameButton, j)
	for (int k = 0; k < NUM_GAME_TO_DEVICE_SLOTS; k++)
		m_GItoDI[i][j][k].MakeInvalid();
}

void
InputMappings::Unmap(InputDevice id)
{
	FOREACH_ENUM(GameController, i)
	{
		FOREACH_ENUM(GameButton, j)
		{
			for (int k = 0; k < NUM_GAME_TO_DEVICE_SLOTS; k++) {
				DeviceInput& di = m_GItoDI[i][j][k];
				if (di.device == id)
					di.MakeInvalid();
			}
		}
	}
}

void
InputMappings::ReadMappings(const InputScheme* pInputScheme,
							const std::string& sFilePath,
							bool bIsAutoMapping)
{
	Clear();

	IniFile ini;
	if (!ini.ReadFile(sFilePath))
		Locator::getLogger()->warn("Couldn't open mapping file \"{}\": {}.",
				   SpecialFiles::KEYMAPS_PATH.c_str(),
				   ini.GetError().c_str());

	if (bIsAutoMapping) {
		if (!ini.GetValue("AutoMapping", "DeviceRegex", m_sDeviceRegex))
			Dialog::OK("Missing AutoMapping::DeviceRegex in '%s'",
					   sFilePath.c_str());

		if (!ini.GetValue("AutoMapping", "Description", m_sDescription))
			Dialog::OK("Missing AutoMapping::Description in '%s'",
					   sFilePath.c_str());
	}

	const XNode* Key = ini.GetChild(pInputScheme->m_szName);

	if (Key != nullptr) {
		FOREACH_CONST_Attr(Key, i)
		{
			const std::string& name = i->first;
			std::string value;
			i->second->GetValue(value);

			GameInput GameI;
			GameI.FromString(pInputScheme, name);
			if (!GameI.IsValid())
				continue;

			std::vector<std::string> sDeviceInputStrings;
			split(value, DEVICE_INPUT_SEPARATOR, sDeviceInputStrings, false);

			for (unsigned j = 0;
				 j < sDeviceInputStrings.size() &&
				 j < static_cast<unsigned>(NUM_GAME_TO_DEVICE_SLOTS);
				 j++) {
				DeviceInput DeviceI;
				DeviceI.FromString(sDeviceInputStrings[j]);
				if (DeviceI.IsValid())
					SetInputMap(DeviceI, GameI, j);
			}
		}
	}
}

void
InputMappings::WriteMappings(const InputScheme* pInputScheme,
							 const std::string& sFilePath)
{
	IniFile ini;
	ini.ReadFile(sFilePath);

	// erase the key so that we overwrite everything for this game
	ini.DeleteKey(pInputScheme->m_szName);

	XNode* pKey = ini.GetChild(pInputScheme->m_szName);
	if (pKey != nullptr)
		ini.RemoveChild(pKey);
	pKey = ini.AppendChild(pInputScheme->m_szName);

	// iterate over our input map and write all mappings to the ini file
	FOREACH_ENUM(GameController, i)
	{
		FOREACH_GameButtonInScheme(pInputScheme, j)
		{
			GameInput GameI(i, j);
			std::string sNameString = GameI.ToString(pInputScheme);

			std::vector<std::string> asValues;
			asValues.reserve(NUM_GAME_TO_DEVICE_SLOTS);
			for (int slot = 0; slot < NUM_GAME_TO_DEVICE_SLOTS;
				 ++slot)
				asValues.push_back(m_GItoDI[i][j][slot].ToString());

			while (!asValues.empty() && asValues.back().empty())
				asValues.erase(asValues.begin() + asValues.size() - 1);

			std::string sValueString = join(DEVICE_INPUT_SEPARATOR, asValues);

			pKey->AppendAttr(sNameString, sValueString);
		}
	}

	ini.WriteFile(sFilePath);
}

void
InputMappings::SetInputMap(const DeviceInput& DeviceI,
						   const GameInput& GameI,
						   int iSlotIndex)
{
	// remove the old input
	ClearFromInputMap(DeviceI);
	ClearFromInputMap(GameI, iSlotIndex);

	ASSERT_M(
	  GameI.controller < NUM_GameController,
	  ssprintf("controller: %u >= %u", GameI.controller, NUM_GameController));
	ASSERT_M(GameI.button < NUM_GameButton,
			 ssprintf("button: %u >= %u", GameI.button, NUM_GameButton));
	ASSERT_M(iSlotIndex < NUM_GAME_TO_DEVICE_SLOTS,
			 ssprintf("slot: %u >= %u", iSlotIndex, NUM_GAME_TO_DEVICE_SLOTS));
	m_GItoDI[GameI.controller][GameI.button][iSlotIndex] = DeviceI;
}

void
InputMappings::ClearFromInputMap(const DeviceInput& DeviceI)
{
	// search for where this DeviceI maps to

	FOREACH_ENUM(GameController, p)
	{
		FOREACH_ENUM(GameButton, b)
		{
			for (int s = 0; s < NUM_GAME_TO_DEVICE_SLOTS; s++) {
				if (m_GItoDI[p][b][s] == DeviceI)
					m_GItoDI[p][b][s].MakeInvalid();
			}
		}
	}
}

bool
InputMappings::ClearFromInputMap(const GameInput& GameI, int iSlotIndex)
{
	if (!GameI.IsValid())
		return false;

	DeviceInput& di = m_GItoDI[GameI.controller][GameI.button][iSlotIndex];
	if (!di.IsValid())
		return false;
	di.MakeInvalid();

	return true;
}

#include "Etterna/Models/Lua/LuaBinding.h"

class LunaInputMapper : public Luna<InputMapper>
{
public:
	static int SetInputMap(T* p, lua_State* L)
	{
		// ex: "Key_z"
		std::string deviceInputString = SArg(1);
		// ex: "Left"
		std::string buttonBeingMapped = SArg(2);
		// 0-4. 2 is the "default" column and we have 2 extras somehow
		int inputSlot = IArg(3);
		// 0 is the left main controller, 1 is the other for doubles
		int playerSlot = IArg(4);

		GameController gc = static_cast<GameController>(playerSlot);
		GameButton gb =
		  StringToGameButton(p->GetInputScheme(), buttonBeingMapped);
		GameInput gameI(gc, gb);
		DeviceInput deviceI;
		deviceI.FromString(deviceInputString);

		p->SetInputMap(deviceI, gameI, inputSlot);
		return 0;
	}
	static int GetGameButtonsToMap(T* p, lua_State* L)
	{
		// includes only the GAMEPLAY buttons
		// no menu buttons
		// I sure hope nobody changes the GameManager defs and InputMapper enum defs...
		std::vector<std::string> keys;
		for (GameButton gb = GAME_BUTTON_CUSTOM_01; gb < INPUTMAPPER->GetInputScheme()->m_iButtonsPerController; enum_add<GameButton>(gb, +1)) {
			keys.push_back(GameButtonToString(INPUTMAPPER->GetInputScheme(), gb));
		}
		LuaHelpers::CreateTableFromArray<std::string>(keys, L);
		return 1;
	}
	static int GetMenuButtonsToMap(T* p, lua_State* L)
	{
		// includes only the MENU buttons
		// no gameplay buttons
		// I sure hope nobody changes the InputMapper enum defs...
		std::vector<std::string> keys;
		for (GameButton gb = GAME_BUTTON_START;
			 gb <= GAME_BUTTON_RESTART;
			 enum_add<GameButton>(gb, +1)) {
			keys.push_back(
			  GameButtonToString(INPUTMAPPER->GetInputScheme(), gb));
		}
		LuaHelpers::CreateTableFromArray<std::string>(keys, L);
		return 1;
	}
	static int GetButtonMapping(T* p, lua_State* L)
	{
		// returns either null or the name of the button mapped
		// for the left controller, column 2 (3rd col) is the default column
		//
		std::string possiblyMappedGameButton = SArg(1);
		int playerSlot = IArg(2);
		CLAMP(playerSlot, 0, 1);
		int bindingColumn = IArg(3);
		CLAMP(bindingColumn, 0, 4);

		GameController gc = static_cast<GameController>(playerSlot);
		GameButton gb =
		  StringToGameButton(p->GetInputScheme(), possiblyMappedGameButton);
		GameInput gameI(gc, gb);
		DeviceInput deviceI;

		if (p->GameToDevice(gameI, bindingColumn, deviceI))
			lua_pushstring(L, deviceI.ToString().c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetButtonMappingString(T* p, lua_State* L)
	{
		// similar to GetButtonMapping except gets the string a user can read
		std::string possiblyMappedGameButton = SArg(1);
		int playerSlot = IArg(2);
		CLAMP(playerSlot, 0, 1);
		int bindingColumn = IArg(3);
		CLAMP(bindingColumn, 0, 4);

		GameController gc = static_cast<GameController>(playerSlot);
		GameButton gb =
		  StringToGameButton(p->GetInputScheme(), possiblyMappedGameButton);
		GameInput gameI(gc, gb);
		DeviceInput deviceI;

		if (p->GameToDevice(gameI, bindingColumn, deviceI))
			lua_pushstring(
			  L, INPUTMAN->GetDeviceSpecificInputString(deviceI).c_str());
		else
			lua_pushnil(L);
		return 1;
	}
	static int SaveMappingsToDisk(T* p, lua_State* L)
	{
		p->SaveMappingsToDisk();
		return 0;
	}
	static int ReadMappingsFromDisk(T* p, lua_State* L)
	{
		p->ReadMappingsFromDisk();
		return 0;
	}


	LunaInputMapper()
	{
		ADD_METHOD(SetInputMap);
		ADD_METHOD(GetGameButtonsToMap);
		ADD_METHOD(GetMenuButtonsToMap);
		ADD_METHOD(GetButtonMapping);
		ADD_METHOD(GetButtonMappingString);
		ADD_METHOD(SaveMappingsToDisk);
		ADD_METHOD(ReadMappingsFromDisk);
	}
};

LUA_REGISTER_CLASS(InputMapper)
