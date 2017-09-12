#include "global.h"
#include "FilterManager.h"
#include "PlayerState.h"

FilterManager* FILTERMAN = NULL;

FilterManager::FilterManager() {
	// filter stuff - mina
	ZERO(SSFilterLowerBounds);
	ZERO(SSFilterUpperBounds);

	FOREACH_PlayerNumber(p)
	{
		m_pPlayerState[p] = new PlayerState;
		m_pPlayerState[p]->SetPlayerNumber(p);
	}

	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring(L, "FILTERMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

FilterManager::~FilterManager() {
	FOREACH_PlayerNumber(p)
		SAFE_DELETE(m_pPlayerState[p]);

	// Unregister with Lua.
	LUA->UnsetGlobal("FILTERMAN");
}

float FilterManager::GetSSFilter(Skillset ss, int bound) {
	if (bound == 0)
		return SSFilterLowerBounds[ss];
	
		return SSFilterUpperBounds[ss];
}

void FilterManager::SetSSFilter(float v, Skillset ss, int bound) {
	if (bound == 0)
		SSFilterLowerBounds[ss] = v;
	else
		SSFilterUpperBounds[ss] = v;
}

// reset button for filters
void FilterManager::ResetSSFilters() {
	FOREACH_ENUM(Skillset, ss) {
		SSFilterLowerBounds[ss] = 0;
		SSFilterUpperBounds[ss] = 0;
	}
}

// tmp filter stuff - mina
bool FilterManager::AnyActiveFilter() {
	FOREACH_ENUM(Skillset, ss) {
		if (SSFilterLowerBounds[ss] > 0)
			return true;
		if (SSFilterUpperBounds[ss] > 0)
			return true;
	}
	return false;
}

// lua start
#include "LuaBinding.h"

class LunaFilterManager : public Luna<FilterManager>
{
public:
	DEFINE_METHOD(AnyActiveFilter, AnyActiveFilter())
	static int SetSSFilter(T* p, lua_State *L) {
		p->SetSSFilter(FArg(1), static_cast<Skillset>(IArg(2) - 1), IArg(3));
		return 1;
	}
	static int GetSSFilter(T* p, lua_State *L) {
		float f = p->GetSSFilter(static_cast<Skillset>(IArg(1) - 1), IArg(2));
		lua_pushnumber(L, f);
		return 1;
	}
	static int ResetSSFilters(T* p, lua_State *L) {
		p->ResetSSFilters();
		return 1;
	}
	static int SetMaxFilterRate(T* p, lua_State* L) {
		float mfr = FArg(1);
		auto loot = p->m_pPlayerState[0];
		CLAMP(mfr, loot->wtFFF, 3.f);
		p->MaxFilterRate = mfr;
		return 1;
	}
	static int GetMaxFilterRate(T* p, lua_State* L) {
		lua_pushnumber(L, p->MaxFilterRate);
		return 1;
	}
	static int SetMinFilterRate(T* p, lua_State* L) {
		float mfr = FArg(1);
		CLAMP(mfr, 0.7f, p->MaxFilterRate);
		auto loot = p->m_pPlayerState[0];
		loot->wtFFF = mfr;
		return 1;
	}
	static int GetMinFilterRate(T* p, lua_State* L) {
		auto loot = p->m_pPlayerState[0];
		lua_pushnumber(L, loot->wtFFF);
		return 1;
	}
	static int ToggleFilterMode(T* p, lua_State* L) {
		p->ExclusiveFilter = !p->ExclusiveFilter;
		return 1;

	}
	static int GetFilterMode(T* p, lua_State* L) {
		lua_pushboolean(L, p->ExclusiveFilter);
		return 1;
	}
	static int ToggleHighestSkillsetsOnly(T* p, lua_State* L) {
		p->HighestSkillsetsOnly = !p->HighestSkillsetsOnly;
		return 1;

	}
	static int GetHighestSkillsetsOnly(T* p, lua_State* L) {
		lua_pushboolean(L, p->HighestSkillsetsOnly);
		return 1;
	}
	LunaFilterManager() {
		ADD_METHOD(SetSSFilter);
		ADD_METHOD(GetSSFilter);
		ADD_METHOD(ResetSSFilters);
		ADD_METHOD(AnyActiveFilter);
		ADD_METHOD(SetMaxFilterRate);
		ADD_METHOD(GetMaxFilterRate);
		ADD_METHOD(SetMinFilterRate);
		ADD_METHOD(GetMinFilterRate);
		ADD_METHOD(ToggleFilterMode);
		ADD_METHOD(GetFilterMode);
		ADD_METHOD(ToggleHighestSkillsetsOnly);
		ADD_METHOD(GetHighestSkillsetsOnly);
	}
};

LUA_REGISTER_CLASS(FilterManager)
// lua end