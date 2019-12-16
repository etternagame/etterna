#include "Etterna/Globals/global.h"
#include "Game.h"

TapNoteScore
Game::MapTapNoteScore(TapNoteScore tns) const
{
	switch (tns) {
		case TNS_W1:
			return m_mapW1To;
		case TNS_W2:
			return m_mapW2To;
		case TNS_W3:
			return m_mapW3To;
		case TNS_W4:
			return m_mapW4To;
		case TNS_W5:
			return m_mapW5To;
		default:
			return tns;
	}
}

static const Game::PerButtonInfo g_CommonButtonInfo[] = {
	{ GameButtonType_Menu }, // GAME_BUTTON_MENULEFT
	{ GameButtonType_Menu }, // GAME_BUTTON_MENURIGHT
	{ GameButtonType_Menu }, // GAME_BUTTON_MENUUP
	{ GameButtonType_Menu }, // GAME_BUTTON_MENUDOWN
	{ GameButtonType_Menu }, // GAME_BUTTON_START
	{ GameButtonType_Menu }, // GAME_BUTTON_SELECT
	{ GameButtonType_Menu }, // GAME_BUTTON_BACK
	{ GameButtonType_Menu }, // GAME_BUTTON_COIN
	{ GameButtonType_Menu }, // GAME_BUTTON_OPERATOR
	{ GameButtonType_Menu }, // GAME_BUTTON_EFFECT_UP
	{ GameButtonType_Menu }, // GAME_BUTTON_EFFECT_DOWN
	{ GameButtonType_Menu }, // GAME_BUTTON_RESTART
};

const Game::PerButtonInfo*
Game::GetPerButtonInfo(GameButton gb) const
{
	COMPILE_ASSERT(GAME_BUTTON_NEXT == ARRAYLEN(g_CommonButtonInfo));
	if (gb < GAME_BUTTON_NEXT)
		return &g_CommonButtonInfo[gb];

	return &m_PerButtonInfo[gb - GAME_BUTTON_NEXT];
}

TapNoteScore
Game::GetMapJudgmentTo(TapNoteScore tns) const
{
	switch (tns) {
		case TNS_W1:
			return m_mapW1To;
		case TNS_W2:
			return m_mapW2To;
		case TNS_W3:
			return m_mapW3To;
		case TNS_W4:
			return m_mapW4To;
		case TNS_W5:
			return m_mapW5To;
		default:
			return TapNoteScore_Invalid;
	}
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the Game. */
class LunaGame : public Luna<Game>
{
  public:
	static int GetName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_szName);
		return 1;
	}
	static int CountNotesSeparately(T* p, lua_State* L)
	{
		lua_pushstring(L, "deprecated use GAMESTATE function instead");
		return 1;
	}
	DEFINE_METHOD(GetMapJudgmentTo,
				  GetMapJudgmentTo(Enum::Check<TapNoteScore>(L, 1)))
	DEFINE_METHOD(GetSeparateStyles, m_PlayersHaveSeparateStyles);

	LunaGame()
	{
		ADD_METHOD(GetName);
		ADD_METHOD(CountNotesSeparately);
		ADD_METHOD(GetMapJudgmentTo);
		ADD_METHOD(GetSeparateStyles);
	}
};

LUA_REGISTER_CLASS(Game)
// lua end
