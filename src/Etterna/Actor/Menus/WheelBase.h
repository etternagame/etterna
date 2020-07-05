#ifndef WHEELBASE_H
#define WHEELBASE_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Models/Lua/LuaExpressionTransform.h"
#include "RageUtil/Sound/RageSound.h"
#include "RageUtil/Misc/RageTimer.h"
#include "ScrollBar.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "WheelItemBase.h"

#define NUM_WHEEL_ITEMS (static_cast<int>(ceil(NUM_WHEEL_ITEMS_TO_DRAW + 2)))

enum WheelState
{
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
const std::string&
WheelStateToString(WheelState ws);
WheelState
StringToWheelState(const std::string& sDC);
LuaDeclareType(WheelState);

/** @brief A wheel with data elements. */
class WheelBase : public ActorFrame
{
  public:
	~WheelBase() override;
	virtual void Load(const string& sType);
	void BeginScreen();

	void Update(float fDeltaTime) override;

	virtual void Move(int n);
	void ChangeMusicUnlessLocked(int n); /* +1 or -1 */
	virtual void ChangeMusic(int dist);	 /* +1 or -1 */
	virtual void SetOpenSection(const std::string& group) {}

	// Return true if we're moving fast automatically.
	int IsMoving() const;
	bool IsSettled() const;

	void GetItemPosition(float fPosOffsetsFromMiddle,
						 float& fX_out,
						 float& fY_out,
						 float& fZ_out,
						 float& fRotationX_out);
	void SetItemPosition(Actor& item, int item_index, float offset_from_middle);

	virtual bool Select(); // return true if this selection can end the screen

	std::string GetCurrentGroup();

	WheelState GetWheelState() { return m_WheelState; }
	void Lock() { m_WheelState = STATE_LOCKED; }
	bool WheelIsLocked()
	{
		return (m_WheelState == STATE_LOCKED ? true : false);
	}
	void RebuildWheelItems(int dist = INT_MAX); // INT_MAX = refresh all
	// Update the list of songs to match whatever songs are indexed by the song
	// manager (SONGMAN)
	virtual void ReloadSongList() {}

	virtual unsigned int GetNumItems() const
	{
		return m_CurWheelItemData.size();
	}
	bool IsEmpty() { return m_bEmpty; }
	WheelItemBaseData* GetItem(unsigned int index);
	WheelItemBaseData* LastSelected();
	WheelItemBase* GetWheelItem(int i)
	{
		if (i < 0 || i >= static_cast<int>(m_WheelBaseItems.size()))
			return nullptr;
		return m_WheelBaseItems[i];
	}
	std::string GetExpandedSectionName() { return m_sExpandedSectionName; }
	int GetCurrentIndex() { return m_iSelection; }

	WheelItemDataType GetSelectedType()
	{
		return m_CurWheelItemData[m_iSelection]->m_Type;
	}

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	void TweenOnScreenForSort();
	void TweenOffScreenForSort();

	virtual WheelItemBase* MakeItem() = 0;
	virtual void UpdateSwitch();
	virtual bool MoveSpecific(int n);
	void SetPositions();

	int FirstVisibleIndex();

	ScrollBar m_ScrollBar;
	AutoActor m_sprHighlight;

	vector<WheelItemBaseData*> m_CurWheelItemData;
	vector<WheelItemBase*> m_WheelBaseItems;
	WheelItemBaseData* m_LastSelection;

	bool m_bEmpty;
	int m_iSelection; // index into m_CurWheelItemBaseData
	std::string m_sExpandedSectionName;

	int m_iSwitchesLeftInSpinDown;
	float m_fLockedWheelVelocity;
	// 0 = none; -1 or 1 = up/down
	int m_Moving;
	RageTimer m_MovingSoundTimer;
	float m_TimeBeforeMovingBegins;
	float m_SpinSpeed;

	WheelState m_WheelState;
	float m_fTimeLeftInState;
	float m_fPositionOffsetFromSelection;

	RageSound m_soundChangeMusic;
	RageSound m_soundExpand;
	RageSound m_soundCollapse;
	RageSound m_soundLocked;

	//	bool WheelItemIsVisible(int n);
	void UpdateScrollbar();

	ThemeMetric<float> SWITCH_SECONDS;
	ThemeMetric<float> LOCKED_INITIAL_VELOCITY;
	ThemeMetric<int> SCROLL_BAR_HEIGHT;
	LuaExpressionTransform m_exprItemTransformFunction;
	ThemeMetric<float> NUM_WHEEL_ITEMS_TO_DRAW;
	ThemeMetric<RageColor> WHEEL_ITEM_LOCKED_COLOR;
};

#endif
