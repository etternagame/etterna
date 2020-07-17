#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenProfileSave.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/StatsManager.h"

REGISTER_SCREEN_CLASS(ScreenProfileSave);

void
ScreenProfileSave::BeginScreen()
{
	ScreenWithMenuElements::BeginScreen();
}

bool
ScreenProfileSave::Input(const InputEventPlus& input)
{
	return false;
}

void
ScreenProfileSave::Continue()
{
	// clear cached leaderboard scores after gameplay
	DLMAN->chartLeaderboards.clear();

	SCREENMAN->ZeroNextUpdate();

	StartTransitioningScreen(SM_GoToNextScreen);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ScreenProfileSave. */
class LunaScreenProfileSave : public Luna<ScreenProfileSave>
{
  public:
	static int Continue(T* p, lua_State* L)
	{
		LUA->YieldLua();
		p->Continue();
		LUA->UnyieldLua();
		COMMON_RETURN_SELF;
	}
	static int HaveProfileToSave(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, GAMESTATE->HaveProfileToSave());
		return 1;
	}

	LunaScreenProfileSave()
	{
		ADD_METHOD(Continue);
		ADD_METHOD(HaveProfileToSave);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenProfileSave, ScreenWithMenuElements)
// lua end
