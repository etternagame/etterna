#ifndef GAMEDEF_H
#define GAMEDEF_H

#include "GameConstantsAndTypes.h"
#include "Etterna/Singletons/InputMapper.h"

struct lua_State;
class Style;

// PrimaryMenuButton and SecondaryMenuButton are used to support using
// DeviceInputs that only  navigate the menus.

// A button being a primary menu button means that this GameButton will generate
// a the corresponding MenuInput IF AND ONLY IF the GameButton corresponding to
// the pimary input is not mapped.

// Example 1: A user is using an arcade machine as their controller. Most
// machines have MenuLeft, MenuStart, and MenuRight buttons on the cabinet, so
// they should be used to navigate menus. The user will map these DeviceInputs
// to the GameButtons "MenuLeft (optional)", "MenuStart", and "MenuRight
// (optional)".

// Example 2:  A user is using PlayStation dance pads to play. These controllers
// don't have dedicated DeviceInputs for MenuLeft and MenuRight. The user maps
// Up, Down, Left, and Right as normal. Since the Left and Right GameButtons
// have the flag FLAG_SECONDARY_MENU_*, they will function as MenuLeft and
// MenuRight as long as "MenuLeft (optional)" and "MenuRight (optional)" are not
// mapped.

/** @brief Holds information about a particular style of a game (e.g. "single",
 * "double"). */
struct Game
{
	const char* m_szName;
	const Style* const* m_apStyles;

	/** @brief Do we count multiple notes in a row as separate notes, or as one
	 * note? */
	bool m_bCountNotesSeparately;
	bool m_bTickHolds;
	bool m_PlayersHaveSeparateStyles;

	InputScheme m_InputScheme;

	struct PerButtonInfo
	{
		GameButtonType m_gbt;
	};
	/**
	 * @brief Data for each Game-specific GameButton.
	 *
	 * This starts at GAME_BUTTON_NEXT. */
	PerButtonInfo m_PerButtonInfo[NUM_GameButton];
	[[nodiscard]] auto GetPerButtonInfo(GameButton gb) const
	  -> const PerButtonInfo*;

	[[nodiscard]] auto MapTapNoteScore(TapNoteScore tns) const -> TapNoteScore;
	TapNoteScore m_mapW1To;
	TapNoteScore m_mapW2To;
	TapNoteScore m_mapW3To;
	TapNoteScore m_mapW4To;
	TapNoteScore m_mapW5To;
	[[nodiscard]] auto GetMapJudgmentTo(TapNoteScore tns) const -> TapNoteScore;

	// Lua
	void PushSelf(lua_State* L);
};

#endif
