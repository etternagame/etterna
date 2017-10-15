#include "global.h"
#include "RoomWheel.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "ScreenTextEntry.h"
#include "LocalizedString.h"
#include "NetworkSyncManager.h"
#include "ScreenManager.h"
#include "ActorUtil.h"
#include "LocalizedString.h"

static LocalizedString EMPTY_STRING	( "RoomWheel", "Empty" );

AutoScreenMessage( SM_BackFromRoomName );
AutoScreenMessage( SM_RoomInfoRetract );
AutoScreenMessage( SM_RoomInfoDeploy );

RoomWheel::~RoomWheel()
{
	FOREACH( WheelItemBaseData*, m_CurWheelItemData, i )
		SAFE_DELETE( *i );
	m_CurWheelItemData.clear();
}

void RoomWheel::Load( const std::string &sType ) 
{
	WheelBase::Load( sType );

	m_offset = 0;
	LOG->Trace( "RoomWheel::Load('%s')", sType.c_str() );

	searching = false;
	currentSearch.title = "";
	currentSearch.desc = "";
	currentSearch.ingame = true;
	currentSearch.open = true;
	currentSearch.password = true;

	AddPermanentItem( new RoomWheelItemData(WheelItemDataType_Generic, "Create Room", "Create a new game room", THEME->GetMetricC( m_sName, "CreateRoomColor")) );

	BuildWheelItemsData( m_CurWheelItemData );
	RebuildWheelItems();
}

WheelItemBase *RoomWheel::MakeItem()
{
	return new RoomWheelItem;
}

RoomWheelItem::RoomWheelItem( const std::string &sType ):
	WheelItemBase( sType )
{
	Load( sType );
}

RoomWheelItem::RoomWheelItem( const RoomWheelItem &cpy ):
	WheelItemBase( cpy ),
	m_sprNormalPart( cpy.m_sprNormalPart ),
	m_sprColorPart( cpy.m_sprColorPart ),
	m_sprOverPart( cpy.m_sprOverPart ),
	m_text( cpy.m_text ),
	m_Desc( cpy.m_Desc )
{
	if( cpy.GetNumChildren() != 0 )
	{
		this->AddChild( m_sprNormalPart );
		this->AddChild( m_sprColorPart );
		this->AddChild( m_sprOverPart );
		this->AddChild( &m_text );
		this->AddChild( &m_Desc );
	}
}

void RoomWheelItem::Load( const std::string &sType )
{
	// colorpart gets added first in MusicWheelItem, so follow that here.
	m_sprColorPart.Load( THEME->GetPathG(sType,"ColorPart") );
	this->AddChild( m_sprColorPart );

	m_sprNormalPart.Load( THEME->GetPathG(sType,"NormalPart") );
	this->AddChild( m_sprNormalPart );

	m_text.SetName( "Text" );
	m_text.LoadFromFont( THEME->GetPathF(sType,"text") );
	LOAD_ALL_COMMANDS_AND_SET_XY( m_text );
	this->AddChild( &m_text );

	m_Desc.SetName( "Description" );
	m_Desc.LoadFromFont( THEME->GetPathF("RoomWheel","text") );
	LOAD_ALL_COMMANDS_AND_SET_XY( m_Desc );
	this->AddChild( &m_Desc );

	m_sprOverPart.Load( THEME->GetPathG(sType,"OverPart") );
	this->AddChild( m_sprOverPart );
}

void RoomWheel::BuildWheelItemsData( vector<WheelItemBaseData*> &arrayWheelItemDatas )
{
	if( arrayWheelItemDatas.empty() )
		arrayWheelItemDatas.push_back( new RoomWheelItemData(WheelItemDataType_Generic, EMPTY_STRING, "", RageColor(1,0,0,1)) );
}

void RoomWheel::AddPermanentItem( RoomWheelItemData *itemdata  )
{
	++m_offset;
	AddItem( itemdata );
}

void RoomWheel::AddItem( WheelItemBaseData *pItemData )
{
	m_CurWheelItemData.push_back( pItemData );
	int iVisible = FirstVisibleIndex();
	int iIndex = m_CurWheelItemData.size();

	if( m_bEmpty )
	{
		m_bEmpty = false;
		// Remove the - Empty - field when we add an object from an empty state.
		RemoveItem( 0 );
	}

	// If the item was shown in the wheel, rebuild the wheel
	if( 0 <= iIndex - iVisible && iIndex - iVisible < NUM_WHEEL_ITEMS )
		RebuildWheelItems();
}

void RoomWheel::RemoveItem( int index )
{
	index += m_offset;

	if( m_bEmpty || index >= (int)m_CurWheelItemData.size() )
		return;

	vector<WheelItemBaseData *>::iterator i = m_CurWheelItemData.begin();
	i += index;

	// If this item's data happened to be last selected, make it NULL.
	if( m_LastSelection == *i )
		m_LastSelection = NULL;

	SAFE_DELETE( *i );
	m_CurWheelItemData.erase( i );

	if( m_CurWheelItemData.size() < 1 )
	{
		m_bEmpty = true;
		m_CurWheelItemData.push_back( new WheelItemBaseData(WheelItemDataType_Generic, "- EMPTY -", RageColor(1,0,0,1)) );
	}

	RebuildWheelItems();
}

static LocalizedString ENTER_ROOM_NAME( "RoomWheel", "Enter room name" );
bool RoomWheel::Select()
{
	SCREENMAN->PostMessageToTopScreen( SM_RoomInfoRetract, 0 );

	if( m_iSelection > 0 )
		return WheelBase::Select();
	if( m_iSelection == 0 )
	{
		// Since this is not actually an option outside of this wheel, NULL is a good idea.
		m_LastSelection = NULL;
		ScreenTextEntry::TextEntry( SM_BackFromRoomName, ENTER_ROOM_NAME, "", 255 );
	}
	return false;
}

void RoomWheelItem::LoadFromWheelItemData( const WheelItemBaseData *pWID, int iIndex, bool bHasFocus, int iDrawIndex )
{
	WheelItemBase::LoadFromWheelItemData( pWID, iIndex, bHasFocus, iDrawIndex );

	m_text.SetText( pWID->m_sText );
	m_text.SetDiffuseColor( pWID->m_color );

	const auto *tmpdata = dynamic_cast<const RoomWheelItemData*>( pWID );
	WheelItemBase::LoadFromWheelItemData( pWID, iIndex, bHasFocus, iDrawIndex );
	m_Desc.SetText( tmpdata->m_sDesc );
	m_Desc.SetDiffuseColor( pWID->m_color );
	m_sprColorPart->SetDiffuse( pWID->m_color );
}

void RoomWheel::Move( int n )
{
	if( n == 0 && m_iSelection >= m_offset )
	{
		const RoomWheelItemData* data = GetItem( m_iSelection-m_offset );
		if( data != NULL )
			SCREENMAN->PostMessageToTopScreen( SM_RoomInfoDeploy, 0 );
	}
	else
	{
		SCREENMAN->PostMessageToTopScreen( SM_RoomInfoRetract, 0 );
	}

	WheelBase::Move( n );
}

unsigned int RoomWheel::GetNumItems() const
{
	return m_CurWheelItemData.size() - m_offset;
}


bool findme(std::string str, std::string findme)
{
	std::transform(begin(str), end(str), begin(str), ::tolower);
	return str.find(findme) != string::npos;
}

void RoomWheel::FilterBySearch()
{
	std::function<bool (RoomData)> check;

	//Function that checks if we should remove the room based on it's state
	std::function<bool(RoomData)> checkState = [this](RoomData x) {
		return (x.State() == 2 && !currentSearch.ingame) ||
			(x.State() != 2 && !currentSearch.open) ||
			(x.GetFlags() % 2 && !currentSearch.password);
	};
	//Assign check function
	if (!(currentSearch.ingame && currentSearch.open && currentSearch.password)) {
		if (currentSearch.title == "" && currentSearch.desc == "")
			check = checkState;
		else {
			if (currentSearch.title == "")
				check = [this, checkState](RoomData x) {
				return checkState(x) ||
					!findme(x.Description(), currentSearch.desc);
			};
			else {
				if (currentSearch.desc == "")
					check = [this, checkState](RoomData x) {
					return checkState(x) ||
						!findme(x.Name(), currentSearch.title);
				};
				else
					check = [this, checkState](RoomData x) {
					return checkState(x) ||
						!(findme(x.Name(), currentSearch.title) && findme(x.Description(), currentSearch.desc));
				};
			}
		}
	}
	else {
		if (currentSearch.title == "")
			check = [this](RoomData x) { return !findme(x.Description(), currentSearch.desc); };
		else {
			if (currentSearch.desc == "")
				check = [this](RoomData x) { return !findme(x.Name(), currentSearch.title); };
			else
				check = [this](RoomData x) { return !(findme(x.Name(), currentSearch.title) && 
					findme(x.Description(), currentSearch.desc)); };
		}
	}
	roomsInWheel.clear();
	for (RoomData x : *allRooms)
		if (!check(x))
			roomsInWheel.emplace_back(x);
}
void RoomWheel::BuildFromRoomDatas()
{
	if (allRooms == NULL)
		return;
	if (searching)
		FilterBySearch();
	else
		roomsInWheel = (*allRooms);
	int difference = 0;
	RoomWheelItemData* itemData = NULL;

	difference = GetNumItems() - roomsInWheel.size();

	if (!IsEmpty())
	{
		if (difference > 0)
			for (int x = 0; x < difference; ++x)
				RemoveItem(GetNumItems() - 1);
		else
		{
			difference = abs(difference);
			for (int x = 0; x < difference; ++x)
				AddItem(new RoomWheelItemData(WheelItemDataType_Generic, "", "", RageColor(1, 1, 1, 1)));
		}
	}
	else
	{
		for (unsigned int x = 0; x < roomsInWheel.size(); ++x)
			AddItem(new RoomWheelItemData(WheelItemDataType_Generic, "", "", RageColor(1, 1, 1, 1)));
	}

	for (unsigned int i = 0; i < roomsInWheel.size(); ++i)
	{
		itemData = GetItem(i);

		itemData->m_sText = roomsInWheel[i].Name();
		itemData->m_sDesc = roomsInWheel[i].Description();
		itemData->m_iFlags = roomsInWheel[i].GetFlags();

		std::string color;
		switch (roomsInWheel[i].State())
		{
		case 2:
			color = "InGameRoomColor";
			break;
		default:
			color = "OpenRoomColor";
			break;
		}
		itemData->m_color = THEME->GetMetricC(m_sName, color);

		if (roomsInWheel[i].GetFlags() % 2)
			itemData->m_color = THEME->GetMetricC(m_sName, "PasswdRoomColor");
	}

	RebuildWheelItems();
}
void RoomWheel::UpdateRoomsList(vector<RoomData> * roomsptr)
{
	allRooms = roomsptr;
	BuildFromRoomDatas();
}
void RoomWheel::Search(RoomSearch findme)
{
	searching = true;
	currentSearch = findme;
	BuildFromRoomDatas();
}
void RoomWheel::StopSearch()
{
	searching = false;
	currentSearch.title = "";
	currentSearch.desc = "";
	currentSearch.ingame = true;
	currentSearch.open = true;
	currentSearch.password = true;
	BuildFromRoomDatas();
}
// lua start
#include "LuaBinding.h"

class LunaRoomWheel : public Luna<RoomWheel>
{
public:
	static int Move(T* p, lua_State *L)
	{
		if (lua_isnil(L, 1)) { p->Move(0); }
		else
		{
			p->Move(IArg(1));
		}
		return 1;
	}
	static int StopSearch(T* p, lua_State *L)
	{
		p->StopSearch();
		return 1;
	}
	static int Search(T* p, lua_State *L)
	{
		if (lua_isnil(L, 5))
		{
			p->StopSearch();
		}
		else
		{
			RoomSearch findme;
			findme.title = SArg(1);
			findme.desc = SArg(2);
			findme.ingame = BArg(3);
			findme.password = BArg(4);
			findme.open = BArg(5);
			p->Search(findme);
		}
		return 1;
	}

	static int MoveAndCheckType(T* p, lua_State *L)
	{
		int n = IArg(1);
		p->Move(n);
		auto tt = p->GetSelectedType();
		LuaHelpers::Push(L, tt);

		return 1;
	}
	LunaRoomWheel()
	{
		ADD_METHOD(Move);
		ADD_METHOD(MoveAndCheckType);
		ADD_METHOD(StopSearch);
		ADD_METHOD(Search);
	}
};

LUA_REGISTER_DERIVED_CLASS(RoomWheel, WheelBase)
// lua end

	
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
