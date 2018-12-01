#include "global.h"

#if !defined(WITHOUT_NETWORKING)
#include "ActorUtil.h"
#include "AnnouncerManager.h"
#include "LocalizedString.h"
#include "CodeDetector.h"
#include "FilterManager.h"
#include "GameConstantsAndTypes.h"
#include "GameSoundManager.h"
#include "GameState.h"
#include "InputEventPlus.h"
#include "InputMapper.h"
#include "MenuTimer.h"
#include "MusicWheel.h"
#include "NetworkSyncManager.h"
#include "ProfileManager.h"
#include "RageInput.h"
#include "RageLog.h"
#include "Style.h"
#include "RageTimer.h"
#include "RageUtil.h"
#include "ScreenManager.h"
#include "ScreenNetSelectMusic.h"
#include "ScreenNetSelectBase.h"
#include "Song.h"
#include "SongManager.h"
#include "CodeDetector.h"
#include "ProfileManager.h"
#include "FilterManager.h"
#include "RageFileManager.h"
#include "ScreenPrompt.h"

AutoScreenMessage(SM_AddToChat);
AutoScreenMessage(SM_FriendsUpdate);
AutoScreenMessage(SM_NoSongs);
AutoScreenMessage(SM_SetWheelSong);
AutoScreenMessage(SM_RefreshWheelLocation);
AutoScreenMessage(SM_SongChanged);
AutoScreenMessage(SM_UsersUpdate);
AutoScreenMessage(SM_BackFromPlayerOptions);
AutoScreenMessage(SM_ConfirmDeleteSong);
AutoScreenMessage(ETTP_SelectChart);
AutoScreenMessage(ETTP_StartChart);
AutoScreenMessage(ETTP_Disconnect);

REGISTER_SCREEN_CLASS(ScreenNetSelectMusic);

static LocalizedString PERMANENTLY_DELETE("ScreenSelectMusic",
										  "PermanentlyDelete");

void
ScreenNetSelectMusic::Init()
{
	ScreenSelectMusic::Init();
	GAMESTATE->m_bPlayingMulti = true;
	SAMPLE_MUSIC_PREVIEW_MODE.Load(m_sName, "SampleMusicPreviewMode");
	MUSIC_WHEEL_TYPE.Load(m_sName, "MusicWheelType");
	PLAYER_OPTIONS_SCREEN.Load(m_sName, "PlayerOptionsScreen");

	// todo: handle me theme-side -aj
	FOREACH_EnabledPlayer(p)
	{
		m_ModIconRow[p].SetName(ssprintf("ModIconsP%d", p + 1));
		m_ModIconRow[p].Load("ModIconRowSelectMusic", p);
		m_ModIconRow[p].SetFromGameState();
		LOAD_ALL_COMMANDS_AND_SET_XY(m_ModIconRow[p]);
		this->AddChild(&m_ModIconRow[p]);
	}

	// Load SFX and music
	m_soundChangeOpt.Load(THEME->GetPathS(m_sName, "change opt"));
	m_soundChangeSel.Load(THEME->GetPathS(m_sName, "change sel"));
	m_sSectionMusicPath = THEME->GetPathS(m_sName, "section music");
	m_sRouletteMusicPath = THEME->GetPathS(m_sName, "roulette music");
	m_sRandomMusicPath = THEME->GetPathS(m_sName, "random music");

	NSMAN->OnMusicSelect();

	m_bInitialSelect = false;
	m_bAllowInput = NSMAN->IsETTP();

	SAMPLE_MUSIC_FALLBACK_FADE_IN_SECONDS.Load(
	  m_sName, "SampleMusicFallbackFadeInSeconds");
	SAMPLE_MUSIC_FADE_OUT_SECONDS.Load(m_sName, "SampleMusicFadeOutSeconds");
	ALIGN_MUSIC_BEATS.Load(m_sName, "AlignMusicBeat");
}

void
ScreenNetSelectMusic::DifferentialReload()
{
	SONGMAN->DifferentialReload();
	m_MusicWheel.ReloadSongList(false, "");
}

bool
ScreenNetSelectMusic::Input(const InputEventPlus& input)
{
	// if (input.pn == PLAYER_2)	could use this to throw out all p2 inputs
	// -mina
	//	return false;

	if (input.DeviceI.button == KEY_KP_ENTER)
		return false;

	if (!m_bAllowInput || IsTransitioning())
		return ScreenWithMenuElements::Input(input);

	if (input.type == IET_RELEASE) {
		m_MusicWheel.Move(0);
		return true;
	}

	if (input.type != IET_FIRST_PRESS && input.type != IET_REPEAT)
		return false;

	bool bHoldingCtrl =
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL)) ||
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL)) ||
	  (!NSMAN->useSMserver); // If we are disconnected, assume no chatting

	bool holding_shift =
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)) ||
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT));

	wchar_t c = INPUTMAN->DeviceInputToChar(input.DeviceI, false);
	MakeUpper(&c, 1);

	bool handled = false;

	/* I'm commenting this here and adding the 2 ctrl+key and the ctrl+shift+key
	inputs
	// Ctrl+[A-Z] to go to that letter of the alphabet
	if( bHoldingCtrl && ( c >= 'A' ) && ( c <= 'Z' ) )
	{
		SortOrder so = GAMESTATE->m_SortOrder;
		if( ( so != SORT_TITLE ) && ( so != SORT_ARTIST ) )
		{
			so = SORT_TITLE;

			GAMESTATE->m_PreferredSortOrder = so;
			GAMESTATE->m_SortOrder.Set( so );
			// Odd, changing the sort order requires us to call SetOpenSection
	more than once m_MusicWheel.ChangeSort( so ); m_MusicWheel.SetOpenSection(
	ssprintf("%c", c ) );
		}
		m_MusicWheel.SelectSection( ssprintf("%c", c ) );
		m_MusicWheel.ChangeSort( so );
		m_MusicWheel.SetOpenSection( ssprintf("%c", c ) );
		m_MusicWheel.Move(+1);
		handled = true;
	}
	*/
	if (holding_shift && bHoldingCtrl && input.type == IET_FIRST_PRESS &&
		m_MusicWheel.IsSettled()) {
		if (c == 'R') {
			// Reload the currently selected song. -Kyz
			Song* to_reload = m_MusicWheel.GetSelectedSong();
			if (to_reload != nullptr) {
				to_reload->ReloadFromSongDir();
				this->AfterMusicChange();
				handled = true;
			}
		} else if (c == 'F') {
			// Favorite the currently selected song. -Not Kyz
			Song* fav_me_biatch = m_MusicWheel.GetSelectedSong();
			if (fav_me_biatch) {
				Profile* pProfile = PROFILEMAN->GetProfile(PLAYER_1);

				if (!fav_me_biatch->IsFavorited()) {
					fav_me_biatch->SetFavorited(true);
					pProfile->AddToFavorites(
					  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
				} else {
					fav_me_biatch->SetFavorited(false);
					pProfile->RemoveFromFavorites(
					  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
				}
				Message msg("FavoritesUpdated");
				MESSAGEMAN->Broadcast(msg);
				m_MusicWheel.ChangeMusic(0);
				return true;
			}
		} else if (c == 'M') {
			// PermaMirror the currently selected song. -Not Kyz
			Song* alwaysmirrorsmh = m_MusicWheel.GetSelectedSong();
			if (alwaysmirrorsmh) {
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
		} else if (c == 'G') {
			Profile* pProfile = PROFILEMAN->GetProfile(PLAYER_1);
			pProfile->AddGoal(GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
			Song* asonglol = m_MusicWheel.GetSelectedSong();
			asonglol->SetHasGoal(true);
			MESSAGEMAN->Broadcast("FavoritesUpdated");
			m_MusicWheel.ChangeMusic(0);
			return true;
		} else if (c == 'Q') {
			DifferentialReload();
			return true;
		} else if (c == 'S') {
			PROFILEMAN->SaveProfile(PLAYER_1);
			SCREENMAN->SystemMessage("Profile Saved");
			return true;
		} else if (input.DeviceI.device == DEVICE_KEYBOARD &&
				   input.DeviceI.button == KEY_BACK) {
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
	return ScreenSelectMusic::Input(input) || handled;
}

void
ScreenNetSelectMusic::HandleScreenMessage(const ScreenMessage SM)
{
	if (SM == SM_GoToNextScreen)
		SOUND->StopMusic();
	else if (SM == SM_UsersUpdate) {
		MESSAGEMAN->Broadcast("UsersUpdate");
	} else if (SM == SM_FriendsUpdate) {
		MESSAGEMAN->Broadcast("FriendsUpdate");
	} else if (SM == SM_GoToPrevScreen) {
		NSMAN->LeaveRoom();
		SCREENMAN->SetNewScreen(THEME->GetMetric(m_sName, "PrevScreen"));
	} else if (SM == SM_GoToNextScreen) {
		GAMESTATE->m_bInNetGameplay = true;
		SOUND->StopMusic();
		SCREENMAN->SetNewScreen(THEME->GetMetric(m_sName, "NextScreen"));
	} else if (SM == SM_GoToDisconnectScreen) {
		SOUND->StopMusic();
		SCREENMAN->SetNewScreen(THEME->GetMetric(m_sName, "DisconnectScreen"));
	} else if (SM == ETTP_Disconnect) {
		SOUND->StopMusic();
		TweenOffScreen();
		Cancel(SM_GoToDisconnectScreen);
	} else if (SM == SM_UsersUpdate) {
		m_MusicWheel.Move(0);
	} else if (SM == SM_NoSongs) {
		SCREENMAN->SetNewScreen(THEME->GetMetric(m_sName, "NoSongsScreen"));
	} else if (SM == SM_SetWheelSong) // After we've done the sort on wheel,
									  // select song.
	{
		m_MusicWheel.SelectSong(m_cSong);
	} else if (SM == SM_RefreshWheelLocation) {
		m_MusicWheel.Select();
		m_MusicWheel.Move(-1);
		m_MusicWheel.Move(1);
		m_MusicWheel.Select();
		m_bAllowInput = true;
	} else if (SM == SM_BackFromPlayerOptions) {
		GAMESTATE->m_EditMode = EditMode_Invalid;
		// XXX HACK: This will cause ScreenSelectOptions to go back here.
		NSMAN->OffOptions();

		// Update changes
		FOREACH_EnabledPlayer(p) m_ModIconRow[p].SetFromGameState();
	} else if (SM == SM_SongChanged) {
		if (m_MusicWheel.GetNumItems() > 0) {
			GAMESTATE->m_pCurSong.Set(m_MusicWheel.GetSelectedSong());
			this->AfterMusicChange();
		}
	} else if (SM == ETTP_StartChart) {
		if (NSMAN->song != nullptr) {
			GAMESTATE->m_pCurSong.Set(NSMAN->song);
			if (NSMAN->steps != nullptr) {
				m_vpSteps[m_iSelection[PLAYER_1]] = NSMAN->steps;
				GAMESTATE->m_pCurSteps[PLAYER_1].Set(NSMAN->steps);
				GAMESTATE->m_PreferredDifficulty[PLAYER_1].Set(
				  NSMAN->steps->GetDifficulty());
			}
			if (!m_MusicWheel.SelectSong(NSMAN->song)) {
				m_MusicWheel.ChangeSort(SORT_GROUP);
				m_MusicWheel.FinishTweening();
				SCREENMAN->PostMessageToTopScreen(SM_SetWheelSong, 0.710f);
				m_MusicWheel.SelectSong(NSMAN->song);
			}
			if (NSMAN->rate > 0) {
				GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate =
				  NSMAN->rate / 1000.f;
				MESSAGEMAN->Broadcast("RateChanged");
			}
			m_MusicWheel.Select();
			m_MusicWheel.Move(-1);
			m_MusicWheel.Move(1);
			StartSelectedSong();
			m_MusicWheel.Select();
		}
	} else if (SM == ETTP_SelectChart) {
		if (NSMAN->song != nullptr) {
			GAMESTATE->m_pCurSong.Set(NSMAN->song);
			if (NSMAN->steps != nullptr) {
				m_vpSteps[m_iSelection[PLAYER_1]] = NSMAN->steps;
				GAMESTATE->m_pCurSteps[PLAYER_1].Set(NSMAN->steps);
				GAMESTATE->m_PreferredDifficulty[PLAYER_1].Set(
				  NSMAN->steps->GetDifficulty());
			}
			if (!m_MusicWheel.SelectSong(NSMAN->song)) {
				m_MusicWheel.ChangeSort(SORT_GROUP);
				m_MusicWheel.FinishTweening();
				SCREENMAN->PostMessageToTopScreen(SM_SetWheelSong, 0.710f);
				m_MusicWheel.SelectSong(NSMAN->song);
			}
			if (NSMAN->rate > 0) {
				GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate =
				  NSMAN->rate / 1000.f;
				MESSAGEMAN->Broadcast("RateChanged");
			}
			m_MusicWheel.Select();
			m_MusicWheel.Move(-1);
			m_MusicWheel.Move(1);
			m_MusicWheel.Select();
		}
	} else if (SM == SM_ConfirmDeleteSong) {
		if (ScreenPrompt::s_LastAnswer == ANSWER_YES) {
			OnConfirmSongDeletion();
		} else {
			// need to resume the song preview that was automatically paused
			m_MusicWheel.ChangeMusic(0);
		}
	}

done:
	// Must be at end, as so it is last resort for SMOnline packets.
	// If it doesn't know what to do, then it'll just remove them.
	ScreenSelectMusic::HandleScreenMessage(SM);
}

void
ScreenNetSelectMusic::OnConfirmSongDeletion()
{
	Song* deletedSong = m_pSongAwaitingDeletionConfirmation;
	if (!deletedSong) {
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
	m_pSongAwaitingDeletionConfirmation = NULL;
}

bool
ScreenNetSelectMusic::LeftAndRightPressed(const PlayerNumber pn)
{
	return INPUTMAPPER->IsBeingPressed(GAME_BUTTON_LEFT, pn) &&
		   INPUTMAPPER->IsBeingPressed(GAME_BUTTON_RIGHT, pn);
}

bool
ScreenNetSelectMusic::MenuLeft(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;

	if (LeftAndRightPressed(pn))
		m_MusicWheel.ChangeSort(SORT_MODE_MENU);
	else
		m_MusicWheel.Move(-1);
	return true;
}

bool
ScreenNetSelectMusic::MenuRight(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;

	if (LeftAndRightPressed(pn))
		m_MusicWheel.ChangeSort(SORT_MODE_MENU);
	else
		m_MusicWheel.Move(+1);
	return true;
}

bool
ScreenNetSelectMusic::MenuStart(const InputEventPlus& input)
{
	// dont allow ctrl + enter to select songs... technically if there's enough
	// lag for some reason we can hit here from a ctrl+enter input but ctrl may
	// be released by now, though this is unlikely to happen -mina
	if (INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL)))
		return false;
	return SelectCurrent();
}
bool
ScreenNetSelectMusic::SelectCurrent()
{

	bool bResult = m_MusicWheel.Select();

	if (!bResult)
		return true;

	if (m_MusicWheel.GetSelectedType() != WheelItemDataType_Song)
		return true;

	Song* pSong = m_MusicWheel.GetSelectedSong();

	if (pSong == NULL)
		return false;

	GAMESTATE->m_pCurSong.Set(pSong);
	Steps* pSteps = m_vpSteps[m_iSelection[PLAYER_1]];
	GAMESTATE->m_pCurSteps[PLAYER_1].Set(pSteps);
	GAMESTATE->m_PreferredDifficulty[PLAYER_1].Set(pSteps->GetDifficulty());

	if (NSMAN->useSMserver) {
		NSMAN->m_sArtist = pSong->GetTranslitArtist();
		NSMAN->m_sMainTitle = pSong->GetTranslitMainTitle();
		NSMAN->m_sSubTitle = pSong->GetTranslitSubTitle();
		NSMAN->m_iSelectMode = 2; // Command for user selecting song
		NSMAN->SelectUserSong();
	} else
		StartSelectedSong();
	return true;
}
bool
ScreenNetSelectMusic::MenuBack(const InputEventPlus& input)
{
	SOUND->StopMusic();
	TweenOffScreen();

	Cancel(SM_GoToPrevScreen);
	return true;
}

void
ScreenNetSelectMusic::TweenOffScreen()
{
	ScreenSelectMusic::TweenOffScreen();

	NSMAN->OffMusicSelect();
}

void
ScreenNetSelectMusic::StartSelectedSong()
{
	Song* pSong = m_MusicWheel.GetSelectedSong();
	GAMESTATE->m_pCurSong.Set(pSong);
	FOREACH_EnabledPlayer(pn)
	{
		StepsType st = GAMESTATE->GetCurrentStyle(pn)
						 ->m_StepsType; // StepsType_dance_single;
		Steps* pSteps = m_vpSteps[m_iSelection[PLAYER_1]];
		GAMESTATE->m_PreferredDifficulty[pn].Set(pSteps->GetDifficulty());
		GAMESTATE->m_pCurSteps[pn].Set(pSteps);
	}

	GAMESTATE->m_PreferredSortOrder = GAMESTATE->m_SortOrder;
	GAMESTATE->m_pPreferredSong = pSong;

	// force event mode
	GAMESTATE->m_bTemporaryEventMode = true;

	TweenOffScreen();
	StartTransitioningScreen(SM_GoToNextScreen);
}

/*
void
ScreenNetSelectMusic::UpdateDifficulties(PlayerNumber pn)
{
	if (GAMESTATE->m_pCurSong == NULL) {
		m_StepsDisplays[pn].SetFromStepsTypeAndMeterAndDifficultyAndCourseType(
		  StepsType_Invalid, 0, Difficulty_Beginner);
		// m_DifficultyIcon[pn].SetFromSteps( pn, NULL );	// It will blank it
		// out
		return;
	}

	StepsType st = GAMESTATE->GetCurrentStyle(pn)->m_StepsType;

	Steps* pSteps =
	  SongUtil::GetStepsByDifficulty(GAMESTATE->m_pCurSong, st, m_DC[pn]);
	GAMESTATE->m_pCurSteps[pn].Set(pSteps);

	if ((m_DC[pn] < NUM_Difficulty) && (m_DC[pn] >= Difficulty_Beginner))
		m_StepsDisplays[pn].SetFromSteps(pSteps);
	else
		m_StepsDisplays[pn].SetFromStepsTypeAndMeterAndDifficultyAndCourseType(
		  StepsType_Invalid, 0, Difficulty_Beginner);
}
*/

void
ScreenNetSelectMusic::BeginScreen()
{
	Profile* prof = PROFILEMAN->GetProfile(PLAYER_1);
	SONGMAN->MakeSongGroupsFromPlaylists();
	SONGMAN->SetFavoritedStatus(prof->FavoritedCharts);
	SONGMAN->SetHasGoal(prof->goalmap);
	ScreenSelectMusic::BeginScreen();
}

void
ScreenNetSelectMusic::Update(float fDeltaTime)
{
	if (!m_bInitialSelect) {
		m_bInitialSelect = true;
		SCREENMAN->PostMessageToTopScreen(SM_RefreshWheelLocation, 1.0f);
	}
	ScreenSelectMusic::Update(fDeltaTime);
}

MusicWheel*
ScreenNetSelectMusic::GetMusicWheel()
{
	return &m_MusicWheel;
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the PlayerState. */
class LunaScreenNetSelectMusic : public Luna<ScreenNetSelectMusic>
{
  public:
	static int GetMusicWheel(T* p, lua_State* L)
	{
		p->GetMusicWheel()->PushSelf(L);
		return 1;
	}
	static int SelectCurrent(T* p, lua_State* L)
	{
		p->SelectCurrent();
		return 1;
	}
	static int GetSelectionState(T* p, lua_State* L)
	{
		lua_pushnumber(L, NSMAN->m_iSelectMode);
		return 1;
	}
	static int GetUserQty(T* p, lua_State* L)
	{
		auto& users = NSMAN->m_PlayerNames;
		lua_pushnumber(L, users.size());
		return 1;
	}
	static int GetUser(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
			return 0;
		auto& users = NSMAN->m_PlayerNames;
		if (static_cast<size_t>(IArg(1)) <= users.size() && IArg(1) >= 1)
			lua_pushstring(L, users[IArg(1) - 1].c_str());
		else
			lua_pushstring(L, "");
		return 1;
	}
	static int GetUserState(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
			return 0;
		auto& states = NSMAN->m_PlayerStatus;
		if (static_cast<size_t>(IArg(1)) <= states.size() && IArg(1) >= 1)
			lua_pushnumber(L, states[IArg(1) - 1]);
		else
			lua_pushnumber(L, 0);
		return 1;
	}
	LunaScreenNetSelectMusic()
	{
		ADD_METHOD(GetMusicWheel);
		ADD_METHOD(SelectCurrent);
		ADD_METHOD(GetSelectionState);
		ADD_METHOD(GetUser);
		ADD_METHOD(GetUserQty);
		ADD_METHOD(GetUserState);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenNetSelectMusic, ScreenSelectMusic)
// lua end

#endif
/*
 * (c) 2004-2005 Charles Lohr
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
