#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/Actor.h"
#include "Etterna/Models/Misc/AdjustSync.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "CryptManager.h"
#include "discord_rpc.h"
#include "DownloadManager.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/Misc/GameCommand.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "GameManager.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "GameState.h"
#include "Etterna/Models/Lua/LuaReference.h"
#include "MessageManager.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "NetworkSyncManager.h"
#include "NoteSkinManager.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "ProfileManager.h"
#include "ScreenManager.h"
#include "Etterna/Screen/Others/Screen.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Songs/SongUtil.h"
#include "StatsManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "ThemeManager.h"
#include "SongManager.h"
#include "Etterna/Models/Misc/Profile.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Globals/rngthing.h"
#include "Core/Services/Locator.hpp"

#include <algorithm>

GameState* GAMESTATE =
  nullptr; // global and accessible from anywhere in our program

class GameStateMessageHandler : public MessageSubscriber
{
	void HandleMessage(const Message& msg) override
	{
		if (msg.GetName() == "RefreshCreditText") {
			std::string sJoined("P1");

			Locator::getLogger()->trace("Players joined: {}", sJoined.c_str());
		}
	}
};

struct GameStateImpl
{
	GameStateMessageHandler m_Subscriber;
	GameStateImpl() { m_Subscriber.SubscribeToMessage("RefreshCreditText"); }
};
static GameStateImpl* g_pImpl = nullptr;

ThemeMetric<bool> ALLOW_LATE_JOIN("GameState", "AllowLateJoin");

ThemeMetric<std::string> DEFAULT_SORT("GameState", "DefaultSort");
SortOrder
GetDefaultSort()
{
	return StringToSortOrder(DEFAULT_SORT);
}
ThemeMetric<std::string> DEFAULT_SONG("GameState", "DefaultSong");
Song*
GameState::GetDefaultSong() const
{
	SongID sid;
	sid.FromString(DEFAULT_SONG);
	return sid.ToSong();
}

static ThemeMetric<bool> ARE_STAGE_PLAYER_MODS_FORCED(
  "GameState",
  "AreStagePlayerModsForced");
static ThemeMetric<bool> ARE_STAGE_SONG_MODS_FORCED("GameState",
													"AreStageSongModsForced");

Preference<bool> GameState::m_bAutoJoin("AutoJoin", false);
Preference<bool> GameState::DisableChordCohesion("DisableChordCohesion", true);

GameState::GameState()
  : processedTiming(nullptr)
  , m_pCurGame(Message_CurrentGameChanged)
  , m_pCurStyle(Message_CurrentStyleChanged)
  , m_sPreferredSongGroup(Message_PreferredSongGroupChanged)
  , m_PreferredStepsType(Message_PreferredStepsTypeChanged)
  , m_PreferredDifficulty(Message_PreferredDifficultyP1Changed)
  , m_SortOrder(Message_SortOrderChanged)
  , m_pCurSong(Message_CurrentSongChanged)
  , m_pCurSteps(Message_CurrentStepsChanged)
  , m_bGameplayLeadIn(Message_GameplayLeadInChanged)
  , m_sEditLocalProfileID(Message_EditLocalProfileIDChanged)
  , m_gameplayMode(Message_GameplayModeChanged)
{
	g_pImpl = new GameStateImpl;

	m_pCurStyle.Set(nullptr);
	m_SeparatedStyles[PLAYER_1] = nullptr;

	m_pCurGame.Set(nullptr);
	m_timeGameStarted.SetZero();

	m_iStageSeed = m_iGameSeed = 0;

	m_gameplayMode.Set(GameplayMode_Normal);
	m_bSideIsJoined =
	  false; // used by GetNumSidesJoined before the first screen

	m_pPlayerState = new PlayerState;
	m_pPlayerState->SetPlayerNumber(PLAYER_1);
	FOREACH_MultiPlayer(p)
	{
		m_pMultiPlayerState[p] = new PlayerState;
		m_pMultiPlayerState[p]->SetPlayerNumber(PLAYER_1);
		m_pMultiPlayerState[p]->m_mp = p;
	}

	m_Environment = new LuaTable;

	sExpandedSectionName = "";

	this->SetMasterPlayerNumber(PLAYER_INVALID);
	FOREACH_MultiPlayer(p) m_MultiPlayerStatus[p] = MultiPlayerStatus_NotJoined;
	m_iNumMultiplayerNoteFields = 1;
	m_bFailTypeWasExplicitlySet = false;
	m_PreferredSortOrder = SORT_GROUP;
	m_iNumStagesOfThisSong = 0;
	m_iCurrentStageIndex = 0;
	m_iPlayerStageTokens = 0;
	m_bLoadingNextSong = false;
	m_pPreferredSong = nullptr;
	m_DanceDuration = 0.f;
	m_bTemporaryEventMode = false;
	m_bRestartedGameplay = false;
	m_LastPositionSeconds = 0.f;
	m_paused = false;

	// Just make sure practice is off for sure.
	TogglePracticeMode(false);

	// Don't reset yet; let the first screen do it, so we can use PREFSMAN and
	// THEME.
	// Reset();

	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "GAMESTATE");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

GameState::~GameState()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("GAMESTATE");

	SAFE_DELETE(m_pPlayerState);
	FOREACH_MultiPlayer(p) SAFE_DELETE(m_pMultiPlayerState[p]);

	SAFE_DELETE(m_Environment);
	SAFE_DELETE(g_pImpl);
	SAFE_DELETE(processedTiming);
}

PlayerNumber
GameState::GetMasterPlayerNumber() const
{
	return this->masterPlayerNumber;
}

void
GameState::SetMasterPlayerNumber(const PlayerNumber p)
{
	this->masterPlayerNumber = p;
}

TimingData*
GameState::GetProcessedTimingData() const
{
	return this->processedTiming;
}

void
GameState::SetProcessedTimingData(TimingData* t)
{
	this->processedTiming = t;
}

void
GameState::ApplyGameCommand(const std::string& sCommand, PlayerNumber pn)
{
	GameCommand m;
	m.Load(0, ParseCommands(sCommand));

	std::string sWhy;
	if (!m.IsPlayable(&sWhy)) {
		LuaHelpers::ReportScriptErrorFmt(
		  "Can't apply GameCommand \"%s\": %s", sCommand.c_str(), sWhy.c_str());
		return;
	}

	if (pn == PLAYER_INVALID)
		m.ApplyToAllPlayers();
	else
		m.Apply(pn);
}

void
GameState::ApplyCmdline()
{
	// We need to join players before we can set the style.
	std::string sPlayer;
	for (int i = 0; GetCommandlineArgument("player", &sPlayer, i); ++i) {
		int pn = StringToInt(sPlayer) - 1;
		if (!IsAnInt(sPlayer) || pn < 0 || pn >= NUM_PLAYERS)
			RageException::Throw("Invalid argument \"--player=%s\".",
								 sPlayer.c_str());

		JoinPlayer((PlayerNumber)pn);
	}

	std::string sMode;
	for (int i = 0; GetCommandlineArgument("mode", &sMode, i); ++i) {
		ApplyGameCommand(sMode);
	}
}

void
GameState::ResetPlayer(PlayerNumber pn)
{
	m_PreferredStepsType.Set(StepsType_Invalid);
	m_PreferredDifficulty.Set(Difficulty_Invalid);
	m_iPlayerStageTokens = 0;
	m_pCurSteps.Set(nullptr);
	m_pPlayerState->Reset();
	PROFILEMAN->UnloadProfile(pn);
	ResetPlayerOptions(pn);
}

void
GameState::ResetPlayerOptions(PlayerNumber pn)
{
	PlayerOptions po;
	GetDefaultPlayerOptions(po);
	m_pPlayerState->m_PlayerOptions.Assign(ModsLevel_Preferred, po);
}

void
GameState::Reset()
{
	this->SetMasterPlayerNumber(
	  PLAYER_INVALID); // must initialize for UnjoinPlayer

	UnjoinPlayer(PLAYER_1);

	ASSERT(THEME != NULL);

	m_timeGameStarted.SetZero();
	SetCurrentStyle(nullptr, PLAYER_INVALID);
	FOREACH_MultiPlayer(p) m_MultiPlayerStatus[p] = MultiPlayerStatus_NotJoined;

	// m_iCoins = 0;	// don't reset coin count!
	m_iNumMultiplayerNoteFields = 1;
	*m_Environment = LuaTable();
	m_sPreferredSongGroup.Set(GROUP_ALL);
	m_bFailTypeWasExplicitlySet = false;
	m_SortOrder.Set(SortOrder_Invalid);
	m_PreferredSortOrder = GetDefaultSort();
	m_iCurrentStageIndex = 0;

	m_bGameplayLeadIn.Set(false);
	m_iNumStagesOfThisSong = 0;
	m_bLoadingNextSong = false;

	NOTESKIN->RefreshNoteSkinData(m_pCurGame);

	m_iGameSeed = g_RandomNumberGenerator();
	SetNewStageSeed();

	m_pCurSong.Set(GetDefaultSong());
	m_pPreferredSong = nullptr;

	FOREACH_MultiPlayer(p) m_pMultiPlayerState[p]->Reset();

	m_SongOptions.Init();

	m_paused = false;
	ResetMusicStatistics();
	ResetStageStatistics();
	AdjustSync::ResetOriginalSyncData();

	STATSMAN->Reset();
	m_bTemporaryEventMode = false;
	sExpandedSectionName = "";

	ApplyCmdline();
}

void
GameState::JoinPlayer(PlayerNumber pn)
{
	pn = PLAYER_1;
	m_bSideIsJoined = true;

	this->SetMasterPlayerNumber(pn);
	if (GetNumSidesJoined() == 1)
		BeginGame();
	const Style* style =
	  GAMEMAN->GetFirstCompatibleStyle(m_pCurGame, 1, StepsType_dance_single);
	SetCurrentStyle(style, pn);

	Message msg(MessageIDToString(Message_PlayerJoined));
	msg.SetParam("Player", pn);
	MESSAGEMAN->Broadcast(msg);
}

void
GameState::UnjoinPlayer(PlayerNumber pn)
{
	/* Unjoin STATSMAN first, so steps used by this player are released
	 * and can be released by PROFILEMAN. */
	STATSMAN->UnjoinPlayer(pn);
	m_bSideIsJoined = false;
	m_iPlayerStageTokens = 0;

	ResetPlayer(pn);

	if (this->GetMasterPlayerNumber() == pn) {
		this->SetMasterPlayerNumber(PLAYER_INVALID);
	}

	Message msg(MessageIDToString(Message_PlayerUnjoined));
	msg.SetParam("Player", pn);
	MESSAGEMAN->Broadcast(msg);

	// If there are no players left, reset some non-player-specific stuff, too.
	if (this->GetMasterPlayerNumber() == PLAYER_INVALID) {
		SongOptions so;
		GetDefaultSongOptions(so);
		m_SongOptions.Assign(ModsLevel_Preferred, so);
		m_bDidModeChangeNoteSkin = false;
	}
}

/* xxx: handle multiplayer join? -aj */

namespace {
bool
JoinInputInternal(PlayerNumber pn)
{
	if (!GAMESTATE->PlayersCanJoin())
		return false;

	// If this side is already in, don't re-join.
	if (GAMESTATE->m_bSideIsJoined)
		return false;

	GAMESTATE->JoinPlayer(pn);
	return true;
}
};

// Handle an input that can join a player. Return true if the player joined.
bool
GameState::JoinInput(PlayerNumber pn)
{
	// When AutoJoin is enabled, join all players on a single start press.
	if (GAMESTATE->m_bAutoJoin.Get())
		return JoinPlayers();
	else
		return JoinInputInternal(pn);
}

// Attempt to join all players, as if each player pressed Start.
bool
GameState::JoinPlayers()
{
	bool bJoined = false;
	if (JoinInputInternal(PLAYER_1))
		bJoined = true;
	return bJoined;
}

/* Game flow:
 *
 * BeginGame() - the first player has joined; the game is starting.
 *
 * PlayersFinalized() - player memory cards are loaded; later joins won't have
 * memory cards this stage
 *
 * BeginStage() - gameplay is beginning
 *
 * optional: CancelStage() - gameplay aborted (Back pressed), undo BeginStage
 * and back up
 *
 * CommitStageStats() - gameplay is finished
 *   Saves STATSMAN->m_CurStageStats to the profiles, so profile information
 *   is up-to-date for Evaluation.
 *
 * FinishStage() - gameplay and evaluation is finished
 *   Clears data which was stored by CommitStageStats. */
void
GameState::BeginGame()
{
	m_timeGameStarted.Touch();
}

void
GameState::LoadProfiles(bool bLoadEdits)
{
	bool bSuccess = PROFILEMAN->LoadFirstAvailableProfile(
	  PLAYER_1, bLoadEdits); // load full profile

	if (!bSuccess)
		return;

	LoadCurrentSettingsFromProfile(PLAYER_1);

	Profile* pPlayerProfile = PROFILEMAN->GetProfile(PLAYER_1);
	if (pPlayerProfile)
		pPlayerProfile->m_iTotalSessions++;
}

void
GameState::SavePlayerProfile()
{
	// AutoplayCPU should not save scores
	if (m_pPlayerState->m_PlayerController != PC_HUMAN)
		return;

	PROFILEMAN->SaveProfile(PLAYER_1);
}

bool
GameState::HaveProfileToLoad()
{
	if (!PROFILEMAN->m_sDefaultLocalProfileID[PLAYER_1].Get().empty())
		return true;

	return false;
}

bool
GameState::HaveProfileToSave()
{
	return true;
}

int
GameState::GetNumStagesMultiplierForSong(const Song* pSong)
{
	int iNumStages = 1;

	ASSERT(pSong != NULL);
	if (pSong->IsMarathon())
		iNumStages *= 3;
	if (pSong->IsLong())
		iNumStages *= 2;

	return iNumStages;
}

int
GameState::GetNumStagesForCurrentSongAndStepsOrCourse() const
{
	int iNumStagesOfThisSong = 1;
	if (m_pCurSong) {
		iNumStagesOfThisSong =
		  GameState::GetNumStagesMultiplierForSong(m_pCurSong);
	} else
		return -1;
	iNumStagesOfThisSong = std::max(iNumStagesOfThisSong, 1);
	return iNumStagesOfThisSong;
}

// Called by ScreenGameplay. Set the length of the current song.
void
GameState::BeginStage()
{

	// This should only be called once per stage.
	if (m_iNumStagesOfThisSong != 0)
		Locator::getLogger()->warn("XXX: m_iNumStagesOfThisSong == {}?", m_iNumStagesOfThisSong);

	ResetStageStatistics();
	AdjustSync::ResetOriginalSyncData();

	if (!ARE_STAGE_PLAYER_MODS_FORCED) {
		ModsGroup<PlayerOptions>& po = m_pPlayerState->m_PlayerOptions;
		po.Assign(ModsLevel_Stage,
				  m_pPlayerState->m_PlayerOptions.GetPreferred());
	}
	if (!ARE_STAGE_SONG_MODS_FORCED)
		m_SongOptions.Assign(ModsLevel_Stage, m_SongOptions.GetPreferred());

	STATSMAN->m_CurStageStats.m_fMusicRate =
	  m_SongOptions.GetSong().m_fMusicRate;
	m_iNumStagesOfThisSong = GetNumStagesForCurrentSongAndStepsOrCourse();
	ASSERT(m_iNumStagesOfThisSong != -1);

	// only do this check with human players, assume CPU players (Rave)
	// always have tokens. -aj (this could probably be moved below, even.)
	if (!IsEventMode() && !IsCpuPlayer(PLAYER_1)) {
		if (m_iPlayerStageTokens < m_iNumStagesOfThisSong) {
			LuaHelpers::ReportScriptErrorFmt(
			  "Player %d only has %d stage tokens, but needs %d.",
			  PLAYER_1,
			  m_iPlayerStageTokens,
			  m_iNumStagesOfThisSong);
		}
	}
	m_iPlayerStageTokens -= m_iNumStagesOfThisSong;
	if (CurrentOptionsDisqualifyPlayer(PLAYER_1))
		STATSMAN->m_CurStageStats.m_player.m_bDisqualified = true;
}

void
GameState::CancelStage()
{
	m_iPlayerStageTokens += m_iNumStagesOfThisSong;
	m_iNumStagesOfThisSong = 0;
	ResetStageStatistics();
}

void
GameState::CommitStageStats()
{

	STATSMAN->CommitStatsToProfiles(&STATSMAN->m_CurStageStats);

	// Update TotalPlaySeconds.
	int iPlaySeconds =
	  std::max(0, static_cast<int>(m_timeGameStarted.GetDeltaTime()));

	Profile* pPlayerProfile = PROFILEMAN->GetProfile(PLAYER_1);
	if (pPlayerProfile) {
		pPlayerProfile->m_iTotalSessionSeconds += iPlaySeconds;
		STATSMAN->AddPlayerStatsToProfile(pPlayerProfile);
	}
}

/* Called by ScreenSelectMusic (etc). Increment the stage counter if we just
 * played a song. Might be called more than once. */
void
GameState::FinishStage()
{
	// Increment the stage counter.
	++m_iCurrentStageIndex;

	m_iNumStagesOfThisSong = 0;

	// Save the current combo to the profiles (why not)
	Profile* pProfile = PROFILEMAN->GetProfile(PLAYER_1);
	pProfile->m_iCurrentCombo = STATSMAN->m_CurStageStats.m_player.m_iCurCombo;
}

void
GameState::LoadCurrentSettingsFromProfile(PlayerNumber pn)
{
	const Profile* pProfile = PROFILEMAN->GetProfile(pn);

	// apply saved default modifiers if any
	std::string sModifiers;
	if (pProfile->GetDefaultModifiers(m_pCurGame, sModifiers)) {
		/* We don't save negative preferences (eg. "no reverse"). If the theme
		 * sets a default of "reverse", and the player turns it off, we should
		 * set it off. However, don't reset modifiers that aren't saved by the
		 * profile, so we don't ignore unsaved modifiers when a profile is in
		 * use. */
		PO_GROUP_CALL(m_pPlayerState->m_PlayerOptions,
					  ModsLevel_Preferred,
					  ResetSavedPrefs);
		ApplyPreferredModifiers(pn, sModifiers);
	}
	// Only set the sort order if it wasn't already set by a GameCommand (or by
	// an earlier profile)
	if (m_PreferredSortOrder == SortOrder_Invalid &&
		pProfile->m_SortOrder != SortOrder_Invalid)
		m_PreferredSortOrder = pProfile->m_SortOrder;
	if (pProfile->m_LastDifficulty != Difficulty_Invalid)
		m_PreferredDifficulty.Set(pProfile->m_LastDifficulty);
	// Only set the PreferredStepsType if it wasn't already set by a GameCommand
	// (or by an earlier profile)
	if (m_PreferredStepsType == StepsType_Invalid &&
		pProfile->m_LastStepsType != StepsType_Invalid)
		m_PreferredStepsType.Set(pProfile->m_LastStepsType);
	if (m_pPreferredSong == nullptr)
		m_pPreferredSong = pProfile->m_lastSong.ToSong();
}

void
GameState::SaveCurrentSettingsToProfile(PlayerNumber pn)
{
	Profile* pProfile = PROFILEMAN->GetProfile(pn);

	pProfile->SetDefaultModifiers(
	  m_pCurGame,
	  m_pPlayerState->m_PlayerOptions.GetPreferred().GetSavedPrefsString());
	if (IsSongSort(m_PreferredSortOrder))
		pProfile->m_SortOrder = m_PreferredSortOrder;
	if (m_PreferredDifficulty != Difficulty_Invalid)
		pProfile->m_LastDifficulty = m_PreferredDifficulty;
	if (m_PreferredStepsType != StepsType_Invalid)
		pProfile->m_LastStepsType = m_PreferredStepsType;
	if (m_pPreferredSong)
		pProfile->m_lastSong.FromSong(m_pPreferredSong);
}

bool
GameState::CanSafelyEnterGameplay(std::string& reason)
{
	Song const* song = m_pCurSong;
	if (song == nullptr) {
		reason = "Current song is NULL.";
		return false;
	}

	Style const* style = GetCurrentStyle(PLAYER_1);
	if (style == nullptr) {
		reason = ssprintf("Style for player %d is NULL.", PLAYER_1 + 1);
		return false;
	}

	Steps const* steps = m_pCurSteps;
	if (steps == nullptr) {
		reason = ssprintf("Steps for player %d is NULL.", PLAYER_1 + 1);
		return false;
	}
	if (steps->m_StepsType != style->m_StepsType) {
		reason = ssprintf("Player %d StepsType %s for steps does not equal "
						  "StepsType %s for style.",
						  PLAYER_1 + 1,
						  GAMEMAN->GetStepsTypeInfo(steps->m_StepsType).szName,
						  GAMEMAN->GetStepsTypeInfo(style->m_StepsType).szName);
		return false;
	}
	if (steps->m_pSong != m_pCurSong) {
		reason = ssprintf("Steps for player %d are not for the current song.",
						  PLAYER_1 + 1);
		return false;
	}
	NoteData ndtemp;
	steps->GetNoteData(ndtemp);
	if (ndtemp.GetNumTracks() != style->m_iColsPerPlayer) {
		reason = ssprintf("Steps for player %d have %d columns, style has %d "
						  "columns.",
						  PLAYER_1 + 1,
						  ndtemp.GetNumTracks(),
						  style->m_iColsPerPlayer);
		return false;
	}
	return true;
}

void
GameState::SetCompatibleStylesForPlayers()
{
	bool style_set = false;
	if (!style_set) {
		StepsType st = StepsType_Invalid;
		if (m_pCurSteps != nullptr) {
			st = m_pCurSteps->m_StepsType;
		} else {
			std::vector<StepsType> vst;
			GAMEMAN->GetStepsTypesForGame(m_pCurGame, vst);
			st = vst[0];
		}
		const Style* style =
		  GAMEMAN->GetFirstCompatibleStyle(m_pCurGame, GetNumSidesJoined(), st);
		SetCurrentStyle(style, PLAYER_1);
	}
}

void
GameState::ForceOtherPlayersToCompatibleSteps(PlayerNumber main)
{
	Steps* steps_to_match = m_pCurSteps.Get();
	if (steps_to_match == nullptr) {
		return;
	}
	std::string music_to_match = steps_to_match->GetMusicFile();
	Steps* pn_steps = m_pCurSteps.Get();
	bool match_failed = pn_steps == nullptr;
	if (steps_to_match != pn_steps && pn_steps != nullptr) {
		if (music_to_match != pn_steps->GetMusicFile()) {
			match_failed = true;
		}
	}
	if (match_failed) {
		m_pCurSteps.Set(steps_to_match);
	}
}

void
GameState::Update(float fDelta)
{
	m_SongOptions.Update(fDelta);

	m_pPlayerState->Update(fDelta);
}

void
GameState::SetCurGame(const Game* pGame)
{
	m_pCurGame.Set(pGame);
	std::string sGame = pGame ? std::string(pGame->m_szName) : std::string();
	PREFSMAN->SetCurrentGame(sGame);
	discordInit();
	updateDiscordPresenceMenu("");
}

const float GameState::MUSIC_SECONDS_INVALID = -5000.0f;

void
GameState::ResetMusicStatistics()
{
	m_Position.Reset();
	m_LastPositionTimer.Touch();
	m_LastPositionSeconds = 0.0f;

	Actor::SetBGMTime(0, 0, 0, 0);
}

void
GameState::ResetStageStatistics()
{
	StageStats OldStats = STATSMAN->m_CurStageStats;
	STATSMAN->m_CurStageStats = StageStats();
	m_pPlayerState->m_HealthState = HealthState_Alive;

	// Reset the round seed. Do this here and not in FinishStage so that players
	// get new shuffle patterns if they Back out of gameplay and play again.
	SetNewStageSeed();
}

void
GameState::UpdateSongPosition(float fPositionSeconds,
							  const TimingData& timing,
							  const RageTimer& timestamp)
{
	if (m_pCurSteps) {
		m_Position.UpdateSongPosition(
		  fPositionSeconds, *m_pCurSteps->GetTimingData(), timestamp);

		Actor::SetPlayerBGMBeat(m_Position.m_fSongBeatVisible,
								m_Position.m_fSongBeatNoOffset);
	} else {
		m_Position.UpdateSongPosition(fPositionSeconds, timing, timestamp);
	}
	Actor::SetBGMTime(GAMESTATE->m_Position.m_fMusicSecondsVisible,
					  GAMESTATE->m_Position.m_fSongBeatVisible,
					  fPositionSeconds,
					  GAMESTATE->m_Position.m_fSongBeatNoOffset);
}

float
GameState::GetSongPercent(float beat) const
{
	// 0 = first step; 1 = last step
	float curTime = this->m_pCurSong->m_SongTiming.WhereUAtBro(beat);
	return (curTime - m_pCurSong->GetFirstSecond()) /
		   m_pCurSong->GetLastSecond();
}

int
GameState::GetNumSidesJoined() const
{
	int iNumSidesJoined = 0;
	if (m_bSideIsJoined)
		iNumSidesJoined++; // left side, and right side
	return iNumSidesJoined;
}

int
GameState::GetCourseSongIndex() const
{
	// iSongsPlayed includes the current song, so it's 1-based; subtract one.
	return STATSMAN->m_CurStageStats.m_player.m_iSongsPlayed - 1;
}
/* Hack: when we're loading a new course song, we want to display the new song
 * number, even though we haven't started that song yet. */
int
GameState::GetLoadingCourseSongIndex() const
{
	int iIndex = GetCourseSongIndex();
	if (m_bLoadingNextSong)
		++iIndex;
	return iIndex;
}

static LocalizedString PLAYER1("GameState", "Player 1");
static LocalizedString PLAYER2("GameState", "Player 2");
static LocalizedString CPU("GameState", "CPU");
std::string
GameState::GetPlayerDisplayName(PlayerNumber pn) const
{
	ASSERT(IsPlayerEnabled(pn));
	const LocalizedString* pDefaultNames[] = { &PLAYER1, &PLAYER2 };
	if (IsHumanPlayer(pn)) {
		if (!PROFILEMAN->GetPlayerName(pn).empty())
			return PROFILEMAN->GetPlayerName(pn);
		else
			return pDefaultNames[pn]->GetValue();
	} else {
		return CPU.GetValue();
	}
}

bool
GameState::PlayersCanJoin() const
{
	return true;
}

const Game*
GameState::GetCurrentGame() const
{
	ASSERT(m_pCurGame != NULL); // the game must be set before calling this
	return m_pCurGame;
}

const Style*
GameState::GetCurrentStyle(PlayerNumber pn) const
{
	if (GetCurrentGame() == nullptr)
		return nullptr;
	return m_pCurStyle;
}

void
GameState::SetCurrentStyle(const Style* style, PlayerNumber pn)
{
	if (!GetCurrentGame()->m_PlayersHaveSeparateStyles) {
		m_pCurStyle.Set(style);
	} else {
		if (pn == PLAYER_INVALID) {
			m_SeparatedStyles[PLAYER_1] = style;
		} else {
			m_SeparatedStyles[pn] = style;
		}
	}
	if (INPUTMAPPER) {
		if (GetCurrentStyle(pn) &&
			GetCurrentStyle(pn)->m_StyleType == StyleType_OnePlayerTwoSides) {
			// If the other player is joined, unjoin them because this style
			// only allows one player.
			PlayerNumber other_pn =
			  OPPOSITE_PLAYER[this->GetMasterPlayerNumber()];
			if (GetNumSidesJoined() > 1) {
				UnjoinPlayer(other_pn);
			}
			INPUTMAPPER->SetJoinControllers(this->GetMasterPlayerNumber());
		} else
			INPUTMAPPER->SetJoinControllers(PLAYER_INVALID);
	}
}

bool
GameState::SetCompatibleStyle(StepsType stype, PlayerNumber pn)
{
	bool style_incompatible = false;
	if (!GetCurrentStyle(pn)) {
		style_incompatible = true;
	} else {
		style_incompatible = stype != GetCurrentStyle(pn)->m_StepsType;
	}
	if (CommonMetrics::AUTO_SET_STYLE && style_incompatible) {
		const Style* compatible_style = GAMEMAN->GetFirstCompatibleStyle(
		  m_pCurGame, GetNumSidesJoined(), stype);
		if (!compatible_style) {
			return false;
		}
		SetCurrentStyle(compatible_style, pn);
	}
	return stype == GetCurrentStyle(pn)->m_StepsType;
}

bool
GameState::IsPlayerEnabled(PlayerNumber pn) const
{
	return IsHumanPlayer(pn);
}

bool
GameState::IsMultiPlayerEnabled(MultiPlayer mp) const
{
	return m_MultiPlayerStatus[mp] == MultiPlayerStatus_Joined;
}

bool
GameState::IsPlayerEnabled(const PlayerState* pPlayerState) const
{
	if (pPlayerState->m_mp != MultiPlayer_Invalid)
		return IsMultiPlayerEnabled(pPlayerState->m_mp);
	if (pPlayerState->m_PlayerNumber != PLAYER_INVALID)
		return IsPlayerEnabled(pPlayerState->m_PlayerNumber);
	return false;
}

int
GameState::GetNumPlayersEnabled() const
{
	return 1;
}

bool
GameState::IsHumanPlayer(PlayerNumber pn) const
{
	// only player 1 can play this game.
	if (pn != PLAYER_1)
		return false;

	if (GetCurrentGame()->m_PlayersHaveSeparateStyles) {
		if (GetCurrentStyle(pn) == nullptr) // no style chosen
		{
			return m_bSideIsJoined;
		} else {
			StyleType type = GetCurrentStyle(pn)->m_StyleType;
			switch (type) {
				case StyleType_OnePlayerOneSide:
				case StyleType_OnePlayerTwoSides:
					return pn == this->GetMasterPlayerNumber();
				default:
					FAIL_M(ssprintf("Invalid style type: %i", type));
			}
		}
	}
	if (GetCurrentStyle(pn) == nullptr) // no style chosen
	{
		return m_bSideIsJoined; // only allow input from sides that have
								// already joined
	}

	StyleType type = GetCurrentStyle(pn)->m_StyleType;
	switch (type) {
		case StyleType_OnePlayerOneSide:
		case StyleType_OnePlayerTwoSides:
			return pn == this->GetMasterPlayerNumber();
		default:
			FAIL_M(ssprintf("Invalid style type: %i", type));
	}
}

int
GameState::GetNumHumanPlayers() const
{
	return 1;
}

PlayerNumber
GameState::GetFirstHumanPlayer() const
{
	return PLAYER_1;
}

PlayerNumber
GameState::GetFirstDisabledPlayer() const
{
	if (!IsPlayerEnabled(PLAYER_1))
		return PLAYER_1;
	return PLAYER_INVALID;
}

bool
GameState::IsCpuPlayer(PlayerNumber pn) const
{
	return IsPlayerEnabled(pn) && !IsHumanPlayer(pn);
}

bool
GameState::AnyPlayersAreCpu() const
{
	return false;
}

void
GameState::GetDefaultPlayerOptions(PlayerOptions& po)
{
	po.Init();
	po.FromString(PREFSMAN->m_sDefaultModifiers);
	po.FromString(CommonMetrics::DEFAULT_MODIFIERS);
	if (po.m_sNoteSkin.empty())
		po.m_sNoteSkin = CommonMetrics::DEFAULT_NOTESKIN_NAME;
}

void
GameState::GetDefaultSongOptions(SongOptions& so)
{
	so.Init();
	so.FromString(PREFSMAN->m_sDefaultModifiers);
	so.FromString(CommonMetrics::DEFAULT_MODIFIERS);
}

void
GameState::ResetToDefaultSongOptions(ModsLevel l)
{
	SongOptions so;
	GetDefaultSongOptions(so);
	m_SongOptions.Assign(l, so);
}

void
GameState::ApplyPreferredModifiers(PlayerNumber pn,
								   const std::string& sModifiers)
{
	m_pPlayerState->m_PlayerOptions.FromString(ModsLevel_Preferred, sModifiers);
	m_SongOptions.FromString(ModsLevel_Preferred, sModifiers);
}

void
GameState::ApplyStageModifiers(PlayerNumber pn, const std::string& sModifiers)
{
	m_pPlayerState->m_PlayerOptions.FromString(ModsLevel_Stage, sModifiers);
	m_SongOptions.FromString(ModsLevel_Stage, sModifiers);
}

bool
GameState::CurrentOptionsDisqualifyPlayer(PlayerNumber pn)
{
	if (!IsHumanPlayer(pn))
		return false;

	const PlayerOptions& po = m_pPlayerState->m_PlayerOptions.GetPreferred();

	// Check the stored player options for disqualify.  Don't disqualify because
	// of mods that were forced.
	return po.IsEasierForSongAndSteps(m_pCurSong, m_pCurSteps, pn);
}

void
GameState::GetAllUsedNoteSkins(std::vector<std::string>& out) const
{
	// if this list returns multiple values, the values should be unique.
	out.push_back(m_pPlayerState->m_PlayerOptions.GetCurrent().m_sNoteSkin);
}

void
GameState::AddStageToPlayer(PlayerNumber pn)
{
	// Add one stage more to player (bonus) -cerbo
	++m_iPlayerStageTokens;
}

bool
GameState::CountNotesSeparately()
{
	return GetCurrentGame()->m_bCountNotesSeparately ||
		   DisableChordCohesion.Get();
}

template<class T>
void
setmin(T& a, const T& b)
{
	a = min(a, b);
}

template<class T>
void
setmax(T& a, const T& b)
{
	a = max(a, b);
}

FailType
GameState::GetPlayerFailType(const PlayerState* pPlayerState) const
{
	FailType ft = pPlayerState->m_PlayerOptions.GetCurrent().m_FailType;
	return ft;
}

bool
GameState::AllAreInDangerOrWorse() const
{
	if (m_pPlayerState->m_HealthState < HealthState_Danger)
		return false;
	return true;
}

bool
GameState::OneIsHot() const
{
	if (m_pPlayerState->m_HealthState == HealthState_Hot)
		return true;
	return false;
}

int
GameState::GetNumCols(int pn)
{
	return m_pPlayerState->GetNumCols();
}

bool
GameState::ChangePreferredDifficultyAndStepsType(PlayerNumber pn,
												 Difficulty dc,
												 StepsType st)
{
	m_PreferredDifficulty.Set(dc);
	m_PreferredStepsType.Set(st);
	return true;
}

/* When only displaying difficulties in DIFFICULTIES_TO_SHOW, use
 * GetClosestShownDifficulty to find which difficulty to show, and
 * ChangePreferredDifficulty(pn, dir) to change difficulty. */
bool
GameState::ChangePreferredDifficulty(PlayerNumber pn, int dir)
{
	const std::vector<Difficulty>& v =
	  CommonMetrics::DIFFICULTIES_TO_SHOW.GetValue();

	Difficulty d = GetClosestShownDifficulty(pn);
	for (;;) {
		d = enum_add2(d, dir);
		if (d < 0 || d >= NUM_Difficulty) {
			return false;
		}
		if (find(v.begin(), v.end(), d) != v.end()) {
			break; // found
		}
	}
	m_PreferredDifficulty.Set(d);
	return true;
}

/* The user may be set to prefer a difficulty that isn't always shown;
 * typically, Difficulty_Edit. Return the closest shown difficulty <=
 * m_PreferredDifficulty. */
Difficulty
GameState::GetClosestShownDifficulty(PlayerNumber pn) const
{
	const std::vector<Difficulty>& v =
	  CommonMetrics::DIFFICULTIES_TO_SHOW.GetValue();

	auto iClosest = static_cast<Difficulty>(0);
	int iClosestDist = -1;
	FOREACH_CONST(Difficulty, v, dc)
	{
		int iDist = m_PreferredDifficulty - *dc;
		if (iDist < 0)
			continue;
		if (iClosestDist != -1 && iDist > iClosestDist)
			continue;
		iClosestDist = iDist;
		iClosest = *dc;
	}

	return iClosest;
}

Difficulty
GameState::GetEasiestStepsDifficulty() const
{
	Difficulty dc = Difficulty_Invalid;

	if (m_pCurSteps == nullptr) {
		LuaHelpers::ReportScriptErrorFmt(
		  "GetEasiestStepsDifficulty called but p%i hasn't chosen notes",
		  PLAYER_1 + 1);
	}

	dc = std::min(dc, m_pCurSteps->GetDifficulty());
	return dc;
}

Difficulty
GameState::GetHardestStepsDifficulty() const
{
	Difficulty dc = Difficulty_Beginner;
	if (m_pCurSteps == nullptr) {
		LuaHelpers::ReportScriptErrorFmt(
		  "GetHardestStepsDifficulty called but p%i hasn't chosen notes",
		  PLAYER_1 + 1);
	}
	dc = std::max(dc, m_pCurSteps->GetDifficulty());
	return dc;
}

void
GameState::SetNewStageSeed()
{
	m_iStageSeed = g_RandomNumberGenerator();
}

bool
GameState::IsEventMode() const
{
	return m_bTemporaryEventMode || PREFSMAN->m_bEventMode;
}

bool
GameState::PlayerIsUsingModifier(PlayerNumber pn, const std::string& sModifier)
{
	PlayerOptions po = m_pPlayerState->m_PlayerOptions.GetCurrent();
	SongOptions so = m_SongOptions.GetCurrent();
	po.FromString(sModifier);
	so.FromString(sModifier);

	return po == m_pPlayerState->m_PlayerOptions.GetCurrent() &&
		   so == m_SongOptions.GetCurrent();
}

Profile*
GameState::GetEditLocalProfile()
{
	if (m_sEditLocalProfileID.Get().empty())
		return nullptr;
	return PROFILEMAN->GetLocalProfile(m_sEditLocalProfileID);
}

PlayerNumber
GetNextHumanPlayer(PlayerNumber pn)
{
	for (enum_add(pn, 1); pn < NUM_PLAYERS; enum_add(pn, 1))
		if (GAMESTATE->IsHumanPlayer(pn))
			return pn;
	return PLAYER_INVALID;
}

PlayerNumber
GetNextEnabledPlayer(PlayerNumber pn)
{
	for (enum_add(pn, 1); pn < NUM_PLAYERS; enum_add(pn, 1))
		if (GAMESTATE->IsPlayerEnabled(pn))
			return pn;
	return PLAYER_INVALID;
}

PlayerNumber
GetNextCpuPlayer(PlayerNumber pn)
{
	for (enum_add(pn, 1); pn < NUM_PLAYERS; enum_add(pn, 1))
		if (GAMESTATE->IsCpuPlayer(pn))
			return pn;
	return PLAYER_INVALID;
}

PlayerNumber
GetNextPotentialCpuPlayer(PlayerNumber pn)
{
	for (enum_add(pn, 1); pn < NUM_PLAYERS; enum_add(pn, 1))
		if (!GAMESTATE->IsHumanPlayer(pn))
			return pn;
	return PLAYER_INVALID;
}

MultiPlayer
GetNextEnabledMultiPlayer(MultiPlayer mp)
{
	for (enum_add(mp, 1); mp < NUM_MultiPlayer; enum_add(mp, 1))
		if (GAMESTATE->IsMultiPlayerEnabled(mp))
			return mp;
	return MultiPlayer_Invalid;
}

void
GameState::discordInit()
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	Discord_Initialize("378543094531883009", &handlers, 1, nullptr);
}

void
GameState::updateDiscordPresence(const std::string& largeImageText,
								 const std::string& details,
								 const std::string& state,
								 const int64_t endTime)
{
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.details = details.c_str();
	discordPresence.state = state.c_str();
	discordPresence.endTimestamp = endTime;
	discordPresence.largeImageKey = "default";
	discordPresence.largeImageText = largeImageText.c_str();
	Discord_RunCallbacks();
	Discord_UpdatePresence(&discordPresence);
}

void
GameState::updateDiscordPresenceMenu(const std::string& largeImageText)
{
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.details = "In Menus";
	discordPresence.largeImageKey = "default";
	discordPresence.largeImageText = largeImageText.c_str();
	Discord_RunCallbacks();
	Discord_UpdatePresence(&discordPresence);
}

void
GameState::TogglePracticeModeSafe(bool set)
{
	auto screenname = SCREENMAN->GetTopScreen()->GetName();
	bool gameplayscreen =
	  screenname.find("ScreenGameplay") != std::string::npos;

	// This isnt really "safe" but should at least make it harder to break
	if (m_gameplayMode != GameplayMode_Replay && !gameplayscreen) {
		TogglePracticeMode(set);
	}
}

void
GameState::TogglePracticeMode(bool set)
{
	// If we are online, never allow turning practice mode on.
	if (NSMAN != nullptr && NSMAN->isSMOnline && NSMAN->loggedIn &&
		NSMAN->IsETTP())
		set = false;

	m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice = set;
	m_pPlayerState->m_PlayerOptions.GetPreferred().m_bPractice = set;
	m_pPlayerState->m_PlayerOptions.GetSong().m_bPractice = set;
	m_pPlayerState->m_PlayerOptions.GetStage().m_bPractice = set;
	m_gameplayMode.Set(set ? GameplayMode_Practice : GameplayMode_Normal);
}

bool
GameState::IsPracticeMode()
{
	GameplayMode mode = GetGameplayMode();
	bool ispractice =
	  mode == GameplayMode_Practice &&
	  m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice &&
	  m_pPlayerState->m_PlayerOptions.GetPreferred().m_bPractice &&
	  m_pPlayerState->m_PlayerOptions.GetSong().m_bPractice &&
	  m_pPlayerState->m_PlayerOptions.GetStage().m_bPractice;
	return ispractice;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the GameState. */
class LunaGameState : public Luna<GameState>
{
  public:
	DEFINE_METHOD(IsPlayerEnabled, IsPlayerEnabled(PLAYER_1))
	DEFINE_METHOD(IsHumanPlayer, IsHumanPlayer(PLAYER_1))
	DEFINE_METHOD(GetPlayerDisplayName, GetPlayerDisplayName(PLAYER_1))
	DEFINE_METHOD(GetMasterPlayerNumber, GetMasterPlayerNumber())
	DEFINE_METHOD(GetNumMultiplayerNoteFields, m_iNumMultiplayerNoteFields)

	static int SetNumMultiplayerNoteFields(T* p, lua_State* L)
	{
		p->m_iNumMultiplayerNoteFields = IArg(1);
		COMMON_RETURN_SELF;
	}
	static int GetPlayerState(T* p, lua_State* L)
	{
		p->m_pPlayerState->PushSelf(L);
		return 1;
	}
	static int GetMultiPlayerState(T* p, lua_State* L)
	{
		MultiPlayer mp = Enum::Check<MultiPlayer>(L, 1);
		p->m_pMultiPlayerState[mp]->PushSelf(L);
		return 1;
	}
	static int ApplyGameCommand(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_INVALID;
		if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
			// Legacy behavior: if an old-style numerical argument
			// is given, decrement it before trying to parse
			if (lua_isnumber(L, 2)) {
				auto arg = static_cast<int>(lua_tonumber(L, 2));
				arg--;
				LuaHelpers::Push(L, arg);
				lua_replace(L, -2);
			}
			pn = Enum::Check<PlayerNumber>(L, 2);
		}
		p->ApplyGameCommand(SArg(1), pn);
		COMMON_RETURN_SELF;
	}
	static int GetCurrentSong(T* p, lua_State* L)
	{
		if (p->m_pCurSong)
			p->m_pCurSong->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int SetCurrentSong(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) {
			p->m_pCurSong.Set(nullptr);
		} else {
			Song* pS = Luna<Song>::check(L, 1, true);
			p->m_pCurSong.Set(pS);
		}
		COMMON_RETURN_SELF;
	}
	static int CanSafelyEnterGameplay(T* p, lua_State* L)
	{
		std::string reason;
		bool can = p->CanSafelyEnterGameplay(reason);
		lua_pushboolean(L, can);
		LuaHelpers::Push(L, reason);
		return 2;
	}
	static void SetCompatibleStyleOrError(T* p,
										  lua_State* L,
										  StepsType stype,
										  PlayerNumber pn)
	{
		if (!p->SetCompatibleStyle(stype, pn)) {
			luaL_error(L, "No compatible style for steps/trail.");
		}
		if (!p->GetCurrentStyle(pn)) {
			luaL_error(L,
					   "No style set and AutoSetStyle is false, cannot set "
					   "steps/trail.");
		}
	}
	static int GetCurrentSteps(T* p, lua_State* L)
	{
		Steps* pSteps = p->m_pCurSteps;
		if (pSteps) {
			pSteps->PushSelf(L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}
	static int SetCurrentSteps(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;
		if (lua_isnil(L, 2)) {
			p->m_pCurSteps.Set(nullptr);
		} else {
			Steps* pS = Luna<Steps>::check(L, 2);
			SetCompatibleStyleOrError(p, L, pS->m_StepsType, pn);
			p->m_pCurSteps.Set(pS);
			p->ForceOtherPlayersToCompatibleSteps(pn);
		}
		COMMON_RETURN_SELF;
	}
	static int GetPreferredSong(T* p, lua_State* L)
	{
		if (p->m_pPreferredSong)
			p->m_pPreferredSong->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int SetPreferredSong(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) {
			p->m_pPreferredSong = nullptr;
		} else {
			Song* pS = Luna<Song>::check(L, 1);
			p->m_pPreferredSong = pS;
		}
		COMMON_RETURN_SELF;
	}
	static int SetTemporaryEventMode(T* p, lua_State* L)
	{
		p->m_bTemporaryEventMode = BArg(1);
		COMMON_RETURN_SELF;
	}
	static int Env(T* p, lua_State* L)
	{
		p->m_Environment->PushSelf(L);
		return 1;
	}
	static int SetPreferredDifficulty(T* p, lua_State* L)
	{
		Difficulty dc = Enum::Check<Difficulty>(L, 2);
		p->m_PreferredDifficulty.Set(dc);
		COMMON_RETURN_SELF;
	}
	DEFINE_METHOD(GetPreferredDifficulty, m_PreferredDifficulty)
	DEFINE_METHOD(GetSortOrder, m_SortOrder)
	DEFINE_METHOD(GetCurrentStageIndex, m_iCurrentStageIndex)
	DEFINE_METHOD(PlayerIsUsingModifier,
				  PlayerIsUsingModifier(PLAYER_1, SArg(2)))
	DEFINE_METHOD(GetLoadingCourseSongIndex, GetLoadingCourseSongIndex())
	DEFINE_METHOD(GetEasiestStepsDifficulty, GetEasiestStepsDifficulty())
	DEFINE_METHOD(GetHardestStepsDifficulty, GetHardestStepsDifficulty())
	DEFINE_METHOD(IsEventMode, IsEventMode())
	DEFINE_METHOD(GetNumPlayersEnabled, GetNumPlayersEnabled())
	static int GetSongPosition(T* p, lua_State* L)
	{
		p->m_Position.PushSelf(L);
		return 1;
	}
	DEFINE_METHOD(GetLastGameplayDuration, m_DanceDuration)
	DEFINE_METHOD(GetGameplayLeadIn, m_bGameplayLeadIn)
	DEFINE_METHOD(IsSideJoined, m_bSideIsJoined)
	DEFINE_METHOD(PlayersCanJoin, PlayersCanJoin())
	DEFINE_METHOD(GetNumSidesJoined, GetNumSidesJoined())
	DEFINE_METHOD(GetSongOptionsString, m_SongOptions.GetCurrent().GetString())
	DEFINE_METHOD(CountNotesSeparately, CountNotesSeparately())
	static int GetSessionTime(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_timeGameStarted.GetTimeSinceStart());
		return 1;
	}
	static int GetSongOptions(T* p, lua_State* L)
	{
		ModsLevel m = Enum::Check<ModsLevel>(L, 1);
		std::string s = p->m_SongOptions.Get(m).GetString();
		LuaHelpers::Push(L, s);
		return 1;
	}
	static int GetSongOptionsObject(T* p, lua_State* L)
	{
		ModsLevel m = Enum::Check<ModsLevel>(L, 1);
		p->m_SongOptions.Get(m).PushSelf(L);
		return 1;
	}
	static int GetDefaultSongOptions(T* p, lua_State* L)
	{
		SongOptions so;
		p->GetDefaultSongOptions(so);
		lua_pushstring(L, so.GetString().c_str());
		return 1;
	}
	static int ApplyPreferredSongOptionsToOtherLevels(T* p, lua_State* L)
	{
		p->m_SongOptions.Assign(ModsLevel_Preferred,
								p->m_SongOptions.Get(ModsLevel_Preferred));
		return 0;
	}
	static int ApplyStageModifiers(T* p, lua_State* L)
	{
		p->ApplyStageModifiers(PLAYER_1, SArg(2));
		COMMON_RETURN_SELF;
	}
	static int ApplyPreferredModifiers(T* p, lua_State* L)
	{
		p->ApplyPreferredModifiers(PLAYER_1, SArg(2));
		COMMON_RETURN_SELF;
	}
	static int SetSongOptions(T* p, lua_State* L)
	{
		ModsLevel m = Enum::Check<ModsLevel>(L, 1);

		SongOptions so;

		so.FromString(SArg(2));
		p->m_SongOptions.Assign(m, so);
		COMMON_RETURN_SELF;
	}
	static int GetCurrentGame(T* p, lua_State* L)
	{
		const_cast<Game*>(p->GetCurrentGame())->PushSelf(L);
		return 1;
	}
	DEFINE_METHOD(GetEditLocalProfileID, m_sEditLocalProfileID.Get());
	static int GetEditLocalProfile(T* p, lua_State* L)
	{
		Profile* pProfile = p->GetEditLocalProfile();
		if (pProfile)
			pProfile->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	static int GetCurrentStepsCredits(T* t, lua_State* L)
	{
		const Song* pSong = t->m_pCurSong;
		if (pSong == nullptr)
			return 0;

		// use a vector and not a set so that ordering is maintained
		std::vector<const Steps*> vpStepsToShow;
		const Steps* pSteps = GAMESTATE->m_pCurSteps;
		if (pSteps == nullptr)
			return 0;
		bool bAlreadyAdded =
		  find(vpStepsToShow.begin(), vpStepsToShow.end(), pSteps) !=
		  vpStepsToShow.end();
		if (!bAlreadyAdded)
			vpStepsToShow.push_back(pSteps);

		for (unsigned i = 0; i < vpStepsToShow.size(); i++) {
			const Steps* pSteps = vpStepsToShow[i];
			std::string sDifficulty =
			  CustomDifficultyToLocalizedString(GetCustomDifficulty(
				pSteps->m_StepsType, pSteps->GetDifficulty()));

			lua_pushstring(L, sDifficulty.c_str());
			lua_pushstring(L, pSteps->GetDescription().c_str());
		}

		return vpStepsToShow.size() * 2;
	}

	static int SetPreferredSongGroup(T* p, lua_State* L)
	{
		p->m_sPreferredSongGroup.Set(SArg(1));
		COMMON_RETURN_SELF;
	}
	DEFINE_METHOD(GetPreferredSongGroup, m_sPreferredSongGroup.Get());
	static int GetHumanPlayers(T* p, lua_State* L)
	{
		std::vector<PlayerNumber> vHP;
		vHP.push_back(PLAYER_1);

		LuaHelpers::CreateTableFromArray(vHP, L);
		return 1;
	}
	static int GetEnabledPlayers(T*, lua_State* L)
	{
		std::vector<PlayerNumber> vEP;
		vEP.push_back(PLAYER_1);
		LuaHelpers::CreateTableFromArray(vEP, L);
		return 1;
	}
	static int GetCurrentStyle(T* p, lua_State* L)
	{
		Style* pStyle = const_cast<Style*>(p->GetCurrentStyle(PLAYER_1));
		LuaHelpers::Push(L, pStyle);
		return 1;
	}
	static int GetNumStagesForCurrentSongAndStepsOrCourse(T*, lua_State* L)
	{
		lua_pushnumber(L, 1);
		return 1;
	}
	static int GetNumStagesLeft(T* p, lua_State* L)
	{
		lua_pushnumber(L, 1);
		return 1;
	}

	static int GetGameSeed(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->m_iGameSeed);
		return 1;
	}
	static int JoinInput(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->m_iStageSeed);
		return 1;
	}

	static int Reset(T* p, lua_State* L)
	{
		p->Reset();
		COMMON_RETURN_SELF;
	}
	static int JoinPlayer(T* p, lua_State* L)
	{
		p->JoinPlayer(PLAYER_1);
		COMMON_RETURN_SELF;
	}
	static int UnjoinPlayer(T* p, lua_State* L)
	{
		p->UnjoinPlayer(PLAYER_1);
		COMMON_RETURN_SELF;
	}

	static int GetSongPercent(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetSongPercent(FArg(1)));
		return 1;
	}
	DEFINE_METHOD(GetCurMusicSeconds, m_Position.m_fMusicSeconds)

	static int GetExpandedSectionName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->sExpandedSectionName.c_str());
		return 1;
	}
	static int AddStageToPlayer(T* p, lua_State* L)
	{
		p->AddStageToPlayer(PLAYER_1);
		COMMON_RETURN_SELF;
	}
	static int InsertCoin(T* p, lua_State* L) { COMMON_RETURN_SELF; }
	static int InsertCredit(T* p, lua_State* L) { COMMON_RETURN_SELF; }
	static int CurrentOptionsDisqualifyPlayer(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->CurrentOptionsDisqualifyPlayer(PLAYER_1));
		return 1;
	}

	static int ResetPlayerOptions(T* p, lua_State* L)
	{
		p->ResetPlayerOptions(PLAYER_1);
		COMMON_RETURN_SELF;
	}

	static int RefreshNoteSkinData(T* p, lua_State* L)
	{
		NOTESKIN->RefreshNoteSkinData(p->m_pCurGame);
		COMMON_RETURN_SELF;
	}

	static int LoadProfiles(T* p, lua_State* L)
	{
		bool LoadEdits = true;
		if (lua_isboolean(L, 1)) {
			LoadEdits = BArg(1);
		}
		p->LoadProfiles(LoadEdits);
		SCREENMAN->ZeroNextUpdate();
		COMMON_RETURN_SELF;
	}

	static int SaveProfiles(T* p, lua_State* L)
	{
		p->SavePlayerProfile();
		SCREENMAN->ZeroNextUpdate();
		COMMON_RETURN_SELF;
	}

	static int SetFailTypeExplicitlySet(T* p, lua_State* L)
	{
		p->m_bFailTypeWasExplicitlySet = true;
		COMMON_RETURN_SELF;
	}

	static int StoreRankingName(T* p, lua_State* L) { COMMON_RETURN_SELF; }

	DEFINE_METHOD(HaveProfileToLoad, HaveProfileToLoad())
	DEFINE_METHOD(HaveProfileToSave, HaveProfileToSave())

	static int SetCurrentStyle(T* p, lua_State* L)
	{
		const Style* pStyle = nullptr;
		if (lua_isstring(L, 1)) {
			std::string style = SArg(1);
			pStyle =
			  GAMEMAN->GameAndStringToStyle(GAMESTATE->m_pCurGame, style);
			if (!pStyle) {
				luaL_error(L,
						   "SetCurrentStyle: %s is not a valid style.",
						   style.c_str());
			}
		} else {
			pStyle = Luna<Style>::check(L, 1);
		}

		StyleType st = pStyle->m_StyleType;
		if (p->GetNumSidesJoined() == 2 &&
			(st == StyleType_OnePlayerOneSide ||
			 st == StyleType_OnePlayerTwoSides)) {
			luaL_error(
			  L, "Too many sides joined for style %s", pStyle->m_szName);
		}

		p->SetCurrentStyle(pStyle, PLAYER_1);
		COMMON_RETURN_SELF;
	}

	static int IsCourseMode(T* p, lua_State* L)
	{ // course mode is dead but leave this here for now -mina
		lua_pushboolean(L, false);
		return 1;
	}
	static int GetCoinMode(T* p, lua_State* L)
	{
		lua_pushstring(L, "CoinMode_Home");
		return 1;
	}

	static int UpdateDiscordMenu(T* p, lua_State* L)
	{
		p->updateDiscordPresenceMenu(SArg(1));
		return 1;
	}

	static int UpdateDiscordPresence(T* p, lua_State* L)
	{
		p->updateDiscordPresence(SArg(1), SArg(2), SArg(3), IArg(4));
		return 1;
	}
	static int IsPaused(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->GetPaused());
		return 1;
	}
	static int SetAutoplay(T* p, lua_State* L)
	{
		// Don't allow disabling replay controlller
		if (PC_REPLAY == p->m_pPlayerState->m_PlayerController ||
			PC_REPLAY == GamePreferences::m_AutoPlay)
			return 0;
		p->m_pPlayerState->m_PlayerController = BArg(1) ? PC_CPU : PC_HUMAN;
		GamePreferences::m_AutoPlay.Set(p->m_pPlayerState->m_PlayerController);
		return 0;
	}
	static int GetGameplayMode(T* p, lua_State* L)
	{
		GameplayMode mode = p->GetGameplayMode();
		LuaHelpers::Push(L, mode);
		return 1;
	}
	static int SetPracticeMode(T* p, lua_State* L)
	{
		p->TogglePracticeModeSafe(BArg(1));
		return 0;
	}
	static int GetTimingScales(T* p, lua_State* L)
	{
		LuaHelpers::CreateTableFromArray(p->timingscales, L);
		return 1;
	}
	static int GetPreferredStepsType(T* p, lua_State* L)
	{
		auto st = p->m_PreferredStepsType;
		LuaHelpers::Push(L, st);
		return 1;
	}

	DEFINE_METHOD(GetEtternaVersion, GetEtternaVersion())
	DEFINE_METHOD(IsPracticeMode, IsPracticeMode())
	LunaGameState()
	{
		ADD_METHOD(SetAutoplay);
		ADD_METHOD(IsPlayerEnabled);
		ADD_METHOD(IsHumanPlayer);
		ADD_METHOD(GetPlayerDisplayName);
		ADD_METHOD(GetMasterPlayerNumber);
		ADD_METHOD(GetNumMultiplayerNoteFields);
		ADD_METHOD(SetNumMultiplayerNoteFields);
		ADD_METHOD(GetPlayerState);
		ADD_METHOD(GetMultiPlayerState);
		ADD_METHOD(ApplyGameCommand);
		ADD_METHOD(CanSafelyEnterGameplay);
		ADD_METHOD(GetCurrentSong);
		ADD_METHOD(SetCurrentSong);
		ADD_METHOD(GetCurrentSteps);
		ADD_METHOD(SetCurrentSteps);
		ADD_METHOD(GetSessionTime);
		ADD_METHOD(SetPreferredSong);
		ADD_METHOD(GetPreferredSong);
		ADD_METHOD(SetTemporaryEventMode);
		ADD_METHOD(Env);
		ADD_METHOD(SetPreferredDifficulty);
		ADD_METHOD(GetPreferredDifficulty);
		ADD_METHOD(GetSortOrder);
		ADD_METHOD(GetCurrentStageIndex);
		ADD_METHOD(PlayerIsUsingModifier);
		ADD_METHOD(GetLoadingCourseSongIndex);
		ADD_METHOD(GetEasiestStepsDifficulty);
		ADD_METHOD(GetHardestStepsDifficulty);
		ADD_METHOD(IsEventMode);
		ADD_METHOD(GetNumPlayersEnabled);
		ADD_METHOD(GetSongPosition);
		ADD_METHOD(GetLastGameplayDuration);
		ADD_METHOD(GetGameplayLeadIn);
		ADD_METHOD(IsSideJoined);
		ADD_METHOD(PlayersCanJoin);
		ADD_METHOD(GetNumSidesJoined);
		ADD_METHOD(GetSongOptionsString);
		ADD_METHOD(GetSongOptions);
		ADD_METHOD(GetSongOptionsObject);
		ADD_METHOD(GetDefaultSongOptions);
		ADD_METHOD(ApplyPreferredSongOptionsToOtherLevels);
		ADD_METHOD(ApplyPreferredModifiers);
		ADD_METHOD(ApplyStageModifiers);
		ADD_METHOD(SetSongOptions);
		ADD_METHOD(GetCurrentGame);
		ADD_METHOD(GetEditLocalProfileID);
		ADD_METHOD(GetEditLocalProfile);
		ADD_METHOD(GetCurrentStepsCredits);
		ADD_METHOD(SetPreferredSongGroup);
		ADD_METHOD(GetPreferredSongGroup);
		ADD_METHOD(GetHumanPlayers);
		ADD_METHOD(GetEnabledPlayers);
		ADD_METHOD(GetCurrentStyle);
		ADD_METHOD(GetNumStagesForCurrentSongAndStepsOrCourse);
		ADD_METHOD(GetNumStagesLeft);
		ADD_METHOD(GetGameSeed);
		ADD_METHOD(Reset);
		ADD_METHOD(JoinPlayer);
		ADD_METHOD(UnjoinPlayer);
		ADD_METHOD(JoinInput);
		ADD_METHOD(GetSongPercent);
		ADD_METHOD(GetCurMusicSeconds);
		ADD_METHOD(GetExpandedSectionName);
		ADD_METHOD(AddStageToPlayer);
		ADD_METHOD(InsertCoin);
		ADD_METHOD(InsertCredit);
		ADD_METHOD(CurrentOptionsDisqualifyPlayer);
		ADD_METHOD(ResetPlayerOptions);
		ADD_METHOD(RefreshNoteSkinData);
		ADD_METHOD(LoadProfiles);
		ADD_METHOD(SaveProfiles);
		ADD_METHOD(HaveProfileToLoad);
		ADD_METHOD(HaveProfileToSave);
		ADD_METHOD(SetFailTypeExplicitlySet);
		ADD_METHOD(SetCurrentStyle);
		ADD_METHOD(IsCourseMode);
		ADD_METHOD(GetEtternaVersion);
		ADD_METHOD(CountNotesSeparately);
		ADD_METHOD(GetCoinMode);
		ADD_METHOD(UpdateDiscordMenu);
		ADD_METHOD(UpdateDiscordPresence);
		ADD_METHOD(IsPaused);
		ADD_METHOD(GetGameplayMode);
		ADD_METHOD(IsPracticeMode);
		ADD_METHOD(SetPracticeMode);
		ADD_METHOD(GetTimingScales);
		ADD_METHOD(GetPreferredStepsType);
	}
};

LUA_REGISTER_CLASS(GameState)
// lua end
