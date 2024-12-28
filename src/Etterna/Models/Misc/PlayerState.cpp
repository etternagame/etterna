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

void
PlayerState::ResetCacheInfo(/*const NoteData& notes*/)
{
	m_CacheDisplayedBeat.clear();

	const auto vScrolls =
	  GetDisplayedTiming().GetTimingSegments(SEGMENT_SCROLL);

	if (!vScrolls.empty()) {
		if (ToScroll(vScrolls.at(0))->GetBeat() > 0.F) {
			CacheDisplayedBeat c = { 0.F, 0.F, 1.F };
			m_CacheDisplayedBeat.push_back(c);
		}
	}

	auto displayedBeat = 0.0F;
	auto lastRealBeat = 0.0F;
	auto lastRatio = 1.0F;
	for (auto vScroll : vScrolls) {
		auto* const seg = ToScroll(vScroll);
		displayedBeat += (seg->GetBeat() - lastRealBeat) * lastRatio;
		lastRealBeat = seg->GetBeat();
		lastRatio = seg->GetRatio();
		CacheDisplayedBeat c = { seg->GetBeat(),
								 displayedBeat,
								 seg->GetRatio() };
		m_CacheDisplayedBeat.push_back(c);
	}

	// optimization: unused for now.
	/*
	m_CacheNoteStat.clear();
	auto it = notes.GetTapNoteRangeAllTracks(0, MAX_NOTE_ROW, true);
	auto count = 0;
	auto lastCount = 0;
	for (; !it.IsAtEnd(); ++it) {
		for (auto t = 0; t < notes.GetNumTracks(); t++) {
			if (notes.GetTapNote(t, it.Row()) != TAP_EMPTY) {
				count++;
			}
		}
		CacheNoteStat c = { NoteRowToBeat(it.Row()), lastCount, count };
		lastCount = count;
		m_CacheNoteStat.push_back(c);
	}
	*/
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
		std::vector<std::string> s;
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
	static int GetGoalTrackerUsesReplay(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->m_bGoalTrackerUsesReplay);
		return 1;
	}
	static int SetGoalTrackerUsesReplay(T* p, lua_State* L)
	{
		auto b = BArg(1);
		p->m_bGoalTrackerUsesReplay = b;
		COMMON_RETURN_SELF
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
		ADD_METHOD(GetGoalTrackerUsesReplay);
		ADD_METHOD(SetGoalTrackerUsesReplay);
	}
};

LUA_REGISTER_CLASS(PlayerState)
// lua end
