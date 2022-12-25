#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Singletons/AnnouncerManager.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/FilterManager.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Actor/Menus/MusicWheel.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenNetSelectMusic.h"
#include "ScreenNetSelectBase.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"

#include <vector>

AutoScreenMessage(SM_AddToChat);
AutoScreenMessage(SM_FriendsUpdate);
AutoScreenMessage(SM_NoSongs);
AutoScreenMessage(SM_SetWheelSong);
AutoScreenMessage(SM_RefreshWheelLocation);
AutoScreenMessage(SM_SongChanged);
AutoScreenMessage(SM_BackFromPlayerOptions);
AutoScreenMessage(ETTP_SelectChart);
AutoScreenMessage(ETTP_StartChart);
AutoScreenMessage(ETTP_Disconnect);

REGISTER_SCREEN_CLASS(ScreenNetSelectMusic);

static LocalizedString PERMANENTLY_DELETE("ScreenSelectMusic",
										  "PermanentlyDelete");

void
ScreenNetSelectMusic::Init()
{
	if (NSMAN->song != nullptr)
		GAMESTATE->m_pCurSong.Set(NSMAN->song);
	if (NSMAN->steps != nullptr)
		GAMESTATE->m_pCurSteps.Set(NSMAN->steps);
	ScreenSelectMusic::Init();
	GAMESTATE->m_bPlayingMulti = true;
	// Load SFX and music
	m_soundChangeOpt.Load(THEME->GetPathS(m_sName, "change opt"));
	m_soundChangeSel.Load(THEME->GetPathS(m_sName, "change sel"));

	NSMAN->OnMusicSelect();

	m_bInitialSelect = false;
	m_bAllowInput = NSMAN->IsETTP();
}

bool
ScreenNetSelectMusic::Input(const InputEventPlus& input)
{
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

	return ScreenSelectMusic::Input(input);
}

void
SelectSongUsingNSMAN(ScreenNetSelectMusic* s, bool start)
{
	auto ptrMusicWheel = s->GetMusicWheel();
	auto& m_MusicWheel = *ptrMusicWheel;
	if (NSMAN->song != nullptr) {
		GAMESTATE->m_pCurSong.Set(NSMAN->song);
		if (NSMAN->steps != nullptr) {
			GAMESTATE->m_pCurSteps.Set(NSMAN->steps);
			GAMESTATE->m_PreferredDifficulty.Set(NSMAN->steps->GetDifficulty());
		}
		if (!m_MusicWheel.SelectSong(NSMAN->song)) {
			FILTERMAN->filteringCommonPacks = false;
			FILTERMAN->ResetSSFilters();
			m_MusicWheel.ChangeSort(SORT_GROUP);
			m_MusicWheel.FinishTweening();
			m_MusicWheel.ReloadSongList(false, "");
			SCREENMAN->PostMessageToTopScreen(SM_SetWheelSong, 0.710f);
			m_MusicWheel.SelectSong(NSMAN->song);
		}
		if (NSMAN->rate > 0) {
			GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate =
			  NSMAN->rate / 1000.f;
			GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate =
			  NSMAN->rate / 1000.f;
			GAMESTATE->m_SongOptions.GetSong().m_fMusicRate =
			  NSMAN->rate / 1000.f;
			MESSAGEMAN->Broadcast("RateChanged");
			MESSAGEMAN->Broadcast("CurrentRateChanged");
		}
		m_MusicWheel.Select();
		m_MusicWheel.Move(-1);
		m_MusicWheel.Move(1);
		m_MusicWheel.Move(0);
		if (start) {
			s->StartSelectedSong();
			m_MusicWheel.Select();
		}
	}
}
void
ScreenNetSelectMusic::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_GoToNextScreen) {
		SOUND->StopMusic();
		if (NSMAN->song != nullptr) {
			GAMESTATE->m_pCurSong.Set(NSMAN->song);
			if (NSMAN->steps != nullptr) {
				GAMESTATE->m_pCurSteps.Set(NSMAN->steps);
				GAMESTATE->m_PreferredDifficulty.Set(
				  NSMAN->steps->GetDifficulty());
			}
			if (!m_MusicWheel.SelectSong(NSMAN->song)) {
				FILTERMAN->filteringCommonPacks = false;
				FILTERMAN->ResetSSFilters();
				m_MusicWheel.ChangeSort(SORT_GROUP);
				m_MusicWheel.FinishTweening();
				m_MusicWheel.ReloadSongList(false, "");
				SCREENMAN->PostMessageToTopScreen(SM_SetWheelSong, 0.710f);
				m_MusicWheel.SelectSong(NSMAN->song);
			}
			if (NSMAN->rate > 0) {
				GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate =
				  NSMAN->rate / 1000.f;
				GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate =
				  NSMAN->rate / 1000.f;
				GAMESTATE->m_SongOptions.GetSong().m_fMusicRate =
				  NSMAN->rate / 1000.f;
				MESSAGEMAN->Broadcast("RateChanged");
				MESSAGEMAN->Broadcast("CurrentRateChanged");
			}
			m_MusicWheel.Select();
			m_MusicWheel.Move(-1);
			m_MusicWheel.Move(1);
		}
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
	} else if (SM == SM_NoSongs) {
		SCREENMAN->SetNewScreen(THEME->GetMetric(m_sName, "NoSongsScreen"));
	} else if (SM == SM_RefreshWheelLocation) {
		m_MusicWheel.Select();
		m_MusicWheel.Move(-1);
		m_MusicWheel.Move(1);
		m_MusicWheel.Select();
		m_bAllowInput = true;
	} else if (SM == SM_BackFromPlayerOptions) {
		NSMAN->OffOptions();
	} else if (SM == SM_SongChanged) {
		if (m_MusicWheel.GetNumItems() > 0) {
			GAMESTATE->m_pCurSong.Set(m_MusicWheel.GetSelectedSong());
			this->AfterMusicChange();
		}
	} else if (SM == ETTP_StartChart) {
		SelectSongUsingNSMAN(this, true);
	} else if (SM == ETTP_SelectChart) {
		SelectSongUsingNSMAN(this, false);
	}

	// Must be at end, as so it is last resort for SMOnline packets.
	// If it doesn't know what to do, then it'll just remove them.
	ScreenSelectMusic::HandleScreenMessage(SM);
}
ScreenNetSelectMusic::~ScreenNetSelectMusic()
{
	Locator::getLogger()->debug("ScreenNetSelectMusic::~ScreenNetSelectMusic()");
}

bool
ScreenNetSelectMusic::MenuLeft(const InputEventPlus& input)
{
	m_MusicWheel.Move(-1);
	return true;
}

bool
ScreenNetSelectMusic::MenuRight(const InputEventPlus& input)
{
	m_MusicWheel.Move(+1);
	return true;
}

void
ScreenNetSelectMusic::OpenOptions()
{
	NSMAN->OnOptions();
	SCREENMAN->AddNewScreenToTop(PLAYER_OPTIONS_SCREEN,
								 SM_BackFromPlayerOptions);
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
	if (static_cast<int>(m_vpSteps.size()) <= m_iSelection)
		return false;
	GAMESTATE->m_pCurSong.Set(pSong);
	Steps* pSteps = m_vpSteps[m_iSelection];
	GAMESTATE->m_pCurSteps.Set(pSteps);
	GAMESTATE->m_PreferredDifficulty.Set(pSteps->GetDifficulty());

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
	GAMESTATE->m_PreferredSortOrder = GAMESTATE->m_SortOrder;
	GAMESTATE->m_pPreferredSong = GAMESTATE->m_pCurSong;

	// force event mode
	GAMESTATE->m_bTemporaryEventMode = true;

	TweenOffScreen();
	StartTransitioningScreen(SM_GoToNextScreen);
}

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
#include "Etterna/Models/Lua/LuaBinding.h"

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
	static int GetUserReady(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
			return 0;
		auto& states = NSMAN->m_PlayerReady;
		if (static_cast<size_t>(IArg(1)) <= states.size() && IArg(1) >= 1)
			lua_pushboolean(L, states[IArg(1) - 1]);
		else
			lua_pushboolean(L, false);
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
		ADD_METHOD(GetUserReady);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenNetSelectMusic, ScreenSelectMusic)
// lua end
