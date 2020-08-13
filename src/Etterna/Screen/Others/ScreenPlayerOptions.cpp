#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/AnnouncerManager.h"
#include "Etterna/Models/Misc/CodeDetector.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenPlayerOptions.h"
#include "ScreenSongOptions.h"
#include "Etterna/Singletons/ThemeManager.h"

REGISTER_SCREEN_CLASS(ScreenPlayerOptions);

void
ScreenPlayerOptions::Init()
{
	ScreenOptionsMaster::Init();
	m_sprDisqualify.Load(THEME->GetPathG(m_sName, "disqualify"));
	m_sprDisqualify->SetName(ssprintf("DisqualifyP%i", PLAYER_1 + 1));
	LOAD_ALL_COMMANDS_AND_SET_XY(m_sprDisqualify);
	m_sprDisqualify->SetVisible(
	  false); // unhide later if handicapping options are discovered
	m_sprDisqualify->SetDrawOrder(2);
	m_frameContainer.AddChild(m_sprDisqualify);

	m_bAskOptionsMessage = PREFSMAN->m_ShowSongOptions == Maybe_ASK;

	m_bAcceptedChoices = false;
	m_bGoToOptions = (PREFSMAN->m_ShowSongOptions == Maybe_YES);

	SOUND->PlayOnceFromDir(ANNOUNCER->GetPathTo("player options intro"));

	m_bRowCausesDisqualified.resize(m_pRows.size(), false);
}

void
ScreenPlayerOptions::BeginScreen()
{
	ON_COMMAND(m_sprDisqualify);

	ScreenOptionsMaster::BeginScreen();

	for (unsigned r = 0; r < m_pRows.size(); r++)
		UpdateDisqualified(r, PLAYER_1);
}

bool
ScreenPlayerOptions::Input(const InputEventPlus& input)
{
	bool bHandled = false;

	if (m_bAskOptionsMessage && input.type == IET_FIRST_PRESS &&
		!m_In.IsTransitioning() && input.MenuI == GAME_BUTTON_START) {
		if (m_bAcceptedChoices && !m_bGoToOptions) {
			m_bGoToOptions = true;
			this->PlayCommand("GoToOptions");
			SCREENMAN->PlayStartSound();
			bHandled = true;
		}
	}

	PlayerNumber pn = input.pn;
	if (GAMESTATE->IsHumanPlayer(pn) &&
		CodeDetector::EnteredCode(input.GameI.controller,
								  CODE_CANCEL_ALL_PLAYER_OPTIONS)) {
		// apply the game default mods, but not the Profile saved mods
		GAMESTATE->m_pPlayerState->ResetToDefaultPlayerOptions(
		  ModsLevel_Preferred);

		MESSAGEMAN->Broadcast(ssprintf("CancelAllP%i", pn + 1));

		for (unsigned r = 0; r < m_pRows.size(); r++) {
			int iOldFocus = m_pRows[r]->GetChoiceInRowWithFocus();
			this->ImportOptions(r, pn);
			m_pRows[r]->AfterImportOptions(pn);
			this->UpdateDisqualified(r, pn);
			m_pRows[r]->SetChoiceInRowWithFocus(pn, iOldFocus);
		}
		bHandled = true;
	}

	bHandled = ScreenOptionsMaster::Input(input) || bHandled;

	// UGLY: Update m_Disqualified whenever Start is pressed
	if (GAMESTATE->IsHumanPlayer(pn) && input.MenuI == GAME_BUTTON_START) {
		int row = m_iCurrentRow;
		UpdateDisqualified(row, pn);
	}
	return bHandled;
}

void
ScreenPlayerOptions::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_BeginFadingOut &&
		m_bAskOptionsMessage) // user accepts the page of options
	{
		m_bAcceptedChoices = true;

		// show "hold START for options"
		this->PlayCommand("AskForGoToOptions");
	}

	ScreenOptionsMaster::HandleScreenMessage(SM);
}

void
ScreenPlayerOptions::UpdateDisqualified(int row, PlayerNumber pn)
{
	ASSERT(GAMESTATE->IsHumanPlayer(pn));

	// save original player options
	PlayerOptions poOrig =
	  GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred();

	// Find out if the current row when exported causes disqualification.
	// Exporting the row will fill GAMESTATE->m_PlayerOptions.
	PO_GROUP_CALL(
	  GAMESTATE->m_pPlayerState->m_PlayerOptions, ModsLevel_Preferred, Init);

	ExportOptions(row, pn);
	bool bRowCausesDisqualified = GAMESTATE->CurrentOptionsDisqualifyPlayer(pn);
	m_bRowCausesDisqualified[row] = bRowCausesDisqualified;

	// Update disqualified graphic
	bool bDisqualified = false;
	FOREACH_CONST(bool, m_bRowCausesDisqualified, b)
	{
		if (*b) {
			bDisqualified = true;
			break;
		}
	}
	m_sprDisqualify->SetVisible(bDisqualified);

	// restore previous player options in case the user escapes back after this
	GAMESTATE->m_pPlayerState->m_PlayerOptions.Assign(ModsLevel_Preferred,
													  poOrig);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ScreenPlayerOptions. */
class LunaScreenPlayerOptions : public Luna<ScreenPlayerOptions>
{
  public:
	static int GetGoToOptions(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->GetGoToOptions());
		return 1;
	}

	LunaScreenPlayerOptions() { ADD_METHOD(GetGoToOptions); }
};

LUA_REGISTER_DERIVED_CLASS(ScreenPlayerOptions, ScreenOptions)
// lua end
