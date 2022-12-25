#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Singletons/GameState.h"
#include "OptionRow.h"
#include "Etterna/Models/Misc/OptionRowHandler.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Style.h"

#include <algorithm>

const std::string NEXT_ROW_NAME = "NextRow";
const std::string EXIT_NAME = "Exit";

std::string
OptionRow::GetThemedItemText(int iChoice) const
{
	std::string s = m_pHand->GetThemedItemText(iChoice);

	// HACK: Always theme the NEXT_ROW and EXIT items.
	if (m_bFirstItemGoesDown && iChoice == 0)
		s = CommonMetrics::LocalizeOptionItem(NEXT_ROW_NAME, false);
	else if (m_RowType == OptionRow::RowType_Exit)
		s = CommonMetrics::LocalizeOptionItem(EXIT_NAME, false);

	return s;
}

std::string
ITEMS_LONG_ROW_X_NAME(size_t p)
{
	return ssprintf("ItemsLongRowP%dX", static_cast<int>(p + 1));
}
std::string
MOD_ICON_X_NAME(size_t p)
{
	return ssprintf("ModIconP%dX", static_cast<int>(p + 1));
}

OptionRow::OptionRow(const OptionRowType* pSource)
{
	m_pParentType = pSource;
	m_pHand = nullptr;

	m_textTitle = nullptr;
	m_ModIcons = nullptr;

	m_RowType = OptionRow::RowType_Normal;
	m_sprFrame = nullptr;

	Clear();
	this->AddChild(&m_Frame);

	m_tsDestination.Init();
}

OptionRow::~OptionRow()
{
	Clear();
}

void
OptionRow::Clear()
{
	ActorFrame::RemoveAllChildren();

	m_vbSelected.clear();

	m_Frame.DeleteAllChildren();
	m_textItems.clear();
	m_Underline.clear();

	if (m_pHand != nullptr) {
		for (auto& m : m_pHand->m_vsReloadRowMessages) {
			MESSAGEMAN->Unsubscribe(this, m);
		}
	}
	SAFE_DELETE(m_pHand);

	m_bFirstItemGoesDown = false;
	m_bRowHasFocus = false;
	m_iChoiceInRowWithFocus = false;
}

void
OptionRowType::Load(const std::string& sMetricsGroup, Actor* pParent)
{
	m_sMetricsGroup = sMetricsGroup;

	ITEMS_START_X.Load(sMetricsGroup, "ItemsStartX");
	ITEMS_END_X.Load(sMetricsGroup, "ItemsEndX");
	ITEMS_GAP_X.Load(sMetricsGroup, "ItemsGapX");
	ITEMS_MIN_BASE_ZOOM.Load(sMetricsGroup, "ItemsMinBaseZoom");
	ITEMS_LONG_ROW_X.Load(sMetricsGroup, ITEMS_LONG_ROW_X_NAME, NUM_PLAYERS);
	ITEMS_LONG_ROW_SHARED_X.Load(sMetricsGroup, "ItemsLongRowSharedX");
	MOD_ICON_X.Load(sMetricsGroup, MOD_ICON_X_NAME, NUM_PLAYERS);
	COLOR_SELECTED.Load(sMetricsGroup, "ColorSelected");
	COLOR_NOT_SELECTED.Load(sMetricsGroup, "ColorNotSelected");
	COLOR_DISABLED.Load(sMetricsGroup, "ColorDisabled");
	TWEEN_SECONDS.Load(sMetricsGroup, "TweenSeconds");
	SHOW_BPM_IN_SPEED_TITLE.Load(sMetricsGroup, "ShowBpmInSpeedTitle");
	SHOW_MOD_ICONS.Load(sMetricsGroup, "ShowModIcons");
	SHOW_UNDERLINES.Load(sMetricsGroup, "ShowUnderlines");
	MOD_ICON_METRICS_GROUP.Load(sMetricsGroup, "ModIconMetricsGroup");

	m_textItem.LoadFromFont(THEME->GetPathF(sMetricsGroup, "Item"));
	m_textItem.SetName("Item");
	ActorUtil::LoadAllCommands(m_textItem, sMetricsGroup);

	if (SHOW_UNDERLINES) {
		m_Underline.Load("OptionsUnderline" + PlayerNumberToString(PLAYER_1),
						 false);
	}

	m_textTitle.LoadFromFont(THEME->GetPathF(sMetricsGroup, "title"));
	m_textTitle.SetName("Title");
	ActorUtil::LoadAllCommandsAndSetXY(m_textTitle, sMetricsGroup);

	Actor* pActor =
	  ActorUtil::MakeActor(THEME->GetPathG(sMetricsGroup, "Frame"), pParent);
	if (pActor == nullptr)
		pActor = new Actor;
	m_sprFrame.Load(pActor);
	m_sprFrame->SetName("Frame");
	ActorUtil::LoadAllCommandsAndSetXY(m_sprFrame, sMetricsGroup);

	if (SHOW_MOD_ICONS) {
		m_ModIcon.Load(MOD_ICON_METRICS_GROUP);
		m_ModIcon.SetName("ModIcon");
		ActorUtil::LoadAllCommands(m_ModIcon, sMetricsGroup);
	}
}

void
OptionRow::LoadNormal(OptionRowHandler* pHand, bool bFirstItemGoesDown)
{
	m_RowType = OptionRow::RowType_Normal;
	m_pHand = pHand;
	m_bFirstItemGoesDown = bFirstItemGoesDown;

	for (auto& m : m_pHand->m_vsReloadRowMessages) {
		MESSAGEMAN->Subscribe(this, m);
	}

	ChoicesChanged(RowType_Normal);
}

void
OptionRow::LoadExit()
{
	m_RowType = OptionRow::RowType_Exit;
	OptionRowHandler* pHand = OptionRowHandlerUtil::MakeNull();
	pHand->m_Def.m_selectType = SELECT_NONE;
	pHand->m_Def.m_sName = EXIT_NAME;
	pHand->m_Def.m_vsChoices.push_back(EXIT_NAME);
	pHand->m_Def.m_layoutType = LAYOUT_SHOW_ONE_IN_ROW;
	pHand->m_Def.m_bOneChoiceForAllPlayers = true;
	m_pHand = pHand;

	ChoicesChanged(RowType_Exit);
}

void
OptionRow::ChoicesChanged(RowType type, bool reset_focus)
{
	ASSERT_M(!m_pHand->m_Def.m_vsChoices.empty(),
			 m_pHand->m_Def.m_sName + " has no choices");

	// Remove the NextRow marker before reloading choices
	if (m_pHand->m_Def.m_vsChoices[0] == NEXT_ROW_NAME) {
		m_pHand->m_Def.m_vsChoices.erase(m_pHand->m_Def.m_vsChoices.begin());
		m_vbSelected.erase(m_vbSelected.begin());
	}

	std::vector<bool>& vbSelected = m_vbSelected;
	vbSelected.resize(0);
	vbSelected.resize(m_pHand->m_Def.m_vsChoices.size(), false);

	// set select the first item if a SELECT_ONE row
	if (!vbSelected.empty() && m_pHand->m_Def.m_selectType == SELECT_ONE)
		vbSelected[0] = true;

	// TRICKY: Insert a down arrow as the first choice in the row.
	if (m_bFirstItemGoesDown) {
		m_pHand->m_Def.m_vsChoices.insert(m_pHand->m_Def.m_vsChoices.begin(),
										  NEXT_ROW_NAME);
		m_vbSelected.insert(m_vbSelected.begin(), false);
	}

	InitText(type);

	// Lua can change the choices now, and when it does, we don't want to change
	// focus.
	if (reset_focus) {
		// When choices change, the old focus position is meaningless; reset it.
		SetChoiceInRowWithFocus(PLAYER_1, 0);
	}

	m_textTitle->SetText(GetRowTitle());
}

std::string
OptionRow::GetRowTitle() const
{
	std::string sTitle = m_pHand->OptionTitle();

	// HACK: tack the BPM onto the name of the speed line
	if (CompareNoCase(m_pHand->m_Def.m_sName, "speed") == 0) {
		const bool bShowBpmInSpeedTitle =
		  m_pParentType->SHOW_BPM_IN_SPEED_TITLE;

		if (bShowBpmInSpeedTitle) {
			DisplayBpms bpms;
			if (GAMESTATE->m_pCurSong) {
				const Song* pSong = GAMESTATE->m_pCurSong;
				pSong->GetDisplayBpms(bpms, false);
			}

			if (bpms.IsSecret())
				sTitle += ssprintf(
				  " (??"
				  "?)"); // split so gcc doesn't think this is a trigraph
			else if (bpms.BpmIsConstant())
				sTitle += ssprintf(" (%.0f)", bpms.GetMin());
			else
				sTitle +=
				  ssprintf(" (%.0f-%.0f)", bpms.GetMin(), bpms.GetMax());
		}
	}

	return sTitle;
}

/* Set up text, underlines and titles for options. This can be called as soon as
 * m_pHand->m_Def is available. */
void
OptionRow::InitText(RowType type)
{
	/* If we have elements already, we're being updated from a new set of
	 * options. Delete the old ones. */
	m_Frame.DeleteAllChildren();
	m_textItems.clear();
	m_Underline.clear();

	m_textTitle = new BitmapText(m_pParentType->m_textTitle);
	m_Frame.AddChild(m_textTitle);

	m_sprFrame = m_pParentType->m_sprFrame->Copy();
	m_sprFrame->SetDrawOrder(-1); // under title
	m_Frame.AddChild(m_sprFrame);

	if (m_pParentType->SHOW_MOD_ICONS) {
		switch (m_RowType) {
			case RowType_Normal: {
				m_ModIcons = new ModIcon(m_pParentType->m_ModIcon);
				m_ModIcons->SetDrawOrder(-1); // under title
				m_ModIcons->PlayCommand("On");

				m_Frame.AddChild(m_ModIcons);

				GameCommand gc;
				SetModIcon(PLAYER_1, "", gc);
				break;
			}
			case RowType_Exit:
				break;
		}
	}

	// If the items will go off the edge of the screen, then force
	// LAYOUT_SHOW_ONE_IN_ROW.
	float fBaseZoom = 1.0f;
	{
		BitmapText bt(m_pParentType->m_textItem);
		bt.PlayCommand("On");

		// Figure out the width of the row.
		float fWidth = 0;
		for (unsigned c = 0; c < m_pHand->m_Def.m_vsChoices.size(); c++) {
			std::string sText = GetThemedItemText(c);
			bt.SetText(sText);

			fWidth += bt.GetZoomedWidth();

			if (c != m_pHand->m_Def.m_vsChoices.size() - 1)
				fWidth += m_pParentType->ITEMS_GAP_X;
		}

		// Try to fit everything on one line.
		const float fTotalWidth =
		  m_pParentType->ITEMS_END_X - m_pParentType->ITEMS_START_X;
		if (fWidth > fTotalWidth) {
			const float fPossibleBaseZoom = fTotalWidth / fWidth;
			if (fPossibleBaseZoom >= m_pParentType->ITEMS_MIN_BASE_ZOOM)
				fBaseZoom = fPossibleBaseZoom;
			else
				m_pHand->m_Def.m_layoutType = LAYOUT_SHOW_ONE_IN_ROW;
		}
	}

	// load m_textItems
	switch (m_pHand->m_Def.m_layoutType) {
		case LAYOUT_SHOW_ONE_IN_ROW: { // init text
			BitmapText* pText = new BitmapText(m_pParentType->m_textItem);
			m_textItems.push_back(pText);

			pText->PlayCommand("On");

			if (m_pHand->m_Def.m_bOneChoiceForAllPlayers) {
				pText->SetX(m_pParentType->ITEMS_LONG_ROW_SHARED_X);
				break; // only initialize one item since it's shared
			} else {
				pText->SetX(m_pParentType->ITEMS_LONG_ROW_X.GetValue(PLAYER_1));
			}

			// Set the text now, so SetWidthXY below is correct.
			UpdateText(PLAYER_1);

			// init underlines
			if (m_pParentType->SHOW_UNDERLINES &&
				GetRowType() != OptionRow::RowType_Exit) {
				OptionsCursor* pCursor =
				  new OptionsCursor(m_pParentType->m_Underline);
				m_Underline.push_back(pCursor);

				int iWidth, iX, iY;
				GetWidthXY(PLAYER_1, 0, iWidth, iX, iY);
				pCursor->SetX(static_cast<float>(iX));
				pCursor->SetBarWidth(iWidth);
			}
			break;
		}
		case LAYOUT_SHOW_ALL_IN_ROW: {
			float fX = m_pParentType->ITEMS_START_X;
			for (unsigned c = 0; c < m_pHand->m_Def.m_vsChoices.size(); c++) {
				// init text
				BitmapText* bt = new BitmapText(m_pParentType->m_textItem);
				m_textItems.push_back(bt);
				bt->SetBaseZoomX(fBaseZoom);
				bt->PlayCommand("On");
				// Set text after running OnCommand so e.g. uppercase,true works
				// -aj
				std::string sText = GetThemedItemText(c);
				bt->SetText(sText);

				// set the X position of each item in the line
				const float fItemWidth = bt->GetZoomedWidth();
				fX += fItemWidth / 2;
				bt->SetX(fX);

				// init underlines
				if (m_pParentType->SHOW_UNDERLINES) {
					OptionsCursor* ul =
					  new OptionsCursor(m_pParentType->m_Underline);
					m_Underline.push_back(ul);
					ul->SetX(fX);
					ul->SetBarWidth(static_cast<int>(fItemWidth));
				}

				fX += fItemWidth / 2 + m_pParentType->ITEMS_GAP_X * fBaseZoom;
			}
		} break;

		default:
			FAIL_M(ssprintf("Invalid option row layout: %i",
							m_pHand->m_Def.m_layoutType));
	}

	for (auto& m_textItem : m_textItems)
		m_Frame.AddChild(m_textItem);
	for (auto& c : m_Underline)
		m_Frame.AddChild(c);

	// This is set in OptionRow::AfterImportOptions, so if we're reused with a
	// different song selected, SHOW_BPM_IN_SPEED_TITLE will show the new BPM.
	// m_textTitle->SetText( GetRowTitle() );
	m_textTitle->PlayCommand("On");

	m_sprFrame->PlayCommand("On");

	m_Frame.SortByDrawOrder();
	this->SortByDrawOrder();
}

// After importing options, choose which item is focused.
void
OptionRow::AfterImportOptions(PlayerNumber pn)
{
	/* We load items for both players on start, since we don't know which
	 * players will be joined when we're displayed. Hide items for inactive
	 * players. */
	if (m_pHand->m_Def.m_layoutType == LAYOUT_SHOW_ONE_IN_ROW &&
		!m_pHand->m_Def.m_bOneChoiceForAllPlayers)
		m_textItems[pn]->SetVisible(GAMESTATE->IsHumanPlayer(pn));

	// Hide underlines for disabled players.
	if (!GAMESTATE->IsHumanPlayer(pn))
		for (auto& c : m_Underline)
			c->SetVisible(false);

	switch (m_pHand->m_Def.m_selectType) {
		case SELECT_ONE: {
			// Make sure the row actually has a selection.
			const int iSelection = GetOneSelection(pn, true);
			if (iSelection == -1) {
				ASSERT(!m_vbSelected.empty());
				m_vbSelected[0] = true;
			}
		}
		default:
			break;
	}

	ResetFocusFromSelection(pn);

	PositionUnderlines(pn);
}

void
OptionRow::PositionUnderlines(PlayerNumber pn)
{
	std::vector<OptionsCursor*>& vpUnderlines = m_Underline;
	if (vpUnderlines.empty())
		return;

	for (int i = 0; i < static_cast<int>(vpUnderlines.size()); i++) {
		OptionsCursor& ul = *vpUnderlines[i];

		const int iChoiceWithFocus =
		  (m_pHand->m_Def.m_layoutType == LAYOUT_SHOW_ONE_IN_ROW)
			? GetChoiceInRowWithFocus()
			: i;

		if (iChoiceWithFocus == -1)
			continue;

		float fAlpha = 1.0f;
		if (m_pHand->m_Def.m_layoutType == LAYOUT_SHOW_ONE_IN_ROW) {
			const bool bRowEnabled =
			  m_pHand->m_Def.m_vEnabledForPlayers.find(pn) !=
			  m_pHand->m_Def.m_vEnabledForPlayers.end();

			if (!m_pHand->m_Def.m_bOneChoiceForAllPlayers) {
				if (m_bRowHasFocus)
					fAlpha = m_pParentType->COLOR_SELECTED.GetValue().a;
				else if (bRowEnabled)
					fAlpha = m_pParentType->COLOR_NOT_SELECTED.GetValue().a;
				else
					fAlpha = m_pParentType->COLOR_DISABLED.GetValue().a;
			}
		}

		// Don't tween X movement and color changes.
		ul.StopTweening();

		int iWidth, iX, iY;
		GetWidthXY(pn, iChoiceWithFocus, iWidth, iX, iY);
		ul.SetX(static_cast<float>(iX));
		// only set alpha, in case a theme tries to color underlines. -aj
		ul.SetDiffuseAlpha(fAlpha);

		ASSERT(m_vbSelected.size() == m_pHand->m_Def.m_vsChoices.size());

		const bool bSelected = m_vbSelected[iChoiceWithFocus];
		const bool bVisible = bSelected && GAMESTATE->IsHumanPlayer(pn);

		ul.BeginTweening(m_pParentType->TWEEN_SECONDS);
		ul.SetVisible(bVisible);
		ul.SetBarWidth(iWidth);
	}
}

void
OptionRow::PositionIcons(PlayerNumber pn)
{
	ModIcon* pIcon = m_ModIcons;
	if (pIcon == nullptr)
		return;

	pIcon->SetX(m_pParentType->MOD_ICON_X.GetValue(pn));
}

// This is called when the focus changes, to update "long row" text.
void
OptionRow::UpdateText(PlayerNumber p)
{
	switch (m_pHand->m_Def.m_layoutType) {
		case LAYOUT_SHOW_ONE_IN_ROW: {
			const unsigned pn =
			  m_pHand->m_Def.m_bOneChoiceForAllPlayers ? 0 : p;
			const int iChoiceWithFocus = m_iChoiceInRowWithFocus;
			if (iChoiceWithFocus == -1)
				break;

			const std::string sText = GetThemedItemText(iChoiceWithFocus);

			// If player_no is 2 and there is no player 1:
			const int index =
			  std::min(pn, static_cast<unsigned>(m_textItems.size()) - 1U);

			// TODO: Always have one textItem for each player

			m_textItems[index]->SetText(sText);
		}
		default:
			break;
	}
}

void
OptionRow::SetRowHasFocus(PlayerNumber pn, bool bRowHasFocus)
{
	m_bRowHasFocus = bRowHasFocus;
}

void
OptionRow::SetDestination(Actor::TweenState& ts, bool bTween)
{
	if (m_Frame.DestTweenState() != ts) {
		m_Frame.StopTweening();
		if (bTween && m_pParentType->TWEEN_SECONDS != 0.0f)
			m_Frame.BeginTweening(m_pParentType->TWEEN_SECONDS);
		m_Frame.DestTweenState() = ts;
	}
}

void
OptionRow::UpdateEnabledDisabled()
{
	bool bThisRowHasFocusByAny = false;
	bThisRowHasFocusByAny |= static_cast<int>(m_bRowHasFocus);

	bool bRowEnabled = !m_pHand->m_Def.m_vEnabledForPlayers.empty();

	// Don't tween selection colors at all.
	std::string sCmdName;
	if (bThisRowHasFocusByAny)
		sCmdName = "GainFocus";
	else if (bRowEnabled)
		sCmdName = "LoseFocus";
	else
		sCmdName = "Disabled";

	RageColor color;
	if (bThisRowHasFocusByAny)
		color = m_pParentType->COLOR_SELECTED;
	else if (bRowEnabled)
		color = m_pParentType->COLOR_NOT_SELECTED;
	else
		color = m_pParentType->COLOR_DISABLED;

	m_sprFrame->PlayCommand(sCmdName);
	m_textTitle->PlayCommand(sCmdName);

	for (unsigned j = 0; j < m_textItems.size(); j++) {
		m_textItems[j]->PlayCommand("LoseFocus");
	}

	switch (m_pHand->m_Def.m_layoutType) {
		case LAYOUT_SHOW_ALL_IN_ROW:
			for (auto& m_textItem : m_textItems) {
				if (m_textItem->DestTweenState().diffuse[0] == color)
					continue;

				m_textItem->StopTweening();
				m_textItem->BeginTweening(m_pParentType->TWEEN_SECONDS);
				m_textItem->SetDiffuse(color);
			}

			break;
		case LAYOUT_SHOW_ONE_IN_ROW:
			bRowEnabled = m_pHand->m_Def.m_vEnabledForPlayers.find(PLAYER_1) !=
						  m_pHand->m_Def.m_vEnabledForPlayers.end();

			if (!m_pHand->m_Def.m_bOneChoiceForAllPlayers) {
				if (m_bRowHasFocus)
					color = m_pParentType->COLOR_SELECTED;
				else if (bRowEnabled)
					color = m_pParentType->COLOR_NOT_SELECTED;
				else
					color = m_pParentType->COLOR_DISABLED;
			}
			{
				unsigned item_no = 0;

				// If player_no is 2 and there is no player 1:
				item_no = std::min(
				  item_no, static_cast<unsigned>(m_textItems.size()) - 1U);

				BitmapText& bt = *m_textItems[item_no];

				if (bt.DestTweenState().diffuse[0] != color) {
					bt.StopTweening();
					bt.BeginTweening(m_pParentType->TWEEN_SECONDS);
					bt.SetDiffuse(color);
				}
			}
			break;
		default:
			FAIL_M(ssprintf("Invalid option row layout: %i",
							m_pHand->m_Def.m_layoutType));
	}
}

void
OptionRow::SetModIcon(PlayerNumber pn,
					  const std::string& sText,
					  GameCommand& gc)
{
	// update row frame
	Message msg("Refresh");
	msg.SetParam("GameCommand", &gc);
	msg.SetParam("Text", sText);
	m_sprFrame->HandleMessage(msg);
	if (m_ModIcons != nullptr)
		m_ModIcons->Set(sText);
}

const BitmapText&
OptionRow::GetTextItemForRow(PlayerNumber pn, int iChoiceOnRow) const
{
	const bool bOneChoice = m_pHand->m_Def.m_bOneChoiceForAllPlayers;
	int index = -1;
	switch (m_pHand->m_Def.m_layoutType) {
		case LAYOUT_SHOW_ONE_IN_ROW:
			index = bOneChoice ? 0 : pn;
			// If only P2 is enabled, his selections will be in index 0.
			if (m_textItems.size() == 1)
				index = 0;
			break;
		case LAYOUT_SHOW_ALL_IN_ROW:
			index = iChoiceOnRow;
			break;
		default:
			FAIL_M(ssprintf("Invalid option row layout: %i",
							m_pHand->m_Def.m_layoutType));
	}

	ASSERT_M(index < static_cast<int>(m_textItems.size()),
			 ssprintf("%i < %i", index, (int)m_textItems.size()));
	return *m_textItems[index];
}

void
OptionRow::GetWidthXY(PlayerNumber pn,
					  int iChoiceOnRow,
					  int& iWidthOut,
					  int& iXOut,
					  int& iYOut) const
{
	const BitmapText& text = GetTextItemForRow(pn, iChoiceOnRow);

	iWidthOut = lround(text.GetZoomedWidth());
	iXOut = lround(text.GetDestX());
	iYOut = lround(m_Frame.GetDestY());
}

int
OptionRow::GetOneSelection(PlayerNumber pn, bool bAllowFail) const
{
	for (unsigned i = 0; i < m_vbSelected.size(); i++)
		if (m_vbSelected[i])
			return i;

	ASSERT(
	  bAllowFail); // shouldn't call this if not expecting one to be selected
	return -1;
}

int
OptionRow::GetOneSharedSelection(bool bAllowFail) const
{
	return GetOneSelection(PLAYER_1, bAllowFail);
}

void
OptionRow::SetOneSelection(PlayerNumber pn, int iChoice)
{
	std::vector<bool>& vb = m_vbSelected;
	if (vb.empty())
		return;
	FOREACH(bool, vb, b)
	*b = false;
	vb[iChoice] = true;
	NotifyHandlerOfSelection(pn, iChoice);
}

void
OptionRow::SetOneSharedSelection(int iChoice)
{
	SetOneSelection(PLAYER_1, iChoice);
}

void
OptionRow::SetOneSharedSelectionIfPresent(const std::string& sChoice)
{
	for (unsigned i = 0; i < m_pHand->m_Def.m_vsChoices.size(); i++) {
		if (sChoice == m_pHand->m_Def.m_vsChoices[i]) {
			SetOneSharedSelection(i);
			break;
		}
	}
}

int
OptionRow::GetChoiceInRowWithFocus() const
{
	if (m_pHand->m_Def.m_vsChoices.empty())
		return -1;
	const int iChoice = m_iChoiceInRowWithFocus;
	return iChoice;
}

int
OptionRow::GetChoiceInRowWithFocusShared() const
{
	return GetChoiceInRowWithFocus();
}

void
OptionRow::SetChoiceInRowWithFocus(PlayerNumber pn, int iChoice)
{
	if (m_pHand->m_Def.m_bOneChoiceForAllPlayers)
		pn = PLAYER_1;
	ASSERT(iChoice >= 0 &&
		   iChoice < static_cast<int>(m_pHand->m_Def.m_vsChoices.size()));
	m_iChoiceInRowWithFocus = iChoice;

	UpdateText(pn);
	// PositionUnderlines( pn );
}

void
OptionRow::ResetFocusFromSelection(PlayerNumber pn)
{
	int iSelection = -1;
	switch (m_pHand->m_Def.m_selectType) {
		case SELECT_ONE:
			// Import the focus from the selected option.
			iSelection = GetOneSelection(pn, true);
		default:
			break;
	}

	// HACK: Set focus to one item in the row, which is "go down"
	if (m_bFirstItemGoesDown)
		iSelection = 0;

	if (iSelection != -1)
		SetChoiceInRowWithFocus(pn, iSelection);
}

bool
OptionRow::GetSelected(int iChoice) const
{
	return m_vbSelected[iChoice];
}

const OptionRowDefinition&
OptionRow::GetRowDef() const
{
	return m_pHand->m_Def;
}

OptionRowDefinition&
OptionRow::GetRowDef()
{
	return m_pHand->m_Def;
}

bool
OptionRow::SetSelected(PlayerNumber pn, int iChoice, bool b)
{
	if (m_pHand->m_Def.m_bOneChoiceForAllPlayers)
		pn = PLAYER_1;
	m_vbSelected[iChoice] = b;
	return NotifyHandlerOfSelection(pn, iChoice);
}

bool
OptionRow::NotifyHandlerOfSelection(PlayerNumber pn, int choice)
{
	const bool changed = m_pHand->NotifyOfSelection(
	  pn, choice - static_cast<int>(m_bFirstItemGoesDown));
	if (changed) {
		ChoicesChanged(m_RowType, false);
		ImportOptions(PLAYER_1);
		PositionUnderlines(PLAYER_1);
		UpdateEnabledDisabled();
	}
	return changed;
}

bool
OptionRow::GoToFirstOnStart()
{
	return m_pHand->GoToFirstOnStart();
}

void
OptionRow::SetExitText(const std::string& sExitText)
{
	BitmapText* bt = m_textItems.back();
	bt->SetText(sExitText);
}

void
OptionRow::Reload()
{
	// TODO: Nothing uses this yet and it causes skips when changing options.
	/*
	if( m_pHand->m_Def.m_bExportOnChange )
	{
		bool bRowHasFocus;
		ZERO( bRowHasFocus );
		ExportOptions( vpns, bRowHasFocus );
	}
	*/

	switch (m_pHand->Reload()) {
		case RELOAD_CHANGED_NONE:
			break;

		case RELOAD_CHANGED_ALL: {
			ChoicesChanged(m_RowType);

			ImportOptions(PLAYER_1);
			AfterImportOptions(PLAYER_1);
			// fall through
		}
		case RELOAD_CHANGED_ENABLED:
			UpdateEnabledDisabled();
			PositionUnderlines(PLAYER_1);
			break;
		default:
			break;
	}

	// TODO: Nothing uses this yet and it causes skips when changing options.
	/*
	if( m_pHand->m_Def.m_bExportOnChange )
	{
		bool bRowHasFocus;
		ZERO( bRowHasFocus );
		ExportOptions( vpns, bRowHasFocus );
	}
	*/
}

void
OptionRow::HandleMessage(const Message& msg)
{
	bool bReload = false;
	FOREACH_CONST(std::string, m_pHand->m_vsReloadRowMessages, m)
	{
		if (*m == msg.GetName())
			bReload = true;
	}
	if (bReload)
		Reload();

	ActorFrame::HandleMessage(msg);
}

/* Hack: the NextRow entry is never set, and should be transparent.
 * Remove it, and readd it below. */
#define ERASE_ONE_BOOL_AT_FRONT_IF_NEEDED(vbSelected)                          \
	if (GetFirstItemGoesDown())                                                \
		(vbSelected).erase((vbSelected).begin());
#define INSERT_ONE_BOOL_AT_FRONT_IF_NEEDED(vbSelected)                         \
	if (GetFirstItemGoesDown())                                                \
		(vbSelected).insert((vbSelected).begin(), false);

void
OptionRow::ImportOptions(const PlayerNumber& vpns)
{
	ASSERT(!m_pHand->m_Def.m_vsChoices.empty());

	FOREACH(bool, m_vbSelected, b)
	*b = false;

	ASSERT(m_vbSelected.size() == m_pHand->m_Def.m_vsChoices.size());
	ERASE_ONE_BOOL_AT_FRONT_IF_NEEDED(m_vbSelected);

	m_pHand->ImportOption(this, vpns, m_vbSelected);

	INSERT_ONE_BOOL_AT_FRONT_IF_NEEDED(m_vbSelected);
	VerifySelected(
	  m_pHand->m_Def.m_selectType, m_vbSelected, m_pHand->m_Def.m_sName);
}

int
OptionRow::ExportOptions(const PlayerNumber& vpns, bool bRowHasFocus)
{
	ASSERT(!m_pHand->m_Def.m_vsChoices.empty());

	int iChangeMask = 0;
	const bool bFocus = bRowHasFocus;

	VerifySelected(
	  m_pHand->m_Def.m_selectType, m_vbSelected, m_pHand->m_Def.m_sName);
	ASSERT(m_vbSelected.size() == m_pHand->m_Def.m_vsChoices.size());
	ERASE_ONE_BOOL_AT_FRONT_IF_NEEDED(m_vbSelected);

	// SELECT_NONE rows get exported if they have focus when the user
	// presses Start.
	const int iChoice = GetChoiceInRowWithFocus();
	if (m_pHand->m_Def.m_selectType == SELECT_NONE && bFocus && iChoice != -1)
		m_vbSelected[iChoice] = true;

	iChangeMask |= m_pHand->ExportOption(vpns, m_vbSelected);

	if (m_pHand->m_Def.m_selectType == SELECT_NONE && bFocus && iChoice != -1)
		m_vbSelected[iChoice] = false;

	INSERT_ONE_BOOL_AT_FRONT_IF_NEEDED(m_vbSelected);

	return iChangeMask;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

class LunaOptionRow : public Luna<OptionRow>
{
  public:
	DEFINE_METHOD(FirstItemGoesDown, GetFirstItemGoesDown())
	static int GetChoiceInRowWithFocus(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetChoiceInRowWithFocus());
		return 1;
	}
	DEFINE_METHOD(GetLayoutType, GetHandler()->m_Def.m_layoutType)
	static int GetName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetHandler()->m_Def.m_sName.c_str());
		return 1;
	}
	static int GetNumChoices(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetHandler()->m_Def.m_vsChoices.size());
		return 1;
	}
	DEFINE_METHOD(GetSelectType, GetHandler()->m_Def.m_selectType)
	DEFINE_METHOD(GetRowTitle, GetRowTitle())
	static int HasFocus(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->GetRowHasFocus(PLAYER_1));
		return 1;
	}
	static int OneChoiceForAllPlayers(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->GetHandler()->m_Def.m_bOneChoiceForAllPlayers);
		return 1;
	}

	LunaOptionRow()
	{
		ADD_METHOD(FirstItemGoesDown);
		ADD_METHOD(GetChoiceInRowWithFocus);
		ADD_METHOD(GetLayoutType);
		ADD_METHOD(GetName);
		ADD_METHOD(GetNumChoices);
		ADD_METHOD(GetRowTitle);
		ADD_METHOD(GetSelectType);
		ADD_METHOD(HasFocus);
		ADD_METHOD(OneChoiceForAllPlayers);
	}
};

LUA_REGISTER_DERIVED_CLASS(OptionRow, ActorFrame)
// lua end
