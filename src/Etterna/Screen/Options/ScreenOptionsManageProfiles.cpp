#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Misc/OptionRowHandler.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Screen/Others/ScreenMiniMenu.h"
#include "ScreenOptionsManageProfiles.h"
#include "Etterna/Screen/Others/ScreenPrompt.h"
#include "Etterna/Screen/Others/ScreenTextEntry.h"

static LocalizedString NEW_PROFILE_DEFAULT_NAME("ScreenOptionsManageProfiles",
												"NewProfileDefaultName");

#define SHOW_CREATE_NEW (!PROFILEMAN->FixedProfiles())

AutoScreenMessage(SM_BackFromEnterNameForNew);
AutoScreenMessage(SM_BackFromRename);
AutoScreenMessage(SM_BackFromClearConfirm);
AutoScreenMessage(SM_BackFromContextMenu);

enum ProfileAction
{
	ProfileAction_SetDefaultP1,
	ProfileAction_Edit,
	ProfileAction_Rename,
	ProfileAction_Clear,
	ProfileAction_MoveUp,
	ProfileAction_MoveDown,
	NUM_ProfileAction
};
static const char* ProfileActionNames[] = {
	"SetDefaultP1", "Edit", "Rename", "Clear", "MoveUp", "MoveDown",
};
XToString(ProfileAction);
XToLocalizedString(ProfileAction);
/** @brief Loop through each ProfileAction. */
#define FOREACH_ProfileAction(i) FOREACH_ENUM(ProfileAction, i)

static MenuDef g_TempMenu("ScreenMiniMenuContext");

static LocalizedString PROFILE_NAME_BLANK("ScreenEditMenu",
										  "Profile name cannot be blank.");
static LocalizedString PROFILE_NAME_CONFLICTS("ScreenEditMenu",
											  "The name you chose conflicts "
											  "with another profile. Please "
											  "use a different name.");
static bool
ValidateLocalProfileName(const std::string& sAnswer, std::string& sErrorOut)
{
	if (sAnswer == "") {
		sErrorOut = PROFILE_NAME_BLANK;
		return false;
	}

	auto pProfile =
	  PROFILEMAN->GetLocalProfile(GAMESTATE->m_sEditLocalProfileID);
	if (pProfile != nullptr && sAnswer == pProfile->m_sDisplayName)
		return true; // unchanged

	vector<std::string> vsProfileNames;
	PROFILEMAN->GetLocalProfileDisplayNames(vsProfileNames);
	auto bAlreadyAProfileWithThisName =
	  find(vsProfileNames.begin(), vsProfileNames.end(), sAnswer) !=
	  vsProfileNames.end();
	if (bAlreadyAProfileWithThisName) {
		sErrorOut = PROFILE_NAME_CONFLICTS;
		return false;
	}

	return true;
}

REGISTER_SCREEN_CLASS(ScreenOptionsManageProfiles);

void
ScreenOptionsManageProfiles::Init()
{
	ScreenOptions::Init();

	SetNavigation(NAV_THREE_KEY_MENU);
	SetInputMode(INPUTMODE_SHARE_CURSOR);
}

void
ScreenOptionsManageProfiles::BeginScreen()
{
	// FIXME
	// int iIndex = 0;
	vector<OptionRowHandler*> OptionRowHandlers;

	if (SHOW_CREATE_NEW) {
		auto pHand = OptionRowHandlerUtil::Make(ParseCommands(
		  ssprintf("gamecommand;screen,%s;name,dummy", m_sName.c_str())));
		auto& def = pHand->m_Def;
		def.m_layoutType = LAYOUT_SHOW_ALL_IN_ROW;
		def.m_bAllowThemeTitle = true;
		def.m_bAllowThemeItems = false;
		def.m_sName = "Create New Profile";
		def.m_sExplanationName = "Create New Profile";
		OptionRowHandlers.push_back(pHand);

		// FIXME
		// gc.Load( iIndex++,  );
	}

	PROFILEMAN->GetLocalProfileIDs(m_vsLocalProfileID);

	for (auto& s : m_vsLocalProfileID) {
		auto pProfile = PROFILEMAN->GetLocalProfile(s);
		ASSERT(pProfile != nullptr);

		auto sCommand =
		  ssprintf("gamecommand;screen,ScreenOptionsCustomizeProfile;profileid,"
				   "%s;name,dummy",
				   s.c_str());
		auto pHand = OptionRowHandlerUtil::Make(ParseCommands(sCommand));
		auto& def = pHand->m_Def;
		def.m_layoutType = LAYOUT_SHOW_ALL_IN_ROW;
		def.m_bAllowThemeTitle = false;
		def.m_bAllowThemeItems = false;
		def.m_sName = pProfile->m_sDisplayName;
		def.m_sExplanationName = "Select Profile";

		auto pn = PLAYER_INVALID;
		if (s == ProfileManager::m_sDefaultLocalProfileID[PLAYER_1].Get())
			pn = PLAYER_1;
		if (pn != PLAYER_INVALID)
			def.m_vsChoices.push_back(PlayerNumberToLocalizedString(pn));
		OptionRowHandlers.push_back(pHand);

		// FIXME
		// gc.Load( iIndex++,  );
	}

	ScreenOptions::InitMenu(OptionRowHandlers);

	// Save sEditLocalProfileID before calling ScreenOptions::BeginScreen,
	// because it will get clobbered.
	std::string sEditLocalProfileID = GAMESTATE->m_sEditLocalProfileID;

	ScreenOptions::BeginScreen();

	// select the last chosen profile
	if (!sEditLocalProfileID.empty()) {
		vector<std::string>::const_iterator iter =
		  find(m_vsLocalProfileID.begin(),
			   m_vsLocalProfileID.end(),
			   sEditLocalProfileID);
		if (iter != m_vsLocalProfileID.end()) {
			int iIndex = iter - m_vsLocalProfileID.begin();
			this->MoveRowAbsolute(PLAYER_1, 1 + iIndex);
		}
	} else if (!m_vsLocalProfileID.empty()) {
		// select the first item below "create new"
		this->MoveRowAbsolute(PLAYER_1, 1);
	}

	AfterChangeRow(PLAYER_1);
}

static LocalizedString CONFIRM_DELETE_PROFILE(
  "ScreenOptionsManageProfiles",
  "Are you sure you want to delete the profile '%s'?");
static LocalizedString CONFIRM_CLEAR_PROFILE(
  "ScreenOptionsManageProfiles",
  "Are you sure you want to clear all data in the profile '%s'?");
static LocalizedString ENTER_PROFILE_NAME("ScreenOptionsManageProfiles",
										  "Enter a name for the profile.");
void
ScreenOptionsManageProfiles::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_GoToNextScreen) {
		auto iCurRow = m_iCurrentRow;
		auto& row = *m_pRows[iCurRow];
		if (row.GetRowType() == OptionRow::RowType_Exit) {
			this->HandleScreenMessage(SM_GoToPrevScreen);
			return; // don't call base
		}
	} else if (SM == SM_BackFromEnterNameForNew) {
		if (!ScreenTextEntry::s_bCancelledLast) {
			ASSERT(ScreenTextEntry::s_sLastAnswer !=
				   ""); // validate should have assured this

			std::string sNewName = ScreenTextEntry::s_sLastAnswer;
			ASSERT(GAMESTATE->m_sEditLocalProfileID.Get().empty());

			auto iNumProfiles = PROFILEMAN->GetNumLocalProfiles();

			// create
			std::string sProfileID;

			// is this the correct way to go about checking the return value?
			// -aj
			bool bCreateProfile = PROFILEMAN->CreateLocalProfile(
			  ScreenTextEntry::s_sLastAnswer, sProfileID);
			ASSERT(bCreateProfile);

			GAMESTATE->m_sEditLocalProfileID.Set(sProfileID);

			if (iNumProfiles < NUM_PLAYERS) {
				auto iFirstUnused = -1;
				FOREACH_CONST(Preference<std::string>*,
							  PROFILEMAN->m_sDefaultLocalProfileID.m_v,
							  i)
				{
					auto sLocalProfileID = (*i)->Get();
					if (sLocalProfileID.empty()) {
						iFirstUnused =
						  i - PROFILEMAN->m_sDefaultLocalProfileID.m_v.begin();
						break;
					}
				}
				if (iFirstUnused != -1) {
					PROFILEMAN->m_sDefaultLocalProfileID.m_v[iFirstUnused]->Set(
					  sProfileID);
				}
			}

			SCREENMAN->SetNewScreen(this->m_sName); // reload
		}
	} else if (SM == SM_BackFromRename) {
		if (!ScreenTextEntry::s_bCancelledLast) {
			ASSERT(ScreenTextEntry::s_sLastAnswer !=
				   ""); // validate should have assured this

			std::string sNewName = ScreenTextEntry::s_sLastAnswer;
			PROFILEMAN->RenameLocalProfile(GAMESTATE->m_sEditLocalProfileID,
										   sNewName);

			SCREENMAN->SetNewScreen(this->m_sName); // reload
		}
	} else if (SM == SM_BackFromClearConfirm) {
		if (ScreenPrompt::s_LastAnswer == ANSWER_YES) {

			SCREENMAN->SetNewScreen(this->m_sName); // reload
		}
	} else if (SM == SM_BackFromContextMenu) {
		if (!ScreenMiniMenu::s_bCancelled) {
			auto pProfile =
			  PROFILEMAN->GetLocalProfile(GAMESTATE->m_sEditLocalProfileID);
			ASSERT(pProfile != NULL);

			switch (ScreenMiniMenu::s_iLastRowCode) {
				default:
					FAIL_M(
					  ssprintf("Last row code not a valid ProfileAction: %i",
							   ScreenMiniMenu::s_iLastRowCode));
				case ProfileAction_SetDefaultP1:
				case ProfileAction_Edit: {
					GAMESTATE->m_sEditLocalProfileID.Set(
					  GetLocalProfileIDWithFocus());

					ScreenOptions::BeginFadingOut();
				} break;
				case ProfileAction_Rename: {
					ScreenTextEntry::TextEntry(SM_BackFromRename,
											   ENTER_PROFILE_NAME,
											   pProfile->m_sDisplayName,
											   PROFILE_MAX_DISPLAY_NAME_LENGTH,
											   ValidateLocalProfileName);
				} break;
				case ProfileAction_Clear: {
					auto sTitle = pProfile->m_sDisplayName;
					std::string sMessage = ssprintf(
					  CONFIRM_CLEAR_PROFILE.GetValue(), sTitle.c_str());
					ScreenPrompt::Prompt(
					  SM_BackFromClearConfirm, sMessage, PROMPT_YES_NO);
				} break;
				case ProfileAction_MoveUp:
					PROFILEMAN->MoveProfilePriority(
					  GetLocalProfileIndexWithFocus(), true);
					SCREENMAN->SetNewScreen(this->m_sName); // reload
					break;
				case ProfileAction_MoveDown:
					PROFILEMAN->MoveProfilePriority(
					  GetLocalProfileIndexWithFocus(), false);
					SCREENMAN->SetNewScreen(this->m_sName); // reload
					break;
			}
		}
	} else if (SM == SM_LoseFocus) {
		this->PlayCommand("ScreenLoseFocus");
	} else if (SM == SM_GainFocus) {
		this->PlayCommand("ScreenGainFocus");
	}

	ScreenOptions::HandleScreenMessage(SM);
}

void
ScreenOptionsManageProfiles::AfterChangeRow(PlayerNumber pn)
{
	GAMESTATE->m_sEditLocalProfileID.Set(GetLocalProfileIDWithFocus());

	ScreenOptions::AfterChangeRow(pn);
}

void
ScreenOptionsManageProfiles::ProcessMenuStart(const InputEventPlus&)
{
	if (IsTransitioning())
		return;

	auto iCurRow = m_iCurrentRow;
	auto& row = *m_pRows[iCurRow];

	if (SHOW_CREATE_NEW && iCurRow == 0) // "create new"
	{
		vector<std::string> vsUsedNames;
		PROFILEMAN->GetLocalProfileDisplayNames(vsUsedNames);

		std::string sPotentialName;
		for (auto i = 1; i < 1000; i++) {
			sPotentialName = ssprintf(
			  "%s%04d", NEW_PROFILE_DEFAULT_NAME.GetValue().c_str(), i);
			auto bNameIsUsed =
			  find(vsUsedNames.begin(), vsUsedNames.end(), sPotentialName) !=
			  vsUsedNames.end();
			if (!bNameIsUsed)
				break;
		}
		ScreenTextEntry::TextEntry(SM_BackFromEnterNameForNew,
								   ENTER_PROFILE_NAME,
								   sPotentialName,
								   PROFILE_MAX_DISPLAY_NAME_LENGTH,
								   ValidateLocalProfileName);
	} else if (row.GetRowType() == OptionRow::RowType_Exit) {
		SCREENMAN->PlayStartSound();
		this->BeginFadingOut();
	} else // a profile
	{
		g_TempMenu.rows.clear();
#define ADD_ACTION(i)                                                          \
	g_TempMenu.rows.push_back(MenuRowDef(                                      \
	  i, ProfileActionToLocalizedString(i), true, false, false, 0, ""));

		ADD_ACTION(ProfileAction_SetDefaultP1);
		if (PROFILEMAN->FixedProfiles()) {
			ADD_ACTION(ProfileAction_Rename);
			ADD_ACTION(ProfileAction_Clear);
		} else {
			ADD_ACTION(ProfileAction_Edit);
			ADD_ACTION(ProfileAction_Rename);
			ADD_ACTION(ProfileAction_MoveUp);
			ADD_ACTION(ProfileAction_MoveDown);
		}

		int iWidth, iX, iY;
		this->GetWidthXY(PLAYER_1, iCurRow, 0, iWidth, iX, iY);

		ScreenMiniMenu::MiniMenu(&g_TempMenu,
								 SM_BackFromContextMenu,
								 SM_BackFromContextMenu,
								 static_cast<float>(iX),
								 SCREEN_CENTER_Y);
	}
}

void
ScreenOptionsManageProfiles::ImportOptions(int /* iRow */,
										   const PlayerNumber& /* vpns */)
{
}

void
ScreenOptionsManageProfiles::ExportOptions(int /* iRow */,
										   const PlayerNumber& /* vpns */)
{
}

int
ScreenOptionsManageProfiles::GetLocalProfileIndexWithFocus() const
{
	auto iCurRow = m_iCurrentRow;
	auto& row = *m_pRows[iCurRow];

	if (SHOW_CREATE_NEW && iCurRow == 0) // "create new"
		return -1;
	if (row.GetRowType() == OptionRow::RowType_Exit)
		return -1;

	// a profile
	auto iIndex = iCurRow + (SHOW_CREATE_NEW ? -1 : 0);
	return iIndex;
}

std::string
ScreenOptionsManageProfiles::GetLocalProfileIDWithFocus() const
{
	auto iIndex = GetLocalProfileIndexWithFocus();
	if (iIndex == -1)
		return std::string();
	return m_vsLocalProfileID[iIndex];
}
