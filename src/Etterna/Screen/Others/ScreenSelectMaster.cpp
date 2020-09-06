#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Singletons/AnnouncerManager.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Models/Misc/GameCommand.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenSelectMaster.h"
#include "Etterna/Singletons/ThemeManager.h"

#include <algorithm>
#include <set>

static const char* MenuDirNames[] = {
	"Up", "Down", "Left", "Right", "Auto",
};
XToString(MenuDir);

AutoScreenMessage(SM_PlayPostSwitchPage);

static std::string
CURSOR_OFFSET_X_FROM_ICON_NAME(size_t p)
{
	return ssprintf("CursorP%dOffsetXFromIcon", static_cast<int>(p + 1));
}
static std::string
CURSOR_OFFSET_Y_FROM_ICON_NAME(size_t p)
{
	return ssprintf("CursorP%dOffsetYFromIcon", static_cast<int>(p + 1));
}
// e.g. "OptionOrderLeft=0:1,1:2,2:3,3:4"
static std::string
OPTION_ORDER_NAME(size_t dir)
{
	return "OptionOrder" + MenuDirToString(static_cast<MenuDir>(dir));
}

REGISTER_SCREEN_CLASS(ScreenSelectMaster);

#define GetActiveElementPlayerNumbers(vpns)                                    \
                                                                               \
	if (SHARED_SELECTION) {                                                    \
		(vpns).push_back(PLAYER_1);                                            \
                                                                               \
	} else {                                                                   \
		(vpns).push_back(PLAYER_1);                                            \
	}

ScreenSelectMaster::ScreenSelectMaster()
{
	m_iChoice = 0;
	m_bChosen = false;
	m_bDoubleChoice = false;
	m_bDoubleChoiceNoSound = false;
	m_TrackingRepeatingInput = GameButton_Invalid;
}

void
ScreenSelectMaster::Init()
{
	SHOW_ICON.Load(m_sName, "ShowIcon");
	SHOW_SCROLLER.Load(m_sName, "ShowScroller");
	SHOW_CURSOR.Load(m_sName, "ShowCursor");
	SHARED_SELECTION.Load(m_sName, "SharedSelection");
	USE_ICON_METRICS.Load(m_sName, "UseIconMetrics");
	NUM_CHOICES_ON_PAGE_1.Load(m_sName, "NumChoicesOnPage1");
	CURSOR_OFFSET_X_FROM_ICON.Load(
	  m_sName, CURSOR_OFFSET_X_FROM_ICON_NAME, NUM_PLAYERS);
	CURSOR_OFFSET_Y_FROM_ICON.Load(
	  m_sName, CURSOR_OFFSET_Y_FROM_ICON_NAME, NUM_PLAYERS);
	PER_CHOICE_ICON_ELEMENT.Load(m_sName, "PerChoiceIconElement");
	PRE_SWITCH_PAGE_SECONDS.Load(m_sName, "PreSwitchPageSeconds");
	POST_SWITCH_PAGE_SECONDS.Load(m_sName, "PostSwitchPageSeconds");
	OPTION_ORDER.Load(m_sName, OPTION_ORDER_NAME, NUM_MenuDir);
	DO_SWITCH_ANYWAYS.Load(m_sName, "DoSwitchAnyways");

	WRAP_CURSOR.Load(m_sName, "WrapCursor");
	WRAP_SCROLLER.Load(m_sName, "WrapScroller");
	LOOP_SCROLLER.Load(m_sName, "LoopScroller");
	PER_CHOICE_SCROLL_ELEMENT.Load(m_sName, "PerChoiceScrollElement");
	ALLOW_REPEATING_INPUT.Load(m_sName, "AllowRepeatingInput");
	SCROLLER_SECONDS_PER_ITEM.Load(m_sName, "ScrollerSecondsPerItem");
	SCROLLER_NUM_ITEMS_TO_DRAW.Load(m_sName, "ScrollerNumItemsToDraw");
	SCROLLER_TRANSFORM.Load(m_sName, "ScrollerTransform");
	// SCROLLER_TWEEN.Load( m_sName, "ScrollerTween" );
	SCROLLER_SUBDIVISIONS.Load(m_sName, "ScrollerSubdivisions");
	DEFAULT_CHOICE.Load(m_sName, "DefaultChoice");
	DOUBLE_PRESS_TO_SELECT.Load(m_sName, "DoublePressToSelect");

	ScreenSelect::Init();

	m_TrackingRepeatingInput = GameButton_Invalid;

	vector<PlayerNumber> vpns;
	GetActiveElementPlayerNumbers(vpns);

#define PLAYER_APPEND_NO_SPACE(p)                                              \
	(SHARED_SELECTION ? std::string() : ssprintf("P%d", (p) + 1))
	this->SubscribeToMessage(SM_MenuTimer);

	// init cursor
	if (SHOW_CURSOR) {
		FOREACH(PlayerNumber, vpns, p)
		{
			std::string sElement = "Cursor" + PLAYER_APPEND_NO_SPACE(*p);
			m_sprCursor.Load(THEME->GetPathG(m_sName, sElement));
			s_replace(sElement, " ", "");
			m_sprCursor->SetName(sElement);
			this->AddChild(m_sprCursor);
			LOAD_ALL_COMMANDS(m_sprCursor);
		}
	}

	// Resize vectors depending on how many choices there are
	m_vsprIcon.resize(m_aGameCommands.size());
	m_vsprScroll.resize(m_aGameCommands.size());

	vector<RageVector3> positions;
	bool positions_set_by_lua = false;
	if (THEME->HasMetric(m_sName, "IconChoicePosFunction")) {
		positions_set_by_lua = true;
		LuaReference command =
		  THEME->GetMetricR(m_sName, "IconChoicePosFunction");
		if (command.GetLuaType() != LUA_TFUNCTION) {
			LuaHelpers::ReportScriptError(
			  m_sName + "::IconChoicePosFunction must be a function.");
			positions_set_by_lua = false;
		} else {
			Lua* L = LUA->Get();
			command.PushSelf(L);
			lua_pushnumber(L, m_aGameCommands.size());
			std::string err = m_sName + "::IconChoicePosFunction: ";
			if (!LuaHelpers::RunScriptOnStack(L, err, 1, 1, true)) {
				positions_set_by_lua = false;
			} else {
				if (!lua_istable(L, -1)) {
					LuaHelpers::ReportScriptError(
					  m_sName + "::IconChoicePosFunction did not return a "
								"table of positions.");
					positions_set_by_lua = false;
				} else {
					size_t poses = lua_objlen(L, -1);
					for (size_t p = 1; p <= poses; ++p) {
						lua_rawgeti(L, -1, p);
						RageVector3 pos(0.0f, 0.0f, 0.0f);
						if (!lua_istable(L, -1)) {
							LuaHelpers::ReportScriptErrorFmt(
							  "Position %zu is not a table.", p);
						} else {
#define SET_POS_PART(i, part)                                                  \
	lua_rawgeti(L, -1, i);                                                     \
	pos.part = static_cast<float>(lua_tonumber(L, -1));                        \
	lua_pop(L, 1);
							// If part of the position is not provided, we want
							// it to default to zero, which lua_tonumber does.
							// -Kyz
							SET_POS_PART(1, x);
							SET_POS_PART(2, y);
							SET_POS_PART(3, z);
#undef SET_POS_PART
						}
						lua_pop(L, 1);
						positions.push_back(pos);
					}
				}
			}
			lua_settop(L, 0);
			LUA->Release(L);
		}
	}

	for (unsigned c = 0; c < m_aGameCommands.size(); c++) {
		GameCommand& mc = m_aGameCommands[c];

		LuaThreadVariable var("GameCommand", LuaReference::Create(&mc));

		// init icon
		if (SHOW_ICON) {
			vector<std::string> vs;
			vs.push_back("Icon");
			if (PER_CHOICE_ICON_ELEMENT)
				vs.push_back("Choice" + mc.m_sName);
			std::string sElement = join(" ", vs);
			m_vsprIcon[c].Load(THEME->GetPathG(m_sName, sElement));
			std::string sName = "Icon"
								"Choice" +
								mc.m_sName;
			m_vsprIcon[c]->SetName(sName);
			if (USE_ICON_METRICS) {
				if (positions_set_by_lua) {
					LOAD_ALL_COMMANDS(m_vsprIcon[c]);
					m_vsprIcon[c]->SetXY(positions[c].x, positions[c].y);
					m_vsprIcon[c]->SetZ(positions[c].z);
				} else {
					LOAD_ALL_COMMANDS_AND_SET_XY(m_vsprIcon[c]);
				}
#define OPTIONAL_COMMAND(onoff)                                                \
	if (THEME->HasMetric(m_sName, "IconChoice" onoff "Command")) {             \
		m_vsprIcon[c]->AddCommand(                                             \
		  onoff,                                                               \
		  THEME->GetMetricA(m_sName, "IconChoice" onoff "Command"),            \
		  false);                                                              \
	}
				OPTIONAL_COMMAND("On");
				OPTIONAL_COMMAND("Off");
#undef OPTIONAL_COMMAND
			}
			this->AddChild(m_vsprIcon[c]);
		}

		// init scroll
		if (SHOW_SCROLLER) {
			FOREACH(PlayerNumber, vpns, p)
			{
				vector<std::string> vs;
				vs.push_back("Scroll");
				if (PER_CHOICE_SCROLL_ELEMENT)
					vs.push_back("Choice" + mc.m_sName);
				if (!SHARED_SELECTION)
					vs.push_back(PLAYER_APPEND_NO_SPACE(*p));
				std::string sElement = join(" ", vs);
				m_vsprScroll[c].Load(THEME->GetPathG(m_sName, sElement));
				std::string sName = "Scroll"
									"Choice" +
									mc.m_sName;
				if (!SHARED_SELECTION)
					sName += PLAYER_APPEND_NO_SPACE(*p);
				m_vsprScroll[c]->SetName(sName);
				m_Scroller.AddChild(m_vsprScroll[c]);
			}
		}
	}

	// init scroll
	if (SHOW_SCROLLER) {
		FOREACH(PlayerNumber, vpns, p)
		{
			m_Scroller.SetLoop(LOOP_SCROLLER);
			m_Scroller.SetNumItemsToDraw(SCROLLER_NUM_ITEMS_TO_DRAW);
			m_Scroller.Load2();
			m_Scroller.SetTransformFromReference(SCROLLER_TRANSFORM);
			m_Scroller.SetSecondsPerItem(SCROLLER_SECONDS_PER_ITEM);
			m_Scroller.SetNumSubdivisions(SCROLLER_SUBDIVISIONS);
			m_Scroller.SetName("Scroller" + PLAYER_APPEND_NO_SPACE(*p));
			LOAD_ALL_COMMANDS_AND_SET_XY(m_Scroller);
			this->AddChild(&m_Scroller);
		}
	}

	FOREACH_ENUM(Page, page)
	{
		m_sprMore[page].Load(
		  THEME->GetPathG(m_sName, ssprintf("more page%d", page + 1)));
		m_sprMore[page]->SetName(ssprintf("MorePage%d", page + 1));
		LOAD_ALL_COMMANDS_AND_SET_XY(m_sprMore[page]);
		this->AddChild(m_sprMore[page]);

		m_sprExplanation[page].Load(
		  THEME->GetPathG(m_sName, ssprintf("explanation page%d", page + 1)));
		m_sprExplanation[page]->SetName(
		  ssprintf("ExplanationPage%d", page + 1));
		LOAD_ALL_COMMANDS_AND_SET_XY(m_sprExplanation[page]);
		this->AddChild(m_sprExplanation[page]);
	}

	m_soundChange.Load(THEME->GetPathS(m_sName, "change"), true);
	m_soundDifficult.Load(ANNOUNCER->GetPathTo("select difficulty challenge"));
	m_soundStart.Load(THEME->GetPathS(m_sName, "start"));

	// init m_Next order info
	FOREACH_MenuDir(dir)
	{
		const std::string order = OPTION_ORDER.GetValue(dir);
		vector<std::string> parts;
		split(order, ",", parts, true);

		for (unsigned part = 0; part < parts.size(); ++part) {
			int from, to;
			if (sscanf(parts[part].c_str(), "%d:%d", &from, &to) != 2) {
				LuaHelpers::ReportScriptErrorFmt(
				  "%s::OptionOrder%s parse error",
				  m_sName.c_str(),
				  MenuDirToString(dir).c_str());
				continue;
			}

			--from;
			--to;

			m_mapCurrentChoiceToNextChoice[dir][from] = to;
		}

		if (m_mapCurrentChoiceToNextChoice[dir]
			  .empty()) // Didn't specify any mappings
		{
			// Fill with reasonable defaults
			for (unsigned c = 0; c < m_aGameCommands.size(); ++c) {
				int add;
				switch (dir) {
					case MenuDir_Up:
					case MenuDir_Left:
						add = -1;
						break;
					default:
						add = +1;
						break;
				}

				m_mapCurrentChoiceToNextChoice[dir][c] = c + add;
				// Always wrap around MenuDir_Auto.
				if (dir == MenuDir_Auto || static_cast<bool>(WRAP_CURSOR))
					wrap(m_mapCurrentChoiceToNextChoice[dir][c],
						 m_aGameCommands.size());
				else
					m_mapCurrentChoiceToNextChoice[dir][c] =
					  std::clamp(m_mapCurrentChoiceToNextChoice[dir][c],
								 0,
								 static_cast<int>(m_aGameCommands.size()) - 1);
			}
		}
	}

	m_bDoubleChoiceNoSound = false;
}

std::string
ScreenSelectMaster::GetDefaultChoice()
{
	return DEFAULT_CHOICE.GetValue();
}

void
ScreenSelectMaster::BeginScreen()
{
	// TODO: Move default choice to ScreenSelect
	int iDefaultChoice = -1;
	for (unsigned c = 0; c < m_aGameCommands.size(); c++) {
		const GameCommand& mc = m_aGameCommands[c];
		if (mc.m_sName == static_cast<std::string>(DEFAULT_CHOICE)) {
			iDefaultChoice = c;
			break;
		}
	}

	m_iChoice = (iDefaultChoice != -1) ? iDefaultChoice : 0;
	CLAMP(m_iChoice, 0, static_cast<int>(m_aGameCommands.size()) - 1);
	m_bChosen = false;
	m_bDoubleChoice = false;

	if (!SHARED_SELECTION) {
		if (GAMESTATE->IsHumanPlayer(PLAYER_1)) {
		} else {
			if (SHOW_CURSOR) {
				if (m_sprCursor)
					m_sprCursor->SetVisible(false);
			}
			if (SHOW_SCROLLER)
				m_Scroller.SetVisible(false);
		}
	}

	this->UpdateSelectableChoices();

	ScreenSelect::BeginScreen();

	// Call GetTweenTimeLeft after the base BeginScreen has started the in
	// Transition.
	m_fLockInputSecs = this->GetTweenTimeLeft();
}

void
ScreenSelectMaster::HandleScreenMessage(const ScreenMessage& SM)
{
	ScreenSelect::HandleScreenMessage(SM);

	vector<PlayerNumber> vpns;
	GetActiveElementPlayerNumbers(vpns);

	if (SM == SM_PlayPostSwitchPage) {
		int iNewChoice = m_iChoice;
		Page newPage = GetPage(iNewChoice);

		Message msg("PostSwitchPage");
		msg.SetParam("NewPageIndex", static_cast<int>(newPage));

		if (SHOW_CURSOR) {
			FOREACH(PlayerNumber, vpns, p)
			m_sprCursor->HandleMessage(msg);
		}

		if (SHOW_SCROLLER) {
			FOREACH(PlayerNumber, vpns, p)
			{
				int iChoice = m_iChoice;
				m_vsprScroll[iChoice]->HandleMessage(msg);
			}
		}
		MESSAGEMAN->Broadcast(msg);

		m_fLockInputSecs = POST_SWITCH_PAGE_SECONDS;
	} else if (SM == SM_MenuTimer) {
		if (DOUBLE_PRESS_TO_SELECT) {
			m_bDoubleChoiceNoSound = true;
			m_bDoubleChoice = true;
			InputEventPlus iep;
			iep.pn = PLAYER_1;
			MenuStart(iep);
		}
	}
}

void
ScreenSelectMaster::HandleMessage(const Message& msg)
{
	if (msg == Message_PlayerJoined) {
		UpdateSelectableChoices();
	}

	ScreenSelect::HandleMessage(msg);
}

int
ScreenSelectMaster::GetSelectionIndex(PlayerNumber pn)
{
	return m_iChoice;
}

void
ScreenSelectMaster::UpdateSelectableChoices()
{
	vector<PlayerNumber> vpns;
	GetActiveElementPlayerNumbers(vpns);
	int first_playable = -1;
	bool on_unplayable;
	on_unplayable = false;

	for (unsigned c = 0; c < m_aGameCommands.size(); c++) {
		std::string command = "Enabled";
		bool disabled = false;
		if (!m_aGameCommands[c].IsPlayable()) {
			command = "Disabled";
			disabled = true;
		} else if (first_playable == -1) {
			first_playable = c;
		}
		if (SHOW_ICON) {
			m_vsprIcon[c]->PlayCommand(command);
		}

		FOREACH(PlayerNumber, vpns, p)
		{
			if (disabled && m_iChoice == c) {
				on_unplayable = true;
			}
			if (m_vsprScroll[c].IsLoaded()) {
				m_vsprScroll[c]->PlayCommand(command);
			}
		}
	}
	FOREACH(PlayerNumber, vpns, pn)
	{
		if (on_unplayable && first_playable != -1) {
			ChangeSelection(*pn,
							first_playable < m_iChoice ? MenuDir_Left
													   : MenuDir_Right,
							first_playable);
		}
	}

	/* If no options are playable at all, just wait.  Some external
	 * stimulus may make options available (such as coin insertion).
	 * If any options are playable, make sure one is selected. */
	if (!m_aGameCommands.empty() && !m_aGameCommands[m_iChoice].IsPlayable())
		Move(PLAYER_1, MenuDir_Auto);
}

bool
ScreenSelectMaster::AnyOptionsArePlayable() const
{
	for (unsigned i = 0; i < m_aGameCommands.size(); ++i)
		if (m_aGameCommands[i].IsPlayable())
			return true;

	return false;
}

bool
ScreenSelectMaster::Move(PlayerNumber pn, MenuDir dir)
{
	if (!AnyOptionsArePlayable())
		return false;

	int iSwitchToIndex = m_iChoice;
	std::set<int> seen;

	do {
		std::map<int, int>::const_iterator iter =
		  m_mapCurrentChoiceToNextChoice[dir].find(iSwitchToIndex);
		if (iter != m_mapCurrentChoiceToNextChoice[dir].end())
			iSwitchToIndex = iter->second;

		if (iSwitchToIndex < 0 ||
			iSwitchToIndex >=
			  static_cast<int>(m_aGameCommands.size())) // out of choice range
			return false;								// can't go that way
		if (seen.find(iSwitchToIndex) != seen.end())
			return false; // went full circle and none found
		seen.insert(iSwitchToIndex);
	} while (!m_aGameCommands.empty() &&
			 !m_aGameCommands[iSwitchToIndex].IsPlayable() &&
			 !DO_SWITCH_ANYWAYS);

	return ChangeSelection(pn, dir, iSwitchToIndex);
}

bool
ScreenSelectMaster::MenuLeft(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;
	if (m_fLockInputSecs > 0 || m_bChosen)
		return false;
	if (input.type == IET_RELEASE)
		return false;
	if (input.type != IET_FIRST_PRESS) {
		if (!ALLOW_REPEATING_INPUT)
			return false;
		if (m_TrackingRepeatingInput != input.MenuI)
			return false;
	}
	if (Move(pn, MenuDir_Left)) {
		m_TrackingRepeatingInput = input.MenuI;
		m_soundChange.Play(true);
		MESSAGEMAN->Broadcast((Message_MenuSelectionChanged));
		MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuLeftP1 + pn));

		// if they use double select
		if (DOUBLE_PRESS_TO_SELECT) {
			m_bDoubleChoice = false; // player has cancelled their selection
		}
		return true;
	}
	return false;
}

bool
ScreenSelectMaster::MenuRight(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;
	if (m_fLockInputSecs > 0 || m_bChosen)
		return false;
	if (input.type == IET_RELEASE)
		return false;
	if (input.type != IET_FIRST_PRESS) {
		if (!ALLOW_REPEATING_INPUT)
			return false;
		if (m_TrackingRepeatingInput != input.MenuI)
			return false;
	}
	if (Move(pn, MenuDir_Right)) {
		m_TrackingRepeatingInput = input.MenuI;
		m_soundChange.Play(true);
		MESSAGEMAN->Broadcast((Message_MenuSelectionChanged));
		MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuRightP1 + pn));

		// if they use double select
		if (DOUBLE_PRESS_TO_SELECT) {
			m_bDoubleChoice = false; // player has cancelled their selection
		}
		return true;
	}
	return false;
}

bool
ScreenSelectMaster::MenuUp(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;
	if (m_fLockInputSecs > 0 || m_bChosen)
		return false;
	if (input.type == IET_RELEASE)
		return false;
	if (input.type != IET_FIRST_PRESS) {
		if (!ALLOW_REPEATING_INPUT)
			return false;
		if (m_TrackingRepeatingInput != input.MenuI)
			return false;
	}
	if (Move(pn, MenuDir_Up)) {
		m_TrackingRepeatingInput = input.MenuI;
		m_soundChange.Play(true);
		MESSAGEMAN->Broadcast((Message_MenuSelectionChanged));
		MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuUpP1 + pn));

		// if they use double select
		if (DOUBLE_PRESS_TO_SELECT) {
			m_bDoubleChoice = false; // player has cancelled their selection
		}
		return true;
	}
	return false;
}

bool
ScreenSelectMaster::MenuDown(const InputEventPlus& input)
{
	PlayerNumber pn = input.pn;
	if (m_fLockInputSecs > 0 || m_bChosen)
		return false;
	if (input.type == IET_RELEASE)
		return false;
	if (input.type != IET_FIRST_PRESS) {
		if (!ALLOW_REPEATING_INPUT)
			return false;
		if (m_TrackingRepeatingInput != input.MenuI)
			return false;
	}
	if (Move(pn, MenuDir_Down)) {
		m_TrackingRepeatingInput = input.MenuI;
		m_soundChange.Play(true);
		MESSAGEMAN->Broadcast((Message_MenuSelectionChanged));
		MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuDownP1 + pn));

		// if they use double select
		if (DOUBLE_PRESS_TO_SELECT) {
			m_bDoubleChoice = false; // player has cancelled their selection
		}
		return true;
	}
	return false;
}

bool
ScreenSelectMaster::ChangePage(int iNewChoice)
{
	Page oldPage = GetPage(m_iChoice);
	Page newPage = GetPage(iNewChoice);

	// If anyone has already chosen, don't allow changing of pages
	if (GAMESTATE->IsHumanPlayer(PLAYER_1) && m_bChosen)
		return false;

	// change both players
	m_iChoice = iNewChoice;

	const std::string sIconAndExplanationCommand =
	  ssprintf("SwitchToPage%d", newPage + 1);
	if (SHOW_ICON)
		for (unsigned c = 0; c < m_aGameCommands.size(); ++c)
			m_vsprIcon[c]->PlayCommand(sIconAndExplanationCommand);

	FOREACH_ENUM(Page, page)
	{
		m_sprExplanation[page]->PlayCommand(sIconAndExplanationCommand);
		m_sprMore[page]->PlayCommand(sIconAndExplanationCommand);
	}

	vector<PlayerNumber> vpns;
	GetActiveElementPlayerNumbers(vpns);

	Message msg("PreSwitchPage");
	msg.SetParam("OldPageIndex", static_cast<int>(oldPage));
	msg.SetParam("NewPageIndex", static_cast<int>(newPage));
	MESSAGEMAN->Broadcast(msg);

	FOREACH(PlayerNumber, vpns, p)
	{
		if (GAMESTATE->IsHumanPlayer(*p)) {
			if (SHOW_CURSOR) {
				m_sprCursor->HandleMessage(msg);
				m_sprCursor->SetXY(GetCursorX(*p), GetCursorY(*p));
			}

			if (SHOW_SCROLLER)
				m_vsprScroll[m_iChoice]->HandleMessage(msg);
		}
	}

	if (newPage == PAGE_2) {
		// XXX: only play this once (I thought we already did that?)
		// Play it on every change to page 2.  -Chris
		// That sounds ugly if you go back and forth quickly. -g
		// Should we lock input while it's scrolling? -Chris
		m_soundDifficult.Stop();
		m_soundDifficult.PlayRandom();
	}

	m_fLockInputSecs = PRE_SWITCH_PAGE_SECONDS;
	this->PostScreenMessage(SM_PlayPostSwitchPage, GetTweenTimeLeft());
	return true;
}

bool
ScreenSelectMaster::ChangeSelection(PlayerNumber pn,
									MenuDir dir,
									int iNewChoice)
{
	if (iNewChoice == m_iChoice)
		return false; // already there

	Page page = GetPage(iNewChoice);
	if (GetPage(m_iChoice) != page) {
		return ChangePage(iNewChoice);
	}

	vector<PlayerNumber> vpns;
	if (SHARED_SELECTION || page != PAGE_1) {
		/* Set the new m_iChoice even for disabled players, since a player might
		 * join on a SHARED_SELECTION after the cursor has been moved. */
		vpns.push_back(PLAYER_1);
	} else {
		vpns.push_back(pn);
	}

	FOREACH(PlayerNumber, vpns, p)
	{
		const int iOldChoice = m_iChoice;
		m_iChoice = iNewChoice;

		if (SHOW_ICON) {
			bool bOldStillHasFocus = false;
			bOldStillHasFocus |= m_iChoice == iOldChoice;

			if (DOUBLE_PRESS_TO_SELECT) {
				// this player is currently on a single press, which they are
				// cancelling
				if (m_bDoubleChoice) {
					if (!bOldStillHasFocus)
						m_vsprIcon[iOldChoice]->PlayCommand(
						  "LostSelectedLoseFocus");
					m_vsprIcon[iNewChoice]->PlayCommand(
					  "LostSelectedGainFocus");
				} else {
					if (!bOldStillHasFocus)
						m_vsprIcon[iOldChoice]->PlayCommand("LoseFocus");
					m_vsprIcon[iNewChoice]->PlayCommand("GainFocus");
				}
			} else // not using double selection
			{
				if (!bOldStillHasFocus)
					m_vsprIcon[iOldChoice]->PlayCommand("LoseFocus");
				m_vsprIcon[iNewChoice]->PlayCommand("GainFocus");
			}
		}

		if (SHOW_CURSOR) {
			if (GAMESTATE->IsHumanPlayer(*p)) {
				m_sprCursor->PlayCommand("Change");
				m_sprCursor->SetXY(GetCursorX(*p), GetCursorY(*p));
			}
		}

		if (SHOW_SCROLLER) {
			ActorScroller& scroller = m_Scroller;
			vector<AutoActor>& vScroll = m_vsprScroll;

			if (WRAP_SCROLLER) {
				// HACK: We can't tell from the option orders whether or not we
				// wrapped. For now, assume that the order is increasing left to
				// right.
				int iPressedDir =
				  (dir == MenuDir_Left || dir == MenuDir_Up) ? -1 : +1;
				int iActualDir = (iOldChoice < iNewChoice) ? +1 : -1;

				if (iPressedDir != iActualDir) // wrapped
				{
					float fItem = scroller.GetCurrentItem();
					int iNumChoices = m_aGameCommands.size();
					fItem += iActualDir * iNumChoices;
					scroller.SetCurrentAndDestinationItem(fItem);
				}
			}

			scroller.SetDestinationItem(static_cast<float>(iNewChoice));

			// using double selections
			if (DOUBLE_PRESS_TO_SELECT) {
				// this player is currently on a single press, which they are
				// cancelling
				if (m_bDoubleChoice) {
					vScroll[iOldChoice]->PlayCommand("LostSelectedLoseFocus");
					vScroll[iNewChoice]->PlayCommand("LostSelectedGainFocus");
				} else // the player hasn't made any selections yet
				{
					vScroll[iOldChoice]->PlayCommand("LoseFocus");
					vScroll[iNewChoice]->PlayCommand("GainFocus");
				}
			} else // regular lose/gain focus
			{
				vScroll[iOldChoice]->PlayCommand("LoseFocus");
				vScroll[iNewChoice]->PlayCommand("GainFocus");
			}
		}
	}

	return true;
}

PlayerNumber
ScreenSelectMaster::GetSharedPlayer()
{
	if (GAMESTATE->GetMasterPlayerNumber() != PLAYER_INVALID)
		return GAMESTATE->GetMasterPlayerNumber();

	return PLAYER_1;
}

ScreenSelectMaster::Page
ScreenSelectMaster::GetPage(int iChoiceIndex) const
{
	return iChoiceIndex < NUM_CHOICES_ON_PAGE_1 ? PAGE_1 : PAGE_2;
}

ScreenSelectMaster::Page
ScreenSelectMaster::GetCurrentPage() const
{
	// Both players are guaranteed to be on the same page.
	return GetPage(m_iChoice);
}

float
ScreenSelectMaster::DoMenuStart(PlayerNumber pn)
{
	if (m_bChosen)
		return 0;

	bool bAnyChosen = false;
	bAnyChosen |= m_bChosen;

	m_bChosen = true;

	this->PlayCommand("MadeChoice" + PlayerNumberToString(pn));

	bool bIsFirstToChoose = bAnyChosen;

	float fSecs = 0;

	if (bIsFirstToChoose) {
		FOREACH_ENUM(Page, page)
		{
			m_sprMore[page]->PlayCommand("Off");
			fSecs = std::max(fSecs, m_sprMore[page]->GetTweenTimeLeft());
		}
	}
	if (SHOW_CURSOR) {
		if (m_sprCursor != nullptr) {
			m_sprCursor->PlayCommand("Choose");
			fSecs = std::max(fSecs, m_sprCursor->GetTweenTimeLeft());
		}
	}

	return fSecs;
}

bool
ScreenSelectMaster::MenuStart(const InputEventPlus& input)
{
	if (input.type != IET_FIRST_PRESS)
		return false;
	PlayerNumber pn = input.pn;

	if (m_fLockInputSecs > 0)
		return false;

	// Return if player has chosen
	if (m_bChosen)
		return false;

	if (!ProcessMenuStart(pn))
		return false;

	// double press is enabled and the player hasn't made their first press
	if (DOUBLE_PRESS_TO_SELECT && !m_bDoubleChoice) {
		m_soundStart.PlayCopy(true);
		m_bDoubleChoice = true;

		if (SHOW_SCROLLER) {
			vector<AutoActor>& vScroll = m_vsprScroll;
			vScroll[m_iChoice]->PlayCommand("InitialSelection");
		}

		return true;
	}

	GameCommand empty_mc;
	// This is so we can avoid having problems when the GameCommands in the
	// choices were all invalid or didn't load or similar. -Kyz
	GameCommand* mc = &empty_mc;
	if (!m_aGameCommands.empty()) {
		mc = &(m_aGameCommands[m_iChoice]);
	}

	/* If no options are playable, then we're just waiting for one to become
	 * available. If any options are playable, then the selection must be
	 * playable. */
	if (!AnyOptionsArePlayable())
		return false;

	SOUND->PlayOnceFromDir(ANNOUNCER->GetPathTo(
	  ssprintf("%s comment %s", m_sName.c_str(), mc->m_sName.c_str())));

	// Play a copy of the sound, so it'll finish playing even if we leave the
	// screen immediately.
	if (mc->m_sSoundPath.empty() && !m_bDoubleChoiceNoSound)
		m_soundStart.PlayCopy(true);

	if (mc->m_sScreen.empty()) {
		mc->ApplyToAllPlayers();
		// We want to be able to broadcast a Start message to the theme, in
		// case a themer wants to handle something. -aj
		Message msg(
		  MessageIDToString(static_cast<MessageID>(Message_MenuStartP1 + pn)));
		msg.SetParam("ScreenEmpty", true);
		MESSAGEMAN->Broadcast(msg);
		return true;
	}

	float fSecs = 0;
	bool bAllDone = true;
	if (static_cast<bool>(SHARED_SELECTION) || GetCurrentPage() == PAGE_2) {
		// Only one player has to pick. Choose this for all the other players,
		// too.
		ASSERT(!m_bChosen);
		fSecs = std::max(fSecs, DoMenuStart(PLAYER_1));
	} else {
		fSecs = std::max(fSecs, DoMenuStart(pn));
		// check to see if everyone has chosen
		bAllDone &= m_bChosen;
	}

	if (bAllDone) {
		// broadcast MenuStart just like MenuLeft/Right/etc.
		MESSAGEMAN->Broadcast(static_cast<MessageID>(Message_MenuStartP1 + pn));
		this->PostScreenMessage(SM_BeginFadingOut,
								fSecs); // tell our owner it's time to move on
	}
	return true;
}

/* We want all items to always run OnCommand and either GainFocus or LoseFocus
 * on tween-in. If we only run OnCommand, then it has to contain a copy of
 * either GainFocus or LoseFocus, which implies that the default setting is
 * hard-coded in the theme. Always run both.
 * However, the actual tween-in is OnCommand; we don't always want to actually
 * run through the Gain/LoseFocus tweens during initial tween-in. So, we run
 * the focus command first, do a FinishTweening to pop it in place, and then
 * run OnCommand. This means that the focus command should be position neutral;
 * eg. only use "addx", not "x". */
void
ScreenSelectMaster::TweenOnScreen()
{
	vector<PlayerNumber> vpns;
	GetActiveElementPlayerNumbers(vpns);

	if (SHOW_ICON) {
		for (unsigned c = 0; c < m_aGameCommands.size(); c++) {
			m_vsprIcon[c]->PlayCommand(
			  (static_cast<int>(c) == m_iChoice) ? "GainFocus" : "LoseFocus");
			m_vsprIcon[c]->FinishTweening();
		}
	}

	if (SHOW_SCROLLER) {
		FOREACH(PlayerNumber, vpns, p)
		{
			// Play Gain/LoseFocus before playing the on command.
			// Gain/Lose will often stop tweening, which ruins the OnCommand.
			for (unsigned c = 0; c < m_aGameCommands.size(); c++) {
				m_vsprScroll[c]->PlayCommand(
				  static_cast<int>(c) == m_iChoice ? "GainFocus" : "LoseFocus");
				m_vsprScroll[c]->FinishTweening();
			}

			m_Scroller.SetCurrentAndDestinationItem(
			  static_cast<float>(m_iChoice));
		}
	}

	// Need to SetXY of Cursor after Icons since it depends on the Icons'
	// positions.
	if (SHOW_CURSOR) {
		FOREACH(PlayerNumber, vpns, p)
		{
			if (GAMESTATE->IsHumanPlayer(*p))
				m_sprCursor->SetXY(GetCursorX(*p), GetCursorY(*p));
		}
	}

	ScreenSelect::TweenOnScreen();
}

void
ScreenSelectMaster::TweenOffScreen()
{
	ScreenSelect::TweenOffScreen();

	vector<PlayerNumber> vpns;
	GetActiveElementPlayerNumbers(vpns);

	for (unsigned c = 0; c < m_aGameCommands.size(); c++) {
		if (GetPage(c) != GetCurrentPage())
			continue; // skip

		bool bSelectedByEitherPlayer = false;
		FOREACH(PlayerNumber, vpns, p)
		{
			if (m_iChoice == static_cast<int>(c))
				bSelectedByEitherPlayer = true;
		}

		if (SHOW_ICON)
			m_vsprIcon[c]->PlayCommand(
			  bSelectedByEitherPlayer ? "OffFocused" : "OffUnfocused");

		if (SHOW_SCROLLER) {
			FOREACH(PlayerNumber, vpns, p)
			m_vsprScroll[c]->PlayCommand(
			  bSelectedByEitherPlayer ? "OffFocused" : "OffUnfocused");
		}
	}
}

// Use DestX and DestY so that the cursor can move to where the icon will be
// rather than where it is.
float
ScreenSelectMaster::GetCursorX(PlayerNumber pn)
{
	int iChoice = m_iChoice;
	AutoActor& spr = m_vsprIcon[iChoice];
	return spr->GetDestX() + CURSOR_OFFSET_X_FROM_ICON.GetValue(pn);
}

float
ScreenSelectMaster::GetCursorY(PlayerNumber pn)
{
	int iChoice = m_iChoice;
	AutoActor& spr = m_vsprIcon[iChoice];
	return spr->GetDestY() + CURSOR_OFFSET_Y_FROM_ICON.GetValue(pn);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ScreenSelectMaster. */
class LunaScreenSelectMaster : public Luna<ScreenSelectMaster>
{
  public:
	static int GetSelectionIndex(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetPlayerSelectionIndex(PLAYER_1));
		return 1;
	}
	static int SetSelectionIndex(T* p, lua_State* L)
	{
		auto i = IArg(1);
		auto current = p->GetPlayerSelectionIndex(PLAYER_1);
		auto result = false;
		CLAMP(i, 0, p->GetChoiceCount() - 1);
		if (i < current) {
			result = p->ChangeSelection(PLAYER_1, MenuDir_Up, i);
			if (result) {
				MESSAGEMAN->Broadcast((Message_MenuSelectionChanged));
				MESSAGEMAN->Broadcast(
				  static_cast<MessageID>(Message_MenuUpP1 + PLAYER_1));
			}
		} else {
			result = p->ChangeSelection(PLAYER_1, MenuDir_Down, i);
			if (result) {
				MESSAGEMAN->Broadcast((Message_MenuSelectionChanged));
				MESSAGEMAN->Broadcast(
				  static_cast<MessageID>(Message_MenuDownP1 + PLAYER_1));
			}
		}
		lua_pushboolean(L, result);
		return 1;
	}
	static int PlayChangeSound(T* p, lua_State* L)
	{
		p->PlayChangeSound();
		return 0;
	}
	static int PlaySelectSound(T* p, lua_State* L)
	{
		p->PlaySelectSound();
		return 0;
	}
	// should I even bother adding this? -aj
	// would have to make a public function to get this in ssmaster.h:
	// m_aGameCommands[i].m_sName
	// static int SelectionIndexToChoiceName( T* p, lua_State *L ){  return 1; }

	LunaScreenSelectMaster()
	{
		ADD_METHOD(GetSelectionIndex);
		ADD_METHOD(SetSelectionIndex);
		ADD_METHOD(PlayChangeSound);
		ADD_METHOD(PlaySelectSound);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenSelectMaster, ScreenWithMenuElements)
// lua end
