#include "Etterna/Globals/global.h"
#include "PlayerInfo.h"
#include "GameConstantsAndTypes.h"
#include "PlayerNumber.h"
#include "PlayerState.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/StatsManager.h"
#include "PlayerStageStats.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeper.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Actor/Gameplay/PlayerPractice.h"
#include "Etterna/Actor/Gameplay/PlayerReplay.h"
#include "Etterna/Actor/Gameplay/LifeMeter.h"
#include "Etterna/Actor/GameplayAndMenus/StepsDisplay.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/LuaManager.h"

static ThemeMetric<std::string> SCORE_KEEPER_CLASS("ScreenGameplay",
												   "ScoreKeeperClass");

PlayerInfo::PlayerInfo()
  : m_pn(PLAYER_INVALID)
  , m_pLifeMeter(nullptr)
  , m_ptextStepsDescription(nullptr)
  , m_pPrimaryScoreKeeper(nullptr)
  , m_ptextPlayerOptions(nullptr)
  , m_pPlayer(nullptr)
  , m_pStepsDisplay(nullptr)
{
}

void
PlayerInfo::Load(PlayerNumber pn,
				 MultiPlayer mp,
				 bool bShowNoteField,
				 int iAddToDifficulty,
				 GameplayMode mode)
{
	m_pn = pn;
	m_mp = mp;
	m_bPlayerEnabled = IsEnabled();
	m_bIsDummy = false;
	m_iAddToDifficulty = iAddToDifficulty;
	m_pLifeMeter = nullptr;
	m_ptextStepsDescription = nullptr;

	const auto pPlayerState = GetPlayerState();
	const auto pPlayerStageStats = GetPlayerStageStats();
	m_pPrimaryScoreKeeper = ScoreKeeper::MakeScoreKeeper(
	  SCORE_KEEPER_CLASS, pPlayerState, pPlayerStageStats);

	m_ptextPlayerOptions = nullptr;
	if (mode == GameplayMode_Replay) {
		m_pPlayer = new PlayerReplay(m_NoteData, bShowNoteField);
	} else if (mode == GameplayMode_Practice) {
		m_pPlayer = new PlayerPractice(m_NoteData, bShowNoteField);
	} else {
		m_pPlayer = new Player(m_NoteData, bShowNoteField);
	}

	m_pStepsDisplay = nullptr;

	if (IsMultiPlayer()) {
		pPlayerState->m_PlayerOptions =
		  GAMESTATE->m_pPlayerState->m_PlayerOptions;
	}
}

PlayerInfo::~PlayerInfo()
{
	SAFE_DELETE(m_pLifeMeter);
	SAFE_DELETE(m_ptextStepsDescription);
	SAFE_DELETE(m_pPrimaryScoreKeeper);
	SAFE_DELETE(m_ptextPlayerOptions);
	SAFE_DELETE(m_pPlayer);
	SAFE_DELETE(m_pStepsDisplay);
}

PlayerState*
PlayerInfo::GetPlayerState()
{
	return IsMultiPlayer()
			 ? GAMESTATE
				 ->m_pMultiPlayerState[GetPlayerStateAndStageStatsIndex()]
			 : GAMESTATE->m_pPlayerState;
}

PlayerStageStats*
PlayerInfo::GetPlayerStageStats()
{
	return &STATSMAN->m_CurStageStats.m_player;
}

bool
PlayerInfo::IsEnabled()
{
	if (m_pn != PLAYER_INVALID)
		return GAMESTATE->IsPlayerEnabled(m_pn);
	if (m_mp != MultiPlayer_Invalid)
		return GAMESTATE->IsMultiPlayerEnabled(m_mp);
	if (m_bIsDummy)
		return true;
	FAIL_M("Invalid non-dummy player.");
}

/** @brief Allow Lua to have access to the PlayerInfo. */
class LunaPlayerInfo : public Luna<PlayerInfo>
{
  public:
	static int GetLifeMeter(T* p, lua_State* L)
	{
		if (p->m_pLifeMeter) {
			p->m_pLifeMeter->PushSelf(L);
			return 1;
		}
		return 0;
	}

	static int GetStepsQueueWrapped(T* p, lua_State* L)
	{
		auto iIndex = IArg(1);
		iIndex %= p->m_vpStepsQueue.size();
		auto pSteps = p->m_vpStepsQueue[iIndex];
		pSteps->PushSelf(L);
		return 1;
	}

	LunaPlayerInfo()
	{
		ADD_METHOD(GetLifeMeter);
		ADD_METHOD(GetStepsQueueWrapped);
	}
};

LUA_REGISTER_CLASS(PlayerInfo)
