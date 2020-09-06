#ifndef ScreenSelectMaster_H
#define ScreenSelectMaster_H

#include "Etterna/Actor/Base/ActorScroller.h"
#include "RageUtil/Sound/RageSound.h"
#include "Etterna/Models/Misc/RandomSample.h"
#include "ScreenSelect.h"

enum MenuDir
{
	MenuDir_Up,
	MenuDir_Down,
	MenuDir_Left,
	MenuDir_Right,
	MenuDir_Auto, // when players join and the selection becomes invalid
	NUM_MenuDir,
};
/** @brief A special foreach loop through the different menu directions. */
#define FOREACH_MenuDir(md) FOREACH_ENUM(MenuDir, md)
const std::string&
MenuDirToString(MenuDir md);

/** @brief The master Screen for many children Screens. */
class ScreenSelectMaster : public ScreenSelect
{
  public:
	ScreenSelectMaster();
	//~ScreenSelectMaster();
	void Init() override;
	virtual std::string GetDefaultChoice();
	void BeginScreen() override;

	bool MenuLeft(const InputEventPlus& input) override;
	bool MenuRight(const InputEventPlus& input) override;
	bool MenuUp(const InputEventPlus& input) override;
	bool MenuDown(const InputEventPlus& input) override;
	bool MenuStart(const InputEventPlus& input) override;
	void TweenOnScreen() override;
	void TweenOffScreen() override;

	void HandleScreenMessage(const ScreenMessage& SM) override;
	void HandleMessage(const Message& msg) override;
	bool AllowLateJoin() const override { return true; }

	// sm-ssc additions:
	int GetPlayerSelectionIndex(PlayerNumber pn)
	{
		return GetSelectionIndex(pn);
	}
	bool ChangeSelection(PlayerNumber pn, MenuDir dir, int iNewChoice);
	int GetChoiceCount() { return m_aGameCommands.size(); }
	void PlayChangeSound() { m_soundChange.PlayCopy(true); }
	void PlaySelectSound() { m_soundStart.PlayCopy(true); }

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	enum Page
	{
		PAGE_1,
		PAGE_2,
		NUM_Page
	}; // on PAGE_2, cursors are locked together
	static PlayerNumber GetSharedPlayer();
	Page GetPage(int iChoiceIndex) const;
	Page GetCurrentPage() const;

	ThemeMetric<bool> DO_SWITCH_ANYWAYS;
	ThemeMetric<bool> DOUBLE_PRESS_TO_SELECT;
	ThemeMetric<bool> SHOW_ICON;
	ThemeMetric<bool> SHOW_SCROLLER;
	ThemeMetric<bool> SHOW_CURSOR;
	ThemeMetric<bool> SHARED_SELECTION;
	ThemeMetric<bool> USE_ICON_METRICS;
	ThemeMetric<int> NUM_CHOICES_ON_PAGE_1;
	ThemeMetric1D<float> CURSOR_OFFSET_X_FROM_ICON;
	ThemeMetric1D<float> CURSOR_OFFSET_Y_FROM_ICON;
	ThemeMetric<bool> PER_CHOICE_ICON_ELEMENT;
	ThemeMetric<float> PRE_SWITCH_PAGE_SECONDS;
	ThemeMetric<float> POST_SWITCH_PAGE_SECONDS;
	ThemeMetric1D<std::string> OPTION_ORDER;
	ThemeMetric<bool> WRAP_CURSOR;
	ThemeMetric<bool> WRAP_SCROLLER;
	ThemeMetric<bool> LOOP_SCROLLER;
	ThemeMetric<bool> PER_CHOICE_SCROLL_ELEMENT;
	ThemeMetric<bool> ALLOW_REPEATING_INPUT;
	ThemeMetric<float> SCROLLER_SECONDS_PER_ITEM;
	ThemeMetric<float> SCROLLER_NUM_ITEMS_TO_DRAW;
	ThemeMetric<LuaReference> SCROLLER_TRANSFORM;
	// ThemeMetric<LuaReference> SCROLLER_TWEEN;
	ThemeMetric<int> SCROLLER_SUBDIVISIONS;
	ThemeMetric<std::string> DEFAULT_CHOICE;

	std::map<int, int> m_mapCurrentChoiceToNextChoice[NUM_MenuDir];

	int GetSelectionIndex(PlayerNumber pn) override;
	void UpdateSelectableChoices() override;
	bool AnyOptionsArePlayable() const;

	bool Move(PlayerNumber pn, MenuDir dir);
	bool ChangePage(int iNewChoice);
	float DoMenuStart(PlayerNumber pn);
	virtual bool ProcessMenuStart(PlayerNumber pn) { return true; }

	float GetCursorX(PlayerNumber pn);
	float GetCursorY(PlayerNumber pn);

	AutoActor m_sprExplanation[NUM_Page];
	AutoActor m_sprMore[NUM_Page];
	// icon is the shared, per-choice piece
	std::vector<AutoActor> m_vsprIcon;

	// preview is per-player, per-choice piece
	std::vector<AutoActor> m_vsprScroll;

	ActorScroller m_Scroller;

	// cursor is the per-player, shared by all choices
	AutoActor m_sprCursor;

	RageSound m_soundChange;
	RandomSample m_soundDifficult;
	RageSound m_soundStart;

	int m_iChoice;
	bool m_bChosen;
	bool m_bDoubleChoice;
	bool m_bDoubleChoiceNoSound;

	GameButton m_TrackingRepeatingInput;
};

#endif
