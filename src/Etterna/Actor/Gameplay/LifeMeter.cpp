#include "Etterna/Globals/global.h"
#include "LifeMeter.h"
#include "LifeMeterBar.h"

LifeMeter*
LifeMeter::MakeLifeMeter(LifeType t)
{
	switch (t) {
		case LifeType_Bar:
			return new LifeMeterBar;
		default:
			FAIL_M(ssprintf("Unrecognized LifeMeter type: %i", t));
	}
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the LifeMeter. */
class LunaLifeMeter : public Luna<LifeMeter>
{
  public:
	static int GetLife(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->GetLife());
		return 1;
	}
	static int IsInDanger(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->IsInDanger());
		return 1;
	}
	static int IsHot(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->IsHot());
		return 1;
	}
	static int IsFailing(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->IsFailing());
		return 1;
	}

	LunaLifeMeter()
	{
		ADD_METHOD(GetLife);
		ADD_METHOD(IsInDanger);
		ADD_METHOD(IsHot);
		ADD_METHOD(IsFailing);
	}
};

LUA_REGISTER_DERIVED_CLASS(LifeMeter, ActorFrame)
// lua end
