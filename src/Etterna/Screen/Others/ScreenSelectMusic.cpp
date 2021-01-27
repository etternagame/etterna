#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/CodeDetector.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Actor/Menus/MenuTimer.h"
#include "Etterna/Models/Misc/StageStats.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenSelectMusic.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/ImageCache.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Actor/Menus/OptionsList.h"
#include "RageUtil/Misc/RageInput.h"
#include "ScreenTextEntry.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Singletons/FilterManager.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Models/Misc/PlayerAI.h"
#include "Etterna/Models/Misc/PlayerOptions.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"

#include <algorithm>

static const char* SelectionStateNames[] = { "SelectingSong",
											 "SelectingSteps",
											 "Finalized" };
XToString(SelectionState);

#define SHOW_OPTIONS_MESSAGE_SECONDS                                           \
	THEME->GetMetricF(m_sName, "ShowOptionsMessageSeconds")

static const ThemeMetric<int> HARD_COMMENT_METER("ScreenSelectMusic",
												 "HardCommentMeter");

AutoScreenMessage(SM_AllowOptionsMenuRepeat);
AutoScreenMessage(SM_SongChanged);
AutoScreenMessage(SM_SortOrderChanging);
AutoScreenMessage(SM_SortOrderChanged);
AutoScreenMessage(SM_BackFromPlayerOptions);
AutoScreenMessage(SM_BackFromNamePlaylist);
AutoScreenMessage(SM_BackFromCalcTestStuff);

static bool g_bSampleMusicWaiting = false;
static bool delayedchartupdatewaiting = false;
static RageTimer g_StartedLoadingAt(RageZeroTimer);
static RageTimer g_ScreenStartedLoadingAt(RageZeroTimer);
RageTimer g_CanOpenOptionsList(RageZeroTimer);

static LocalizedString PERMANENTLY_DELETE("ScreenSelectMusic",
										  "PermanentlyDelete");
static LocalizedString NAME_PLAYLIST("ScreenSelectMusic", "NamePlaylist");
static LocalizedString ADDED_TO_PLAYLIST("ScreenSelectMusic",
										 "AddedToPlaylist");

REGISTER_SCREEN_CLASS(ScreenSelectMusic);

void
ScreenSelectMusic::Init()
{
	GAMESTATE->m_bPlayingMulti = false;
	g_ScreenStartedLoadingAt.Touch();
	if (PREFSMAN->m_sTestInitialScreen.Get() == m_sName) {
		GAMESTATE->SetCurrentStyle(
		  GAMEMAN->GameAndStringToStyle(GAMEMAN->GetDefaultGame(), "versus"),
		  PLAYER_INVALID);
		GAMESTATE->JoinPlayer(PLAYER_1);
		GAMESTATE->SetMasterPlayerNumber(PLAYER_1);
	}
	if (GamePreferences::m_AutoPlay == PC_REPLAY)
		GamePreferences::m_AutoPlay.Set(PC_HUMAN);
	if (GAMESTATE->m_pPlayerState->m_PlayerController == PC_REPLAY)
		GAMESTATE->m_pPlayerState->m_PlayerController = PC_HUMAN;

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
	m_bSelectIsDown = false; // used by UpdateSelectButton
	m_bAcceptSelectRelease = false;

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

	// build the playlist groups here, songmanager's init from disk can't
	// because profiles aren't loaded until after that's done -mina
	SONGMAN->MakeSongGroupsFromPlaylists();

	m_MusicWheel.SetName("MusicWheel");
	m_MusicWheel.Load(MUSIC_WHEEL_TYPE);
	LOAD_ALL_COMMANDS_AND_SET_XY(m_MusicWheel);
	this->AddChild(&m_MusicWheel);

	if (USE_OPTIONS_LIST) {
		m_OptionsList.SetName("OptionsList" + PlayerNumberToString(PLAYER_1));
		m_OptionsList.Load("OptionsList", PLAYER_1);
		m_OptionsList.SetDrawOrder(100);
		ActorUtil::LoadAllCommands(m_OptionsList, m_sName);
		this->AddChild(&m_OptionsList);
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

	// Replay Data Manager Reset.
	PlayerAI::ResetScoreData();

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
		Locator::getLogger()->trace("The Style has not been set.  A theme must set the Style "
				   "before loading ScreenSelectMusic.");
		// Instead of crashing, set the first compatible style.
		vector<StepsType> vst;
		GAMEMAN->GetStepsTypesForGame(GAMESTATE->m_pCurGame, vst);
		const auto* pStyle = GAMEMAN->GetFirstCompatibleStyle(
		  GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined(), vst[0]);
		if (pStyle == nullptr) {
			Locator::getLogger()->warn(ssprintf("No compatible styles for %s with %d player%s.",
							   GAMESTATE->m_pCurGame->m_szName,
							   GAMESTATE->GetNumSidesJoined(),
							   GAMESTATE->GetNumSidesJoined() == 1 ? "" : "s")
						.c_str());
			SCREENMAN->SetNewScreen("ScreenTitleMenu");
		}
		GAMESTATE->SetCurrentStyle(pStyle, PLAYER_INVALID);
	}

	OPTIONS_MENU_AVAILABLE.Load(m_sName, "OptionsMenuAvailable");
	PlayCommand("Mods");
	m_MusicWheel.BeginScreen();

	m_SelectionState = SelectionState_SelectingSong;
	m_bStepsChosen = false;
	m_bGoToOptions = false;
	m_bAllowOptionsMenu = m_bAllowOptionsMenuRepeat = false;
	m_iSelection = 0;

	if (USE_OPTIONS_LIST)
		m_OptionsList.Reset();

	AfterMusicChange();

	SOUND->PlayOnceFromAnnouncer("select music intro");

	if (GAMESTATE->IsPlaylistCourse()) {
		GAMESTATE->isplaylistcourse = false;
		SONGMAN->playlistcourse = "";
	}

	// Update the leaderboard for the file we may have just left
	// If it was empty, just let the players request it themselves (to prevent a
	// theme bug)
	if (GAMESTATE->m_pCurSteps != nullptr &&
		DLMAN->chartLeaderboards.count(GAMESTATE->m_pCurSteps->GetChartKey()) !=
		  0)
		DLMAN->RequestChartLeaderBoard(GAMESTATE->m_pCurSteps->GetChartKey());

	GAMESTATE->m_bRestartedGameplay = false;

	ScreenWithMenuElements::BeginScreen();
}

ScreenSelectMusic::~ScreenSelectMusic()
{
	if (PREFSMAN->m_verbose_log > 1)
		Locator::getLogger()->trace("ScreenSelectMusic::~ScreenSelectMusic()");
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
	if (delayedchartupdatewaiting) {
		if (g_ScreenStartedLoadingAt
			  .Ago() > // not sure if i need the "moving fast" check -mina
			SAMPLE_MUSIC_DELAY_INIT)
		{
			MESSAGEMAN->Broadcast("DelayedChartUpdate");
			delayedchartupdatewaiting = false;
		}
	}

	// Nothing else is going.  Start the music, if we haven't yet.
	if (g_bSampleMusicWaiting) {
		PlayCurrentSongSampleMusic(bForce);
	}
}

void
ScreenSelectMusic::PlayCurrentSongSampleMusic(bool bForcePlay, bool bForceAccurate, bool bExtended)
{
	if (g_bSampleMusicWaiting || bForcePlay) {
		if (g_ScreenStartedLoadingAt.Ago() < SAMPLE_MUSIC_DELAY_INIT)
			return;

		// Don't start the music sample when moving fast.
		if (g_StartedLoadingAt.Ago() < SAMPLE_MUSIC_DELAY && !bForcePlay)
			return;

		g_bSampleMusicWaiting = false;

		Song* pSong = GAMESTATE->m_pCurSong;
		// Lua is what usually calls this with force on
		// Since that bypasses a lot, update values if being forced.
		if (bForcePlay && pSong != nullptr) {
			m_sSampleMusicToPlay = pSong->GetPreviewMusicPath();
			if (!m_sSampleMusicToPlay.empty() &&
				ActorUtil::GetFileType(m_sSampleMusicToPlay) != FT_Sound) {
				LuaHelpers::ReportScriptErrorFmt(
				  "Music file %s for song is not a sound file, "
				  "ignoring.",
				  m_sSampleMusicToPlay.c_str());
				m_sSampleMusicToPlay = "";
			}
			m_pSampleMusicTimingData = &pSong->m_SongTiming;
			m_fSampleStartSeconds = pSong->GetPreviewStartSeconds();
			if (bExtended) {
				m_fSampleLengthSeconds =
				  pSong->GetLastSecond() - m_fSampleStartSeconds + 2.F;
				if (m_fSampleLengthSeconds < 3.F) {
					m_fSampleStartSeconds = 5.F;
					m_fSampleLengthSeconds = pSong->GetLastSecond() + 2.F;
				}
			}
			else
				m_fSampleLengthSeconds = pSong->m_fMusicSampleLengthSeconds;
		}

		GameSoundManager::PlayMusicParams PlayParams;
		PlayParams.sFile = HandleLuaMusicFile(m_sSampleMusicToPlay);
		PlayParams.pTiming = m_pSampleMusicTimingData;
		PlayParams.bForceLoop = SAMPLE_MUSIC_LOOPS || bExtended;
		PlayParams.fStartSecond = m_fSampleStartSeconds;
		PlayParams.fLengthSeconds = m_fSampleLengthSeconds;
		PlayParams.fFadeOutLengthSeconds = SAMPLE_MUSIC_FADE_OUT_SECONDS;
		PlayParams.bAlignBeat = ALIGN_MUSIC_BEATS;
		PlayParams.bApplyMusicRate = true;

		// We will leave this FALSE for standard sample music
		// Because accurate seeking is slow for MP3.
		// The way music playing works does not cause stutter, but
		// will cause inconsistent music playing experience and an overall
		// negative feel.
		// But if chart preview is active, it should probably be synced
		PlayParams.bAccurateSync = bForceAccurate;

		GameSoundManager::PlayMusicParams FallbackMusic;
		FallbackMusic.sFile = m_sLoopMusicPath;
		FallbackMusic.fFadeInLengthSeconds =
		  SAMPLE_MUSIC_FALLBACK_FADE_IN_SECONDS;
		FallbackMusic.bAlignBeat = ALIGN_MUSIC_BEATS;
		SOUND->PlayMusic(PlayParams);
		GAMESTATE->SetPaused(false);
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
	// reload songs
	SONGMAN->DifferentialReload();

	const auto selSong = GAMESTATE->m_pCurSong;
	const auto currentHoveredGroup = m_MusicWheel.GetCurrentGroup();

	// reset wheel
	m_MusicWheel.ReloadSongList(false, "");

	// place selection on the last song we were on
	// or fall back to section in case no song selected
	// (forces the section to open if so)
	if (selSong != nullptr)
		m_MusicWheel.SelectSongOrCourse();
	else {
		m_MusicWheel.SelectSection(currentHoveredGroup);
		m_MusicWheel.SetOpenSection(currentHoveredGroup);
	}
}

bool
ScreenSelectMusic::Input(const InputEventPlus& input)
{
	// HACK: This screen eats mouse inputs if we don't check for them first.
	auto mouse_evt = false;
	for (int i = MOUSE_LEFT; i <= MOUSE_WHEELDOWN; i++) {
		if (input.DeviceI ==
			DeviceInput(DEVICE_MOUSE, static_cast<DeviceButton>(i)))
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
		auto bHoldingCtrl =
		  INPUTFILTER->IsBeingPressed(
			DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL)) ||
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL));

		auto holding_shift =
		  INPUTFILTER->IsBeingPressed(
			DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)) ||
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT));

		auto c = INPUTMAN->DeviceInputToChar(input.DeviceI, false);
		MakeUpper(&c, 1);

		// Reload currently selected song
		if (holding_shift && bHoldingCtrl && c == 'R' &&
			m_MusicWheel.IsSettled() && input.type == IET_FIRST_PRESS) {
			auto* to_reload = m_MusicWheel.GetSelectedSong();
			if (to_reload != nullptr) {
				auto stepses = to_reload->GetAllSteps();
				vector<string> oldChartkeys;
				for (auto* steps : stepses)
					oldChartkeys.emplace_back(steps->GetChartKey());

				to_reload->ReloadFromSongDir();
				SONGMAN->ReconcileChartKeysForReloadedSong(to_reload,
														   oldChartkeys);

				AfterMusicChange();
				return true;
			}
		} else if (holding_shift && bHoldingCtrl && c == 'P' &&
				   m_MusicWheel.IsSettled() && input.type == IET_FIRST_PRESS) {
			SONGMAN->ForceReloadSongGroup(
			  GetMusicWheel()->GetSelectedSection());
			AfterMusicChange();
			SCREENMAN->SystemMessage("Current pack reloaded");
			return true;
		} else if (bHoldingCtrl && c == 'F' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS) {
			// Favorite the currently selected song. -Not Kyz
			auto* fav_me_biatch = m_MusicWheel.GetSelectedSong();
			if (fav_me_biatch != nullptr) {
				auto* pProfile = PROFILEMAN->GetProfile(PLAYER_1);

				if (!fav_me_biatch->IsFavorited()) {
					fav_me_biatch->SetFavorited(true);
					pProfile->AddToFavorites(
					  GAMESTATE->m_pCurSteps->GetChartKey());
					DLMAN->AddFavorite(GAMESTATE->m_pCurSteps->GetChartKey());

					// now update favorites playlist
					// we have to do this here or it won't work for ??? reasons
					pProfile->allplaylists.erase("Favorites");
					SONGMAN->MakePlaylistFromFavorites(
					  pProfile->FavoritedCharts, pProfile->allplaylists);
				} else {
					fav_me_biatch->SetFavorited(false);
					pProfile->RemoveFromFavorites(
					  GAMESTATE->m_pCurSteps->GetChartKey());
					DLMAN->RemoveFavorite(
					  GAMESTATE->m_pCurSteps->GetChartKey());

					// we have to do this here or it won't work for ??? reasons
					pProfile->allplaylists.erase("Favorites");
					SONGMAN->MakePlaylistFromFavorites(
					  pProfile->FavoritedCharts, pProfile->allplaylists);
				}
				DLMAN->RefreshFavourites();
				MESSAGEMAN->Broadcast("FavoritesUpdated");

				// update favorites playlist _display_
				MESSAGEMAN->Broadcast("DisplayAll");

				m_MusicWheel.ChangeMusic(0);
				return true;
			}
		} else if (bHoldingCtrl && c == 'M' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS) {
			// PermaMirror the currently selected song. -Not Kyz
			auto* alwaysmirrorsmh = m_MusicWheel.GetSelectedSong();
			if (alwaysmirrorsmh != nullptr) {
				auto* pProfile = PROFILEMAN->GetProfile(PLAYER_1);

				if (!alwaysmirrorsmh->IsPermaMirror()) {
					alwaysmirrorsmh->SetPermaMirror(true);
					pProfile->AddToPermaMirror(
					  GAMESTATE->m_pCurSteps->GetChartKey());
				} else {
					alwaysmirrorsmh->SetPermaMirror(false);
					pProfile->RemoveFromPermaMirror(
					  GAMESTATE->m_pCurSteps->GetChartKey());
				}
				MESSAGEMAN->Broadcast("FavoritesUpdated");
				m_MusicWheel.ChangeMusic(0);
				return true;
			}
		} else if (bHoldingCtrl && c == 'G' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS &&
				   GAMESTATE->m_pCurSteps != nullptr) {
			auto* pProfile = PROFILEMAN->GetProfile(PLAYER_1);
			pProfile->AddGoal(GAMESTATE->m_pCurSteps->GetChartKey());
			auto* asonglol = m_MusicWheel.GetSelectedSong();
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
		} else if (bHoldingCtrl && c == 'O' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS) {
			auto opposite = !GAMESTATE->IsPracticeMode();
			// don't allow changing practice mode if online
			auto online =
			  NSMAN->isSMOnline && NSMAN->loggedIn && NSMAN->IsETTP();
			opposite = opposite && !online;
			// this function handles the same above logic for online toggling
			GAMESTATE->TogglePracticeMode(opposite);

			if (opposite)
				SCREENMAN->SystemMessage("Practice Mode On");
			else
				SCREENMAN->SystemMessage("Practice Mode Off");
			return true;
		} else if (bHoldingCtrl && c == 'S' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS) {

			auto saved = PROFILEMAN->SaveProfile(PLAYER_1);

			if (!saved) {
				SCREENMAN->SystemMessage("Error Saving Profile");
			} else {
				SCREENMAN->SystemMessage("Profile Saved");
			}
			return true;
		} else if (bHoldingCtrl && c == 'P' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS) {
			ScreenTextEntry::TextEntry(
			  SM_BackFromNamePlaylist, NAME_PLAYLIST, "", 128);
			MESSAGEMAN->Broadcast("DisplayAll");
			return true;
		} else if (bHoldingCtrl && c == 'A' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS &&
				   GAMESTATE->m_pCurSteps != nullptr) {
			if (SONGMAN->GetPlaylists().empty())
				return true;

			SONGMAN->GetPlaylists()[SONGMAN->activeplaylist].AddChart(
			  GAMESTATE->m_pCurSteps->GetChartKey());
			MESSAGEMAN->Broadcast("DisplaySinglePlaylist");
			SCREENMAN->SystemMessage(
			  ssprintf(ADDED_TO_PLAYLIST.GetValue(),
					   GAMESTATE->m_pCurSong->GetDisplayMainTitle().c_str(),
					   SONGMAN->activeplaylist.c_str()));
			return true;
		} else if (bHoldingCtrl && c == 'T' && m_MusicWheel.IsSettled() &&
				   input.type == IET_FIRST_PRESS &&
				   GAMESTATE->m_pCurSteps != nullptr) {

			auto ck = GAMESTATE->m_pCurSteps->GetChartKey();
			auto foundSS = Skillset_Invalid;
			for (const auto& ss : SONGMAN->testChartList) {
				if (ss.second.filemapping.count(ck)) {
					foundSS = ss.first;
					break;
				}
			}
			if (foundSS == Skillset_Invalid)
				ScreenTextEntry::TextEntry(
				  SM_BackFromCalcTestStuff,
				  "FORMAT: MSD RATE SKILLSET  or   MSD SKILLSET\nOnly 1 rate "
				  "per chart",
				  "",
				  128);
			else {
				// SONGMAN->testChartList[foundSS].filemapping.erase(ck);
				SCREENMAN->SystemMessage(ssprintf(
				  "Removed this chart from the test list (skillset %d)",
				  foundSS));
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
	if (m_SelectionState == SelectionState_SelectingSteps && m_bStepsChosen &&
		input.MenuI == GAME_BUTTON_SELECT && input.type == IET_FIRST_PRESS) {
		Message msg("StepsUnchosen");
		msg.SetParam("Player", input.pn);
		MESSAGEMAN->Broadcast(msg);
		m_bStepsChosen = false;
		return true;
	}

	if (m_SelectionState == SelectionState_Finalized || m_bStepsChosen)
		return false; // ignore

	if (USE_PLAYER_SELECT_MENU) {
		if (input.type == IET_RELEASE && input.MenuI == GAME_BUTTON_SELECT) {
			SCREENMAN->AddNewScreenToTop(SELECT_MENU_NAME,
										 SM_BackFromPlayerOptions);
		}
	}

	// handle OptionsList input
	if (USE_OPTIONS_LIST) {
		auto pn = input.pn;
		if (pn != PLAYER_INVALID) {
			if (m_OptionsList.IsOpened()) {
				return m_OptionsList.Input(input);
			} else {
				if (input.type == IET_RELEASE &&
					input.MenuI == GAME_BUTTON_SELECT && m_bAcceptSelectRelease)
					m_OptionsList.Open();
			}
		}
	}

	if (input.MenuI == GAME_BUTTON_SELECT && input.type != IET_REPEAT)
		m_bAcceptSelectRelease = (input.type == IET_FIRST_PRESS);

	if (SELECT_MENU_AVAILABLE && input.MenuI == GAME_BUTTON_SELECT &&
		input.type != IET_REPEAT)
		UpdateSelectButton(input.pn, input.type == IET_FIRST_PRESS);

	if (SELECT_MENU_AVAILABLE && m_bSelectIsDown) {
		if (input.type == IET_FIRST_PRESS && SELECT_MENU_CHANGES_DIFFICULTY) {
			switch (input.MenuI) {
				case GAME_BUTTON_LEFT:
					ChangeSteps(input.pn, -1);
					m_bAcceptSelectRelease = false;
					break;
				case GAME_BUTTON_RIGHT:
					ChangeSteps(input.pn, +1);
					m_bAcceptSelectRelease = false;
					break;
				case GAME_BUTTON_START:
					m_bAcceptSelectRelease = false;
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
			m_bAcceptSelectRelease = false;
		}
		if (input.type == IET_FIRST_PRESS)
			g_CanOpenOptionsList.Touch();
		if (g_CanOpenOptionsList.Ago() > OPTIONS_LIST_TIMEOUT)
			m_bAcceptSelectRelease = false;
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

			auto bLeftIsDown = false;
			auto bRightIsDown = false;

			if (m_OptionsList.IsOpened())
				return false;
			if (SELECT_MENU_AVAILABLE &&
				INPUTMAPPER->IsBeingPressed(GAME_BUTTON_SELECT, PLAYER_1))
				return false;

			bLeftIsDown |= static_cast<int>(
			  INPUTMAPPER->IsBeingPressed(m_GameButtonPreviousSong, PLAYER_1));
			bRightIsDown |= static_cast<int>(
			  INPUTMAPPER->IsBeingPressed(m_GameButtonNextSong, PLAYER_1));

			auto bBothDown = bLeftIsDown && bRightIsDown;
			auto bNeitherDown = !bLeftIsDown && !bRightIsDown;

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
				auto sNewGroup = m_MusicWheel.JumpToPrevGroup();
				m_MusicWheel.SelectSection(sNewGroup);
				m_MusicWheel.SetOpenSection(sNewGroup);
				MESSAGEMAN->Broadcast("PreviousGroup");
				AfterMusicChange();
			} else if (input.MenuI == m_GameButtonNextGroup) {
				auto sNewGroup = m_MusicWheel.JumpToNextGroup();
				m_MusicWheel.SelectSection(sNewGroup);
				m_MusicWheel.SetOpenSection(sNewGroup);
				MESSAGEMAN->Broadcast("NextGroup");
				AfterMusicChange();
			}
		}
	}

	if (m_SelectionState == SelectionState_SelectingSteps &&
		input.type == IET_FIRST_PRESS && !m_bStepsChosen) {
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
			auto fSeconds = m_MenuTimer->GetSeconds();
			if (fSeconds > 10) {
				Message msg("SongUnchosen");
				msg.SetParam("Player", input.pn);
				MESSAGEMAN->Broadcast(msg);
				// unset all steps
				m_bStepsChosen = false;
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
		const auto sNewGroup = m_MusicWheel.JumpToNextGroup();
		m_MusicWheel.SelectSection(sNewGroup);
		m_MusicWheel.SetOpenSection(sNewGroup);
		MESSAGEMAN->Broadcast("NextGroup");
		AfterMusicChange();
	} else if (CodeDetector::EnteredPrevGroup(input.GameI.controller) &&
			   !CHANGE_GROUPS_WITH_GAME_BUTTONS) {
		const auto sNewGroup = m_MusicWheel.JumpToPrevGroup();
		m_MusicWheel.SelectSection(sNewGroup);
		m_MusicWheel.SetOpenSection(sNewGroup);
		MESSAGEMAN->Broadcast("PreviousGroup");
		AfterMusicChange();
	} else if (CodeDetector::EnteredCloseFolder(input.GameI.controller)) {
		const auto sCurSection = m_MusicWheel.GetSelectedSection();
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

	if (m_bSelectIsDown != bSelectIsDown) {
		m_bSelectIsDown = bSelectIsDown;
		Message msg(bSelectIsDown ? "SelectMenuOpened" : "SelectMenuClosed");
		msg.SetParam("Player", pn);
		MESSAGEMAN->Broadcast(msg);
	}
}

void
ScreenSelectMusic::ChangeSteps(PlayerNumber pn, int dir)
{
	Locator::getLogger()->trace("ScreenSelectMusic::ChangeSteps( {}, {} )", pn, dir);

	ASSERT(GAMESTATE->IsHumanPlayer(pn));

	if (GAMESTATE->m_pCurSong) {
		m_iSelection += dir;
		if (WRAP_CHANGE_STEPS) {
			wrap(m_iSelection, m_vpSteps.size());
		} else {
			if (CLAMP(m_iSelection, 0, m_vpSteps.size() - 1))
				return;
		}

		// the user explicity switched difficulties. Update the preferred
		// Difficulty and StepsType
		auto* pSteps = m_vpSteps[m_iSelection];
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
	if (pn == PLAYER_1 || GAMESTATE->DifficultiesLocked()) {
		vpns.push_back(PLAYER_1);
	}
	AfterStepsOrTrailChange(vpns);

	const auto fBalance = GameSoundManager::GetPlayerBalance(pn);
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

	ScreenWithMenuElements::HandleMessage(msg);
}

void
ScreenSelectMusic::HandleScreenMessage(const ScreenMessage& SM)
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
		DLMAN->UpdateDLSpeed(false);
		CodeDetector::RefreshCacheItems(CODES);
	} else if (SM == SM_LoseFocus) {
		CodeDetector::RefreshCacheItems(); // reset for other screens
	} else if (SM == SM_BackFromCalcTestStuff) {
		auto ans = ScreenTextEntry::s_sLastAnswer;
		std::vector<std::string> words;
		std::istringstream iss(ans);

		for (std::string s; iss >> s;) {
			words.push_back(s);
		}

		// OOPS I COPY PASTED THE SAME CODE TWICE OH NO ITS TOO LATE I ALREADY
		// FINISHED WRITING EVERYTHING AAAAAHHHH
		if (words.size() == 2) {
			try {
				auto target = stof(words[0]);
				auto ss = static_cast<Skillset>(stoi(words[1]));
				if (ss < 0 || ss >= NUM_Skillset)
					SCREENMAN->SystemMessage("invalid skillset number");
				else if (GAMESTATE->m_pCurSteps != nullptr) {
					CalcTest thetest;
					auto ck = GAMESTATE->m_pCurSteps->GetChartKey();
					if (SONGMAN->testChartList.count(ss)) {
						thetest.ck = ck;
						thetest.ev = target;
						thetest.rate = 1.f;
						SONGMAN->testChartList[ss].filemapping[ck] = thetest;
					} else {
						CalcTestList tl;
						tl.skillset = ss;
						thetest.ck = ck;
						thetest.ev = target;
						thetest.rate = 1.f;
						tl.filemapping[ck] = thetest;
						SONGMAN->testChartList[ss] = tl;
					}
					SCREENMAN->SystemMessage(
					  ssprintf("added %s to %s at rate 1.0",
							   ck.c_str(),
							   SkillsetToString(ss).c_str()));
					SONGMAN->SaveCalcTestXmlToDir();
					GAMESTATE->m_pCurSteps->DoATestThing(
					  target, ss, 1.f, SONGMAN->calc.get());
				}
			} catch (...) {
				SCREENMAN->SystemMessage("you messed up (input exception)");
			}
		} else if (words.size() == 3) {
			try {
				auto target = stof(words[0]);
				auto rate = stof(words[1]);
				auto ss = static_cast<Skillset>(stoi(words[2]));
				if (ss < 0 || ss >= NUM_Skillset)
					SCREENMAN->SystemMessage("invalid skillset number");
				else if (GAMESTATE->m_pCurSteps != nullptr) {
					CalcTest thetest;
					auto ck = GAMESTATE->m_pCurSteps->GetChartKey();
					if (SONGMAN->testChartList.count(ss)) {
						thetest.ck = ck;
						thetest.ev = target;
						thetest.rate = rate;
						SONGMAN->testChartList[ss].filemapping[ck] = thetest;
					} else {
						CalcTestList tl;
						tl.skillset = ss;
						thetest.ck = ck;
						thetest.ev = target;
						thetest.rate = rate;
						tl.filemapping[ck] = thetest;
						SONGMAN->testChartList[ss] = tl;
					}
					SCREENMAN->SystemMessage(
					  ssprintf("added %s to %s at rate %f",
							   ck.c_str(),
							   SkillsetToString(ss).c_str(),
							   rate));
					SONGMAN->SaveCalcTestXmlToDir();
					GAMESTATE->m_pCurSteps->DoATestThing(
					  target, ss, rate, SONGMAN->calc.get());
				}
			} catch (...) {
				SCREENMAN->SystemMessage("you messed up (input exception)");
			}
		} else {
			SCREENMAN->SystemMessage("you messed up (wrong input format)");
		}
	}

	if (SM == SM_BackFromNamePlaylist) {
		Playlist pl;
		pl.name = ScreenTextEntry::s_sLastAnswer;
		auto& pls = SONGMAN->GetPlaylists();
		// require name not empty and name not a duplicate
		if (pl.name != "" && pls.count(pl.name) == 0) {
			SONGMAN->GetPlaylists().emplace(pl.name, pl);
			SONGMAN->activeplaylist = pl.name;
			MESSAGEMAN->Broadcast("DisplayAll");
		}

		// restart preview music after finishing or cancelling playlist creation
		// this is just copypasta'd and should be made a function? or we have
		// something better? idk
		GameSoundManager::PlayMusicParams PlayParams;
		PlayParams.sFile = HandleLuaMusicFile(m_sSampleMusicToPlay);
		PlayParams.pTiming = m_pSampleMusicTimingData;
		PlayParams.bForceLoop = SAMPLE_MUSIC_LOOPS;
		PlayParams.fStartSecond = m_fSampleStartSeconds;
		PlayParams.fLengthSeconds = m_fSampleLengthSeconds;
		PlayParams.fFadeOutLengthSeconds = SAMPLE_MUSIC_FADE_OUT_SECONDS;
		PlayParams.bAlignBeat = ALIGN_MUSIC_BEATS;
		PlayParams.bApplyMusicRate = true;
		PlayParams.bAccurateSync = false;
		GameSoundManager::PlayMusicParams FallbackMusic;
		FallbackMusic.sFile = m_sLoopMusicPath;
		FallbackMusic.fFadeInLengthSeconds =
		  SAMPLE_MUSIC_FALLBACK_FADE_IN_SECONDS;
		FallbackMusic.bAlignBeat = ALIGN_MUSIC_BEATS;
		SOUND->PlayMusic(PlayParams);
		GAMESTATE->SetPaused(false);
		MESSAGEMAN->Broadcast("PlayingSampleMusic");
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
ScreenSelectMusic::SelectCurrent(PlayerNumber pn, GameplayMode mode)
{

	switch (m_SelectionState) {
		case SelectionState_Finalized: {
			Locator::getLogger()->warn("song selection made while selectionstate_finalized");
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
			} else {
				// We haven't made a selection yet.
				return false;
			}
			// I believe this is for those who like pump pro. -aj
			MESSAGEMAN->Broadcast("SongChosen");
			break;
		case SelectionState_SelectingSteps: {
		} break;
	}
	if (m_SelectionState == SelectionState_SelectingSteps) {
		if (m_OptionsList.IsOpened())
			m_OptionsList.Close();
	}
	UpdateSelectButton(PLAYER_1, false);

	m_SelectionState = GetNextSelectionState();
	Message msg("Start" + SelectionStateToString(m_SelectionState));
	MESSAGEMAN->Broadcast(msg);

	m_soundStart.Play(true);

	if (m_SelectionState == SelectionState_Finalized) {
		DLMAN->UpdateDLSpeed(true);

		if (!m_bStepsChosen) {
			m_bStepsChosen = true;
			// Don't play start sound. We play it again below on finalized
			// m_soundStart.Play(true);

			Message lMsg("StepsChosen");
			lMsg.SetParam("Player", PLAYER_1);
			MESSAGEMAN->Broadcast(lMsg);
		}

		// Now that Steps have been chosen, set a Style that can play them.
		GAMESTATE->SetCompatibleStylesForPlayers();

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
			const auto fTime =
			  std::max(SHOW_OPTIONS_MESSAGE_SECONDS, this->GetTweenTimeLeft());
			this->PostScreenMessage(SM_BeginFadingOut, fTime);
		} else {
			StartTransitioningScreen(SM_BeginFadingOut);
		}
		// mild hack:
		/* a true return value (for all cases where the return value of this
		 * function is handled) normally means the "input was handled" and
		 * doesn't get pushed to more screens here, we call this function with a
		 * non default mode value and that means that we aren't going to care
		 * about handling input because we already handled input ok now that i
		 * think about it this really doesn't matter but i still want to leave
		 * this comment here to explain my logic for the odd return value (and
		 * we want to know if the function call returned early to prevent
		 * loading replay/practice stuff at the wrong time)
		 */
		return mode == GameplayMode_Practice || mode == GameplayMode_Replay;
	}
	return false;
}

bool
ScreenSelectMusic::MenuBack(const InputEventPlus& /* input */)
{
	Cancel(SM_GoToPrevScreen);
	return true;
}

void
ScreenSelectMusic::AfterStepsOrTrailChange(const vector<PlayerNumber>& vpns)
{
	const auto pn = PLAYER_1;
	ASSERT(GAMESTATE->IsHumanPlayer(pn));

	if (GAMESTATE->m_pCurSong) {
		CLAMP(m_iSelection, 0, m_vpSteps.size() - 1);

		Song* pSong = GAMESTATE->m_pCurSong;
		auto* pSteps = m_vpSteps.empty() ? nullptr : m_vpSteps[m_iSelection];

		GAMESTATE->m_pCurSteps.Set(pSteps);
		if (pSteps != nullptr)
			GAMESTATE->SetCompatibleStyle(pSteps->m_StepsType, pn);

		if (pSteps) {
			GAMESTATE->UpdateSongPosition(pSong->m_fMusicSampleStartSeconds,
										  *pSteps->GetTimingData());
			delayedchartupdatewaiting = true;
		}
	}
}

void
ScreenSelectMusic::SwitchToPreferredDifficulty()
{

	// Find the closest match to the user's preferred difficulty and
	// StepsType.
	auto iCurDifference = -1;
	auto& iSelection = m_iSelection;
	FOREACH_CONST(Steps*, m_vpSteps, s)
	{
		const int i = s - m_vpSteps.begin();

		// If the current steps are listed, use them.
		if (GAMESTATE->m_pCurSteps == *s) {
			iSelection = i;
			break;
		}

		if (GAMESTATE->m_PreferredDifficulty != Difficulty_Invalid) {
			const auto iDifficultyDifference =
			  abs((*s)->GetDifficulty() - GAMESTATE->m_PreferredDifficulty);
			auto iStepsTypeDifference = 0;
			if (GAMESTATE->m_PreferredStepsType != StepsType_Invalid)
				iStepsTypeDifference =
				  abs((*s)->m_StepsType - GAMESTATE->m_PreferredStepsType);
			const auto iTotalDifference =
			  iStepsTypeDifference * NUM_Difficulty + iDifficultyDifference;

			if (iCurDifference == -1 || iTotalDifference < iCurDifference) {
				iSelection = i;
				iCurDifference = iTotalDifference;
			}
		}
	}

	CLAMP(iSelection, 0, m_vpSteps.size() - 1);
}

void
ScreenSelectMusic::AfterMusicChange()
{
	auto* pSong = m_MusicWheel.GetSelectedSong();
	GAMESTATE->m_pCurSong.Set(pSong);
	if (pSong == nullptr) {
		GAMESTATE->m_pCurSteps.Set(nullptr);
		if (b_PreviewNoteFieldIsActive)
		// if previewnotefield we are moving out of a pack
		// into the pack list (that's what this block of code is for
		// handling) manually call songmans cleanup function (compresses all
		// steps); we could optimize by only compressing the pack but this
		// is pretty fast anyway -mina
			SONGMAN->Cleanup();
	} else {
		GAMESTATE->m_pPreferredSong = pSong;
	}

	GAMESTATE->SetPaused(false); // hacky can see this being problematic
								 // if we forget about it -mina

	m_vpSteps.clear();
	vector<std::string> m_Artists, m_AltArtists;

	if (SAMPLE_MUSIC_PREVIEW_MODE != SampleMusicPreviewMode_LastSong) {
		m_sSampleMusicToPlay = "";
	}
	m_pSampleMusicTimingData = nullptr;

	static auto s_lastSortOrder = SortOrder_Invalid;
	if (GAMESTATE->m_SortOrder != s_lastSortOrder) {
		// Reload to let Lua metrics have a chance to change the help text.
		s_lastSortOrder = GAMESTATE->m_SortOrder;
	}

	const auto wtype = m_MusicWheel.GetSelectedType();
	SampleMusicPreviewMode pmode;
	switch (wtype) {
		case WheelItemDataType_Section:
		case WheelItemDataType_Sort:
		case WheelItemDataType_Roulette:
		case WheelItemDataType_Random:
		case WheelItemDataType_Custom:
			m_iSelection = -1;
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
					if (pSong != nullptr) {
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
						m_fSampleLengthSeconds =
						  pSong->m_fMusicSampleLengthSeconds;
					}
					break;
				default:
					FAIL_M(ssprintf("Invalid preview mode: %i", pmode));
			}

			if (pSong != nullptr)
				SongUtil::GetPlayableSteps(
				  pSong, m_vpSteps, FILTERMAN->AnyActiveFilter());
			if (m_vpSteps.empty()) {
				// LuaHelpers::ReportScriptError("GetPlayableSteps returned
				// nothing.");
			}

			SwitchToPreferredDifficulty();
			break;
		default:
			FAIL_M(ssprintf("Invalid WheelItemDataType: %i", wtype));
	}

	// Don't stop music if it's already playing the right file.
	g_bSampleMusicWaiting = false;
	if (!m_MusicWheel.IsRouletting() &&
		SOUND->GetMusicPath() != m_sSampleMusicToPlay) {
		SOUND->StopMusic();
		// some SampleMusicPreviewModes don't want the sample music immediately.
		if (SAMPLE_MUSIC_PREVIEW_MODE !=
			SampleMusicPreviewMode_StartToPreview) {
			if (!m_sSampleMusicToPlay.empty())
				// dont run basic preview if chart preview is running
				// lua handles that stuff (we need to change that)
				g_bSampleMusicWaiting = true;
		}
	}

	g_StartedLoadingAt.Touch();

	vector<PlayerNumber> vpns;
	vpns.push_back(PLAYER_1);

	AfterStepsOrTrailChange(vpns);
}

void
ScreenSelectMusic::OpenOptionsList(PlayerNumber pn)
{
	if (pn != PLAYER_INVALID) {
		m_MusicWheel.Move(0);
		m_OptionsList.Open();
	}
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
	if (m_OptionsList.IsOpened()) {
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
ScreenSelectMusic::SetSampleMusicPosition(float given)
{
	SOUND->WithRageSoundPlaying([given](RageSound* pMusic) {
		SOUND->SetSoundPosition(pMusic, given);
		if (GAMESTATE->GetPaused())
			pMusic->Pause(true);
	});
}

void
ScreenSelectMusic::PauseSampleMusic()
{
	auto paused = GAMESTATE->GetPaused();
	SOUND->WithRageSoundPlaying([paused](RageSound* pMusic) {
		const auto success = pMusic->Pause(!paused);
		// sometimes we might attempt to pause a sound before it starts and that
		// fails, but returns a false state on failure which is good for telling
		// us we didnt really pause anything (wow who would have thought)
		GAMESTATE->SetPaused(success && pMusic->m_bPaused);
	});
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

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
		const auto pn = PLAYER_1;
		if (p->can_open_options_list(pn)) {
			p->OpenOptionsList(pn);
		}
		COMMON_RETURN_SELF;
	}
	static int CanOpenOptionsList(T* p, lua_State* L)
	{
		const auto pn = PLAYER_1;
		lua_pushboolean(L, p->can_open_options_list(pn));
		return 1;
	}
	static int SelectCurrent(T* p, lua_State* L)
	{
		p->SelectCurrent(PLAYER_1);
		return 0;
	}

	static int GetSelectionState(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetSelectionState());
		return 1;
	}

	static int StartPlaylistAsCourse(T* p, lua_State* L)
	{
		const string name = SArg(1);
		auto& pl = SONGMAN->GetPlaylists()[name];

		// don't allow empty playlists to be started as a course
		if (pl.chartlist.empty()) {
			lua_pushboolean(L, false);
			return 1;
		}

		// dont allow playlists with an unloaded chart to be played as a course
		for (auto ch : pl.chartlist) {
			if (!ch.loaded) {
				lua_pushboolean(L, false);
				return 1;
			}
		}

		// dont start a playlist in practice or replay
		if (GAMESTATE->GetGameplayMode() != GameplayMode_Normal) {
			lua_pushboolean(L, false);
			return 1;
		}

		SONGMAN->playlistcourse = name;
		GAMESTATE->isplaylistcourse = true;
		p->GetMusicWheel()->SelectSong(pl.chartlist[0].songptr);
		GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate =
		  pl.chartlist[0].rate;
		MESSAGEMAN->Broadcast("RateChanged");
		p->SelectCurrent(PLAYER_1);

		// success
		lua_pushboolean(L, true);
		return 1;
	}

	static int PlayReplay(T* p, lua_State* L)
	{
		// get the highscore from lua and make the AI load it
		auto* hs = Luna<HighScore>::check(L, 1);

		// Sometimes the site doesn't send a replay when we ask for one.
		// This is not our fault.
		// All scores should have keys.
		if (hs->GetScoreKey().empty()) {
			SCREENMAN->SystemMessage(
			  "Replay appears to be empty. Report this score to developers.");
			lua_pushboolean(L, false);
			return 1;
		}

		// Warn but do not disallow Replays that use playback-breaking mods.
		PlayerOptions potmp;
		potmp.FromString(hs->GetModifiers());
		if (potmp.ContainsTransformOrTurn()) {
			SCREENMAN->SystemMessage("Warning: This Replay uses modifiers "
									 "which may break the Replay playback.");
		}

		auto likely_entering_gameplay =
		  p->SelectCurrent(PLAYER_1, GameplayMode_Replay);

		// just in case
		if (!likely_entering_gameplay) {
			lua_pushboolean(L, false);
			return 1;
		}

		GAMESTATE->m_gameplayMode.Set(GameplayMode_Replay);
		auto nd = GAMESTATE->m_pCurSteps->GetNoteData();

		// we get timestamps not noterows when getting online replays from the
		// site, since order is deterministic we'll just auto set the noterows
		// from the existing, if the score was cc off then we need to fill in
		// extra rows for each tap in the chord -mina
		auto timestamps = hs->GetCopyOfSetOnlineReplayTimestampVector();
		auto noterows = hs->GetNoteRowVector();

		// Construct noterows from given timestamps if timestamps are given
		// alone
		if (!timestamps.empty() && noterows.empty()) {
			GAMESTATE->SetProcessedTimingData(
			  GAMESTATE->m_pCurSteps->GetTimingData());
			auto* td = GAMESTATE->m_pCurSteps->GetTimingData();
			auto nerv = nd.BuildAndGetNerv(td);
			auto sdifs = td->BuildAndGetEtaner(nerv);
			std::vector<int> noterows;
			for (auto t : timestamps) {
				auto timestamptobeat =
				  td->GetBeatFromElapsedTime(t * hs->GetMusicRate());
				auto somenumberscaledbyoffsets =
				  sdifs[0] - (timestamps[0] * hs->GetMusicRate());
				timestamptobeat += somenumberscaledbyoffsets;
				auto noterowfrombeat = BeatToNoteRow(timestamptobeat);
				noterows.emplace_back(noterowfrombeat);
			}
			auto noterowoffsetter = nerv[0] - noterows[0];
			for (auto& noterowwithoffset : noterows)
				noterowwithoffset += noterowoffsetter;
			GAMESTATE->SetProcessedTimingData(nullptr);
			hs->SetNoteRowVector(noterows);
		}

		// Since we keep misses on EO as 180ms, need to convert them back.
		if (!timestamps.empty()) {
			auto offsets = hs->GetCopyOfOffsetVector();
			for (auto& offset : offsets) {
				if (fabs(offset) >= .18f)
					offset = -1.1f; // This is a miss to the replay reader.
			}
			hs->SetOffsetVector(offsets);
		}

		// Player AI Setup.
		PlayerAI::ResetScoreData();
		PlayerAI::SetScoreData(hs, 0, &nd);

		// prepare old mods to return to
		const auto oldMods =
		  GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred().GetString(
			true);

		// Set Replay mods and rate to let it handle stuff
		auto scoreRate = hs->GetMusicRate();
		auto oldRate = GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate;
		PlayerAI::replayRate = scoreRate;
		PlayerAI::oldModifiers = oldMods;
		PlayerAI::oldRate = oldRate;
		auto ns =
		  GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred().m_sNoteSkin;
		auto ft =
		  GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred().m_FailType;
		if (ns.empty())
			ns = CommonMetrics::DEFAULT_NOTESKIN_NAME;
		PlayerAI::oldNoteskin = ns;
		auto usesMirror = potmp.m_bTurns[PlayerOptions::TURN_MIRROR];
		auto hsMods = hs->GetModifiers();
		PlayerAI::replayModifiers = hsMods;
		PlayerAI::replayUsedMirror = usesMirror;
		PlayerAI::oldFailType = ft;

		// lock the game into replay mode and GO
		Locator::getLogger()->trace("Viewing replay for score key {}",
				   hs->GetScoreKey().c_str());
		GamePreferences::m_AutoPlay.Set(PC_REPLAY);
		GAMESTATE->m_pPlayerState->m_PlayerController = PC_REPLAY;

		// success
		lua_pushboolean(L, true);
		return 1;
	}

	static int ShowEvalScreenForScore(T* p, lua_State* L)
	{
		// get the highscore from lua and fake it to the most recent score
		auto* hs = Luna<HighScore>::check(L, 1);
		SCOREMAN->PutScoreAtTheTop(hs->GetScoreKey());

		// set to replay mode to disable score saving
		GamePreferences::m_AutoPlay.Set(PC_REPLAY);

		// construct the current stage stats and stuff to the best of our
		// ability
		StageStats ss;
		RadarValues rv;
		NoteData nd;
		Steps* steps = GAMESTATE->m_pCurSteps;
		steps->GetNoteData(nd);
		ss.Init();
		SCOREMAN->camefromreplay =
		  false; // disallow viewing online score eval screens -mina
		auto* score = SCOREMAN->GetMostRecentScore();
		if (score == nullptr || !score->LoadReplayData()) {
			SCREENMAN->SystemMessage(
			  "Failed to load Replay Data for some reason.");
			lua_pushboolean(L, false);
			return 1;
		}

		auto* td = steps->GetTimingData();
		PlayerAI::ResetScoreData();
		PlayerAI::SetScoreData(score, 0, &nd);
		PlayerAI::SetUpExactTapMap(td);

		auto& pss = ss.m_player;
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
		  GAMESTATE->m_pCurSteps->GetTimingData());
		NoteDataUtil::CalculateRadarValues(nd, rv);
		pss.m_radarPossible += rv;
		RadarValues realRV;
		PlayerAI::CalculateRadarValuesForReplay(realRV, rv);
		score->SetRadarValues(realRV);
		pss.m_radarActual += realRV;
		GAMESTATE->SetProcessedTimingData(nullptr);
		pss.everusedautoplay = true;
		for (int i = TNS_Miss; i < NUM_TapNoteScore; i++) {
			pss.m_iTapNoteScores[i] =
			  score->GetTapNoteScore(static_cast<TapNoteScore>(i));
		}
		for (auto i = 0; i < NUM_HoldNoteScore; i++) {
			pss.m_iHoldNoteScores[i] =
			  score->GetHoldNoteScore(static_cast<HoldNoteScore>(i));
		}
		PlayerOptions potmp;
		potmp.FromString(hs->GetModifiers());
		if (!hs->GetChordCohesion() && !potmp.ContainsTransformOrTurn()) {
			pss.m_fLifeRecord = PlayerAI::GenerateLifeRecordForReplay();
			pss.m_ComboList = PlayerAI::GenerateComboListForReplay();
		}
		ss.m_vpPlayedSongs.emplace_back(GAMESTATE->m_pCurSong);
		ss.m_vpPossibleSongs.emplace_back(GAMESTATE->m_pCurSong);
		ss.m_fMusicRate = score->GetMusicRate();
		STATSMAN->m_CurStageStats = ss;
		STATSMAN->m_vPlayedStageStats.emplace_back(ss);

		// set the rate so the MSD and rate display doesn't look weird
		auto scoreRate = hs->GetMusicRate();
		auto oldRate = GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate;
		GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = scoreRate;
		GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = scoreRate;
		GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = scoreRate;
		MESSAGEMAN->Broadcast("RateChanged");

		// go
		Locator::getLogger()->trace("Viewing evaluation screen for score key {}",
				   score->GetScoreKey().c_str());
		SCREENMAN->SetNewScreen("ScreenEvaluationNormal");

		// set rate back to what it was before
		GAMEMAN->m_bResetModifiers = true;
		GAMEMAN->m_fPreviousRate = oldRate;

		// success
		lua_pushboolean(L, true);
		return 1;
	}

	static int SetSampleMusicPosition(T* p, lua_State* L)
	{
		const auto given = FArg(1);
		p->SetSampleMusicPosition(given);
		return 0;
	}

	static int GetSampleMusicPosition(T* p, lua_State* L)
	{
		lua_pushnumber(L, GAMESTATE->m_Position.m_fMusicSeconds);
		return 1;
	}

	static int PauseSampleMusic(T* p, lua_State* L)
	{
		p->PauseSampleMusic();
		return 0;
	}
	static int IsSampleMusicPaused(T* p, lua_State* L)
	{
		lua_pushboolean(L, GAMESTATE->GetPaused());
		return 1;
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
	static int PlayCurrentSongSampleMusic(T* p, lua_State* L)
	{
		p->PlayCurrentSongSampleMusic(true, BArg(1), BArg(2));
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
		ADD_METHOD(SetSampleMusicPosition);
		ADD_METHOD(GetSampleMusicPosition);
		ADD_METHOD(PauseSampleMusic);
		ADD_METHOD(IsSampleMusicPaused);
		ADD_METHOD(ChangeSteps);
		ADD_METHOD(PlayCurrentSongSampleMusic);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenSelectMusic, ScreenWithMenuElements)
// lua end
