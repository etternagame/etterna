#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Models/Misc/OptionRowHandler.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenNetworkOptions.h"
#include "Etterna/Screen/Others/ScreenPrompt.h"
#include "Etterna/Screen/Others/ScreenTextEntry.h"
#include "Etterna/Singletons/ThemeManager.h"

static LocalizedString CLIENT_CONNECT("ScreenNetworkOptions", "Connect");
static LocalizedString CLIENT_DISCONNECT("ScreenNetworkOptions", "Disconnect");
static LocalizedString SCORE_ON("ScreenNetworkOptions", "ScoreOn");
static LocalizedString SCORE_OFF("ScreenNetworkOptions", "ScoreOff");

static LocalizedString DISCONNECTED("ScreenNetworkOptions",
									"Disconnected from server.");
static LocalizedString ENTER_NETWORK_ADDRESS("ScreenNetworkOptions",
											 "Enter a network address.");
static LocalizedString CONNECT_TO_YOURSELF(
  "ScreenNetworkOptions",
  "Use 127.0.0.1 to connect to yourself.");

enum NetworkOptionRow
{
	PO_CONNECTION,
	PO_SCOREBOARD,
	PO_SERVERS,
	NUM_NETWORK_OPTIONS_LINES
};

enum DisplayScoreboard
{
	NO_SCOREBOARD_OFF = 0,
	NO_SCOREBOARD_ON
};

AutoScreenMessage(SM_DoneConnecting);

Preference<std::string> g_sLastServer("LastConnectedMultiServer",
									  "multi.etternaonline.com");

REGISTER_SCREEN_CLASS(ScreenNetworkOptions);

void
ScreenNetworkOptions::Init()
{
	ScreenOptions::Init();

	vector<OptionRowHandler*> vHands;
	{
		OptionRowHandler* pHand = OptionRowHandlerUtil::MakeNull();
		vHands.push_back(pHand);
		pHand->m_Def.m_sName = "Connection";
		pHand->m_Def.m_bOneChoiceForAllPlayers = true;
		pHand->m_Def.m_bAllowThemeItems = false;
		if (NSMAN->useSMserver)
			pHand->m_Def.m_vsChoices.push_back(CLIENT_DISCONNECT);
		else
			pHand->m_Def.m_vsChoices.push_back(CLIENT_CONNECT);
	}
	{
		OptionRowHandler* pHand = OptionRowHandlerUtil::MakeNull();
		vHands.push_back(pHand);
		pHand->m_Def.m_sName = "Scoreboard";
		pHand->m_Def.m_vsChoices.clear();
		pHand->m_Def.m_bOneChoiceForAllPlayers = true;
		pHand->m_Def.m_bAllowThemeItems = false;
		pHand->m_Def.m_vsChoices.push_back(SCORE_OFF);
		pHand->m_Def.m_vsChoices.push_back(SCORE_ON);
	}
	/*
	{
		// Get info on all received servers from NSMAN.
		AllServers.clear();
		NSMAN->GetListOfLANServers( AllServers );
		if( !AllServers.empty() )
		{
			OptionRowHandler *pHand = OptionRowHandlerUtil::MakeNull();
			pHand->m_Def.m_sName = "Servers";
			pHand->m_Def.m_bAllowThemeItems = false;
			for( unsigned int j = 0; j < AllServers.size(); j++ )
				pHand->m_Def.m_vsChoices.push_back( AllServers[j].Name );
			vHands.push_back( pHand );
		}
	}
	*/

	InitMenu(vHands);

	m_pRows[PO_SCOREBOARD]->SetOneSharedSelection(
	  PREFSMAN->m_bEnableScoreboard);
}

void
ScreenNetworkOptions::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_DoneConnecting) {
		if (!ScreenTextEntry::s_bCancelledLast) {
			std::string sNewName = ScreenTextEntry::s_sLastAnswer;
			NSMAN->PostStartUp(sNewName);
			UpdateConnectStatus();
		}
	}

	ScreenOptions::HandleScreenMessage(SM);
}

bool
ScreenNetworkOptions::MenuStart(const InputEventPlus& input)
{
	switch (GetCurrentRow()) {
		case PO_CONNECTION:
			if (!NSMAN->useSMserver) {
				ScreenTextEntry::TextEntry(SM_DoneConnecting,
										   ENTER_NETWORK_ADDRESS.GetValue() +
											 "\n\n" +
											 CONNECT_TO_YOURSELF.GetValue(),
										   g_sLastServer,
										   128);
			} else {
				NSMAN->CloseConnection();
				SCREENMAN->SystemMessage(DISCONNECTED);
				UpdateConnectStatus();
			}
			return true;
		case PO_SCOREBOARD:
			if (m_pRows[PO_SCOREBOARD]->GetOneSharedSelection() ==
				NO_SCOREBOARD_ON)
				PREFSMAN->m_bEnableScoreboard.Set(true);
			else
				PREFSMAN->m_bEnableScoreboard.Set(false);
			return true;
		/*
		case NetworkOptionRow::PO_SERVERS:
			if ( !AllServers.empty() )
			{
				string sNewName =
		AllServers[m_pRows[GetCurrentRow()]->GetOneSharedSelection()].Address;
				NSMAN->PostStartUp(sNewName);
				NSMAN->DisplayStartupStatus();
				UpdateConnectStatus( );
			}
			else
			{
				//If the server list is empty, keep passing the message on so
		exit works ScreenOptions::MenuStart( input );
			}
			return true;
		*/
		default:
			return ScreenOptions::MenuStart(input);
	}
}

void
ScreenNetworkOptions::ImportOptions(int /* iRow */,
									const PlayerNumber& /* vpns */)
{
}
void
ScreenNetworkOptions::ExportOptions(int /* iRow */,
									const PlayerNumber& /* vpns */)
{
}

void
ScreenNetworkOptions::UpdateConnectStatus()
{
	SCREENMAN->SetNewScreen(THEME->GetMetric(m_sName, "NextScreen"));
}
