#ifndef WHEELBASE_H
#define WHEELBASE_H

#include "AutoActor.h"
#include "ActorFrame.h"
#include "RageSound.h"
#include "GameConstantsAndTypes.h"
#include "ScreenMessage.h"
#include "ScrollBar.h"
#include "RageTimer.h"
#include "WheelItemBase.h"
#include "ThemeMetric.h"
#include "LuaExpressionTransform.h"

#define NUM_WHEEL_ITEMS		(static_cast<int>(ceil(NUM_WHEEL_ITEMS_TO_DRAW+2)))

enum WheelState {
	STATE_SELECTING,
	STATE_FLYING_OFF_BEFORE_NEXT_SORT, 
	STATE_FLYING_ON_AFTER_NEXT_SORT, 
	STATE_ROULETTE_SPINNING,
	STATE_ROULETTE_SLOWING_DOWN,
	STATE_RANDOM_SPINNING,
	STATE_LOCKED,
	NUM_WheelState,
	WheelState_Invalid,
};
const RString& WheelStateToString( WheelState ws );
WheelState StringToWheelState( const RString& sDC );
LuaDeclareType( WheelState );

/** @brief A wheel with data elements. */
class WheelBase : public ActorFrame
{
public:
	~WheelBase() override;
	virtual void Load( const RString &sType );
	void BeginScreen();

	void Update( float fDeltaTime ) override;

	virtual void Move(int n);
	void ChangeMusicUnlessLocked( int n ); /* +1 or -1 */
	virtual void ChangeMusic(int dist); /* +1 or -1 */
	virtual void SetOpenSection( const RString &group ) { }

	// Return true if we're moving fast automatically.
	int IsMoving() const;
	bool IsSettled() const;

	void GetItemPosition( float fPosOffsetsFromMiddle, float& fX_out, float& fY_out, float& fZ_out, float& fRotationX_out );
	void SetItemPosition(Actor &item, int item_index, float offset_from_middle);

	virtual bool Select();	// return true if this selection can end the screen

	WheelState GetWheelState() { return m_WheelState; }
	bool WheelIsLocked() { return (m_WheelState == STATE_LOCKED ? true : false); }
	void RebuildWheelItems( int dist = INT_MAX );	// INT_MAX = refresh all
	// Update the list of songs to match whatever songs are indexed by the song manager (SONGMAN)
	virtual void ReloadSongList() {}

	virtual unsigned int GetNumItems() const { return m_CurWheelItemData.size(); }
	bool IsEmpty() { return m_bEmpty; }
	WheelItemBaseData* GetItem(unsigned int index);
	WheelItemBaseData* LastSelected();
	WheelItemBase *GetWheelItem( int i ) { if( i < 0 || i >= (int) m_WheelBaseItems.size() ) return NULL; return m_WheelBaseItems[i]; }
	RString GetExpandedSectionName() { return m_sExpandedSectionName; }
	int GetCurrentIndex() { return m_iSelection; }

	WheelItemDataType GetSelectedType() { return m_CurWheelItemData[m_iSelection]->m_Type; }

	// Lua
	void PushSelf( lua_State *L ) override;

protected:
	void TweenOnScreenForSort();
	void TweenOffScreenForSort();

	virtual WheelItemBase *MakeItem() = 0;
	virtual void UpdateSwitch();
	virtual bool MoveSpecific(int n);
	void SetPositions();

	int FirstVisibleIndex();

	ScrollBar	m_ScrollBar;
	AutoActor	m_sprHighlight;

	vector<WheelItemBaseData *> m_CurWheelItemData;
	vector<WheelItemBase *> m_WheelBaseItems;
	WheelItemBaseData* m_LastSelection;
	
	bool		m_bEmpty;
	int		m_iSelection;		// index into m_CurWheelItemBaseData
	RString		m_sExpandedSectionName;

	int			m_iSwitchesLeftInSpinDown;
	float		m_fLockedWheelVelocity;
	// 0 = none; -1 or 1 = up/down
	int		m_Moving;
	RageTimer	m_MovingSoundTimer;
	float		m_TimeBeforeMovingBegins;
	float		m_SpinSpeed;

	WheelState	m_WheelState;
	float		m_fTimeLeftInState;
	float		m_fPositionOffsetFromSelection;

	RageSound m_soundChangeMusic;
	RageSound m_soundExpand;
	RageSound m_soundCollapse;
	RageSound m_soundLocked;

//	bool WheelItemIsVisible(int n);
	void UpdateScrollbar();

	ThemeMetric<float>	SWITCH_SECONDS;
	ThemeMetric<float>	LOCKED_INITIAL_VELOCITY;
	ThemeMetric<int>	SCROLL_BAR_HEIGHT;
	LuaExpressionTransform	m_exprItemTransformFunction;
	ThemeMetric<float>	NUM_WHEEL_ITEMS_TO_DRAW;
	ThemeMetric<RageColor>	WHEEL_ITEM_LOCKED_COLOR;
};

#endif

/*
 * (c) 2001-2004 Chris Danford, Chris Gomez, Glenn Maynard, Josh Allen
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
