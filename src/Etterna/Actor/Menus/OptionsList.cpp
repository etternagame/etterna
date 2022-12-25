#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/InputMapper.h"
#include "OptionsList.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Models/Songs/SongUtil.h"

#include <algorithm>
#include <map>

#define LINE(sLineName)                                                        \
	THEME->GetMetric(m_sName, ssprintf("Line%s", (sLineName).c_str()))
#define MAX_ITEMS_BEFORE_SPLIT THEME->GetMetricI(m_sName, "MaxItemsBeforeSplit")
#define ITEMS_SPLIT_WIDTH THEME->GetMetricF(m_sName, "ItemsSplitWidth")
#define DIRECT_LINES THEME->GetMetric(m_sName, "DirectLines")
#define TOP_MENUS THEME->GetMetric(m_sName, "TopMenus")

static const std::string RESET_ROW = "ResetOptions";

void
OptionListRow::Load(OptionsList* pOptions, const std::string& sType)
{
	m_pOptions = pOptions;
	ITEMS_SPACING_Y.Load(sType, "ItemsSpacingY");

	m_Text.resize(1);
	m_Text[0].SetName("Text");
	m_Text[0].LoadFromFont(THEME->GetPathF(sType, "normal"));
	ActorUtil::LoadAllCommands(m_Text[0], sType);

	m_Underlines.resize(1);
	m_Underlines[0].Load(THEME->GetPathG(sType, "underline"));
	m_Underlines[0]->SetName("Underline");
	ActorUtil::LoadAllCommands(*m_Underlines[0], sType);

	m_Text[0].PlayCommand("On");
	m_Underlines[0]->PlayCommand("On");
}

// GetTitleForHandler
// can always use the title
// can have special speed row titles, note skin, or any SELECT_ONE

void
OptionListRow::SetFromHandler(const OptionRowHandler* pHandler)
{
	this->FinishTweening();
	this->RemoveAllChildren();

	if (pHandler == nullptr)
		return;

	const int iNum =
	  std::max(pHandler->m_Def.m_vsChoices.size(), m_Text.size()) + 1;
	m_Text.resize(iNum, m_Text[0]);
	m_Underlines.resize(iNum, m_Underlines[0]);

	for (unsigned i = 0; i < pHandler->m_Def.m_vsChoices.size(); ++i) {
		// init underlines
		this->AddChild(m_Underlines[i]);

		// init text
		this->AddChild(&m_Text[i]);
	}

	SetTextFromHandler(pHandler);

	const unsigned iCnt = pHandler->m_Def.m_vsChoices.size();
	m_bItemsInTwoRows = static_cast<int>(iCnt) > MAX_ITEMS_BEFORE_SPLIT;
	const float fWidth = ITEMS_SPLIT_WIDTH;
	float fY = 0;
	for (unsigned i = 0; i < iCnt; ++i) {
		float fX = 0;
		if (m_bItemsInTwoRows) {
			if ((i % 2) == 0)
				fX = -fWidth / 2;
			else
				fX = +fWidth / 2;
		}

		// set the Y position of each item in the line
		m_Text[i].SetXY(fX, fY);
		m_Underlines[i]->SetXY(fX, fY);

		if (m_bItemsInTwoRows)
			m_Underlines[i]->PlayCommand("SetTwoRows");
		else
			m_Underlines[i]->PlayCommand("SetOneRow");

		if (!m_bItemsInTwoRows || (i % 2) == 1 || i + 1 == iCnt)
			fY += ITEMS_SPACING_Y;
	}

	const int iExit = pHandler->m_Def.m_vsChoices.size();
	m_Text[iExit].SetText("Exit"); // XXX localize
	m_Text[iExit].SetXY(0, fY);
	this->AddChild(&m_Text[iExit]);
}

void
OptionListRow::SetTextFromHandler(const OptionRowHandler* pHandler)
{
	ASSERT(pHandler != NULL);
	for (unsigned i = 0; i < pHandler->m_Def.m_vsChoices.size(); ++i) {
		// init text
		std::string sText = pHandler->GetThemedItemText(i);

		std::string sDest = pHandler->GetScreen(i);
		if (m_pOptions->m_setDirectRows.find(sDest) !=
			  m_pOptions->m_setDirectRows.end() &&
			!sDest.empty()) {
			const OptionRowHandler* pTarget = m_pOptions->m_Rows[sDest];
			if (pTarget->m_Def.m_selectType == SELECT_ONE) {
				const int iSelection = m_pOptions->GetOneSelection(sDest);
				sText += ": " + pTarget->GetThemedItemText(iSelection);
			}
		}

		m_Text[i].SetText(sText);
	}
}

void
OptionListRow::SetUnderlines(const std::vector<bool>& aSelections,
							 const OptionRowHandler* pHandler)
{
	for (unsigned i = 0; i < aSelections.size(); ++i) {
		Actor* pActor = m_Underlines[i];

		bool bSelected = aSelections[i];
		std::string sDest = pHandler->GetScreen(i);
		if (!sDest.empty()) {
			/* This is a submenu.  Underline the row if its options have been
			 * changed from the default. */
			const OptionRowHandler* pTarget = m_pOptions->m_Rows[sDest];
			if (pTarget->m_Def.m_selectType == SELECT_ONE) {
				const int iSelection = m_pOptions->GetOneSelection(sDest);
				const OptionRowHandler* lHandler =
				  m_pOptions->m_Rows.find(sDest)->second;
				const int iDefault = lHandler->GetDefaultOption();
				if (iDefault != -1 && iSelection != iDefault)
					bSelected = true;
			} else if (pTarget->m_Def.m_selectType == SELECT_MULTIPLE) {
				const std::vector<bool>& bTargetSelections =
				  m_pOptions->m_bSelections.find(sDest)->second;
				for (bool bTargetSelection : bTargetSelections) {
					if (bTargetSelection)
						bSelected = true;
				}
			}
		}

		pActor->PlayCommand(bSelected ? "Show" : "Hide");
	}
}

void
OptionListRow::PositionCursor(Actor* pCursor, int iSelection)
{
	const float fX = m_Text[iSelection].GetDestX();
	const float fY = m_Text[iSelection].GetDestY();
	if (m_bItemsInTwoRows)
		pCursor->PlayCommand("PositionTwoRows");
	else
		pCursor->PlayCommand("PositionOneRow");
	pCursor->SetXY(fX, fY);
}

OptionsList::OptionsList()
{
	m_iCurrentRow = 0;
	m_pLinked = nullptr;
	m_bStartIsDown = false;
	m_bAcceptStartRelease = false;
	m_pn = PLAYER_1;
	m_iMenuStackSelection = 0;
}

OptionsList::~OptionsList()
{
	for (auto& hand : m_Rows)
		delete hand.second;
}

void
OptionsList::Load(const std::string& sType, PlayerNumber pn)
{
	TOP_MENU.Load(sType, "TopMenu");

	m_pn = pn;
	m_bStartIsDown = false;

	m_Codes.Load(sType);

	m_Cursor.Load(THEME->GetPathG(sType, "cursor"));
	m_Cursor->SetName("Cursor");
	ActorUtil::LoadAllCommands(*m_Cursor, sType);
	this->AddChild(m_Cursor);

	std::vector<std::string> asDirectLines;
	split(DIRECT_LINES, ",", asDirectLines, true);
	for (auto& s : asDirectLines)
		m_setDirectRows.insert(s);

	std::vector<std::string> setToLoad;
	split(TOP_MENUS, ",", setToLoad);
	m_setTopMenus.insert(setToLoad.begin(), setToLoad.end());

	while (!setToLoad.empty()) {
		std::string sLineName = *setToLoad.begin();
		setToLoad.erase(setToLoad.begin());

		if (m_Rows.find(sLineName) != m_Rows.end())
			continue;

		std::string sRowCommands = LINE(sLineName);
		Commands cmds;
		ParseCommands(sRowCommands, cmds, false);

		OptionRowHandler* pHand = OptionRowHandlerUtil::Make(cmds);
		if (pHand == nullptr) {
			LuaHelpers::ReportScriptErrorFmt(
			  "Invalid OptionRowHandler '%s' in %s::Line%s",
			  cmds.GetOriginalCommandString().c_str(),
			  m_sName.c_str(),
			  sLineName.c_str());
			continue;
		}

		m_Rows[sLineName] = pHand;
		m_asLoadedRows.push_back(sLineName);

		for (size_t i = 0; i < pHand->m_Def.m_vsChoices.size(); ++i) {
			std::string sScreen = pHand->GetScreen(i);
			if (!sScreen.empty())
				setToLoad.push_back(sScreen);
		}
	}

	for (auto& i : m_Row) {
		i.SetName("OptionsList");
		i.Load(this, "OptionsList");
		ActorUtil::LoadAllCommands(i, sType);
		this->AddChild(&i);
	}

	this->PlayCommand("TweenOff");
	this->FinishTweening();
}

void
OptionsList::Reset()
{
	/* Import options. */
	for (auto& hand : m_Rows) {
		std::string sLineName = hand.first;
		ImportRow(sLineName);
	}
}

void
OptionsList::Open()
{
	this->PlayCommand("Reset");

	Message msg("OptionsListOpened");
	msg.SetParam("Player", m_pn);
	MESSAGEMAN->Broadcast(msg);

	/* Push the initial menu. */
	ASSERT(m_asMenuStack.empty());
	Push(TOP_MENU);

	this->FinishTweening();
	m_Row[!m_iCurrentRow].SetFromHandler(nullptr);
	this->PlayCommand("TweenOn");
}

void
OptionsList::Close()
{
	Message msg("OptionsListClosed");
	msg.SetParam("Player", m_pn);
	MESSAGEMAN->Broadcast(msg);

	m_bStartIsDown = false;
	m_asMenuStack.clear();
	this->PlayCommand("TweenOff");
}

std::string
OptionsList::GetCurrentRow() const
{
	ASSERT(!m_asMenuStack.empty()); // called while the menu was closed
	return m_asMenuStack.back();
}

const OptionRowHandler*
OptionsList::GetCurrentHandler()
{
	const std::string sCurrentRow = GetCurrentRow();
	return m_Rows[sCurrentRow];
}

int
OptionsList::GetOneSelection(const std::string& sRow, bool bAllowFail) const
{
	const std::map<std::string, std::vector<bool>>::const_iterator it =
	  m_bSelections.find(sRow);
	ASSERT_M(it != m_bSelections.end(), sRow);
	const std::vector<bool>& bSelections = it->second;
	for (unsigned i = 0; i < bSelections.size(); i++) {
		if (bSelections[i])
			return i;
	}

	ASSERT(
	  bAllowFail); // shouldn't call this if not expecting one to be selected
	return -1;
}

void
OptionsList::PositionCursor()
{
	m_Row[m_iCurrentRow].PositionCursor(m_Cursor, m_iMenuStackSelection);
}

/* Toggle to the next menu.  This is used to switch quickly through option
 * submenus, to choose many options or to find the one you're looking for.  For
 * that goal, it's not helpful to switch only through the options listed in the
 * current parent menu; always toggle through the whole set.  Skip menus that
 * only contain other menus. */
void
OptionsList::SwitchMenu(int iDir)
{
	/* Consider the menu as a list, where the main menu is the first item and
	 * the submenus follow.  This allows consistent navigation; moving right
	 * from the main menu walks through the menus, moving left goes back as far
	 * as the main menu.  Don't loop, so it's harder to lose track of menus. */
	const std::string sTopRow = m_asMenuStack.front();
	const OptionRowHandler* pHandler = m_Rows[sTopRow];
	int iCurrentRow = 0;
	if (m_asMenuStack.size() == 1)
		iCurrentRow = -1;
	else
		iCurrentRow = FindScreenInHandler(pHandler, m_asMenuStack.back());

	iCurrentRow += iDir;
	if (iCurrentRow >= 0) {
		if (iCurrentRow >= static_cast<int>(pHandler->m_Def.m_vsChoices.size()))
			return;
		const std::string sDest = pHandler->GetScreen(iCurrentRow);
		if (sDest.empty())
			return;

		if (m_asMenuStack.size() == 1)
			m_asMenuStack.push_back(sDest);
		else
			m_asMenuStack.back() = sDest;
	} else {
		if (m_asMenuStack.size() == 1)
			return;

		m_asMenuStack.pop_back();
	}

	SetDefaultCurrentRow();
	SwitchToCurrentRow();
	TweenOnCurrentRow(iDir > 0);
}

bool
OptionsList::Input(const InputEventPlus& input)
{
	Message msg("");
	if (m_Codes.InputMessage(input, msg))
		this->HandleMessage(msg);

	const OptionRowHandler* pHandler = GetCurrentHandler();

	const PlayerNumber pn = input.pn;
	if (m_bStartIsDown) {
		if (input.MenuI == GAME_BUTTON_LEFT ||
			input.MenuI == GAME_BUTTON_RIGHT) {
			if (input.type != IET_FIRST_PRESS)
				return false;

			m_bAcceptStartRelease = false;

			const std::string& sCurrentRow = m_asMenuStack.back();
			std::vector<bool>& bSelections = m_bSelections[sCurrentRow];
			if (m_iMenuStackSelection == static_cast<int>(bSelections.size()))
				return false;

			const std::string sDest =
			  pHandler->GetScreen(m_iMenuStackSelection);
			if (m_setDirectRows.find(sDest) != m_setDirectRows.end() &&
				!sDest.empty()) {
				const OptionRowHandler* pTarget = m_Rows[sDest];
				std::vector<bool>& bTargetSelections = m_bSelections[sDest];

				if (pTarget->m_Def.m_selectType == SELECT_ONE) {
					int iSelection = GetOneSelection(sDest);
					const int iDir =
					  (input.MenuI == GAME_BUTTON_RIGHT ? +1 : -1);
					iSelection += iDir;
					wrap(iSelection, bTargetSelections.size());
					SelectItem(sDest, iSelection);

					Message lMsg("OptionsListQuickChange");
					lMsg.SetParam("Player", pn);
					lMsg.SetParam("Direction", iDir);
					lMsg.SetParam("Selection", iSelection);
					MESSAGEMAN->Broadcast(lMsg);
				}
			}
			return true;
		}
	}

	if (input.MenuI == GAME_BUTTON_LEFT) {
		if (input.type == IET_RELEASE)
			return false;

		if (INPUTMAPPER->IsBeingPressed(GAME_BUTTON_RIGHT, pn)) {
			if (input.type == IET_FIRST_PRESS)
				SwitchMenu(-1);
			return true;
		}

		--m_iMenuStackSelection;
		wrap(m_iMenuStackSelection,
			 pHandler->m_Def.m_vsChoices.size() + 1); // +1 for exit row
		PositionCursor();

		Message lMsg("OptionsListLeft");
		lMsg.SetParam("Player", input.pn);
		lMsg.SetParam("Selection", m_iMenuStackSelection);
		MESSAGEMAN->Broadcast(lMsg);
		return true;
	}
	if (input.MenuI == GAME_BUTTON_RIGHT) {
		if (input.type == IET_RELEASE)
			return false;

		if (INPUTMAPPER->IsBeingPressed(GAME_BUTTON_LEFT, pn)) {
			if (input.type == IET_FIRST_PRESS)
				SwitchMenu(+1);
			return true;
		}

		++m_iMenuStackSelection;
		wrap(m_iMenuStackSelection,
			 pHandler->m_Def.m_vsChoices.size() + 1); // +1 for exit row
		PositionCursor();

		Message lMsg("OptionsListRight");
		lMsg.SetParam("Player", input.pn);
		lMsg.SetParam("Selection", m_iMenuStackSelection);
		MESSAGEMAN->Broadcast(lMsg);
		return true;
	}
	if (input.MenuI == GAME_BUTTON_START) {
		if (input.type == IET_FIRST_PRESS) {
			m_bStartIsDown = true;
			m_bAcceptStartRelease = true;
			return true;
		}
		if (input.type == IET_RELEASE) {
			if (m_bAcceptStartRelease)
				Start();
			m_bStartIsDown = false;
		}

		return true;
	}
	if (input.MenuI == GAME_BUTTON_SELECT) {
		if (input.type != IET_FIRST_PRESS)
			return false;
		// if( input.type == IET_RELEASE )
		{
			Close();
			return true;
		}
		return true;
	}
	return false;
}

void
OptionsList::SwitchToCurrentRow()
{
	m_iCurrentRow = static_cast<int>(m_iCurrentRow) == 0;

	/* Set up the new row. */
	m_Row[m_iCurrentRow].SetFromHandler(GetCurrentHandler());
	m_Row[m_iCurrentRow].SetUnderlines(m_bSelections[m_asMenuStack.back()],
									   GetCurrentHandler());
	PositionCursor();

	Message msg("OptionsMenuChanged");
	msg.SetParam("Player", m_pn);
	msg.SetParam("Menu", m_asMenuStack.back());
	MESSAGEMAN->Broadcast(msg);
}

/* After setting up a new row, tween it on. */
void
OptionsList::TweenOnCurrentRow(bool bForward)
{
	OptionListRow& OldRow = m_Row[!m_iCurrentRow];
	OptionListRow& NewRow = m_Row[m_iCurrentRow];

	/* Tween out the old row. */
	if (bForward)
		OldRow.PlayCommand("TweenOutForward");
	else
		OldRow.PlayCommand("TweenOutBackward");

	/* Tween in the old row. */
	if (bForward)
		NewRow.PlayCommand("TweenInForward");
	else
		NewRow.PlayCommand("TweenInBackward");
}

void
OptionsList::ImportRow(const std::string& sRow)
{
	std::vector<bool> aSelections;
	OptionRowHandler* pHandler = m_Rows[sRow];
	aSelections.resize(pHandler->m_Def.m_vsChoices.size());
	pHandler->ImportOption(nullptr, m_pn, aSelections);
	m_bSelections[sRow] = aSelections;

	if (m_setTopMenus.find(sRow) != m_setTopMenus.end())
		fill(m_bSelections[sRow].begin(), m_bSelections[sRow].end(), false);
}

void
OptionsList::ExportRow(const std::string& sRow)
{
	if (m_setTopMenus.find(sRow) != m_setTopMenus.end())
		return;

	std::vector<bool> aSelections;
	aSelections = m_bSelections[sRow];

	(void)m_Rows[sRow]->ExportOption(m_pn, aSelections);
}

void
OptionsList::SetDefaultCurrentRow()
{
	/* If all items on the row just point to other menus, default to 0. */
	m_iMenuStackSelection = 0;

	const std::string& sCurrentRow = m_asMenuStack.back();
	const OptionRowHandler* pHandler = m_Rows.find(sCurrentRow)->second;
	if (pHandler->m_Def.m_selectType == SELECT_ONE) {
		/* One item is selected, so position the cursor on it. */
		m_iMenuStackSelection = GetOneSelection(sCurrentRow, true);
		if (m_iMenuStackSelection == -1)
			m_iMenuStackSelection = 0;
	}
}

int
OptionsList::FindScreenInHandler(const OptionRowHandler* pHandler,
								 const std::string& sScreen)
{
	for (size_t i = 0; i < pHandler->m_Def.m_vsChoices.size(); ++i) {
		if (pHandler->GetScreen(i) == sScreen)
			return i;
	}
	return -1;
}

void
OptionsList::Pop()
{
	if (m_asMenuStack.size() == 1) {
		Close();
		return;
	}

	const std::string sLastMenu = m_asMenuStack.back();

	m_asMenuStack.pop_back();

	/* Choose the default option. */
	SetDefaultCurrentRow();

	/* If the old menu exists as a target from the new menu, switch to it. */
	const OptionRowHandler* pHandler = GetCurrentHandler();
	const int iIndex = FindScreenInHandler(pHandler, sLastMenu);
	if (iIndex != -1)
		m_iMenuStackSelection = iIndex;

	SwitchToCurrentRow();
	TweenOnCurrentRow(false);
}

void
OptionsList::Push(const std::string& sDest)
{
	m_asMenuStack.push_back(sDest);
	SetDefaultCurrentRow();
	SwitchToCurrentRow();
}

void
OptionsList::SelectItem(const std::string& sRowName, int iMenuItem)
{
	const OptionRowHandler* pHandler = m_Rows[sRowName];
	std::vector<bool>& bSelections = m_bSelections[sRowName];

	if (pHandler->m_Def.m_selectType == SELECT_MULTIPLE) {
		const bool bSelected = !bSelections[iMenuItem];
		bSelections[iMenuItem] = bSelected;

		//		if( bSelected )
		//			m_SoundToggleOn.Play();
		//		else
		//			m_SoundToggleOff.Play();
	} else // data.selectType != SELECT_MULTIPLE
	{
		fill(bSelections.begin(), bSelections.end(), false);
		bSelections[iMenuItem] = true;
	}

	SelectionsChanged(sRowName);
	UpdateMenuFromSelections();
}

void
OptionsList::SelectionsChanged(const std::string& sRowName)
{
	const OptionRowHandler* pHandler = m_Rows[sRowName];
	std::vector<bool>& bSelections = m_bSelections[sRowName];

	if (pHandler->m_Def.m_bOneChoiceForAllPlayers && m_pLinked != nullptr) {
		std::vector<bool>& bLinkedSelections =
		  m_pLinked->m_bSelections[sRowName];
		bLinkedSelections = bSelections;

		if (m_pLinked->IsOpened())
			m_pLinked->UpdateMenuFromSelections();

		m_pLinked->ExportRow(sRowName);
	}

	(void)ExportRow(sRowName);
}

void
OptionsList::UpdateMenuFromSelections()
{
	const std::vector<bool>& bCurrentSelections =
	  m_bSelections.find(GetCurrentRow())->second;
	m_Row[m_iCurrentRow].SetUnderlines(bCurrentSelections, GetCurrentHandler());
	m_Row[m_iCurrentRow].SetTextFromHandler(GetCurrentHandler());
}

bool
OptionsList::Start()
{
	const OptionRowHandler* pHandler = GetCurrentHandler();
	const std::string& sCurrentRow = m_asMenuStack.back();
	std::vector<bool>& bSelections = m_bSelections[sCurrentRow];
	if (m_iMenuStackSelection == static_cast<int>(bSelections.size())) {
		Pop();

		Message msg("OptionsListPop");
		msg.SetParam("Player", m_pn);
		MESSAGEMAN->Broadcast(msg);

		return m_asMenuStack.empty();
	}

	{
		std::string sIconText;
		GameCommand gc;
		pHandler->GetIconTextAndGameCommand(
		  m_iMenuStackSelection, sIconText, gc);
		if (gc.m_sName == RESET_ROW) {
			GAMESTATE->m_pPlayerState->ResetToDefaultPlayerOptions(
			  ModsLevel_Preferred);
			GAMESTATE->ResetToDefaultSongOptions(ModsLevel_Preferred);

			/* Import options. */
			for (auto& hand : m_Rows) {
				ImportRow(hand.first);
				SelectionsChanged(hand.first);
			}

			UpdateMenuFromSelections();

			Message msg("OptionsListReset");
			msg.SetParam("Player", m_pn);
			MESSAGEMAN->Broadcast(msg);

			return false;
		}
	}

	const std::string sDest = pHandler->GetScreen(m_iMenuStackSelection);
	if (!sDest.empty()) {
		Push(sDest);
		TweenOnCurrentRow(true);

		Message msg("OptionsListPush");
		msg.SetParam("Player", m_pn);
		MESSAGEMAN->Broadcast(msg);

		return false;
	}

	SelectItem(GetCurrentRow(), m_iMenuStackSelection);

	/* Move to the exit row. */
	m_iMenuStackSelection = static_cast<int>(bSelections.size());
	PositionCursor();

	Message msg("OptionsListStart");
	msg.SetParam("Player", m_pn);
	MESSAGEMAN->Broadcast(msg);

	return false;
}
