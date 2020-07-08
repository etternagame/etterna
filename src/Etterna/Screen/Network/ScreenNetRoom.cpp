#include "Etterna/Globals/global.h"

#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenNetRoom.h"
#include "Etterna/Screen/Others/ScreenTextEntry.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Actor/Menus/WheelItemBase.h"

AutoScreenMessage(SM_BackFromRoomName);
AutoScreenMessage(SM_BackFromRoomDesc);
AutoScreenMessage(SM_BackFromRoomPass);
AutoScreenMessage(SM_BackFromReqPass);
AutoScreenMessage(SM_RoomInfoRetract);
AutoScreenMessage(SM_RoomInfoDeploy);

AutoScreenMessage(ETTP_Disconnect);
AutoScreenMessage(ETTP_RoomsChange);

static LocalizedString ENTER_ROOM_DESCRIPTION(
  "ScreenNetRoom",
  "Enter a description for the room:");
static LocalizedString ENTER_ROOM_PASSWORD(
  "ScreenNetRoom",
  "Enter a password for the room (blank, no password):");
static LocalizedString ENTER_ROOM_REQPASSWORD("ScreenNetRoom",
											  "Enter Room's Password:");

REGISTER_SCREEN_CLASS(ScreenNetRoom);

ScreenNetRoom::ScreenNetRoom()
{
	m_Rooms = nullptr;
	m_iRoomPlace = 0;
}

void
ScreenNetRoom::Init()
{
	ScreenNetSelectBase::Init();

	m_soundChangeSel.Load(THEME->GetPathS("ScreenNetRoom", "change sel"));

	m_iRoomPlace = 0;
	m_Rooms = &(NSMAN->m_Rooms);
	m_RoomWheel.SetName("RoomWheel");
	m_RoomWheel.Load("RoomWheel");
	m_RoomWheel.BeginScreen();
	LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_RoomWheel);
	this->AddChild(&m_RoomWheel);

	// Since the room info display does not start active, and it is activated by
	// code elsewhere, it should not be put on screen to begin with.
	m_roomInfo.SetName("RoomInfoDisplay");
	m_roomInfo.Load("RoomInfoDisplay");
	m_roomInfo.SetDrawOrder(1);
	this->AddChild(&m_roomInfo);

	this->SortByDrawOrder();
	UpdateRoomsList();
	NSMAN->OnRoomSelect();
}

bool
ScreenNetRoom::Input(const InputEventPlus& input)
{
	if ((input.MenuI == GAME_BUTTON_LEFT || input.MenuI == GAME_BUTTON_RIGHT) &&
		input.type == IET_RELEASE)
		m_RoomWheel.Move(0);

	return ScreenNetSelectBase::Input(input);
}

RoomWheel*
ScreenNetRoom::GetRoomWheel()
{
	return &m_RoomWheel;
}

void
ScreenNetRoom::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_GoToPrevScreen) {
		SCREENMAN->SetNewScreen(THEME->GetMetric(m_sName, "PrevScreen"));
	} else if (SM == ETTP_Disconnect) {
		TweenOffScreen();
		Cancel(SM_GoToPrevScreen);
	} else if (SM == SM_BackFromReqPass) {
		if (!ScreenTextEntry::s_bCancelledLast) {
			NSMAN->EnterRoom(m_sLastPickedRoom, ScreenTextEntry::s_sLastAnswer);
		}
	} else if (SM == ETTP_RoomsChange) {
		UpdateRoomsList();
	} else if (SM == SM_BackFromRoomName) {
		if (!ScreenTextEntry::s_bCancelledLast) {
			m_newRoomName = ScreenTextEntry::s_sLastAnswer;
			ScreenTextEntry::TextEntry(
			  SM_BackFromRoomDesc, ENTER_ROOM_DESCRIPTION, "", 255);
		}
	} else if (SM == SM_BackFromRoomDesc) {
		if (!ScreenTextEntry::s_bCancelledLast) {
			m_newRoomDesc = ScreenTextEntry::s_sLastAnswer;
			ScreenTextEntry::Password(SM_BackFromRoomPass, ENTER_ROOM_PASSWORD);
		}
	} else if (SM == SM_BackFromRoomPass) {
		if (!ScreenTextEntry::s_bCancelledLast) {
			m_newRoomPass = ScreenTextEntry::s_sLastAnswer;
			CreateNewRoom(m_newRoomName, m_newRoomDesc, m_newRoomPass);
		}
	} else if (SM == SM_RoomInfoRetract) {
		m_roomInfo.RetractInfoBox();
	} else if (SM == SM_RoomInfoDeploy) {
		int i =
		  m_RoomWheel.GetCurrentIndex() - m_RoomWheel.GetPerminateOffset();
		const RoomWheelItemData* data = m_RoomWheel.GetItem(i);
		if (data != nullptr)
			m_roomInfo.SetRoom(data);
	}

	ScreenNetSelectBase::HandleScreenMessage(SM);
}

void
ScreenNetRoom::TweenOffScreen()
{
	NSMAN->OffRoomSelect();
}

bool
ScreenNetRoom::MenuStart(const InputEventPlus& input)
{
	SelectCurrent();
	ScreenNetSelectBase::MenuStart(input);
	return true;
}

void
ScreenNetRoom::SelectCurrent()
{
	if (NSMAN->IsETTP() && ((ETTProtocol*)NSMAN->curProtocol)->creatingRoom) {
		SCREENMAN->SystemMessage("Error: Already trying to create a room");
		return;
	}
	m_RoomWheel.Select();
	RoomWheelItemData* rwd =
	  dynamic_cast<RoomWheelItemData*>(m_RoomWheel.LastSelected());
	if (rwd != nullptr) {
		if (rwd->m_iFlags % 2 != 0u || rwd->hasPassword) {
			m_sLastPickedRoom = rwd->m_sText;
			ScreenTextEntry::Password(SM_BackFromReqPass,
									  ENTER_ROOM_REQPASSWORD);
		} else {
			NSMAN->EnterRoom(rwd->m_sText);
		}
	}
	return;
}

bool
ScreenNetRoom::MenuBack(const InputEventPlus& input)
{
	TweenOffScreen();

	Cancel(SM_GoToPrevScreen);

	ScreenNetSelectBase::MenuBack(input);
	return true;
}

bool
ScreenNetRoom::MenuLeft(const InputEventPlus& input)
{
	bool bHandled = false;
	if (input.type == IET_FIRST_PRESS) {
		m_RoomWheel.Move(-1);
		bHandled = true;
	}

	return ScreenNetSelectBase::MenuLeft(input) || bHandled;
}

bool
ScreenNetRoom::MenuRight(const InputEventPlus& input)
{
	bool bHandled = false;
	if (input.type == IET_FIRST_PRESS) {
		m_RoomWheel.Move(1);
		bHandled = true;
	}

	return ScreenNetSelectBase::MenuRight(input) || bHandled;
}

void
ScreenNetRoom::UpdateRoomsList()
{
	if (m_iRoomPlace < 0)
		m_iRoomPlace = 0;
	if (m_iRoomPlace >= (int)m_Rooms->size())
		m_iRoomPlace = m_Rooms->size() - 1;
	m_RoomWheel.UpdateRoomsList(m_Rooms);
}

void
ScreenNetRoom::CreateNewRoom(const std::string& rName,
							 const std::string& rDesc,
							 const std::string& rPass)
{
	NSMAN->CreateNewRoom(rName, rDesc, rPass);
}

void
ScreenNetRoom::InfoSetVisible(bool visibility)
{
	m_roomInfo.SetVisible(visibility);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the PlayerState. */
class LunaScreenNetRoom : public Luna<ScreenNetRoom>
{
  public:
	static int GetMusicWheel(T* p, lua_State* L)
	{
		p->GetRoomWheel()->PushSelf(L);
		return 1;
	}
	static int GetRoomWheel(T* p, lua_State* L)
	{
		p->GetRoomWheel()->PushSelf(L);
		return 1;
	}
	static int SelectCurrent(T* p, lua_State* L)
	{
		p->SelectCurrent();
		return 1;
	}
	static int GetSelectionState(T* p, lua_State* L)
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	static int InfoSetVisible(T* p, lua_State* L)
	{
		p->InfoSetVisible(BArg(1));
		return 1;
	}

	LunaScreenNetRoom()
	{
		ADD_METHOD(GetMusicWheel);
		ADD_METHOD(GetRoomWheel);
		ADD_METHOD(SelectCurrent);
		ADD_METHOD(GetSelectionState);
		ADD_METHOD(InfoSetVisible);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenNetRoom, ScreenNetSelectBase)
// lua end
