#ifndef OptionRowHandler_H
#define OptionRowHandler_H

#include "GameCommand.h"
#include "Etterna/Models/Lua/LuaReference.h"
#include "RageUtil/Utils/RageUtil.h"
#include <set>

struct MenuRowDef;
class OptionRow;
struct ConfOption;

/** @brief How many options can be selected on this row? */
enum SelectType
{
	SELECT_ONE,		 /**< Only one option can be chosen on this row. */
	SELECT_MULTIPLE, /**< Multiple options can be chosen on this row. */
	SELECT_NONE,	 /**< No options can be chosen on this row. */
	NUM_SelectType,
	SelectType_Invalid
};
const RString&
SelectTypeToString(SelectType pm);
SelectType
StringToSelectType(const RString& s);
LuaDeclareType(SelectType);
/** @brief How many items are shown on the row? */
enum LayoutType
{
	LAYOUT_SHOW_ALL_IN_ROW, /**< All of the options are shown at once. */
	LAYOUT_SHOW_ONE_IN_ROW, /**< Only one option is shown at a time. */
	NUM_LayoutType,
	LayoutType_Invalid
};
const RString&
LayoutTypeToString(LayoutType pm);
LayoutType
StringToLayoutType(const RString& s);
LuaDeclareType(LayoutType);

/** @brief Define the purpose of the OptionRow. */
struct OptionRowDefinition
{
	/** @brief the name of the option row. */
	RString m_sName;
	/** @brief an explanation of the row's purpose. */
	RString m_sExplanationName;
	/** @brief Do all players have to share one option from the row? */
	bool m_bOneChoiceForAllPlayers{ false };
	SelectType m_selectType{ SELECT_ONE };
	LayoutType m_layoutType{ LAYOUT_SHOW_ALL_IN_ROW };
	std::vector<RString> m_vsChoices;
	std::set<PlayerNumber> m_vEnabledForPlayers; // only players in this set may
											// change focus to this row
	int m_iDefault{ -1 };
	bool m_bExportOnChange{ false };
	/**
	 * @brief Are theme items allowed here?
	 *
	 * This should be true for dynamic strings. */
	bool m_bAllowThemeItems{ true };
	/**
	 * @brief Are theme titles allowed here?
	 *
	 * This should be true for dynamic strings. */
	bool m_bAllowThemeTitle{ true };
	/**
	 * @brief Are explanations allowed for this row?
	 *
	 * If this is false, it will ignore the ScreenOptions::SHOW_EXPLANATIONS
	 * metric.
	 *
	 * This should be true for dynamic strings. */
	bool m_bAllowExplanation{ true };
	bool m_bShowChoicesListOnSelect{ false }; // (currently unused)

	/**
	 * @brief Is this option enabled for the Player?
	 * @param pn the Player the PlayerNumber represents.
	 * @return true if the option is enabled, false otherwise. */
	bool IsEnabledForPlayer(PlayerNumber pn) const
	{
		return m_vEnabledForPlayers.find(pn) != m_vEnabledForPlayers.end();
	}

	OptionRowDefinition()
	  : m_sName("")
	  , m_sExplanationName("")
	  , m_vsChoices()
	  , m_vEnabledForPlayers()

	{
		m_vEnabledForPlayers.insert(PLAYER_1);
	}
	void Init()
	{
		m_sName = "";
		m_sExplanationName = "";
		m_bOneChoiceForAllPlayers = false;
		m_selectType = SELECT_ONE;
		m_layoutType = LAYOUT_SHOW_ALL_IN_ROW;
		m_vsChoices.clear();
		m_vEnabledForPlayers.clear();
		m_vEnabledForPlayers.insert(PLAYER_1);
		m_iDefault = -1;
		m_bExportOnChange = false;
		m_bAllowThemeItems = true;
		m_bAllowThemeTitle = true;
		m_bAllowExplanation = true;
		m_bShowChoicesListOnSelect = false;
	}

	OptionRowDefinition(const char* n,
						bool b,
						const char* c0 = NULL,
						const char* c1 = NULL,
						const char* c2 = NULL,
						const char* c3 = NULL,
						const char* c4 = NULL,
						const char* c5 = NULL,
						const char* c6 = NULL,
						const char* c7 = NULL,
						const char* c8 = NULL,
						const char* c9 = NULL,
						const char* c10 = NULL,
						const char* c11 = NULL,
						const char* c12 = NULL,
						const char* c13 = NULL,
						const char* c14 = NULL,
						const char* c15 = NULL,
						const char* c16 = NULL,
						const char* c17 = NULL,
						const char* c18 = NULL,
						const char* c19 = NULL)
	  : m_sName(n)
	  , m_sExplanationName("")
	  , m_bOneChoiceForAllPlayers(b)
	  , m_vsChoices()
	  , m_vEnabledForPlayers()

	{
		m_vEnabledForPlayers.insert(PLAYER_1);

#define PUSH(c)                                                                \
	if (c)                                                                     \
		m_vsChoices.push_back(c);
		PUSH(c0);
		PUSH(c1);
		PUSH(c2);
		PUSH(c3);
		PUSH(c4);
		PUSH(c5);
		PUSH(c6);
		PUSH(c7);
		PUSH(c8);
		PUSH(c9);
		PUSH(c10);
		PUSH(c11);
		PUSH(c12);
		PUSH(c13);
		PUSH(c14);
		PUSH(c15);
		PUSH(c16);
		PUSH(c17);
		PUSH(c18);
		PUSH(c19);
#undef PUSH
	}
};

/** @brief Shows PlayerOptions and SongOptions in icon form. */
class OptionRowHandler
{
  public:
	OptionRowDefinition m_Def;
	std::vector<RString>
	  m_vsReloadRowMessages; // refresh this row on on these messages

	OptionRowHandler()
	  : m_Def()
	  , m_vsReloadRowMessages()
	{
	}
	virtual ~OptionRowHandler() = default;
	virtual void Init()
	{
		m_Def.Init();
		m_vsReloadRowMessages.clear();
	}
	bool Load(const Commands& cmds)
	{
		Init();
		return this->LoadInternal(cmds);
	}
	RString OptionTitle() const;
	RString GetThemedItemText(int iChoice) const;

	virtual bool LoadInternal(const Commands&) { return true; }

	/* We may re-use OptionRowHandlers. This is called before each use. If the
	 * contents of the row are dependent on external state (for example, the
	 * current song), clear the row contents and reinitialize them. As an
	 * optimization, rows which do not change can be initialized just once and
	 * left alone.
	 * If the row has been reinitialized, return RELOAD_CHANGED_ALL, and the
	 * graphic elements will also be reinitialized. If only m_vEnabledForPlayers
	 * has been changed, return RELOAD_CHANGED_ENABLED. If the row is static,
	 * and nothing has changed, return RELOAD_CHANGED_NONE. */
	enum ReloadChanged
	{
		RELOAD_CHANGED_NONE,
		RELOAD_CHANGED_ENABLED,
		RELOAD_CHANGED_ALL
	};
	virtual ReloadChanged Reload() { return RELOAD_CHANGED_NONE; }

	virtual int GetDefaultOption() const { return -1; }
	virtual void ImportOption(OptionRow*,
							  const PlayerNumber&,
							  std::vector<bool>& vbSelectedOut) const
	{
	}
	// Returns an OPT mask.
	virtual int ExportOption(const PlayerNumber&,
							 const std::vector<bool>& vbSelected) const
	{
		return 0;
	}
	virtual void GetIconTextAndGameCommand(int iFirstSelection,
										   RString& sIconTextOut,
										   GameCommand& gcOut) const;
	virtual RString GetScreen(int /* iChoice */) const { return RString(); }
	// Exists so that a lua function can act on the selection.  Returns true if
	// the choices should be reloaded.
	virtual bool NotifyOfSelection(PlayerNumber pn, int choice)
	{
		return false;
	}
	virtual bool GoToFirstOnStart() { return true; }
};

/** @brief Utilities for the OptionRowHandlers. */
namespace OptionRowHandlerUtil {
OptionRowHandler*
Make(const Commands& cmds);
OptionRowHandler*
MakeNull();
OptionRowHandler*
MakeSimple(const MenuRowDef& mrd);

void
SelectExactlyOne(int iSelection, std::vector<bool>& vbSelectedOut);
int
GetOneSelection(const std::vector<bool>& vbSelected);
}

inline void
VerifySelected(SelectType st, std::vector<bool>& selected, const RString& sName)
{
	int num_selected = 0;
	if (st == SELECT_ONE) {
		int first_selected = -1;
		if (selected.empty()) {
			LuaHelpers::ReportScriptErrorFmt(
			  "Option row %s requires only one "
			  "thing to be selected, but the list of selected things has zero "
			  "elements.",
			  sName.c_str());
			return;
		}
		for (unsigned int e = 0; e < selected.size(); ++e) {
			if (selected[e]) {
				num_selected++;
				if (first_selected == -1) {
					first_selected = static_cast<int>(e);
				}
			}
		}
		if (num_selected != 1) {
			LuaHelpers::ReportScriptErrorFmt(
			  "Option row %s requires only one "
			  "thing to be selected, but %i out of %i things are selected.",
			  sName.c_str(),
			  num_selected,
			  static_cast<int>(selected.size()));
			for (unsigned int e = 0; e < selected.size(); ++e) {
				if (selected[e] && e != first_selected) {
					selected[e] = false;
				}
			}
			if (num_selected == 0) {
				selected[0] = true;
			}
			return;
		}
	}
}

#endif
