#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/LuaManager.h"
#include "ModIconRow.h"
#include "Etterna/Models/Misc/PlayerOptions.h"
#include "Etterna/Models/Misc/PlayerState.h"

int
OptionToPreferredColumn(std::string sOptionText);

REGISTER_ACTOR_CLASS(ModIconRow);

ModIconRow::ModIconRow()
{
	m_pn = PlayerNumber_Invalid;

	this->SubscribeToMessage("PlayerOptionsChanged");
}

ModIconRow::~ModIconRow()
{
	for (auto& p : m_vpModIcon) {
		SAFE_DELETE(p);
	}
	this->RemoveAllChildren();
}

void
ModIconRow::Load(const std::string& sMetricsGroup, PlayerNumber pn)
{
	ASSERT_M(m_pn == PlayerNumber_Invalid, "Multiple calls to Load");

	m_sMetricsGroup = sMetricsGroup;
	m_pn = pn;

	SPACING_X.Load(sMetricsGroup, "SpacingX");
	SPACING_Y.Load(sMetricsGroup, "SpacingY");
	NUM_OPTION_ICONS.Load(sMetricsGroup, "NumModIcons");
	OPTION_ICON_METRICS_GROUP.Load(sMetricsGroup, "ModIconMetricsGroup");

	for (int i = 0; i < NUM_OPTION_ICONS; i++) {
		ModIcon* p = new ModIcon;
		p->SetName("ModIcon");
		float fOffset = SCALE(i,
							  0,
							  NUM_OPTION_ICONS - 1,
							  -(NUM_OPTION_ICONS - 1) / 2.0f,
							  static_cast<float>(NUM_OPTION_ICONS - 1) / 2.0f);
		p->SetXY(fOffset * SPACING_X, fOffset * SPACING_Y);
		p->Load(OPTION_ICON_METRICS_GROUP);
		ActorUtil::LoadAllCommands(p, sMetricsGroup);
		m_vpModIcon.push_back(p);
		this->AddChild(p);
	}

	SetFromGameState();
}

void
ModIconRow::HandleMessage(const Message& msg)
{
	if (msg.GetName() == "PlayerOptionsChanged")
		SetFromGameState();

	ActorFrame::HandleMessage(msg);
}

struct OptionColumnEntry
{
	const char* szString;
	int iSlotIndex;

	// void FromStack( lua_State *L, int iPos );
};

// todo: metric these? -aj
static const OptionColumnEntry g_OptionColumnEntries[] = {
	{ "Boost", 0 },
	{ "Brake", 0 },
	{ "Wave", 0 },
	{ "Expand", 0 },
	{ "Boomerang", 0 },
	//--------------------//
	{ "Drunk", 1 },
	{ "Dizzy", 1 },
	{ "Mini", 1 },
	{ "Flip", 1 },
	{ "Tornado", 1 },
	//--------------------//
	{ "Hidden", 2 },
	{ "Sudden", 2 },
	{ "Stealth", 2 },
	{ "Blink", 2 },
	{ "RandomVanish", 2 },
	//--------------------//
	{ "Mirror", 3 },
	{ "Left", 3 },
	{ "Right", 3 },
	{ "Shuffle", 3 },
	{ "SuperShuffle", 3 },
	//--------------------//
	{ "Little", 4 },
	{ "NoHolds", 4 },
	{ "Dark", 4 },
	{ "Blind", 4 },
	//--------------------//
	{ "Reverse", 5 },
	{ "Split", 5 },
	{ "Alternate", 5 },
	{ "Cross", 5 },
	{ "Centered", 5 },
	//--------------------//
	{ "Incoming", 6 },
	{ "Space", 6 },
	{ "Hallway", 6 },
	{ "Distant", 6 },
};

int
OptionToPreferredColumn(std::string sOptionText)
{
	// Speedups always go in column 0. digit ... x
	if (sOptionText.size() > 1 && isdigit(sOptionText[0]) &&
		tolower(sOptionText[sOptionText.size() - 1]) == 'x') {
		return 0;
	}

	for (auto g_OptionColumnEntry : g_OptionColumnEntries)
		if (g_OptionColumnEntry.szString == sOptionText)
			return g_OptionColumnEntry.iSlotIndex;
	return 0;
}

void
ModIconRow::SetFromGameState()
{
	std::string sOptions =
	  GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage().GetString();
	std::vector<std::string> vsOptions;
	split(sOptions, ", ", vsOptions, true);

	std::vector<std::string>
	  vsText; // fill these with what will be displayed on the tabs
	vsText.resize(m_vpModIcon.size());

	// for each option, look for the best column to place it in
	for (auto sOption : vsOptions) {
		int iPerferredCol = OptionToPreferredColumn(sOption);
		CLAMP(iPerferredCol, 0, static_cast<int>(m_vpModIcon.size()) - 1);

		if (iPerferredCol == -1)
			continue; // skip

		// search for a vacant spot
		for (int j = iPerferredCol; j < NUM_OPTION_ICONS; j++) {
			if (!vsText[j].empty()) {
				continue;
			} else {
				vsText[j] = sOption;
				break;
			}
		}
	}

	for (unsigned i = 0; i < m_vpModIcon.size(); i++)
		m_vpModIcon[i]->Set(vsText[i]);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ModIconRow. */
class LunaModIconRow : public Luna<ModIconRow>
{
  public:
	static int Load(T* p, lua_State* L)
	{
		p->Load(SArg(1), Enum::Check<PlayerNumber>(L, 2));
		COMMON_RETURN_SELF;
	}

	LunaModIconRow() { ADD_METHOD(Load); }
};

LUA_REGISTER_DERIVED_CLASS(ModIconRow, ActorFrame)

// lua end
