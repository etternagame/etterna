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
auto
WheelStateToString(WheelState ws) -> const std::string&;
auto
StringToWheelState(const std::string& sDC) -> WheelState;
LuaDeclareType(WheelState);

/** @brief A wheel with data elements. */
class WheelBase : public ActorFrame
{
  public:
	~WheelBase() override;
	virtual void Load(const std::string& sType);
	void BeginScreen();

	void Update(float fDeltaTime) override;

	virtual void Move(int n);
	void ChangeMusicUnlessLocked(int n); /* +1 or -1 */
	virtual void ChangeMusic(int dist);	 /* +1 or -1 */
	virtual void SetOpenSection(const std::string& group) {}

	// Return true if we're moving fast automatically.
	auto IsMoving() const -> int;
	auto IsSettled() const -> bool;

	void GetItemPosition(float fPosOffsetsFromMiddle,
						 float& fX_out,
						 float& fY_out,
						 float& fZ_out,
						 float& fRotationX_out);
	void SetItemPosition(Actor& item, int item_index, float offset_from_middle);

	virtual auto Select()
	  -> bool; // return true if this selection can end the screen

	auto GetCurrentGroup() -> std::string;

	auto GetWheelState() -> WheelState { return m_WheelState; }
	void Lock() { m_WheelState = STATE_LOCKED; }
	auto WheelIsLocked() -> bool
	{
		return (m_WheelState == STATE_LOCKED ? true : false);
	}
	void RebuildWheelItems(int dist = INT_MAX); // INT_MAX = refresh all
	// Update the list of songs to match whatever songs are indexed by the song
	// manager (SONGMAN)
	virtual void ReloadSongList(bool searching, const std::string& findme) {}

	virtual auto GetNumItems() const -> unsigned int
	{
		return m_CurWheelItemData.size();
	}
	auto IsEmpty() -> bool { return m_bEmpty; }
	auto GetItem(unsigned int index) -> WheelItemBaseData*;
	auto LastSelected() -> WheelItemBaseData*;
	auto GetWheelItem(int i) -> WheelItemBase*
	{
		if (i < 0 || i >= static_cast<int>(m_WheelBaseItems.size())) {
			return nullptr;
		}
		return m_WheelBaseItems[i];
	}
	auto GetExpandedSectionName() -> std::string
	{
		return m_sExpandedSectionName;
	}
	auto GetCurrentIndex() -> int { return m_iSelection; }

	auto GetSelectedType() -> WheelItemDataType
	{
		return m_CurWheelItemData[m_iSelection]->m_Type;
	}

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	void TweenOnScreenForSort();
	void TweenOffScreenForSort();

	virtual auto MakeItem() -> WheelItemBase* = 0;
	virtual void UpdateSwitch();
	virtual auto MoveSpecific(int n) -> bool;
	void SetPositions();

	auto FirstVisibleIndex() -> int;

	ScrollBar m_ScrollBar;
	AutoActor m_sprHighlight;

	std::vector<WheelItemBaseData*> m_CurWheelItemData;
	std::vector<WheelItemBase*> m_WheelBaseItems;
	WheelItemBaseData* m_LastSelection{};

	bool m_bEmpty{};
	int m_iSelection{}; // index into m_CurWheelItemBaseData
	std::string m_sExpandedSectionName;

	int m_iSwitchesLeftInSpinDown{};
	float m_fLockedWheelVelocity{};
	// 0 = none; -1 or 1 = up/down
	int m_Moving{};
	RageTimer m_MovingSoundTimer;
	float m_TimeBeforeMovingBegins{};
	float m_SpinSpeed{};

	WheelState m_WheelState;
	float m_fTimeLeftInState{};
	float m_fPositionOffsetFromSelection{};

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
