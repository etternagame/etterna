/* RoomWheel - A wheel containing data about rooms. */

#ifndef ROOM_WHEEL_H
#define ROOM_WHEEL_H

#include "WheelBase.h"
#include "WheelItemBase.h"
#include "ThemeMetric.h"

class RoomData {
public:
	void SetName(const std::string& name) { m_name = name; }
	void SetDescription(const std::string& desc) { m_description = desc; }
	void SetState(unsigned int state) { m_state = state; }
	void SetFlags(unsigned int iFlags) { m_iFlags = iFlags; }
	inline std::string Name() { return m_name; }
	inline std::string Description() { return m_description; }
	inline unsigned int State() { return m_state; }
	inline unsigned int GetFlags() { return m_iFlags; }
	RoomData() { m_name=""; m_description=""; m_state=0; m_iFlags=0; }
private:
	std::string m_name;
	std::string m_description;
	unsigned int m_state;
	unsigned int m_iFlags;
};

struct RoomWheelItemData : public WheelItemBaseData
{
	RoomWheelItemData() = default;
	RoomWheelItemData( WheelItemDataType type, const std::string& sTitle, const std::string& sDesc, const RageColor &color ):
		WheelItemBaseData( type, sTitle, color ), m_sDesc(sDesc), m_iFlags(0) { };

	std::string		m_sDesc;
	unsigned int	m_iFlags{0};
};

class RoomWheelItem : public WheelItemBase
{
public:
	RoomWheelItem( const std::string &sType = "RoomWheelItem" );
	RoomWheelItem( const RoomWheelItem &cpy );

	void LoadFromWheelItemData( const WheelItemBaseData* pWID, int iIndex, bool bHasFocus, int iDrawIndex ) override;
	RoomWheelItem *Copy() const override { return new RoomWheelItem(*this); }
	void Load( const std::string &sType );

private:
	AutoActor	m_sprNormalPart;
	AutoActor	m_sprColorPart;
	AutoActor	m_sprOverPart;
	BitmapText	m_text;
	BitmapText	m_Desc;
};

struct RoomInfo
{
	std::string songTitle;
	std::string songSubTitle;
	std::string songArtist;
	int numPlayers;
	int maxPlayers;
	vector<std::string> players;
};

struct RoomSearch
{
	std::string title;
	std::string desc;
	bool ingame;
	bool password;
	bool open;
	RoomSearch() { ingame = password = open = true; title = desc = ""; }
};


class RoomWheel : public WheelBase
{
public:
	~RoomWheel() override;
	void Load( const std::string&sType ) override;
	virtual void BuildWheelItemsData( vector<WheelItemBaseData*> &arrayWheelItemDatas );
	unsigned int GetNumItems() const override;
	bool Select() override;
	void Move( int n ) override;

	inline RoomWheelItemData *GetItem( unsigned int i ) { return dynamic_cast<RoomWheelItemData*>( WheelBase::GetItem(i + m_offset) ); }
	void AddPermanentItem( RoomWheelItemData *itemdata );
	int GetCurrentIndex() const { return m_iSelection; }
	int GetPerminateOffset() const { return m_offset; }
	void AddItem( WheelItemBaseData *itemdata );
	void RemoveItem( int index );


	void StopSearch();
	void Search(RoomSearch findme);

	void BuildFromRoomDatas();
	void UpdateRoomsList(vector<RoomData> * m_Roomsptr);
	void FilterBySearch();

	// Lua
	void PushSelf(lua_State *L) override;

private:
	WheelItemBase *MakeItem() override;
	int m_offset;

	vector<RoomData> * allRooms;
	vector<RoomData> roomsInWheel;

	RoomSearch currentSearch;
	bool searching;
};

#endif

/*
 * (c) 2004 Josh Allen
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
