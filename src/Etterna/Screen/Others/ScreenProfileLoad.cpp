#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenProfileLoad.h"

REGISTER_SCREEN_CLASS(ScreenProfileLoad);

void
ScreenProfileLoad::Init()
{
	LOAD_EDITS.Load(m_sName, "LoadEdits");

	ScreenWithMenuElements::Init();
}

void
ScreenProfileLoad::BeginScreen()
{
	m_bHaveProfileToLoad = GAMESTATE->HaveProfileToLoad();
	ScreenWithMenuElements::BeginScreen();
}

bool
ScreenProfileLoad::Input(const InputEventPlus& input)
{
	return false;
}

void
ScreenProfileLoad::Continue()
{
	if (m_bHaveProfileToLoad) {
		GAMESTATE->LoadProfiles(LOAD_EDITS);
		SCREENMAN->ZeroNextUpdate();
	}

	StartTransitioningScreen(SM_GoToNextScreen);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ScreenProfileLoad. */
class LunaScreenProfileLoad : public Luna<ScreenProfileLoad>
{
  public:
	static int Continue(T* p, lua_State* L)
	{
		LUA->YieldLua();
		p->Continue();
		LUA->UnyieldLua();
		COMMON_RETURN_SELF;
	}
	static int HaveProfileToLoad(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->m_bHaveProfileToLoad);
		return 1;
	}

	LunaScreenProfileLoad()
	{
		ADD_METHOD(Continue);
		ADD_METHOD(HaveProfileToLoad);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenProfileLoad, ScreenWithMenuElements)
// lua end
