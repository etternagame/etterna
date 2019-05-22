#include "Etterna/Globals/global.h"

#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Globals/GameLoop.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/OptionRowHandler.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenOptionsMaster.h"
#include "ScreenOptionsMasterPrefs.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Globals/StepMania.h"
#include "Etterna/Singletons/ThemeManager.h"

#define LINE_NAMES THEME->GetMetric(m_sName, "LineNames")
#define LINE(sLineName)                                                        \
	THEME->GetMetric(m_sName, ssprintf("Line%s", (sLineName).c_str()))
#define FORCE_ALL_PLAYERS THEME->GetMetricB(m_sName, "ForceAllPlayers")
#define INPUT_MODE THEME->GetMetric(m_sName, "InputMode")
#define NAVIGATION_MODE THEME->GetMetric(m_sName, "NavigationMode")

REGISTER_SCREEN_CLASS(ScreenOptionsMaster);

void
ScreenOptionsMaster::Init()
{
	std::vector<RString> asLineNames;
	split(LINE_NAMES, ",", asLineNames);
	if (asLineNames.empty()) {
		LuaHelpers::ReportScriptErrorFmt("\"%s:LineNames\" is empty.",
										 m_sName.c_str());
	}

	if (FORCE_ALL_PLAYERS) {
		GAMESTATE->JoinPlayer(PLAYER_1);
	}

	if (NAVIGATION_MODE == "toggle")
		SetNavigation(PREFSMAN->m_iArcadeOptionsNavigation
						? NAV_TOGGLE_THREE_KEY
						: NAV_TOGGLE_FIVE_KEY);
	else if (NAVIGATION_MODE == "menu")
		SetNavigation(NAV_THREE_KEY_MENU);

	SetInputMode(StringToInputMode(INPUT_MODE));

	// Call this after enabling players, if any.
	ScreenOptions::Init();

	std::vector<OptionRowHandler*> OptionRowHandlers;
	for (unsigned i = 0; i < asLineNames.size(); ++i) {
		RString sLineName = asLineNames[i];
		RString sRowCommands = LINE(sLineName);

		Commands cmds;
		ParseCommands(sRowCommands, cmds, false);

		OptionRowHandler* pHand = OptionRowHandlerUtil::Make(cmds);
		if (pHand == NULL) {
			LuaHelpers::ReportScriptErrorFmt(
			  "Invalid OptionRowHandler \"%s\" in \"%s:Line:%s\".",
			  cmds.GetOriginalCommandString().c_str(),
			  m_sName.c_str(),
			  sLineName.c_str());
		} else {
			OptionRowHandlers.push_back(pHand);
		}
	}
	InitMenu(OptionRowHandlers);
}

void
ScreenOptionsMaster::ImportOptions(int r, const PlayerNumber& vpns)
{
	ASSERT(GAMESTATE->IsHumanPlayer(vpns));
	OptionRow& row = *m_pRows[r];
	row.ImportOptions(vpns);
}

void
ScreenOptionsMaster::ExportOptions(int r, const PlayerNumber& vpns)
{
	CHECKPOINT_M(ssprintf("%i/%i", r, static_cast<int>(m_pRows.size())));

	OptionRow& row = *m_pRows[r];
	bool bRowHasFocus = false;
	int iCurRow = m_iCurrentRow;
	bRowHasFocus = iCurRow == r;
	m_iChangeMask |= row.ExportOptions(vpns, bRowHasFocus);
}

void
ScreenOptionsMaster::HandleScreenMessage(const ScreenMessage SM)
{
	if (SM == SM_ExportOptions) {
		// Override ScreenOptions's calling of ExportOptions
		m_iChangeMask = 0;

		CHECKPOINT_M("Starting the export handling.");

		for (unsigned r = 0; r < m_pRows.size(); r++) // foreach row
			ExportOptions(r, PLAYER_1);

		if ((m_iChangeMask & OPT_APPLY_ASPECT_RATIO) != 0) {
			THEME->UpdateLuaGlobals(); // This needs to be done before resetting
									   // the projection matrix below
			THEME->ReloadSubscribers(); // SCREEN_* has changed, so re-read all
										// subscribing ThemeMetrics
			SCREENMAN
			  ->ThemeChanged(); // recreate ScreenSystemLayer and SharedBGA
		}

		/* If the theme changes, we need to reset RageDisplay to apply the new
		 * window title and icon. If the aspect ratio changes, we need to reset
		 * RageDisplay so that the projection matrix is re-created using the new
		 * screen dimensions. */
		if (((m_iChangeMask & OPT_APPLY_THEME) != 0) ||
			((m_iChangeMask & OPT_APPLY_GRAPHICS) != 0) ||
			((m_iChangeMask & OPT_APPLY_ASPECT_RATIO) != 0)) {
			/* If the resolution or aspect ratio changes, always reload the
			 * theme. Otherwise, only reload it if it changed. */
			RString sNewTheme = PREFSMAN->m_sTheme.Get();
			GameLoop::ChangeTheme(sNewTheme);
		}

		if ((m_iChangeMask & OPT_SAVE_PREFERENCES) != 0) {
			// Save preferences.
			LOG->Trace("ROW_CONFIG used; saving ...");
			PREFSMAN->SavePrefsToDisk();
		}

		if ((m_iChangeMask & OPT_CHANGE_GAME) != 0) {
			GameLoop::ChangeGame(PREFSMAN->GetCurrentGame());
		}

		if ((m_iChangeMask & OPT_APPLY_SOUND) != 0) {
			SOUNDMAN->SetMixVolume();
		}

		CHECKPOINT_M("Transferring to the next screen now.");
		this->HandleScreenMessage(SM_GoToNextScreen);
		return;
	}

	ScreenOptions::HandleScreenMessage(SM);
}

/*
 * (c) 2003-2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
