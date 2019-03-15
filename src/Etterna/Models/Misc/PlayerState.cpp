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

const SongPosition&
PlayerState::GetDisplayedPosition() const
{
	if (GAMESTATE->m_bIsUsingStepTiming)
		return m_Position;
	return GAMESTATE->m_Position;
}

const TimingData&
PlayerState::GetDisplayedTiming() const
{
	Steps* steps = GAMESTATE->m_pCurSteps;
	if (steps == NULL)
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
		p->m_Position.PushSelf(L);
		return 1;
	}
	DEFINE_METHOD(GetMultiPlayerNumber, m_mp);
	DEFINE_METHOD(GetPlayerController, m_PlayerController);
	static int SetPlayerOptions(T* p, lua_State* L)
	{
		ModsLevel m = Enum::Check<ModsLevel>(L, 1);
		PlayerOptions po;
		po.FromString(SArg(2));
		p->m_PlayerOptions.Assign(m, po);
		return 0;
	}
	static int GetPlayerOptions(T* p, lua_State* L)
	{
		ModsLevel m = Enum::Check<ModsLevel>(L, 1);
		p->m_PlayerOptions.Get(m).PushSelf(L);
		return 1;
	}
	static int GetPlayerOptionsArray(T* p, lua_State* L)
	{
		ModsLevel m = Enum::Check<ModsLevel>(L, 1);
		vector<RString> s;
		p->m_PlayerOptions.Get(m).GetMods(s);
		LuaHelpers::CreateTableFromArray<RString>(s, L);
		return 1;
	}
	static int GetPlayerOptionsString(T* p, lua_State* L)
	{
		ModsLevel m = Enum::Check<ModsLevel>(L, 1);
		RString s = p->m_PlayerOptions.Get(m).GetString();
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

/*
 * (c) 2001-2004 Chris Danford, Chris Gomez
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
