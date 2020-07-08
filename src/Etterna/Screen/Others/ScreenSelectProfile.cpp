#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenSelectProfile.h"

REGISTER_SCREEN_CLASS(ScreenSelectProfile);

void
ScreenSelectProfile::Init()
{
	// no selection initially
	m_iSelectedProfiles = -1;
	m_TrackingRepeatingInput = GameButton_Invalid;
	ScreenWithMenuElements::Init();
}

bool
ScreenSelectProfile::Input(const InputEventPlus& input)
{
	if (IsTransitioning())
		return false;

	return ScreenWithMenuElements::Input(input);
}

bool
ScreenSelectProfile::MenuLeft(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;
	if (m_fLockInputSecs > 0)
		return false;
	if (input.type == IET_RELEASE)
		return false;
	if (input.type != IET_FIRST_PRESS) {
		/*
		if( !ALLOW_REPEATING_INPUT )
			return false;
		*/
		if (m_TrackingRepeatingInput != input.MenuI)
			return false;
	}
	m_TrackingRepeatingInput = input.MenuI;
	MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuLeftP1 + pn));
	return true;
}

bool
ScreenSelectProfile::MenuRight(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;
	if (m_fLockInputSecs > 0)
		return false;
	if (input.type == IET_RELEASE)
		return false;
	if (input.type != IET_FIRST_PRESS) {
		/*
		if( !ALLOW_REPEATING_INPUT )
			return false;
		*/
		if (m_TrackingRepeatingInput != input.MenuI)
			return false;
	}
	m_TrackingRepeatingInput = input.MenuI;
	MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuRightP1 + pn));
	return true;
}

bool
ScreenSelectProfile::MenuUp(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;
	if (m_fLockInputSecs > 0)
		return false;
	if (input.type == IET_RELEASE)
		return false;
	if (input.type != IET_FIRST_PRESS) {
		/*
		if( !ALLOW_REPEATING_INPUT )
			return false;
		*/
		if (m_TrackingRepeatingInput != input.MenuI)
			return false;
	}
	m_TrackingRepeatingInput = input.MenuI;
	MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuUpP1 + pn));
	return true;
}

bool
ScreenSelectProfile::MenuDown(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;
	if (m_fLockInputSecs > 0)
		return false;
	if (input.type == IET_RELEASE)
		return false;
	if (input.type != IET_FIRST_PRESS) {
		/*
		if( !ALLOW_REPEATING_INPUT )
			return false;
		*/
		if (m_TrackingRepeatingInput != input.MenuI)
			return false;
	}
	m_TrackingRepeatingInput = input.MenuI;
	MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuDownP1 + pn));
	return true;
}

bool
ScreenSelectProfile::SetProfileIndex(PlayerNumber pn, int iProfileIndex)
{
	if (!GAMESTATE->IsHumanPlayer(pn)) {
		if (iProfileIndex == -1) {
			GAMESTATE->JoinPlayer(pn);
			SCREENMAN->PlayStartSound();
			return true;
		}
		return false;
	}

	if (iProfileIndex > PROFILEMAN->GetNumLocalProfiles())
		return false;

	// wrong selection
	if (iProfileIndex < -2)
		return false;

	// unload player
	if (iProfileIndex == -2) {
		// GAMESTATE->UnjoinPlayer takes care of unloading the profile.
		GAMESTATE->UnjoinPlayer(pn);
		m_iSelectedProfiles = -1;
		return true;
	}

	m_iSelectedProfiles = iProfileIndex;

	return true;
}

bool
ScreenSelectProfile::Finish()
{
	if (GAMESTATE->GetNumPlayersEnabled() == 0)
		return false;

	// if profile indexes are the same for both players
	if (GAMESTATE->GetNumPlayersEnabled() == 2 && m_iSelectedProfiles > 0)
		return false;

	int iUsedLocalProfiles = 0;
	int iUnselectedProfiles = 0;

	// not all players has made their choices
	if (GAMESTATE->IsHumanPlayer(PLAYER_1) && (m_iSelectedProfiles == -1))
		iUnselectedProfiles++;

	// card not ready
	if (m_iSelectedProfiles == 0)
		return false;

	// profile index too big
	if (m_iSelectedProfiles > PROFILEMAN->GetNumLocalProfiles())
		return false;

	// inc used profile count
	if (m_iSelectedProfiles > 0)
		iUsedLocalProfiles++;

	// this allows to continue if there is less local profiles than number of
	// human players
	if ((iUnselectedProfiles != 0) &&
		iUsedLocalProfiles < PROFILEMAN->GetNumLocalProfiles())
		return false;

	// all ok - load profiles and go to next screen
	PROFILEMAN->UnloadProfile(PLAYER_1);

	if (m_iSelectedProfiles > 0) {
		PROFILEMAN->m_sDefaultLocalProfileID[PLAYER_1].Set(
		  PROFILEMAN->GetLocalProfileIDFromIndex(m_iSelectedProfiles - 1));
		PROFILEMAN->LoadLocalProfileFromMachine(PLAYER_1);
		GAMESTATE->LoadCurrentSettingsFromProfile(PLAYER_1);
	}
	StartTransitioningScreen(SM_GoToNextScreen);
	return true;
}

void
ScreenSelectProfile::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_MenuTimer) {
		bool bFinished = Finish();
		if (!bFinished) {
			// TODO: we need to decide how to handle unfinished business.
		}
	}

	ScreenWithMenuElements::HandleScreenMessage(SM);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ScreenSelectProfile. */
class LunaScreenSelectProfile : public Luna<ScreenSelectProfile>
{
  public:
	static int SetProfileIndex(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;
		int iProfileIndex = IArg(2);
		bool bRet = p->SetProfileIndex(pn, iProfileIndex);
		LuaHelpers::Push(L, bRet);
		return 1;
	}

	static int GetProfileIndex(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;
		LuaHelpers::Push(L, p->GetProfileIndex(pn));
		return 1;
	}

	static int Finish(T* p, lua_State* L)
	{
		bool bRet = p->Finish();
		LuaHelpers::Push(L, bRet);
		return 1;
	}

	LunaScreenSelectProfile()
	{
		ADD_METHOD(SetProfileIndex);
		ADD_METHOD(GetProfileIndex);
		ADD_METHOD(Finish);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenSelectProfile, ScreenWithMenuElements)
