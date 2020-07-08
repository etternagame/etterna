#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/AdjustSync.h"
#include "Etterna/Actor/Gameplay/ArrowEffects.h"
#include "Etterna/Actor/Gameplay/Background.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Models/Misc/Foreach.h"
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
#include "Etterna/Models/Misc/PlayerAI.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Models/Misc/Profile.h"
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

#include <algorithm>
#include <Tracy.hpp>

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

ScreenGameplay::ScreenGameplay()
{
	TracyMessageC(GAMESTATE->m_pCurSong->GetMainTitle().c_str(),
				  GAMESTATE->m_pCurSong->GetMainTitle().length(),
				  0xAF0000);
	m_pSongBackground = nullptr;
	m_pSongForeground = nullptr;
	m_delaying_ready_announce = false;

	// Tell DownloadManager we are in Gameplay
	DLMAN->UpdateDLSpeed(true);

	// Unload all Replay Data to prevent some things (if not replaying)
	if (GamePreferences::m_AutoPlay != PC_REPLAY) {
		LOG->Trace("Unloading excess data.");
		SCOREMAN->UnloadAllReplayData();
	}

	m_DancingState = STATE_INTRO;
	m_fTimeSinceLastDancingComment = 0.f;
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
	if (GAMESTATE->m_pCurSong == nullptr)
		return;

	/* Called once per stage (single song or single course). */
	GAMESTATE->BeginStage();

	// Starting gameplay, make sure Gamestate doesn't think we are paused
	// because we ... don't start paused.
	GAMESTATE->SetPaused(false);

	// Make sure we have NoteData to play
	unsigned int count = m_vPlayerInfo.m_vpStepsQueue.size();
	for (unsigned int i = 0; i < count; i++) {
		Steps* curSteps = m_vPlayerInfo.m_vpStepsQueue[i];
		if (curSteps->IsNoteDataEmpty()) {
			if (curSteps->GetNoteDataFromSimfile()) {
				LOG->Trace("Notes should be loaded for player 1");
			} else {
				LOG->Trace("Error loading notes for player 1");
			}
		}
	}

	ASSERT(GAMESTATE->m_pCurSteps.Get() != NULL);

	// Doesn't technically do anything for now
	// Since playmodes/courses are gone
	STATSMAN->m_CurStageStats.m_playMode = GAMESTATE->m_PlayMode;
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
		std::string err = "Error running MarginFunction:  ";
		if (LuaHelpers::RunScriptOnStack(L, err, 2, 3, true)) {
			std::string marge = "Margin value must be a number.";
			margins[0] = static_cast<float>(SafeFArg(L, -3, marge, 40));
			float center = static_cast<float>(SafeFArg(L, -2, marge, 80));
			margins[1] = center / 2.0f;
		}
		lua_settop(L, 0);
		LUA->Release(L);
	}

	float left_edge = 0.0f;
	std::string sName("PlayerP1");
	m_vPlayerInfo.m_pPlayer->SetName(sName);
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
	m_vPlayerInfo.GetPlayerState()->m_NotefieldZoom = 1.f;

	if (!Center1Player())
		m_vPlayerInfo.m_pPlayer->SetX(player_x);
	else
		m_vPlayerInfo.m_pPlayer->SetX(SCREEN_CENTER_X);
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
	if (GAMESTATE->m_bPlayingMulti)
		NSMAN->StartRequest(0);

	// Add individual life meter
	switch (GAMESTATE->m_PlayMode) {
		case PLAY_MODE_REGULAR:
			if (!GAMESTATE->IsPlayerEnabled(m_vPlayerInfo.m_pn) ||
				m_sName == "ScreenGameplaySyncMachine")
				break;

			m_vPlayerInfo.m_pLifeMeter =
			  LifeMeter::MakeLifeMeter(m_vPlayerInfo.GetPlayerState()
										 ->m_PlayerOptions.GetStage()
										 .m_LifeType);
			m_vPlayerInfo.m_pLifeMeter->Load(
			  m_vPlayerInfo.GetPlayerState(),
			  m_vPlayerInfo.GetPlayerStageStats());
			m_vPlayerInfo.m_pLifeMeter->SetName(
			  ssprintf("Life%s", m_vPlayerInfo.GetName().c_str()));
			LOAD_ALL_COMMANDS_AND_SET_XY(m_vPlayerInfo.m_pLifeMeter);
			this->AddChild(m_vPlayerInfo.m_pLifeMeter);
			break;
		default:
			break;
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

	if (m_pSongBackground != nullptr)
		m_pSongBackground->Init();

	std::string sType = PLAYER_TYPE;
	if (m_vPlayerInfo.m_bIsDummy)
		sType += "Dummy";
	m_vPlayerInfo.m_pPlayer->Init(sType,
								  m_vPlayerInfo.GetPlayerState(),
								  m_vPlayerInfo.GetPlayerStageStats(),
								  m_vPlayerInfo.m_pLifeMeter,
								  m_vPlayerInfo.m_pPrimaryScoreKeeper);

	// fill in m_apSongsQueue, m_vpStepsQueue, m_asModifiersQueue
	InitSongQueues();

	// Fill StageStats
	STATSMAN->m_CurStageStats.m_vpPossibleSongs = m_apSongsQueue;
	if (m_vPlayerInfo.GetPlayerStageStats())
		m_vPlayerInfo.GetPlayerStageStats()->m_vpPossibleSteps =
		  m_vPlayerInfo.m_vpStepsQueue;

	ASSERT(!m_vPlayerInfo.m_vpStepsQueue.empty());
	if (m_vPlayerInfo.GetPlayerStageStats())
		m_vPlayerInfo.GetPlayerStageStats()->m_bJoined = true;
	if (m_vPlayerInfo.m_pPrimaryScoreKeeper)
		m_vPlayerInfo.m_pPrimaryScoreKeeper->Load(m_apSongsQueue,
												  m_vPlayerInfo.m_vpStepsQueue);

	GAMESTATE->m_bGameplayLeadIn.Set(true);

	/* LoadNextSong first, since that positions some elements which need to be
	 * positioned before we TweenOnScreen. */
	LoadNextSong();

	m_GiveUpTimer.SetZero();
	m_gave_up = false;
	GAMESTATE->m_bRestartedGameplay = false;
}

bool
ScreenGameplay::Center1Player() const
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

		Playlist& pl = SONGMAN->GetPlaylists()[SONGMAN->playlistcourse];
		FOREACH(Chart, pl.chartlist, ch)
		{
			m_apSongsQueue.emplace_back(ch->songptr);
			m_vPlayerInfo.m_vpStepsQueue.emplace_back(ch->stepsptr);
			ratesqueue.emplace_back(ch->rate);
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

	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("ScreenGameplay::~ScreenGameplay()");

	SAFE_DELETE(m_pSongBackground);
	SAFE_DELETE(m_pSongForeground);

	if (m_pSoundMusic != nullptr)
		m_pSoundMusic->StopPlaying();

	m_GameplayAssist.StopPlaying();

	// If we didn't just restart gameplay...
	if (!GAMESTATE->m_bRestartedGameplay) {
		// Tell Multiplayer we ended the song
		if (GAMESTATE->m_bPlayingMulti)
			NSMAN->ReportSongOver();

		// Tell DownloadManager we aren't in Gameplay
		DLMAN->UpdateDLSpeed(false);

		GAMESTATE->m_gameplayMode.Set(GameplayMode_Normal);
		GAMESTATE->TogglePracticeMode(false);
	}

	// Always unpause when exiting gameplay (or restarting)
	GAMESTATE->SetPaused(false);
}

void
ScreenGameplay::SetupNoteDataFromRow(Steps* pSteps, int row)
{
	NoteData originalNoteData;
	pSteps->GetNoteData(originalNoteData);

	const Style* pStyle = GAMESTATE->GetCurrentStyle(m_vPlayerInfo.m_pn);
	NoteData ndTransformed;
	pStyle->GetTransformedNoteDataForStyle(
	  m_vPlayerInfo.GetStepsAndTrailIndex(), originalNoteData, ndTransformed);

	m_vPlayerInfo.GetPlayerState()->Update(0);

	NoteDataUtil::RemoveAllButRange(ndTransformed, row, MAX_NOTE_ROW);

	// load player
	{
		m_vPlayerInfo.m_NoteData = ndTransformed;
		NoteDataUtil::RemoveAllTapsOfType(m_vPlayerInfo.m_NoteData,
										  TapNoteType_AutoKeysound);
		m_vPlayerInfo.m_pPlayer->Reload();
	}

	// load auto keysounds
	{
		NoteData nd = ndTransformed;
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
ScreenGameplay::SetupSong(int iSongIndex)
{
	/* This is the first beat that can be changed without it being visible.
	 * Until we draw for the first time, any beat can be changed. */
	m_vPlayerInfo.GetPlayerState()->m_fLastDrawnBeat = -100;

	Steps* pSteps = m_vPlayerInfo.m_vpStepsQueue[iSongIndex];
	GAMESTATE->m_pCurSteps.Set(pSteps);

	NoteData originalNoteData;
	pSteps->GetNoteData(originalNoteData);

	const Style* pStyle = GAMESTATE->GetCurrentStyle(m_vPlayerInfo.m_pn);
	NoteData ndTransformed;
	pStyle->GetTransformedNoteDataForStyle(
	  m_vPlayerInfo.GetStepsAndTrailIndex(), originalNoteData, ndTransformed);

	m_vPlayerInfo.GetPlayerState()->Update(0);

	// load player
	{
		m_vPlayerInfo.m_NoteData = ndTransformed;
		NoteDataUtil::RemoveAllTapsOfType(m_vPlayerInfo.m_NoteData,
										  TapNoteType_AutoKeysound);
		m_vPlayerInfo.m_pPlayer->Load();
	}

	// load auto keysounds
	{
		NoteData nd = ndTransformed;
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

	GAMESTATE->ResetMusicStatistics();

	m_vPlayerInfo.GetPlayerStageStats()->m_iSongsPlayed++;

	int iPlaySongIndex = GAMESTATE->GetCourseSongIndex();
	iPlaySongIndex %= m_apSongsQueue.size();
	GAMESTATE->m_pCurSong.Set(m_apSongsQueue[iPlaySongIndex]);
	// Check if the music actually exists, this is to avoid an issue in
	// AutoKeysounds.cpp, where the reader will ignore whether the file opener
	// function actually returned a valid object or an error. - Terra
	GAMESTATE->m_pCurSong.Get()->ReloadIfNoMusic();

	STATSMAN->m_CurStageStats.m_vpPlayedSongs.push_back(GAMESTATE->m_pCurSong);

	SetupSong(iPlaySongIndex);

	Song* pSong = GAMESTATE->m_pCurSong;
	Steps* pSteps = GAMESTATE->m_pCurSteps;
	++m_vPlayerInfo.GetPlayerStageStats()->m_iStepsPlayed;

	ASSERT(GAMESTATE->m_pCurSteps != NULL);
	if (m_vPlayerInfo.m_ptextStepsDescription)
		m_vPlayerInfo.m_ptextStepsDescription->SetText(
		  pSteps->GetDescription());

	if (m_vPlayerInfo.m_ptextPlayerOptions)
		m_vPlayerInfo.m_ptextPlayerOptions->SetText(
		  m_vPlayerInfo.GetPlayerState()
			->m_PlayerOptions.GetCurrent()
			.GetString());

	if (m_vPlayerInfo.m_pStepsDisplay)
		m_vPlayerInfo.m_pStepsDisplay->SetFromSteps(pSteps);

	/* The actual note data for scoring is the base class of Player.  This
	 * includes transforms, like Wide.  Otherwise, the scoring will operate
	 * on the wrong data. */
	if (m_vPlayerInfo.m_pPrimaryScoreKeeper)
		m_vPlayerInfo.m_pPrimaryScoreKeeper->OnNextSong(
		  GAMESTATE->GetCourseSongIndex(),
		  pSteps,
		  &m_vPlayerInfo.m_pPlayer->GetNoteData());

	// Don't mess with the PlayerController of the Dummy player
	if (!m_vPlayerInfo.m_bIsDummy) {
		if (GAMESTATE->IsCpuPlayer(m_vPlayerInfo.GetStepsAndTrailIndex())) {
			m_vPlayerInfo.GetPlayerState()->m_PlayerController = PC_CPU;
			int iMeter = pSteps->GetMeter();
			int iNewSkill = SCALE(iMeter, MIN_METER, MAX_METER, 0, 5);
			/* Watch out: songs aren't actually bound by MAX_METER. */
			iNewSkill = std::clamp(iNewSkill, 0, 5);
			m_vPlayerInfo.GetPlayerState()->m_iCpuSkill = iNewSkill;
		} else {
			if (m_vPlayerInfo.GetPlayerState()
				  ->m_PlayerOptions.GetCurrent()
				  .m_fPlayerAutoPlay != 0)
				m_vPlayerInfo.GetPlayerState()->m_PlayerController =
				  PC_AUTOPLAY;
			else
				m_vPlayerInfo.GetPlayerState()->m_PlayerController =
				  GamePreferences::m_AutoPlay;
		}
	}

	bool bAllReverse = true;
	bool bAtLeastOneReverse = false;
	if (m_vPlayerInfo.GetPlayerState()
		  ->m_PlayerOptions.GetCurrent()
		  .m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1)
		bAtLeastOneReverse = true;
	else
		bAllReverse = false;

	bool bReverse = m_vPlayerInfo.GetPlayerState()
					  ->m_PlayerOptions.GetCurrent()
					  .m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1;

	if (m_vPlayerInfo.m_pStepsDisplay)
		m_vPlayerInfo.m_pStepsDisplay->PlayCommand(bReverse ? "SetReverse"
															: "SetNoReverse");

	m_LyricDisplay.PlayCommand(bAllReverse ? "SetReverse" : "SetNoReverse");

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

	if (!m_vPlayerInfo.GetPlayerStageStats()->m_bFailed) {
		// give a little life back between stages
		if (m_vPlayerInfo.m_pLifeMeter)
			m_vPlayerInfo.m_pLifeMeter->OnLoadSong();
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
	RageSoundReader* pPlayerSound =
	  m_AutoKeysounds.GetPlayerSound(m_vPlayerInfo.m_pn);
	if (pPlayerSound == nullptr &&
		m_vPlayerInfo.m_pn == GAMESTATE->GetMasterPlayerNumber())
		pPlayerSound = m_AutoKeysounds.GetSharedSound();
	m_vPlayerInfo.m_SoundEffectControl.SetSoundReader(pPlayerSound);

	if (!GAMESTATE->GetPaused())
		MESSAGEMAN->Broadcast("DoneLoadingNextSong");
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
	Player& player = *m_vPlayerInfo.m_pPlayer;
	const NoteData& nd = player.GetNoteData();
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
	if (m_DancingState != STATE_DANCING)
		return;
	if (GAMESTATE->m_pCurSong ==
		  nullptr || // this will be true on ScreenDemonstration sometimes
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
	const float fSeconds = m_pSoundMusic->GetPositionSeconds(nullptr, &tm);
	const float fAdjust = SOUND->GetFrameTimingAdjustment(fDeltaTime);
	GAMESTATE->UpdateSongPosition(
	  fSeconds + fAdjust, GAMESTATE->m_pCurSong->m_SongTiming, tm + fAdjust);
}

void
ScreenGameplay::BeginScreen()
{
	if (GAMESTATE->m_pCurSong == nullptr)
		return;

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

			  std::string doot = ssprintf("%d I %d I %d I %d I %d I %d  x%d",
										  ptns[TNS_W1],
										  ptns[TNS_W2],
										  ptns[TNS_W3],
										  ptns[TNS_W4],
										  ptns[TNS_W5],
										  ptns[TNS_Miss],
										  this->GetPlayerInfo(PLAYER_1)
											->GetPlayerStageStats()
											->m_iCurCombo);
			  auto player = this->GetPlayerInfo(PLAYER_1)->m_pPlayer;
			  if (player->maxwifescore > 0)
				  NSMAN->SendMPLeaderboardUpdate(
					player->curwifescore / player->maxwifescore, doot);
		  },
		  0.25f,
		  -1);
	}
}

bool
ScreenGameplay::AllAreFailing()
{
	if (m_vPlayerInfo.m_pLifeMeter && !m_vPlayerInfo.m_pLifeMeter->IsFailing())
		return false;
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
	ZoneScoped;
	if (GAMESTATE->m_pCurSong == nullptr) {
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

	FailType failtype =
	  GAMESTATE->GetPlayerFailType(m_vPlayerInfo.GetPlayerState());
	// update GameState HealthState
	HealthState& hs = m_vPlayerInfo.GetPlayerState()->m_HealthState;
	HealthState OldHealthState = hs;
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
		float fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		RageSoundParams p = m_pSoundMusic->GetParams();
		if (std::fabs(p.m_fSpeed - fSpeed) > 0.01f && fSpeed >= 0.0f) {
			p.m_fSpeed = fSpeed;
			m_pSoundMusic->SetParams(p);
		}
	}

	switch (m_DancingState) {
		case STATE_DANCING: {
			/* Set STATSMAN->m_CurStageStats.bFailed for failed players.  In,
			 * FAIL_IMMEDIATE, send SM_BeginFailed if all players failed, and
			 * kill dead Oni players. */
			PlayerNumber pn = m_vPlayerInfo.GetStepsAndTrailIndex();

			if (m_vPlayerInfo.m_pLifeMeter != nullptr) {
				LifeType lt = m_vPlayerInfo.GetPlayerState()
								->m_PlayerOptions.GetStage()
								.m_LifeType;

				// check for individual fail
				if (!(failtype == FailType_Off) &&
					m_vPlayerInfo.m_pLifeMeter->IsFailing() &&
					!m_vPlayerInfo.GetPlayerStageStats()->m_bFailed) {

					LOG->Trace("Player %d failed", static_cast<int>(pn));
					m_vPlayerInfo.GetPlayerStageStats()->m_bFailed =
					  true; // fail

					{
						Message msg("PlayerFailed");
						msg.SetParam("PlayerNumber", m_vPlayerInfo.m_pn);
						MESSAGEMAN->Broadcast(msg);
					}
				}

				// Check for and do Oni die.
				bool bAllowOniDie = false;
				switch (lt) {
					case LifeType_Battery:
						bAllowOniDie = true;
						break;
					default:
						break;
				}
				if (bAllowOniDie && failtype == FailType_Immediate) {
					if (!STATSMAN->m_CurStageStats
						   .AllFailed()) // if not the last one to fail
					{
						// kill them!
						FailFadeRemovePlayer(&m_vPlayerInfo);
					}
				}

				bool bAllFailed = true;
				switch (failtype) {
					case FailType_Immediate:
						if (!m_vPlayerInfo.m_pLifeMeter->IsFailing())
							bAllFailed = false;
						break;
					case FailType_ImmediateContinue:
						bAllFailed =
						  false; // wait until the end of the song to fail.
						break;
					case FailType_Off:
						bAllFailed = false; // never fail.
						break;
					default:
						FAIL_M("Invalid fail type! Aborting...");
				}

				if (bAllFailed) {
					m_pSoundMusic->StopPlaying();
					SCREENMAN->PostMessageToTopScreen(SM_NotesEnded, 0);
					m_LyricDisplay.Stop();
				}
			}

			// Update living players' alive time
			// HACK: Don't scale alive time when using tab/tilde.  Instead of
			// accumulating time from a timer, this time should instead be tied
			// to the music position.
			float fUnscaledDeltaTime = m_timerGameplaySeconds.GetDeltaTime();
			if (!m_vPlayerInfo.GetPlayerStageStats()->m_bFailed)
				m_vPlayerInfo.GetPlayerStageStats()->m_fAliveSeconds +=
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

			// update give up
			bool bGiveUpTimerFired = false;
			bGiveUpTimerFired =
			  !m_GiveUpTimer.IsZero() && m_GiveUpTimer.Ago() > GIVE_UP_SECONDS;
			m_gave_up = bGiveUpTimerFired;

			// Quitters deserve a failed score.
			if (bGiveUpTimerFired) {
				STATSMAN->m_CurStageStats.m_bGaveUp = true;
				m_vPlayerInfo.GetPlayerStageStats()->m_bDisqualified = true;
				m_vPlayerInfo.GetPlayerStageStats()->gaveuplikeadumbass = true;
				ResetGiveUpTimers(false);
				LOG->Trace("Exited Gameplay to Evaluation");
				this->PostScreenMessage(SM_LeaveGameplay, 0);
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
					break;
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
	int tracks = pi->m_NoteData.GetNumTracks();
	pi->m_NoteData.Init();				 // remove all notes and scoring
	pi->m_NoteData.SetNumTracks(tracks); // reset the number of tracks.
	pi->m_pPlayer->FadeToFail();		 // tell the NoteField to fade to white
}

void
ScreenGameplay::SendCrossedMessages()
{
	// hmmm...
	if (GAMESTATE->m_pCurSong == nullptr)
		return;

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
		if (GAMESTATE->m_pCurSteps->GetDifficulty() == Difficulty_Beginner) {
			pn = m_vPlayerInfo.m_pn;
		}

		if (pn == PLAYER_INVALID)
			return;

		const NoteData& nd = m_vPlayerInfo.m_pPlayer->GetNoteData();

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
						const Style* pStyle =
						  GAMESTATE->GetCurrentStyle(m_vPlayerInfo.m_pn);
						std::string sButton = pStyle->ColToButtonName(t);
						Message msg(i == 0 ? "NoteCrossed" : "NoteWillCross");
						msg.SetParam("ButtonName", sButton);
						msg.SetParam("NumMessagesFromCrossed", i);
						msg.SetParam("PlayerNumber", m_vPlayerInfo.m_pn);
						MESSAGEMAN->Broadcast(msg);
					} else {
						const Style* pStyle =
						  GAMESTATE->GetCurrentStyle(PLAYER_INVALID);
						std::string sButton = pStyle->ColToButtonName(t);
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
	if (m_sName.find("Net") != std::string::npos)
		SetPrevScreenName("ScreenNetStageInformation");
	else
		SetPrevScreenName("ScreenStageInformation");
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

	m_textDebug.BeginTweening(1 / 2.f);
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

bool
ScreenGameplay::Input(const InputEventPlus& input)
{
	// LOG->Trace( "ScreenGameplay::Input()" );

	Message msg("");
	if (m_Codes.InputMessage(input, msg))
		this->HandleMessage(msg);

	if (m_DancingState != STATE_OUTRO && GAMESTATE->IsHumanPlayer(input.pn) &&
		!m_Cancel.IsTransitioning()) {
		/* Allow bailing out by holding any START button. */
		bool bHoldingGiveUp = false;
		if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(
			  input.GameI) == Column_Invalid) {
			bHoldingGiveUp |= (input.MenuI == GAME_BUTTON_START);
		}

		// Exiting gameplay by holding Start (Forced Fail)
		if (bHoldingGiveUp) {
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
		bool bHoldingBack = false;
		if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(
			  input.GameI) == Column_Invalid) {
			bHoldingBack |= input.MenuI == GAME_BUTTON_BACK;
		}

		if (bHoldingBack) {
			if (((!PREFSMAN->m_bDelayedBack && input.type == IET_FIRST_PRESS) ||
				 (input.DeviceI.device == DEVICE_KEYBOARD &&
				  input.type == IET_REPEAT) ||
				 (input.DeviceI.device != DEVICE_KEYBOARD &&
				  INPUTFILTER->GetSecsHeld(input.DeviceI) >= 1.0f))) {
				if (PREFSMAN->m_verbose_log > 1)
					LOG->Trace("Player %i went back", input.pn + 1);
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

	/* Restart gameplay button moved from theme to allow for rebinding for
	 * people who dont want to edit lua files :)
	 */
	bool bHoldingRestart = false;
	if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(input.GameI) ==
		Column_Invalid) {
		bHoldingRestart |= input.MenuI == GAME_BUTTON_RESTART;
	}
	if (bHoldingRestart) {
		RestartGameplay();
	}

	// handle a step or battle item activate
	if (GAMESTATE->IsHumanPlayer(input.pn)) {
		ResetGiveUpTimers(true);

		if (GamePreferences::m_AutoPlay == PC_HUMAN &&
			GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
				.m_fPlayerAutoPlay == 0) {

			ASSERT(input.GameI.IsValid());

			GameButtonType gbt =
			  GAMESTATE->m_pCurGame->GetPerButtonInfo(input.GameI.button)
				->m_gbt;
			switch (gbt) {
				case GameButtonType_Menu:
					return false;
				case GameButtonType_Step:
					if (iCol != -1)
						m_vPlayerInfo.m_pPlayer->Step(
						  iCol, -1, input.DeviceI.ts, false, bRelease);
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
	float fMusicLen = GAMESTATE->m_pCurSong->m_fMusicLengthSeconds;

	/* Note that adding stats is only meaningful for the counters (eg.
	 * RadarCategory_Jumps), not for the percentages (RadarCategory_Air). */
	RadarValues rv;
	PlayerStageStats& pss = *m_vPlayerInfo.GetPlayerStageStats();
	const NoteData& nd = m_vPlayerInfo.m_pPlayer->GetNoteData();
	PlayerNumber pn = m_vPlayerInfo.m_pn;

	GAMESTATE->SetProcessedTimingData(GAMESTATE->m_pCurSteps->GetTimingData());
	NoteDataUtil::CalculateRadarValues(nd, fMusicLen, rv);
	pss.m_radarPossible += rv;
	NoteDataWithScoring::GetActualRadarValues(nd, pss, fMusicLen, rv);
	pss.m_radarActual += rv;
	GAMESTATE->SetProcessedTimingData(nullptr);
}

void
ScreenGameplay::SongFinished()
{

	if (GAMESTATE->m_pCurSteps) {
		GAMESTATE->m_pCurSteps->GetTimingData()->ReleaseLookup();
	}
	SaveStats(); // Let subclasses save the stats.
}

void
ScreenGameplay::StageFinished(bool bBackedOut)
{
	CHECKPOINT_M("Finishing Stage");
	if (bBackedOut) {
		GAMESTATE->CancelStage();
		return;
	}

	// If all players failed, kill.
	if (STATSMAN->m_CurStageStats.AllFailed()) {
		GAMESTATE->m_iPlayerStageTokens = 0;
	}

	// Properly set the LivePlay bool
	STATSMAN->m_CurStageStats.m_bLivePlay = true;

	STATSMAN->m_CurStageStats.FinalizeScores(false);

	// If we didn't cheat and aren't in Practice
	// (Replay does its own thing somewhere else here)
	if (GamePreferences::m_AutoPlay == PC_HUMAN &&
		!GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice) {
		HighScore* pHS = &STATSMAN->m_CurStageStats.m_player.m_HighScore;
		auto nd = GAMESTATE->m_pCurSteps->GetNoteData();

		// Load the replay data for the current score so some cool functionality
		// works immediately
		PlayerAI::ResetScoreData();
		PlayerAI::SetScoreData(pHS, 0, &nd);
		GAMESTATE->CommitStageStats();
	}

	// save current stage stats
	STATSMAN->m_vPlayedStageStats.push_back(STATSMAN->m_CurStageStats);

	STATSMAN->CalcAccumPlayedStageStats();
	GAMESTATE->FinishStage();
	CHECKPOINT_M("Done Finishing Stage");
}

void
ScreenGameplay::HandleScreenMessage(const ScreenMessage& SM)
{
	CHECKPOINT_M(
	  ssprintf("HandleScreenMessage(%s)",
			   ScreenMessageHelpers::ScreenMessageToString(SM).c_str())
		.c_str());
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
		if (GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_bPractice)
			return; // don't auto leave gameplay when finishing notes during
					// practice mode this prevents use of eval screen during
					// practice which im pretty sure nobody cares about?

		ResetGiveUpTimers(
		  false); // don't allow giveup while the next song is loading

		// Mark failure.
		if (GAMESTATE->GetPlayerFailType(m_vPlayerInfo.GetPlayerState()) !=
			  FailType_Off &&
			(m_vPlayerInfo.m_pLifeMeter &&
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
			  0, STATSMAN->m_CurStageStats.m_fGameplaySeconds);
		}

		/* If all players have *really* failed (bFailed, not the life meter or
		 * bFailedEarlier): */
		const bool bAllReallyFailed = STATSMAN->m_CurStageStats.AllFailed();
		const bool bIsLastSong = m_apSongsQueue.size() == 1;

		LOG->Trace("bAllReallyFailed = %d "
				   "bIsLastSong = %d, m_gave_up = %d",
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

		if (!bIsLastSong) {
			// Load the next song in the course.
			HandleScreenMessage(SM_StartLoadingNextSong);
			return;
		} else {
			// Time to leave from ScreenGameplay
			HandleScreenMessage(SM_LeaveGameplay);
		}
	} else if (SM == SM_LeaveGameplay) {
		GAMESTATE->m_DanceDuration = GAMESTATE->m_DanceStartTime.Ago();

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
		if (m_vPlayerInfo.m_pLifeMeter)
			m_vPlayerInfo.m_pLifeMeter->OnSongEnded();

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
			if (m_Toasty.IsWaiting()) {
				m_Toasty.Reset();
				m_Toasty.StartTransitioning();
			}
		}
	} else if (ScreenMessageHelpers::ScreenMessageToString(SM).find("0Combo") !=
			   string::npos) {
		int iCombo;
		std::string sCropped =
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
		if (m_vPlayerInfo.GetPlayerState()->m_PlayerController ==
			PC_REPLAY) // don't duplicate replay saves
		{
			replaying = true;
		}

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

		if (m_vPlayerInfo.m_pn == pn && m_vPlayerInfo.GetPlayerState()
										  ->m_PlayerOptions.GetCurrent()
										  .m_bMuteOnError) {

			RageSoundReader* pSoundReader = m_AutoKeysounds.GetPlayerSound(pn);
			if (pSoundReader == nullptr)
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
	if (m_vPlayerInfo.m_pn == pn)
		return &m_vPlayerInfo;
	return nullptr;
}

const float
ScreenGameplay::GetSongPosition()
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
	static int Center1Player(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->Center1Player());
		return 1;
	}
	static int GetLifeMeter(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;

		PlayerInfo* pi = p->GetPlayerInfo(pn);
		if (pi == nullptr)
			return 0;
		LifeMeter* pLM = pi->m_pLifeMeter;
		if (pLM == nullptr)
			return 0;

		pLM->PushSelf(L);
		return 1;
	}
	static int GetPlayerInfo(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;

		PlayerInfo* pi = p->GetPlayerInfo(pn);
		if (pi == nullptr)
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
