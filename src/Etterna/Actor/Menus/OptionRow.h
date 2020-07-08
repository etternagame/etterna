/* OptionRow - One line in ScreenOptions. */

#ifndef OptionRow_H
#define OptionRow_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "ModIcon.h"
#include "OptionsCursor.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

class OptionRowHandler;
class GameCommand;
struct OptionRowDefinition;

std::string
ITEMS_LONG_ROW_X_NAME(size_t p);
std::string
MOD_ICON_X_NAME(size_t p);

class OptionRowType
{
  public:
	void Load(const std::string& sMetricsGroup, Actor* pParent);

  private:
	std::string m_sMetricsGroup;

	BitmapText m_textItem;
	OptionsCursor m_Underline;
	AutoActor m_sprFrame;
	BitmapText m_textTitle;
	ModIcon m_ModIcon;

	ThemeMetric<float> ITEMS_START_X;
	ThemeMetric<float> ITEMS_END_X;
	ThemeMetric<float> ITEMS_GAP_X;
	ThemeMetric<float> ITEMS_MIN_BASE_ZOOM;
	ThemeMetric1D<float> ITEMS_LONG_ROW_X;
	ThemeMetric<float> ITEMS_LONG_ROW_SHARED_X;
	ThemeMetric1D<float> MOD_ICON_X;
	ThemeMetric<RageColor> COLOR_SELECTED;
	ThemeMetric<RageColor> COLOR_NOT_SELECTED;
	ThemeMetric<RageColor> COLOR_DISABLED;
	ThemeMetric<float> TWEEN_SECONDS;
	ThemeMetric<bool> SHOW_BPM_IN_SPEED_TITLE;
	ThemeMetric<bool> SHOW_MOD_ICONS;
	ThemeMetric<bool> SHOW_UNDERLINES;
	ThemeMetric<std::string> MOD_ICON_METRICS_GROUP;

	friend class OptionRow;
};

class OptionRow : public ActorFrame
{
  public:
	OptionRow(const OptionRowType* pType);
	~OptionRow() override;

	void Clear();
	void LoadNormal(OptionRowHandler* pHand, bool bFirstItemGoesDown);
	void LoadExit();

	void SetModIcon(PlayerNumber pn, const std::string& sText, GameCommand& gc);

	void ImportOptions(const PlayerNumber& vpns);
	int ExportOptions(const PlayerNumber& vpns, bool bRowHasFocus);

	enum RowType
	{
		RowType_Normal,
		RowType_Exit
	};

	void InitText(RowType type);
	void AfterImportOptions(PlayerNumber pn);

	std::string GetRowTitle() const;

	void ChoicesChanged(RowType type, bool reset_focus = true);
	void PositionUnderlines(PlayerNumber pn);
	void PositionIcons(PlayerNumber pn);
	void UpdateText(PlayerNumber pn);
	bool GetRowHasFocus(PlayerNumber pn) const { return m_bRowHasFocus; }
	void SetRowHasFocus(PlayerNumber pn, bool bRowHasFocus);
	void UpdateEnabledDisabled();

	int GetOneSelection(PlayerNumber pn, bool bAllowFail = false) const;
	int GetOneSharedSelection(bool bAllowFail = false) const;
	void SetOneSelection(PlayerNumber pn, int iChoice);
	void SetOneSharedSelection(int iChoice);
	void SetOneSharedSelectionIfPresent(const std::string& sChoice);

	int GetChoiceInRowWithFocus() const;
	int GetChoiceInRowWithFocusShared() const;
	void SetChoiceInRowWithFocus(PlayerNumber pn, int iChoice);
	void ResetFocusFromSelection(PlayerNumber pn);

	bool GetSelected(int iChoice) const;
	// SetSelected returns true if the choices changed because of setting.
	bool SetSelected(PlayerNumber pn, int iChoice, bool b);

	bool NotifyHandlerOfSelection(PlayerNumber pn, int choice);

	const OptionRowDefinition& GetRowDef() const;
	OptionRowDefinition& GetRowDef();
	RowType GetRowType() const { return m_RowType; }
	const OptionRowHandler* GetHandler() const { return m_pHand; }

	const BitmapText& GetTextItemForRow(PlayerNumber pn,
										int iChoiceOnRow) const;
	void GetWidthXY(PlayerNumber pn,
					int iChoiceOnRow,
					int& iWidthOut,
					int& iXOut,
					int& iYOut) const;

	// ScreenOptions calls positions m_FrameDestination, then m_Frame tween to
	// that same TweenState.
	unsigned GetTextItemsSize() const { return m_textItems.size(); }
	bool GetFirstItemGoesDown() const { return m_bFirstItemGoesDown; }
	bool GoToFirstOnStart();

	std::string GetThemedItemText(int iChoice) const;

	void SetExitText(const std::string& sExitText);

	void Reload();

	// Messages
	void HandleMessage(const Message& msg) override;

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	const OptionRowType* m_pParentType;
	RowType m_RowType;
	OptionRowHandler* m_pHand;

	ActorFrame m_Frame;

	vector<BitmapText*>
	  m_textItems; // size depends on m_bRowIsLong and which players are joined
	vector<OptionsCursor*> m_Underline; // size depends on
										// m_bRowIsLong and which
										// players are joined

	Actor* m_sprFrame;
	BitmapText* m_textTitle;
	ModIcon* m_ModIcons;

	bool m_bFirstItemGoesDown;
	bool m_bRowHasFocus;

	int m_iChoiceInRowWithFocus; // this choice has input focus
	// Only one will true at a time if m_pHand->m_Def.bMultiSelect
	vector<bool> m_vbSelected;		   // size = m_pHand->m_Def.choices.size()
	Actor::TweenState m_tsDestination; // this should approach m_tsDestination.

  public:
	void SetDestination(Actor::TweenState& ts, bool bTween);
};

#endif
