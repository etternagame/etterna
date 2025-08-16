#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenSMOnlineLogin.h"
#include "Etterna/Screen/Others/ScreenTextEntry.h"
#include "Etterna/Singletons/ThemeManager.h"

REGISTER_SCREEN_CLASS(ScreenSMOnlineLogin);

AutoScreenMessage(SM_PasswordDone);
AutoScreenMessage(SM_UsernameDone);
AutoScreenMessage(SM_NoProfilesDefined);

AutoScreenMessage(ETTP_Disconnect);
AutoScreenMessage(ETTP_LoginResponse);

static LocalizedString DEFINE_A_PROFILE("ScreenSMOnlineLogin",
										"You must define a Profile.");
static LocalizedString LOGIN_BUTTON("ScreenSMOnlineLogin", "LoginButton");
static LocalizedString ENTER_USERNAME("ScreenSMOnlineLogin", "EnterUsername");
static LocalizedString TYPE_USERNAME("ScreenSMOnlineLogin", "TypeUsername");
static LocalizedString YOU_ARE_LOGGING_ON_AS("ScreenSMOnlineLogin",
											 "You are logging on as:");
static LocalizedString ENTER_YOUR_PASSWORD("ScreenSMOnlineLogin",
										   "Enter your password.");
static LocalizedString NO_USERNAME("ScreenSMOnlineLogin",
										"No username entered");

void
ScreenSMOnlineLogin::Init()
{
	ScreenWithMenuElements::Init();
	m_iPlayer = 0;

	SOUND->PlayMusic(THEME->GetPathS("ScreenOptionsServiceChild", "music"));

	PostScreenMessage(SM_GoToNextScreen, 0);
}


void
ScreenSMOnlineLogin::HandleScreenMessage(const ScreenMessage& SM)
{
	std::string sLoginQuestion;
	//	if( GAMESTATE->IsPlayerEnabled((PlayerNumber) m_iPlayer) )

	if (SM == SM_PasswordDone) {
		if (!ScreenTextEntry::s_bCancelledLast)
			SendLogin(ScreenTextEntry::s_sLastAnswer, username);
		else
			SCREENMAN->PostMessageToTopScreen(SM_GoToPrevScreen, 0);
	} else if (SM == SM_UsernameDone) {
		username = ScreenTextEntry::s_sLastAnswer.c_str();
		sLoginQuestion =
		  YOU_ARE_LOGGING_ON_AS.GetValue() + "\n" +
		  username + "\n" +
		  ENTER_YOUR_PASSWORD.GetValue();
		if (username == "") {
			SCREENMAN->SystemMessage(NO_USERNAME);
			Cancel(SM_GoToPrevScreen);
		} else {
			ScreenTextEntry::s_bMustResetInputRedirAtClose = true;
			ScreenTextEntry::Password(SM_PasswordDone,
								  NSMAN->loginResponse + "\n\n" +
									sLoginQuestion,
								  NULL);
		}
	} else if (SM == SM_NoProfilesDefined) {
		SCREENMAN->SystemMessage(DEFINE_A_PROFILE);
		SCREENMAN->SetNewScreen("ScreenOptionsManageProfiles");
	} else if (SM == ETTP_Disconnect) {
		Cancel(SM_GoToPrevScreen);
	} else if (SM == ETTP_LoginResponse) {
		if (NSMAN->loggedIn) {
			SCREENMAN->SetNewScreen(
			  THEME->GetMetric("ScreenSMOnlineLogin", "NextScreen"));
			m_iPlayer = 0;
		} else {
			sLoginQuestion = ENTER_USERNAME.GetValue();
			ScreenTextEntry::s_bMustResetInputRedirAtClose = true;
			ScreenTextEntry::TextEntry(SM_UsernameDone,
										NSMAN->loginResponse + "\n\n" +
											sLoginQuestion,
										"",
										255);
		}
	} else if (SM == SM_GoToNextScreen) {
		Locator::getLogger()->trace("[ScreenSMOnlineLogin::HandleScreenMessage] GoToNextScreen");

		PREFSMAN->SavePrefsToDisk();
		PROFILEMAN->LoadLocalProfileFromMachine(PLAYER_1);

		sLoginQuestion = ENTER_USERNAME.GetValue();
		ScreenTextEntry::s_bMustResetInputRedirAtClose = true;
		ScreenTextEntry::TextEntry(SM_UsernameDone,
									NSMAN->loginResponse + "\n\n" +
										sLoginQuestion,
									"",
									255);
		return;
	}

	ScreenWithMenuElements::HandleScreenMessage(SM);
}

void
ScreenSMOnlineLogin::SendLogin(std::string sPassword, std::string user)
{
	NSMAN->Login(user, sPassword);
}
