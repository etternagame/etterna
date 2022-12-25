#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/AdjustSync.h"
#include "Etterna/Actor/Gameplay/ArrowEffects.h"
#include "Etterna/Actor/Gameplay/Background.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Actor/Gameplay/Foreground.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Actor/Gameplay/LifeMeter.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Models/Misc/LyricsLoader.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"
#include "Etterna/Models/NoteData/NoteDataWithScoring.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Models/Misc/Profile.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Sound/RageSoundReader.h"
#include "RageUtil/Misc/RageTimer.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "ScreenGameplay.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Screen/Others/ScreenSaveSync.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Actor/GameplayAndMenus/StepsDisplay.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Models/Misc/PlayerInfo.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Singletons/ReplayManager.h"

#include <algorithm>

#include "Core/Platform/Platform.hpp"

#define SONG_POSITION_METER_WIDTH                                              \
	THEME->GetMetricF(m_sName, "SongPositionMeterWidth")

static ThemeMetric<float> INITIAL_BACKGROUND_BRIGHTNESS(
  "ScreenGameplay",
  "InitialBackgroundBrightness");
static ThemeMetric<float> SECONDS_BETWEEN_COMMENTS("ScreenGameplay",
												   "SecondsBetweenComments");

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

static Preference<bool> g_bCenter1Player("Center1Player", true);
static Preference<bool> g_bShowLyrics("ShowLyrics", false);
static std::map<int, std::set<DeviceButton>> g_buttonsByColumnPressed{};

ScreenGameplay::ScreenGameplay()
{
	m_pSongBackground = nullptr;
	m_pSongForeground = nullptr;
	m_delaying_ready_announce = false;
	g_buttonsByColumnPressed.clear();

	// Tell DownloadManager we are in Gameplay
	DLMAN->UpdateGameplayState(true);

	// Unload all Replay Data to prevent some things (if not replaying)
	if (GamePreferences::m_AutoPlay != PC_REPLAY) {
		Locator::getLogger()->info("Freeing loaded replay data");
		SCOREMAN->UnloadAllReplayData();
	}

	SONGMAN->UnloadAllCalcDebugOutput();

	m_DancingState = STATE_INTRO;
	m_fTimeSinceLastDancingComment = 0.F;
	m_bShowScoreboard = false;
	m_gave_up = false;
	m_bZeroDeltaOnNextUpdate = false;
	m_pSoundMusic = nullptr;
}

void
ScreenGameplay::Init()
{
	SubscribeToMessage("Judgment");

	// Load some stuff from metrics ... for now.
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

	if (UseSongBackgroundAndForeground()) {
		m_pSongBackground = new Background;
		m_pSongForeground = new Foreground;
	}

	ScreenWithMenuElements::Init();

	// Tells the screen what player we are using
	// specifically Normal, Practice, or Replay
	this->FillPlayerInfo(&m_vPlayerInfo);

	m_pSoundMusic = nullptr;

	// Prevent some crashes
	// This happens when the screen changes but we dont have a song (obviously)
	if (GAMESTATE->m_pCurSong == nullptr) {
		return;
	}

	/* Called once per stage (single song or single course). */
	GAMESTATE->BeginStage();

	// Starting gameplay, make sure Gamestate doesn't think we are paused
	// because we ... don't start paused.
	GAMESTATE->SetPaused(false);

	// Make sure we have NoteData to play
	const unsigned int count = m_vPlayerInfo.m_vpStepsQueue.size();
	for (unsigned int i = 0; i < count; i++) {
		auto* curSteps = m_vPlayerInfo.m_vpStepsQueue[i];
		if (curSteps->IsNoteDataEmpty()) {
			if (curSteps->GetNoteDataFromSimfile()) {
				Locator::getLogger()->debug("Notes should be loaded for player 1");
			} else {
				Locator::getLogger()->error("Error loading notes for player 1");
			}
		}
	}

	ASSERT(GAMESTATE->m_pCurSteps.Get() != nullptr);

	STATSMAN->m_CurStageStats.m_player.m_pStyle =
	  GAMESTATE->GetCurrentStyle(PLAYER_1);

	/* Record combo rollover. */
	m_vPlayerInfo.GetPlayerStageStats()->UpdateComboList(0, true);

	m_DancingState = STATE_INTRO;

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

	// //
	// Start a bunch of stuff to make sure the Notefield is placed correctly
	//

	// Use the margin function to calculate where the notefields should be and
	// what size to zoom them to.  This way, themes get margins to put cut-ins
	// in, and the engine can have players on different styles without the
	// notefields overlapping. -Kyz
	LuaReference margarine;
	std::array<float, 2> margins = { 40, 40 };
	THEME->GetMetric(m_sName, "MarginFunction", margarine);
	if (margarine.GetLuaType() != LUA_TFUNCTION) {
		LuaHelpers::ReportScriptErrorFmt(
		  "MarginFunction metric for %s must be a function.", m_sName.c_str());
	} else {
		auto* L = LUA->Get();
		margarine.PushSelf(L);
		lua_createtable(L, 0, 0);
		const auto next_player_slot = 1;
		Enum::Push(L, PLAYER_1);
		lua_rawseti(L, -2, next_player_slot);
		Enum::Push(L, GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StyleType);
		std::string err = "Error running MarginFunction:  ";
		if (LuaHelpers::RunScriptOnStack(L, err, 2, 3, true)) {
			const std::string marge = "Margin value must be a number.";
			margins[0] = static_cast<float>(SafeFArg(L, -3, marge, 40));
			const auto center = static_cast<float>(SafeFArg(L, -2, marge, 80));
			margins[1] = center / 2.0F;
		}
		lua_settop(L, 0);
		LUA->Release(L);
	}

	const auto left_edge = 0.0F;
	const std::string sName("PlayerP1");
	m_vPlayerInfo.m_pPlayer->SetName(sName);
	const auto edge = left_edge;
	const auto screen_space = SCREEN_WIDTH / 2.0F;
	const auto left_marge = margins[0];
	const auto right_marge = margins[1];
	const auto field_space = screen_space - left_marge - right_marge;
	const auto player_x = edge + left_marge + (field_space / 2.0F);
	m_vPlayerInfo.GetPlayerState()->m_NotefieldZoom = 1.F;

	if (!Center1Player()) {
		m_vPlayerInfo.m_pPlayer->SetX(player_x);
	} else {
		m_vPlayerInfo.m_pPlayer->SetX(SCREEN_CENTER_X);
	}
	m_vPlayerInfo.m_pPlayer->RunCommands(PLAYER_INIT_COMMAND);
	// ActorUtil::LoadAllCommands(m_vPlayerInfo.m_pPlayer, m_sName);
	this->AddChild(m_vPlayerInfo.m_pPlayer);
	m_vPlayerInfo.m_pPlayer->PlayCommand("On");

	//
	//
	// //

	m_NextSong.Load(THEME->GetPathB(m_sName, "next course song"));
	m_NextSong.SetDrawOrder(DRAW_ORDER_TRANSITIONS - 1);
	this->AddChild(&m_NextSong);

	// Multiplayer-specific gameplay check
	if (GAMESTATE->m_bPlayingMulti) {
		NSMAN->StartRequest(0);
	}

	// Add individual life meter, when not in sync mode
	if (m_sName != "ScreenGameplaySyncMachine") {
		m_vPlayerInfo.m_pLifeMeter =
		  LifeMeter::MakeLifeMeter(m_vPlayerInfo.GetPlayerState()
									 ->m_PlayerOptions.GetStage()
									 .m_LifeType);
		m_vPlayerInfo.m_pLifeMeter->Load(m_vPlayerInfo.GetPlayerState(),
										 m_vPlayerInfo.GetPlayerStageStats());
		m_vPlayerInfo.m_pLifeMeter->SetName(
		  ssprintf("Life%s", m_vPlayerInfo.GetName().c_str()));
		LOAD_ALL_COMMANDS_AND_SET_XY(m_vPlayerInfo.m_pLifeMeter);
		this->AddChild(m_vPlayerInfo.m_pLifeMeter);
	}

	// For multi scoreboard; may be used in the future
	m_bShowScoreboard = false;

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

	if (m_pSongBackground != nullptr) {
		m_pSongBackground->Init();
	}

	std::string sType = PLAYER_TYPE;
	if (m_vPlayerInfo.m_bIsDummy) {
		sType += "Dummy";
	}
	m_vPlayerInfo.m_pPlayer->Init(sType,
								  m_vPlayerInfo.GetPlayerState(),
								  m_vPlayerInfo.GetPlayerStageStats(),
								  m_vPlayerInfo.m_pLifeMeter,
								  m_vPlayerInfo.m_pPrimaryScoreKeeper);

	// fill in m_apSongsQueue, m_vpStepsQueue, m_asModifiersQueue
	InitSongQueues();

	// Fill StageStats
	STATSMAN->m_CurStageStats.m_vpPossibleSongs = m_apSongsQueue;
	if (m_vPlayerInfo.GetPlayerStageStats() != nullptr) {
		m_vPlayerInfo.GetPlayerStageStats()->m_vpPossibleSteps =
		  m_vPlayerInfo.m_vpStepsQueue;
	}

	ASSERT(!m_vPlayerInfo.m_vpStepsQueue.empty());
	if (m_vPlayerInfo.GetPlayerStageStats() != nullptr) {
		m_vPlayerInfo.GetPlayerStageStats()->m_bJoined = true;
	}
	LoadScoreKeeper();

	GAMESTATE->m_bGameplayLeadIn.Set(true);

	/* LoadNextSong first, since that positions some elements which need to be
	 * positioned before we TweenOnScreen. */
	LoadNextSong();

	m_GiveUpTimer.SetZero();
	m_gave_up = false;
	GAMESTATE->m_bRestartedGameplay = false;
}

auto
ScreenGameplay::Center1Player() -> bool
{
	return g_bCenter1Player;
}

// fill in m_apSongsQueue, m_vpStepsQueue, m_asModifiersQueue
void
ScreenGameplay::InitSongQueues()
{
	m_apSongsQueue.push_back(GAMESTATE->m_pCurSong);
	Steps* pSteps = GAMESTATE->m_pCurSteps;
	m_vPlayerInfo.m_vpStepsQueue.push_back(pSteps);

	if (GAMESTATE->IsPlaylistCourse()) {

		m_apSongsQueue.clear();
		m_vPlayerInfo.m_vpStepsQueue.clear();

		auto& pl = SongManager::GetPlaylists()[SONGMAN->playlistcourse];
		for (auto& ch : pl.chartlist) {
			m_apSongsQueue.emplace_back(ch.songptr);
			m_vPlayerInfo.m_vpStepsQueue.emplace_back(ch.stepsptr);
			ratesqueue.emplace_back(ch.rate);
		}
	}
}

ScreenGameplay::~ScreenGameplay()
{
	if (this->IsFirstUpdate()) {
		/* We never received any updates. That means we were deleted without
		 * being used, and never actually played. (This can happen when backing
		 * out of ScreenStage.) Cancel the stage. */
		GAMESTATE->CancelStage();
	}

	Locator::getLogger()->debug("ScreenGameplay::~ScreenGameplay()");

	SAFE_DELETE(m_pSongBackground);
	SAFE_DELETE(m_pSongForeground);

	if (m_pSoundMusic != nullptr) {
		m_pSoundMusic->StopPlaying();
	}

	m_GameplayAssist.StopPlaying();

	// If we didn't just restart gameplay...
	if (!GAMESTATE->m_bRestartedGameplay) {
		// Tell Multiplayer we ended the song
		if (GAMESTATE->m_bPlayingMulti) {
			NSMAN->ReportSongOver();
		}

		// Tell DownloadManager we aren't in Gameplay
		DLMAN->UpdateGameplayState(false);

		GAMESTATE->m_gameplayMode.Set(GameplayMode_Normal);
		GAMESTATE->TogglePracticeMode(false);
	}

	// Always unpause when exiting gameplay (or restarting)
	GAMESTATE->SetPaused(false);
}

void
ScreenGameplay::SetupNoteDataFromRow(Steps* pSteps, int row, int maxrow)
{
	NoteData originalNoteData;
	pSteps->GetNoteData(originalNoteData);

	const auto* pStyle = GAMESTATE->GetCurrentStyle(m_vPlayerInfo.m_pn);
	NoteData ndTransformed;
	pStyle->GetTransformedNoteDataForStyle(
	  m_vPlayerInfo.GetStepsAndTrailIndex(), originalNoteData, ndTransformed);

	m_vPlayerInfo.GetPlayerState()->Update(0);

	NoteDataUtil::RemoveAllButRange(ndTransformed, row, maxrow);

	// load player
	{
		m_vPlayerInfo.m_NoteData = ndTransformed;
		NoteDataUtil::RemoveAllTapsOfType(m_vPlayerInfo.m_NoteData,
										  TapNoteType_AutoKeysound);
		ReloadPlayer();
	}

	// load auto keysounds
	{
		auto nd = ndTransformed;
		NoteDataUtil::RemoveAllTapsExceptForType(nd, TapNoteType_AutoKeysound);
		m_AutoKeysounds.Load(m_vPlayerInfo.GetStepsAndTrailIndex(), nd);
	}

	{
		std::string sType;
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

		m_vPlayerInfo.m_SoundEffectControl.Load(
		  sType, m_vPlayerInfo.GetPlayerState(), &m_vPlayerInfo.m_NoteData);
	}
}

void
ScreenGameplay::ReloadPlayer()
{
	m_vPlayerInfo.m_pPlayer->Reload();
}

void
ScreenGameplay::LoadPlayer()
{
	m_vPlayerInfo.m_pPlayer->Load();
}

void
ScreenGameplay::LoadScoreKeeper()
{
	if (m_vPlayerInfo.m_pPrimaryScoreKeeper != nullptr) {
		m_vPlayerInfo.m_pPrimaryScoreKeeper->Load(m_apSongsQueue,
												  m_vPlayerInfo.m_vpStepsQueue);
	}
}

void
ScreenGameplay::SetupSong(int iSongIndex)
{
	/* This is the first beat that can be changed without it being visible.
	 * Until we draw for the first time, any beat can be changed. */
	m_vPlayerInfo.GetPlayerState()->m_fLastDrawnBeat = -100;

	auto* pSteps = m_vPlayerInfo.m_vpStepsQueue[iSongIndex];
	GAMESTATE->m_pCurSteps.Set(pSteps);

	NoteData originalNoteData;
	pSteps->GetNoteData(originalNoteData);

	const auto* pStyle = GAMESTATE->GetCurrentStyle(m_vPlayerInfo.m_pn);
	NoteData ndTransformed;
	pStyle->GetTransformedNoteDataForStyle(
	  m_vPlayerInfo.GetStepsAndTrailIndex(), originalNoteData, ndTransformed);

	m_vPlayerInfo.GetPlayerState()->Update(0);

	// load player
	{
		m_vPlayerInfo.m_NoteData = ndTransformed;
		NoteDataUtil::RemoveAllTapsOfType(m_vPlayerInfo.m_NoteData,
										  TapNoteType_AutoKeysound);
		LoadPlayer();
	}

	// load auto keysounds
	{
		auto nd = ndTransformed;
		NoteDataUtil::RemoveAllTapsExceptForType(nd, TapNoteType_AutoKeysound);
		m_AutoKeysounds.Load(m_vPlayerInfo.GetStepsAndTrailIndex(), nd);
	}

	{
		std::string sType;
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

		m_vPlayerInfo.m_SoundEffectControl.Load(
		  sType, m_vPlayerInfo.GetPlayerState(), &m_vPlayerInfo.m_NoteData);
	}

	m_vPlayerInfo.GetPlayerState()->Update(0);

	// Hack: Course modifiers that are set to start immediately shouldn't
	// tween on.
	m_vPlayerInfo.GetPlayerState()->m_PlayerOptions.SetCurrentToLevel(
	  ModsLevel_Stage);
}

void
ScreenGameplay::ReloadCurrentSong()
{
	m_vPlayerInfo.GetPlayerStageStats()->m_iSongsPlayed--;

	LoadNextSong();
}

void
ScreenGameplay::LoadNextSong()
{
	// never allow input to remain redirected during gameplay unless an lua
	// script forces it when loaded below -mina
	SCREENMAN->set_input_redirected(m_vPlayerInfo.m_pn, false);

	// Time begins now (so each individual song can be counted)
	m_initTimer.Touch();

	GAMESTATE->ResetMusicStatistics();

	m_vPlayerInfo.GetPlayerStageStats()->m_iSongsPlayed++;

	auto iPlaySongIndex = GAMESTATE->GetCourseSongIndex();
	iPlaySongIndex %= m_apSongsQueue.size();
	GAMESTATE->m_pCurSong.Set(m_apSongsQueue[iPlaySongIndex]);
	// Check if the music actually exists, this is to avoid an issue in
	// AutoKeysounds.cpp, where the reader will ignore whether the file opener
	// function actually returned a valid object or an error. - Terra
	GAMESTATE->m_pCurSong.Get()->ReloadIfNoMusic();

	STATSMAN->m_CurStageStats.m_vpPlayedSongs.push_back(GAMESTATE->m_pCurSong);

	// apply permamirror
	if (GamePreferences::m_AutoPlay != PC_REPLAY &&
		GAMESTATE->m_pPlayerState->m_PlayerController != PC_REPLAY) {
		auto& pmc = PROFILEMAN->GetProfile(PLAYER_1)->PermaMirrorCharts;
		auto* pSteps = m_vPlayerInfo.m_vpStepsQueue[iPlaySongIndex];
		if (pSteps != nullptr && pmc.count(pSteps->GetChartKey())) {
			// apply mirror to only stage so it turns on temporarily
			PO_GROUP_ASSIGN_N(GAMESTATE->m_pPlayerState->m_PlayerOptions,
							  ModsLevel_Stage,
							  m_bTurns,
							  PlayerOptions::TURN_MIRROR,
							  true);
		}
	}

	SetupSong(iPlaySongIndex);

	Steps* pSteps = GAMESTATE->m_pCurSteps;
	++m_vPlayerInfo.GetPlayerStageStats()->m_iStepsPlayed;

	ASSERT(GAMESTATE->m_pCurSteps != nullptr);
	if (m_vPlayerInfo.m_ptextStepsDescription != nullptr) {
		m_vPlayerInfo.m_ptextStepsDescription->SetText(
		  pSteps->GetDescription());
	}

	if (m_vPlayerInfo.m_ptextPlayerOptions != nullptr) {
		m_vPlayerInfo.m_ptextPlayerOptions->SetText(
		  m_vPlayerInfo.GetPlayerState()
			->m_PlayerOptions.GetCurrent()
			.GetString());
	}

	if (m_vPlayerInfo.m_pStepsDisplay != nullptr) {
		m_vPlayerInfo.m_pStepsDisplay->SetFromSteps(pSteps);
	}

	/* The actual note data for scoring is the base class of Player.  This
	 * includes transforms, like Wide.  Otherwise, the scoring will operate
	 * on the wrong data. */
	if (m_vPlayerInfo.m_pPrimaryScoreKeeper != nullptr) {
		m_vPlayerInfo.m_pPrimaryScoreKeeper->OnNextSong(
		  GAMESTATE->GetCourseSongIndex(),
		  pSteps,
		  &m_vPlayerInfo.m_pPlayer->GetNoteData());
	}

	// Don't mess with the PlayerController of the Dummy player
	if (!m_vPlayerInfo.m_bIsDummy) {
		if (GAMESTATE->IsCpuPlayer(m_vPlayerInfo.GetStepsAndTrailIndex())) {
			m_vPlayerInfo.GetPlayerState()->m_PlayerController = PC_CPU;
			const auto iMeter = pSteps->GetMeter();
			auto iNewSkill = SCALE(iMeter, MIN_METER, MAX_METER, 0, 5);
			/* Watch out: songs aren't actually bound by MAX_METER. */
			iNewSkill = std::clamp(iNewSkill, 0, 5);
			m_vPlayerInfo.GetPlayerState()->m_iCpuSkill = iNewSkill;
		} else {
			if (m_vPlayerInfo.GetPlayerState()
				  ->m_PlayerOptions.GetCurrent()
				  .m_fPlayerAutoPlay != 0) {
				m_vPlayerInfo.GetPlayerState()->m_PlayerController =
				  PC_AUTOPLAY;
			} else {
				m_vPlayerInfo.GetPlayerState()->m_PlayerController =
				  GamePreferences::m_AutoPlay;
			}
		}
	}

	const auto using_reverse =
	  m_vPlayerInfo.GetPlayerState()
		->m_PlayerOptions.GetCurrent()
		.m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1;

	if (m_vPlayerInfo.m_pStepsDisplay != nullptr) {
		m_vPlayerInfo.m_pStepsDisplay->PlayCommand(
		  using_reverse ? "SetReverse" : "SetNoReverse");
	}

	m_LyricDisplay.PlayCommand(using_reverse ? "SetReverse" : "SetNoReverse");

	// Load lyrics
	// XXX: don't load this here (who and why? -aj)
	LyricsLoader LL;
	if (GAMESTATE->m_pCurSong->HasLyrics()) {
		LL.LoadFromLRCFile(GAMESTATE->m_pCurSong->GetLyricsPath(),
						   *GAMESTATE->m_pCurSong);
	}

	// Set up song-specific graphics.

	if (m_pSongBackground != nullptr) {
		m_pSongBackground->Unload();
	}

	if (m_pSongForeground != nullptr) {
		m_pSongForeground->Unload();
	}

	if (m_pSongBackground != nullptr) {
		m_pSongBackground->LoadFromSong(GAMESTATE->m_pCurSong);
	}

	if (m_pSongBackground != nullptr) {
		m_pSongBackground->SetBrightness(INITIAL_BACKGROUND_BRIGHTNESS);
		m_pSongBackground->FadeToActualBrightness();
	}

	if (!m_vPlayerInfo.GetPlayerStageStats()->m_bFailed) {
		// give a little life back between stages
		if (m_vPlayerInfo.m_pLifeMeter != nullptr) {
			m_vPlayerInfo.m_pLifeMeter->OnLoadSong();
		}
	}

	if (m_pSongForeground != nullptr) {
		m_pSongForeground->LoadFromSong(GAMESTATE->m_pCurSong);
	}

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
	auto* pPlayerSound = m_AutoKeysounds.GetPlayerSound(m_vPlayerInfo.m_pn);
	if (pPlayerSound == nullptr &&
		m_vPlayerInfo.m_pn == GAMESTATE->GetMasterPlayerNumber()) {
		pPlayerSound = m_AutoKeysounds.GetSharedSound();
	}
	m_vPlayerInfo.m_SoundEffectControl.SetSoundReader(pPlayerSound);

	if (!GAMESTATE->GetPaused()) {
		MESSAGEMAN->Broadcast("DoneLoadingNextSong");
	}
}

void
ScreenGameplay::StartPlayingSong(float fMinTimeToNotes, float fMinTimeToMusic)
{
	ASSERT(fMinTimeToNotes >= 0);
	ASSERT(fMinTimeToMusic >= 0);

	RageSoundParams p;
	p.m_fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	p.StopMode = RageSoundParams::M_CONTINUE;
	p.m_bAccurateSync = true;

	{
		const auto fFirstSecond = GAMESTATE->m_pCurSong->GetFirstSecond();
		auto fStartDelay = fMinTimeToNotes - fFirstSecond;
		fStartDelay = std::max(fStartDelay, fMinTimeToMusic);
		p.m_StartSecond = -fStartDelay * p.m_fSpeed;
	}

	ASSERT(!m_pSoundMusic->IsPlaying());
	{
		float fSecondsToStartFadingOutMusic;
		float fSecondsToStartTransitioningOut;
		GetMusicEndTiming(fSecondsToStartFadingOutMusic,
						  fSecondsToStartTransitioningOut);

		if (fSecondsToStartFadingOutMusic <
			GAMESTATE->m_pCurSteps->lastsecond) {
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
								MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		}
	}
	m_pSoundMusic->Play(false, &p);

	/* Make sure GAMESTATE->m_fMusicSeconds is set up. */
	GAMESTATE->m_Position.m_fMusicSeconds = -5000;
	UpdateSongPosition();

	ASSERT(GAMESTATE->m_Position.m_fMusicSeconds >
		   -4000); /* make sure the "fake timer" code doesn't trigger */

	if (GAMESTATE->m_pCurSteps != nullptr) {
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
	auto& player = *m_vPlayerInfo.m_pPlayer;
	const auto& nd = player.GetNoteData();
	m_GameplayAssist.PlayTicks(nd, player.GetPlayerState());
}

/* Play announcer "type" if it's been at least fSeconds since the last
 * announcer. */
void
ScreenGameplay::PlayAnnouncer(const std::string& type,
							  float fSeconds,
							  float* fDeltaSeconds)
{
	/* Don't play before the first beat, or after we're finished. */
	if (m_DancingState != STATE_DANCING) {
		return;
	}
	if (GAMESTATE->m_pCurSong ==
		  nullptr || // this will be true on ScreenDemonstration sometimes
		GAMESTATE->m_Position.m_fSongBeat <
		  GAMESTATE->m_pCurSong->GetFirstBeat()) {
		return;
	}

	if (*fDeltaSeconds < fSeconds) {
		return;
	}
	*fDeltaSeconds = 0;

	SOUND->PlayOnceFromAnnouncer(type);
}

void
ScreenGameplay::UpdateSongPosition()
{
	if (!m_pSoundMusic->IsPlaying()) {
		return;
	}

	RageTimer tm = RageZeroTimer;
	const auto fSeconds = m_pSoundMusic->GetPositionSeconds(nullptr, &tm);
	GAMESTATE->UpdateSongPosition(
	  fSeconds, GAMESTATE->m_pCurSong->m_SongTiming, tm);
}

void
ScreenGameplay::BeginScreen()
{
	if (GAMESTATE->m_pCurSong == nullptr) {
		return;
	}

	ScreenWithMenuElements::BeginScreen();

	SOUND->PlayOnceFromAnnouncer("gameplay intro"); // crowd cheer

	// Tell multi to do its thing (this really does nothing right now) -poco
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

			  auto doot = ssprintf("%d I %d I %d I %d I %d I %d  x%d",
								   ptns[TNS_W1],
								   ptns[TNS_W2],
								   ptns[TNS_W3],
								   ptns[TNS_W4],
								   ptns[TNS_W5],
								   ptns[TNS_Miss],
								   this->GetPlayerInfo(PLAYER_1)
									 ->GetPlayerStageStats()
									 ->m_iCurCombo);
			  auto* player = this->GetPlayerInfo(PLAYER_1)->m_pPlayer;
			  if (player->maxwifescore > 0) {
				  NSMAN->SendMPLeaderboardUpdate(
					player->curwifescore / player->maxwifescore, doot);
			  }
		  },
		  0.25F,
		  -1);
	}
}

auto
ScreenGameplay::AllAreFailing() -> bool
{
	return !((m_vPlayerInfo.m_pLifeMeter != nullptr) &&
			 !m_vPlayerInfo.m_pLifeMeter->IsFailing());
}

void
ScreenGameplay::GetMusicEndTiming(float& fSecondsToStartFadingOutMusic,
								  float& fSecondsToStartTransitioningOut)
{
	auto fLastStepSeconds = GAMESTATE->m_pCurSteps->lastsecond;
	fLastStepSeconds += Player::GetMaxStepDistanceSeconds();

	const float fTransitionLength = OUT_TRANSITION_LENGTH;

	fSecondsToStartTransitioningOut = fLastStepSeconds;

	// Align the end of the music fade to the end of the transition.
	const auto fSecondsToFinishFadingOutMusic =
	  fSecondsToStartTransitioningOut + fTransitionLength;
	if (fSecondsToFinishFadingOutMusic <
		GAMESTATE->m_pCurSteps->GetLengthSeconds()) {
		fSecondsToStartFadingOutMusic =
		  fSecondsToFinishFadingOutMusic - MUSIC_FADE_OUT_SECONDS;
	} else {
		fSecondsToStartFadingOutMusic =
		  GAMESTATE->m_pCurSteps->GetLengthSeconds(); // don't fade
	}

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
	if (GAMESTATE->m_pCurSong == nullptr) {
		/* ScreenDemonstration will move us to the next screen.  We just need to
		 * survive for one update without crashing.  We need to call
		 * Screen::Update to make sure we receive the next-screen message. */
		ScreenWithMenuElements::Update(fDeltaTime);
		return;
	}

	UpdateSongPosition();

	if (m_bZeroDeltaOnNextUpdate) {
		ScreenWithMenuElements::Update(0);
		m_bZeroDeltaOnNextUpdate = false;
	} else {
		ScreenWithMenuElements::Update(fDeltaTime);
	}

	/* This can happen if ScreenDemonstration::HandleScreenMessage sets a new
	 * screen when !PREFSMAN->m_bDelayedScreenLoad.  (The new screen was loaded
	 * when we called Screen::Update, and the ctor might set a new
	 * GAMESTATE->m_pCurSong, so the above check can fail.) */
	if (SCREENMAN->GetTopScreen() != this) {
		return;
	}

	// LOG->Trace( "m_fOffsetInBeats = %f, m_fBeatsPerSecond = %f,
	// m_Music.GetPositionSeconds = %f", m_fOffsetInBeats, m_fBeatsPerSecond,
	// m_Music.GetPositionSeconds() );

	m_AutoKeysounds.Update(fDeltaTime);

	const auto failtype =
	  GAMESTATE->GetPlayerFailType(m_vPlayerInfo.GetPlayerState());
	// update GameState HealthState
	auto& hs = m_vPlayerInfo.GetPlayerState()->m_HealthState;
	const auto OldHealthState = hs;
	if (m_vPlayerInfo.m_pLifeMeter != nullptr) {
		if (failtype != FailType_Off &&
			m_vPlayerInfo.m_pLifeMeter->IsFailing()) {
			hs = HealthState_Dead;
		} else if (m_vPlayerInfo.m_pLifeMeter->IsHot()) {
			hs = HealthState_Hot;
		} else if (failtype != FailType_Off &&
				   m_vPlayerInfo.m_pLifeMeter->IsInDanger()) {
			hs = HealthState_Danger;
		} else {
			hs = HealthState_Alive;
		}

		if (hs != OldHealthState) {
			Message msg("HealthStateChanged");
			msg.SetParam("PlayerNumber", m_vPlayerInfo.m_pn);
			msg.SetParam("HealthState", hs);
			msg.SetParam("OldHealthState", OldHealthState);
			MESSAGEMAN->Broadcast(msg);
		}
	}

	m_vPlayerInfo.m_SoundEffectControl.Update(fDeltaTime);

	{
		const auto fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		auto p = m_pSoundMusic->GetParams();
		if (std::fabs(p.m_fSpeed - fSpeed) > 0.01F && fSpeed >= 0.0F) {
			p.m_fSpeed = fSpeed;
			m_pSoundMusic->SetParams(p);
		}
	}

	switch (m_DancingState) {
		case STATE_DANCING: {
			/* Set STATSMAN->m_CurStageStats.bFailed for failed players.  In,
			 * FAIL_IMMEDIATE, send SM_BeginFailed if all players failed, and
			 * kill dead Oni players. */
			const auto pn = m_vPlayerInfo.GetStepsAndTrailIndex();

			if (m_vPlayerInfo.m_pLifeMeter != nullptr) {
				const auto lt = m_vPlayerInfo.GetPlayerState()
								  ->m_PlayerOptions.GetStage()
								  .m_LifeType;

				// check for individual fail
				if (!(failtype == FailType_Off) &&
					m_vPlayerInfo.m_pLifeMeter->IsFailing() &&
					!m_vPlayerInfo.GetPlayerStageStats()->m_bFailed) {

					Locator::getLogger()->info("Player {} failed", static_cast<int>(pn));
					m_vPlayerInfo.GetPlayerStageStats()->m_bFailed =
					  true; // fail

					{
						Message msg("PlayerFailed");
						msg.SetParam("PlayerNumber", m_vPlayerInfo.m_pn);
						MESSAGEMAN->Broadcast(msg);
					}
				}

				// Check for and do Oni die.
				auto bAllowOniDie = false;
				switch (lt) {
					case LifeType_Battery:
						bAllowOniDie = true;
						break;
					default:
						break;
				}
				if (bAllowOniDie && failtype == FailType_Immediate) {
					if (!STATSMAN->m_CurStageStats
						   .Failed()) // if not the last one to fail
					{
						// kill them!
						FailFadeRemovePlayer(&m_vPlayerInfo);
					}
				}

				auto failed = true;
				switch (failtype) {
					case FailType_Immediate:
						if (!m_vPlayerInfo.m_pLifeMeter->IsFailing()) {
							failed = false;
						}
						break;
					case FailType_ImmediateContinue: // fail at end
					case FailType_Off:
						failed = false; // never fail.
						break;
					default:
						FAIL_M("Invalid fail type! Aborting...");
				}

				if (failed) {
					m_pSoundMusic->StopPlaying();
					SCREENMAN->PostMessageToTopScreen(SM_NotesEnded, 0);
					m_LyricDisplay.Stop();
				}
			}

			// Check for end of song
			{
				float fSecondsToStartFadingOutMusic;
				float fSecondsToStartTransitioningOut;
				GetMusicEndTiming(fSecondsToStartFadingOutMusic,
								  fSecondsToStartTransitioningOut);

				const auto bAllReallyFailed =
				  STATSMAN->m_CurStageStats.Failed();
				if (bAllReallyFailed) {
					fSecondsToStartTransitioningOut += BEGIN_FAILED_DELAY;
				}

				// fire NoteEnded a bit after the last note.
				// this is to deal with possibly missing the last note
				//  that would ruin an FC
				//  otherwise this fires before the last note is judged
				//   granting a fake FC (or more)
				//  (HACK?)
				if (GAMESTATE->m_Position.m_fMusicSeconds >=
					  fSecondsToStartTransitioningOut + Player::GetMaxStepDistanceSeconds() &&
					!m_NextSong.IsTransitioning()) {
					this->PostScreenMessage(SM_NotesEnded, 0);
				}
			}

			const auto bGiveUpTimerFired =
			  !m_GiveUpTimer.IsZero() && m_GiveUpTimer.Ago() > GIVE_UP_SECONDS;
			m_gave_up = bGiveUpTimerFired;

			// Quitters deserve a failed score.
			if (bGiveUpTimerFired) {
				STATSMAN->m_CurStageStats.m_bGaveUp = true;
				m_vPlayerInfo.GetPlayerStageStats()->m_bDisqualified = true;
				m_vPlayerInfo.GetPlayerStageStats()->gaveuplikeadumbass = true;
				ResetGiveUpTimers(false);
				Locator::getLogger()->info("Exited Gameplay to Evaluation");
				this->PostScreenMessage(SM_LeaveGameplay, 0);
				return;
			}

			// Check to see if it's time to play a ScreenGameplay comment
			m_fTimeSinceLastDancingComment += fDeltaTime;

			if (GAMESTATE->OneIsHot()) {
				PlayAnnouncer("gameplay comment hot", SECONDS_BETWEEN_COMMENTS);
			} else if (GAMESTATE->AllAreInDangerOrWorse()) {
				PlayAnnouncer("gameplay comment danger",
							  SECONDS_BETWEEN_COMMENTS);
			} else {
				PlayAnnouncer("gameplay comment good",
							  SECONDS_BETWEEN_COMMENTS);
			}
		}
		default:
			break;
	}

	PlayTicks();
	SendCrossedMessages();

	/*
	// Multiplayer Life & C++ Scoreboard Update Stuff. Useless for now.
	if (GAMESTATE->m_bPlayingMulti && NSMAN->useSMserver) {
		if (m_vPlayerInfo.m_pLifeMeter)
			NSMAN->m_playerLife =
			  int(m_vPlayerInfo.m_pLifeMeter->GetLife() * 10000);

		if (m_bShowScoreboard)
			FOREACH_NSScoreBoardColumn(
			  cn) if (m_bShowScoreboard && NSMAN->ChangedScoreboard(cn) &&
					  GAMESTATE->GetFirstDisabledPlayer() != PLAYER_INVALID)
			  m_Scoreboard[cn]
				.SetText(NSMAN->m_Scoreboard[cn]);
	}
	*/

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
	m_vPlayerInfo.m_pPlayer->DrawNoteFieldBoard();
	ScreenWithMenuElements::DrawPrimitives();
}

void
ScreenGameplay::FailFadeRemovePlayer(PlayerInfo* pi)
{
	SOUND->PlayOnceFromDir(THEME->GetPathS(m_sName, "oni die"));
	const auto tracks = pi->m_NoteData.GetNumTracks();
	pi->m_NoteData.Init();				 // remove all notes and scoring
	pi->m_NoteData.SetNumTracks(tracks); // reset the number of tracks.
	pi->m_pPlayer->FadeToFail();		 // tell the NoteField to fade to white
}

void
ScreenGameplay::SendCrossedMessages()
{
	// hmmm...
	if (GAMESTATE->m_pCurSong == nullptr) {
		return;
	}

	{
		static auto iRowLastCrossed = 0;

		const auto fPositionSeconds = GAMESTATE->m_Position.m_fMusicSeconds;
		const auto fSongBeat =
		  GAMESTATE->m_pCurSong->m_SongTiming.GetBeatFromElapsedTime(
			fPositionSeconds);

		auto iRowNow = BeatToNoteRow(fSongBeat);
		iRowNow = std::max(0, iRowNow);

		for (auto r = iRowLastCrossed + 1; r <= iRowNow; r++) {
			if (GetNoteType(r) == NOTE_TYPE_4TH) {
				MESSAGEMAN->Broadcast(Message_BeatCrossed);
			}
		}

		iRowLastCrossed = iRowNow;
	}

	{
		const auto NUM_MESSAGES_TO_SEND = 4;
		const auto MESSAGE_SPACING_SECONDS = 0.4F;

		auto pn = PLAYER_INVALID;
		if (GAMESTATE->m_pCurSteps->GetDifficulty() == Difficulty_Beginner) {
			pn = m_vPlayerInfo.m_pn;
		}

		if (pn == PLAYER_INVALID) {
			return;
		}

		const auto& nd = m_vPlayerInfo.m_pPlayer->GetNoteData();

		static int iRowLastCrossedAll[NUM_MESSAGES_TO_SEND] = { 0, 0, 0, 0 };
		for (auto i = 0; i < NUM_MESSAGES_TO_SEND; i++) {
			const auto fNoteWillCrossInSeconds = MESSAGE_SPACING_SECONDS * i;

			const auto fPositionSeconds =
			  GAMESTATE->m_Position.m_fMusicSeconds + fNoteWillCrossInSeconds;
			const auto fSongBeat =
			  GAMESTATE->m_pCurSong->m_SongTiming.GetBeatFromElapsedTime(
				fPositionSeconds);

			auto iRowNow = BeatToNoteRow(fSongBeat);
			iRowNow = std::max(0, iRowNow);
			auto& iRowLastCrossed = iRowLastCrossedAll[i];

			FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(
			  nd, r, iRowLastCrossed + 1, iRowNow + 1)
			{
				auto iNumTracksWithTapOrHoldHead = 0;
				for (auto t = 0; t < nd.GetNumTracks(); t++) {
					if (nd.GetTapNote(t, r).type == TapNoteType_Empty) {
						continue;
					}

					iNumTracksWithTapOrHoldHead++;

					// send crossed message
					if (GAMESTATE->GetCurrentGame()
						  ->m_PlayersHaveSeparateStyles) {
						const auto* pStyle =
						  GAMESTATE->GetCurrentStyle(m_vPlayerInfo.m_pn);
						auto sButton = pStyle->ColToButtonName(t);
						Message msg(i == 0 ? "NoteCrossed" : "NoteWillCross");
						msg.SetParam("ButtonName", sButton);
						msg.SetParam("NumMessagesFromCrossed", i);
						msg.SetParam("PlayerNumber", m_vPlayerInfo.m_pn);
						MESSAGEMAN->Broadcast(msg);
					} else {
						const auto* pStyle =
						  GAMESTATE->GetCurrentStyle(PLAYER_INVALID);
						auto sButton = pStyle->ColToButtonName(t);
						Message msg(i == 0 ? "NoteCrossed" : "NoteWillCross");
						msg.SetParam("ButtonName", sButton);
						msg.SetParam("NumMessagesFromCrossed", i);
						MESSAGEMAN->Broadcast(msg);
					}
				}

				if (iNumTracksWithTapOrHoldHead > 0) {
					MESSAGEMAN->Broadcast(
					  static_cast<MessageID>(Message_NoteCrossed + i));
				}
				if (i == 0 && iNumTracksWithTapOrHoldHead >= 2) {
					std::string sMessageName = "NoteCrossedJump";
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
}

void
ScreenGameplay::RestartGameplay()
{
	GAMESTATE->m_bRestartedGameplay = true;
	if (m_sName.find("Net") != std::string::npos) {
		SetPrevScreenName("ScreenNetStageInformation");
	} else {
		SetPrevScreenName("ScreenStageInformation");
	}
	BeginBackingOutFromGameplay();
}

void
ScreenGameplay::AbortGiveUpText(bool show_abort_text)
{
	m_textDebug.StopTweening();
	if (show_abort_text) {
		m_textDebug.SetText(GIVE_UP_ABORTED_TEXT);
	}
	// otherwise tween out the text that's there

	m_textDebug.BeginTweening(1 / 2.F);
	m_textDebug.SetDiffuse(RageColor(1, 1, 1, 0));
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
	AbortGiveUp(show_text);
}

auto
ScreenGameplay::Input(const InputEventPlus& input) -> bool
{
	// LOG->Trace( "ScreenGameplay::Input()" );

	Message msg("");
	if (m_Codes.InputMessage(input, msg)) {
		this->HandleMessage(msg);
	}

	if (m_DancingState != STATE_OUTRO && GAMESTATE->IsHumanPlayer(input.pn) &&
		!m_Cancel.IsTransitioning()) {
		/* Allow bailing out by holding any START button. */
		auto bHoldingGiveUp = false;
		if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(
			  input.GameI) == Column_Invalid) {
			bHoldingGiveUp |= (input.MenuI == GAME_BUTTON_START);
		}

		// Exiting gameplay by holding Start (Forced Fail)
		// Never allow holding start in Practice Mode
		if (bHoldingGiveUp && !GAMESTATE->IsPracticeMode() &&
			PREFSMAN->m_AllowStartToGiveUp) {
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

		// Exiting gameplay by pressing Back (Immediate Exit)
		auto bHoldingBack = false;
		if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(
			  input.GameI) == Column_Invalid) {
			bHoldingBack |= input.MenuI == GAME_BUTTON_BACK;
		}

		if (bHoldingBack) {
			if (((!PREFSMAN->m_bDelayedBack && input.type == IET_FIRST_PRESS) ||
				 (input.DeviceI.device == DEVICE_KEYBOARD &&
				  input.type == IET_REPEAT) ||
				 (input.DeviceI.device != DEVICE_KEYBOARD &&
				  INPUTFILTER->GetSecsHeld(input.DeviceI) >= 1.0F))) {
				Locator::getLogger()->info("Player {} went back", input.pn + 1);
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

	const auto bRelease = input.type == IET_RELEASE;
	if (!input.GameI.IsValid()) {
		return false;
	}

	/* Restart gameplay button moved from theme to allow for rebinding for
	 * people who dont want to edit lua files :)
	 */
	auto bHoldingRestart = false;
	if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(input.GameI) ==
		Column_Invalid) {
		bHoldingRestart |= input.MenuI == GAME_BUTTON_RESTART;
	}
	if (bHoldingRestart && (m_DancingState != STATE_OUTRO || AllAreFailing())) {
		// delayedback pref will work, or if it's off just go immediately
		// but also just let it be instant if you failed
		if ((PREFSMAN->m_bDelayedBack &&
			 INPUTFILTER->GetSecsHeld(input.DeviceI) >= 1.0F) ||
			!PREFSMAN->m_bDelayedBack || AllAreFailing())
			RestartGameplay();
	}

	// Don't pass on any inputs to Player that aren't a press or a release.
	switch (input.type) {
		case IET_FIRST_PRESS:
		case IET_RELEASE:
			break;
		default:
			return false;
	}

	const auto iCol =
	  GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(input.GameI);

	// handle a step or battle item activate
	if (GAMESTATE->IsHumanPlayer(input.pn)) {
		ResetGiveUpTimers(true);

		if (GamePreferences::m_AutoPlay == PC_HUMAN &&
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
				.m_fPlayerAutoPlay == 0) {

			ASSERT(input.GameI.IsValid());

			const auto gbt =
			  GAMESTATE->m_pCurGame->GetPerButtonInfo(input.GameI.button)
				->m_gbt;
			switch (gbt) {
				case GameButtonType_Menu:
					return false;
				case GameButtonType_Step:
					if (iCol != -1) {

						if (g_buttonsByColumnPressed.count(iCol) == 0u) {
							std::set<DeviceButton> newset;
							g_buttonsByColumnPressed[iCol] = newset;
						}
						g_buttonsByColumnPressed[iCol].emplace(
						  input.DeviceI.button);

						m_vPlayerInfo.m_pPlayer->Step(
						  iCol, -1, input.DeviceI.ts, false, bRelease);
					}
					return true;
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
	/* Note that adding stats is only meaningful for the counters (eg.
	 * RadarCategory_Jumps), not for the percentages (RadarCategory_Air). */
	RadarValues rv;
	auto& pss = *m_vPlayerInfo.GetPlayerStageStats();
	const auto& nd = m_vPlayerInfo.m_pPlayer->GetNoteData();

	GAMESTATE->SetProcessedTimingData(GAMESTATE->m_pCurSteps->GetTimingData());
	NoteDataUtil::CalculateRadarValues(nd, rv);
	pss.m_radarPossible += rv;
	NoteDataWithScoring::GetActualRadarValues(nd, pss, rv);
	pss.m_radarActual += rv;
	GAMESTATE->SetProcessedTimingData(nullptr);
}

void
ScreenGameplay::SongFinished()
{

	if (GAMESTATE->m_pCurSteps != nullptr) {
		GAMESTATE->m_pCurSteps->GetTimingData()->ReleaseLookup();
	}
	SaveStats(); // Let subclasses save the stats.
}

void
ScreenGameplay::StageFinished(bool bBackedOut)
{
	Locator::getLogger()->info("Finishing Stage");
	if (bBackedOut) {
		GAMESTATE->CancelStage();
		return;
	}

	// If all players failed, kill.
	if (STATSMAN->m_CurStageStats.Failed()) {
		GAMESTATE->m_iPlayerStageTokens = 0;
	}

	// How long did this Gameplay session last?
	auto tDiff = m_initTimer.GetDeltaTime();
	STATSMAN->m_CurStageStats.m_player.m_fPlayedSeconds = tDiff;

	// Properly set the LivePlay bool
	STATSMAN->m_CurStageStats.m_bLivePlay = true;

	bool usedDoubleSetup = false;
	for (auto& s : g_buttonsByColumnPressed) {
		if (s.second.size() > 1) {
			usedDoubleSetup = true;
			Locator::getLogger()->info("Double setup detected");
		}
	}

	STATSMAN->m_CurStageStats.m_player.usedDoubleSetup = usedDoubleSetup;
	STATSMAN->m_CurStageStats.FinalizeScores();

	// If we didn't cheat and aren't in Practice
	// (Replay does its own thing somewhere else here)
	if (GamePreferences::m_AutoPlay == PC_HUMAN &&
		!GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice) {
		auto* pHS = &STATSMAN->m_CurStageStats.m_player.m_HighScore;

		// Load the replay data for the current score so some cool functionality
		// works immediately
		REPLAYS->InitReplayPlaybackForScore(pHS);
		GAMESTATE->CommitStageStats();
	}

	// save current stage stats
	STATSMAN->m_vPlayedStageStats.push_back(STATSMAN->m_CurStageStats);

	STATSMAN->CalcAccumPlayedStageStats();
	GAMESTATE->FinishStage();
	Locator::getLogger()->info("Done Finishing Stage");
}

void
ScreenGameplay::HandleScreenMessage(const ScreenMessage& SM)
{
	Locator::getLogger()->trace("HandleScreenMessage({})",
			   ScreenMessageHelpers::ScreenMessageToString(SM).c_str());
	if (SM == SM_DoneFadingIn) {
		// If the ready animation is zero length, then playing the sound will
		// make it overlap with the go sound.
		// If the Ready animation is zero length, and the Go animation is not,
		// only play the Go sound.
		// If they're both zero length, only play the Ready sound.
		// Otherwise, play both sounds.
		// -Kyz
		m_Ready.StartTransitioning(SM_PlayGo);
		if (m_Ready.GetTweenTimeLeft() <= .0F) {
			m_delaying_ready_announce = true;
		} else {
			m_delaying_ready_announce = false;
			SOUND->PlayOnceFromAnnouncer("gameplay ready");
		}
	} else if (SM == SM_PlayGo) {
		m_Go.StartTransitioning(SM_None);
		auto should_play_go = true;
		if (m_delaying_ready_announce) {
			if (m_Go.GetTweenTimeLeft() <= .0F) {
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
		if (GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
			  .m_bPractice) {
			return; // don't auto leave gameplay when finishing notes during
		}
		// practice mode this prevents use of eval screen during
		// practice which im pretty sure nobody cares about?

		ResetGiveUpTimers(
		  false); // don't allow giveup while the next song is loading

		// Mark failure.
		if (GAMESTATE->GetPlayerFailType(m_vPlayerInfo.GetPlayerState()) !=
			  FailType_Off &&
			((m_vPlayerInfo.m_pLifeMeter != nullptr) &&
			 m_vPlayerInfo.m_pLifeMeter->IsFailing())) {
			m_vPlayerInfo.GetPlayerStageStats()->m_bFailed = true;
			Message msg("SongFinished");
			MESSAGEMAN->Broadcast(msg);
		}

		if (!m_vPlayerInfo.GetPlayerStageStats()->m_bFailed) {
			m_vPlayerInfo.GetPlayerStageStats()->m_iSongsPassed++;
		}

		// set a life record at the point of failure
		if (m_vPlayerInfo.GetPlayerStageStats()->m_bFailed) {
			m_vPlayerInfo.GetPlayerStageStats()->SetLifeRecordAt(
			  0.F, GAMESTATE->m_Position.m_fMusicSeconds);
		}

		/* If all players have *really* failed (bFailed, not the life meter or
		 * bFailedEarlier): */
		const auto bAllReallyFailed = STATSMAN->m_CurStageStats.Failed();
		const auto bIsLastSong = m_apSongsQueue.size() == 1;

		Locator::getLogger()->info("bAllReallyFailed = {} bIsLastSong = {}, m_gave_up = {}",
				   bAllReallyFailed,
				   bIsLastSong,
				   m_gave_up);

		if (GAMESTATE->IsPlaylistCourse()) {
			m_apSongsQueue.erase(m_apSongsQueue.begin(),
								 m_apSongsQueue.begin() + 1);
			m_vPlayerInfo.m_vpStepsQueue.erase(
			  m_vPlayerInfo.m_vpStepsQueue.begin(),
			  m_vPlayerInfo.m_vpStepsQueue.begin() + 1);
			ratesqueue.erase(ratesqueue.begin(), ratesqueue.begin() + 1);

			this->StageFinished(false);
			if (!m_apSongsQueue.empty()) {
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

		if (!bIsLastSong) {
			// Load the next song in the course.
			HandleScreenMessage(SM_StartLoadingNextSong);
			return;
		}

		// Time to leave from ScreenGameplay
		HandleScreenMessage(SM_LeaveGameplay);
	} else if (SM == SM_LeaveGameplay) {
		GAMESTATE->m_DanceDuration = GAMESTATE->m_DanceStartTime.Ago();

		// End round.
		if (m_DancingState == STATE_OUTRO) { // ScreenGameplay already ended
			return;							 // ignore
		}
		m_DancingState = STATE_OUTRO;
		ResetGiveUpTimers(false);

		const auto bAllReallyFailed = STATSMAN->m_CurStageStats.Failed();

		if (bAllReallyFailed) {
			this->PostScreenMessage(SM_BeginFailed, 0);
			return;
		}

		// todo: add GameplayCleared, StartTransitioningCleared commands -aj

		Message msg("SongFinished");
		MESSAGEMAN->Broadcast(msg);

		if (GAMESTATE->IsPlaylistCourse()) {
			SongManager::GetPlaylists()[SONGMAN->playlistcourse]
			  .courseruns.emplace_back(playlistscorekeys);
		}

		TweenOffScreen();

		m_Out.StartTransitioning(SM_DoNextScreen);
		SOUND->PlayOnceFromAnnouncer("gameplay cleared");
	} else if (SM == SM_StartLoadingNextSong) {
		// Next song.
		// give a little life back between stages
		if (m_vPlayerInfo.m_pLifeMeter != nullptr) {
			m_vPlayerInfo.m_pLifeMeter->OnSongEnded();
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
		if (PREFSMAN->m_bEasterEggs) {
			if (m_Toasty.IsWaiting() || PREFSMAN->m_AllowMultipleToasties) {
				m_Toasty.Reset();
				m_Toasty.StartTransitioning();
			}
		}
	} else if (ScreenMessageHelpers::ScreenMessageToString(SM).find("0Combo") !=
			   string::npos) {
		int iCombo;
		const auto sCropped =
		  ScreenMessageHelpers::ScreenMessageToString(SM).substr(3);
		sscanf(sCropped.c_str(), "%d%*s", &iCombo);
		PlayAnnouncer(ssprintf("gameplay %d combo", iCombo), 2);
	} else if (SM == SM_ComboStopped) {
		PlayAnnouncer("gameplay combo stopped", 2);
	} else if (SM == SM_ComboContinuing) {
		PlayAnnouncer("gameplay combo overflow", 2);
	} else if (SM == SM_DoPrevScreen) {
		SongFinished();
		this->StageFinished(true);

		m_sNextScreen = GetPrevScreen();

		if (!GAMESTATE->IsPlaylistCourse() && AdjustSync::IsSyncDataChanged()) {
			ScreenSaveSync::PromptSaveSync(SM_GoToPrevScreen);
		} else {
			HandleScreenMessage(SM_GoToPrevScreen);
		}
	} else if (SM == SM_DoNextScreen) {
		SongFinished();

		// Don't save here for Playlists
		// SM_NotesEnded handles all saving for that case (always saves at end of song)
		if (!GAMESTATE->IsPlaylistCourse())
			this->StageFinished(false);

		const auto syncing =
		  !GAMESTATE->IsPlaylistCourse() && AdjustSync::IsSyncDataChanged();

		if (syncing) {
			ScreenSaveSync::PromptSaveSync(SM_GoToPrevScreen);
		} else {
			HandleScreenMessage(SM_GoToNextScreen);
		}

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

		if (m_vPlayerInfo.m_pn == pn && m_vPlayerInfo.GetPlayerState()
										  ->m_PlayerOptions.GetCurrent()
										  .m_bMuteOnError) {

			auto* pSoundReader = m_AutoKeysounds.GetPlayerSound(pn);
			if (pSoundReader == nullptr) {
				pSoundReader = m_AutoKeysounds.GetSharedSound();
			}

			HoldNoteScore hns;
			msg.GetParam("HoldNoteScore", hns);
			TapNoteScore tns;
			msg.GetParam("TapNoteScore", tns);

			bool bOn;
			if (hns != HoldNoteScore_Invalid) {
				bOn = hns != HNS_LetGo;
			} else {
				bOn = tns != TNS_Miss;
			}

			if (pSoundReader != nullptr) {
				pSoundReader->SetProperty("Volume", bOn ? 1.0F : 0.0F);
			}
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

auto
ScreenGameplay::GetPlayerInfo(PlayerNumber pn) -> PlayerInfo*
{
	if (m_vPlayerInfo.m_pn == pn) {
		return &m_vPlayerInfo;
	}
	return nullptr;
}

auto
ScreenGameplay::GetSongPosition() -> const float
{
	// Really, this is the music position...
	RageTimer tm;
	return m_pSoundMusic->GetPositionSeconds(nullptr, &tm);
}

// lua start

/** @brief Allow Lua to have access to the ScreenGameplay. */
class LunaScreenGameplay : public Luna<ScreenGameplay>
{
  public:
	static auto Center1Player(T* p, lua_State* L) -> int
	{
		lua_pushboolean(L, static_cast<int>(p->Center1Player()));
		return 1;
	}
	static auto GetLifeMeter(T* p, lua_State* L) -> int
	{
		const auto pn = PLAYER_1;

		auto* pi = p->GetPlayerInfo(pn);
		if (pi == nullptr) {
			return 0;
		}
		auto* pLM = pi->m_pLifeMeter;
		if (pLM == nullptr) {
			return 0;
		}

		pLM->PushSelf(L);
		return 1;
	}
	static auto GetPlayerInfo(T* p, lua_State* L) -> int
	{
		const auto pn = PLAYER_1;

		auto* pi = p->GetPlayerInfo(pn);
		if (pi == nullptr) {
			return 0;
		}

		pi->PushSelf(L);
		return 1;
	}
	static auto TurningPointsValid(lua_State* L, int index) -> bool
	{
		const auto size = lua_objlen(L, index);
		if (size < 2) {
			luaL_error(L, "Invalid number of entries %zu", size);
		}
		const float prev_turning = -1;
		for (size_t n = 1; n < size; ++n) {
			lua_pushnumber(L, n);
			lua_gettable(L, index);
			const auto v = FArg(-1);
			if (v < prev_turning || v > 1) {
				luaL_error(L, "Invalid value %f", v);
			}
			lua_pop(L, 1);
		}
		return true;
	}
	static auto AddAmountsValid(lua_State* L, int index) -> bool
	{
		return TurningPointsValid(L, index);
	}
	static auto begin_backing_out(T* p, lua_State* L) -> int
	{
		p->BeginBackingOutFromGameplay();
		COMMON_RETURN_SELF;
	}
	static auto GetTrueBPS(T* /*p*/, lua_State* L) -> int
	{
		const auto rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		const auto bps = GAMESTATE->m_Position.m_fCurBPS;
		const auto true_bps = rate * bps;
		lua_pushnumber(L, true_bps);
		return 1;
	}
	static auto GetSongPosition(T* p, lua_State* L) -> int
	{
		const auto pos = p->GetSongPosition();
		lua_pushnumber(L, pos);
		return 1;
	}

	LunaScreenGameplay()
	{
		ADD_METHOD(Center1Player);
		ADD_METHOD(GetLifeMeter);
		ADD_METHOD(GetPlayerInfo);
		// sm-ssc additions:
		ADD_METHOD(begin_backing_out);
		ADD_METHOD(GetTrueBPS);

		ADD_METHOD(GetSongPosition);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenGameplay, ScreenWithMenuElements)

// lua end
