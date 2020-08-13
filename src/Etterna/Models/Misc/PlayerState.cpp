#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "PlayerState.h"
#include "RadarValues.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"

PlayerState::PlayerState()
{
	m_PlayerNumber = PLAYER_INVALID;
	m_mp = MultiPlayer_Invalid;
	Reset();
}

void
PlayerState::Reset()
{
	m_NotefieldZoom = 1.0f;
	m_PlayerOptions.Init();

	m_fLastDrawnBeat = -100;

	m_HealthState = HealthState_Alive;

	m_PlayerController = PC_HUMAN;

	m_iCpuSkill = 5;

	// basic defaults but not necessarily always correct
	m_fReadBPM = 60.f;
	m_NumCols = 4;
}

// pointless if attacks are gone?
void
PlayerState::Update(float fDelta)
{
	// TRICKY: GAMESTATE->Update is run before any of the Screen update's,
	// so we'll clear these flags here and let them get turned on later

	// Update after enabling attacks, so we approach the new state.
	m_PlayerOptions.Update(fDelta);
}

void
PlayerState::SetPlayerNumber(PlayerNumber pn)
{
	m_PlayerNumber = pn;
	FOREACH_ENUM(ModsLevel, ml) { m_PlayerOptions.Get(ml).m_pn = pn; }
}

void
PlayerState::ResetToDefaultPlayerOptions(ModsLevel l)
{
	PlayerOptions po;
	GAMESTATE->GetDefaultPlayerOptions(po);
	m_PlayerOptions.Assign(l, po);
}

const TimingData&
PlayerState::GetDisplayedTiming() const
{
	Steps* steps = GAMESTATE->m_pCurSteps;
	if (steps == nullptr)
		return GAMESTATE->m_pCurSong->m_SongTiming;
	return *steps->GetTimingData();
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the PlayerState. */
class LunaPlayerState : public Luna<PlayerState>
{
  public:
	static int ApplyPreferredOptionsToOtherLevels(T* p, lua_State* L)
	{
		p->m_PlayerOptions.Assign(ModsLevel_Preferred,
								  p->m_PlayerOptions.Get(ModsLevel_Preferred));
		return 0;
	}
	DEFINE_METHOD(GetPlayerNumber, m_PlayerNumber);
	static int GetSongPosition(T* p, lua_State* L)
	{
		GAMESTATE->m_Position.PushSelf(L);
		return 1;
	}
	DEFINE_METHOD(GetMultiPlayerNumber, m_mp);
	DEFINE_METHOD(GetPlayerController, m_PlayerController);
	static int SetPlayerOptions(T* p, lua_State* L)
	{
		const auto m = Enum::Check<ModsLevel>(L, 1);
		PlayerOptions po;
		po.FromString(SArg(2));
		p->m_PlayerOptions.Assign(m, po);
		return 0;
	}
	static int GetPlayerOptions(T* p, lua_State* L)
	{
		const auto m = Enum::Check<ModsLevel>(L, 1);
		p->m_PlayerOptions.Get(m).PushSelf(L);
		return 1;
	}
	static int GetPlayerOptionsArray(T* p, lua_State* L)
	{
		const auto m = Enum::Check<ModsLevel>(L, 1);
		vector<std::string> s;
		p->m_PlayerOptions.Get(m).GetMods(s);
		LuaHelpers::CreateTableFromArray<std::string>(s, L);
		return 1;
	}
	static int GetPlayerOptionsString(T* p, lua_State* L)
	{
		const auto m = Enum::Check<ModsLevel>(L, 1);
		const auto s = p->m_PlayerOptions.Get(m).GetString();
		LuaHelpers::Push(L, s);
		return 1;
	}
	static int GetCurrentPlayerOptions(T* p, lua_State* L)
	{
		p->m_PlayerOptions.GetCurrent().PushSelf(L);
		return 1;
	}
	DEFINE_METHOD(GetHealthState, m_HealthState);
	static int GetSuperMeterLevel(T* p, lua_State* L)
	{
		lua_pushnumber(L, 0.f);
		return 1;
	}
	static int SetTargetGoal(T* p, lua_State* L)
	{
		p->playertargetgoal = FArg(1);
		return 1;
	}

	LunaPlayerState()
	{
		ADD_METHOD(ApplyPreferredOptionsToOtherLevels);
		ADD_METHOD(GetPlayerNumber);
		ADD_METHOD(GetMultiPlayerNumber);
		ADD_METHOD(GetPlayerController);
		ADD_METHOD(SetPlayerOptions);
		ADD_METHOD(GetPlayerOptions);
		ADD_METHOD(GetPlayerOptionsArray);
		ADD_METHOD(GetPlayerOptionsString);
		ADD_METHOD(GetCurrentPlayerOptions);
		ADD_METHOD(GetSongPosition);
		ADD_METHOD(GetHealthState);
		ADD_METHOD(GetSuperMeterLevel);
		ADD_METHOD(SetTargetGoal);
	}
};

LUA_REGISTER_CLASS(PlayerState)
// lua end
