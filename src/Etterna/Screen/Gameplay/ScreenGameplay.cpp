#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/AdjustSync.h"
#include "Etterna/Actor/Gameplay/ArrowEffects.h"
#include "Etterna/Actor/Gameplay/Background.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Actor/Gameplay/DancingCharacters.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Actor/Gameplay/Foreground.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Actor/Gameplay/LifeMeter.h"
#include "Etterna/Actor/Gameplay/LifeMeterBar.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Models/Misc/LyricsLoader.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"
#include "Etterna/Models/NoteData/NoteDataWithScoring.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Models/Misc/PlayerAI.h" // for NUM_SKILL_LEVELS
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Models/Misc/Profile.h" // for replay data stuff
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Sound/RageSoundReader.h"
#include "RageUtil/Misc/RageTimer.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "ScreenGameplay.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Screen/Others/ScreenSaveSync.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/Songs/SongUtil.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Actor/GameplayAndMenus/StepsDisplay.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"
#include "Etterna/Models/Misc/Profile.h" // for replay data stuff
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/ScoreManager.h"

// Defines
#define SHOW_LIFE_METER_FOR_DISABLED_PLAYERS                                   \
	THEME->GetMetricB(m_sName, "ShowLifeMeterForDisabledPlayers")
#define SONG_POSITION_METER_WIDTH                                              \
	THEME->GetMetricF(m_sName, "SongPositionMeterWidth")

static ThemeMetric<float> INITIAL_BACKGROUND_BRIGHTNESS(
  "ScreenGameplay",
  "InitialBackgroundBrightness");
static ThemeMetric<float> SECONDS_BETWEEN_COMMENTS("ScreenGameplay",
												   "SecondsBetweenComments");
static ThemeMetric<RString> SCORE_KEEPER_CLASS("ScreenGameplay",
											   "ScoreKeeperClass");
static ThemeMetric<bool> FORCE_IMMEDIATE_FAIL_FOR_BATTERY(
  "ScreenGameplay",
  "ForceImmediateFailForBattery");

AutoScreenMessage(SM_PlayGo);

// received while STATE_DANCING
AutoScreenMessage(SM_LoadNextSong);
AutoScreenMessage(SM_StartLoadingNextSong);

// received while STATE_OUTRO
AutoScreenMessage(SM_DoPrevScreen);
AutoScreenMessage(SM_DoNextScreen);

// received while STATE_INTRO
AutoScreenMessage(SM_StartHereWeGo);
AutoScreenMessage(SM_StopHereWeGo);

AutoScreenMessage(SM_BattleTrickLevel1);
AutoScreenMessage(SM_BattleTrickLevel2);
AutoScreenMessage(SM_BattleTrickLevel3);

static Preference<bool> g_bCenter1Player("Center1Player", true);
static Preference<bool> g_bShowLyrics("ShowLyrics", false);
static Preference<float> g_fNetStartOffset("NetworkStartOffset", -3.0);
static Preference<bool> g_bEasterEggs("EasterEggs", true);

PlayerInfo::PlayerInfo()
  : m_pn(PLAYER_INVALID)
  , m_PlayerStateDummy()
  , m_PlayerStageStatsDummy()
  , m_SoundEffectControl()
  , m_vpStepsQueue()
  , m_pLifeMeter(NULL)
  , m_ptextStepsDescription(NULL)
  , m_pPrimaryScoreKeeper(NULL)
  , m_pSecondaryScoreKeeper(NULL)
  , m_ptextPlayerOptions(NULL)
  , m_NoteData()
  , m_pPlayer(NULL)
  , m_pStepsDisplay(NULL)
{
}

void
PlayerInfo::Load(PlayerNumber pn,
				 MultiPlayer mp,
				 bool bShowNoteField,
				 int iAddToDifficulty)
{
	m_pn = pn;
	m_mp = mp;
	m_bPlayerEnabled = IsEnabled();
	m_bIsDummy = false;
	m_iAddToDifficulty = iAddToDifficulty;
	m_pLifeMeter = NULL;
	m_ptextStepsDescription = NULL;

	if (!IsMultiPlayer()) {
		PlayMode mode = GAMESTATE->m_PlayMode;
		switch (mode) {
			case PLAY_MODE_REGULAR:
				break;
			default:
				FAIL_M(ssprintf("Invalid PlayMode: %i", mode));
		}
	}

	PlayerState* const pPlayerState = GetPlayerState();
	PlayerStageStats* const pPlayerStageStats = GetPlayerStageStats();
	m_pPrimaryScoreKeeper = ScoreKeeper::MakeScoreKeeper(
	  SCORE_KEEPER_CLASS, pPlayerState, pPlayerStageStats);

	m_ptextPlayerOptions = NULL;
	m_pPlayer = new Player(m_NoteData, bShowNoteField);
	m_pStepsDisplay = NULL;

	if (IsMultiPlayer()) {
		pPlayerState->m_PlayerOptions =
		  GAMESTATE->m_pPlayerState->m_PlayerOptions;
	}
}

void
PlayerInfo::LoadDummyP1(int iDummyIndex, int iAddToDifficulty)
{
	m_pn = PLAYER_1;
	m_bPlayerEnabled = IsEnabled();
	m_bIsDummy = true;
	m_iDummyIndex = iDummyIndex;
	m_iAddToDifficulty = iAddToDifficulty;

	// don't init any of the scoring objects
	m_pPlayer = new Player(m_NoteData, true);

	// PlayerOptions needs to be set now so that we load the correct NoteSkin.
	m_PlayerStateDummy = *GAMESTATE->m_pPlayerState;
}

PlayerInfo::~PlayerInfo()
{
	SAFE_DELETE(m_pLifeMeter);
	SAFE_DELETE(m_ptextStepsDescription);
	SAFE_DELETE(m_pPrimaryScoreKeeper);
	SAFE_DELETE(m_pSecondaryScoreKeeper);
	SAFE_DELETE(m_ptextPlayerOptions);
	SAFE_DELETE(m_pPlayer);
	SAFE_DELETE(m_pStepsDisplay);
}

PlayerState*
PlayerInfo::GetPlayerState()
{
	if (m_bIsDummy)
		return &m_PlayerStateDummy;
	return IsMultiPlayer()
			 ? GAMESTATE
				 ->m_pMultiPlayerState[GetPlayerStateAndStageStatsIndex()]
			 : GAMESTATE->m_pPlayerState;
}

PlayerStageStats*
PlayerInfo::GetPlayerStageStats()
{
	// multiplayer chooses the PlayerStageStats with the highest score on
	// StageFinalized
	if (m_bIsDummy || IsMultiPlayer())
		return &m_PlayerStageStatsDummy;
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

std::vector<PlayerInfo>::iterator
GetNextEnabledPlayerInfo(std::vector<PlayerInfo>::iterator iter,
						 std::vector<PlayerInfo>& v)
{
	for (; iter != v.end(); ++iter) {
		if (!iter->m_bPlayerEnabled)
			continue;
		return iter;
	}
	return iter;
}

std::vector<PlayerInfo>::iterator
GetNextEnabledPlayerInfoNotDummy(std::vector<PlayerInfo>::iterator iter,
								 std::vector<PlayerInfo>& v)
{
	for (; iter != v.end(); iter++) {
		if (iter->m_bIsDummy)
			continue;
		if (!iter->m_bPlayerEnabled)
			continue;
		return iter;
	}
	return iter;
}

std::vector<PlayerInfo>::iterator
GetNextEnabledPlayerNumberInfo(std::vector<PlayerInfo>::iterator iter,
							   std::vector<PlayerInfo>& v)
{
	for (; iter != v.end(); ++iter) {
		if (iter->m_bIsDummy)
			continue;
		if (!iter->m_bPlayerEnabled)
			continue;
		if (iter->m_mp != MultiPlayer_Invalid)
			continue;
		return iter;
	}
	return iter;
}

std::vector<PlayerInfo>::iterator
GetNextPlayerNumberInfo(std::vector<PlayerInfo>::iterator iter,
						std::vector<PlayerInfo>& v)
{
	for (; iter != v.end(); ++iter) {
		if (iter->m_bIsDummy)
			continue;
		if (iter->m_pn == PLAYER_INVALID)
			continue;
		return iter;
	}
	return iter;
}

std::vector<PlayerInfo>::iterator
GetNextVisiblePlayerInfo(std::vector<PlayerInfo>::iterator iter,
						 std::vector<PlayerInfo>& v)
{
	for (; iter != v.end(); ++iter) {
		if (!iter->m_pPlayer->HasVisibleParts())
			continue;
		return iter;
	}
	return iter;
}

////////////////////////////////////////////////////////////////////////////////

ScreenGameplay::ScreenGameplay()
{
	m_pSongBackground = NULL;
	m_pSongForeground = NULL;
	m_delaying_ready_announce = false;
	GAMESTATE->m_AdjustTokensBySongCostForFinalStageCheck = false;
	DLMAN->UpdateDLSpeed(true);
	if (GamePreferences::m_AutoPlay != PC_REPLAY) {
		LOG->Trace("Unloading replaydata.");
		SCOREMAN->UnloadAllReplayData();
	}
}

void
ScreenGameplay::Init()
{
	SubscribeToMessage("Judgment");

	PLAYER_TYPE.Load(m_sName, "PlayerType");
	PLAYER_INIT_COMMAND.Load(m_sName, "PlayerInitCommand");
	GIVE_UP_START_TEXT.Load(m_sName, "GiveUpStartText");
	GIVE_UP_BACK_TEXT.Load(m_sName, "GiveUpBackText");
	GIVE_UP_ABORTED_TEXT.Load(m_sName, "GiveUpAbortedText");
	SKIP_SONG_TEXT.Load(m_sName, "SkipSongText");
	GIVE_UP_SECONDS.Load(m_sName, "GiveUpSeconds");
	MUSIC_FADE_OUT_SECONDS.Load(m_sName, "MusicFadeOutSeconds");
	OUT_TRANSITION_LENGTH.Load(m_sName, "OutTransitionLength");
	BEGIN_FAILED_DELAY.Load(m_sName, "BeginFailedDelay");
	MIN_SECONDS_TO_STEP.Load(m_sName, "MinSecondsToStep");
	MIN_SECONDS_TO_MUSIC.Load(m_sName, "MinSecondsToMusic");
	MIN_SECONDS_TO_STEP_NEXT_SONG.Load(m_sName, "MinSecondsToStepNextSong");
	START_GIVES_UP.Load(m_sName, "StartGivesUp");
	BACK_GIVES_UP.Load(m_sName, "BackGivesUp");
	SELECT_SKIPS_SONG.Load(m_sName, "SelectSkipsSong");
	GIVING_UP_GOES_TO_PREV_SCREEN.Load(m_sName, "GivingUpGoesToPrevScreen");
	FAIL_ON_MISS_COMBO.Load(m_sName, "FailOnMissCombo");
	ALLOW_CENTER_1_PLAYER.Load(m_sName, "AllowCenter1Player");

	if (UseSongBackgroundAndForeground()) {
		m_pSongBackground = new Background;
		m_pSongForeground = new Foreground;
	}

	ScreenWithMenuElements::Init();

	this->FillPlayerInfo(m_vPlayerInfo);

	{
		ASSERT_M(!m_vPlayerInfo.empty(),
				 "m_vPlayerInfo must be filled by FillPlayerInfo");

		int iNumEnabledPlayers = 0;
		FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)++ iNumEnabledPlayers;

		/* If this is 0, we have no active players and havn't been initialized
		 * correctly. */
		ASSERT(iNumEnabledPlayers > 0);
	}

	m_pSoundMusic = NULL;

	if (GAMESTATE->m_pCurSong == NULL)
		return; // ScreenDemonstration will move us to the next screen.  We just
				// need to survive for one update without crashing.

	/* Called once per stage (single song or single course). */
	GAMESTATE->BeginStage();

	GAMESTATE->SetPaused(false);
	m_fReplayBookmarkSeconds = 0.f;

	int player = 1;
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		unsigned int count = pi->m_vpStepsQueue.size();

		for (unsigned int i = 0; i < count; i++) {
			Steps* curSteps = pi->m_vpStepsQueue[i];
			if (curSteps->IsNoteDataEmpty()) {
				if (curSteps->GetNoteDataFromSimfile()) {
					LOG->Trace("Notes should be loaded for player %d", player);
				} else {
					LOG->Trace("Error loading notes for player %d", player);
				}
			}
		}
		player++;
	}

	FOREACH_PotentialCpuPlayer(p)
	{
		PlayerNumber human_pn = GAMESTATE->GetFirstHumanPlayer();
		GAMESTATE->m_pCurSteps.Set(GAMESTATE->m_pCurSteps);
		if (GAMESTATE->GetCurrentGame()->m_PlayersHaveSeparateStyles) {
			GAMESTATE->SetCurrentStyle(GAMESTATE->GetCurrentStyle(human_pn), p);
		}
	}

	ASSERT(GAMESTATE->m_pCurSteps.Get() != NULL);

	STATSMAN->m_CurStageStats.m_playMode = GAMESTATE->m_PlayMode;
	STATSMAN->m_CurStageStats.m_player.m_pStyle =
	  GAMESTATE->GetCurrentStyle(PLAYER_1);
	FOREACH_MultiPlayer(pn)
	{
		STATSMAN->m_CurStageStats.m_multiPlayer[pn].m_pStyle =
		  GAMESTATE->GetCurrentStyle(PLAYER_INVALID);
	}

	/* Record combo rollover. */
	FOREACH_EnabledPlayerInfoNotDummy(m_vPlayerInfo, pi)
	  pi->GetPlayerStageStats()
		->UpdateComboList(0, true);

	m_DancingState = STATE_INTRO;

	// Set this in LoadNextSong()
	// m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;

	m_bZeroDeltaOnNextUpdate = false;

	if (m_pSongBackground != nullptr) {
		m_pSongBackground->SetName("SongBackground");
		m_pSongBackground->SetDrawOrder(DRAW_ORDER_BEFORE_EVERYTHING);
		ActorUtil::LoadAllCommands(*m_pSongBackground, m_sName);
		this->AddChild(m_pSongBackground);
	}

	if (m_pSongForeground != nullptr) {
		m_pSongForeground->SetName("SongForeground");
		m_pSongForeground->SetDrawOrder(
		  DRAW_ORDER_OVERLAY +
		  1); // on top of the overlay, but under transitions
		ActorUtil::LoadAllCommands(*m_pSongForeground, m_sName);
		this->AddChild(m_pSongForeground);
	}

	m_Toasty.Load(THEME->GetPathB(m_sName, "toasty"));
	this->AddChild(&m_Toasty);

	// Use the margin function to calculate where the notefields should be and
	// what size to zoom them to.  This way, themes get margins to put cut-ins
	// in, and the engine can have players on different styles without the
	// notefields overlapping. -Kyz
	LuaReference margarine;
	float margins[2] = { 40, 40 };
	THEME->GetMetric(m_sName, "MarginFunction", margarine);
	if (margarine.GetLuaType() != LUA_TFUNCTION) {
		LuaHelpers::ReportScriptErrorFmt(
		  "MarginFunction metric for %s must be a function.", m_sName.c_str());
	} else {
		Lua* L = LUA->Get();
		margarine.PushSelf(L);
		lua_createtable(L, 0, 0);
		int next_player_slot = 1;
		Enum::Push(L, PLAYER_1);
		lua_rawseti(L, -2, next_player_slot);
		++next_player_slot;
		Enum::Push(L, GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StyleType);
		RString err = "Error running MarginFunction:  ";
		if (LuaHelpers::RunScriptOnStack(L, err, 2, 3, true)) {
			RString marge = "Margin value must be a number.";
			margins[0] = static_cast<float>(SafeFArg(L, -3, marge, 40));
			float center = static_cast<float>(SafeFArg(L, -2, marge, 80));
			margins[1] = center / 2.0f;
		}
		lua_settop(L, 0);
		LUA->Release(L);
	}

	float left_edge = 0.0f;
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		RString sName("PlayerP1");
		pi->m_pPlayer->SetName(sName);
		Style const* style = GAMESTATE->GetCurrentStyle(PLAYER_1);
		float style_width = style->GetWidth(PLAYER_1);
		float edge = left_edge;
		float screen_space;
		float field_space;
		float left_marge;
		float right_marge;
		screen_space = SCREEN_WIDTH / 2.0f;
		left_marge = margins[0];
		right_marge = margins[1];
		field_space = screen_space - left_marge - right_marge;
		float player_x = edge + left_marge + (field_space / 2.0f);
		pi->GetPlayerState()->m_NotefieldZoom = 1.f;

		if (!Center1Player())
			pi->m_pPlayer->SetX(player_x);
		else
			pi->m_pPlayer->SetX(SCREEN_CENTER_X);
		pi->m_pPlayer->RunCommands(PLAYER_INIT_COMMAND);
		// ActorUtil::LoadAllCommands(pi->m_pPlayer, m_sName);
		this->AddChild(pi->m_pPlayer);
		pi->m_pPlayer->PlayCommand("On");
	}

	m_NextSong.Load(THEME->GetPathB(m_sName, "next course song"));
	m_NextSong.SetDrawOrder(DRAW_ORDER_TRANSITIONS - 1);
	this->AddChild(&m_NextSong);

	// Before the lifemeter loads, if Networking is required
	// we need to wait, so that there is no Dead On Start issues.
	// if you wait too long at the second checkpoint, you will
	// appear dead when you begin your game.
	if (GAMESTATE->m_bPlayingMulti)
		NSMAN->StartRequest(0);

	// Add individual life meter
	switch (GAMESTATE->m_PlayMode) {
		case PLAY_MODE_REGULAR:
			FOREACH_PlayerNumberInfo(m_vPlayerInfo, pi)
			{
				if (!GAMESTATE->IsPlayerEnabled(pi->m_pn) &&
					!SHOW_LIFE_METER_FOR_DISABLED_PLAYERS)
					continue; // skip

				pi->m_pLifeMeter = LifeMeter::MakeLifeMeter(
				  pi->GetPlayerState()->m_PlayerOptions.GetStage().m_LifeType);
				pi->m_pLifeMeter->Load(pi->GetPlayerState(),
									   pi->GetPlayerStageStats());
				pi->m_pLifeMeter->SetName(
				  ssprintf("Life%s", pi->GetName().c_str()));
				LOAD_ALL_COMMANDS_AND_SET_XY(pi->m_pLifeMeter);
				this->AddChild(pi->m_pLifeMeter);
			}
			break;
		default:
			break;
	}

	m_bShowScoreboard = false;
	m_textSongOptions.LoadFromFont(THEME->GetPathF(m_sName, "song options"));
	m_textSongOptions.SetShadowLength(0);
	m_textSongOptions.SetName("SongOptions");
	LOAD_ALL_COMMANDS_AND_SET_XY(m_textSongOptions);
	m_textSongOptions.SetText(
	  GAMESTATE->m_SongOptions.GetStage().GetLocalizedString());
	this->AddChild(&m_textSongOptions);

	if (g_bShowLyrics) {
		m_LyricDisplay.SetName("LyricDisplay");
		LOAD_ALL_COMMANDS(m_LyricDisplay);
		this->AddChild(&m_LyricDisplay);
	}

	m_Ready.Load(THEME->GetPathB(m_sName, "ready"));
	this->AddChild(&m_Ready);

	m_Go.Load(THEME->GetPathB(m_sName, "go"));
	this->AddChild(&m_Go);

	m_Failed.Load(THEME->GetPathB(m_sName, "failed"));
	m_Failed.SetDrawOrder(DRAW_ORDER_TRANSITIONS -
						  1); // on top of everything else
	this->AddChild(&m_Failed);

	m_textDebug.LoadFromFont(THEME->GetPathF(m_sName, "debug"));
	m_textDebug.SetName("Debug");
	LOAD_ALL_COMMANDS_AND_SET_XY(m_textDebug);
	m_textDebug.SetDrawOrder(DRAW_ORDER_TRANSITIONS -
							 1); // just under transitions, over the foreground
	this->AddChild(&m_textDebug);

	m_GameplayAssist.Init();

	if (m_pSongBackground != nullptr)
		m_pSongBackground->Init();

	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		RString sType = PLAYER_TYPE;
		if (pi->m_bIsDummy)
			sType += "Dummy";
		pi->m_pPlayer->Init(sType,
							pi->GetPlayerState(),
							pi->GetPlayerStageStats(),
							pi->m_pLifeMeter,
							pi->m_pPrimaryScoreKeeper,
							pi->m_pSecondaryScoreKeeper);
	}

	// fill in m_apSongsQueue, m_vpStepsQueue, m_asModifiersQueue
	InitSongQueues();

	// Fill StageStats
	STATSMAN->m_CurStageStats.m_vpPossibleSongs = m_apSongsQueue;
	FOREACH(PlayerInfo, m_vPlayerInfo, pi)
	{
		if (pi->GetPlayerStageStats())
			pi->GetPlayerStageStats()->m_vpPossibleSteps = pi->m_vpStepsQueue;
	}

	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		ASSERT(!pi->m_vpStepsQueue.empty());
		if (pi->GetPlayerStageStats())
			pi->GetPlayerStageStats()->m_bJoined = true;
		if (pi->m_pPrimaryScoreKeeper)
			pi->m_pPrimaryScoreKeeper->Load(m_apSongsQueue, pi->m_vpStepsQueue);
		if (pi->m_pSecondaryScoreKeeper)
			pi->m_pSecondaryScoreKeeper->Load(m_apSongsQueue,
											  pi->m_vpStepsQueue);
	}

	GAMESTATE->m_bGameplayLeadIn.Set(true);

	/* LoadNextSong first, since that positions some elements which need to be
	 * positioned before we TweenOnScreen. */
	LoadNextSong();

	m_GiveUpTimer.SetZero();
	m_SkipSongTimer.SetZero();
	m_gave_up = false;
	m_skipped_song = false;

	// Force FailOff in Practice Mode
	if (GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice) {
		GAMEMAN->m_iPreviousFail =
		  GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred().m_FailType;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_FailType =
		  FailType_Off;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong().m_FailType =
		  FailType_Off;
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred().m_FailType =
		  FailType_Off;
	}

	if (!GAMEMAN->m_bRestartedGameplay &&
		GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice) {
		GAMEMAN->m_bResetModifiers = true;
		GAMEMAN->m_fPreviousRate =
		  GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate;
		GAMEMAN->m_sModsToReset =
		  GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().GetString();
	}
}

bool
ScreenGameplay::Center1Player() const
{
	/* Perhaps this should be handled better by defining a new
	 * StyleType for ONE_PLAYER_ONE_CREDIT_AND_ONE_COMPUTER,
	 * but for now just ignore Center1Player when it's Battle or Rave
	 * Mode. This doesn't begin to address two-player solo (6 arrows) */
	return g_bCenter1Player && (bool)ALLOW_CENTER_1_PLAYER;
}

// fill in m_apSongsQueue, m_vpStepsQueue, m_asModifiersQueue
void
ScreenGameplay::InitSongQueues()
{
	m_apSongsQueue.push_back(GAMESTATE->m_pCurSong);
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		Steps* pSteps = GAMESTATE->m_pCurSteps;
		pi->m_vpStepsQueue.push_back(pSteps);
	}

	if (GAMESTATE->IsPlaylistCourse()) {

		m_apSongsQueue.clear();
		FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi) pi->m_vpStepsQueue.clear();

		Playlist& pl = SONGMAN->GetPlaylists()[SONGMAN->playlistcourse];
		FOREACH(Chart, pl.chartlist, ch)
		{
			m_apSongsQueue.emplace_back(ch->songptr);
			FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
			{
				pi->m_vpStepsQueue.emplace_back(ch->stepsptr);
				ratesqueue.emplace_back(ch->rate);
			}
		}
	}

	if (GAMESTATE->m_bMultiplayer) {
		for (int i = 0; i < static_cast<int>(m_apSongsQueue.size()); i++) {
			Song* pSong = m_apSongsQueue[i];

			FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
			{
				Steps* pOldSteps = pi->m_vpStepsQueue[i];

				std::vector<Steps*> vpSteps;
				SongUtil::GetSteps(pSong, vpSteps, pOldSteps->m_StepsType);
				StepsUtil::SortNotesArrayByDifficulty(vpSteps);
				std::vector<Steps*>::iterator iter =
				  find(vpSteps.begin(), vpSteps.end(), pOldSteps);
				int iIndexBase = 0;
				if (iter != vpSteps.end()) {
					iIndexBase = iter - vpSteps.begin();
					CLAMP(iIndexBase,
						  0,
						  vpSteps.size() -
							GAMESTATE->m_iNumMultiplayerNoteFields);
				}

				int iIndexToUse = iIndexBase + pi->m_iAddToDifficulty;
				CLAMP(iIndexToUse, 0, vpSteps.size() - 1);

				Steps* pSteps = vpSteps[iIndexToUse];
				pi->m_vpStepsQueue[i] = pSteps;
			}
		}
	}
}

ScreenGameplay::~ScreenGameplay()
{
	GAMESTATE->m_AdjustTokensBySongCostForFinalStageCheck = true;
	if (this->IsFirstUpdate()) {
		/* We never received any updates. That means we were deleted without
		 * being used, and never actually played. (This can happen when backing
		 * out of ScreenStage.) Cancel the stage. */
		GAMESTATE->CancelStage();
	}

	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("ScreenGameplay::~ScreenGameplay()");

	SAFE_DELETE(m_pSongBackground);
	SAFE_DELETE(m_pSongForeground);

	if (m_pSoundMusic != nullptr)
		m_pSoundMusic->StopPlaying();

	m_GameplayAssist.StopPlaying();

	if (GAMESTATE->m_bPlayingMulti)
		NSMAN->ReportSongOver();
	DLMAN->UpdateDLSpeed(false);
}

void
ScreenGameplay::SetupSong(int iSongIndex)
{
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		/* This is the first beat that can be changed without it being visible.
		 * Until we draw for the first time, any beat can be changed. */
		pi->GetPlayerState()->m_fLastDrawnBeat = -100;

		Steps* pSteps = pi->m_vpStepsQueue[iSongIndex];
		GAMESTATE->m_pCurSteps.Set(pSteps);

		NoteData originalNoteData;
		pSteps->GetNoteData(originalNoteData);

		const Style* pStyle = GAMESTATE->GetCurrentStyle(pi->m_pn);
		NoteData ndTransformed;
		pStyle->GetTransformedNoteDataForStyle(
		  pi->GetStepsAndTrailIndex(), originalNoteData, ndTransformed);

		pi->GetPlayerState()->Update(0);

		// load player
		{
			pi->m_NoteData = ndTransformed;
			NoteDataUtil::RemoveAllTapsOfType(pi->m_NoteData,
											  TapNoteType_AutoKeysound);
			pi->m_pPlayer->Load();
		}

		// load auto keysounds
		{
			NoteData nd = ndTransformed;
			NoteDataUtil::RemoveAllTapsExceptForType(nd,
													 TapNoteType_AutoKeysound);
			m_AutoKeysounds.Load(pi->GetStepsAndTrailIndex(), nd);
		}

		{
			RString sType;
			switch (GAMESTATE->m_SongOptions.GetCurrent().m_SoundEffectType) {
				case SoundEffectType_Off:
					sType = "SoundEffectControl_Off";
					break;
				case SoundEffectType_Speed:
					sType = "SoundEffectControl_Speed";
					break;
				case SoundEffectType_Pitch:
					sType = "SoundEffectControl_Pitch";
					break;
				default:
					break;
			}

			pi->m_SoundEffectControl.Load(
			  sType, pi->GetPlayerState(), &pi->m_NoteData);
		}

		pi->GetPlayerState()->Update(0);

		// Hack: Course modifiers that are set to start immediately shouldn't
		// tween on.
		pi->GetPlayerState()->m_PlayerOptions.SetCurrentToLevel(
		  ModsLevel_Stage);
	}
}

void
ScreenGameplay::ReloadCurrentSong()
{
	FOREACH_EnabledPlayerInfoNotDummy(m_vPlayerInfo, pi)
	  pi->GetPlayerStageStats()
		->m_iSongsPlayed--;

	LoadNextSong();
}

void
ScreenGameplay::LoadNextSong()
{
	// never allow input to remain redirected during gameplay unless an lua
	// script forces it when loaded below -mina
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	  SCREENMAN->set_input_redirected(pi->m_pn, false);

	GAMESTATE->ResetMusicStatistics();

	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		pi->GetPlayerStageStats()->m_iSongsPlayed++;
	}

	if (GAMESTATE->m_bMultiplayer) {
		FOREACH_ENUM(MultiPlayer, mp)
		this->UpdateStageStats(mp);
	}

	int iPlaySongIndex = GAMESTATE->GetCourseSongIndex();
	iPlaySongIndex %= m_apSongsQueue.size();
	GAMESTATE->m_pCurSong.Set(m_apSongsQueue[iPlaySongIndex]);
	// Check if the music actually exists, this is to avoid an issue in
	// AutoKeysounds.cpp, where the reader will ignore whether the file opener
	// function actually returned a valid object or an error. - Terra
	GAMESTATE->m_pCurSong.Get()->ReloadIfNoMusic();

	STATSMAN->m_CurStageStats.m_vpPlayedSongs.push_back(GAMESTATE->m_pCurSong);

	// Force immediate fail behavior changed to theme metric by Kyz.
	if (FORCE_IMMEDIATE_FAIL_FOR_BATTERY) {
		FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
		{
			if (pi->GetPlayerState()->m_PlayerOptions.GetStage().m_LifeType ==
				LifeType_Battery) {
				PO_GROUP_ASSIGN(pi->GetPlayerState()->m_PlayerOptions,
								ModsLevel_Song,
								m_FailType,
								FailType_Immediate);
			}
		}
	}

	m_textSongOptions.SetText(
	  GAMESTATE->m_SongOptions.GetCurrent().GetString());

	SetupSong(iPlaySongIndex);

	Song* pSong = GAMESTATE->m_pCurSong;
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		Steps* pSteps = GAMESTATE->m_pCurSteps;
		++pi->GetPlayerStageStats()->m_iStepsPlayed;

		ASSERT(GAMESTATE->m_pCurSteps != NULL);
		if (pi->m_ptextStepsDescription)
			pi->m_ptextStepsDescription->SetText(pSteps->GetDescription());

		if (pi->m_ptextPlayerOptions)
			pi->m_ptextPlayerOptions->SetText(
			  pi->GetPlayerState()->m_PlayerOptions.GetCurrent().GetString());

		if (pi->m_pStepsDisplay)
			pi->m_pStepsDisplay->SetFromSteps(pSteps);

		/* The actual note data for scoring is the base class of Player.  This
		 * includes transforms, like Wide.  Otherwise, the scoring will operate
		 * on the wrong data. */
		if (pi->m_pPrimaryScoreKeeper)
			pi->m_pPrimaryScoreKeeper->OnNextSong(
			  GAMESTATE->GetCourseSongIndex(),
			  pSteps,
			  &pi->m_pPlayer->GetNoteData());
		if (pi->m_pSecondaryScoreKeeper)
			pi->m_pSecondaryScoreKeeper->OnNextSong(
			  GAMESTATE->GetCourseSongIndex(),
			  pSteps,
			  &pi->m_pPlayer->GetNoteData());

		// Don't mess with the PlayerController of the Dummy player
		if (!pi->m_bIsDummy) {
			if (GAMESTATE->IsCpuPlayer(pi->GetStepsAndTrailIndex())) {
				pi->GetPlayerState()->m_PlayerController = PC_CPU;
				int iMeter = pSteps->GetMeter();
				int iNewSkill =
				  SCALE(iMeter, MIN_METER, MAX_METER, 0, NUM_SKILL_LEVELS - 1);
				/* Watch out: songs aren't actually bound by MAX_METER. */
				iNewSkill = clamp<int>(iNewSkill, 0, NUM_SKILL_LEVELS - 1);
				pi->GetPlayerState()->m_iCpuSkill = iNewSkill;
			} else {
				if (pi->GetPlayerState()
					  ->m_PlayerOptions.GetCurrent()
					  .m_fPlayerAutoPlay != 0)
					pi->GetPlayerState()->m_PlayerController = PC_AUTOPLAY;
				else
					pi->GetPlayerState()->m_PlayerController =
					  GamePreferences::m_AutoPlay;
			}
		}
	}

	bool bAllReverse = true;
	bool bAtLeastOneReverse = false;
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		if (pi->GetPlayerState()
			  ->m_PlayerOptions.GetCurrent()
			  .m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1)
			bAtLeastOneReverse = true;
		else
			bAllReverse = false;
	}

	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		bool bReverse = pi->GetPlayerState()
						  ->m_PlayerOptions.GetCurrent()
						  .m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1;

		if (pi->m_pStepsDisplay)
			pi->m_pStepsDisplay->PlayCommand(bReverse ? "SetReverse"
													  : "SetNoReverse");
	}

	m_LyricDisplay.PlayCommand(
	  bAllReverse ? "SetReverse"
				  : bAtLeastOneReverse ? "SetOneReverse" : "SetNoReverse");

	// Load lyrics
	// XXX: don't load this here (who and why? -aj)
	LyricsLoader LL;
	if (GAMESTATE->m_pCurSong->HasLyrics())
		LL.LoadFromLRCFile(GAMESTATE->m_pCurSong->GetLyricsPath(),
						   *GAMESTATE->m_pCurSong);

	// Set up song-specific graphics.

	if (m_pSongBackground != nullptr)
		m_pSongBackground->Unload();

	if (m_pSongForeground != nullptr)
		m_pSongForeground->Unload();

	// BeginnerHelper disabled, or failed to load.
	if (m_pSongBackground)
		m_pSongBackground->LoadFromSong(GAMESTATE->m_pCurSong);

	if (m_pSongBackground != nullptr) {
		m_pSongBackground->SetBrightness(INITIAL_BACKGROUND_BRIGHTNESS);
		m_pSongBackground->FadeToActualBrightness();
	}

	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		if (!pi->GetPlayerStageStats()->m_bFailed) {
			// give a little life back between stages
			if (pi->m_pLifeMeter)
				pi->m_pLifeMeter->OnLoadSong();
		}
	}

	if (m_pSongForeground)
		m_pSongForeground->LoadFromSong(GAMESTATE->m_pCurSong);

	m_fTimeSinceLastDancingComment = 0;

	/* m_soundMusic and m_pSongBackground take a very long time to load,
	 * so cap fDelta at 0 so m_NextSong will show up on screen.
	 * -Chris */
	m_bZeroDeltaOnNextUpdate = true;
	SCREENMAN->ZeroNextUpdate();

	/* Load the music last, since it may start streaming and we don't want the
	 * music to compete with other loading. */
	m_AutoKeysounds.FinishLoading();
	m_pSoundMusic = m_AutoKeysounds.GetSound();

	/* Give SoundEffectControls the new RageSoundReaders. */
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		RageSoundReader* pPlayerSound =
		  m_AutoKeysounds.GetPlayerSound(pi->m_pn);
		if (pPlayerSound == NULL &&
			pi->m_pn == GAMESTATE->GetMasterPlayerNumber())
			pPlayerSound = m_AutoKeysounds.GetSharedSound();
		pi->m_SoundEffectControl.SetSoundReader(pPlayerSound);
	}

	if (!GAMESTATE->GetPaused())
		MESSAGEMAN->Broadcast("DoneLoadingNextSong");
}

void
ScreenGameplay::StartPlayingSong(float fMinTimeToNotes, float fMinTimeToMusic)
{
	ASSERT(fMinTimeToNotes >= 0);
	ASSERT(fMinTimeToMusic >= 0);

	m_pSoundMusic->SetProperty("AccurateSync", true);

	RageSoundParams p;
	p.m_fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	p.StopMode = RageSoundParams::M_CONTINUE;

	{
		const float fFirstSecond = GAMESTATE->m_pCurSong->GetFirstSecond();
		float fStartDelay = fMinTimeToNotes - fFirstSecond;
		fStartDelay = std::max(fStartDelay, fMinTimeToMusic);
		p.m_StartSecond = -fStartDelay * p.m_fSpeed;
	}

	ASSERT(!m_pSoundMusic->IsPlaying());
	{
		float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
		GetMusicEndTiming(fSecondsToStartFadingOutMusic,
						  fSecondsToStartTransitioningOut);

		if (fSecondsToStartFadingOutMusic <
			GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
								MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		}
	}
	m_pSoundMusic->Play(false, &p);

	/* Make sure GAMESTATE->m_fMusicSeconds is set up. */
	GAMESTATE->m_Position.m_fMusicSeconds = -5000;
	UpdateSongPosition(0);

	ASSERT(GAMESTATE->m_Position.m_fMusicSeconds >
		   -4000); /* make sure the "fake timer" code doesn't trigger */

	if (GAMESTATE->m_pCurSteps) {
		GAMESTATE->m_pCurSteps->GetTimingData()->PrepareLookup();
	}
}

// play assist ticks
void
ScreenGameplay::PlayTicks()
{
	/* TODO: Allow all players to have ticks. Not as simple as it looks.
	 * If a loop takes place, it could make one player's ticks come later
	 * than intended. Any help here would be appreciated. -Wolfman2000 */
	Player& player =
	  *m_vPlayerInfo[GAMESTATE->GetMasterPlayerNumber()].m_pPlayer;
	const NoteData& nd = player.GetNoteData();
	m_GameplayAssist.PlayTicks(nd, player.GetPlayerState());
}

/* Play announcer "type" if it's been at least fSeconds since the last
 * announcer. */
void
ScreenGameplay::PlayAnnouncer(const RString& type,
							  float fSeconds,
							  float* fDeltaSeconds)
{
	/* Don't play before the first beat, or after we're finished. */
	if (m_DancingState != STATE_DANCING)
		return;
	if (GAMESTATE->m_pCurSong ==
		  NULL || // this will be true on ScreenDemonstration sometimes
		GAMESTATE->m_Position.m_fSongBeat <
		  GAMESTATE->m_pCurSong->GetFirstBeat())
		return;

	if (*fDeltaSeconds < fSeconds)
		return;
	*fDeltaSeconds = 0;

	SOUND->PlayOnceFromAnnouncer(type);
}

void
ScreenGameplay::UpdateSongPosition(float fDeltaTime)
{
	if (!m_pSoundMusic->IsPlaying())
		return;

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);
	const float fAdjust = SOUND->GetFrameTimingAdjustment(fDeltaTime);
	GAMESTATE->UpdateSongPosition(
	  fSeconds + fAdjust, GAMESTATE->m_pCurSong->m_SongTiming, tm + fAdjust);
}

void
ScreenGameplay::BeginScreen()
{
	if (GAMESTATE->m_pCurSong == NULL)
		return;

	ScreenWithMenuElements::BeginScreen();

	SOUND->PlayOnceFromAnnouncer("gameplay intro"); // crowd cheer

	// Tell multi to do its thing -poco
	if (GAMESTATE->m_bPlayingMulti && NSMAN->useSMserver) {
		NSMAN->StartRequest(1);
	}

	// Then go
	StartPlayingSong(MIN_SECONDS_TO_STEP, MIN_SECONDS_TO_MUSIC);

	if (GAMESTATE->m_bPlayingMulti) {
		this->SetInterval(
		  [this]() {
			  auto& ptns = this->GetPlayerInfo(PLAYER_1)
							 ->GetPlayerStageStats()
							 ->m_iTapNoteScores;

			  RString doot = ssprintf("%d I %d I %d I %d I %d I %d  x%d",
									  ptns[TNS_W1],
									  ptns[TNS_W2],
									  ptns[TNS_W3],
									  ptns[TNS_W4],
									  ptns[TNS_W5],
									  ptns[TNS_Miss],
									  this->GetPlayerInfo(PLAYER_1)
										->GetPlayerStageStats()
										->m_iCurCombo);
			  NSMAN->SendMPLeaderboardUpdate(
				this->GetPlayerInfo(PLAYER_1)->m_pPlayer->curwifescore /
				  this->GetPlayerInfo(PLAYER_1)->m_pPlayer->maxwifescore,
				doot);
		  },
		  0.25f,
		  -1);
	}
}

bool
ScreenGameplay::AllAreFailing()
{
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		if (pi->m_pLifeMeter && !pi->m_pLifeMeter->IsFailing())
			return false;
	}
	return true;
}

void
ScreenGameplay::GetMusicEndTiming(float& fSecondsToStartFadingOutMusic,
								  float& fSecondsToStartTransitioningOut)
{
	float fLastStepSeconds = GAMESTATE->m_pCurSong->GetLastSecond();
	fLastStepSeconds += Player::GetMaxStepDistanceSeconds();

	float fTransitionLength;
	fTransitionLength = OUT_TRANSITION_LENGTH;

	fSecondsToStartTransitioningOut = fLastStepSeconds;

	// Align the end of the music fade to the end of the transition.
	float fSecondsToFinishFadingOutMusic =
	  fSecondsToStartTransitioningOut + fTransitionLength;
	if (fSecondsToFinishFadingOutMusic <
		GAMESTATE->m_pCurSong->m_fMusicLengthSeconds)
		fSecondsToStartFadingOutMusic =
		  fSecondsToFinishFadingOutMusic - MUSIC_FADE_OUT_SECONDS;
	else
		fSecondsToStartFadingOutMusic =
		  GAMESTATE->m_pCurSong->m_fMusicLengthSeconds; // don't fade

	/* Make sure we keep going long enough to register a miss for the last note,
	 * and never start fading before the last note. */
	fSecondsToStartFadingOutMusic =
	  std::max(fSecondsToStartFadingOutMusic, fLastStepSeconds);
	fSecondsToStartTransitioningOut =
	  std::max(fSecondsToStartTransitioningOut, fLastStepSeconds);

	/* Make sure the fade finishes before the transition finishes. */
	fSecondsToStartTransitioningOut =
	  std::max(fSecondsToStartTransitioningOut,
		  fSecondsToStartFadingOutMusic + MUSIC_FADE_OUT_SECONDS -
			fTransitionLength);
}

void
ScreenGameplay::Update(float fDeltaTime)
{
	if (GAMESTATE->m_pCurSong == NULL) {
		/* ScreenDemonstration will move us to the next screen.  We just need to
		 * survive for one update without crashing.  We need to call
		 * Screen::Update to make sure we receive the next-screen message. */
		Screen::Update(fDeltaTime);
		return;
	}

	UpdateSongPosition(fDeltaTime);

	if (m_bZeroDeltaOnNextUpdate) {
		Screen::Update(0);
		m_bZeroDeltaOnNextUpdate = false;
	} else {
		Screen::Update(fDeltaTime);
	}

	/* This happens if ScreenDemonstration::HandleScreenMessage sets a new
	 * screen when PREFSMAN->m_bDelayedScreenLoad. */
	if (GAMESTATE->m_pCurSong == NULL)
		return;
	/* This can happen if ScreenDemonstration::HandleScreenMessage sets a new
	 * screen when !PREFSMAN->m_bDelayedScreenLoad.  (The new screen was loaded
	 * when we called Screen::Update, and the ctor might set a new
	 * GAMESTATE->m_pCurSong, so the above check can fail.) */
	if (SCREENMAN->GetTopScreen() != this)
		return;

	// LOG->Trace( "m_fOffsetInBeats = %f, m_fBeatsPerSecond = %f,
	// m_Music.GetPositionSeconds = %f", m_fOffsetInBeats, m_fBeatsPerSecond,
	// m_Music.GetPositionSeconds() );

	m_AutoKeysounds.Update(fDeltaTime);

	// update GameState HealthState
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		HealthState& hs = pi->GetPlayerState()->m_HealthState;
		HealthState OldHealthState = hs;
		if (GAMESTATE->GetPlayerFailType(pi->GetPlayerState()) !=
			  FailType_Off &&
			pi->m_pLifeMeter && pi->m_pLifeMeter->IsFailing()) {
			hs = HealthState_Dead;
		} else if (pi->m_pLifeMeter && pi->m_pLifeMeter->IsHot()) {
			hs = HealthState_Hot;
		} else if (GAMESTATE->GetPlayerFailType(pi->GetPlayerState()) !=
					 FailType_Off &&
				   pi->m_pLifeMeter && pi->m_pLifeMeter->IsInDanger()) {
			hs = HealthState_Danger;
		} else {
			hs = HealthState_Alive;
		}

		if (hs != OldHealthState) {
			Message msg("HealthStateChanged");
			msg.SetParam("PlayerNumber", pi->m_pn);
			msg.SetParam("HealthState", hs);
			msg.SetParam("OldHealthState", OldHealthState);
			MESSAGEMAN->Broadcast(msg);
		}

		pi->m_SoundEffectControl.Update(fDeltaTime);
	}

	{
		float fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		RageSoundParams p = m_pSoundMusic->GetParams();
		if (fabsf(p.m_fSpeed - fSpeed) > 0.01f && fSpeed >= 0.0f) {
			p.m_fSpeed = fSpeed;
			m_pSoundMusic->SetParams(p);
		}
	}

	switch (m_DancingState) {
		case STATE_DANCING: {
			/* Set STATSMAN->m_CurStageStats.bFailed for failed players.  In,
			 * FAIL_IMMEDIATE, send SM_BeginFailed if all players failed, and
			 * kill dead Oni players. */
			FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
			{
				PlayerNumber pn = pi->GetStepsAndTrailIndex();

				FailType ft =
				  GAMESTATE->GetPlayerFailType(pi->GetPlayerState());
				LifeType lt =
				  pi->GetPlayerState()->m_PlayerOptions.GetStage().m_LifeType;
				if (ft == FailType_Off || ft == FailType_EndOfSong)
					continue;

				// check for individual fail
				if (pi->m_pLifeMeter == NULL || !pi->m_pLifeMeter->IsFailing())
					continue; /* isn't failing */
				if (pi->GetPlayerStageStats()->m_bFailed)
					continue; /* failed and is already dead */

				LOG->Trace("Player %d failed", static_cast<int>(pn));
				pi->GetPlayerStageStats()->m_bFailed = true; // fail

				{
					Message msg("PlayerFailed");
					msg.SetParam("PlayerNumber", pi->m_pn);
					MESSAGEMAN->Broadcast(msg);
				}

				// Check for and do Oni die.
				bool bAllowOniDie = false;
				switch (lt) {
					case LifeType_Battery:
						bAllowOniDie = true;
					default:
						break;
				}
				if (bAllowOniDie && ft == FailType_Immediate) {
					if (!STATSMAN->m_CurStageStats
						   .AllFailed()) // if not the last one to fail
					{
						// kill them!
						FailFadeRemovePlayer(&*pi);
					}
				}
			}

			bool bAllFailed = true;
			FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
			{
				FailType ft =
				  GAMESTATE->GetPlayerFailType(pi->GetPlayerState());
				switch (ft) {
					case FailType_Immediate:
						if (pi->m_pLifeMeter == NULL ||
							(pi->m_pLifeMeter &&
							 !pi->m_pLifeMeter->IsFailing()))
							bAllFailed = false;
						break;
					case FailType_ImmediateContinue:
					case FailType_EndOfSong:
						bAllFailed =
						  false; // wait until the end of the song to fail.
						break;
					case FailType_Off:
						bAllFailed = false; // never fail.
						break;
					default:
						FAIL_M("Invalid fail type! Aborting...");
				}
			}

			if (bAllFailed) {
				m_pSoundMusic->StopPlaying();
				SCREENMAN->PostMessageToTopScreen(SM_NotesEnded, 0);
				m_LyricDisplay.Stop();
			}

			// Update living players' alive time
			// HACK: Don't scale alive time when using tab/tilde.  Instead of
			// accumulating time from a timer, this time should instead be tied
			// to the music position.
			float fUnscaledDeltaTime = m_timerGameplaySeconds.GetDeltaTime();

			FOREACH_EnabledPlayerInfo(
			  m_vPlayerInfo, pi) if (!pi->GetPlayerStageStats()->m_bFailed)
			  pi->GetPlayerStageStats()
				->m_fAliveSeconds +=
			  fUnscaledDeltaTime *
			  GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

			// update fGameplaySeconds
			STATSMAN->m_CurStageStats.m_fGameplaySeconds += fUnscaledDeltaTime;
			float curBeat = GAMESTATE->m_Position.m_fSongBeat;
			Song& s = *GAMESTATE->m_pCurSong;

			if (curBeat >= s.GetFirstBeat() && curBeat < s.GetLastBeat()) {
				STATSMAN->m_CurStageStats.m_fStepsSeconds += fUnscaledDeltaTime;
			}

			// Check for end of song
			{
				float fSecondsToStartFadingOutMusic,
				  fSecondsToStartTransitioningOut;
				GetMusicEndTiming(fSecondsToStartFadingOutMusic,
								  fSecondsToStartTransitioningOut);

				bool bAllReallyFailed = STATSMAN->m_CurStageStats.AllFailed();
				if (bAllReallyFailed)
					fSecondsToStartTransitioningOut += BEGIN_FAILED_DELAY;

				if (GAMESTATE->m_Position.m_fMusicSeconds >=
					  fSecondsToStartTransitioningOut &&
					!m_NextSong.IsTransitioning())
					this->PostScreenMessage(SM_NotesEnded, 0);
			}

			FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi)
			{
				DancingCharacters* pCharacter = NULL;
				if (m_pSongBackground)
					pCharacter = m_pSongBackground->GetDancingCharacters();
				if (pCharacter != NULL) {
					TapNoteScore tns = pi->m_pPlayer->GetLastTapNoteScore();

					ANIM_STATES_2D state = AS2D_MISS;

					switch (tns) {
						case TNS_W4:
						case TNS_W3:
							state = AS2D_GOOD;
							break;
						case TNS_W2:
						case TNS_W1:
							state = AS2D_GREAT;
							break;
						default:
							state = AS2D_MISS;
							break;
					}

					if (state == AS2D_GREAT &&
						pi->GetPlayerState()->m_HealthState == HealthState_Hot)
						state = AS2D_FEVER;

					pCharacter->Change2DAnimState(pi->m_pn, state);
				}
			}

			// update give up
			bool bGiveUpTimerFired = false;
			bGiveUpTimerFired =
			  !m_GiveUpTimer.IsZero() && m_GiveUpTimer.Ago() > GIVE_UP_SECONDS;
			m_gave_up = bGiveUpTimerFired;
			m_skipped_song = !m_SkipSongTimer.IsZero() &&
							 m_SkipSongTimer.Ago() > GIVE_UP_SECONDS;

			bool bAllHumanHaveBigMissCombo = true;
			FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi)
			{
				if (pi->GetPlayerState()
						->m_PlayerOptions.GetCurrent()
						.m_FailType == FailType_Off ||
					pi->GetPlayerState()->m_HealthState < HealthState_Dead) {
					bAllHumanHaveBigMissCombo = false;
					break;
				}
			}
			if (bAllHumanHaveBigMissCombo) // possible to get in here.
			{
				bAllHumanHaveBigMissCombo =
				  FAIL_ON_MISS_COMBO.GetValue() != -1 &&
				  STATSMAN->m_CurStageStats.GetMinimumMissCombo() >=
					(unsigned int)FAIL_ON_MISS_COMBO;
			}
			if (bGiveUpTimerFired || bAllHumanHaveBigMissCombo ||
				m_skipped_song) {
				STATSMAN->m_CurStageStats.m_bGaveUp = true;
				FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi)
				{
					pi->GetPlayerStageStats()->m_bFailed |=
					  bAllHumanHaveBigMissCombo;
					pi->GetPlayerStageStats()->m_bDisqualified |=
					  bGiveUpTimerFired; // Don't disqualify if failing for miss
										 // combo.  The player should still be
										 // eligable for a high score on
										 // courses.
					pi->GetPlayerStageStats()->gaveuplikeadumbass |= m_gave_up;
				}
				ResetGiveUpTimers(false);
				if (GIVING_UP_GOES_TO_PREV_SCREEN && !m_skipped_song) {
					if (GamePreferences::m_AutoPlay == PC_REPLAY ||
						GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
						  .m_bPractice) {
						if (GAMEMAN->m_bResetModifiers) {
							float oldRate = GAMEMAN->m_fPreviousRate;
							const RString mods = GAMEMAN->m_sModsToReset;
							/*
							GAMESTATE->m_pPlayerState
							  ->m_PlayerOptions.GetSong()
							  .FromString("clearall");
							GAMESTATE->m_pPlayerState
							  ->m_PlayerOptions.GetCurrent()
							  .FromString("clearall");
							GAMESTATE->m_pPlayerState
							  ->m_PlayerOptions.GetPreferred()
							  .FromString("clearall");
							GAMESTATE->m_pPlayerState
							  ->m_PlayerOptions.GetSong()
							  .FromString(mods);
							GAMESTATE->m_pPlayerState
							  ->m_PlayerOptions.GetCurrent()
							  .FromString(mods);
							GAMESTATE->m_pPlayerState
							  ->m_PlayerOptions.GetPreferred()
							  .FromString(mods);
							*/
							const std::vector<RString> oldturns =
							  GAMEMAN->m_vTurnsToReset;
							if (GAMEMAN->m_bResetTurns) {
								GAMESTATE->m_pPlayerState->m_PlayerOptions
								  .GetSong()
								  .ResetModsToStringVector(oldturns);
								GAMESTATE->m_pPlayerState->m_PlayerOptions
								  .GetCurrent()
								  .ResetModsToStringVector(oldturns);
								GAMESTATE->m_pPlayerState->m_PlayerOptions
								  .GetPreferred()
								  .ResetModsToStringVector(oldturns);
								GAMEMAN->m_bResetTurns = false;
								GAMEMAN->m_vTurnsToReset.clear();
							}
							GAMESTATE->m_SongOptions.GetSong().m_fMusicRate =
							  oldRate;
							GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate =
							  oldRate;
							GAMESTATE->m_SongOptions.GetPreferred()
							  .m_fMusicRate = oldRate;
							FailType failreset = GAMEMAN->m_iPreviousFail;
							GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong()
							  .m_FailType = failreset;
							GAMESTATE->m_pPlayerState->m_PlayerOptions
							  .GetCurrent()
							  .m_FailType = failreset;
							GAMESTATE->m_pPlayerState->m_PlayerOptions
							  .GetPreferred()
							  .m_FailType = failreset;
							GAMEMAN->m_bResetModifiers = false;
							GAMEMAN->m_sModsToReset = "";
							MESSAGEMAN->Broadcast("RateChanged");
						}
						GamePreferences::m_AutoPlay.Set(PC_HUMAN);
					}
					BeginBackingOutFromGameplay();
				} else {
					m_pSoundMusic->StopPlaying();
					this->PostScreenMessage(SM_NotesEnded, 0);
				}
				return;
			}

			// Check to see if it's time to play a ScreenGameplay comment
			m_fTimeSinceLastDancingComment += fDeltaTime;

			PlayMode mode = GAMESTATE->m_PlayMode;
			switch (mode) {
				case PLAY_MODE_REGULAR:
					if (GAMESTATE->OneIsHot())
						PlayAnnouncer("gameplay comment hot",
									  SECONDS_BETWEEN_COMMENTS);
					else if (GAMESTATE->AllAreInDangerOrWorse())
						PlayAnnouncer("gameplay comment danger",
									  SECONDS_BETWEEN_COMMENTS);
					else
						PlayAnnouncer("gameplay comment good",
									  SECONDS_BETWEEN_COMMENTS);
					break;
				default:
					FAIL_M(ssprintf("Invalid PlayMode: %i", mode));
			}
		}
		default:
			break;
	}

	PlayTicks();
	SendCrossedMessages();

	if (GAMESTATE->m_bPlayingMulti && NSMAN->useSMserver) {
		FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi) if (pi->m_pLifeMeter)
		  NSMAN->m_playerLife = int(pi->m_pLifeMeter->GetLife() * 10000);

		if (m_bShowScoreboard)
			FOREACH_NSScoreBoardColumn(
			  cn) if (m_bShowScoreboard && NSMAN->ChangedScoreboard(cn) &&
					  GAMESTATE->GetFirstDisabledPlayer() != PLAYER_INVALID)
			  m_Scoreboard[cn]
				.SetText(NSMAN->m_Scoreboard[cn]);
	}
	// ArrowEffects::Update call moved because having it happen once per
	// NoteField (which means twice in two player) seemed wasteful. -Kyz
	ArrowEffects::Update();
}

void
ScreenGameplay::DrawPrimitives()
{
	// ScreenGameplay::DrawPrimitives exists so that the notefield board can be
	// above the song background and underneath everything else.  This way, a
	// theme can put a screen filter in the notefield board and not have it
	// obscure custom elements on the screen.  Putting the screen filter in the
	// notefield board simplifies placement because it ensures that the filter
	// is in the same place as the notefield, instead of forcing the filter to
	// check conditions and metrics that affect the position of the notefield.
	// This also solves the problem of the ComboUnderField metric putting the
	// combo underneath the opaque notefield board.
	// -Kyz
	if (m_pSongBackground != nullptr) {
		m_pSongBackground->m_disable_draw = false;
		m_pSongBackground->Draw();
		m_pSongBackground->m_disable_draw = true;
	}
	m_vPlayerInfo[0].m_pPlayer->DrawNoteFieldBoard();
	ScreenWithMenuElements::DrawPrimitives();
}

void
ScreenGameplay::FailFadeRemovePlayer(PlayerInfo* pi)
{
	SOUND->PlayOnceFromDir(THEME->GetPathS(m_sName, "oni die"));
	int tracks = pi->m_NoteData.GetNumTracks();
	pi->m_NoteData.Init();				 // remove all notes and scoring
	pi->m_NoteData.SetNumTracks(tracks); // reset the number of tracks.
	pi->m_pPlayer->FadeToFail();		 // tell the NoteField to fade to white
}

void
ScreenGameplay::SendCrossedMessages()
{
	{
		static int iRowLastCrossed = 0;

		float fPositionSeconds = GAMESTATE->m_Position.m_fMusicSeconds;
		float fSongBeat =
		  GAMESTATE->m_pCurSong->m_SongTiming.GetBeatFromElapsedTime(
			fPositionSeconds);

		int iRowNow = BeatToNoteRow(fSongBeat);
		iRowNow = std::max(0, iRowNow);

		for (int r = iRowLastCrossed + 1; r <= iRowNow; r++) {
			if (GetNoteType(r) == NOTE_TYPE_4TH)
				MESSAGEMAN->Broadcast(Message_BeatCrossed);
		}

		iRowLastCrossed = iRowNow;
	}

	{
		const int NUM_MESSAGES_TO_SEND = 4;
		const float MESSAGE_SPACING_SECONDS = 0.4f;

		PlayerNumber pn = PLAYER_INVALID;
		FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi)
		{
			if (GAMESTATE->m_pCurSteps->GetDifficulty() ==
				Difficulty_Beginner) {
				pn = pi->m_pn;
				break;
			}
		}
		if (pn == PLAYER_INVALID)
			return;

		const NoteData& nd = m_vPlayerInfo[pn].m_pPlayer->GetNoteData();

		static int iRowLastCrossedAll[NUM_MESSAGES_TO_SEND] = { 0, 0, 0, 0 };
		for (int i = 0; i < NUM_MESSAGES_TO_SEND; i++) {
			float fNoteWillCrossInSeconds = MESSAGE_SPACING_SECONDS * i;

			float fPositionSeconds =
			  GAMESTATE->m_Position.m_fMusicSeconds + fNoteWillCrossInSeconds;
			float fSongBeat =
			  GAMESTATE->m_pCurSong->m_SongTiming.GetBeatFromElapsedTime(
				fPositionSeconds);

			int iRowNow = BeatToNoteRow(fSongBeat);
			iRowNow = std::max(0, iRowNow);
			int& iRowLastCrossed = iRowLastCrossedAll[i];

			FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(
			  nd, r, iRowLastCrossed + 1, iRowNow + 1)
			{
				int iNumTracksWithTapOrHoldHead = 0;
				for (int t = 0; t < nd.GetNumTracks(); t++) {
					if (nd.GetTapNote(t, r).type == TapNoteType_Empty)
						continue;

					iNumTracksWithTapOrHoldHead++;

					// send crossed message
					if (GAMESTATE->GetCurrentGame()
						  ->m_PlayersHaveSeparateStyles) {
						FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi)
						{
							const Style* pStyle =
							  GAMESTATE->GetCurrentStyle(pi->m_pn);
							RString sButton = pStyle->ColToButtonName(t);
							Message msg(i == 0 ? "NoteCrossed"
											   : "NoteWillCross");
							msg.SetParam("ButtonName", sButton);
							msg.SetParam("NumMessagesFromCrossed", i);
							msg.SetParam("PlayerNumber", pi->m_pn);
							MESSAGEMAN->Broadcast(msg);
						}
					} else {
						const Style* pStyle =
						  GAMESTATE->GetCurrentStyle(PLAYER_INVALID);
						RString sButton = pStyle->ColToButtonName(t);
						Message msg(i == 0 ? "NoteCrossed" : "NoteWillCross");
						msg.SetParam("ButtonName", sButton);
						msg.SetParam("NumMessagesFromCrossed", i);
						MESSAGEMAN->Broadcast(msg);
					}
				}

				if (iNumTracksWithTapOrHoldHead > 0)
					MESSAGEMAN->Broadcast(
					  static_cast<MessageID>(Message_NoteCrossed + i));
				if (i == 0 && iNumTracksWithTapOrHoldHead >= 2) {
					RString sMessageName = "NoteCrossedJump";
					MESSAGEMAN->Broadcast(sMessageName);
				}
			}

			iRowLastCrossed = iRowNow;
		}
	}
}

void
ScreenGameplay::BeginBackingOutFromGameplay()
{
	m_DancingState = STATE_OUTRO;
	ResetGiveUpTimers(false);

	m_pSoundMusic->StopPlaying();
	m_GameplayAssist.StopPlaying(); // Stop any queued assist ticks.
	this->ClearMessageQueue();

	m_Cancel.StartTransitioning(SM_DoPrevScreen);

	GAMEMAN->m_bRestartedGameplay = true;
}

void
ScreenGameplay::AbortGiveUpText(bool show_abort_text)
{
	m_textDebug.StopTweening();
	if (show_abort_text) {
		m_textDebug.SetText(GIVE_UP_ABORTED_TEXT);
	}
	// otherwise tween out the text that's there

	m_textDebug.BeginTweening(1 / 2.f);
	m_textDebug.SetDiffuse(RageColor(1, 1, 1, 0));
}

void
ScreenGameplay::AbortSkipSong(bool show_text)
{
	if (m_SkipSongTimer.IsZero()) {
		return;
	}
	AbortGiveUpText(show_text);
	m_SkipSongTimer.SetZero();
}

void
ScreenGameplay::AbortGiveUp(bool bShowText)
{
	if (m_GiveUpTimer.IsZero()) {
		return;
	}
	AbortGiveUpText(bShowText);
	m_GiveUpTimer.SetZero();
}

void
ScreenGameplay::ResetGiveUpTimers(bool show_text)
{
	AbortSkipSong(show_text);
	AbortGiveUp(show_text);
}

bool
ScreenGameplay::Input(const InputEventPlus& input)
{
	// LOG->Trace( "ScreenGameplay::Input()" );

	Message msg("");
	if (m_Codes.InputMessage(input, msg))
		this->HandleMessage(msg);

	if (m_DancingState != STATE_OUTRO && GAMESTATE->IsHumanPlayer(input.pn) &&
		!m_Cancel.IsTransitioning()) {
		/* Allow bailing out by holding any START button.
		 * This gives a way to "give up" when a back button isn't available.
		 * If this is also a style button, don't do this; pump center is start.
		 */
		bool bHoldingGiveUp = false;
		if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(
			  input.GameI) == Column_Invalid) {
			bHoldingGiveUp |=
			  (START_GIVES_UP && input.MenuI == GAME_BUTTON_START);
			bHoldingGiveUp |=
			  (BACK_GIVES_UP && input.MenuI == GAME_BUTTON_BACK);
		}

		if (bHoldingGiveUp) {
			// No PREFSMAN->m_bDelayedEscape; always delayed.
			if (input.type == IET_RELEASE) {
				AbortGiveUp(true);
			} else if (input.type == IET_FIRST_PRESS &&
					   m_GiveUpTimer.IsZero()) {
				m_textDebug.SetText(GIVE_UP_START_TEXT);
				m_textDebug.PlayCommand("StartOn");
				m_GiveUpTimer.Touch(); // start the timer
			}

			return true;
		}

		/* Only handle GAME_BUTTON_BACK as a regular BACK button if
		 * BACK_GIVES_UP is disabled. */
		bool bHoldingBack = false;
		if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(
			  input.GameI) == Column_Invalid) {
			bHoldingBack |= input.MenuI == GAME_BUTTON_BACK && !BACK_GIVES_UP;
		}

		if (bHoldingBack) {
			if (((!PREFSMAN->m_bDelayedBack && input.type == IET_FIRST_PRESS) ||
				 (input.DeviceI.device == DEVICE_KEYBOARD &&
				  input.type == IET_REPEAT) ||
				 (input.DeviceI.device != DEVICE_KEYBOARD &&
				  INPUTFILTER->GetSecsHeld(input.DeviceI) >= 1.0f))) {
				if (PREFSMAN->m_verbose_log > 1)
					LOG->Trace("Player %i went back", input.pn + 1);
				if (GamePreferences::m_AutoPlay == PC_REPLAY ||
					GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
					  .m_bPractice) {
					if (GAMEMAN->m_bResetModifiers) {
						float oldRate = GAMEMAN->m_fPreviousRate;
						const RString mods = GAMEMAN->m_sModsToReset;
						/*
						GAMESTATE->m_pPlayerState
						  ->m_PlayerOptions.GetSong()
						  .FromString("clearall");
						GAMESTATE->m_pPlayerState
						  ->m_PlayerOptions.GetCurrent()
						  .FromString("clearall");
						GAMESTATE->m_pPlayerState
						  ->m_PlayerOptions.GetPreferred()
						  .FromString("clearall");
						GAMESTATE->m_pPlayerState
						  ->m_PlayerOptions.GetSong()
						  .FromString(mods);
						GAMESTATE->m_pPlayerState
						  ->m_PlayerOptions.GetCurrent()
						  .FromString(mods);
						GAMESTATE->m_pPlayerState
						  ->m_PlayerOptions.GetPreferred()
						  .FromString(mods);
						*/
						const std::vector<RString> oldturns =
						  GAMEMAN->m_vTurnsToReset;
						if (GAMEMAN->m_bResetTurns) {
							GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong()
							  .ResetModsToStringVector(oldturns);
							GAMESTATE->m_pPlayerState->m_PlayerOptions
							  .GetCurrent()
							  .ResetModsToStringVector(oldturns);
							GAMESTATE->m_pPlayerState->m_PlayerOptions
							  .GetPreferred()
							  .ResetModsToStringVector(oldturns);
							GAMEMAN->m_bResetTurns = false;
							GAMEMAN->m_vTurnsToReset.clear();
						}
						GAMESTATE->m_SongOptions.GetSong().m_fMusicRate =
						  oldRate;
						GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate =
						  oldRate;
						GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate =
						  oldRate;
						FailType failreset = GAMEMAN->m_iPreviousFail;
						GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong()
						  .m_FailType = failreset;
						GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
						  .m_FailType = failreset;
						GAMESTATE->m_pPlayerState->m_PlayerOptions
						  .GetPreferred()
						  .m_FailType = failreset;
						GAMEMAN->m_bResetModifiers = false;
						GAMEMAN->m_sModsToReset = "";
						MESSAGEMAN->Broadcast("RateChanged");
					}
					GamePreferences::m_AutoPlay.Set(PC_HUMAN);
				}
				BeginBackingOutFromGameplay();
			} else if (PREFSMAN->m_bDelayedBack &&
					   input.type == IET_FIRST_PRESS) {
				m_textDebug.SetText(GIVE_UP_BACK_TEXT);
				m_textDebug.PlayCommand("BackOn");
			} else if (PREFSMAN->m_bDelayedBack && input.type == IET_RELEASE) {
				m_textDebug.PlayCommand("TweenOff");
			}

			return true;
		}
	}

	bool bRelease = input.type == IET_RELEASE;
	if (!input.GameI.IsValid())
		return false;

	int iCol =
	  GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(input.GameI);

	// Don't pass on any inputs to Player that aren't a press or a release.
	switch (input.type) {
		case IET_FIRST_PRESS:
		case IET_RELEASE:
			break;
		default:
			return false;
	}

	// RestartGameplay may only be pressed when in Singleplayer.
	// RestartGameplay may not be pressed within Replays.
	// Clever theming or something can probably break this, but we should at
	// least try.
	if (SCREENMAN->GetTopScreen()->GetPrevScreen() == "ScreenSelectMusic" &&
		GamePreferences::m_AutoPlay != PC_REPLAY) {
		/* Restart gameplay button moved from theme to allow for rebinding for
		 * people who dont want to edit lua files :)
		 */
		bool bHoldingRestart = false;
		if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(
			  input.GameI) == Column_Invalid) {
			bHoldingRestart |= input.MenuI == GAME_BUTTON_RESTART;
		}
		if (bHoldingRestart) {
			SCREENMAN->GetTopScreen()->SetPrevScreenName(
			  "ScreenStageInformation");
			BeginBackingOutFromGameplay();
		}
	}

	if (GAMESTATE->m_bMultiplayer) {
		if (input.mp != MultiPlayer_Invalid &&
			GAMESTATE->IsMultiPlayerEnabled(input.mp) && iCol != -1) {
			FOREACH(PlayerInfo, m_vPlayerInfo, pi)
			{
				if (input.mp == pi->m_mp)
					pi->m_pPlayer->Step(
					  iCol, -1, input.DeviceI.ts, false, bRelease);
			}
			return true;
		}
	} else {
		// handle a step or battle item activate
		if (GAMESTATE->IsHumanPlayer(input.pn)) {
			ResetGiveUpTimers(true);

			if (GamePreferences::m_AutoPlay == PC_HUMAN &&
				GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
					.m_fPlayerAutoPlay == 0) {
				PlayerInfo& pi = GetPlayerInfoForInput(input);

				ASSERT(input.GameI.IsValid());

				GameButtonType gbt =
				  GAMESTATE->m_pCurGame->GetPerButtonInfo(input.GameI.button)
					->m_gbt;
				switch (gbt) {
					case GameButtonType_Menu:
						return false;
					case GameButtonType_Step:
						if (iCol != -1)
							pi.m_pPlayer->Step(
							  iCol, -1, input.DeviceI.ts, false, bRelease);
						return true;
				}
			}
		}
	}
	return false;
}

/* Saving StageStats that are affected by the note pattern is a little tricky:
 *
 * Stats are cumulative for course play.
 *
 * For regular songs, it doesn't matter how we do it; the pattern doesn't change
 * during play.
 *
 * The pattern changes during play in battle and course mode. We want to include
 * these changes, so run stats for a song after the song finishes.
 *
 * If we fail, be sure to include the current song in stats,
 * with the current modifier set. So:
 * 1. At the end of a song in any mode, pass or fail, add stats for that song
 *    (from m_pPlayer).
 * 2. At the end of gameplay in course mode, add stats for any songs that
 * weren't played, applying the modifiers the song would have been played with.
 *    This doesn't include songs that were played but failed; that was done in
 * #1.
 */
void
ScreenGameplay::SaveStats()
{
	float fMusicLen = GAMESTATE->m_pCurSong->m_fMusicLengthSeconds;

	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		/* Note that adding stats is only meaningful for the counters (eg.
		 * RadarCategory_Jumps), not for the percentages (RadarCategory_Air). */
		RadarValues rv;
		PlayerStageStats& pss = *pi->GetPlayerStageStats();
		const NoteData& nd = pi->m_pPlayer->GetNoteData();
		PlayerNumber pn = pi->m_pn;

		GAMESTATE->SetProcessedTimingData(
		  GAMESTATE->m_pCurSteps->GetTimingData());
		NoteDataUtil::CalculateRadarValues(nd, fMusicLen, rv);
		pss.m_radarPossible += rv;
		NoteDataWithScoring::GetActualRadarValues(nd, pss, fMusicLen, rv);
		pss.m_radarActual += rv;
		GAMESTATE->SetProcessedTimingData(NULL);
	}
	if (GamePreferences::m_AutoPlay.Get() == PC_REPLAY) {
		// We need to replace the newly created replay data with the actual old
		// data Because to keep consistently lazy practices, we can just hack
		// things together instead of fixing the real issue -poco
		// (doing this fixes a lot of issues in the eval screen)
		PlayerStageStats* pss = m_vPlayerInfo[PLAYER_1].GetPlayerStageStats();
		HighScore* hs = PlayerAI::pScoreData;
		pss->m_vHoldReplayData = hs->GetHoldReplayDataVector();
		pss->m_vNoteRowVector = hs->GetNoteRowVector();
		pss->m_vOffsetVector = hs->GetOffsetVector();
		pss->m_vTapNoteTypeVector = hs->GetTapNoteTypeVector();
		pss->m_vTrackVector = hs->GetTrackVector();
	}
}

void
ScreenGameplay::SongFinished()
{

	if (GAMESTATE->m_pCurSteps) {
		GAMESTATE->m_pCurSteps->GetTimingData()->ReleaseLookup();
	}
	AdjustSync::HandleSongEnd();
	SaveStats(); // Let subclasses save the stats.
}

void
ScreenGameplay::StageFinished(bool bBackedOut)
{
	if (bBackedOut) {
		GAMESTATE->CancelStage();
		return;
	}

	// If all players failed, kill.
	if (STATSMAN->m_CurStageStats.AllFailed()) {
		GAMESTATE->m_iPlayerStageTokens = 0;
	}

	STATSMAN->m_CurStageStats.FinalizeScores(false);
	if (GamePreferences::m_AutoPlay == PC_HUMAN &&
		!GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice)
		GAMESTATE->CommitStageStats();
	// save current stage stats
	STATSMAN->m_vPlayedStageStats.push_back(STATSMAN->m_CurStageStats);

	STATSMAN->CalcAccumPlayedStageStats();
	GAMESTATE->FinishStage();
}

void
ScreenGameplay::HandleScreenMessage(const ScreenMessage SM)
{
	CHECKPOINT_M(
	  ssprintf("HandleScreenMessage(%s)",
			   ScreenMessageHelpers::ScreenMessageToString(SM).c_str()));
	if (SM == SM_DoneFadingIn) {
		// If the ready animation is zero length, then playing the sound will
		// make it overlap with the go sound.
		// If the Ready animation is zero length, and the Go animation is not,
		// only play the Go sound.
		// If they're both zero length, only play the Ready sound.
		// Otherwise, play both sounds.
		// -Kyz
		m_Ready.StartTransitioning(SM_PlayGo);
		if (m_Ready.GetTweenTimeLeft() <= .0f) {
			m_delaying_ready_announce = true;
		} else {
			m_delaying_ready_announce = false;
			SOUND->PlayOnceFromAnnouncer("gameplay ready");
		}
	} else if (SM == SM_PlayGo) {
		m_Go.StartTransitioning(SM_None);
		bool should_play_go = true;
		if (m_delaying_ready_announce) {
			if (m_Go.GetTweenTimeLeft() <= .0f) {
				SOUND->PlayOnceFromAnnouncer("gameplay ready");
				should_play_go = false;
			} else {
				should_play_go = true;
			}
		}
		if (should_play_go) {
			SOUND->PlayOnceFromAnnouncer("gameplay here we go normal");
		}

		GAMESTATE->m_DanceStartTime.Touch();

		GAMESTATE->m_bGameplayLeadIn.Set(false);
		m_DancingState =
		  STATE_DANCING; // STATE CHANGE!  Now the user is allowed to press Back
	} else if (SM == SM_NotesEnded) // received while STATE_DANCING
	{
		if(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
		  .m_bPractice)
			return;	// don't auto leave gameplay when finishing notes during practice mode
					// this prevents use of eval screen during practice which im pretty sure nobody cares about?

		ResetGiveUpTimers(
		  false); // don't allow giveup while the next song is loading

		FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
		{
			// Mark failure.
			if (GAMESTATE->GetPlayerFailType(pi->GetPlayerState()) !=
				  FailType_Off &&
				(pi->m_pLifeMeter && pi->m_pLifeMeter->IsFailing())) {
				pi->GetPlayerStageStats()->m_bFailed = true;
				Message msg("SongFinished");
				MESSAGEMAN->Broadcast(msg);
			}

			if (!pi->GetPlayerStageStats()->m_bFailed) {
				pi->GetPlayerStageStats()->m_iSongsPassed++;
			}

			// set a life record at the point of failure
			if (pi->GetPlayerStageStats()->m_bFailed) {
				pi->GetPlayerStageStats()->SetLifeRecordAt(
				  0, STATSMAN->m_CurStageStats.m_fGameplaySeconds);
			}
		}

		/* If all players have *really* failed (bFailed, not the life meter or
		 * bFailedEarlier): */
		const bool bAllReallyFailed = STATSMAN->m_CurStageStats.AllFailed();
		const bool bIsLastSong = m_apSongsQueue.size() == 1;

		LOG->Trace("bAllReallyFailed = %d "
				   "bIsLastSong = %d, m_gave_up = %d, m_skipped_song = %d",
				   bAllReallyFailed,
				   bIsLastSong,
				   m_gave_up,
				   m_skipped_song);

		if (GAMESTATE->IsPlaylistCourse()) {
			m_apSongsQueue.erase(m_apSongsQueue.begin(),
								 m_apSongsQueue.begin() + 1);
			FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
			  pi->m_vpStepsQueue.erase(pi->m_vpStepsQueue.begin(),
									   pi->m_vpStepsQueue.begin() + 1);
			ratesqueue.erase(ratesqueue.begin(), ratesqueue.begin() + 1);

			this->StageFinished(false);
			if (m_apSongsQueue.size() > 0) {
				GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate =
				  ratesqueue[0];
				GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = ratesqueue[0];
				GAMESTATE->m_SongOptions.GetStage().m_fMusicRate =
				  ratesqueue[0];
				GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate =
				  ratesqueue[0];

				STATSMAN->m_CurStageStats.m_player.InternalInit();
			}
			playlistscorekeys.emplace_back(
			  STATSMAN->m_CurStageStats.mostrecentscorekey);
		}

		if (!bIsLastSong && m_skipped_song) {
			// Load the next song in the course.
			HandleScreenMessage(SM_StartLoadingNextSong);
			return;
		}
		if (bAllReallyFailed || bIsLastSong || m_gave_up) {
			// Time to leave from ScreenGameplay
			HandleScreenMessage(SM_LeaveGameplay);
		} else {
			// Load the next song in the course.
			HandleScreenMessage(SM_StartLoadingNextSong);
			return;
		}
	} else if (SM == SM_LeaveGameplay) {
		GAMESTATE->m_DanceDuration = GAMESTATE->m_DanceStartTime.Ago();
		// update dancing characters for win / lose
		DancingCharacters* pDancers = NULL;
		if (m_pSongBackground != nullptr)
			pDancers = m_pSongBackground->GetDancingCharacters();
		if (pDancers != nullptr) {
			FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi)
			{
				// XXX: In battle modes, switch( GAMESTATE->GetStageResult(p) ).
				if (pi->GetPlayerStageStats()->m_bFailed)
					pDancers->Change2DAnimState(pi->m_pn,
												AS2D_FAIL); // fail anim
				else if (pi->m_pLifeMeter &&
						 pi->GetPlayerState()->m_HealthState == HealthState_Hot)
					pDancers->Change2DAnimState(
					  pi->m_pn, AS2D_WINFEVER); // full life pass anim
				else
					pDancers->Change2DAnimState(pi->m_pn,
												AS2D_WIN); // pass anim
			}
		}

		// End round.
		if (m_DancingState == STATE_OUTRO) // ScreenGameplay already ended
			return;						   // ignore
		m_DancingState = STATE_OUTRO;
		ResetGiveUpTimers(false);

		bool bAllReallyFailed = STATSMAN->m_CurStageStats.AllFailed();

		if (bAllReallyFailed) {
			this->PostScreenMessage(SM_BeginFailed, 0);
			return;
		}

		// todo: add GameplayCleared, StartTransitioningCleared commands -aj

		Message msg("SongFinished");
		MESSAGEMAN->Broadcast(msg);

		if (GAMESTATE->IsPlaylistCourse()) {
			SONGMAN->GetPlaylists()[SONGMAN->playlistcourse]
			  .courseruns.emplace_back(playlistscorekeys);
		}

		TweenOffScreen();

		m_Out.StartTransitioning(SM_DoNextScreen);
		SOUND->PlayOnceFromAnnouncer("gameplay cleared");
	} else if (SM == SM_StartLoadingNextSong) {
		// Next song.
		// give a little life back between stages
		FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
		{
			if (pi->m_pLifeMeter)
				pi->m_pLifeMeter->OnSongEnded();
		}

		GAMESTATE->m_bLoadingNextSong = true;
		MESSAGEMAN->Broadcast("BeforeLoadingNextCourseSong");
		m_NextSong.Reset();
		m_NextSong.PlayCommand("Start");
		m_NextSong.StartTransitioning(SM_LoadNextSong);
		MESSAGEMAN->Broadcast("ChangeCourseSongIn");
	} else if (SM == SM_LoadNextSong) {
		m_pSoundMusic->Stop();
		SongFinished();

		MESSAGEMAN->Broadcast("ChangeCourseSongOut");

		GAMESTATE->m_bLoadingNextSong = false;
		LoadNextSong();

		m_NextSong.Reset();
		m_NextSong.PlayCommand("Finish");
		m_NextSong.StartTransitioning(SM_None);

		StartPlayingSong(MIN_SECONDS_TO_STEP_NEXT_SONG, 0);
	} else if (SM == SM_PlayToasty) {
		if (g_bEasterEggs) {
			if (m_Toasty.IsWaiting()) {
				m_Toasty.Reset();
				m_Toasty.StartTransitioning();
			}
		}
	} else if (ScreenMessageHelpers::ScreenMessageToString(SM).find("0Combo") !=
			   std::string::npos) {
		int iCombo;
		RString sCropped =
		  ScreenMessageHelpers::ScreenMessageToString(SM).substr(3);
		sscanf(sCropped.c_str(), "%d%*s", &iCombo);
		PlayAnnouncer(ssprintf("gameplay %d combo", iCombo), 2);
	} else if (SM == SM_ComboStopped) {
		PlayAnnouncer("gameplay combo stopped", 2);
	} else if (SM == SM_ComboContinuing) {
		PlayAnnouncer("gameplay combo overflow", 2);
	} else if (SM >= SM_BattleTrickLevel1 && SM <= SM_BattleTrickLevel3) {
		int iTrickLevel = SM - SM_BattleTrickLevel1 + 1;
		PlayAnnouncer(ssprintf("gameplay battle trick level%d", iTrickLevel),
					  3);
		if (SM == SM_BattleTrickLevel1)
			m_soundBattleTrickLevel1.Play(false);
		else if (SM == SM_BattleTrickLevel2)
			m_soundBattleTrickLevel2.Play(false);
		else if (SM == SM_BattleTrickLevel3)
			m_soundBattleTrickLevel3.Play(false);
	} else if (SM == SM_DoPrevScreen) {
		SongFinished();
		this->StageFinished(true);

		m_sNextScreen = GetPrevScreen();

		if (!GAMESTATE->IsPlaylistCourse() && AdjustSync::IsSyncDataChanged())
			ScreenSaveSync::PromptSaveSync(SM_GoToPrevScreen);
		else
			HandleScreenMessage(SM_GoToPrevScreen);
	} else if (SM == SM_DoNextScreen) {
		SongFinished();
		this->StageFinished(false);
		auto syncing =
		  !GAMESTATE->IsPlaylistCourse() && AdjustSync::IsSyncDataChanged();
		bool replaying = false;
		FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi)
		{
			if (pi->GetPlayerState()->m_PlayerController ==
				PC_REPLAY) // don't duplicate replay saves
			{
				replaying = true;
			}
		}
		// only save replays if the player chose to
		if (GAMESTATE->m_SongOptions.GetCurrent().m_bSaveReplay && !syncing &&
			!replaying)
			SaveReplay();

		if (syncing)
			ScreenSaveSync::PromptSaveSync(SM_GoToPrevScreen);
		else
			HandleScreenMessage(SM_GoToNextScreen);

		if (GAMESTATE->IsPlaylistCourse()) {
			GAMESTATE->isplaylistcourse = false;
			SONGMAN->playlistcourse = "";
		}
	} else if (SM == SM_GainFocus) {
		// We do this ourself.
		SOUND->HandleSongTimer(false);
	} else if (SM == SM_LoseFocus) {
		// We might have turned the song timer off. Be sure to turn it back on.
		SOUND->HandleSongTimer(true);
	} else if (SM == SM_BeginFailed) {
		m_DancingState = STATE_OUTRO;
		ResetGiveUpTimers(false);
		m_GameplayAssist.StopPlaying(); // Stop any queued assist ticks.
		TweenOffScreen();
		m_Failed.StartTransitioning(SM_DoNextScreen);

		SOUND->PlayOnceFromAnnouncer("gameplay failed");
	}

	ScreenWithMenuElements::HandleScreenMessage(SM);
}

void
ScreenGameplay::HandleMessage(const Message& msg)
{
	if (msg == "Judgment") {
		PlayerNumber pn;
		msg.GetParam("Player", pn);

		FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi)
		{
			if (pi->m_pn != pn)
				continue;
			if (!pi->GetPlayerState()
				   ->m_PlayerOptions.GetCurrent()
				   .m_bMuteOnError)
				continue;

			RageSoundReader* pSoundReader = m_AutoKeysounds.GetPlayerSound(pn);
			if (pSoundReader == NULL)
				pSoundReader = m_AutoKeysounds.GetSharedSound();

			HoldNoteScore hns;
			msg.GetParam("HoldNoteScore", hns);
			TapNoteScore tns;
			msg.GetParam("TapNoteScore", tns);

			bool bOn = false;
			if (hns != HoldNoteScore_Invalid)
				bOn = hns != HNS_LetGo;
			else
				bOn = tns != TNS_Miss;

			if (pSoundReader)
				pSoundReader->SetProperty("Volume", bOn ? 1.0f : 0.0f);
		}
	}

	ScreenWithMenuElements::HandleMessage(msg);
}

void
ScreenGameplay::Cancel(ScreenMessage smSendWhenDone)
{
	m_pSoundMusic->Stop();

	ScreenWithMenuElements::Cancel(smSendWhenDone);
}

PlayerInfo*
ScreenGameplay::GetPlayerInfo(PlayerNumber pn)
{
	FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi)
	{
		if (pi->m_pn == pn)
			return &*pi;
	}
	return NULL;
}

PlayerInfo*
ScreenGameplay::GetDummyPlayerInfo(int iDummyIndex)
{
	FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
	{
		if (pi->m_bIsDummy && pi->m_iDummyIndex == iDummyIndex)
			return &*pi;
	}
	return NULL;
}

void
ScreenGameplay::SaveReplay()
{
	/* Replay data TODO:
	 * Add more player information (?)
	 * Add AutoGen flag if steps were autogen?
	 * Add proper steps hash?
	 * Add modifiers used
	 * Add date played, machine played on, etc.
	 * Hash of some stuff to validate data (see Profile)
	 */
	FOREACH_HumanPlayer(pn)
	{
		FOREACH_EnabledPlayerInfo(m_vPlayerInfo, pi)
		{
			Profile* pTempProfile = PROFILEMAN->GetProfile(pn);

			XNode* p = new XNode("ReplayData");
			// append version number (in case the format changes)
			p->AppendAttr("Version", 0);

			// song information node
			SongID songID;
			songID.FromSong(GAMESTATE->m_pCurSong);
			XNode* pSongInfoNode = songID.CreateNode();
			pSongInfoNode->AppendChild(
			  "Title", GAMESTATE->m_pCurSong->GetDisplayFullTitle());
			pSongInfoNode->AppendChild(
			  "Artist", GAMESTATE->m_pCurSong->GetDisplayArtist());
			p->AppendChild(pSongInfoNode);

			// steps information
			StepsID stepsID;
			stepsID.FromSteps(GAMESTATE->m_pCurSteps);
			XNode* pStepsInfoNode = stepsID.CreateNode();
			// hashing = argh
			// pStepsInfoNode->AppendChild("StepsHash",
			// stepsID.ToSteps(GAMESTATE->m_pCurSong,false)->GetHash());
			p->AppendChild(pStepsInfoNode);

			// player information node (rival data sup)
			XNode* pPlayerInfoNode = new XNode("Player");
			pPlayerInfoNode->AppendChild("DisplayName",
										 pTempProfile->m_sDisplayName);
			pPlayerInfoNode->AppendChild("Guid", pTempProfile->m_sGuid);
			p->AppendChild(pPlayerInfoNode);

			// the timings.
			p->AppendChild(pi->m_pPlayer->GetNoteData().CreateNode());

			// Find a file name for the replay
			std::vector<RString> files;
			GetDirListing("Save/Replays/replay*", files, false, false);
			std::sort(files.begin(), files.end());

			// Files should be of the form "replay#####.xml".
			int iIndex = 0;

			for (int i = files.size() - 1; i >= 0; --i) {
				static Regex re("^replay([0-9]{5})\\....$");
				std::vector<RString> matches;
				if (!re.Compare(files[i], matches))
					continue;

				ASSERT(matches.size() == 1);
				iIndex = StringToInt(matches[0]) + 1;
				break;
			}

			RString sFileName = ssprintf("replay%05d.xml", iIndex);

			XmlFileUtil::SaveToFile(p, "Save/Replays/" + sFileName);
			SAFE_DELETE(p);
			return;
		}
	}
}

float
ScreenGameplay::SetRate(float newRate)
{
	float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

	// Rates outside of this range may crash
	if (newRate < 0.3f || newRate > 5.f)
		return rate;

	bool paused = GAMESTATE->GetPaused();

	// Stop the music and generate a new "music"
	m_pSoundMusic->Stop();

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);

	float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
	GetMusicEndTiming(fSecondsToStartFadingOutMusic,
					  fSecondsToStartTransitioningOut);

	RageSoundParams p;
	p.m_StartSecond = fSeconds;
	// Turns out the music rate doesn't affect anything by itself, so we have to
	// set every rate
	p.m_fSpeed = newRate;
	GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = newRate;
	GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = newRate;
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = newRate;
	// Prevent music from making noise when doing things in pause mode
	// Volume gets reset when leaving pause mode or doing almost anything else
	if (paused)
		p.m_Volume = 0.f;
	// Set up the music so we don't wait for an Etternaty when messing around
	// near the end of the song.
	if (fSecondsToStartFadingOutMusic <
		GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
		p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
		p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
							MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
	}
	p.StopMode = RageSoundParams::M_CONTINUE;

	// Go
	m_pSoundMusic->Play(false, &p);
	// But only for like 1 frame if we are paused
	if (paused)
		m_pSoundMusic->Pause(true);

	// misc info update
	GAMESTATE->m_Position.m_fMusicSeconds = fSeconds;
	UpdateSongPosition(0);
	MESSAGEMAN->Broadcast(
	  "CurrentRateChanged"); // Tell the theme we changed the rate

	return newRate;
}

void
ScreenGameplay::SetSongPosition(float newPositionSeconds)
{
	// If you go too far negative, bad things may happen
	// But remember some files have notes at 0.0 seconds
	if (newPositionSeconds <= 0)
		newPositionSeconds = 0.f;
	bool paused = GAMESTATE->GetPaused();

	// Stop the music and generate a new "music"
	m_pSoundMusic->Stop();

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);
	float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

	float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
	GetMusicEndTiming(fSecondsToStartFadingOutMusic,
					  fSecondsToStartTransitioningOut);

	// Set up current rate and new position to play
	RageSoundParams p;
	p.m_StartSecond = newPositionSeconds;
	p.m_fSpeed = rate;
	GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = rate;
	GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = rate;
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = rate;

	// Prevent endless music or something
	if (fSecondsToStartFadingOutMusic <
		GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
		p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
		p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
							MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
	}
	p.StopMode = RageSoundParams::M_CONTINUE;

	// If we scroll backwards, we need to render those notes again
	if (newPositionSeconds < fSeconds) {
		m_vPlayerInfo[PLAYER_1].m_pPlayer->RenderAllNotesIgnoreScores();
	}

	// If we are paused, set the volume to 0 so we don't make weird noises
	if (paused) {
		p.m_Volume = 0.f;
	} else {
		// Restart the file to make sure nothing weird is going on
		ReloadCurrentSong();
		STATSMAN->m_CurStageStats.m_player.InternalInit();
	}

	// Go
	m_pSoundMusic->Play(false, &p);
	// But only for like 1 frame if we are paused
	if (paused)
		m_pSoundMusic->Pause(true);

	// misc info update
	GAMESTATE->m_Position.m_fMusicSeconds = newPositionSeconds;
	UpdateSongPosition(0);
}

void
ScreenGameplay::TogglePracticePause()
{
	// True if we were paused before now
	bool oldPause = GAMESTATE->GetPaused();
	// True if we are becoming paused
	bool newPause = !GAMESTATE->GetPaused();

	if (!GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice)
		return;

	if (oldPause) {
		float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		m_pSoundMusic->Stop();

		RageTimer tm;
		const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);

		float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
		GetMusicEndTiming(fSecondsToStartFadingOutMusic,
						  fSecondsToStartTransitioningOut);

		RageSoundParams p;
		p.m_StartSecond = fSeconds;
		p.m_fSpeed = rate;
		if (fSecondsToStartFadingOutMusic <
			GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
								MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		}
		p.StopMode = RageSoundParams::M_CONTINUE;
		// Go
		m_pSoundMusic->Play(false, &p);
	} else {
		m_pSoundMusic->Pause(newPause);
	}
	GAMESTATE->SetPaused(newPause);
}

void
ScreenGameplay::SetPracticeSongPosition(float newPositionSeconds)
{
	if (!GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice)
		return;

	m_pSoundMusic->SetPositionSeconds(newPositionSeconds);

	bool isPaused = GAMESTATE->GetPaused();
	m_pSoundMusic->Pause(isPaused);

	m_vPlayerInfo[PLAYER_1].m_pPlayer->RenderAllNotesIgnoreScores();
}

float
ScreenGameplay::AddToPracticeRate(float amountAdded)
{
	float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

	if (!GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice)
		return rate;

	float newRate = rate + amountAdded;

	// Rates outside of this range may crash
	// Use 0.25 because of floating point errors...
	if (newRate <= 0.25f || newRate > 3.f)
		return rate;

	bool paused = GAMESTATE->GetPaused();

	m_pSoundMusic->Stop();

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);

	float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
	GetMusicEndTiming(fSecondsToStartFadingOutMusic,
					  fSecondsToStartTransitioningOut);

	RageSoundParams p;
	p.m_StartSecond = fSeconds;
	p.m_fSpeed = newRate;
	GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = newRate;
	GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = newRate;
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = newRate;
	if (paused)
		p.m_Volume = 0.f;
	if (fSecondsToStartFadingOutMusic <
		GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
		p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
		p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
							MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
	}
	p.StopMode = RageSoundParams::M_CONTINUE;
	// Go
	m_pSoundMusic->Play(false, &p);
	// But only for like 1 frame if we are paused
	if (paused)
		m_pSoundMusic->Pause(true);
	GAMESTATE->m_Position.m_fMusicSeconds = fSeconds;

	MESSAGEMAN->Broadcast(
	  "CurrentRateChanged"); // Tell the theme we changed the rate
	return newRate;
}

const float
ScreenGameplay::GetSongPosition()
{
	// Really, this is the music position...
	RageTimer tm;
	return m_pSoundMusic->GetPositionSeconds(NULL, &tm);
}

void
ScreenGameplay::ToggleReplayPause()
{

	// True if we were paused before now
	bool oldPause = GAMESTATE->GetPaused();
	// True if we are becoming paused
	bool newPause = !GAMESTATE->GetPaused();

	// We are leaving pause mode
	if (oldPause) {
		RageTimer tm;

		const float fSeconds = m_pSoundMusic->GetPositionSeconds(NULL, &tm);
		float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

		// Restart the stage, technically (This will cause a lot of lag if there
		// are a lot of notes.)
		ReloadCurrentSong();
		STATSMAN->m_CurStageStats.m_player.InternalInit();
		PlayerAI::SetScoreData(PlayerAI::pScoreData);
		PlayerAI::SetUpExactTapMap(PlayerAI::pReplayTiming);

		// Reset the wife/judge counter related visible stuff
		FOREACH_ENUM(TapNoteScore, tns)
		{
			Message msg = Message("Judgment");
			msg.SetParam("Judgment", tns);
			msg.SetParam("WifePercent", 0);
			msg.SetParam("Player", 0);
			msg.SetParam("TapNoteScore", tns);
			msg.SetParam("FirstTrack", 0);
			msg.SetParam("CurWifeScore", 0);
			msg.SetParam("MaxWifeScore", 0);
			msg.SetParam("WifeDifferential", 0);
			msg.SetParam("TotalPercent", 0);
			msg.SetParam("Type", RString("Tap"));
			msg.SetParam("Val", 0);
			MESSAGEMAN->Broadcast(msg);
		}

		// Set up the stage music to current params, simply
		float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
		GetMusicEndTiming(fSecondsToStartFadingOutMusic,
						  fSecondsToStartTransitioningOut);

		RageSoundParams p;
		p.m_StartSecond = fSeconds;
		p.m_fSpeed = rate;
		if (fSecondsToStartFadingOutMusic <
			GAMESTATE->m_pCurSong->m_fMusicLengthSeconds) {
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
								MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		}
		p.StopMode = RageSoundParams::M_CONTINUE;

		// Unpause
		m_pSoundMusic->Play(false, &p);
		GAMESTATE->m_Position.m_fMusicSeconds = fSeconds;
		UpdateSongPosition(0);
		// SCREENMAN->SystemMessage("Unpaused Replay");
	} else {
		// Almost all of gameplay is based on the music moving.
		// If the music is paused, nothing works.
		// This is all we have to do.
		m_pSoundMusic->Pause(newPause);
		// SCREENMAN->SystemMessage("Paused Replay");
	}
	GAMESTATE->SetPaused(newPause);
}

/*
bool ScreenGameplay::LoadReplay()
{
	// Load replay which was selected via options
}
*/

// lua start

/** @brief Allow Lua to have access to the ScreenGameplay. */
class LunaScreenGameplay : public Luna<ScreenGameplay>
{
  public:
	static int Center1Player(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->Center1Player());
		return 1;
	}
	static int GetLifeMeter(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;

		PlayerInfo* pi = p->GetPlayerInfo(pn);
		if (pi == NULL)
			return 0;
		LifeMeter* pLM = pi->m_pLifeMeter;
		if (pLM == NULL)
			return 0;

		pLM->PushSelf(L);
		return 1;
	}
	static int GetPlayerInfo(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;

		PlayerInfo* pi = p->GetPlayerInfo(pn);
		if (pi == NULL)
			return 0;

		pi->PushSelf(L);
		return 1;
	}
	static int GetDummyPlayerInfo(T* p, lua_State* L)
	{
		int iDummyIndex = IArg(1);
		PlayerInfo* pi = p->GetDummyPlayerInfo(iDummyIndex);
		if (pi == NULL)
			return 0;
		pi->PushSelf(L);
		return 1;
	}
	static bool TurningPointsValid(lua_State* L, int index)
	{
		size_t size = lua_objlen(L, index);
		if (size < 2) {
			luaL_error(L, "Invalid number of entries %zu", size);
		}
		float prev_turning = -1;
		for (size_t n = 1; n < size; ++n) {
			lua_pushnumber(L, n);
			lua_gettable(L, index);
			float v = FArg(-1);
			if (v < prev_turning || v > 1) {
				luaL_error(L, "Invalid value %f", v);
			}
			lua_pop(L, 1);
		}
		return true;
	}
	static bool AddAmountsValid(lua_State* L, int index)
	{
		return TurningPointsValid(L, index);
	}
	static int begin_backing_out(T* p, lua_State* L)
	{
		p->BeginBackingOutFromGameplay();
		COMMON_RETURN_SELF;
	}
	static int GetTrueBPS(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;
		float rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		float bps = GAMESTATE->m_pPlayerState->m_Position.m_fCurBPS;
		float true_bps = rate * bps;
		lua_pushnumber(L, true_bps);
		return 1;
	}
	static int GetSongPosition(T* p, lua_State* L)
	{
		float pos = p->GetSongPosition();
		lua_pushnumber(L, pos);
		return 1;
	}
	static int SetReplayPosition(T* p, lua_State* L)
	{
		float newpos = FArg(1);
		if (GAMESTATE->GetPaused() &&
			GamePreferences::m_AutoPlay == PC_REPLAY) {
			p->SetSongPosition(newpos);
		}
		/*
		else
			SCREENMAN->SystemMessage(
			  "You must be paused to move the song position of a Replay.");
		*/
		return 0;
	}
	static int SetReplayRate(T* p, lua_State* L)
	{
		float newrate = FArg(1);
		if (!GAMESTATE->GetPaused()) {
			/*
			SCREENMAN->SystemMessage(
			  "You must be paused to change the rate of a Replay.");
			*/
			lua_pushnumber(L, -1.f);
			return 1;
		}
		if (GamePreferences::m_AutoPlay != PC_REPLAY) {
			/*
			SCREENMAN->SystemMessage(
			  "You cannot change the rate outside of a Replay.");
			*/
			lua_pushnumber(L, -1.f);
			return 1;
		}
		lua_pushnumber(L, p->SetRate(newrate));
		return 1;
	}
	static int ToggleReplayPause(T* p, lua_State* L)
	{
		if (GamePreferences::m_AutoPlay != PC_REPLAY) {
			/*
			SCREENMAN->SystemMessage(
			  "You cannot pause the game outside of a Replay.");
			*/
			return 0;
		}
		p->ToggleReplayPause();
		return 0;
	}
	static int SetReplayBookmark(T* p, lua_State* L)
	{
		float position = FArg(1);
		if (GamePreferences::m_AutoPlay == PC_REPLAY) {
			p->m_fReplayBookmarkSeconds = position;
			return 0;
		}
		return 0;
	}
	static int JumpToReplayBookmark(T* p, lua_State* L)
	{
		if (GamePreferences::m_AutoPlay == PC_REPLAY &&
			GAMESTATE->GetPaused()) {
			p->SetSongPosition(p->m_fReplayBookmarkSeconds);
			return 0;
		}
		return 0;
	}

	static int SetPreviewNoteFieldMusicPosition(T* p, lua_State* L)
	{
		float given = FArg(1);
		p->SetPracticeSongPosition(given);
		return 0;
	}

	static int AddToPracticeRate(T* p, lua_State* L)
	{
		float rate = FArg(1);
		lua_pushnumber(L, p->AddToPracticeRate(rate));
		return 1;
	}

	static int TogglePracticePause(T* p, lua_State* L)
	{
		p->TogglePracticePause();
		return 0;
	}

	LunaScreenGameplay()
	{
		ADD_METHOD(Center1Player);
		ADD_METHOD(GetLifeMeter);
		ADD_METHOD(GetPlayerInfo);
		ADD_METHOD(GetDummyPlayerInfo);
		// sm-ssc additions:
		ADD_METHOD(begin_backing_out);
		ADD_METHOD(GetTrueBPS);

		ADD_METHOD(GetSongPosition);
		ADD_METHOD(SetReplayPosition);
		ADD_METHOD(SetReplayRate);
		ADD_METHOD(ToggleReplayPause);
		ADD_METHOD(SetReplayBookmark);
		ADD_METHOD(JumpToReplayBookmark);
		ADD_METHOD(SetPreviewNoteFieldMusicPosition);
		ADD_METHOD(AddToPracticeRate);
		ADD_METHOD(TogglePracticePause);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenGameplay, ScreenWithMenuElements)

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
		int iIndex = IArg(1);
		iIndex %= p->m_vpStepsQueue.size();
		Steps* pSteps = p->m_vpStepsQueue[iIndex];
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
// lua end

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
