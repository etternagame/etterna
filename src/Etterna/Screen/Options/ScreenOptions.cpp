#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/GameCommand.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Models/Misc/OptionRowHandler.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenOptions.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Models/Misc/Foreach.h"

#include <algorithm>

/*
 * These navigation types are provided:
 *
 * All modes:
 *  left, right -> change option
 *  up, down -> change row
 *   (in toggle modes, focus on nearest item to old focus)
 *
 * NAV_THREE_KEY:
 *  start -> move to next row
 *  left+right+start -> move to prev row
 *  (next screen via "exit" entry)
 * This is the minimal navigation, for using menus with only three buttons.
 *
 * NAV_THREE_KEY_ALT:
 *  left, right -> next, prev row
 *  start -> change option
 *
 * NAV_FIVE_KEY:
 *  start -> next screen
 * This is a much more convenient navigation, requiring five keys.
 *
 * NAV_TOGGLE_THREE_KEY:
 *  start -> on first choice, move to next row; otherewise toggle option and
 * move to first choice left+right+start -> move to prev row
 *
 * NAV_TOGGLE_FIVE_KEY:
 *  start -> toggle option
 *
 * Regular modes and toggle modes must be enabled by the theme.  We could simply
 * automatically switch to toggle mode for multiselect rows, but that would be
 * strange for non-multiselect rows (eg. scroll speed).
 *
 * THREE_KEY modes are navigatable with only MenuLeft, MenuRight and MenuStart,
 * and are used when PREFSMAN->m_iArcadeOptionsNavigation is enabled.  However,
 * they can still use MenuUp and MenuDown for nonessential behavior.
 *
 * NAV_THREE_KEY_MENU:
 *  left, right -> change row
 *  up, down -> change row
 *  start -> next screen
 * This is a specialized navigation for ScreenOptionsService.  It must be
 * enabled to allow screens that use rows to select other screens to work with
 * only three buttons.  (It's also used when in five-key mode.)
 *
 * We don't want to simply allow left/right to move up and down on single-entry
 * rows when in NAV_THREE_KEY, becasue left and right shouldn't exit the "exit"
 * row in player options menus, but it should in the options menu.
 */

static std::string
OPTION_EXPLANATION(std::string s)
{
	return THEME->GetString("OptionExplanations", s);
}

static const char* InputModeNames[] = { "Individual", "Together" };
StringToX(InputMode);

// REGISTER_SCREEN_CLASS( ScreenOptions );	// can't be instantiated
ScreenOptions::ScreenOptions()
{
	// These can be overridden in a derived Init().
	switch (PREFSMAN->m_iArcadeOptionsNavigation) {
		case 0:
			SetNavigation(NAV_FIVE_KEY);
			break;
		case 1:
			SetNavigation(NAV_THREE_KEY);
			break;
		case 2:
			SetNavigation(NAV_THREE_KEY_ALT);
			break;
		default:
			SetNavigation(NAV_THREE_KEY);
			break;
	}
	m_InputMode = INPUTMODE_SHARE_CURSOR;
	m_iCurrentRow = 0;
	m_iFocusX = 0;
	m_bWasOnExit = false;
	m_bGotAtLeastOneStartPressed = false;
}

void
ScreenOptions::Init()
{
	NUM_ROWS_SHOWN.Load(m_sName, "NumRowsShown");
	ROW_INIT_COMMAND.Load(m_sName, "RowInitCommand");
	ROW_ON_COMMAND.Load(m_sName, "RowOnCommand");
	ROW_OFF_COMMAND.Load(m_sName, "RowOffCommand");
	SHOW_SCROLL_BAR.Load(m_sName, "ShowScrollBar");
	SCROLL_BAR_HEIGHT.Load(m_sName, "ScrollBarHeight");
	SCROLL_BAR_TIME.Load(m_sName, "ScrollBarTime");
	LINE_HIGHLIGHT_X.Load(m_sName, "LineHighlightX");
	SHOW_EXIT_ROW.Load(m_sName, "ShowExitRow");
	SEPARATE_EXIT_ROW.Load(m_sName, "SeparateExitRow");
	SEPARATE_EXIT_ROW_Y.Load(m_sName, "SeparateExitRowY");
	SHOW_EXPLANATIONS.Load(m_sName, "ShowExplanations");
	ALLOW_REPEATING_CHANGE_VALUE_INPUT.Load(m_sName,
											"AllowRepeatingChangeValueInput");
	CURSOR_TWEEN_SECONDS.Load(m_sName, "CursorTweenSeconds");
	WRAP_VALUE_IN_ROW.Load(m_sName, "WrapValueInRow");
	OPTION_ROW_NORMAL_METRICS_GROUP.Load(m_sName,
										 "OptionRowNormalMetricsGroup");
	OPTION_ROW_EXIT_METRICS_GROUP.Load(m_sName, "OptionRowExitMetricsGroup");

	m_exprRowPositionTransformFunction.SetFromReference(
	  THEME->GetMetricR(m_sName, "RowPositionTransformFunction"));

	ScreenWithMenuElements::Init();

	m_SoundChangeCol.Load(THEME->GetPathS(m_sName, "change"), true);
	m_SoundNextRow.Load(THEME->GetPathS(m_sName, "next"), true);
	m_SoundPrevRow.Load(THEME->GetPathS(m_sName, "prev"), true);
	m_SoundToggleOn.Load(THEME->GetPathS(m_sName, "toggle on"), true);
	m_SoundToggleOff.Load(THEME->GetPathS(m_sName, "toggle off"), true);
	m_SoundStart.Load(THEME->GetPathS(m_sName, "start"), true);

	// add everything to m_frameContainer so we can animate everything at once
	m_frameContainer.SetName("Container");
	LOAD_ALL_COMMANDS(m_frameContainer);
	this->AddChild(&m_frameContainer);

	m_sprPage.Load(THEME->GetPathG(m_sName, "page"));
	m_sprPage->SetName("Page");
	LOAD_ALL_COMMANDS_AND_SET_XY(m_sprPage);
	m_frameContainer.AddChild(m_sprPage);

	// init line highlights
	m_sprLineHighlight.Load(
	  THEME->GetPathG(m_sName, ssprintf("LineHighlight P%d", PLAYER_1 + 1)));
	m_sprLineHighlight->SetName(ssprintf("LineHighlightP%d", PLAYER_1 + 1));
	m_sprLineHighlight->SetX(LINE_HIGHLIGHT_X);
	LOAD_ALL_COMMANDS(m_sprLineHighlight);
	m_frameContainer.AddChild(m_sprLineHighlight);

	// init cursors
	m_Cursor.Load("OptionsCursor" + PlayerNumberToString(PLAYER_1), true);
	m_Cursor.SetName("Cursor");
	LOAD_ALL_COMMANDS(m_Cursor);
	m_frameContainer.AddChild(&m_Cursor);

	switch (m_InputMode) {
		case INPUTMODE_INDIVIDUAL:
			m_textExplanation.LoadFromFont(
			  THEME->GetPathF(m_sName, "explanation"));
			m_textExplanation.SetDrawOrder(2);
			m_textExplanation.SetName("Explanation" +
									  PlayerNumberToString(PLAYER_1));
			LOAD_ALL_COMMANDS_AND_SET_XY(m_textExplanation);
			m_frameContainer.AddChild(&m_textExplanation);
			break;
		case INPUTMODE_SHARE_CURSOR:
			m_textExplanationTogether.LoadFromFont(
			  THEME->GetPathF(m_sName, "explanation"));
			m_textExplanationTogether.SetDrawOrder(2);
			m_textExplanationTogether.SetName("ExplanationTogether");
			LOAD_ALL_COMMANDS_AND_SET_XY(m_textExplanationTogether);
			m_frameContainer.AddChild(&m_textExplanationTogether);
			break;
		default:
			FAIL_M(ssprintf("Invalid InputMode: %i", m_InputMode));
	}

	if (SHOW_SCROLL_BAR) {
		m_ScrollBar.SetName("ScrollBar");
		m_ScrollBar.SetBarHeight(SCROLL_BAR_HEIGHT);
		m_ScrollBar.SetBarTime(SCROLL_BAR_TIME);
		m_ScrollBar.Load("DualScrollBar");
		m_ScrollBar.EnablePlayer(PLAYER_1, GAMESTATE->IsHumanPlayer(PLAYER_1));
		LOAD_ALL_COMMANDS_AND_SET_XY(m_ScrollBar);
		m_ScrollBar.SetDrawOrder(2);
		m_frameContainer.AddChild(&m_ScrollBar);
	}

	m_sprMore.Load(THEME->GetPathG(m_sName, "more"));
	m_sprMore->SetName("More");
	LOAD_ALL_COMMANDS_AND_SET_XY(m_sprMore);
	m_sprMore->SetDrawOrder(2);
	m_sprMore->PlayCommand("LoseFocus");
	m_frameContainer.AddChild(m_sprMore);

	m_OptionRowTypeNormal.Load(OPTION_ROW_NORMAL_METRICS_GROUP, this);
	m_OptionRowTypeExit.Load(OPTION_ROW_EXIT_METRICS_GROUP, this);
}

void
ScreenOptions::InitMenu(const vector<OptionRowHandler*>& vHands)
{
	Locator::getLogger()->trace("ScreenOptions::InitMenu()");

	for (unsigned i = 0; i < m_pRows.size(); i++) {
		m_frameContainer.RemoveChild(m_pRows[i]);
		SAFE_DELETE(m_pRows[i]);
	}
	m_pRows.clear();

	for (unsigned r = 0; r < vHands.size(); r++) // foreach row
	{
		m_pRows.push_back(new OptionRow(&m_OptionRowTypeNormal));
		OptionRow& row = *m_pRows.back();
		row.SetDrawOrder(1);
		m_frameContainer.AddChild(&row);

		bool bFirstRowGoesDown = m_OptionsNavigation == NAV_TOGGLE_THREE_KEY;

		row.LoadNormal(vHands[r], bFirstRowGoesDown);
	}

	if (SHOW_EXIT_ROW) {
		m_pRows.push_back(new OptionRow(&m_OptionRowTypeExit));
		OptionRow& row = *m_pRows.back();
		row.LoadExit();
		row.SetDrawOrder(1);
		m_frameContainer.AddChild(&row);
	}

	m_frameContainer.SortByDrawOrder();

	FOREACH(OptionRow*, m_pRows, p)
	{
		int iIndex = p - m_pRows.begin();

		Lua* L = LUA->Get();
		LuaHelpers::Push(L, iIndex);
		(*p)->m_pLuaInstance->Set(L, "iIndex");
		LUA->Release(L);

		(*p)->RunCommands(ROW_INIT_COMMAND);
	}

	// poke once at all the explanation metrics so that we catch missing ones
	// early
	for (int r = 0; r < static_cast<int>(m_pRows.size()); r++) // foreach row
	{
		GetExplanationText(r);
	}
}

/* Call when option rows have been re-initialized. */
void
ScreenOptions::RestartOptions()
{
	m_exprRowPositionTransformFunction.ClearCache();

	for (unsigned r = 0; r < m_pRows.size(); r++) // foreach row
	{
		OptionRow* pRow = m_pRows[r];
		pRow->Reload();

		this->ImportOptions(r, PLAYER_1);

		// HACK: Process disabled players, too, to hide inactive underlines.
		pRow->AfterImportOptions(PLAYER_1);
	}

	m_iCurrentRow = -1;
	m_iFocusX = -1;
	m_bWasOnExit = false;

	// put focus on the first enabled row
	for (unsigned r = 0; r < m_pRows.size(); r++) {
		const OptionRow& row = *m_pRows[r];
		if (row.GetRowDef().IsEnabledForPlayer(PLAYER_1)) {
			m_iCurrentRow = r;
			break;
		}
	}

	// Hide the highlight if no rows are enabled.
	m_sprLineHighlight->SetVisible(m_iCurrentRow != -1 &&
								   GAMESTATE->IsHumanPlayer(PLAYER_1));

	Locator::getLogger()->trace("About to get the rows positioned right.");

	PositionRows(false);
	for (unsigned r = 0; r < m_pRows.size(); ++r) {
		this->RefreshIcons(r, PLAYER_1);
	}
	PositionCursor(PLAYER_1);

	AfterChangeRow(PLAYER_1);
	Locator::getLogger()->trace("Rows positioned.");
}

void
ScreenOptions::BeginScreen()
{
	ScreenWithMenuElements::BeginScreen();

	RestartOptions();

	m_bGotAtLeastOneStartPressed = false;

	ON_COMMAND(m_frameContainer);

	m_Cursor.SetVisible(GAMESTATE->IsHumanPlayer(PLAYER_1));
	ON_COMMAND(m_Cursor);

	this->SortByDrawOrder();
}

void
ScreenOptions::TweenOnScreen()
{
	ScreenWithMenuElements::TweenOnScreen();

	for (auto& p : m_pRows) {
		p->RunCommands(ROW_ON_COMMAND);
	}

	m_frameContainer.SortByDrawOrder();
}

void
ScreenOptions::TweenOffScreen()
{
	ScreenWithMenuElements::TweenOffScreen();

	for (auto& p : m_pRows) {
		p->RunCommands(ROW_OFF_COMMAND);
	}
}

ScreenOptions::~ScreenOptions()
{
	Locator::getLogger()->trace("ScreenOptions::~ScreenOptions()");
	for (unsigned i = 0; i < m_pRows.size(); i++)
		SAFE_DELETE(m_pRows[i]);
	MESSAGEMAN->Broadcast("OptionsScreenClosed");
}

std::string
ScreenOptions::GetExplanationText(int iRow) const
{
	const OptionRow& row = *m_pRows[iRow];

	bool bAllowExplanation = row.GetRowDef().m_bAllowExplanation;
	bool bShowExplanations = bAllowExplanation && SHOW_EXPLANATIONS.GetValue();
	if (!bShowExplanations)
		return std::string();

	std::string sExplanationName = row.GetRowDef().m_sExplanationName;
	if (sExplanationName.empty())
		sExplanationName = row.GetRowDef().m_sName;
	ASSERT(!sExplanationName.empty());

	return OPTION_EXPLANATION(sExplanationName);
}

void
ScreenOptions::GetWidthXY(PlayerNumber pn,
						  int iRow,
						  int iChoiceOnRow,
						  int& iWidthOut,
						  int& iXOut,
						  int& iYOut) const
{
	ASSERT_M(iRow < static_cast<int>(m_pRows.size()),
			 ssprintf("%i < %i", iRow, (int)m_pRows.size()));
	const OptionRow& row = *m_pRows[iRow];
	row.GetWidthXY(pn, iChoiceOnRow, iWidthOut, iXOut, iYOut);
}

void
ScreenOptions::RefreshIcons(int iRow, PlayerNumber pn)
{
	OptionRow& row = *m_pRows[iRow];

	const OptionRowDefinition& def = row.GetRowDef();

	// find first selection and whether multiple are selected
	int iFirstSelection = row.GetOneSelection(pn, true);

	// set icon name and bullet
	std::string sIcon;
	GameCommand gc;

	if (iFirstSelection == -1) {
		sIcon = "Multi";
	} else if (iFirstSelection != -1) {
		const OptionRowHandler* pHand = row.GetHandler();
		if (pHand != nullptr) {
			int iSelection =
			  iFirstSelection +
			  (m_OptionsNavigation == NAV_TOGGLE_THREE_KEY ? -1 : 0);
			pHand->GetIconTextAndGameCommand(iSelection, sIcon, gc);
		}
	}

	// XXX: hack to not display text in the song options menu
	if (def.m_bOneChoiceForAllPlayers)
		sIcon = "";

	m_pRows[iRow]->SetModIcon(pn, sIcon, gc);
}

void
ScreenOptions::PositionCursor(PlayerNumber pn)
{
	// Set the position of the cursor showing the current option the user is
	// changing.
	const int iRow = m_iCurrentRow;
	if (iRow == -1)
		return;

	ASSERT_M(iRow >= 0 && iRow < static_cast<int>(m_pRows.size()),
			 ssprintf("%i < %i", iRow, (int)m_pRows.size()));
	const OptionRow& row = *m_pRows[iRow];

	const int iChoiceWithFocus = row.GetChoiceInRowWithFocus();
	if (iChoiceWithFocus == -1)
		return;

	int iWidth, iX, iY;
	GetWidthXY(pn, iRow, iChoiceWithFocus, iWidth, iX, iY);

	OptionsCursor& cursor = m_Cursor;
	cursor.SetBarWidth(iWidth);
	cursor.SetXY(static_cast<float>(iX), static_cast<float>(iY));
	bool bCanGoLeft = iChoiceWithFocus > 0;
	bool bCanGoRight =
	  iChoiceWithFocus >= 0 &&
	  iChoiceWithFocus <
		static_cast<int>(row.GetRowDef().m_vsChoices.size()) - 1;
	cursor.SetCanGo(bCanGoLeft, bCanGoRight);
}

void
ScreenOptions::TweenCursor(PlayerNumber pn)
{
	// Set the position of the cursor showing the current option the user is
	// changing.
	const int iRow = m_iCurrentRow;
	ASSERT_M(iRow >= 0 && iRow < static_cast<int>(m_pRows.size()),
			 ssprintf("%i < %i", iRow, (int)m_pRows.size()));

	const OptionRow& row = *m_pRows[iRow];
	const int iChoiceWithFocus = row.GetChoiceInRowWithFocus();

	if (iChoiceWithFocus == -1) {
		Locator::getLogger()->warn("Tried to tween cursor on row with no choices.");
		return;
	}

	int iWidth, iX, iY;
	GetWidthXY(pn, iRow, iChoiceWithFocus, iWidth, iX, iY);

	OptionsCursor& cursor = m_Cursor;
	if (cursor.GetDestX() != static_cast<float>(iX) ||
		cursor.GetDestY() != static_cast<float>(iY) ||
		cursor.GetBarWidth() != iWidth) {
		cursor.StopTweening();
		cursor.BeginTweening(CURSOR_TWEEN_SECONDS);
		cursor.SetXY(static_cast<float>(iX), static_cast<float>(iY));
		cursor.SetBarWidth(iWidth);
	}

	bool bCanGoLeft = false;
	bool bCanGoRight = false;
	switch (row.GetRowDef().m_layoutType) {
		case LAYOUT_SHOW_ONE_IN_ROW:
			bCanGoLeft = iChoiceWithFocus > 0;
			bCanGoRight =
			  iChoiceWithFocus >= 0 &&
			  iChoiceWithFocus <
				static_cast<int>(row.GetRowDef().m_vsChoices.size()) - 1;
			break;
		case LAYOUT_SHOW_ALL_IN_ROW:
			break;
		default:
			break;
	}
	cursor.SetCanGo(bCanGoLeft, bCanGoRight);

	if (GAMESTATE->IsHumanPlayer(pn)) {
		COMMAND(m_sprLineHighlight[pn], "Change");
		if (row.GetRowType() == OptionRow::RowType_Exit)
			COMMAND(m_sprLineHighlight[pn], "ChangeToExit");

		m_sprLineHighlight->SetY(static_cast<float>(iY));
	}
}

void
ScreenOptions::Update(float fDeltaTime)
{
	// LOG->Trace( "ScreenOptions::Update(%f)", fDeltaTime );

	ScreenWithMenuElements::Update(fDeltaTime);
}

bool
ScreenOptions::Input(const InputEventPlus& input)
{
	// HACK: This screen eats mouse inputs if we don't check for them first.
	bool mouse_evt = false;
	for (int i = MOUSE_LEFT; i <= MOUSE_WHEELDOWN; i++) {
		if (input.DeviceI ==
			DeviceInput(DEVICE_MOUSE, static_cast<DeviceButton>(i)))
			mouse_evt = true;
	}
	if (mouse_evt) {
		return ScreenWithMenuElements::Input(input);
	}

	/* Allow input when transitioning in (m_In.IsTransitioning()), but ignore it
	 * when we're transitioning out. */
	if (m_Cancel.IsTransitioning() || m_Out.IsTransitioning() ||
		m_fLockInputSecs > 0)
		return false;

	if (!GAMESTATE->IsHumanPlayer(input.pn))
		return false;

	if (input.type == IET_RELEASE) {
		switch (input.MenuI) {
			case GAME_BUTTON_START:
			case GAME_BUTTON_SELECT:
			case GAME_BUTTON_MENURIGHT:
			case GAME_BUTTON_MENULEFT:
				INPUTMAPPER->ResetKeyRepeat(GAME_BUTTON_START, input.pn);
				INPUTMAPPER->ResetKeyRepeat(GAME_BUTTON_RIGHT, input.pn);
				INPUTMAPPER->ResetKeyRepeat(GAME_BUTTON_LEFT, input.pn);
			default:
				break;
		}
	}

	// default input handler
	return Screen::Input(input);
}

void
ScreenOptions::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_MenuTimer) {
		this->BeginFadingOut();
	} else if (SM == SM_BeginFadingOut) {
		if (IsTransitioning())
			return; // already transitioning

		// If the selected option sets a screen, honor it.
		std::string sThisScreen =
		  GetNextScreenForFocusedItem(GAMESTATE->GetMasterPlayerNumber());
		if (sThisScreen != "")
			m_sNextScreen = sThisScreen;

		// If options set a NextScreen or one is specified in metrics, then fade
		// out
		if (GetNextScreenName() == "") {
			LuaHelpers::ReportScriptErrorFmt("%s::HandleScreenMessage: Tried "
											 "to fade out, but we have no next "
											 "screen",
											 m_sName.c_str());
			return;
		}

		StartTransitioningScreen(SM_ExportOptions);
	} else if (SM == SM_ExportOptions) {
		for (unsigned r = 0; r < m_pRows.size(); r++) // foreach row
		{
			if (m_pRows[r]->GetRowType() == OptionRow::RowType_Exit)
				continue;
			this->ExportOptions(r, PLAYER_1);
		}

		this->HandleScreenMessage(SM_GoToNextScreen);
	}

	ScreenWithMenuElements::HandleScreenMessage(SM);
}

void
ScreenOptions::PositionRows(bool bTween)
{
	const int total = NUM_ROWS_SHOWN;
	const int halfsize = total / 2;

	int first_start, first_end, second_start, second_end;

	// Choices for the player.
	int P1Choice = m_iCurrentRow;

	vector<OptionRow*> Rows(m_pRows);
	OptionRow* pSeparateExitRow = nullptr;

	if (static_cast<bool>(SEPARATE_EXIT_ROW) && !Rows.empty() &&
		Rows.back()->GetRowType() == OptionRow::RowType_Exit) {
		pSeparateExitRow = Rows.back();

		// Remove the exit row for purposes of positioning everything else.
		if (P1Choice == static_cast<int>(Rows.size()) - 1)
			--P1Choice;
		/*if( P2Choice == (int) Rows.size()-1 )
			--P2Choice;
			*/
		Rows.pop_back();
	}

	const bool BothPlayersActivated = GAMESTATE->IsHumanPlayer(PLAYER_1);
	if (m_InputMode == INPUTMODE_SHARE_CURSOR || !BothPlayersActivated) {
		// Simply center the cursor.
		first_start = std::max(P1Choice - halfsize, 0);
		first_end = first_start + total;
		second_start = second_end = first_end;
	} else {
		// First half:
		const int earliest = P1Choice;
		first_start = std::max(earliest - halfsize / 2, 0);
		first_end = first_start + halfsize;

		// Second half:
		const int latest = P1Choice;

		second_start = std::max(latest - halfsize / 2, 0);

		// Don't overlap.
		second_start = std::max(second_start, first_end);

		second_end = second_start + halfsize;
	}

	first_end = std::min(first_end, static_cast<int>(Rows.size()));
	second_end = std::min(second_end, static_cast<int>(Rows.size()));

	/* If less than total (and Rows.size()) are displayed, fill in the empty
	 * space intelligently. */
	for (;;) {
		const int sum = (first_end - first_start) + (second_end - second_start);
		if (sum >= static_cast<int>(Rows.size()) || sum >= total) {
			break; // nothing more to display, or no room
		}
		/* First priority: expand the top of the second half until it meets
		 * the first half. */
		if (second_start > first_end) {
			second_start--;
		}
		// Otherwise, expand either end.
		else if (first_start > 0) {
			first_start--;
		} else if (second_end < static_cast<int>(Rows.size())) {
			second_end++;
		} else {
			FAIL_M("Do we have room to grow or don't we?");
		}
	}

	int pos = 0;
	for (int i = 0; i < static_cast<int>(Rows.size()); i++) // foreach row
	{
		OptionRow& row = *Rows[i];

		float fPos = static_cast<float>(pos);

		if (i < first_start)
			fPos = -0.5f;
		else if (i >= first_end && i < second_start)
			fPos =
			  (static_cast<int>(static_cast<int>(NUM_ROWS_SHOWN) / 2)) - 0.5f;
		else if (i >= second_end)
			fPos = static_cast<int>(NUM_ROWS_SHOWN) - 0.5f;

		Actor::TweenState tsDestination =
		  m_exprRowPositionTransformFunction.GetTransformCached(
			fPos,
			i,
			std::min(static_cast<int>(Rows.size()),
					 static_cast<int>(NUM_ROWS_SHOWN)));

		bool bHidden = i < first_start ||
					   (i >= first_end && i < second_start) || i >= second_end;
		for (int j = 0; j < NUM_DIFFUSE_COLORS; j++)
			tsDestination.diffuse[j].a = bHidden ? 0.0f : 1.0f;
		if (!bHidden)
			pos++;
		row.SetDestination(tsDestination, bTween);
	}

	if (pSeparateExitRow != nullptr) {
		Actor::TweenState tsDestination;
		tsDestination.Init();
		tsDestination.pos.y = SEPARATE_EXIT_ROW_Y;

		for (auto& j : tsDestination.diffuse)
			j.a = 1.0f;
		pSeparateExitRow->SetDestination(tsDestination, bTween);
	}
}

void
ScreenOptions::AfterChangeValueOrRow(PlayerNumber pn)
{
	if (!GAMESTATE->IsHumanPlayer(pn))
		return;

	const int iCurRow = m_iCurrentRow;

	if (iCurRow == -1)
		return;

	// Update m_fY and m_bHidden[].
	PositionRows(true);

	// Do positioning.
	RefreshIcons(iCurRow, pn);
	for (unsigned r = 0; r < m_pRows.size(); r++) {
		/* After changing a value, position underlines. Do this for both
		 * players, since underlines for both players will change with
		 * m_bOneChoiceForAllPlayers. */
		m_pRows[r]->PositionUnderlines(PLAYER_1);
		m_pRows[r]->PositionIcons(pn);
		m_pRows[r]->SetRowHasFocus(
		  pn, GAMESTATE->IsHumanPlayer(pn) && iCurRow == static_cast<int>(r));
		m_pRows[r]->UpdateEnabledDisabled();
	}

	if (SHOW_SCROLL_BAR) {
		float fPercent = 0;
		if (m_pRows.size() > 1)
			fPercent = iCurRow / static_cast<float>(m_pRows.size() - 1);
		m_ScrollBar.SetPercentage(pn, fPercent);
	}

	// Update all players, since changing one player can move both cursors.
	TweenCursor(PLAYER_1);
	OptionRow& row = *m_pRows[iCurRow];
	const bool bExitSelected = row.GetRowType() == OptionRow::RowType_Exit;
	if (GAMESTATE->GetNumHumanPlayers() != 1 && PLAYER_1 != pn)
		return;
	if (m_bWasOnExit != bExitSelected) {
		m_bWasOnExit = bExitSelected;
		COMMAND(m_sprMore,
				ssprintf("Exit%sP%i",
						 bExitSelected ? "Selected" : "Unselected",
						 PLAYER_1 + 1));
		m_sprMore->PlayCommand(bExitSelected ? "GainFocus" : "LoseFocus");
	}

	const std::string text = GetExplanationText(iCurRow);
	BitmapText* pText = nullptr;
	switch (m_InputMode) {
		case INPUTMODE_INDIVIDUAL:
			pText = &m_textExplanation;
			break;
		default:
			// case INPUTMODE_SHARE_CURSOR:
			pText = &m_textExplanationTogether;
			break;
	}
	if (pText->GetText() != text) {
		pText->FinishTweening();
		ON_COMMAND(pText);
		pText->SetText(text);
	}
}

bool
ScreenOptions::MenuBack(const InputEventPlus&)
{
	Cancel(SM_GoToPrevScreen);
	return true;
}

bool
ScreenOptions::AllAreOnLastRow() const
{
	if (m_iCurrentRow != static_cast<int>(m_pRows.size() - 1))
		return false;
	return true;
}

bool
ScreenOptions::MenuStart(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;
	switch (input.type) {
		case IET_FIRST_PRESS:
			m_bGotAtLeastOneStartPressed = true;
			break;
		case IET_RELEASE:
			return false; // ignore
		default:		  // repeat type
			if (!m_bGotAtLeastOneStartPressed)
				return false; // don't allow repeat
			break;
	}

	/* If we are in a three-button mode, check to see if GAME_BUTTON_LEFT and
	 * GAME_BUTTON_RIGHT are being held. */
	switch (m_OptionsNavigation) {
		case NAV_THREE_KEY:
		case NAV_TOGGLE_THREE_KEY: {
			bool bHoldingLeftAndRight =
			  INPUTMAPPER->IsBeingPressed(GAME_BUTTON_RIGHT, pn) &&
			  INPUTMAPPER->IsBeingPressed(GAME_BUTTON_LEFT, pn);
			if (bHoldingLeftAndRight) {
				if (MoveRowRelative(pn, -1, input.type != IET_FIRST_PRESS))
					m_SoundPrevRow.Play(true);
				return true;
			}
		}
		default:
			break;
	}

	this->ProcessMenuStart(input);
	return true;
}

void
ScreenOptions::ProcessMenuStart(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;

	int iCurRow = m_iCurrentRow;

	if (iCurRow < 0) {
		// this shouldn't be happening, but it is, so we need to bail out. -aj
		m_SoundStart.PlayCopy(true);
		this->BeginFadingOut();
		return;
	}

	OptionRow& row = *m_pRows[iCurRow];

	if (m_OptionsNavigation == NAV_THREE_KEY_MENU &&
		row.GetRowType() != OptionRow::RowType_Exit) {
		/* In NAV_THREE_KEY_MENU mode, if a row doesn't set a screen, it does
		 * something.  Apply it now, and don't go to the next screen. */
		if (!FocusedItemEndsScreen(input.pn)) {
			ExportOptions(iCurRow, input.pn);
			return;
		}
	}

	// Check whether Start ends this screen.
	{
		bool bEndThisScreen = false;

		// If we didn't apply and return above in NAV_THREE_KEY_MENU, then the
		// selection sets a screen.
		if (m_OptionsNavigation == NAV_THREE_KEY_MENU)
			bEndThisScreen = true;

		// If there's no exit row, then pressing Start on any row ends the
		// screen.
		if (!SHOW_EXIT_ROW)
			bEndThisScreen = true;

		// If all players are on "Exit"
		if (AllAreOnLastRow())
			bEndThisScreen = true;

		// Don't accept START to go to the next screen if we're still
		// transitioning in.
		if (bEndThisScreen &&
			(input.type != IET_FIRST_PRESS || IsTransitioning()))
			return;

		if (bEndThisScreen) {
			m_SoundStart.PlayCopy(true);
			this->BeginFadingOut();
			return;
		}
	}

	if (row.GetFirstItemGoesDown()) {
		int iChoiceInRow = row.GetChoiceInRowWithFocus();
		if (iChoiceInRow == 0 || iChoiceInRow == -1) {
			MenuDown(input);
			return;
		}
	}

	if (row.GetRowDef().m_selectType == SELECT_MULTIPLE) {
		int iChoiceInRow = row.GetChoiceInRowWithFocus();
		if (iChoiceInRow == -1) {
			Locator::getLogger()->warn("MenuStart used on SelectMultiple OptionRow with no choices.");
			return;
		}
		bool bSelected = !row.GetSelected(iChoiceInRow);
		bool changed = row.SetSelected(pn, iChoiceInRow, bSelected);
		if (changed) {
			AfterChangeValueOrRow(pn);
		}

		if (bSelected)
			m_SoundToggleOn.Play(true);
		else
			m_SoundToggleOff.Play(true);

		m_pRows[iCurRow]->PositionUnderlines(pn);
		RefreshIcons(iCurRow, pn);

		Message msg("SelectMultiple");
		msg.SetParam("PlayerNumber", pn);
		msg.SetParam("RowIndex", iCurRow);
		msg.SetParam("ChoiceInRow", iChoiceInRow);
		msg.SetParam("Selected", bSelected);
		MESSAGEMAN->Broadcast(msg);

		if (row.GetFirstItemGoesDown() && row.GoToFirstOnStart()) {
			// move to the first choice in the row
			ChangeValueInRowRelative(m_iCurrentRow,
									 pn,
									 -row.GetChoiceInRowWithFocus(),
									 input.type != IET_FIRST_PRESS);
		}
	} else // data.selectType != SELECT_MULTIPLE
	{
		switch (m_OptionsNavigation) {
			case NAV_THREE_KEY:
				// don't wrap
				if (iCurRow == static_cast<int>(m_pRows.size()) - 1)
					return;
				MenuDown(input);
				break;
			case NAV_THREE_KEY_ALT:
				ChangeValueInRowRelative(
				  m_iCurrentRow, input.pn, +1, input.type != IET_FIRST_PRESS);
				break;

			case NAV_TOGGLE_THREE_KEY:
			case NAV_TOGGLE_FIVE_KEY: {
				int iChoiceInRow = row.GetChoiceInRowWithFocus();
				if (iChoiceInRow == -1) {
					Locator::getLogger()->warn("MenuStart used on other SelectType OptionRow with no choices.");
					return;
				}
				if (row.GetRowDef().m_bOneChoiceForAllPlayers)
					row.SetOneSharedSelection(iChoiceInRow);
				else
					row.SetOneSelection(pn, iChoiceInRow);

				if (row.GetFirstItemGoesDown())
					ChangeValueInRowRelative(
					  m_iCurrentRow,
					  pn,
					  -row.GetChoiceInRowWithFocus(),
					  input.type !=
						IET_FIRST_PRESS); // move to the first choice
				else
					ChangeValueInRowRelative(
					  m_iCurrentRow, pn, 0, input.type != IET_FIRST_PRESS);
				break;
			}
			case NAV_THREE_KEY_MENU:
				FAIL_M("NAV_THREE_KEY_MENU should be unreachable");
			case NAV_FIVE_KEY:
				/* Jump to the exit row.  (If everyone's already on the exit
				 * row, then we'll have already gone to the next screen above.)
				 */
				if (MoveRowAbsolute(pn, m_pRows.size() - 1))
					m_SoundNextRow.Play(true);

				break;
		}
	}
}

void
ScreenOptions::StoreFocus(PlayerNumber pn)
{
	// Long rows always put us in the center, so don't update the focus.
	int iCurrentRow = m_iCurrentRow;
	const OptionRow& row = *m_pRows[iCurrentRow];
	if (row.GetRowDef().m_layoutType == LAYOUT_SHOW_ONE_IN_ROW)
		return;

	int iWidth, iY;
	int iChoiceOnRow = row.GetChoiceInRowWithFocus();
	if (iChoiceOnRow == -1) {
		Locator::getLogger()->warn("No choices found when setting focus.");
	} else {
		GetWidthXY(pn, m_iCurrentRow, iChoiceOnRow, iWidth, m_iFocusX, iY);
		Locator::getLogger()->trace("cur selection {}x{} @ {}",
				   m_iCurrentRow,
				   row.GetChoiceInRowWithFocus(),
				   m_iFocusX);
	}
}

bool
ScreenOptions::FocusedItemEndsScreen(PlayerNumber pn) const
{
	std::string sScreen = GetNextScreenForFocusedItem(pn);
	return !sScreen.empty();
}

std::string
ScreenOptions::GetNextScreenForFocusedItem(PlayerNumber pn) const
{
	int iCurRow = this->GetCurrentRow(pn);

	if (iCurRow == -1)
		return std::string();

	ASSERT(iCurRow >= 0 && iCurRow < static_cast<int>(m_pRows.size()));
	const OptionRow* pRow = m_pRows[iCurRow];

	int iChoice = pRow->GetChoiceInRowWithFocus();
	if (pRow->GetFirstItemGoesDown())
		iChoice--;

	// not the "goes down" item
	if (iChoice == -1)
		return std::string();

	const OptionRowHandler* pHand = pRow->GetHandler();
	if (pHand == nullptr)
		return std::string();
	return pHand->GetScreen(iChoice);
}

void
ScreenOptions::BeginFadingOut()
{
	this->PostScreenMessage(SM_BeginFadingOut, 0);
}

// Left/right
void
ScreenOptions::ChangeValueInRowAbsolute(int iRow,
										PlayerNumber pn,
										int iChoiceIndex,
										bool bRepeat)
{
	if (iRow == -1) // no row selected
		return;		// don't allow a move

	OptionRow& row = *m_pRows[iRow];

	const int iNumChoices = row.GetRowDef().m_vsChoices.size();
	ASSERT(iNumChoices >= 0 && iChoiceIndex < iNumChoices);

	int iCurrentChoiceWithFocus = row.GetChoiceInRowWithFocus();
	int iDelta = iChoiceIndex - iCurrentChoiceWithFocus;

	Message msg("ChangeValue");
	msg.SetParam("PlayerNumber", pn);
	msg.SetParam("RowIndex", iRow);
	MESSAGEMAN->Broadcast(msg);

	ChangeValueInRowRelative(iRow, pn, iDelta, bRepeat);
}

void
ScreenOptions::ChangeValueInRowRelative(int iRow,
										PlayerNumber pn,
										int iDelta,
										bool bRepeat)
{
	if (iRow == -1) // no row selected
		return;		// don't allow a move

	OptionRow& row = *m_pRows[iRow];

	const int iNumChoices = row.GetRowDef().m_vsChoices.size();

	if (m_OptionsNavigation == NAV_THREE_KEY_MENU && iNumChoices <= 1) // 1 or 0
	{
		/* There are no other options on the row; move up or down instead of
		 * left and right. This allows navigating the options menu with
		 * left/right/start.
		 * XXX: Only allow repeats if the opposite key isn't pressed; otherwise,
		 * holding both directions will repeat in place continuously, which is
		 * weird. */
		if (MoveRowRelative(pn, iDelta, bRepeat)) {
			if (iDelta < 0)
				m_SoundPrevRow.Play(true);
			else
				m_SoundNextRow.Play(true);
		}
		return;
	}

	if (iNumChoices <= 1) // nowhere to move
		return;

	if (bRepeat && !ALLOW_REPEATING_CHANGE_VALUE_INPUT)
		return;

	bool bOneChanged = false;

	int iCurrentChoiceWithFocus = row.GetChoiceInRowWithFocus();
	int iNewChoiceWithFocus = iCurrentChoiceWithFocus + iDelta;
	if (!bRepeat && WRAP_VALUE_IN_ROW.GetValue())
		wrap(iNewChoiceWithFocus, iNumChoices);
	else
		CLAMP(iNewChoiceWithFocus, 0, iNumChoices - 1);

	if (iCurrentChoiceWithFocus != iNewChoiceWithFocus)
		bOneChanged = true;

	row.SetChoiceInRowWithFocus(pn, iNewChoiceWithFocus);
	StoreFocus(pn);

	if (row.GetRowDef().m_bOneChoiceForAllPlayers) {
		/* If this row is bOneChoiceForAllPlayers, then lock the cursors
		 * together for this row. Don't do this in toggle modes, since the
		 * current selection and the current focus are detached. */
		bool bForceFocusedChoiceTogether = false;
		if (m_OptionsNavigation != NAV_TOGGLE_THREE_KEY &&
			m_OptionsNavigation != NAV_TOGGLE_FIVE_KEY &&
			row.GetRowDef().m_bOneChoiceForAllPlayers) {
			bForceFocusedChoiceTogether = true;
		}

		// Also lock focus if the screen is explicitly set to share cursors.
		if (m_InputMode == INPUTMODE_SHARE_CURSOR)
			bForceFocusedChoiceTogether = true;

		if (bForceFocusedChoiceTogether) {
			// lock focus together
			row.SetChoiceInRowWithFocus(PLAYER_1, iNewChoiceWithFocus);
			StoreFocus(PLAYER_1);
		}
	}

	if (!row.GetRowDef().m_bOneChoiceForAllPlayers && PLAYER_1 != pn) {
	} else {
		if (m_OptionsNavigation == NAV_TOGGLE_THREE_KEY ||
			m_OptionsNavigation == NAV_TOGGLE_FIVE_KEY) {
			; // do nothing
		} else {
			if (row.GetRowDef().m_selectType == SELECT_MULTIPLE)
				; // do nothing. User must press Start to toggle the selection.
			else
				row.SetOneSelection(PLAYER_1, iNewChoiceWithFocus);
		}
	}

	if (bOneChanged)
		m_SoundChangeCol.Play(true);

	if (row.GetRowDef().m_bExportOnChange) {
		ExportOptions(iRow, PLAYER_1);
	}

	this->AfterChangeValueInRow(iRow, pn);
}

void
ScreenOptions::AfterChangeValueInRow(int iRow, PlayerNumber pn)
{
	AfterChangeValueOrRow(pn);
}

// Move up/down. Returns true if we actually moved.
bool
ScreenOptions::MoveRowRelative(PlayerNumber pn, int iDir, bool bRepeat)
{
	// LOG->Trace( "MoveRowRelative(pn %i, dir %i, rep %i)", pn, iDir, bRepeat
	// );

	int iDest = -1;
	ASSERT(m_pRows.size() != 0);
	for (int r = 1; r < static_cast<int>(m_pRows.size()); r++) {
		int iDelta = r * iDir;
		iDest = m_iCurrentRow + iDelta;
		wrap(iDest, m_pRows.size());

		OptionRow& row = *m_pRows[iDest];
		if (row.GetRowDef().IsEnabledForPlayer(pn))
			break;

		iDest = -1;
	}

	if (iDest == -1)
		return false;
	if (bRepeat) {
		// Don't wrap on repeating inputs.
		if (iDir > 0 && iDest < m_iCurrentRow)
			return false;
		if (iDir < 0 && iDest > m_iCurrentRow)
			return false;
	}

	return MoveRowAbsolute(pn, iDest);
}

void
ScreenOptions::AfterChangeRow(PlayerNumber pn)
{
	const int iRow = m_iCurrentRow;
	if (iRow != -1) {
		// In FIVE_KEY, keep the selection in the row near the focus.
		OptionRow& row = *m_pRows[iRow];
		switch (m_OptionsNavigation) {
			case NAV_TOGGLE_FIVE_KEY: {
				if (row.GetRowDef().m_layoutType != LAYOUT_SHOW_ONE_IN_ROW) {
					int iSelectionDist = -1;
					for (unsigned i = 0; i < row.GetTextItemsSize(); ++i) {
						int iWidth, iX, iY;
						GetWidthXY(pn, m_iCurrentRow, i, iWidth, iX, iY);
						const int iDist = abs(iX - m_iFocusX);
						if (iSelectionDist == -1 || iDist < iSelectionDist) {
							iSelectionDist = iDist;
							row.SetChoiceInRowWithFocus(pn, i);
						}
					}
				}
				break;
			}
			default:
				break;
		}

		if (row.GetFirstItemGoesDown()) {
			// If moving to a bFirstChoiceGoesDown row, put focus back on
			// the first choice.
			row.SetChoiceInRowWithFocus(pn, 0);
		}
	}

	AfterChangeValueOrRow(pn);
}

bool
ScreenOptions::MoveRowAbsolute(PlayerNumber pn, int iRow)
{
	bool bChanged = false;
	if (m_InputMode == INPUTMODE_INDIVIDUAL && PLAYER_1 != pn) {
	} // skip
	else if (m_iCurrentRow == iRow) {
		// also skip
	} else {

		m_iCurrentRow = iRow;

		AfterChangeRow(PLAYER_1);
		bChanged = true;

		const OptionRow& row = *m_pRows[iRow];
		Message msg("ChangeRow");
		msg.SetParam("PlayerNumber", PLAYER_1);
		msg.SetParam("RowIndex", GetCurrentRow(PLAYER_1));
		msg.SetParam("ChangedToExit",
					 row.GetRowType() == OptionRow::RowType_Exit);
		MESSAGEMAN->Broadcast(msg);
	}

	return bChanged;
}

bool
ScreenOptions::MenuLeft(const InputEventPlus& input)
{
	if (m_OptionsNavigation == NAV_THREE_KEY_ALT)
		MenuUpDown(input, -1);
	else
		ChangeValueInRowRelative(
		  m_iCurrentRow, input.pn, -1, input.type != IET_FIRST_PRESS);

	PlayerNumber pn = input.pn;
	MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuLeftP1 + pn));
	return true;
}

bool
ScreenOptions::MenuRight(const InputEventPlus& input)
{
	if (m_OptionsNavigation == NAV_THREE_KEY_ALT)
		MenuUpDown(input, +1);
	else
		ChangeValueInRowRelative(
		  m_iCurrentRow, input.pn, +1, input.type != IET_FIRST_PRESS);

	PlayerNumber pn = input.pn;
	MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuRightP1 + pn));
	return true;
}

bool
ScreenOptions::MenuUp(const InputEventPlus& input)
{
	MenuUpDown(input, -1);
	PlayerNumber pn = input.pn;
	MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuUpP1 + pn));
	return true;
}

bool
ScreenOptions::MenuDown(const InputEventPlus& input)
{
	MenuUpDown(input, +1);
	PlayerNumber pn = input.pn;
	MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuDownP1 + pn));
	return true;
}

bool
ScreenOptions::MenuSelect(const InputEventPlus& input)
{
	MenuUpDown(input, -1);
	return true;
}

void
ScreenOptions::MenuUpDown(const InputEventPlus& input, int iDir)
{
	ASSERT(iDir == -1 || iDir == +1);
	PlayerNumber pn = input.pn;

	if (input.type == IET_REPEAT) {
		/* If down is pressed, don't allow up to repeat, and vice versa. This
		 * prevents holding both up and down from toggling repeatedly in-place.
		 */
		if (iDir == +1) {
			if (INPUTMAPPER->IsBeingPressed(GAME_BUTTON_MENUUP, pn) ||
				INPUTMAPPER->IsBeingPressed(GAME_BUTTON_SELECT, pn))
				return;
		} else {
			if (INPUTMAPPER->IsBeingPressed(GAME_BUTTON_MENUDOWN, pn) ||
				INPUTMAPPER->IsBeingPressed(GAME_BUTTON_START, pn))
				return;
		}
	}

	if (MoveRowRelative(pn, iDir, input.type != IET_FIRST_PRESS)) {
		if (iDir < 0)
			m_SoundPrevRow.Play(true);
		else
			m_SoundNextRow.Play(true);
	}
}

/*
void ScreenOptions::SetOptionRowFromName( const std::string& nombre )
	{
		for( unsigned i=0; i<m_pRows.size(); i++ )
		{
			if( m_pRows[i]->GetRowTitle() == nombre) &&
m_pRows[i]->GetRowDef().IsEnabledForPlayer(PLAYER_1) )
MoveRowAbsolute(PLAYER_1,i)
		}
	}
*/
// lua start

/** @brief Allow Lua to have access to ScreenOptions. */
class LunaScreenOptions : public Luna<ScreenOptions>
{
  public:
	static int AllAreOnLastRow(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->AllAreOnLastRow());
		return 1;
	}
	static int FocusedItemEndsScreen(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->FocusedItemEndsScreen(PLAYER_1));
		return 1;
	}
	static int GetCurrentRowIndex(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetCurrentRow(PLAYER_1));
		return 1;
	}
	static int GetOptionRow(T* p, lua_State* L)
	{
		int row_index = IArg(1);
		// TODO:  Change row indices to be 1-indexed when breaking compatibility
		// is allowed. -Kyz
		if (row_index < 0 || row_index >= p->GetNumRows()) {
			luaL_error(L, "Row index %d is invalid.", row_index);
		}
		OptionRow* pOptRow = p->GetRow(row_index);
		if (pOptRow != nullptr)
			pOptRow->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	DEFINE_METHOD(GetNumRows, GetNumRows());
	// static int SetOptionRowFromName( T* p, lua_State *L ) {
	// p->SetOptionRowFromName( SArg(1) ); return 0; }

	LunaScreenOptions()
	{
		ADD_METHOD(AllAreOnLastRow);
		ADD_METHOD(FocusedItemEndsScreen);
		ADD_METHOD(GetCurrentRowIndex);
		ADD_METHOD(GetOptionRow);
		ADD_METHOD(GetNumRows);
		// ADD_METHOD( SetOptionRowFromName );
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenOptions, ScreenWithMenuElements)
// lua end
