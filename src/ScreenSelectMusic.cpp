#include "global.h"
#include "ActorUtil.h"
#include "CodeDetector.h"
#include "CommonMetrics.h"
#include "Foreach.h"
#include "Game.h"
#include "GameConstantsAndTypes.h"
#include "GameManager.h"
#include "GameSoundManager.h"
#include "ScoreManager.h"
#include "GameState.h"
#include "InputMapper.h"
#include "MenuTimer.h"
#include "StageStats.h"
#include "PlayerState.h"
#include "PrefsManager.h"
#include "ProfileManager.h"
#include "RageLog.h"
#include "RageTextureManager.h"
#include "ScreenManager.h"
#include "ScreenSelectMusic.h"
#include "SongManager.h"
#include "StatsManager.h"
#include "Steps.h"
#include "StepsUtil.h"
#include "Style.h"
#include "PlayerState.h"
#include "CommonMetrics.h"
#include "ImageCache.h"
#include "ScreenPrompt.h"
#include "Song.h"
#include "InputEventPlus.h"
#include "OptionsList.h"
#include "ProfileManager.h"
#include "RageFileManager.h"
#include "RageInput.h"
#include "ScreenPrompt.h"
#include "ScreenTextEntry.h"
#include "ProfileManager.h"
#include "DownloadManager.h"
#include "GamePreferences.h"
#include "PlayerAI.h"
#include "PlayerOptions.h"
#include "NoteData.h"
#include "Player.h"
#include "NoteDataUtil.h"

static const char* SelectionStateNames[] = { "SelectingSong",
											 "SelectingSteps",
											 "Finalized" };
XToString(SelectionState);

/** @brief The maximum number of digits for the ScoreDisplay. */
const int NUM_SCORE_DIGITS = 9;

#define SHOW_OPTIONS_MESSAGE_SECONDS                                           \
	THEME->GetMetricF(m_sName, "ShowOptionsMessageSeconds")

static const ThemeMetric<int> HARD_COMMENT_METER("ScreenSelectMusic",
												 "HardCommentMeter");

AutoScreenMessage(SM_AllowOptionsMenuRepeat);
AutoScreenMessage(SM_SongChanged);
AutoScreenMessage(SM_SortOrderChanging);
AutoScreenMessage(SM_SortOrderChanged);
AutoScreenMessage(SM_BackFromPlayerOptions);
AutoScreenMessage(SM_ConfirmDeleteSong);
AutoScreenMessage(SM_BackFromNamePlaylist);

static bool g_bSampleMusicWaiting = false;
static bool delayedchartupdatewaiting = false;
static RageTimer g_StartedLoadingAt(RageZeroTimer);
static RageTimer g_ScreenStartedLoadingAt(RageZeroTimer);
RageTimer g_CanOpenOptionsList(RageZeroTimer);

static LocalizedString PERMANENTLY_DELETE("ScreenSelectMusic",
										  "PermanentlyDelete");

REGISTER_SCREEN_CLASS(ScreenSelectMusic);
void
ScreenSelectMusic::Init()
{
	GAMESTATE->m_bPlayingMulti = false;
	g_ScreenStartedLoadingAt.Touch();
	if (PREFSMAN->m_sTestInitialScreen.Get() == m_sName) {
		GAMESTATE->m_PlayMode.Set(PLAY_MODE_REGULAR);
		GAMESTATE->SetCurrentStyle(
		  GAMEMAN->GameAndStringToStyle(GAMEMAN->GetDefaultGame(), "versus"),
		  PLAYER_INVALID);
		GAMESTATE->JoinPlayer(PLAYER_1);
		GAMESTATE->SetMasterPlayerNumber(PLAYER_1);
	}
	if (GamePreferences::m_AutoPlay == PC_REPLAY)
		GamePreferences::m_AutoPlay.Set(PC_HUMAN);
	if (GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerController == PC_REPLAY)
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerController = PC_HUMAN;

	// Remove failOff if we enter SSM with Practice Mode on so if the player turns it back off when entering a song, we don't have to worry about it
	if (GAMESTATE->m_pPlayerState[PLAYER_1]
		  ->m_PlayerOptions.GetCurrent()
		  .m_bPractice) {
		GAMESTATE->m_pPlayerState[PLAYER_1]
		  ->m_PlayerOptions.GetPreferred()
		  .m_FailType = FailType_Immediate;
		GAMESTATE->m_pPlayerState[PLAYER_1]
		  ->m_PlayerOptions.GetSong()
		  .m_FailType = FailType_Immediate;
		GAMESTATE->m_pPlayerState[PLAYER_1]
		  ->m_PlayerOptions.GetCurrent()
		  .m_FailType = FailType_Immediate;
	}

	IDLE_COMMENT_SECONDS.Load(m_sName, "IdleCommentSeconds");
	SAMPLE_MUSIC_DELAY_INIT.Load(m_sName, "SampleMusicDelayInit");
	SAMPLE_MUSIC_DELAY.Load(m_sName, "SampleMusicDelay");
	SAMPLE_MUSIC_LOOPS.Load(m_sName, "SampleMusicLoops");
	SAMPLE_MUSIC_PREVIEW_MODE.Load(m_sName, "SampleMusicPreviewMode");
	SAMPLE_MUSIC_FALLBACK_FADE_IN_SECONDS.Load(
	  m_sName, "SampleMusicFallbackFadeInSeconds");
	SAMPLE_MUSIC_FADE_OUT_SECONDS.Load(m_sName, "SampleMusicFadeOutSeconds");
	DO_ROULETTE_ON_MENU_TIMER.Load(m_sName, "DoRouletteOnMenuTimer");
	ROULETTE_TIMER_SECONDS.Load(m_sName, "RouletteTimerSeconds");
	ALIGN_MUSIC_BEATS.Load(m_sName, "AlignMusicBeat");
	CODES.Load(m_sName, "Codes");
	PLAYER_OPTIONS_SCREEN.Load(m_sName, "PlayerOptionsScreen");
	MUSIC_WHEEL_TYPE.Load(m_sName, "MusicWheelType");
	SELECT_MENU_AVAILABLE.Load(m_sName, "SelectMenuAvailable");
	MODE_MENU_AVAILABLE.Load(m_sName, "ModeMenuAvailable");
	USE_OPTIONS_LIST.Load(m_sName, "UseOptionsList");
	USE_PLAYER_SELECT_MENU.Load(m_sName, "UsePlayerSelectMenu");
	SELECT_MENU_NAME.Load(m_sName, "SelectMenuScreenName");
	OPTIONS_LIST_TIMEOUT.Load(m_sName, "OptionsListTimeout");
	SELECT_MENU_CHANGES_DIFFICULTY.Load(m_sName, "SelectMenuChangesDifficulty");
	WRAP_CHANGE_STEPS.Load(m_sName, "WrapChangeSteps");
	NULL_SCORE_STRING.Load(m_sName, "NullScoreString");
	PLAY_SOUND_ON_ENTERING_OPTIONS_MENU.Load(m_sName,
											 "PlaySoundOnEnteringOptionsMenu");
	// To allow changing steps with gamebuttons -DaisuMaster
	CHANGE_STEPS_WITH_GAME_BUTTONS.Load(m_sName, "ChangeStepsWithGameButtons");
	CHANGE_GROUPS_WITH_GAME_BUTTONS.Load(m_sName,
										 "ChangeGroupsWithGameButtons");

	m_GameButtonPreviousSong = INPUTMAPPER->GetInputScheme()->ButtonNameToIndex(
	  THEME->GetMetric(m_sName, "PreviousSongButton"));
	m_GameButtonNextSong = INPUTMAPPER->GetInputScheme()->ButtonNameToIndex(
	  THEME->GetMetric(m_sName, "NextSongButton"));

	// Ask for those only if changing steps with gamebuttons is allowed
	// -DaisuMaster
	if (CHANGE_STEPS_WITH_GAME_BUTTONS) {
		m_GameButtonPreviousDifficulty =
		  INPUTMAPPER->GetInputScheme()->ButtonNameToIndex(
			THEME->GetMetric(m_sName, "PreviousDifficultyButton"));
		m_GameButtonNextDifficulty =
		  INPUTMAPPER->GetInputScheme()->ButtonNameToIndex(
			THEME->GetMetric(m_sName, "NextDifficultyButton"));
	}
	// same here but for groups -DaisuMaster
	if (CHANGE_GROUPS_WITH_GAME_BUTTONS) {
		m_GameButtonPreviousGroup =
		  INPUTMAPPER->GetInputScheme()->ButtonNameToIndex(
			THEME->GetMetric(m_sName, "PreviousGroupButton"));
		m_GameButtonNextGroup =
		  INPUTMAPPER->GetInputScheme()->ButtonNameToIndex(
			THEME->GetMetric(m_sName, "NextGroupButton"));
	}

	FOREACH_ENUM(PlayerNumber, p)
	{
		m_bSelectIsDown[p] = false; // used by UpdateSelectButton
		m_bAcceptSelectRelease[p] = false;
	}

	ScreenWithMenuElements::Init();

	this->SubscribeToMessage(Message_PlayerJoined);

	// Cache these values
	// Marking for change -- Midiman (why? -aj)
	m_sSectionMusicPath = THEME->GetPathS(m_sName, "section music");
	m_sSortMusicPath = THEME->GetPathS(m_sName, "sort music");
	m_sRouletteMusicPath = THEME->GetPathS(m_sName, "roulette music");
	m_sRandomMusicPath = THEME->GetPathS(m_sName, "random music");
	m_sLoopMusicPath = THEME->GetPathS(m_sName, "loop music");
	m_sFallbackCDTitlePath = THEME->GetPathG(m_sName, "fallback cdtitle");

	m_TexturePreload.Load(m_sFallbackCDTitlePath);

	// load banners
	if (PREFSMAN->m_ImageCache != IMGCACHE_OFF) {
		m_TexturePreload.Load(
		  Banner::SongBannerTexture(THEME->GetPathG("Banner", "all music")));
		m_TexturePreload.Load(Banner::SongBannerTexture(
		  THEME->GetPathG("Common", "fallback banner")));
		m_TexturePreload.Load(
		  Banner::SongBannerTexture(THEME->GetPathG("Banner", "roulette")));
		m_TexturePreload.Load(
		  Banner::SongBannerTexture(THEME->GetPathG("Banner", "random")));
		m_TexturePreload.Load(
		  Banner::SongBannerTexture(THEME->GetPathG("Banner", "mode")));
	}

	// Load low-res banners and backgrounds if needed.
	IMAGECACHE->Demand("Banner");

	// build the playlist groups here, songmanager's init from disk can't
	// because profiles aren't loaded until after that's done -mina
	SONGMAN->MakeSongGroupsFromPlaylists();

	m_MusicWheel.SetName("MusicWheel");
	m_MusicWheel.Load(MUSIC_WHEEL_TYPE);
	LOAD_ALL_COMMANDS_AND_SET_XY(m_MusicWheel);
	this->AddChild(&m_MusicWheel);

	if (USE_OPTIONS_LIST) {
		FOREACH_PlayerNumber(p)
		{
			m_OptionsList[p].SetName("OptionsList" + PlayerNumberToString(p));
			m_OptionsList[p].Load("OptionsList", p);
			m_OptionsList[p].SetDrawOrder(100);
			ActorUtil::LoadAllCommands(m_OptionsList[p], m_sName);
			this->AddChild(&m_OptionsList[p]);
		}
		m_OptionsList[PLAYER_1].Link(&m_OptionsList[PLAYER_2]);
		m_OptionsList[PLAYER_2].Link(&m_OptionsList[PLAYER_1]);
	}

	RageSoundLoadParams SoundParams;
	SoundParams.m_bSupportPan = true;

	m_soundStart.Load(THEME->GetPathS(m_sName, "start"));
	m_soundDifficultyEasier.Load(
	  THEME->GetPathS(m_sName, "difficulty easier"), false, &SoundParams);
	m_soundDifficultyHarder.Load(
	  THEME->GetPathS(m_sName, "difficulty harder"), false, &SoundParams);
	m_soundOptionsChange.Load(THEME->GetPathS(m_sName, "options"));
	m_soundLocked.Load(THEME->GetPathS(m_sName, "locked"));

	m_pPreviewNoteField = nullptr;

	this->SortByDrawOrder();
}

void
ScreenSelectMusic::BeginScreen()
{
	g_ScreenStartedLoadingAt.Touch();
	m_timerIdleComment.GetDeltaTime();

	SONGMAN->MakeSongGroupsFromPlaylists();
	SONGMAN->SetFavoritedStatus(
	  PROFILEMAN->GetProfile(PLAYER_1)->FavoritedCharts);
	SONGMAN->SetHasGoal(PROFILEMAN->GetProfile(PLAYER_1)->goalmap);
	if (CommonMetrics::AUTO_SET_STYLE) {
		GAMESTATE->SetCompatibleStylesForPlayers();
	}

	if (GAMESTATE->GetCurrentStyle(PLAYER_INVALID) == nullptr) {
		LOG->Trace("The Style has not been set.  A theme must set the Style "
				   "before loading ScreenSelectMusic.");
		// Instead of crashing, set the first compatible style.
		vector<StepsType> vst;
		GAMEMAN->GetStepsTypesForGame(GAMESTATE->m_pCurGame, vst);
		const Style* pStyle = GAMEMAN->GetFirstCompatibleStyle(
		  GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined(), vst[0]);
		if (pStyle == nullptr) {
			LOG->Warn(ssprintf("No compatible styles for %s with %d player%s.",
							   GAMESTATE->m_pCurGame->m_szName,
							   GAMESTATE->GetNumSidesJoined(),
							   GAMESTATE->GetNumSidesJoined() == 1 ? "" : "s") +
					  "Returning to title menu.");
			SCREENMAN->SetNewScreen("ScreenTitleMenu");
		}
		GAMESTATE->SetCurrentStyle(pStyle, PLAYER_INVALID);
	}

	if (GAMESTATE->m_PlayMode == PlayMode_Invalid) {
		// Instead of crashing here, let's just set the PlayMode to regular
		GAMESTATE->m_PlayMode.Set(PLAY_MODE_REGULAR);
		LOG->Trace("PlayMode not set, setting as regular.");
	}

	OPTIONS_MENU_AVAILABLE.Load(m_sName, "OptionsMenuAvailable");
	PlayCommand("Mods");
	m_MusicWheel.BeginScreen();

	m_SelectionState = SelectionState_SelectingSong;
	ZERO(m_bStepsChosen);
	m_bGoToOptions = false;
	m_bAllowOptionsMenu = m_bAllowOptionsMenuRepeat = false;
	ZERO(m_iSelection);

	if (USE_OPTIONS_LIST)
		FOREACH_PlayerNumber(pn) m_OptionsList[pn].Reset();

	AfterMusicChange();

	SOUND->PlayOnceFromAnnouncer("select music intro");

	if (GAMESTATE->IsPlaylistCourse()) {
		GAMESTATE->isplaylistcourse = false;
		SONGMAN->playlistcourse = "";
	}
	if (GAMESTATE->m_pCurSteps[PLAYER_1] != nullptr)
		DLMAN->RequestChartLeaderBoard(
		  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());

	ScreenWithMenuElements::BeginScreen();
}

ScreenSelectMusic::~ScreenSelectMusic()
{
	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("ScreenSelectMusic::~ScreenSelectMusic()");
	IMAGECACHE->Undemand("Banner");
}

// If bForce is true, the next request will be started even if it might cause a
// skip.
void
ScreenSelectMusic::CheckBackgroundRequests(bool bForce)
{
	/* Loading the rest can cause small skips, so don't do it until the wheel
	 * settles. Do load if we're transitioning out, though, so we don't miss
	 * starting the music for the options screen if a song is selected quickly.
	 * Also, don't do this if the wheel is locked, since we're just bouncing
	 * around after selecting TYPE_RANDOM, and it'll take a while before the
	 * wheel will settle. */
	if (!m_MusicWheel.IsSettled() && !m_MusicWheel.WheelIsLocked() && !bForce)
		return;

	// we need something similar to the previewmusic delay except for charts, so
	// heavy duty chart specific operations can be delayed when scrolling (chord
	// density graph, possibly chart leaderboards, etc) -mina

	// in theory the notedata load for chartpreviews could go here however a
	// delay might make it weird when swapping between difficulties to compare
	// sections for which you would want instantaneous action -mina
	if (delayedchartupdatewaiting) {
		if (g_ScreenStartedLoadingAt
			  .Ago() > // not sure if i need the "moving fast" check -mina
			SAMPLE_MUSIC_DELAY_INIT) // todo: decoupled this mina
		{
			MESSAGEMAN->Broadcast("DelayedChartUpdate");
			delayedchartupdatewaiting = false;
		}
	}

	// Nothing else is going.  Start the music, if we haven't yet.
	if (g_bSampleMusicWaiting) {
		if (g_ScreenStartedLoadingAt.Ago() < SAMPLE_MUSIC_DELAY_INIT)
			return;

		// Don't start the music sample when moving fast.
		if (g_StartedLoadingAt.Ago() < SAMPLE_MUSIC_DELAY && !bForce)
			return;

		g_bSampleMusicWaiting = false;

		GameSoundManager::PlayMusicParams PlayParams;
		PlayParams.sFile = HandleLuaMusicFile(m_sSampleMusicToPlay);
		PlayParams.pTiming = m_pSampleMusicTimingData;
		PlayParams.bForceLoop = SAMPLE_MUSIC_LOOPS;
		PlayParams.fStartSecond = m_fSampleStartSeconds;
		PlayParams.fLengthSeconds = m_fSampleLengthSeconds;
		PlayParams.fFadeOutLengthSeconds = SAMPLE_MUSIC_FADE_OUT_SECONDS;
		PlayParams.bAlignBeat = ALIGN_MUSIC_BEATS;
		PlayParams.bApplyMusicRate = true;

		GameSoundManager::PlayMusicParams FallbackMusic;
		FallbackMusic.sFile = m_sLoopMusicPath;
		FallbackMusic.fFadeInLengthSeconds =
		  SAMPLE_MUSIC_FALLBACK_FADE_IN_SECONDS;
		FallbackMusic.bAlignBeat = ALIGN_MUSIC_BEATS;
		SOUND->PlayMusic(PlayParams);
		MESSAGEMAN->Broadcast("PlayingSampleMusic");
	}
}

void
ScreenSelectMusic::Update(float fDeltaTime)
{
	if (!IsTransitioning()) {
		if (IDLE_COMMENT_SECONDS > 0 &&
			m_timerIdleComment.PeekDeltaTime() >= IDLE_COMMENT_SECONDS) {
			SOUND->PlayOnceFromAnnouncer(m_sName + " IdleComment");
			m_timerIdleComment.GetDeltaTime();
		}
	}
	ScreenWithMenuElements::Update(fDeltaTime);

	CheckBackgroundRequests(false);
}

void
ScreenSelectMusic::OpenOptions()
{
	SCREENMAN->AddNewScreenToTop(PLAYER_OPTIONS_SCREEN,
								 SM_BackFromPlayerOptions);
}
void
ScreenSelectMusic::DifferentialReload()
{
	SONGMAN->DifferentialReload();
	m_MusicWheel.ReloadSongList(false, "");
}

bool
ScreenSelectMusic::Input(const InputEventPlus& input)
{
	// HACK: This screen eats mouse inputs if we don't check for them first.
	bool mouse_evt = false;
	for (int i = MOUSE_LEFT; i <= MOUSE_WHEELDOWN; i++) {
		if (input.DeviceI == DeviceInput(DEVICE_MOUSE, (DeviceButton)i))
			mouse_evt = true;
	}

	// Right in the between the mouse inputs we can casually barge in and
	// check for Chart Preview related inputs.
	// This is to deal with scrolling.
	// This is expecting that the creation and destruction of the NoteField is
	// being handled by the Theme.

	if (mouse_evt) {
		return ScreenWithMenuElements::Input(input);
	}
	//	LOG->Trace( "ScreenSelectMusic::Input()" );

	// reset announcer timer
	m_timerIdleComment.GetDeltaTime();

	// debugging?
	// I just like being able to see untransliterated titles occasionally.
	if (input.DeviceI.device == DEVICE_KEYBOARD &&
		input.DeviceI.button == KEY_F9) {
		if (input.type != IET_FIRST_PRESS)
			return false;
		PREFSMAN->m_bShowNativeLanguage.Set(!PREFSMAN->m_bShowNativeLanguage);
		MESSAGEMAN->Broadcast("DisplayLanguageChanged");
		m_MusicWheel.RebuildWheelItems();
		return true;
	}

	if (!IsTransitioning() && m_SelectionState != SelectionState_Finalized) {
		bool bHoldingCtrl =
		  INPUTFILTER->IsBeingPressed(
			DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL)) ||
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL));

		bool holding_shift =
		  INPUTFILTER->IsBeingPressed(
			DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)) ||
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT));

		wchar_t c = INPUTMAN->DeviceInputToChar(input.DeviceI, false);
		MakeUpper(&c, 1);

		if (holding_shift && bHoldingCtrl && c == 'R' &&
			m_MusicWheel.IsSettled()) {
			// Reload the currently selected song. -Kyz
			Song* to_reload = m_MusicWheel.GetSelectedSong();
			if (to_reload != nullptr) {
				auto stepses = to_reload->GetAllSteps();
				vector<string> oldChartkeys;
				for (auto steps : stepses)
					oldChartkeys.emplace_back(steps->GetChartKey());

				to_reload->ReloadFromSongDir();
				SONGMAN->ReconcileChartKeysForReloadedSong(to_reload,
														   oldChartkeys);

				AfterMusicChange();
				return true;
			}
		} else if (holding_shift && bHoldingCtrl && c == 'P' &&
				   m_MusicWheel.IsSettled()) {
			SONGMAN->ForceReloadSongGroup(
			  GetMusicWheel()->GetSelectedSection());
			AfterMusicChange();
			SCREENMAN->SystemMessage("Current pack reloaded");
			return true;
		} else if (bHoldingCtrl && c == 'F' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS) {
			// Favorite the currently selected song. -Not Kyz
			Song* fav_me_biatch = m_MusicWheel.GetSelectedSong();
			if (fav_me_biatch != nullptr) {
				Profile* pProfile = PROFILEMAN->GetProfile(PLAYER_1);

				if (!fav_me_biatch->IsFavorited()) {
					fav_me_biatch->SetFavorited(true);
					pProfile->AddToFavorites(
					  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
					DLMAN->AddFavorite(
					  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
					pProfile->allplaylists.erase("Favorites");
					SONGMAN->MakePlaylistFromFavorites(
					  pProfile->FavoritedCharts, pProfile->allplaylists);
				} else {
					fav_me_biatch->SetFavorited(false);
					pProfile->RemoveFromFavorites(
					  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
					DLMAN->RemoveFavorite(
					  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
				}
				DLMAN->RefreshFavourites();
				Message msg("FavoritesUpdated");
				MESSAGEMAN->Broadcast(msg);
				m_MusicWheel.ChangeMusic(0);
				return true;
			}
		} else if (bHoldingCtrl && c == 'M' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS) {
			// PermaMirror the currently selected song. -Not Kyz
			Song* alwaysmirrorsmh = m_MusicWheel.GetSelectedSong();
			if (alwaysmirrorsmh != nullptr) {
				Profile* pProfile = PROFILEMAN->GetProfile(PLAYER_1);

				if (!alwaysmirrorsmh->IsPermaMirror()) {
					alwaysmirrorsmh->SetPermaMirror(true);
					pProfile->AddToPermaMirror(
					  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
				} else {
					alwaysmirrorsmh->SetPermaMirror(false);
					pProfile->RemoveFromPermaMirror(
					  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
				}
				Message msg("FavoritesUpdated");
				MESSAGEMAN->Broadcast(msg);
				m_MusicWheel.ChangeMusic(0);
				return true;
			}
		} else if (bHoldingCtrl && c == 'G' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS &&
				   GAMESTATE->m_pCurSteps[PLAYER_1] != nullptr) {
			Profile* pProfile = PROFILEMAN->GetProfile(PLAYER_1);
			pProfile->AddGoal(GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
			Song* asonglol = m_MusicWheel.GetSelectedSong();
			if (!asonglol)
				return true;
			asonglol->SetHasGoal(true);
			MESSAGEMAN->Broadcast("FavoritesUpdated");
			m_MusicWheel.ChangeMusic(0);
			return true;
		} else if (bHoldingCtrl && c == 'Q' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS) {
			DifferentialReload();
			return true;
		} else if (bHoldingCtrl && c == 'S' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS) {
			PROFILEMAN->SaveProfile(PLAYER_1);
			SCREENMAN->SystemMessage("Profile Saved");
			return true;
		} else if (bHoldingCtrl && c == 'P' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS) {
			ScreenTextEntry::TextEntry(
			  SM_BackFromNamePlaylist, "Name Playlist", "", 128);
			MESSAGEMAN->Broadcast("DisplayAll");
			return true;
		} else if (bHoldingCtrl && c == 'A' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS &&
				   GAMESTATE->m_pCurSteps[PLAYER_1] != nullptr) {
			if (SONGMAN->GetPlaylists().empty())
				return true;

			SONGMAN->GetPlaylists()[SONGMAN->activeplaylist].AddChart(
			  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
			MESSAGEMAN->Broadcast("DisplaySinglePlaylist");
			SCREENMAN->SystemMessage(
			  ssprintf("Added chart: %s to playlist: %s",
					   GAMESTATE->m_pCurSong->GetDisplayMainTitle().c_str(),
					   SONGMAN->activeplaylist.c_str()));
			return true;
		} else if (input.DeviceI.device == DEVICE_KEYBOARD && bHoldingCtrl &&
				   input.DeviceI.button == KEY_BACK &&
				   input.type == IET_FIRST_PRESS && m_MusicWheel.IsSettled()) {
			// Keyboard shortcut to delete a song from disk (ctrl + backspace)
			Song* songToDelete = m_MusicWheel.GetSelectedSong();
			if (songToDelete && PREFSMAN->m_bAllowSongDeletion.Get()) {
				m_pSongAwaitingDeletionConfirmation = songToDelete;
				ScreenPrompt::Prompt(
				  SM_ConfirmDeleteSong,
				  ssprintf(PERMANENTLY_DELETE.GetValue(),
						   songToDelete->m_sMainTitle.c_str(),
						   songToDelete->GetSongDir().c_str()),
				  PROMPT_YES_NO);
				return true;
			}
		}
	}

	if (!input.GameI.IsValid())
		return false; // don't care

	if (!GAMESTATE->IsHumanPlayer(input.pn))
		return false;

	// Check for "Press START again for options" button press
	if (m_SelectionState == SelectionState_Finalized &&
		input.MenuI == GAME_BUTTON_START && input.type != IET_RELEASE &&
		OPTIONS_MENU_AVAILABLE.GetValue() && !GAMESTATE->m_bPlayingMulti) {
		if (m_bGoToOptions)
			return false; // got it already
		if (!m_bAllowOptionsMenu)
			return false; // not allowed

		if (!m_bAllowOptionsMenuRepeat && input.type == IET_REPEAT) {
			return false; // not allowed yet
		}

		m_bGoToOptions = true;
		if (PLAY_SOUND_ON_ENTERING_OPTIONS_MENU)
			m_soundStart.Play(true);
		this->PlayCommand("ShowEnteringOptions");

		// Re-queue SM_BeginFadingOut, since ShowEnteringOptions may have
		// short-circuited animations.
		this->ClearMessageQueue(SM_BeginFadingOut);
		this->PostScreenMessage(SM_BeginFadingOut, this->GetTweenTimeLeft());

		return true;
	}

	if (IsTransitioning())
		return false; // ignore

	// Handle unselect steps
	// xxx: select button could conflict with OptionsList here -aj
	if (m_SelectionState == SelectionState_SelectingSteps &&
		m_bStepsChosen[input.pn] && input.MenuI == GAME_BUTTON_SELECT &&
		input.type == IET_FIRST_PRESS) {
		Message msg("StepsUnchosen");
		msg.SetParam("Player", input.pn);
		MESSAGEMAN->Broadcast(msg);
		m_bStepsChosen[input.pn] = false;
		return true;
	}

	if (m_SelectionState == SelectionState_Finalized ||
		m_bStepsChosen[input.pn])
		return false; // ignore

	if (USE_PLAYER_SELECT_MENU) {
		if (input.type == IET_RELEASE && input.MenuI == GAME_BUTTON_SELECT) {
			SCREENMAN->AddNewScreenToTop(SELECT_MENU_NAME,
										 SM_BackFromPlayerOptions);
		}
	}

	// handle OptionsList input
	if (USE_OPTIONS_LIST) {
		PlayerNumber pn = input.pn;
		if (pn != PLAYER_INVALID) {
			if (m_OptionsList[pn].IsOpened()) {
				return m_OptionsList[pn].Input(input);
			} else {
				if (input.type == IET_RELEASE &&
					input.MenuI == GAME_BUTTON_SELECT &&
					m_bAcceptSelectRelease[pn])
					m_OptionsList[pn].Open();
			}
		}
	}

	if (input.MenuI == GAME_BUTTON_SELECT && input.type != IET_REPEAT)
		m_bAcceptSelectRelease[input.pn] = (input.type == IET_FIRST_PRESS);

	if (SELECT_MENU_AVAILABLE && input.MenuI == GAME_BUTTON_SELECT &&
		input.type != IET_REPEAT)
		UpdateSelectButton(input.pn, input.type == IET_FIRST_PRESS);

	if (SELECT_MENU_AVAILABLE && m_bSelectIsDown[input.pn]) {
		if (input.type == IET_FIRST_PRESS && SELECT_MENU_CHANGES_DIFFICULTY) {
			switch (input.MenuI) {
				case GAME_BUTTON_LEFT:
					ChangeSteps(input.pn, -1);
					m_bAcceptSelectRelease[input.pn] = false;
					break;
				case GAME_BUTTON_RIGHT:
					ChangeSteps(input.pn, +1);
					m_bAcceptSelectRelease[input.pn] = false;
					break;
				case GAME_BUTTON_START:
					m_bAcceptSelectRelease[input.pn] = false;
					if (MODE_MENU_AVAILABLE)
						m_MusicWheel.NextSort();
					else
						m_soundLocked.Play(true);
					break;
				default:
					break;
			}
		}
		if (input.type == IET_FIRST_PRESS &&
			input.MenuI != GAME_BUTTON_SELECT) {
			Message msg("SelectMenuInput");
			msg.SetParam("Player", input.pn);
			msg.SetParam(
			  "Button",
			  GameButtonToString(INPUTMAPPER->GetInputScheme(), input.MenuI));
			MESSAGEMAN->Broadcast(msg);
			m_bAcceptSelectRelease[input.pn] = false;
		}
		if (input.type == IET_FIRST_PRESS)
			g_CanOpenOptionsList.Touch();
		if (g_CanOpenOptionsList.Ago() > OPTIONS_LIST_TIMEOUT)
			m_bAcceptSelectRelease[input.pn] = false;
		return true;
	}

	if (m_SelectionState == SelectionState_SelectingSong &&
		(input.MenuI == m_GameButtonNextSong ||
		 input.MenuI == m_GameButtonPreviousSong ||
		 input.MenuI == GAME_BUTTON_SELECT)) {
		{
			// If we're rouletting, hands off.
			if (m_MusicWheel.IsRouletting())
				return false;

			bool bLeftIsDown = false;
			bool bRightIsDown = false;

			FOREACH_HumanPlayer(p)
			{
				if (m_OptionsList[p].IsOpened())
					continue;
				if (SELECT_MENU_AVAILABLE &&
					INPUTMAPPER->IsBeingPressed(GAME_BUTTON_SELECT, p))
					continue;

				bLeftIsDown |= static_cast<int>(
				  INPUTMAPPER->IsBeingPressed(m_GameButtonPreviousSong, p));
				bRightIsDown |= static_cast<int>(
				  INPUTMAPPER->IsBeingPressed(m_GameButtonNextSong, p));
			}

			bool bBothDown = bLeftIsDown && bRightIsDown;
			bool bNeitherDown = !bLeftIsDown && !bRightIsDown;

			if (bNeitherDown) {
				// Both buttons released.
				m_MusicWheel.Move(0);
			} else if (bBothDown) {
				m_MusicWheel.Move(0);
				if (input.type == IET_FIRST_PRESS) {
					if (input.MenuI == m_GameButtonPreviousSong)
						m_MusicWheel.ChangeMusicUnlessLocked(-1);
					else if (input.MenuI == m_GameButtonNextSong)
						m_MusicWheel.ChangeMusicUnlessLocked(+1);
				}
			} else if (bLeftIsDown) {
				if (input.type != IET_RELEASE) {
					MESSAGEMAN->Broadcast("PreviousSong");
					m_MusicWheel.Move(-1);
				}
			} else if (bRightIsDown) {
				if (input.type != IET_RELEASE) {
					MESSAGEMAN->Broadcast("NextSong");
					m_MusicWheel.Move(+1);
				}
			} else {
				FAIL_M("Logic bug: L/R keys in an impossible state?");
			}

			// Reset the repeat timer when the button is released.
			// This fixes jumping when you release Left and Right after entering
			// the sort code at the same if L & R aren't released at the exact
			// same time.
			if (input.type == IET_RELEASE) {
				INPUTMAPPER->ResetKeyRepeat(m_GameButtonPreviousSong, input.pn);
				INPUTMAPPER->ResetKeyRepeat(m_GameButtonNextSong, input.pn);
			}
		}
	}

	// To allow changing steps with gamebuttons, NOT WITH THE GODDAMN
	// CODEDETECTOR yeah, wanted this since a while ago... -DaisuMaster
	if (CHANGE_STEPS_WITH_GAME_BUTTONS) {
		// Avoid any event not being first press
		if (input.type != IET_FIRST_PRESS)
			return false;

		if (m_SelectionState == SelectionState_SelectingSong) {
			if (input.MenuI == m_GameButtonPreviousDifficulty) {
				ChangeSteps(input.pn, -1);
			} else if (input.MenuI == m_GameButtonNextDifficulty) {
				ChangeSteps(input.pn, +1);
			}
		}
	}

	// Actually I don't like to just copy and paste code because it may go
	// wrong if something goes overlooked -DaisuMaster
	if (CHANGE_GROUPS_WITH_GAME_BUTTONS) {
		if (input.type != IET_FIRST_PRESS)
			return false;

		if (m_SelectionState == SelectionState_SelectingSong) {
			if (input.MenuI == m_GameButtonPreviousGroup) {
				RString sNewGroup = m_MusicWheel.JumpToPrevGroup();
				m_MusicWheel.SelectSection(sNewGroup);
				m_MusicWheel.SetOpenSection(sNewGroup);
				MESSAGEMAN->Broadcast("PreviousGroup");
				AfterMusicChange();
			} else if (input.MenuI == m_GameButtonNextGroup) {
				RString sNewGroup = m_MusicWheel.JumpToNextGroup();
				m_MusicWheel.SelectSection(sNewGroup);
				m_MusicWheel.SetOpenSection(sNewGroup);
				MESSAGEMAN->Broadcast("NextGroup");
				AfterMusicChange();
			}
		}
	}

	if (m_SelectionState == SelectionState_SelectingSteps &&
		input.type == IET_FIRST_PRESS && !m_bStepsChosen[input.pn]) {
		if (input.MenuI == m_GameButtonNextSong ||
			input.MenuI == m_GameButtonPreviousSong) {
			if (input.MenuI == m_GameButtonPreviousSong) {
				ChangeSteps(input.pn, -1);
			} else if (input.MenuI == m_GameButtonNextSong) {
				ChangeSteps(input.pn, +1);
			}
		} else if (input.MenuI == GAME_BUTTON_MENUUP ||
				   input.MenuI ==
					 GAME_BUTTON_MENUDOWN) // &&
										   // TWO_PART_DESELECTS_WITH_MENUUPDOWN
		{
			// XXX: should this be called "TwoPartCancelled"?
			float fSeconds = m_MenuTimer->GetSeconds();
			if (fSeconds > 10) {
				Message msg("SongUnchosen");
				msg.SetParam("Player", input.pn);
				MESSAGEMAN->Broadcast(msg);
				// unset all steps
				FOREACH_ENUM(PlayerNumber, p)
				m_bStepsChosen[p] = false;
				m_SelectionState = SelectionState_SelectingSong;
			}
		}
	}

	if (input.type == IET_FIRST_PRESS && DetectCodes(input))
		return true;

	return ScreenWithMenuElements::Input(input);
}

bool
ScreenSelectMusic::DetectCodes(const InputEventPlus& input)
{
	if (CodeDetector::EnteredPrevSteps(input.GameI.controller) &&
		!CHANGE_STEPS_WITH_GAME_BUTTONS) {
		ChangeSteps(input.pn, -1);
	} else if (CodeDetector::EnteredNextSteps(input.GameI.controller) &&
			   !CHANGE_STEPS_WITH_GAME_BUTTONS) {
		ChangeSteps(input.pn, +1);
	} else if (CodeDetector::EnteredModeMenu(input.GameI.controller)) {
		if (MODE_MENU_AVAILABLE)
			m_MusicWheel.ChangeSort(SORT_MODE_MENU);
		else
			m_soundLocked.Play(true);
	} else if (CodeDetector::EnteredNextSort(input.GameI.controller)) {
		m_MusicWheel.NextSort();
	} else if (CodeDetector::DetectAndAdjustMusicOptions(
				 input.GameI.controller)) {
		m_soundOptionsChange.Play(true);

		Message msg("PlayerOptionsChanged");
		msg.SetParam("PlayerNumber", input.pn);
		MESSAGEMAN->Broadcast(msg);

		MESSAGEMAN->Broadcast("SongOptionsChanged");
	} else if (CodeDetector::EnteredNextGroup(input.GameI.controller) &&
			   !CHANGE_GROUPS_WITH_GAME_BUTTONS) {
		RString sNewGroup = m_MusicWheel.JumpToNextGroup();
		m_MusicWheel.SelectSection(sNewGroup);
		m_MusicWheel.SetOpenSection(sNewGroup);
		MESSAGEMAN->Broadcast("NextGroup");
		AfterMusicChange();
	} else if (CodeDetector::EnteredPrevGroup(input.GameI.controller) &&
			   !CHANGE_GROUPS_WITH_GAME_BUTTONS) {
		RString sNewGroup = m_MusicWheel.JumpToPrevGroup();
		m_MusicWheel.SelectSection(sNewGroup);
		m_MusicWheel.SetOpenSection(sNewGroup);
		MESSAGEMAN->Broadcast("PreviousGroup");
		AfterMusicChange();
	} else if (CodeDetector::EnteredCloseFolder(input.GameI.controller)) {
		RString sCurSection = m_MusicWheel.GetSelectedSection();
		m_MusicWheel.SelectSection(sCurSection);
		m_MusicWheel.SetOpenSection("");
		AfterMusicChange();
	} else {
		return false;
	}
	return true;
}

void
ScreenSelectMusic::UpdateSelectButton(PlayerNumber pn, bool bSelectIsDown)
{
	if (!SELECT_MENU_AVAILABLE || !CanChangeSong())
		bSelectIsDown = false;

	if (m_bSelectIsDown[pn] != bSelectIsDown) {
		m_bSelectIsDown[pn] = bSelectIsDown;
		Message msg(bSelectIsDown ? "SelectMenuOpened" : "SelectMenuClosed");
		msg.SetParam("Player", pn);
		MESSAGEMAN->Broadcast(msg);
	}
}

void
ScreenSelectMusic::ChangeSteps(PlayerNumber pn, int dir)
{
	LOG->Trace("ScreenSelectMusic::ChangeSteps( %d, %d )", pn, dir);

	ASSERT(GAMESTATE->IsHumanPlayer(pn));

	if (GAMESTATE->m_pCurSong) {
		m_iSelection[pn] += dir;
		if (WRAP_CHANGE_STEPS) {
			wrap(m_iSelection[pn], m_vpSteps.size());
		} else {
			if (CLAMP(m_iSelection[pn], 0, m_vpSteps.size() - 1))
				return;
		}

		// the user explicity switched difficulties. Update the preferred
		// Difficulty and StepsType
		Steps* pSteps = m_vpSteps[m_iSelection[pn]];
		GAMESTATE->ChangePreferredDifficultyAndStepsType(
		  pn, pSteps->GetDifficulty(), pSteps->m_StepsType);
	} else {
		// If we're showing multiple StepsTypes in the list, don't allow
		// changing the difficulty/StepsType when a non-Song, non-Course is
		// selected. Changing the preferred Difficulty and StepsType by
		// direction is complicated when multiple StepsTypes are being shown,
		// so we don't support it.
		if (CommonMetrics::AUTO_SET_STYLE)
			return;
		if (!GAMESTATE->ChangePreferredDifficulty(pn, dir))
			return;
	}

	vector<PlayerNumber> vpns;
	FOREACH_HumanPlayer(p)
	{
		if (pn == p || GAMESTATE->DifficultiesLocked()) {
			m_iSelection[p] = m_iSelection[pn];
			vpns.push_back(p);
		}
	}
	AfterStepsOrTrailChange(vpns);

	float fBalance = GameSoundManager::GetPlayerBalance(pn);
	if (dir < 0) {
		m_soundDifficultyEasier.SetProperty("Pan", fBalance);
		m_soundDifficultyEasier.PlayCopy(true);
	} else {
		m_soundDifficultyHarder.SetProperty("Pan", fBalance);
		m_soundDifficultyHarder.PlayCopy(true);
	}
	GAMESTATE->ForceOtherPlayersToCompatibleSteps(pn);

	Message msg("ChangeSteps");
	msg.SetParam("Player", pn);
	msg.SetParam("Direction", dir);
	MESSAGEMAN->Broadcast(msg);
}

void
ScreenSelectMusic::HandleMessage(const Message& msg)
{
	if (m_bRunning && msg == Message_PlayerJoined) {
		PlayerNumber master_pn = GAMESTATE->GetMasterPlayerNumber();
		// The current steps may no longer be playable. If one player has double
		// steps selected, they are no longer playable now that P2 has joined.

		// TODO: Invalidate the CurSteps only if they are no longer playable.
		// That way, after music change will clamp to the nearest in the
		// StepsDisplayList.
		GAMESTATE->m_pCurSteps[master_pn].SetWithoutBroadcast(NULL);
		FOREACH_ENUM(PlayerNumber, p)
		GAMESTATE->m_pCurSteps[p].SetWithoutBroadcast(NULL);

		/* If a course is selected, it may no longer be playable.
		 * Let MusicWheel know about the late join. */
		m_MusicWheel.PlayerJoined();

		AfterMusicChange();

		int iSel = 0;
		PlayerNumber pn;
		bool b = msg.GetParam("Player", pn);
		ASSERT(b);

		// load player profiles
		if (GAMESTATE->HaveProfileToLoad()) {
			GAMESTATE->LoadProfiles(
			  true); // I guess you could always load edits here...
			SCREENMAN->ZeroNextUpdate(); // be kind, don't skip frames if you
										 // can avoid it
		}

		m_iSelection[pn] = iSel;
		Steps* pSteps =
		  m_vpSteps.empty() ? nullptr : m_vpSteps[m_iSelection[pn]];

		GAMESTATE->m_pCurSteps[pn].Set(pSteps);
	}

	ScreenWithMenuElements::HandleMessage(msg);
}

void
ScreenSelectMusic::HandleScreenMessage(const ScreenMessage SM)
{
	if (SM == SM_AllowOptionsMenuRepeat) {
		m_bAllowOptionsMenuRepeat = true;
	} else if (SM == SM_MenuTimer) {
		if (m_MusicWheel.IsRouletting()) {
			MenuStart(InputEventPlus());
			m_MenuTimer->SetSeconds(ROULETTE_TIMER_SECONDS);
			m_MenuTimer->Start();
		} else if (DO_ROULETTE_ON_MENU_TIMER &&
				   m_MusicWheel.GetSelectedSong() == nullptr) {
			m_MenuTimer->SetSeconds(ROULETTE_TIMER_SECONDS);
			m_MenuTimer->Start();
		} else {
			// Finish sort changing so that the wheel can respond immediately to
			// our request to choose random.
			m_MusicWheel.FinishChangingSorts();

			MenuStart(InputEventPlus());
		}
		return;
	} else if (SM == SM_GoToPrevScreen) {
		/* We may have stray SM_SongChanged messages from the music wheel.
		 * We can't handle them anymore, since the title menu (and attract
		 * screens) reset the game state, so just discard them. */
		ClearMessageQueue();
	} else if (SM == SM_BeginFadingOut) {
		m_bAllowOptionsMenu = false;
		if (OPTIONS_MENU_AVAILABLE && !m_bGoToOptions)
			this->PlayCommand("HidePressStartForOptions");
		GAMESTATE->m_bInNetGameplay = false;
		this->PostScreenMessage(SM_GoToNextScreen, this->GetTweenTimeLeft());
	} else if (SM == SM_GoToNextScreen) {
		if (!m_bGoToOptions)
			SOUND->StopMusic();
	} else if (SM == SM_SongChanged) {
		AfterMusicChange();
	} else if (SM == SM_SortOrderChanging) // happens immediately
	{
		this->PlayCommand("SortChange");
	} else if (SM == SM_GainFocus) {
#if !defined(WITHOUT_NETWORKING)
		DLMAN->UpdateDLSpeed(false);
#endif
		CodeDetector::RefreshCacheItems(CODES);
	} else if (SM == SM_LoseFocus) {
		CodeDetector::RefreshCacheItems(); // reset for other screens
	} else if (SM == SM_ConfirmDeleteSong) {
		if (ScreenPrompt::s_LastAnswer == ANSWER_YES) {
			OnConfirmSongDeletion();
		} else {
			// need to resume the song preview that was automatically paused
			m_MusicWheel.ChangeMusic(0);
		}
	}

	if (SM == SM_BackFromNamePlaylist) {
		Playlist pl;
		pl.name = ScreenTextEntry::s_sLastAnswer;
		if (pl.name != "") {
			SONGMAN->GetPlaylists().emplace(pl.name, pl);
			SONGMAN->activeplaylist = pl.name;
			Message msg("DisplayAll");
			MESSAGEMAN->Broadcast(msg);
		}
	}

	ScreenWithMenuElements::HandleScreenMessage(SM);
}

bool
ScreenSelectMusic::MenuStart(const InputEventPlus& input)
{
	if (input.type != IET_FIRST_PRESS)
		return false;

	/* If select is being pressed at the same time, this is probably an attempt
	 * to change the sort, not to pick a song or difficulty. If it gets here,
	 * the actual select press was probably hit during a tween and ignored.
	 * Ignore it. */
	if (input.pn != PLAYER_INVALID &&
		INPUTMAPPER->IsBeingPressed(GAME_BUTTON_SELECT, input.pn))
		return false;

	// Honor locked input for start presses.
	if (m_fLockInputSecs > 0)
		return false;

	return SelectCurrent(input.pn);
}

bool
ScreenSelectMusic::SelectCurrent(PlayerNumber pn)
{

	switch (m_SelectionState) {
		case SelectionState_Finalized: {
			LOG->Warn("song selection made while selectionstate_finalized");
			return false;
		}
		case SelectionState_SelectingSong:
			// If false, we don't have a selection just yet.
			if (!m_MusicWheel.Select())
				return false;

			// a song was selected
			if (m_MusicWheel.GetSelectedSong() != nullptr) {
				if (SAMPLE_MUSIC_PREVIEW_MODE ==
					SampleMusicPreviewMode_StartToPreview) {
					// start playing the preview music.
					g_bSampleMusicWaiting = true;
					CheckBackgroundRequests(true);
				}

				const bool bIsNew =
				  PROFILEMAN->IsSongNew(m_MusicWheel.GetSelectedSong());
				bool bIsHard = false;
				FOREACH_HumanPlayer(p)
				{
					if (GAMESTATE->m_pCurSteps[p] &&
						GAMESTATE->m_pCurSteps[p]->GetMeter() >=
						  HARD_COMMENT_METER)
						bIsHard = true;
				}

				// See if this song is a repeat.
				// If we're in event mode, only check the last five songs.
				bool bIsRepeat = false;
				int i = 0;
				if (GAMESTATE->IsEventMode())
					i = max(0, int(STATSMAN->m_vPlayedStageStats.size()) - 5);
				for (; i < (int)STATSMAN->m_vPlayedStageStats.size(); ++i)
					if (STATSMAN->m_vPlayedStageStats[i]
						  .m_vpPlayedSongs.back() ==
						m_MusicWheel.GetSelectedSong())
						bIsRepeat = true;

				if (bIsRepeat)
					SOUND->PlayOnceFromAnnouncer("select music comment repeat");
				else if (bIsNew)
					SOUND->PlayOnceFromAnnouncer("select music comment new");
				else if (bIsHard)
					SOUND->PlayOnceFromAnnouncer("select music comment hard");
				else
					SOUND->PlayOnceFromAnnouncer(
					  "select music comment general");

			} else {
				// We haven't made a selection yet.
				return false;
			}
			// I believe this is for those who like pump pro. -aj
			MESSAGEMAN->Broadcast("SongChosen");

			break;

		case SelectionState_SelectingSteps: {
			bool bInitiatedByMenuTimer = pn == PLAYER_INVALID;
			bool bAllOtherHumanPlayersDone = true;
			FOREACH_HumanPlayer(p)
			{
				if (p == pn)
					continue;
				bAllOtherHumanPlayersDone &=
				  static_cast<int>(m_bStepsChosen[p]);
			}

			bool bAllPlayersDoneSelectingSteps =
			  bInitiatedByMenuTimer || bAllOtherHumanPlayersDone;

			if (!bAllPlayersDoneSelectingSteps) {
				m_bStepsChosen[pn] = true;
				m_soundStart.Play(true);

				// impldiff: Pump it Up Pro uses "StepsSelected". -aj
				Message msg("StepsChosen");
				msg.SetParam("Player", pn);
				MESSAGEMAN->Broadcast(msg);
				return true;
			}
		} break;
	}

	FOREACH_ENUM(PlayerNumber, p)
	{
		if (m_SelectionState == SelectionState_SelectingSteps) {
			if (m_OptionsList[p].IsOpened())
				m_OptionsList[p].Close();
		}
		UpdateSelectButton(p, false);
	}

	m_SelectionState = GetNextSelectionState();
	Message msg("Start" + SelectionStateToString(m_SelectionState));
	MESSAGEMAN->Broadcast(msg);

	m_soundStart.Play(true);

	if (m_SelectionState == SelectionState_Finalized) {
#if !defined(WITHOUT_NETWORKING)
		DLMAN->UpdateDLSpeed(true);
#endif
		m_MenuTimer->Stop();

		if (GAMESTATE->m_bIsChartPreviewActive) {
			MESSAGEMAN->Broadcast("hELPidontDNOKNOW");
		} // we dont know who owns the notefield preview so we broadcast to get
		  // the owner to submit itself for deletion -mina

		FOREACH_HumanPlayer(p)
		{
			if (!m_bStepsChosen[p]) {
				m_bStepsChosen[p] = true;
				// Don't play start sound. We play it again below on finalized
				// m_soundStart.Play(true);

				Message lMsg("StepsChosen");
				lMsg.SetParam("Player", p);
				MESSAGEMAN->Broadcast(lMsg);
			}
		}

		// Now that Steps have been chosen, set a Style that can play them.
		GAMESTATE->SetCompatibleStylesForPlayers();
		GAMESTATE->ForceSharedSidesMatch();

		/* If we're currently waiting on song assets, abort all except the music
		 * and start the music, so if we make a choice quickly before background
		 * requests come through, the music will still start. */
		m_BackgroundLoader.Abort();
		CheckBackgroundRequests(true);
		m_MusicWheel.Lock();
		if (OPTIONS_MENU_AVAILABLE) {
			// show "hold START for options"
			this->PlayCommand("ShowPressStartForOptions");

			m_bAllowOptionsMenu = true;

			/* Don't accept a held START for a little while, so it's not
			 * hit accidentally.  Accept an initial START right away, though,
			 * so we don't ignore deliberate fast presses (which would be
			 * annoying). */
			if (PREFSMAN->m_AllowHoldForOptions.Get()) {
				this->PostScreenMessage(SM_AllowOptionsMenuRepeat, 0.5f);
			}

			StartTransitioningScreen(SM_None);
			float fTime =
			  max(SHOW_OPTIONS_MESSAGE_SECONDS, this->GetTweenTimeLeft());
			this->PostScreenMessage(SM_BeginFadingOut, fTime);
		} else {
			StartTransitioningScreen(SM_BeginFadingOut);
		}
	}
	return false;
}

bool
ScreenSelectMusic::MenuBack(const InputEventPlus& /* input */)
{
	m_BackgroundLoader.Abort();
	Cancel(SM_GoToPrevScreen);
	return true;
}

void
ScreenSelectMusic::AfterStepsOrTrailChange(const vector<PlayerNumber>& vpns)
{
	PlayerNumber pn = PLAYER_1;
	ASSERT(GAMESTATE->IsHumanPlayer(pn));

	if (GAMESTATE->m_pCurSong) {
		CLAMP(m_iSelection[pn], 0, m_vpSteps.size() - 1);

		Song* pSong = GAMESTATE->m_pCurSong;
		Steps* pSteps =
		  m_vpSteps.empty() ? nullptr : m_vpSteps[m_iSelection[pn]];

		GAMESTATE->m_pCurSteps[pn].Set(pSteps);
		if (pSteps != nullptr)
			GAMESTATE->SetCompatibleStyle(pSteps->m_StepsType, pn);

		int iScore = 0;
		if (pSteps) {
			const Profile* pProfile = PROFILEMAN->GetProfile(pn);
			iScore = pProfile->GetStepsHighScoreList(pSong, pSteps)
					   .GetTopScore()
					   .GetScore();
			if (m_pPreviewNoteField != nullptr) {
				pSteps->GetNoteData(m_PreviewNoteData);
				m_pPreviewNoteField->Load(&m_PreviewNoteData, 0, 800);
			}
			delayedchartupdatewaiting = true;
		}
	}
}

void
ScreenSelectMusic::SwitchToPreferredDifficulty()
{

	FOREACH_HumanPlayer(pn)
	{
		// Find the closest match to the user's preferred difficulty and
		// StepsType.
		int iCurDifference = -1;
		int& iSelection = m_iSelection[pn];
		FOREACH_CONST(Steps*, m_vpSteps, s)
		{
			int i = s - m_vpSteps.begin();

			// If the current steps are listed, use them.
			if (GAMESTATE->m_pCurSteps[pn] == *s) {
				iSelection = i;
				break;
			}

			if (GAMESTATE->m_PreferredDifficulty[pn] != Difficulty_Invalid) {
				int iDifficultyDifference = abs(
				  (*s)->GetDifficulty() - GAMESTATE->m_PreferredDifficulty[pn]);
				int iStepsTypeDifference = 0;
				if (GAMESTATE->m_PreferredStepsType != StepsType_Invalid)
					iStepsTypeDifference =
					  abs((*s)->m_StepsType - GAMESTATE->m_PreferredStepsType);
				int iTotalDifference =
				  iStepsTypeDifference * NUM_Difficulty + iDifficultyDifference;

				if (iCurDifference == -1 || iTotalDifference < iCurDifference) {
					iSelection = i;
					iCurDifference = iTotalDifference;
				}
			}
		}

		CLAMP(iSelection, 0, m_vpSteps.size() - 1);
	}

	if (GAMESTATE->DifficultiesLocked()) {
		FOREACH_HumanPlayer(p) m_iSelection[p] =
		  m_iSelection[GAMESTATE->GetMasterPlayerNumber()];
	}
}

void
ScreenSelectMusic::AfterMusicChange()
{
	if (!m_MusicWheel.IsRouletting())
		m_MenuTimer->Stall();

	Song* pSong = m_MusicWheel.GetSelectedSong();
	GAMESTATE->m_pCurSong.Set(pSong);
	if (pSong != nullptr)
		GAMESTATE->m_pPreferredSong = pSong;
	else {
		GAMESTATE->m_pCurSteps[PLAYER_1].Set(nullptr);
		if (m_pPreviewNoteField) {
			m_pPreviewNoteField->SetVisible(false);
			// if previewnotefield is active and we are moving out of a pack
			// into the pack list (that's what this block of code is for
			// handling) manually call songmans cleanup function (compresses all
			// steps); we could optimize by only compressing the pack but this
			// is pretty fast anyway -mina
			SONGMAN->Cleanup();
		}
	}

	GAMESTATE->SetPaused(false); // hacky can see this being problematic
								 // if we forget about it -mina

	m_vpSteps.clear();
	vector<RString> m_Artists, m_AltArtists;

	if (SAMPLE_MUSIC_PREVIEW_MODE != SampleMusicPreviewMode_LastSong) {
		m_sSampleMusicToPlay = "";
	}
	m_pSampleMusicTimingData = nullptr;

	static SortOrder s_lastSortOrder = SortOrder_Invalid;
	if (GAMESTATE->m_SortOrder != s_lastSortOrder) {
		// Reload to let Lua metrics have a chance to change the help text.
		s_lastSortOrder = GAMESTATE->m_SortOrder;
	}

	WheelItemDataType wtype = m_MusicWheel.GetSelectedType();
	SampleMusicPreviewMode pmode;
	switch (wtype) {
		case WheelItemDataType_Section:
		case WheelItemDataType_Sort:
		case WheelItemDataType_Roulette:
		case WheelItemDataType_Random:
		case WheelItemDataType_Custom:
			FOREACH_PlayerNumber(p) m_iSelection[p] = -1;
			if (SAMPLE_MUSIC_PREVIEW_MODE == SampleMusicPreviewMode_LastSong) {
				// HACK: Make random music work in LastSong mode. -aj
				if (m_sSampleMusicToPlay == m_sRandomMusicPath) {
					m_fSampleStartSeconds = 0;
					m_fSampleLengthSeconds = -1;
				}
			} else {
				m_fSampleStartSeconds = 0;
				m_fSampleLengthSeconds = -1;
			}

			switch (wtype) {
				case WheelItemDataType_Section:
					// reduce scope
					{
						if (SAMPLE_MUSIC_PREVIEW_MODE !=
							SampleMusicPreviewMode_LastSong)
							m_sSampleMusicToPlay = m_sSectionMusicPath;
					}
					break;
				case WheelItemDataType_Sort:
					if (SAMPLE_MUSIC_PREVIEW_MODE !=
						SampleMusicPreviewMode_LastSong)
						m_sSampleMusicToPlay = m_sSortMusicPath;
					break;
				case WheelItemDataType_Roulette:
					if (SAMPLE_MUSIC_PREVIEW_MODE !=
						SampleMusicPreviewMode_LastSong)
						m_sSampleMusicToPlay = m_sRouletteMusicPath;
					break;
				case WheelItemDataType_Random:
					// if( SAMPLE_MUSIC_PREVIEW_MODE !=
					// SampleMusicPreviewMode_LastSong )
					m_sSampleMusicToPlay = m_sRandomMusicPath;
					break;
				case WheelItemDataType_Custom: {
					if (SAMPLE_MUSIC_PREVIEW_MODE !=
						SampleMusicPreviewMode_LastSong)
						m_sSampleMusicToPlay = m_sSectionMusicPath;
				} break;
				default:
					FAIL_M(ssprintf("Invalid WheelItemDataType: %i", wtype));
			}
			// override this if the sample music mode wants to.
			/*
			if(SAMPLE_MUSIC_PREVIEW_MODE == SampleMusicPreviewMode_LastSong)
			{
			m_sSampleMusicToPlay = pSong->GetMusicPath();
			m_pSampleMusicTimingData = &pSong->m_SongTiming;
			m_fSampleStartSeconds = pSong->m_fMusicSampleStartSeconds;
			m_fSampleLengthSeconds = pSong->m_fMusicSampleLengthSeconds;
			}
			*/
			break;
		case WheelItemDataType_Song:
		case WheelItemDataType_Portal:
			// check SampleMusicPreviewMode here.
			pmode = SAMPLE_MUSIC_PREVIEW_MODE;
			switch (pmode) {
				case SampleMusicPreviewMode_ScreenMusic:
					// play the screen music
					m_sSampleMusicToPlay = m_sLoopMusicPath;
					m_fSampleStartSeconds = 0;
					m_fSampleLengthSeconds = -1;
					break;
				case SampleMusicPreviewMode_StartToPreview:
					// we want to load the sample music, but we don't want to
					// actually play it. fall through. -aj
				case SampleMusicPreviewMode_Normal:
				case SampleMusicPreviewMode_LastSong: // fall through
													  // play the sample music
					m_sSampleMusicToPlay = pSong->GetPreviewMusicPath();
					if (!m_sSampleMusicToPlay.empty() &&
						ActorUtil::GetFileType(m_sSampleMusicToPlay) !=
						  FT_Sound) {
						LuaHelpers::ReportScriptErrorFmt(
						  "Music file %s for song is not a sound file, "
						  "ignoring.",
						  m_sSampleMusicToPlay.c_str());
						m_sSampleMusicToPlay = "";
					}
					m_pSampleMusicTimingData = &pSong->m_SongTiming;
					m_fSampleStartSeconds = pSong->GetPreviewStartSeconds();
					m_fSampleLengthSeconds = pSong->m_fMusicSampleLengthSeconds;
					break;
				default:
					FAIL_M(ssprintf("Invalid preview mode: %i", pmode));
			}

			SongUtil::GetPlayableSteps(pSong, m_vpSteps);
			if (m_vpSteps.empty()) {
				// LuaHelpers::ReportScriptError("GetPlayableSteps returned
				// nothing.");
			}

			SwitchToPreferredDifficulty();
			break;
		default:
			FAIL_M(ssprintf("Invalid WheelItemDataType: %i", wtype));
	}

	// Cancel any previous, incomplete requests for song assets,
	// since we need new ones.
	m_BackgroundLoader.Abort();

	// Don't stop music if it's already playing the right file.
	g_bSampleMusicWaiting = false;
	if (!m_MusicWheel.IsRouletting() &&
		SOUND->GetMusicPath() != m_sSampleMusicToPlay) {
		SOUND->StopMusic();
		// some SampleMusicPreviewModes don't want the sample music immediately.
		if (SAMPLE_MUSIC_PREVIEW_MODE !=
			SampleMusicPreviewMode_StartToPreview) {
			if (!m_sSampleMusicToPlay.empty())
				g_bSampleMusicWaiting = true;
		}
	}

	g_StartedLoadingAt.Touch();

	vector<PlayerNumber> vpns;
	FOREACH_HumanPlayer(p) vpns.push_back(p);

	AfterStepsOrTrailChange(vpns);
}

void
ScreenSelectMusic::OpenOptionsList(PlayerNumber pn)
{
	if (pn != PLAYER_INVALID) {
		m_MusicWheel.Move(0);
		m_OptionsList[pn].Open();
	}
}

void
ScreenSelectMusic::OnConfirmSongDeletion()
{
	Song* deletedSong = m_pSongAwaitingDeletionConfirmation;
	if (deletedSong == nullptr) {
		LOG->Warn("Attempted to delete a null song "
				  "(ScreenSelectMusic::OnConfirmSongDeletion)");
		return;
	}
	// ensure Stepmania is configured to allow song deletion
	if (!PREFSMAN->m_bAllowSongDeletion.Get()) {
		LOG->Warn("Attemped to delete a song but AllowSongDeletion was set to "
				  "false (ScreenSelectMusic::OnConfirmSongDeletion)");
		return;
	}

	RString deleteDir = deletedSong->GetSongDir();
	// flush the deleted song from any caches
	SONGMAN->UnlistSong(deletedSong);
	// refresh the song list
	m_MusicWheel.ReloadSongList(false, "");
	LOG->Trace("Deleting song: '%s'\n", deleteDir.c_str());
	// delete the song directory from disk
	FILEMAN->DeleteRecursive(deleteDir);

	m_pSongAwaitingDeletionConfirmation = nullptr;
}

bool
ScreenSelectMusic::can_open_options_list(PlayerNumber pn)
{
	if (!USE_OPTIONS_LIST) {
		return false;
	}
	if (pn >= NUM_PLAYERS) {
		return false;
	}
	if (m_OptionsList[pn].IsOpened()) {
		return false;
	}
	return true;
}

int
ScreenSelectMusic::GetSelectionState()
{
	return static_cast<int>(m_SelectionState);
}

void
ScreenSelectMusic::GeneratePreviewNoteField()
{
	if (m_pPreviewNoteField != nullptr)
		return;
	auto song = GAMESTATE->m_pCurSong;
	Steps* steps = GAMESTATE->m_pCurSteps[PLAYER_1];

	if (song && steps) {
		steps->GetNoteData(m_PreviewNoteData);
	} else {
		return;
	}

	GAMESTATE->m_bIsChartPreviewActive = true;

	// Create and Render the NoteField afterwards
	// It is done in this order so we don't see it before the music changes.
	m_pPreviewNoteField = new NoteField;
	m_pPreviewNoteField->SetName(
	  "NoteField"); // Use this to get the ActorFrame from the Screen Children

	m_pPreviewNoteField->Init(GAMESTATE->m_pPlayerState[PLAYER_1], 100);
	m_pPreviewNoteField->Load(&m_PreviewNoteData, 0, 800);
}

void
ScreenSelectMusic::DeletePreviewNoteField()
{
	if (m_pPreviewNoteField != nullptr) {
		SAFE_DELETE(m_pPreviewNoteField);
		GAMESTATE->m_bIsChartPreviewActive = false;
		auto song = GAMESTATE->m_pCurSong;
		if (song && m_SelectionState != SelectionState_Finalized) {
			SOUND->StopMusic();
			m_sSampleMusicToPlay = song->GetPreviewMusicPath();
			m_fSampleStartSeconds = song->GetPreviewStartSeconds();
			m_fSampleLengthSeconds = song->m_fMusicSampleLengthSeconds;
			g_bSampleMusicWaiting = true;
			CheckBackgroundRequests(true);
		}
	}
}

void
ScreenSelectMusic::SetPreviewNoteFieldMusicPosition(float given)
{
	if (m_pPreviewNoteField != nullptr && GAMESTATE->m_bIsChartPreviewActive) {
		RageSound* pMusic = SOUND->GetRageSoundPlaying();
		pMusic->SetPositionSeconds(given);
		if (GAMESTATE->GetPaused())
			SOUND->GetRageSoundPlaying()->Pause(true);
	}
}

void
ScreenSelectMusic::PausePreviewNoteFieldMusic()
{
	bool paused = GAMESTATE->GetPaused();
	SOUND->GetRageSoundPlaying()->Pause(!paused);
	GAMESTATE->SetPaused(!paused);
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the ScreenSelectMusic. */
class LunaScreenSelectMusic : public Luna<ScreenSelectMusic>
{
  public:
	static int GetGoToOptions(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->GetGoToOptions());
		return 1;
	}
	static int GetMusicWheel(T* p, lua_State* L)
	{
		p->GetMusicWheel()->PushSelf(L);
		return 1;
	}
	static int OpenOptionsList(T* p, lua_State* L)
	{
		PlayerNumber pn = Enum::Check<PlayerNumber>(L, 1);
		if (p->can_open_options_list(pn)) {
			p->OpenOptionsList(pn);
		}
		COMMON_RETURN_SELF;
	}
	static int CanOpenOptionsList(T* p, lua_State* L)
	{
		PlayerNumber pn = Enum::Check<PlayerNumber>(L, 1);
		lua_pushboolean(L, p->can_open_options_list(pn));
		return 1;
	}
	static int SelectCurrent(T* p, lua_State* L)
	{
		p->SelectCurrent(Enum::Check<PlayerNumber>(L, 1));
		return 1;
	}

	static int GetSelectionState(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetSelectionState());
		return 1;
	}

	static int StartPlaylistAsCourse(T* p, lua_State* L)
	{
		string name = SArg(1);
		Playlist& pl = SONGMAN->GetPlaylists()[name];

		// don't allow empty playlists to be started as a course
		if (pl.chartlist.empty())
			return 1;

		// dont allow playlists with an unloaded chart to be played as a course
		FOREACH(Chart, pl.chartlist, ch)
		if (!ch->loaded)
			return 1;

		SONGMAN->playlistcourse = name;
		GAMESTATE->isplaylistcourse = true;
		p->GetMusicWheel()->SelectSong(pl.chartlist[0].songptr);
		GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate =
		  pl.chartlist[0].rate;
		MESSAGEMAN->Broadcast("RateChanged");
		p->SelectCurrent(PLAYER_1);
		return 1;
	}

	static int PlayReplay(T* p, lua_State* L)
	{
		// get the highscore from lua and make the AI load it
		HighScore* hs = Luna<HighScore>::check(L, 1);

		// we get timestamps not noterows when getting online replays from the
		// site, since order is deterministic we'll just auto set the noterows
		// from the existing, if the score was cc off then we need to fill in
		// extra rows for each tap in the chord -mina
		auto timestamps = hs->GetCopyOfSetOnlineReplayTimestampVector();
		auto noterows = hs->GetNoteRowVector();
		auto REEEEEEEEEEEEEE = hs->GetOffsetVector();
		if (!timestamps.empty() &&
			noterows.empty()) { // if we have noterows from newer uploads, just
								// use them -mina
			GAMESTATE->SetProcessedTimingData(
			  GAMESTATE->m_pCurSteps[PLAYER_1]->GetTimingData());
			auto* td = GAMESTATE->m_pCurSteps[PLAYER_1]->GetTimingData();
			// vector<int> ihatemylife;
			auto nd = GAMESTATE->m_pCurSteps[PLAYER_1]->GetNoteData();
			auto nerv = nd.BuildAndGetNerv();
			/* functionally dead code, may be removed -poco
			if (!hs->GetChordCohesion()) {
				for (auto r : nerv)
					for (int i = 0; i < nd.GetNumTapNotesInRow(r); ++i)
						ihatemylife.emplace_back(r);
			} else {
				for (auto r : nerv)
					ihatemylife.emplace_back(r);
			}
			*/
			auto sdifs = td->BuildAndGetEtaner(nerv);
			vector<int> noterows;
			for (auto t : timestamps) {
				auto timestamptobeat =
				  td->GetBeatFromElapsedTime(t * hs->GetMusicRate());
				auto somenumberscaledbyoffsets =
				  sdifs[0] - (timestamps[0] * hs->GetMusicRate());
				timestamptobeat += somenumberscaledbyoffsets;
				auto noterowfrombeat = BeatToNoteRow(timestamptobeat);
				noterows.emplace_back(noterowfrombeat);
			}
			int noterowoffsetter = nerv[0] - noterows[0];
			for (auto& noterowwithoffset : noterows)
				noterowwithoffset += noterowoffsetter;
			GAMESTATE->SetProcessedTimingData(nullptr);
			// hs->SetNoteRowVector(ihatemylife);
			hs->SetNoteRowVector(noterows);
		}

		// Since we keep misses on EO as 180ms, we need to convert them
		// back.
		if (!timestamps.empty()) {
			auto offsets = hs->GetCopyOfOffsetVector();
			for (auto& offset : offsets) {
				if (fabs(offset) >= .18f)
					offset = -1.1f; // This is a miss to the replay reader.
			}
			hs->SetOffsetVector(offsets);
		}

		PlayerAI::SetScoreData(hs);

		// prepare old mods to return to
		const RString oldMods = GAMESTATE->m_pPlayerState[PLAYER_1]
								  ->m_PlayerOptions.GetCurrent()
								  .GetString();

		// set the heck out of the current rate to make sure everything runs
		// correctly
		float scoreRate = hs->GetMusicRate();
		float oldRate = GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate;
		GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = scoreRate;
		GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = scoreRate;
		GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = scoreRate;
		MESSAGEMAN->Broadcast("RateChanged");

		// set mods based on the score, hopefully
		// it is known that xmod->cmod and back does not work most of the time.
		/*
		CHECKPOINT_M("Setting mods for Replay Viewing.");
		RString mods = hs->GetModifiers();
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.GetSong().FromString(mods);
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.GetCurrent().FromString(mods);
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.GetPreferred().FromString(mods);
		CHECKPOINT_M("Replay mods set.");
		*/

		// Set mirror mode on if mirror was on in the replay
		// Also get ready to reset the turn mods to what they were before
		RString mods = hs->GetModifiers();
		vector<RString> oldTurns;
		GAMESTATE->m_pPlayerState[PLAYER_1]
		  ->m_PlayerOptions.GetSong()
		  .GetTurnMods(oldTurns);
		if (mods.find("Mirror") != mods.npos) {
			GAMESTATE->m_pPlayerState[PLAYER_1]
			  ->m_PlayerOptions.GetSong()
			  .m_bTurns[PlayerOptions::TURN_MIRROR] = true;
			GAMESTATE->m_pPlayerState[PLAYER_1]
			  ->m_PlayerOptions.GetCurrent()
			  .m_bTurns[PlayerOptions::TURN_MIRROR] = true;
			GAMESTATE->m_pPlayerState[PLAYER_1]
			  ->m_PlayerOptions.GetPreferred()
			  .m_bTurns[PlayerOptions::TURN_MIRROR] = true;
		} else {
			GAMESTATE->m_pPlayerState[PLAYER_1]
			  ->m_PlayerOptions.GetSong()
			  .m_bTurns[PlayerOptions::TURN_MIRROR] = false;
			GAMESTATE->m_pPlayerState[PLAYER_1]
			  ->m_PlayerOptions.GetCurrent()
			  .m_bTurns[PlayerOptions::TURN_MIRROR] = false;
			GAMESTATE->m_pPlayerState[PLAYER_1]
			  ->m_PlayerOptions.GetPreferred()
			  .m_bTurns[PlayerOptions::TURN_MIRROR] = false;
		}
		GAMEMAN->m_bResetTurns = true;
		GAMEMAN->m_vTurnsToReset = oldTurns;
		GAMEMAN->m_iPreviousFail = GAMESTATE->m_pPlayerState[PLAYER_1]
									 ->m_PlayerOptions.GetSong()
									 .m_FailType;

		// REALLY BAD way to set fail off for a replay
		GAMESTATE->m_pPlayerState[PLAYER_1]
		  ->m_PlayerOptions.GetSong()
		  .m_FailType = FailType_Off;
		GAMESTATE->m_pPlayerState[PLAYER_1]
		  ->m_PlayerOptions.GetCurrent()
		  .m_FailType = FailType_Off;
		GAMESTATE->m_pPlayerState[PLAYER_1]
		  ->m_PlayerOptions.GetPreferred()
		  .m_FailType = FailType_Off;

		// lock the game into replay mode and GO
		LOG->Trace("Viewing replay for score key %s",
				   hs->GetScoreKey().c_str());
		GamePreferences::m_AutoPlay.Set(PC_REPLAY);
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerController = PC_REPLAY;
		p->SelectCurrent(PLAYER_1);

		// set mods back to what they were before
		GAMEMAN->m_bResetModifiers = true;
		GAMEMAN->m_fPreviousRate = oldRate;
		GAMEMAN->m_sModsToReset = oldMods;
		return 1;
	}

	static int ShowEvalScreenForScore(T* p, lua_State* L)
	{
		// get the highscore from lua and fake it to the most recent score
		HighScore* hs = Luna<HighScore>::check(L, 1);
		SCOREMAN->PutScoreAtTheTop(hs->GetScoreKey());

		// set to replay mode to disable score saving
		GamePreferences::m_AutoPlay.Set(PC_REPLAY);

		// construct the current stage stats and stuff to the best of our
		// ability
		StageStats ss;
		RadarValues rv;
		NoteData nd;
		Steps* steps = GAMESTATE->m_pCurSteps[PLAYER_1];
		steps->GetNoteData(nd);
		float songlength = GAMESTATE->m_pCurSong->m_fMusicLengthSeconds;
		ss.Init();
		SCOREMAN->camefromreplay =
		  false; // disallow viewing online score eval screens -mina
		auto score = SCOREMAN->GetMostRecentScore();
		score->LoadReplayData();
		PlayerAI::SetScoreData(score);

		auto& pss = ss.m_player[0];
		pss.m_HighScore = *score;
		pss.CurWifeScore = score->GetWifeScore();
		pss.m_fWifeScore = score->GetWifeScore();
		pss.m_vNoteRowVector = score->GetNoteRowVector();
		pss.m_vOffsetVector = score->GetOffsetVector();
		pss.m_vTapNoteTypeVector = score->GetTapNoteTypeVector();
		pss.m_vTrackVector = score->GetTrackVector();
		// score->UnloadReplayData();
		pss.m_iSongsPassed = 1;
		pss.m_iSongsPlayed = 1;
		GAMESTATE->SetProcessedTimingData(
		  GAMESTATE->m_pCurSteps[PLAYER_1]->GetTimingData());
		NoteDataUtil::CalculateRadarValues(nd, songlength, rv);
		pss.m_radarPossible += rv;
		RadarValues realRV;
		PlayerAI::CalculateRadarValuesForReplay(realRV, rv);
		score->SetRadarValues(realRV);
		pss.m_radarActual += realRV;
		GAMESTATE->SetProcessedTimingData(nullptr);
		pss.everusedautoplay = true;
		for (int i = TNS_Miss; i < NUM_TapNoteScore; i++) {
			pss.m_iTapNoteScores[i] = score->GetTapNoteScore((TapNoteScore)i);
		}
		for (int i = 0; i < NUM_HoldNoteScore; i++) {
			pss.m_iHoldNoteScores[i] =
			  score->GetHoldNoteScore((HoldNoteScore)i);
		}
		ss.m_vpPlayedSongs.emplace_back(GAMESTATE->m_pCurSong);
		STATSMAN->m_CurStageStats = ss;
		STATSMAN->m_vPlayedStageStats.emplace_back(ss);

		// set the rate so the MSD and rate display doesnt look weird
		float scoreRate = hs->GetMusicRate();
		float oldRate = GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate;
		GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = scoreRate;
		GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = scoreRate;
		GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = scoreRate;
		MESSAGEMAN->Broadcast("RateChanged");

		GAMEMAN->m_iPreviousFail = GAMESTATE->m_pPlayerState[PLAYER_1]
									 ->m_PlayerOptions.GetSong()
									 .m_FailType;

		// go
		LOG->Trace("Viewing evaluation screen for score key %s",
				   score->GetScoreKey().c_str());
		SCREENMAN->SetNewScreen("ScreenEvaluationNormal");

		// set rate back to what it was before
		GAMEMAN->m_bResetModifiers = true;
		GAMEMAN->m_fPreviousRate = oldRate;

		return 1;
	}

	// This will return the Preview Notefield if it is successful.
	static int CreatePreviewNoteField(T* p, lua_State* L)
	{
		float helloiamafloat = GAMESTATE->m_pPlayerState[PLAYER_1]
								 ->GetDisplayedPosition()
								 .m_fMusicSeconds;
		p->GeneratePreviewNoteField();
		if (p->m_pPreviewNoteField != nullptr) {
			p->SetPreviewNoteFieldMusicPosition(helloiamafloat);
			p->m_pPreviewNoteField->PushSelf(L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	// This will delete the Preview Notefield if it exists.
	// NOTE: This is triggered by a DeletePreviewNoteField Message.
	// It is not necessary to use this except for rare circumstances.
	static int DeletePreviewNoteField(T* p, lua_State* L)
	{
		ActorFrame* king = Luna<ActorFrame>::check(L, 1);
		king->RemoveChild(p->m_pPreviewNoteField);
		p->DeletePreviewNoteField();
		return 0;
	}

	// Get the Preview Notefield ActorFrame if it exists.
	static int GetPreviewNoteField(T* p, lua_State* L)
	{
		if (p->m_pPreviewNoteField != nullptr) {
			p->m_pPreviewNoteField->PushSelf(L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	static int SetPreviewNoteFieldMusicPosition(T* p, lua_State* L)
	{
		float given = FArg(1);
		if (GAMESTATE->m_bIsChartPreviewActive) {
			p->SetPreviewNoteFieldMusicPosition(given);
		}
		return 0;
	}

	static int GetPreviewNoteFieldMusicPosition(T* p, lua_State* L)
	{
		lua_pushnumber(L,
					   GAMESTATE->m_pPlayerState[PLAYER_1]
						 ->GetDisplayedPosition()
						 .m_fMusicSeconds);
		return 1;
	}

	static int PausePreviewNoteField(T* p, lua_State* L)
	{
		p->PausePreviewNoteFieldMusic();
		return 0;
	}
	static int IsPreviewNoteFieldPaused(T* p, lua_State* L)
	{
		lua_pushboolean(L, GAMESTATE->GetPaused());
		return 1;
	}
	static int dootforkfive(T* p, lua_State* L)
	{
		ActorFrame* king = Luna<ActorFrame>::check(L, 1);
		king->AddChild(p->m_pPreviewNoteField);
		COMMON_RETURN_SELF;
	}
	static int ChangeSteps(T* p, lua_State* L)
	{
		p->ChangeSteps(PLAYER_1, IArg(1));
		return 0;
	}
	static int OpenOptions(T* p, lua_State* L)
	{
		p->OpenOptions();
		return 0;
	}
	LunaScreenSelectMusic()
	{
		ADD_METHOD(OpenOptions);
		ADD_METHOD(GetGoToOptions);
		ADD_METHOD(GetMusicWheel);
		ADD_METHOD(OpenOptionsList);
		ADD_METHOD(CanOpenOptionsList);
		ADD_METHOD(SelectCurrent);
		ADD_METHOD(GetSelectionState);
		ADD_METHOD(StartPlaylistAsCourse);
		ADD_METHOD(PlayReplay);
		ADD_METHOD(ShowEvalScreenForScore);
		ADD_METHOD(CreatePreviewNoteField);
		ADD_METHOD(DeletePreviewNoteField);
		ADD_METHOD(GetPreviewNoteField);
		ADD_METHOD(SetPreviewNoteFieldMusicPosition);
		ADD_METHOD(GetPreviewNoteFieldMusicPosition);
		ADD_METHOD(PausePreviewNoteField);
		ADD_METHOD(IsPreviewNoteFieldPaused);
		ADD_METHOD(dootforkfive);
		ADD_METHOD(ChangeSteps);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenSelectMusic, ScreenWithMenuElements)
// lua end

/*
 * (c) 2001-2004 Chris Danford
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
