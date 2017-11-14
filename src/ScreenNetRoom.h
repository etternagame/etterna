/* ScreenNetSelectMusic - A method for Online/Net song selection */

#ifndef SCREEN_NET_ROOM_H
#define SCREEN_NET_ROOM_H

#include "ScreenWithMenuElements.h"
#include "ScreenNetSelectBase.h"
#include <vector>
#include "RoomWheel.h"
#include "RoomInfoDisplay.h"

class RoomData {
public:
	void SetName( const RString& name ) { m_name = name; }
	void SetDescription( const RString& desc ) { m_description = desc; }
	void SetState(unsigned int state) { m_state = state; }
	void SetFlags( unsigned int iFlags ) { m_iFlags = iFlags; }
	inline RString Name() { return m_name; }
	inline RString Description() { return m_description; }
	inline unsigned int State() { return m_state; }
	inline unsigned int GetFlags() { return m_iFlags; }
private:
	RString m_name;
	RString m_description;
	unsigned int m_state;
	unsigned int m_iFlags;
};

class ScreenNetRoom : public ScreenNetSelectBase
{
public:
	void Init() override;
	bool Input( const InputEventPlus &input ) override;
	void HandleScreenMessage( const ScreenMessage SM ) override;
	RoomWheel* GetRoomWheel();
	void SelectCurrent();

	// Lua
	void PushSelf(lua_State *L) override;

protected:
	bool MenuStart( const InputEventPlus &input ) override;
	bool MenuBack( const InputEventPlus &input ) override;

	void TweenOffScreen( ) override;

private:
	void UpdateRoomsList();
	bool MenuLeft( const InputEventPlus &input ) override;
	bool MenuRight( const InputEventPlus &input ) override;
	void CreateNewRoom( const RString& rName,  const RString& rDesc, const RString& rPass );

	RageSound m_soundChangeSel;

	vector < BitmapText > m_RoomList;
	vector < RoomData > m_Rooms;
	int m_iRoomPlace;

	string m_sLastPickedRoom;

	RString m_newRoomName, m_newRoomDesc, m_newRoomPass;

	RoomWheel m_RoomWheel;
	RoomInfoDisplay m_roomInfo;
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
