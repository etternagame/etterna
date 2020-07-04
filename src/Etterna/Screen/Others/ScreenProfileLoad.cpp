#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Singletons/ProfileManager.h"
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
		// So basically if we only have 1 profile nothing works
		// the fix is to force the first profile to load
		// then act like nothing happened
		if (PROFILEMAN->GetNumLocalProfiles() == 1) {
			PROFILEMAN->m_sDefaultLocalProfileID[PLAYER_1].Set(
			  PROFILEMAN->GetLocalProfileIDFromIndex(0));
			PROFILEMAN->LoadLocalProfileFromMachine(PLAYER_1);
			GAMESTATE->LoadCurrentSettingsFromProfile(PLAYER_1);
		} else if (PROFILEMAN->GetNumLocalProfiles() == 0) {
			// but also sometimes we might have 0 profiles
			// in that case make a new one, duh
			std::string id;
			PROFILEMAN->CreateLocalProfile("Default Profile", id);
			PROFILEMAN->m_sDefaultLocalProfileID[PLAYER_1].Set(id);
			PROFILEMAN->LoadLocalProfileFromMachine(PLAYER_1);
			GAMESTATE->LoadCurrentSettingsFromProfile(PLAYER_1);
		}
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
