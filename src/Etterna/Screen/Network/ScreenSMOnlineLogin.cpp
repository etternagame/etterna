#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Models/Misc/OptionRowHandler.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Screen/Others/ScreenPrompt.h"
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

void
ScreenSMOnlineLogin::Init()
{
	ScreenOptions::Init();
	m_iPlayer = 0;

	vector<OptionRowHandler*> vHands;

	OptionRowHandler* pHand = OptionRowHandlerUtil::MakeNull();

	pHand->m_Def.m_sName = "Profile";
	pHand->m_Def.m_bOneChoiceForAllPlayers = false;
	pHand->m_Def.m_bAllowThemeItems = false;
	pHand->m_Def.m_vEnabledForPlayers.clear();

	pHand->m_Def.m_vEnabledForPlayers.insert(PLAYER_1);

	PROFILEMAN->GetLocalProfileDisplayNames(pHand->m_Def.m_vsChoices);
	pHand->m_Def.m_vsChoices.emplace_back(TYPE_USERNAME.GetValue());
	if (pHand->m_Def.m_vsChoices.empty()) {
		// Give myself a message so that I can bail out later
		PostScreenMessage(SM_NoProfilesDefined, 0);
		SAFE_DELETE(pHand);
	} else
		vHands.push_back(pHand);

	InitMenu(vHands);
	SOUND->PlayMusic(THEME->GetPathS("ScreenOptionsServiceChild", "music"));
	OptionRow& row = *m_pRows.back();
	row.SetExitText(LOGIN_BUTTON);
}

void
ScreenSMOnlineLogin::ImportOptions(int iRow, const PlayerNumber& vpns)
{
	switch (iRow) {
		case 0: {
			vector<std::string> vsProfiles;
			PROFILEMAN->GetLocalProfileIDs(vsProfiles);

			vector<std::string>::iterator iter =
			  find(vsProfiles.begin(),
				   vsProfiles.end(),
				   ProfileManager::m_sDefaultLocalProfileID[PLAYER_1].Get());
			if (iter != vsProfiles.end())
				m_pRows[0]->SetOneSelection((PlayerNumber)PLAYER_1,
											iter - vsProfiles.begin());
		} break;
	}
}

void
ScreenSMOnlineLogin::ExportOptions(int iRow, const PlayerNumber& vpns)
{
	switch (iRow) {
		case 0: {
			vector<std::string> vsProfiles;
			PROFILEMAN->GetLocalProfileIDs(vsProfiles);

			auto selection = m_pRows[0]->GetOneSelection(PLAYER_1);
			if (selection <
				static_cast<int>(
				  m_pRows[0]->GetHandler()->m_Def.m_vsChoices.size()) -
				  1) {
				ProfileManager::m_sDefaultLocalProfileID[PLAYER_1].Set(
				  vsProfiles[selection]);
				typeUsername = false;
			} else
				typeUsername = true;
		} break;
	}
}

static LocalizedString UNIQUE_PROFILE("ScreenSMOnlineLogin",
									  "Each player needs a unique Profile.");
static LocalizedString YOU_ARE_LOGGING_ON_AS("ScreenSMOnlineLogin",
											 "You are logging on as:");
static LocalizedString ENTER_YOUR_PASSWORD("ScreenSMOnlineLogin",
										   "Enter your password.");

void
ScreenSMOnlineLogin::HandleScreenMessage(const ScreenMessage& SM)
{
	std::string sLoginQuestion;
	//	if( GAMESTATE->IsPlayerEnabled((PlayerNumber) m_iPlayer) )

	if (SM == SM_PasswordDone) {
		if (!ScreenTextEntry::s_bCancelledLast)
			if (typeUsername)
				SendLogin(ScreenTextEntry::s_sLastAnswer, username);
			else
				SendLogin(ScreenTextEntry::s_sLastAnswer);
		else
			SCREENMAN->PostMessageToTopScreen(SM_GoToPrevScreen, 0);
	} else if (SM == SM_UsernameDone) {
		username = ScreenTextEntry::s_sLastAnswer.c_str();
		sLoginQuestion =
		  YOU_ARE_LOGGING_ON_AS.GetValue() + "\n" +
		  GAMESTATE->GetPlayerDisplayName((PlayerNumber)m_iPlayer) + "\n" +
		  ENTER_YOUR_PASSWORD.GetValue();
		ScreenTextEntry::Password(SM_PasswordDone,
								  NSMAN->loginResponse + "\n\n" +
									sLoginQuestion,
								  NULL);
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
			if (!typeUsername) {
				sLoginQuestion =
				  YOU_ARE_LOGGING_ON_AS.GetValue() + "\n" +
				  GAMESTATE->GetPlayerDisplayName((PlayerNumber)m_iPlayer) +
				  "\n" + ENTER_YOUR_PASSWORD.GetValue();
				ScreenTextEntry::Password(SM_PasswordDone,
										  NSMAN->loginResponse + "\n\n" +
											sLoginQuestion,
										  NULL);
			} else {
				sLoginQuestion = ENTER_USERNAME.GetValue();
				ScreenTextEntry::TextEntry(SM_UsernameDone,
										   NSMAN->loginResponse + "\n\n" +
											 sLoginQuestion,
										   "",
										   255);
			}
		}
	} else if (SM == SM_GoToNextScreen) {
		Locator::getLogger()->trace("[ScreenSMOnlineLogin::HandleScreenMessage] GoToNextScreen");
		for (unsigned r = 0; r < m_pRows.size(); r++)
			ExportOptions(r, GAMESTATE->GetMasterPlayerNumber());

		PREFSMAN->SavePrefsToDisk();
		PROFILEMAN->LoadLocalProfileFromMachine(PLAYER_1);

		if (GAMESTATE->IsPlayerEnabled((PlayerNumber)0) &&
			GAMESTATE->IsPlayerEnabled((PlayerNumber)1) &&
			(GAMESTATE->GetPlayerDisplayName((PlayerNumber)0) ==
			 GAMESTATE->GetPlayerDisplayName((PlayerNumber)1))) {
			SCREENMAN->SystemMessage(UNIQUE_PROFILE);
			SCREENMAN->SetNewScreen("ScreenSMOnlineLogin");
		} else {
			if (!typeUsername) {
				m_iPlayer = 0;
				while (!GAMESTATE->IsPlayerEnabled(
				  static_cast<PlayerNumber>(m_iPlayer)))
					++m_iPlayer;
				sLoginQuestion = YOU_ARE_LOGGING_ON_AS.GetValue() + "\n" +
								 GAMESTATE->GetPlayerDisplayName(
								   static_cast<PlayerNumber>(m_iPlayer)) +
								 "\n" + ENTER_YOUR_PASSWORD.GetValue();
				ScreenTextEntry::Password(
				  SM_PasswordDone, sLoginQuestion, NULL);
			} else {
				sLoginQuestion = ENTER_USERNAME.GetValue();
				ScreenTextEntry::TextEntry(SM_UsernameDone,
										   NSMAN->loginResponse + "\n\n" +
											 sLoginQuestion,
										   "",
										   255);
			}
		}
		return;
	}

	ScreenOptions::HandleScreenMessage(SM);
}

bool
ScreenSMOnlineLogin::MenuStart(const InputEventPlus& input)
{
	return ScreenOptions::MenuStart(input);
}

std::string
ScreenSMOnlineLogin::GetSelectedProfileID()
{
	vector<std::string> vsProfiles;
	PROFILEMAN->GetLocalProfileIDs(vsProfiles);

	const OptionRow& row = *m_pRows[GetCurrentRow()];
	const int Selection = row.GetOneSharedSelection();
	if (!Selection)
		return std::string();
	return vsProfiles[Selection - 1];
}

void
ScreenSMOnlineLogin::SendLogin(std::string sPassword)
{
	SendLogin(sPassword,
			  GAMESTATE->GetPlayerDisplayName((PlayerNumber)this->m_iPlayer));
}
void
ScreenSMOnlineLogin::SendLogin(std::string sPassword, std::string user)
{
	NSMAN->Login(user, sPassword);
}
