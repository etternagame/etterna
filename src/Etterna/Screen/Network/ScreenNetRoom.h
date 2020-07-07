/* ScreenNetSelectMusic - A method for Online/Net song selection */

#ifndef SCREEN_NET_ROOM_H
#define SCREEN_NET_ROOM_H

#include "Etterna/Actor/Menus/RoomInfoDisplay.h"
#include "Etterna/Models/Misc/RoomWheel.h"
#include "ScreenNetSelectBase.h"
#include "Etterna/Screen/Others/ScreenWithMenuElements.h"

#include <vector>

class ScreenNetRoom : public ScreenNetSelectBase
{
  public:
	ScreenNetRoom();
	void Init() override;
	bool Input(const InputEventPlus& input) override;
	void HandleScreenMessage(const ScreenMessage& SM) override;
	RoomWheel* GetRoomWheel();
	void SelectCurrent();
	void InfoSetVisible(bool visibility);

	void UpdateRoomsList();
	vector<BitmapText> m_RoomList;
	vector<RoomData>* m_Rooms;
	int m_iRoomPlace;
	RoomInfoDisplay m_roomInfo;

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	bool MenuStart(const InputEventPlus& input) override;
	bool MenuBack(const InputEventPlus& input) override;

	void TweenOffScreen() override;

  private:
	bool MenuLeft(const InputEventPlus& input) override;
	bool MenuRight(const InputEventPlus& input) override;
	void CreateNewRoom(const std::string& rName,
					   const std::string& rDesc,
					   const std::string& rPass);

	RageSound m_soundChangeSel;

	std::string m_sLastPickedRoom;

	std::string m_newRoomName, m_newRoomDesc, m_newRoomPass;

	RoomWheel m_RoomWheel;
};
#endif
