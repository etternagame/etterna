#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/Actor.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Fonts/Font.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "RageUtil/Misc/RageInput.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Core/Services/Locator.hpp"
#include "Core/Platform/Platform.hpp"
#include "ScreenNetSelectBase.h"

#define CHAT_TEXT_OUTPUT_WIDTH THEME->GetMetricF(m_sName, "ChatTextOutputWidth")
#define CHAT_TEXT_INPUT_WIDTH THEME->GetMetricF(m_sName, "ChatTextInputWidth")
#define SHOW_CHAT_LINES THEME->GetMetricI(m_sName, "ChatOutputLines")

#define USERS_X THEME->GetMetricF(m_sName, "UsersX")
#define USERS_Y THEME->GetMetricF(m_sName, "UsersY")
#define USER_SPACING_X THEME->GetMetricF(m_sName, "UserSpacingX")
#define USER_ADD_Y THEME->GetMetricF(m_sName, "UserLine2Y")

AutoScreenMessage(SM_AddToChat);
AutoScreenMessage(SM_UsersUpdate);
AutoScreenMessage(SM_FriendsUpdate);

REGISTER_SCREEN_CLASS(ScreenNetSelectBase);

void
ScreenNetSelectBase::Init()
{
	ScreenWithMenuElements::Init();
	if (!NSMAN->IsETTP()) {
		// Chat boxes
		m_sprChatInputBox.Load(THEME->GetPathG(m_sName, "ChatInputBox"));
		m_sprChatInputBox->SetName("ChatInputBox");
		LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_sprChatInputBox);
		this->AddChild(m_sprChatInputBox);

		m_sprChatOutputBox.Load(THEME->GetPathG(m_sName, "ChatOutputBox"));
		m_sprChatOutputBox->SetName("ChatOutputBox");
		LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_sprChatOutputBox);
		this->AddChild(m_sprChatOutputBox);

		m_textChatInput.LoadFromFont(THEME->GetPathF(m_sName, "chat"));
		m_textChatInput.SetName("ChatInput");
		m_textChatInput.SetWrapWidthPixels(
		  static_cast<int>(CHAT_TEXT_INPUT_WIDTH));
		LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_textChatInput);
		this->AddChild(&m_textChatInput);

		m_textChatOutput.LoadFromFont(THEME->GetPathF(m_sName, "chat"));
		m_textChatOutput.SetName("ChatOutput");
		m_textChatOutput.SetWrapWidthPixels(
		  static_cast<int>(CHAT_TEXT_OUTPUT_WIDTH));
		LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_textChatOutput);
		this->AddChild(&m_textChatOutput);

		m_textChatOutput.SetText(NSMAN->m_sChatText);
		m_textChatOutput.SetMaxLines(SHOW_CHAT_LINES, 1);
	}
	scroll = 0;

	// Display users list
	UpdateUsers();

	return;
}

bool
ScreenNetSelectBase::Input(const InputEventPlus& input)
{
	if (m_In.IsTransitioning() || m_Out.IsTransitioning())
		return false;

	if (input.type != IET_FIRST_PRESS && input.type != IET_REPEAT)
		return false;

	bool bHoldingCtrl =
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL)) ||
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL)) ||
	  (!NSMAN->useSMserver); // If we are disconnected, assume no chatting.

	bool holding_shift =
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)) ||
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT));

	// If holding control skip chatbox input
	// This allows lua input bindings to work on regular keys+control
	if (!NSMAN->IsETTP() && bHoldingCtrl) {
		wchar_t ch = INPUTMAN->DeviceInputToChar(input.DeviceI, false);
		MakeUpper(&ch, 1);
		if (ch == 'V') {
			PasteClipboard();
		}
		return ScreenWithMenuElements::Input(input);
	}

	if (!NSMAN->IsETTP()) {
		switch (input.DeviceI.button) {
			case KEY_PGUP:
				if (!holding_shift)
					ShowPreviousMsg();
				else {
					Scroll(1);
					Scroll(1);
				}
				break;
			case KEY_PGDN:
				if (!holding_shift)
					ShowNextMsg();
				else {
					Scroll(-1);
					Scroll(-1);
				}
				break;
			case KEY_ENTER:
			case KEY_KP_ENTER:
				if (m_sTextInput != "") {
					NSMAN->SendChat(m_sTextInput);
					m_sTextLastestInputs.push_back(m_sTextInput);
					m_sTextLastestInputsIndex = 0;
					if (m_sTextLastestInputs.size() > 10)
						m_sTextLastestInputs.erase(
						  m_sTextLastestInputs.begin());
				}
				m_sTextInput = "";
				UpdateTextInput();
				return true;
			case KEY_BACK:
				if (!m_sTextInput.empty())
					m_sTextInput = m_sTextInput.erase(m_sTextInput.size() - 1);
				UpdateTextInput();
				break;
			default:
				wchar_t c;
				c = INPUTMAN->DeviceInputToChar(input.DeviceI, true);
				if (c >= L' ' && enableChatboxInput) {
					m_sTextInput += WStringToString(std::wstring() + c);
					UpdateTextInput();
					return true;
				}
				break;
		}
	}
	return ScreenWithMenuElements::Input(input);
}

void
ScreenNetSelectBase::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_GoToNextScreen)
		SOUND->StopMusic();
	else if (SM == SM_AddToChat && !NSMAN->IsETTP()) {
		m_textChatOutput.SetText(NSMAN->m_sChatText);
		m_textChatOutput.SetMaxLines(SHOW_CHAT_LINES, 1);
	} else if (SM == SM_UsersUpdate) {
		UpdateUsers();
	} else if (SM == SM_FriendsUpdate) {
		MESSAGEMAN->Broadcast("FriendsUpdate");
	}

	ScreenWithMenuElements::HandleScreenMessage(SM);
}

void
ScreenNetSelectBase::TweenOffScreen()
{
	if (!NSMAN->IsETTP()) {
		OFF_COMMAND(m_sprChatInputBox);
		OFF_COMMAND(m_sprChatOutputBox);
		OFF_COMMAND(m_textChatInput);
		OFF_COMMAND(m_textChatOutput);
	}
	for (unsigned i = 0; i < m_textUsers.size(); i++)
		OFF_COMMAND(m_textUsers[i]);
}

void
ScreenNetSelectBase::UpdateTextInput()
{
	m_textChatInput.SetText(m_sTextInput);
}

void
ScreenNetSelectBase::PasteClipboard()
{
	m_sTextInput.append(Core::Platform::getClipboard());
	UpdateTextInput();
}

void
ScreenNetSelectBase::UpdateUsers()
{
	float tX = USERS_X - USER_SPACING_X;
	float tY = USERS_Y;

	for (unsigned i = 0; i < m_textUsers.size(); i++)
		this->RemoveChild(&m_textUsers[i]);

	m_textUsers.clear();

	m_textUsers.resize(NSMAN->m_ActivePlayer.size());

	for (unsigned i = 0; i < NSMAN->m_ActivePlayer.size(); i++) {
		m_textUsers[i].LoadFromFont(THEME->GetPathF(m_sName, "users"));
		m_textUsers[i].SetHorizAlign(align_center);
		m_textUsers[i].SetVertAlign(align_top);
		m_textUsers[i].SetShadowLength(0);
		m_textUsers[i].SetName("Users");

		tX += USER_SPACING_X;

		if ((i % 2) == 1)
			tY = USER_ADD_Y + USERS_Y;
		else
			tY = USERS_Y;
		m_textUsers[i].SetXY(tX, tY);

		ActorUtil::LoadAllCommands(m_textUsers[i], m_sName);
		ActorUtil::OnCommand(m_textUsers[i]);

		m_textUsers[i].SetText(NSMAN->m_PlayerNames[NSMAN->m_ActivePlayer[i]]);
		m_textUsers[i].RunCommands(THEME->GetMetricA(
		  m_sName,
		  ssprintf("Users%dCommand",
				   NSMAN->m_PlayerStatus[NSMAN->m_ActivePlayer[i]])));

		this->AddChild(&m_textUsers[i]);
	}
	if (!usersVisible)
		for (unsigned i = 0; i < NSMAN->m_ActivePlayer.size(); i++)
			m_textUsers[i].SetVisible(false);
	MESSAGEMAN->Broadcast("UsersUpdate");
}

void
ScreenNetSelectBase::Scroll(unsigned int movescroll)
{
	if (NSMAN->IsETTP())
		return;
	if ((int)(scroll + movescroll) <= m_textChatOutput.lines - SHOW_CHAT_LINES)
		scroll += movescroll;
	m_textChatOutput.ResetText();
	m_textChatOutput.SetMaxLines(SHOW_CHAT_LINES, 1, scroll);
	return;
}

std::string
ScreenNetSelectBase::GetPreviousMsg()
{
	m_sTextLastestInputsIndex += 1;
	if (m_sTextLastestInputsIndex <= m_sTextLastestInputs.size() &&
		m_sTextLastestInputsIndex > 0)
		return m_sTextLastestInputs[m_sTextLastestInputs.size() -
									m_sTextLastestInputsIndex];
	m_sTextLastestInputsIndex = m_sTextLastestInputs.size();
	return m_sTextLastestInputsIndex == 0
			 ? ""
			 : m_sTextLastestInputs[m_sTextLastestInputs.size() -
									m_sTextLastestInputsIndex];
}

std::string
ScreenNetSelectBase::GetNextMsg()
{
	m_sTextLastestInputsIndex -= 1;
	if (m_sTextLastestInputsIndex <= m_sTextLastestInputs.size() &&
		m_sTextLastestInputsIndex > 0)
		return m_sTextLastestInputs[m_sTextLastestInputs.size() -
									m_sTextLastestInputsIndex];
	m_sTextLastestInputsIndex = 0;
	return "";
}
void
ScreenNetSelectBase::ShowPreviousMsg()
{
	SetInputText(GetPreviousMsg());
	return;
}
void
ScreenNetSelectBase::ShowNextMsg()
{
	SetInputText(GetNextMsg());
	return;
}
void
ScreenNetSelectBase::SetInputText(std::string text)
{
	m_sTextInput = text;
	UpdateTextInput();
	return;
}

void
ScreenNetSelectBase::SetChatboxVisible(bool visibility)
{
	m_textChatInput.SetVisible(visibility);
	m_textChatOutput.SetVisible(visibility);
	return;
}
void
ScreenNetSelectBase::SetUsersVisible(bool visibility)
{
	usersVisible = visibility;
	for (unsigned int i = 0; i < m_textUsers.size(); i++)
		m_textUsers[i].SetVisible(visibility);
	return;
}

vector<BitmapText>*
ScreenNetSelectBase::ToUsers()
{
	return &m_textUsers;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the PlayerState. */
class LunaScreenNetSelectBase : public Luna<ScreenNetSelectBase>
{
	static int ChatboxInput(T* p, lua_State* L)
	{
		if (!lua_isnil(L, 1))
			p->enableChatboxInput = BArg(1);
		return 1;
	}
	static int UsersVisible(T* p, lua_State* L)
	{
		if (!lua_isnil(L, 1))
			p->SetUsersVisible(BArg(1));
		return 1;
	}
	static int ChatboxVisible(T* p, lua_State* L)
	{
		if (!lua_isnil(L, 1))
			p->SetChatboxVisible(BArg(1));
		return 1;
	}
	static int GetUserQty(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->ToUsers()->size());
		return 1;
	}
	static int GetUser(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
			return 0;
		if (static_cast<size_t>(IArg(1)) <= p->ToUsers()->size() &&
			IArg(1) >= 1)
			lua_pushstring(L, (*(p->ToUsers()))[IArg(1) - 1].GetText().c_str());
		else
			lua_pushstring(L, "");
		return 1;
	}
	static int GetUserState(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
			return 0;
		if (static_cast<size_t>(IArg(1)) <= p->ToUsers()->size() &&
			IArg(1) >= 1)
			lua_pushnumber(
			  L, NSMAN->m_PlayerStatus[NSMAN->m_ActivePlayer[IArg(1) - 1]]);
		else
			lua_pushnumber(L, 0);
		return 1;
	}
	static int GetFriendQty(T* p, lua_State* L)
	{
		lua_pushnumber(L, NSMAN->fl_PlayerNames.size());
		return 1;
	}
	static int GetFriendName(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
			return 0;
		if (static_cast<size_t>(IArg(1)) <= NSMAN->fl_PlayerNames.size() &&
			IArg(1) >= 1)
			lua_pushstring(L, (NSMAN->fl_PlayerNames[IArg(1) - 1]).c_str());
		else
			lua_pushstring(L, "");
		return 1;
	}
	static int GetFriendState(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
			return 0;
		if (static_cast<size_t>(IArg(1)) <= NSMAN->fl_PlayerStates.size() &&
			IArg(1) >= 1)
			lua_pushnumber(L, NSMAN->fl_PlayerStates[IArg(1) - 1]);
		else
			lua_pushnumber(L, 0);
		return 1;
	}
	static int ScrollChatUp(T* p, lua_State* L)
	{
		p->Scroll(1);
		return 1;
	}
	static int ScrollChatDown(T* p, lua_State* L)
	{
		p->Scroll(-1);
		return 1;
	}
	static int ShowNextMsg(T* p, lua_State* L)
	{
		p->ShowNextMsg();
		return 1;
	}
	static int ShowPreviousMsg(T* p, lua_State* L)
	{
		p->ShowPreviousMsg();
		return 1;
	}
	static int GetChatScroll(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetScroll());
		return 1;
	}
	static int GetChatLines(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetLines());
		return 1;
	}
	static int PasteClipboard(T* p, lua_State* L)
	{
		p->PasteClipboard();
		return 1;
	}

  public:
	LunaScreenNetSelectBase()
	{
		ADD_METHOD(GetUser);
		ADD_METHOD(UsersVisible);
		ADD_METHOD(ChatboxInput);
		ADD_METHOD(ChatboxVisible);
		ADD_METHOD(GetUserQty);
		ADD_METHOD(GetUserState);
		ADD_METHOD(GetFriendQty);
		ADD_METHOD(GetFriendState);
		ADD_METHOD(GetFriendName);
		ADD_METHOD(ScrollChatUp);
		ADD_METHOD(ScrollChatDown);
		ADD_METHOD(ShowNextMsg);
		ADD_METHOD(ShowPreviousMsg);
		ADD_METHOD(GetChatScroll);
		ADD_METHOD(GetChatLines);
		ADD_METHOD(PasteClipboard);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenNetSelectBase, ScreenWithMenuElements)
// lua end
