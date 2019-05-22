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
	void Init() override;
	bool Input(const InputEventPlus& input) override;
	void HandleScreenMessage(ScreenMessage SM) override;
	RoomWheel* GetRoomWheel();
	void SelectCurrent();
	void InfoSetVisible(bool visibility);

	void UpdateRoomsList();
	std::vector<BitmapText> m_RoomList;
	std::vector<RoomData>* m_Rooms;
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
	void CreateNewRoom(const RString& rName,
					   const RString& rDesc,
					   const RString& rPass);

	RageSound m_soundChangeSel;

	std::string m_sLastPickedRoom;

	RString m_newRoomName, m_newRoomDesc, m_newRoomPass;

	RoomWheel m_RoomWheel;
};
#endif

/*
 * (c) 2004 Charles Lohr, Joshua Allen
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
