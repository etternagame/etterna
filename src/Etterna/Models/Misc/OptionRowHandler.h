#ifndef OptionRowHandler_H
#define OptionRowHandler_H

#include "GameCommand.h"
#include "Etterna/Models/Lua/LuaReference.h"

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
const std::string&
SelectTypeToString(SelectType x);
SelectType
StringToSelectType(const std::string& s);
LuaDeclareType(SelectType);
/** @brief How many items are shown on the row? */
enum LayoutType
{
	LAYOUT_SHOW_ALL_IN_ROW, /**< All of the options are shown at once. */
	LAYOUT_SHOW_ONE_IN_ROW, /**< Only one option is shown at a time. */
	NUM_LayoutType,
	LayoutType_Invalid
};
const std::string&
LayoutTypeToString(LayoutType x);
LayoutType
StringToLayoutType(const std::string& s);
LuaDeclareType(LayoutType);

enum ReloadChanged
{
	RELOAD_CHANGED_NONE,
	RELOAD_CHANGED_ENABLED,
	RELOAD_CHANGED_ALL,
	NUM_ReloadChanged,
	ReloadChanged_Invalid
};
const std::string&
ReloadChangedToString(ReloadChanged x);
ReloadChanged
StringToReloadChanged(const std::string& s);
LuaDeclareType(ReloadChanged);

/** @brief Define the purpose of the OptionRow. */
struct OptionRowDefinition
{
	/** @brief the name of the option row. */
	std::string m_sName;
	/** @brief an explanation of the row's purpose. */
	std::string m_sExplanationName;
	/** @brief Do all players have to share one option from the row? */
	bool m_bOneChoiceForAllPlayers{ false };
	SelectType m_selectType{ SELECT_ONE };
	LayoutType m_layoutType{ LAYOUT_SHOW_ALL_IN_ROW };
	vector<std::string> m_vsChoices;
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
	[[nodiscard]] bool IsEnabledForPlayer(PlayerNumber pn) const
	{
		return m_vEnabledForPlayers.find(pn) != m_vEnabledForPlayers.end();
	}

	OptionRowDefinition()
	  : m_sName("")
	  , m_sExplanationName("")

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
						const char* c0 = nullptr,
						const char* c1 = nullptr,
						const char* c2 = nullptr,
						const char* c3 = nullptr,
						const char* c4 = nullptr,
						const char* c5 = nullptr,
						const char* c6 = nullptr,
						const char* c7 = nullptr,
						const char* c8 = nullptr,
						const char* c9 = nullptr,
						const char* c10 = nullptr,
						const char* c11 = nullptr,
						const char* c12 = nullptr,
						const char* c13 = nullptr,
						const char* c14 = nullptr,
						const char* c15 = nullptr,
						const char* c16 = nullptr,
						const char* c17 = nullptr,
						const char* c18 = nullptr,
						const char* c19 = nullptr)
	  : m_sName(n)
	  , m_sExplanationName("")
	  , m_bOneChoiceForAllPlayers(b)

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
	std::vector<std::string>
	  m_vsReloadRowMessages; // refresh this row on on these messages

	OptionRowHandler() = default;
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

	[[nodiscard]] std::string OptionTitle() const;
	[[nodiscard]] std::string GetThemedItemText(int iChoice) const;

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
	virtual ReloadChanged Reload() { return RELOAD_CHANGED_NONE; }

	[[nodiscard]] virtual int GetDefaultOption() const { return -1; }
	virtual void ImportOption(OptionRow*,
							  const PlayerNumber&,
							  std::vector<bool>& vbSelectedOut) const
	{
	}
	// Returns an OPT mask.
	[[nodiscard]] virtual int ExportOption(const PlayerNumber&,
										   const vector<bool>& vbSelected) const
	{
		return 0;
	}
	virtual void GetIconTextAndGameCommand(int iFirstSelection,
										   std::string& sIconTextOut,
										   GameCommand& gcOut) const;

	[[nodiscard]] virtual std::string GetScreen(int /* iChoice */) const
	{
		return std::string();
	}
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
SelectExactlyOne(int iSelection, vector<bool>& vbSelectedOut);
int
GetOneSelection(const vector<bool>& vbSelected);
}

inline void
VerifySelected(SelectType st, vector<bool>& selected, const std::string& sName)
{
	auto num_selected = 0;
	if (st == SELECT_ONE) {
		auto first_selected = -1;
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
