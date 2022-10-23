#include "Etterna/Globals/global.h"

#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/OptionRowHandler.h"
#include "Etterna/Models/Misc/Profile.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenOptionsEditProfile.h"
#include "Etterna/Screen/Others/ScreenPrompt.h"

enum EditProfileRow
{
	ROW_NOTHING,
};

REGISTER_SCREEN_CLASS(ScreenOptionsEditProfile);

void
ScreenOptionsEditProfile::Init()
{
	ScreenOptions::Init();
}

void
ScreenOptionsEditProfile::BeginScreen()
{
	m_Original = *GAMESTATE->GetEditLocalProfile();

	std::vector<OptionRowHandler*> vHands;

	Profile* pProfile =
	  PROFILEMAN->GetLocalProfile(GAMESTATE->m_sEditLocalProfileID);
	ASSERT(pProfile != NULL);

	{
		vHands.push_back(OptionRowHandlerUtil::MakeNull());
		OptionRowDefinition& def = vHands.back()->m_Def;
		def.m_layoutType = LAYOUT_SHOW_ONE_IN_ROW;
		def.m_bOneChoiceForAllPlayers = true;
		def.m_bAllowThemeItems = false;
		def.m_bAllowThemeTitle = false;
		def.m_bAllowExplanation = false;
		def.m_bExportOnChange = true;
		def.m_sName = "nothing";
		def.m_vsChoices.clear();
		def.m_vsChoices.push_back(std::string());
	}

	InitMenu(vHands);

	ScreenOptions::BeginScreen();
}

ScreenOptionsEditProfile::~ScreenOptionsEditProfile() = default;

void
ScreenOptionsEditProfile::ImportOptions(int iRow, const PlayerNumber& vpns)
{
	Profile* pProfile =
	  PROFILEMAN->GetLocalProfile(GAMESTATE->m_sEditLocalProfileID);
	ASSERT(pProfile != NULL);
}

void
ScreenOptionsEditProfile::ExportOptions(int iRow, const PlayerNumber& vpns)
{
	Profile* pProfile =
	  PROFILEMAN->GetLocalProfile(GAMESTATE->m_sEditLocalProfileID);
	ASSERT(pProfile != NULL);
	OptionRow& row = *m_pRows[iRow];
	int iIndex = row.GetOneSharedSelection(true);
	std::string sValue;
	if (iIndex >= 0)
		sValue = row.GetRowDef().m_vsChoices[iIndex];

	switch (iRow) {
		case ROW_NOTHING:
			break;
	}
}

void
ScreenOptionsEditProfile::GoToNextScreen()
{
}

void
ScreenOptionsEditProfile::GoToPrevScreen()
{
}

void
ScreenOptionsEditProfile::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_GoToNextScreen) {
		PROFILEMAN->SaveLocalProfile(GAMESTATE->m_sEditLocalProfileID);
	} else if (SM == SM_GoToPrevScreen) {
		*GAMESTATE->GetEditLocalProfile() = m_Original;
	}

	ScreenOptions::HandleScreenMessage(SM);
}

void
ScreenOptionsEditProfile::AfterChangeValueInRow(int iRow, PlayerNumber pn)
{
	ScreenOptions::AfterChangeValueInRow(iRow, pn);

	// cause the overlay to reload
	GAMESTATE->m_sEditLocalProfileID.Set(GAMESTATE->m_sEditLocalProfileID);
}

void
ScreenOptionsEditProfile::ProcessMenuStart(const InputEventPlus& input)
{
	if (IsTransitioning())
		return;

	int iRow = GetCurrentRow();
	// OptionRow &row = *m_pRows[iRow];

	switch (iRow) {
		case ROW_NOTHING: {
		} break;
		default:
			ScreenOptions::ProcessMenuStart(input);
			break;
	}
}
