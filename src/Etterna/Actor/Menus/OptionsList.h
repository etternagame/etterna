#ifndef SCREEN_OPTIONS_LIST_H
#define SCREEN_OPTIONS_LIST_H

#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/CodeSet.h"
#include "Etterna/Models/Misc/OptionRowHandler.h"
#include "Etterna/Actor/Menus/OptionsCursor.h"
#include "Etterna/Screen/Others/ScreenWithMenuElements.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

class OptionsList;
class OptionListRow : public ActorFrame
{
  public:
	void Load(OptionsList* pOptions, const RString& sType);
	void SetFromHandler(const OptionRowHandler* pHandler);
	void SetTextFromHandler(const OptionRowHandler* pHandler);
	void SetUnderlines(const vector<bool>& aSelections,
					   const OptionRowHandler* pHandler);

	void PositionCursor(Actor* pCursor, int iSelection);

	void Start();

  private:
	OptionsList* m_pOptions;

	vector<BitmapText> m_Text;
	// underline for each ("self or child has selection")
	vector<AutoActor> m_Underlines;

	bool m_bItemsInTwoRows;

	ThemeMetric<float> ITEMS_SPACING_Y;
};
/** @brief A popup options list. */
class OptionsList : public ActorFrame
{
  public:
	friend class OptionListRow;

	OptionsList();
	~OptionsList() override;

	void Load(const RString& sType, PlayerNumber pn);
	void Reset();

	void Link(OptionsList* pLink) { m_pLinked = pLink; }

	/** @brief Show the top-level menu. */
	void Open();

	/** @brief Close all menus (for menu timer). */
	void Close();

	bool Input(const InputEventPlus& input);
	bool IsOpened() const { return m_asMenuStack.size() > 0; }

	bool Start(); // return true if the last menu was popped in response to this
				  // press

  private:
	ThemeMetric<RString> TOP_MENU;

	void SelectItem(const RString& sRowName, int iMenuItem);
	void MoveItem(const RString& sRowName, int iMove);
	void SwitchMenu(int iDir);
	void PositionCursor();
	void SelectionsChanged(const RString& sRowName);
	void UpdateMenuFromSelections();
	RString GetCurrentRow() const;
	const OptionRowHandler* GetCurrentHandler();
	int GetOneSelection(const RString& sRow, bool bAllowFail = false) const;
	void SwitchToCurrentRow();
	void TweenOnCurrentRow(bool bForward);
	void SetDefaultCurrentRow();
	void Push(const RString& sDest);
	void Pop();
	void ImportRow(const RString& sRow);
	void ExportRow(const RString& sRow);
	static int FindScreenInHandler(const OptionRowHandler* pHandler,
								   const RString& sScreen);

	InputQueueCodeSet m_Codes;

	OptionsList* m_pLinked;

	bool m_bStartIsDown;
	bool m_bAcceptStartRelease;

	vector<RString> m_asLoadedRows;
	map<RString, OptionRowHandler*> m_Rows;
	map<RString, vector<bool>> m_bSelections;
	set<RString> m_setDirectRows;
	set<RString> m_setTopMenus; // list of top-level menus, pointing to submenus

	PlayerNumber m_pn;
	AutoActor m_Cursor;
	OptionListRow m_Row[2];
	int m_iCurrentRow;

	vector<RString> m_asMenuStack;
	int m_iMenuStackSelection;
};

#endif
