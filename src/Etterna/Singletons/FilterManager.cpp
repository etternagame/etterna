#include "Etterna/Globals/global.h"
#include "FilterManager.h"
#include "Etterna/Models/Misc/PlayerState.h"

FilterManager* FILTERMAN = NULL;

FilterManager::FilterManager()
{
	// filter stuff - mina
	ZERO(SSFilterLowerBounds);
	ZERO(SSFilterUpperBounds);
	m_pPlayerState = new PlayerState;
	m_pPlayerState->SetPlayerNumber(PLAYER_1);

	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "FILTERMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

FilterManager::~FilterManager()
{
	SAFE_DELETE(m_pPlayerState);

	// Unregister with Lua.
	LUA->UnsetGlobal("FILTERMAN");
}

float
FilterManager::GetSSFilter(Skillset ss, int bound)
{
	if (bound == 0)
		return SSFilterLowerBounds[ss];

	return SSFilterUpperBounds[ss];
}

void
FilterManager::SetSSFilter(float v, Skillset ss, int bound)
{
	if (bound == 0)
		SSFilterLowerBounds[ss] = v;
	else
		SSFilterUpperBounds[ss] = v;
}

// reset button for filters
void
FilterManager::ResetSSFilters()
{
	for (int ss = 0; ss < NUM_Skillset + 1; ss++) {
		SSFilterLowerBounds[ss] = 0;
		SSFilterUpperBounds[ss] = 0;
	}
}

// tmp filter stuff - mina
bool
FilterManager::AnyActiveFilter()
{
	for (int ss = 0; ss < NUM_Skillset + 1; ss++) {
		if (SSFilterLowerBounds[ss] > 0)
			return true;
		if (SSFilterUpperBounds[ss] > 0)
			return true;
	}
	return false;
}

// coords garb
// store x/y persistently by name

void
FilterManager::savepos(std::string name, int x, int y)
{
	watte[name].first = x;
	watte[name].second = y;
}

std::pair<int, int>
FilterManager::loadpos(std::string name)
{
	return watte[name];
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

class LunaFilterManager : public Luna<FilterManager>
{
  public:
	DEFINE_METHOD(AnyActiveFilter, AnyActiveFilter())
	static int SetSSFilter(T* p, lua_State* L)
	{
		p->SetSSFilter(FArg(1), static_cast<Skillset>(IArg(2) - 1), IArg(3));
		return 0;
	}
	static int GetSSFilter(T* p, lua_State* L)
	{
		float f = p->GetSSFilter(static_cast<Skillset>(IArg(1) - 1), IArg(2));
		lua_pushnumber(L, f);
		return 1;
	}
	static int ResetSSFilters(T* p, lua_State* L)
	{
		p->ResetSSFilters();
		return 0;
	}
	static int SetMaxFilterRate(T* p, lua_State* L)
	{
		float mfr = FArg(1);
		auto loot = p->m_pPlayerState;
		CLAMP(mfr, loot->wtFFF, 3.f);
		p->MaxFilterRate = mfr;
		return 0;
	}
	static int GetMaxFilterRate(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->MaxFilterRate);
		return 1;
	}
	static int SetMinFilterRate(T* p, lua_State* L)
	{
		float mfr = FArg(1);
		CLAMP(mfr, 0.7f, p->MaxFilterRate);
		auto loot = p->m_pPlayerState;
		loot->wtFFF = mfr;
		return 0;
	}
	static int GetMinFilterRate(T* p, lua_State* L)
	{
		auto loot = p->m_pPlayerState;
		lua_pushnumber(L, loot->wtFFF);
		return 1;
	}
	static int ToggleFilterMode(T* p, lua_State* L)
	{
		p->ExclusiveFilter = !p->ExclusiveFilter;
		return 0;
	}
	static int GetFilterMode(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->ExclusiveFilter);
		return 1;
	}
	static int ToggleHighestSkillsetsOnly(T* p, lua_State* L)
	{
		p->HighestSkillsetsOnly = !p->HighestSkillsetsOnly;
		return 0;
	}
	static int GetHighestSkillsetsOnly(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HighestSkillsetsOnly);
		return 1;
	}

	static int HelpImTrappedInAChineseFortuneCodingFactory(T* p, lua_State* L)
	{
		p->galaxycollapsed = BArg(1);
		return 0;
	}
	static int oopsimlazylol(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->galaxycollapsed);
		return 1;
	}
	static int grabposx(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->watte[SArg(1)].first);
		return 1;
	}
	static int grabposy(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->watte[SArg(1)].second);
		return 1;
	}
	static int savepos(T* p, lua_State* L)
	{
		p->watte[SArg(1)].first = IArg(2);
		p->watte[SArg(1)].second = IArg(3);
		return 0;
	}
	static int GetFilteringCommonPacks(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->filteringCommonPacks);
		return 1;
	}
	static int ToggleCommonPackFilter(T* p, lua_State* L)
	{
		p->filteringCommonPacks = !p->filteringCommonPacks;
		lua_pushboolean(L, p->filteringCommonPacks);
		return 1;
	}

	LunaFilterManager()
	{
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
		ADD_METHOD(HelpImTrappedInAChineseFortuneCodingFactory);
		ADD_METHOD(oopsimlazylol);
		ADD_METHOD(grabposx);
		ADD_METHOD(grabposy);
		ADD_METHOD(GetFilteringCommonPacks);
		ADD_METHOD(ToggleCommonPackFilter);
	}
};

LUA_REGISTER_CLASS(FilterManager)
// lua end
