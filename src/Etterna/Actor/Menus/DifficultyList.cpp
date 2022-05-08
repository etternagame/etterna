#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "DifficultyList.h"

#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Songs/SongUtil.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Actor/GameplayAndMenus/StepsDisplay.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/FileTypes/XmlFile.h"

#include <algorithm>

/** @brief Specifies the max number of charts available for a song.
 *
 * This includes autogenned charts. */

// initial allocation for number of difficulties to load in the display
// should be a reasonable number that covers most cases, but the code
// will handle cases where the number needs to be larger
#define LINES_INITIAL_ALLOCATION 12

REGISTER_ACTOR_CLASS(StepsDisplayList);

StepsDisplayList::StepsDisplayList()
{
	m_CurSong = nullptr;
	m_bShown = true;
	SubscribeToMessage(
	  static_cast<MessageID>(Message_CurrentStepsChanged + PLAYER_1));
}

StepsDisplayList::~StepsDisplayList() = default;

void
StepsDisplayList::LoadFromNode(const XNode* pNode)
{
	ActorFrame::LoadFromNode(pNode);

	if (m_sName.empty()) {
		LuaHelpers::ReportScriptError("StepsDisplayList must have a Name");
		return;
	}

	ITEMS_SPACING_Y.Load(m_sName, "ItemsSpacingY");
	NUM_SHOWN_ITEMS.Load(m_sName, "NumShownItems");
	CAPITALIZE_DIFFICULTY_NAMES.Load(m_sName, "CapitalizeDifficultyNames");
	MOVE_COMMAND.Load(m_sName, "MoveCommand");

	m_Lines.lines.resize(LINES_INITIAL_ALLOCATION);
	m_CurSong = nullptr;

	const XNode* pChild = pNode->GetChild(ssprintf("CursorP%i", PLAYER_1 + 1));
	if (pChild == nullptr) {
		LuaHelpers::ReportScriptErrorFmt(
		  "%s: StepsDisplayList: missing the node \"CursorP%d\"",
		  ActorUtil::GetWhere(pNode).c_str(),
		  PLAYER_1 + 1);
	} else {
		m_Cursors.LoadActorFromNode(pChild, this);
	}

	/* Hack: we need to tween cursors both up to down (cursor motion) and
	 * visible to invisible (fading).  Cursor motion needs to stoptweening,
	 * so multiple motions don't queue and look unresponsive.  However, that
	 * stoptweening interrupts fading, resulting in the cursor remaining
	 * invisible or partially invisible.  So, do them in separate tweening
	 * stacks.  This means the Cursor command can't change diffuse colors; I
	 * think we do need a diffuse color stack ... */
	pChild = pNode->GetChild(ssprintf("CursorP%iFrame", PLAYER_1 + 1));
	if (pChild == nullptr) {
		LuaHelpers::ReportScriptErrorFmt(
		  "%s: StepsDisplayList: missing the node \"CursorP%dFrame\"",
		  ActorUtil::GetWhere(pNode).c_str(),
		  PLAYER_1 + 1);
	} else {
		m_CursorFrames.LoadFromNode(pChild);
		m_CursorFrames.AddChild(m_Cursors);
		this->AddChild(&m_CursorFrames);
	}

	for (auto& m_Line : m_Lines.lines) {
		m_Line = std::make_unique<StepsDisplay>();
		// todo: Use Row1, Row2 for names? also m_sName+"Row" -aj
		m_Line->SetName("Row");
		m_Line->Load("StepsDisplayListRow", nullptr);
		this->AddChild(m_Line.get());
	}

	UpdatePositions();
	PositionItems();
}

int
StepsDisplayList::GetCurrentRowIndex(PlayerNumber pn) const
{
	const Difficulty ClosestDifficulty =
	  GAMESTATE->GetClosestShownDifficulty(pn);

	for (unsigned i = 0; i < m_Rows.size(); i++) {
		const Row& row = m_Rows[i];

		if (GAMESTATE->m_pCurSteps == nullptr) {
			if (row.m_dc == ClosestDifficulty)
				return i;
		} else {
			if (GAMESTATE->m_pCurSteps.Get() == row.m_Steps)
				return i;
		}
	}

	return 0;
}

// Update m_fY and m_bHidden[].
void
StepsDisplayList::UpdatePositions()
{
	const int iCurrentRow = GetCurrentRowIndex(PLAYER_1);

	const int total = NUM_SHOWN_ITEMS;
	const int halfsize = total / 2;

	int first_start, first_end, second_start, second_end;

	// Choices for each player. If only one player is active, it's the same for
	// both.
	const int P1Choice = iCurrentRow;

	std::vector<Row>& Rows = m_Rows;

	const bool BothPlayersActivated = GAMESTATE->IsHumanPlayer(PLAYER_1);
	if (!BothPlayersActivated) {
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
		if (sum >= static_cast<int>(Rows.size()) || sum >= total)
			break; // nothing more to display, or no room

		/* First priority: expand the top of the second half until it meets
		 * the first half. */
		if (second_start > first_end)
			second_start--;
		// Otherwise, expand either end.
		else if (first_start > 0)
			first_start--;
		else if (second_end < static_cast<int>(Rows.size()))
			second_end++;
		else
			FAIL_M("Do we have room to grow, or don't we?");
	}

	int pos = 0;
	for (int i = 0; i < static_cast<int>(Rows.size()); i++) // foreach row
	{
		float ItemPosition;
		if (i < first_start)
			ItemPosition = -0.5f;
		else if (i < first_end)
			ItemPosition = static_cast<float>(pos++);
		else if (i < second_start)
			ItemPosition = halfsize - 0.5f;
		else if (i < second_end)
			ItemPosition = static_cast<float>(pos++);
		else
			ItemPosition = static_cast<float>(total) - 0.5f;

		Row& row = Rows[i];

		const float fY = ITEMS_SPACING_Y * ItemPosition;
		row.m_fY = fY;
		row.m_bHidden = i < first_start ||
						(i >= first_end && i < second_start) || i >= second_end;
	}
}

void
StepsDisplayList::PositionItems()
{
	for (size_t i = 0; i < m_Lines.lines.size(); ++i) {
		const bool bUnused = i >= m_Rows.size();
		m_Lines.lines[i]->SetVisible(!bUnused);
	}

	for (int m = 0; m < static_cast<int>(m_Rows.size()); ++m) {
		Row& row = m_Rows[m];
		bool bHidden = row.m_bHidden;
		if (!m_bShown)
			bHidden = true;

		const float fDiffuseAlpha = bHidden ? 0.0f : 1.0f;
		if (m_Lines.lines[m]->GetDestY() != row.m_fY ||
			m_Lines.lines[m]->DestTweenState().diffuse[0][3] !=
			  fDiffuseAlpha) {
			m_Lines.lines[m]->RunCommands(MOVE_COMMAND.GetValue());
			m_Lines.lines[m]->RunCommandsOnChildren(MOVE_COMMAND.GetValue());
		}
		m_Lines.lines[m]->mypos = m;
		m_Lines.lines[m]->SetY(row.m_fY);
	}

	for (size_t m = 0; m < m_Lines.lines.size(); ++m) {
		bool bHidden = true;
		if (m_bShown && m < m_Rows.size())
			bHidden = m_Rows[m].m_bHidden;

		const float fDiffuseAlpha = bHidden ? 0.0f : 1.0f;

		m_Lines.lines[m]->SetDiffuseAlpha(fDiffuseAlpha);
	}

	const int iCurrentRow = GetCurrentRowIndex(PLAYER_1);

	float fY = 0;
	if (iCurrentRow < static_cast<int>(m_Rows.size()))
		fY = m_Rows[iCurrentRow].m_fY;

	m_CursorFrames.PlayCommand("Change");
	m_CursorFrames.SetY(fY);
}

void
StepsDisplayList::SetFromGameState()
{
	const Song* pSong = GAMESTATE->m_pCurSong;
	unsigned i = 0;

	if (pSong == nullptr) {
		// FIXME: This clamps to between the min and the max difficulty, but
		// it really should round to the nearest difficulty that's in
		// DIFFICULTIES_TO_SHOW.
		const std::vector<Difficulty>& difficulties =
		  CommonMetrics::DIFFICULTIES_TO_SHOW.GetValue();
		m_Rows.resize(difficulties.size());

		// m_Lines.lines needs to be resized to allow the number of rows used
		if (m_Rows.size() > m_Lines.lines.size()) {
			auto startingIndex = m_Lines.lines.size();
			m_Lines.lines.resize(m_Rows.size());

			for (auto j = startingIndex; j < m_Lines.lines.size(); j++) {
				auto& m_Line = m_Lines.lines[j];
				m_Line = std::make_unique<StepsDisplay>();
				m_Line->SetName("Row");
				m_Line->Load("StepsDisplayListRow", nullptr);
				this->AddChild(m_Line.get());
			}
			SortByDrawOrder();
		}
		
		FOREACH_CONST(Difficulty, difficulties, d)
		{
			m_Rows[i].m_dc = *d;
			m_Lines.lines[i]->SetFromStepsTypeAndMeterAndDifficultyAndCourseType(
				GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType, 0, *d);
			++i;
		}
	} else {
		std::vector<Steps*> vpSteps;
		SongUtil::GetPlayableSteps(pSong, vpSteps);
		// Should match the sort in ScreenSelectMusic::AfterMusicChange.

		m_Rows.resize(vpSteps.size());

		// m_Lines.lines needs to be resized to allow the number of rows used
		if (m_Rows.size() > m_Lines.lines.size()) {
			auto startingIndex = m_Lines.lines.size();
			m_Lines.lines.resize(m_Rows.size());

			for (auto j = startingIndex; j < m_Lines.lines.size(); j++) {
				auto& m_Line = m_Lines.lines[j];
				m_Line = std::make_unique<StepsDisplay>();
				m_Line->SetName("Row");
				m_Line->Load("StepsDisplayListRow", nullptr);
				this->AddChild(m_Line.get());
			}
			SortByDrawOrder();
		}

		FOREACH_CONST(Steps*, vpSteps, s)
		{
			// LOG->Trace(ssprintf("setting steps for row %i",i));
			m_Rows[i].m_Steps = *s;
			m_Lines.lines[i]->SetFromSteps(*s);
			++i;
		}
	}

	for (auto& m_Line : m_Lines.lines)
		m_Line->Unset();

	UpdatePositions();
	PositionItems();

	for (auto& m_Line : m_Lines.lines)
		m_Line->FinishTweening();
}

void
StepsDisplayList::HideRows()
{
	for (unsigned m = 0; m < m_Rows.size(); ++m) {
		auto& l = m_Lines.lines[m];

		l->FinishTweening();
		l->SetDiffuseAlpha(0);
	}
}

void
StepsDisplayList::TweenOnScreen()
{
	ON_COMMAND(m_Cursors);

	for (auto& m_Line : m_Lines.lines)
		ON_COMMAND(m_Line.get());

	m_bShown = true;
	for (unsigned m = 0; m < m_Rows.size(); ++m) {
		auto& l = m_Lines.lines[m];

		l->FinishTweening();
	}

	HideRows();
	PositionItems();

	COMMAND(m_Cursors, "TweenOn");
}

void
StepsDisplayList::TweenOffScreen()
{
}

void
StepsDisplayList::Show()
{
	m_bShown = true;

	SetFromGameState();

	HideRows();
	PositionItems();

	COMMAND(m_Cursors, "Show");
}

void
StepsDisplayList::Hide()
{
	m_bShown = false;
	PositionItems();

	COMMAND(m_Cursors, "Hide");
}

void
StepsDisplayList::HandleMessage(const Message& msg)
{
	if (msg.GetName() == MessageIDToString(static_cast<MessageID>(
						   Message_CurrentStepsChanged + PLAYER_1)))
		SetFromGameState();

	ActorFrame::HandleMessage(msg);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the StepsDisplayList. */
class LunaStepsDisplayList : public Luna<StepsDisplayList>
{
  public:
	static int setfromgamestate(T* p, lua_State* L)
	{
		p->SetFromGameState();
		COMMON_RETURN_SELF;
	}
	static int GetCurrentIndex(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetCurrentRowIndex(PLAYER_1));
		return 1;
	}

	LunaStepsDisplayList()
	{
		ADD_METHOD(setfromgamestate);
		ADD_METHOD(GetCurrentIndex);
	}
};

LUA_REGISTER_DERIVED_CLASS(StepsDisplayList, ActorFrame)
// lua end
