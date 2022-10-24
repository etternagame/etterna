#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "ScreenSongOptions.h"
#include "Etterna/Singletons/ThemeManager.h"

REGISTER_SCREEN_CLASS(ScreenSongOptions);

void
ScreenSongOptions::Init()
{
	ScreenOptionsMaster::Init();

	/* Hack: If we're coming in from "press start for more options", we need a
	 * different fade in. */
	if (PREFSMAN->m_ShowSongOptions == Maybe_ASK) {
		m_In.Load(THEME->GetPathB("ScreenSongOptions", "option in"));
		m_In.StartTransitioning();
	}
}

void
ScreenSongOptions::ExportOptions(int iRow, const PlayerNumber& vpns)
{
	PlayerState* pPS = GAMESTATE->m_pPlayerState;
	const FailType ft = pPS->m_PlayerOptions.GetPreferred().m_FailType;

	ScreenOptionsMaster::ExportOptions(iRow, vpns);

	if (ft != pPS->m_PlayerOptions.GetPreferred().m_FailType) {
		GAMESTATE->m_bFailTypeWasExplicitlySet = true;
	}
}
