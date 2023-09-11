#include "Etterna/Globals/global.h"
#include "FilterManager.h"

FilterManager* FILTERMAN = nullptr;

FilterManager::FilterManager()
{
	// filter stuff - mina
	FilterUpperBounds.fill(0);
	FilterLowerBounds.fill(0);

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
	// Unregister with Lua.
	LUA->UnsetGlobal("FILTERMAN");
}

float
FilterManager::GetFilter(Skillset ss, int bound)
{
	// Bound checking is done within the Lua binding
	if (bound == 0)
		return FilterLowerBounds.at(ss);

	return FilterUpperBounds.at(ss);
}

void
FilterManager::SetFilter(float v, Skillset ss, int bound)
{
	// Bound checking is done within the Lua binding
	if (bound == 0)
		FilterLowerBounds.at(ss) = v;
	else
		FilterUpperBounds.at(ss) = v;
}

// reset button for filters
void
FilterManager::ResetSSFilters()
{
	FilterLowerBounds.fill(0);
	FilterUpperBounds.fill(0);
}

void
FilterManager::ResetAllFilters()
{
	ResetSSFilters();
	ExclusiveFilter = FilterManagerDefault::ExclusiveFilter;
	HighestSkillsetsOnly = FilterManagerDefault::HighestSkillsetsOnly;
	HighestDifficultyOnly = FilterManagerDefault::HighestDifficultyOnly;
	MinFilterRate = FilterManagerDefault::MinFilterRate;
	MaxFilterRate = FilterManagerDefault::MaxFilterRate;
}

// tmp filter stuff - mina
bool
FilterManager::AnyActiveFilter()
{
	for (const auto& val : FilterLowerBounds) {
		if (val > 0)
			return true;
	}
	for (const auto& val : FilterUpperBounds) {
		if (val > 0)
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
		// float v, Skillset ss, int bound
		int ss = IArg(2) - 1;
		if (ss < 0 || ss >= FilterManager::NUM_FILTERS) {
			luaL_error(
			  L, "Invalid skillset value %d in call to SetSSFilter", ss);
			return 0;
		}
		p->SetFilter(FArg(1), static_cast<Skillset>(IArg(2) - 1), IArg(3));
		return 0;
	}
	static int GetSSFilter(T* p, lua_State* L)
	{
		// Skillset ss, int bound
		int ss = IArg(1) - 1;
		if (ss < 0 || ss >= FilterManager::NUM_FILTERS) {
			luaL_error(
			  L, "Invalid skillset value %d in call to GetSSFilter", ss);
			lua_pushnumber(L, 0);
			return 1;
		}
		float f = p->GetFilter(static_cast<Skillset>(IArg(1) - 1), IArg(2));
		lua_pushnumber(L, f);
		return 1;
	}
	static int ResetSSFilters(T* p, lua_State* L)
	{
		p->ResetSSFilters();
		return 0;
	}
	static int ResetAllFilters(T* p, lua_State* L)
	{
		p->ResetAllFilters();
		return 0;
	}
	static int SetMaxFilterRate(T* p, lua_State* L)
	{
		float mfr = FArg(1);
		CLAMP(mfr, p->MinFilterRate, MAX_MUSIC_RATE);
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
		CLAMP(mfr, MIN_MUSIC_RATE, p->MaxFilterRate);
		p->MinFilterRate = mfr;
		return 0;
	}
	static int GetMinFilterRate(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->MinFilterRate);
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
	static int ToggleHighestDifficultyOnly(T* p, lua_State* L)
	{
		p->HighestDifficultyOnly = !p->HighestDifficultyOnly;
		return 0;
	}
	static int GetHighestDifficultyOnly(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HighestDifficultyOnly);
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
		ADD_METHOD(ResetAllFilters);
		ADD_METHOD(AnyActiveFilter);
		ADD_METHOD(SetMaxFilterRate);
		ADD_METHOD(GetMaxFilterRate);
		ADD_METHOD(SetMinFilterRate);
		ADD_METHOD(GetMinFilterRate);
		ADD_METHOD(ToggleFilterMode);
		ADD_METHOD(GetFilterMode);
		ADD_METHOD(ToggleHighestSkillsetsOnly);
		ADD_METHOD(GetHighestSkillsetsOnly);
		ADD_METHOD(ToggleHighestDifficultyOnly);
		ADD_METHOD(GetHighestDifficultyOnly);
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
