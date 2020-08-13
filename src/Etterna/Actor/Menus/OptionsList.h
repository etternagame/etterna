#ifndef SCREEN_OPTIONS_LIST_H
#define SCREEN_OPTIONS_LIST_H

#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/CodeSet.h"
#include "Etterna/Models/Misc/OptionRowHandler.h"
#include "Etterna/Screen/Others/ScreenWithMenuElements.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

class OptionsList;
class OptionListRow : public ActorFrame
{
  public:
	void Load(OptionsList* pOptions, const std::string& sType);
	void SetFromHandler(const OptionRowHandler* pHandler);
	void SetTextFromHandler(const OptionRowHandler* pHandler);
	void SetUnderlines(const vector<bool>& aSelections,
					   const OptionRowHandler* pHandler);

	void PositionCursor(Actor* pCursor, int iSelection);

	void Start();

  private:
	OptionsList* m_pOptions = nullptr;

	std::vector<BitmapText> m_Text;
	// underline for each ("self or child has selection")
	std::vector<AutoActor> m_Underlines;

	bool m_bItemsInTwoRows = false;

	ThemeMetric<float> ITEMS_SPACING_Y;
};
/** @brief A popup options list. */
class OptionsList : public ActorFrame
{
  public:
	friend class OptionListRow;

	OptionsList();
	~OptionsList() override;

	void Load(const std::string& sType, PlayerNumber pn);
	void Reset();

	void Link(OptionsList* pLink) { m_pLinked = pLink; }

	/** @brief Show the top-level menu. */
	void Open();

	/** @brief Close all menus (for menu timer). */
	void Close();

	bool Input(const InputEventPlus& input);
	bool IsOpened() const { return !m_asMenuStack.empty(); }

	bool Start(); // return true if the last menu was popped in response to this
				  // press

  private:
	ThemeMetric<std::string> TOP_MENU;

	void SelectItem(const std::string& sRowName, int iMenuItem);
	void MoveItem(const std::string& sRowName, int iMove);
	void SwitchMenu(int iDir);
	void PositionCursor();
	void SelectionsChanged(const std::string& sRowName);
	void UpdateMenuFromSelections();
	std::string GetCurrentRow() const;
	const OptionRowHandler* GetCurrentHandler();
	int GetOneSelection(const std::string& sRow, bool bAllowFail = false) const;
	void SwitchToCurrentRow();
	void TweenOnCurrentRow(bool bForward);
	void SetDefaultCurrentRow();
	void Push(const std::string& sDest);
	void Pop();
	void ImportRow(const std::string& sRow);
	void ExportRow(const std::string& sRow);
	static int FindScreenInHandler(const OptionRowHandler* pHandler,
								   const std::string& sScreen);

	InputQueueCodeSet m_Codes;

	OptionsList* m_pLinked;

	bool m_bStartIsDown;
	bool m_bAcceptStartRelease;

	std::vector<std::string> m_asLoadedRows;
	std::map<std::string, OptionRowHandler*> m_Rows;
	std::map<std::string, std::vector<bool>> m_bSelections;
	std::set<std::string> m_setDirectRows;
	std::set<std::string>
	  m_setTopMenus; // list of top-level menus, pointing to submenus

	PlayerNumber m_pn;
	AutoActor m_Cursor;
	OptionListRow m_Row[2];
	int m_iCurrentRow;

	std::vector<std::string> m_asMenuStack;
	int m_iMenuStackSelection;
};

#endif
