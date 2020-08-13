#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/OptionRowHandler.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenMiniMenu.h"

#include <utility>
#include "Etterna/Singletons/ThemeManager.h"

void
PrepareToLoadScreen(const std::string& sScreenName);
void
FinishedLoadingScreen();

AutoScreenMessage(SM_GoToOK);
AutoScreenMessage(SM_GoToCancel);

bool ScreenMiniMenu::s_bCancelled = false;
int ScreenMiniMenu::s_iLastRowCode = -1;
vector<int> ScreenMiniMenu::s_viLastAnswers;

// Hooks for profiling
void
PrepareToLoadScreen(const std::string& sScreenName)
{
}
void
FinishedLoadingScreen()
{
}

// Settings:
namespace {
const MenuDef* g_pMenuDef = NULL;
ScreenMessage g_SendOnOK;
ScreenMessage g_SendOnCancel;
} // namespace;

void
ScreenMiniMenu::MiniMenu(const MenuDef* pDef,
						 ScreenMessage SM_SendOnOK,
						 ScreenMessage SM_SendOnCancel,
						 float fX,
						 float fY)
{
	PrepareToLoadScreen(pDef->sClassName);

	g_pMenuDef = pDef;
	g_SendOnOK = std::move(SM_SendOnOK);
	g_SendOnCancel = std::move(SM_SendOnCancel);

	SCREENMAN->AddNewScreenToTop(pDef->sClassName);
	Screen* pNewScreen = SCREENMAN->GetTopScreen();
	pNewScreen->SetXY(fX, fY);

	FinishedLoadingScreen();
}

REGISTER_SCREEN_CLASS(ScreenMiniMenu);

void
ScreenMiniMenu::Init()
{
	if (PREFSMAN->m_iArcadeOptionsNavigation)
		SetNavigation(NAV_THREE_KEY_MENU);

	ScreenOptions::Init();
}

void
ScreenMiniMenu::BeginScreen()
{
	ASSERT(g_pMenuDef != NULL);

	LoadMenu(g_pMenuDef);
	m_SMSendOnOK = g_SendOnOK;
	m_SMSendOnCancel = g_SendOnCancel;
	g_pMenuDef = NULL;

	ScreenOptions::BeginScreen();

	/* HACK: An OptionRow exits if a screen is set. ScreenMiniMenu is always
	 * pushed, so we don't set screens to load. Set a dummy screen, so
	 * ScreenOptions::GetNextScreenForSelection will know to move on. */
	m_sNextScreen = "xxx";
}

void
ScreenMiniMenu::LoadMenu(const MenuDef* pDef)
{
	m_vMenuRows = pDef->rows;

	s_viLastAnswers.resize(m_vMenuRows.size());
	// Convert from m_vMenuRows to vector<OptionRowDefinition>
	vector<OptionRowHandler*> vHands;
	for (unsigned r = 0; r < m_vMenuRows.size(); r++) {
		const MenuRowDef& mr = m_vMenuRows[r];
		OptionRowHandler* pHand = OptionRowHandlerUtil::MakeSimple(mr);
		vHands.push_back(pHand);
	}

	ScreenOptions::InitMenu(vHands);
}

void
ScreenMiniMenu::AfterChangeValueOrRow(PlayerNumber pn)
{
	ScreenOptions::AfterChangeValueOrRow(pn);

	for (unsigned i = 0; i < m_pRows.size(); i++)
		ExportOptions(i, PLAYER_1);

	// Changing one option can affect whether other options are available.
	for (unsigned i = 0; i < m_pRows.size(); i++) {
		const MenuRowDef& mr = m_vMenuRows[i];
		if (mr.pfnEnabled) {
			OptionRow& optrow = *m_pRows[i];
			optrow.GetRowDef().m_vEnabledForPlayers.clear();
			if (mr.pfnEnabled())
				optrow.GetRowDef().m_vEnabledForPlayers.insert(
				  GAMESTATE->GetMasterPlayerNumber());
		}
		m_pRows[i]->UpdateEnabledDisabled();
	}
}

void
ScreenMiniMenu::ImportOptions(int r, const PlayerNumber& vpns)
{
	OptionRow& optrow = *m_pRows[r];
	const MenuRowDef& mr = m_vMenuRows[r];
	if (!mr.choices.empty())
		optrow.SetOneSharedSelection(mr.iDefaultChoice);
}

void
ScreenMiniMenu::ExportOptions(int r, const PlayerNumber& vpns)
{
	if (r == GetCurrentRow())
		s_iLastRowCode = m_vMenuRows[r].iRowCode;
	s_viLastAnswers.resize(m_vMenuRows.size());
	s_viLastAnswers[r] = m_pRows[r]->GetOneSharedSelection(true);
}

void
ScreenMiniMenu::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_GoToNextScreen) {
		s_bCancelled = false;
		SCREENMAN->PopTopScreen(m_SMSendOnOK);
		return;
	}
	if (SM == SM_GoToPrevScreen) {
		s_bCancelled = true;
		SCREENMAN->PopTopScreen(m_SMSendOnCancel);
		return;
	}

	ScreenOptions::HandleScreenMessage(SM);
}

bool
ScreenMiniMenu::FocusedItemEndsScreen(PlayerNumber pn) const
{
	return true;
}
