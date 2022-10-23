#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/AnnouncerManager.h"
#include "Foreach.h"
#include "GameCommand.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "LocalizedString.h"
#include "PlayerOptions.h"
#include "PlayerState.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Screen/Others/ScreenPrompt.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Core/Services/Locator.hpp"
#include "Core/Platform/Platform.hpp"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/Songs/SongUtil.h"

static LocalizedString COULD_NOT_LAUNCH_BROWSER(
  "GameCommand",
  "Could not launch web browser.");

void
GameCommand::Init()
{
	m_bApplyCommitsScreens = true;
	m_sName = "";
	m_sText = "";
	m_bInvalid = true;
	m_iIndex = -1;
	m_MultiPlayer = MultiPlayer_Invalid;
	m_pStyle = nullptr;
	m_dc = Difficulty_Invalid;
	m_sPreferredModifiers = "";
	m_sStageModifiers = "";
	m_sAnnouncer = "";
	m_sScreen = "";
	m_LuaFunction.Unset();
	m_pSong = nullptr;
	m_pSteps = nullptr;
	m_SortOrder = SortOrder_Invalid;
	m_sSoundPath = "";
	m_vsScreensToPrepare.clear();
	m_sProfileID = "";
	m_sUrl = "";
	m_bUrlExits = true;
	m_bStopMusic = false;
	m_bApplyDefaultOptions = false;
	m_bFadeMusic = false;
	m_fMusicFadeOutVolume = -1.0f;
	m_fMusicFadeOutSeconds = -1.0f;
}

class SongOptions;
bool
CompareSongOptions(const SongOptions& so1, const SongOptions& so2);

bool
GameCommand::DescribesCurrentModeForAllPlayers() const
{
	if (!DescribesCurrentMode(PLAYER_1))
		return false;

	return true;
}

bool
GameCommand::DescribesCurrentMode(PlayerNumber pn) const
{
	if ((m_pStyle != nullptr) && GAMESTATE->GetCurrentStyle(pn) != m_pStyle)
		return false;
	// HACK: don't compare m_dc if m_pSteps is set.  This causes problems
	// in ScreenSelectOptionsMaster::ImportOptions if m_PreferredDifficulty
	// doesn't match the difficulty of m_pCurSteps.
	if (m_pSteps == nullptr && m_dc != Difficulty_Invalid) {
		// Why is this checking for all players?
		if (GAMESTATE->m_PreferredDifficulty != m_dc)
			return false;
	}

	if (!m_sAnnouncer.empty() &&
		m_sAnnouncer != ANNOUNCER->GetCurAnnouncerName())
		return false;

	if (!m_sPreferredModifiers.empty()) {
		auto po = GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred();
		auto so = GAMESTATE->m_SongOptions.GetPreferred();
		po.FromString(m_sPreferredModifiers);
		so.FromString(m_sPreferredModifiers);

		if (po != GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred())
			return false;
		if (so != GAMESTATE->m_SongOptions.GetPreferred())
			return false;
	}
	if (!m_sStageModifiers.empty()) {
		auto po = GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage();
		auto so = GAMESTATE->m_SongOptions.GetStage();
		po.FromString(m_sStageModifiers);
		so.FromString(m_sStageModifiers);

		if (po != GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage())
			return false;
		if (so != GAMESTATE->m_SongOptions.GetStage())
			return false;
	}

	if (m_pSong && GAMESTATE->m_pCurSong.Get() != m_pSong)
		return false;
	if (m_pSteps && GAMESTATE->m_pCurSteps.Get() != m_pSteps)
		return false;
	if (!m_sSongGroup.empty() &&
		GAMESTATE->m_sPreferredSongGroup != m_sSongGroup)
		return false;
	if (m_SortOrder != SortOrder_Invalid &&
		GAMESTATE->m_PreferredSortOrder != m_SortOrder)
		return false;
	if (!m_sProfileID.empty() &&
		ProfileManager::m_sDefaultLocalProfileID[pn].Get() != m_sProfileID)
		return false;

	return true;
}

void
GameCommand::Load(int iIndex, const Commands& cmds)
{
	m_iIndex = iIndex;
	m_bInvalid = false;
	m_Commands = cmds;

	FOREACH_CONST(Command, cmds.v, cmd)
	LoadOne(*cmd);
}

void
GameCommand::LoadOne(const Command& cmd)
{
	const auto sName = cmd.GetName();
	if (sName.empty())
		return;

	std::string sValue;
	for (unsigned i = 1; i < cmd.m_vsArgs.size(); ++i) {
		if (i > 1)
			sValue += ",";
		sValue += cmd.m_vsArgs[i];
	}

#define MAKE_INVALID(expr)                                                     \
	m_sInvalidReason = (expr);                                                 \
	LuaHelpers::ReportScriptError(m_sInvalidReason, "INVALID_GAME_COMMAND");   \
	m_bInvalid = true;

#define CHECK_INVALID_COND(member, value, cond, message)                       \
	if (cond) {                                                                \
		MAKE_INVALID(message);                                                 \
	} else {                                                                   \
		(member) = value;                                                      \
	}

#define CHECK_INVALID_VALUE(member, value, invalid_value, value_name)          \
	CHECK_INVALID_COND(                                                        \
	  member,                                                                  \
	  value,                                                                   \
	  ((value) == (invalid_value)),                                            \
	  ssprintf("Invalid " #value_name " \"%s\".", sValue.c_str()));

	if (sName == "style") {
		const auto* style =
		  GAMEMAN->GameAndStringToStyle(GAMESTATE->m_pCurGame, sValue);
		CHECK_INVALID_VALUE(m_pStyle, style, NULL, style);
	}

	else if (sName == "difficulty") {
		const auto dc = StringToDifficulty(sValue);
		CHECK_INVALID_VALUE(m_dc, dc, Difficulty_Invalid, difficulty);
	}

	else if (sName == "announcer") {
		m_sAnnouncer = sValue;
	}

	else if (sName == "name") {
		m_sName = sValue;
	}

	else if (sName == "text") {
		m_sText = sValue;
	}

	else if (sName == "mod") {
		if (!m_sPreferredModifiers.empty())
			m_sPreferredModifiers += ",";
		m_sPreferredModifiers += sValue;
	}

	else if (sName == "stagemod") {
		if (!m_sStageModifiers.empty())
			m_sStageModifiers += ",";
		m_sStageModifiers += sValue;
	}

	else if (sName == "lua") {
		m_LuaFunction.SetFromExpression(sValue);
		if (m_LuaFunction.IsNil()) {
			MAKE_INVALID("Lua error in game command: \"" + sValue +
						 "\" evaluated to nil");
		}
	}

	else if (sName == "screen") {
		// OptionsList uses the screen command to push onto its stack.
		// OptionsList "screen"s are OptionRow entries in ScreenOptionsMaster.
		// So if the metric exists in ScreenOptionsMaster, consider it valid.
		// Additionally, the screen value can be used to create an OptionRow.
		// When used to create an OptionRow, it pulls a metric from OptionsList.
		// -Kyz

		if (!THEME->HasMetric("ScreenOptionsMaster", sValue)) {
			if (!THEME->HasMetric("OptionsList", "Line" + sValue)) {
				if (!SCREENMAN->IsScreenNameValid(sValue)) {
					MAKE_INVALID("screen arg '" + sValue +
								 "' is not a screen name, ScreenOptionsMaster "
								 "list or OptionsList entry.");
				}
			}
		}
		if (!m_bInvalid) {
			m_sScreen = sValue;
		}
	}

	else if (sName == "steps") {
		const auto sSteps = sValue;

		// This must be processed after "song" and "style" commands.
		if (!m_bInvalid) {
			auto* pSong =
			  (m_pSong != nullptr) ? m_pSong : GAMESTATE->m_pCurSong;
			const auto* pStyle = m_pStyle != nullptr
								   ? m_pStyle
								   : GAMESTATE->GetCurrentStyle(
									   GAMESTATE->GetMasterPlayerNumber());
			if (pSong == nullptr || pStyle == nullptr) {
				MAKE_INVALID("Must set Song and Style to set Steps.");
			} else {
				const auto dc = StringToDifficulty(sSteps);
				Steps* st;
				if (dc < Difficulty_Edit) {
					st = SongUtil::GetStepsByDifficulty(
					  pSong, pStyle->m_StepsType, dc);
				} else {
					st = SongUtil::GetStepsByDescription(
					  pSong, pStyle->m_StepsType, sSteps);
				}
				CHECK_INVALID_COND(
				  m_pSteps,
				  st,
				  (st == NULL),
				  (ssprintf("Steps \"%s\" not found", sSteps.c_str())));
			}
		}
	}

	else if (sName == "setenv") {
		if ((cmd.m_vsArgs.size() - 1) % 2 != 0) {
			MAKE_INVALID(
			  "Arguments to setenv game command must be key,value pairs.");
		} else {
			for (size_t i = 1; i < cmd.m_vsArgs.size(); i += 2) {
				m_SetEnv[cmd.m_vsArgs[i]] = cmd.m_vsArgs[i + 1];
			}
		}
	}

	else if (sName == "songgroup") {
		CHECK_INVALID_COND(m_sSongGroup,
						   sValue,
						   (!SONGMAN->DoesSongGroupExist(sValue)),
						   ("Song group \"" + sValue + "\" does not exist."));
	}

	else if (sName == "sort") {
		const auto so = StringToSortOrder(sValue);
		CHECK_INVALID_VALUE(m_SortOrder, so, SortOrder_Invalid, sortorder);
	}

	else if (sName == "profileid") {
		m_sProfileID = sValue;
	}

	else if (sName == "url") {
		m_sUrl = sValue;
		m_bUrlExits = true;
	}

	else if (sName == "sound") {
		m_sSoundPath = sValue;
	}

	else if (sName == "preparescreen") {
		m_vsScreensToPrepare.push_back(sValue);
	}

	else if (sName == "stopmusic") {
		m_bStopMusic = true;
	}

	else if (sName == "applydefaultoptions") {
		m_bApplyDefaultOptions = true;
	}

	// sm-ssc additions begin:
	else if (sName == "urlnoexit") {
		m_sUrl = sValue;
		m_bUrlExits = false;
	}

	else if (sName == "setpref") {
		if ((cmd.m_vsArgs.size() - 1) % 2 != 0) {
			MAKE_INVALID(
			  "Arguments to setpref game command must be key,value pairs.");
		} else {
			for (size_t i = 1; i < cmd.m_vsArgs.size(); i += 2) {
				if (IPreference::GetPreferenceByName(cmd.m_vsArgs[i]) ==
					nullptr) {
					MAKE_INVALID("Unknown preference \"" + cmd.m_vsArgs[i] +
								 "\".");
				} else {
					m_SetPref[cmd.m_vsArgs[i]] = cmd.m_vsArgs[i + 1];
				}
			}
		}
	}

	else if (sName == "fademusic") {
		if (cmd.m_vsArgs.size() == 3) {
			m_bFadeMusic = true;
			m_fMusicFadeOutVolume =
			  static_cast<float>(atof(cmd.m_vsArgs[1].c_str()));
			m_fMusicFadeOutSeconds =
			  static_cast<float>(atof(cmd.m_vsArgs[2].c_str()));
		} else {
			MAKE_INVALID("Wrong number of args to fademusic.");
		}
	}

	else {
		MAKE_INVALID(ssprintf("Command '%s' is not valid.",
							  cmd.GetOriginalCommandString().c_str()));
	}
#undef CHECK_INVALID_VALUE
#undef CHECK_INVALID_COND
#undef MAKE_INVALID
}

bool
GameCommand::IsPlayable(std::string* why) const
{
	if (m_bInvalid) {
		if (why)
			*why = m_sInvalidReason;
		return false;
	}

	if (!CompareNoCase(m_sScreen, "ScreenEditMenu")) {
		if (SONGMAN->GetNumSongs() == 0) {
			if (why)
				*why = "No songs are installed";
			return false;
		}
	}
	return true;
}

void
GameCommand::ApplyToAllPlayers() const
{
	std::vector<PlayerNumber> vpns;

	vpns.push_back(PLAYER_1);

	Apply(vpns);
}

void
GameCommand::Apply(PlayerNumber pn) const
{
	std::vector<PlayerNumber> vpns;
	vpns.push_back(pn);
	Apply(vpns);
}

void
GameCommand::Apply(const std::vector<PlayerNumber>& vpns) const
{
	if (!m_Commands.v.empty()) {
		// We were filled using a GameCommand from metrics. Apply the options in
		// order.
		FOREACH_CONST(Command, m_Commands.v, cmd)
		{
			GameCommand gc;
			gc.m_bInvalid = false;
			gc.m_bApplyCommitsScreens = m_bApplyCommitsScreens;
			gc.LoadOne(*cmd);
			gc.ApplySelf(vpns);
		}
	} else {
		// We were filled by an OptionRowHandler in code. m_Commands isn't
		// filled, so just apply the values that are already set in this.
		this->ApplySelf(vpns);
	}
}

void
GameCommand::ApplySelf(const std::vector<PlayerNumber>& vpns) const
{
	if (m_pStyle != nullptr) {
		GAMESTATE->SetCurrentStyle(m_pStyle,
								   GAMESTATE->GetMasterPlayerNumber());
		// If only one side is joined and we picked a style that requires both
		// sides, join the other side.
		switch (m_pStyle->m_StyleType) {
			case StyleType_OnePlayerOneSide:
			case StyleType_OnePlayerTwoSides:
				break;
			default:
				LuaHelpers::ReportScriptError(ssprintf(
				  "%s%d", "Invalid StyleType: ", m_pStyle->m_StyleType));
		}
	}
	if (m_dc != Difficulty_Invalid)
		FOREACH_CONST(PlayerNumber, vpns, pn)
	GAMESTATE->m_PreferredDifficulty.Set(m_dc);
	if (!m_sAnnouncer.empty())
		ANNOUNCER->SwitchAnnouncer(m_sAnnouncer);
	if (!m_sPreferredModifiers.empty())
		FOREACH_CONST(PlayerNumber, vpns, pn)
	GAMESTATE->ApplyPreferredModifiers(*pn, m_sPreferredModifiers);
	if (!m_sStageModifiers.empty())
		FOREACH_CONST(PlayerNumber, vpns, pn)
	GAMESTATE->ApplyStageModifiers(*pn, m_sStageModifiers);
	if (m_LuaFunction.IsSet() && !m_LuaFunction.IsNil()) {
		auto* L = LUA->Get();
		FOREACH_CONST(PlayerNumber, vpns, pn)
		{
			m_LuaFunction.PushSelf(L);
			ASSERT(!lua_isnil(L, -1));

			lua_pushnumber(L, *pn); // 1st parameter
			std::string error = "Lua GameCommand error: ";
			LuaHelpers::RunScriptOnStack(L, error, 1, 0, true);
		}
		LUA->Release(L);
	}
	if (!m_sScreen.empty() && m_bApplyCommitsScreens)
		SCREENMAN->SetNewScreen(m_sScreen);
	if (m_pSong != nullptr) {
		GAMESTATE->m_pCurSong.Set(m_pSong);
		GAMESTATE->m_pPreferredSong = m_pSong;
	}
	if (m_pSteps)
		GAMESTATE->m_pCurSteps.Set(m_pSteps);
	for (const auto& i : m_SetEnv) {
		auto* L = LUA->Get();
		GAMESTATE->m_Environment->PushSelf(L);
		lua_pushstring(L, i.first.c_str());
		lua_pushstring(L, i.second.c_str());
		lua_settable(L, -3);
		lua_pop(L, 1);
		LUA->Release(L);
	}
	for (const auto& setting : m_SetPref) {
		auto* pref = IPreference::GetPreferenceByName(setting.first);
		if (pref != nullptr) {
			pref->FromString(setting.second);
		}
	}
	if (!m_sSongGroup.empty())
		GAMESTATE->m_sPreferredSongGroup.Set(m_sSongGroup);
	if (m_SortOrder != SortOrder_Invalid)
		GAMESTATE->m_PreferredSortOrder = m_SortOrder;
	if (!m_sSoundPath.empty())
		SOUND->PlayOnce(THEME->GetPathS("", m_sSoundPath));
	if (!m_sProfileID.empty())
		FOREACH_CONST(PlayerNumber, vpns, pn)
	ProfileManager::m_sDefaultLocalProfileID[*pn].Set(m_sProfileID);
	if (!m_sUrl.empty()) {
		if (Core::Platform::openWebsite(m_sUrl)) {
			if (m_bUrlExits)
				SCREENMAN->SetNewScreen("ScreenExit");
		} else
			ScreenPrompt::Prompt(SM_None, COULD_NOT_LAUNCH_BROWSER);
	}

	/* If we're going to stop music, do so before preparing new screens, so we
	 * don't stop music between preparing screens and loading screens. */
	if (m_bStopMusic)
		SOUND->StopMusic();
	if (m_bFadeMusic)
		SOUND->DimMusic(m_fMusicFadeOutVolume, m_fMusicFadeOutSeconds);

	FOREACH_CONST(std::string, m_vsScreensToPrepare, s)
	SCREENMAN->PrepareScreen(*s);

	if (m_bApplyDefaultOptions) {
		// applying options affects only the current stage
		PlayerOptions po;
		GAMESTATE->GetDefaultPlayerOptions(po);
		GAMESTATE->m_pPlayerState->m_PlayerOptions.Assign(ModsLevel_Stage, po);

		SongOptions so;
		GAMESTATE->GetDefaultSongOptions(so);
		GAMESTATE->m_SongOptions.Assign(ModsLevel_Stage, so);
	}
}

bool
GameCommand::IsZero() const
{
	if (m_pStyle != nullptr || m_dc != Difficulty_Invalid ||
		!m_sAnnouncer.empty() || !m_sPreferredModifiers.empty() ||
		!m_sStageModifiers.empty() || m_pSong != nullptr ||
		m_pSteps != nullptr || !m_sSongGroup.empty() ||
		m_SortOrder != SortOrder_Invalid || !m_sProfileID.empty() ||
		!m_sUrl.empty())
		return false;

	return true;
}

// lua start
#include "Game.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"

/** @brief Allow Lua to have access to the GameCommand. */
class LunaGameCommand : public Luna<GameCommand>
{
  public:
	static int GetName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sName.c_str());
		return 1;
	}
	static int GetText(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sText.c_str());
		return 1;
	}
	static int GetIndex(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iIndex);
		return 1;
	}
	static int GetMultiPlayer(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_MultiPlayer);
		return 1;
	}
	static int GetStyle(T* p, lua_State* L)
	{
		if (p->m_pStyle == nullptr)
			lua_pushnil(L);
		else {
			auto* pStyle = (Style*)p->m_pStyle;
			pStyle->PushSelf(L);
		}
		return 1;
	}
	static int GetScreen(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sScreen.c_str());
		return 1;
	}
	static int GetProfileID(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sProfileID.c_str());
		return 1;
	}
	static int GetSong(T* p, lua_State* L)
	{
		if (p->m_pSong == nullptr)
			lua_pushnil(L);
		else
			p->m_pSong->PushSelf(L);
		return 1;
	}
	static int GetSteps(T* p, lua_State* L)
	{
		if (p->m_pSteps == nullptr)
			lua_pushnil(L);
		else
			p->m_pSteps->PushSelf(L);
		return 1;
	}
	static int GetSongGroup(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sSongGroup.c_str());
		return 1;
	}
	static int GetUrl(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sUrl.c_str());
		return 1;
	}
	static int GetAnnouncer(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sAnnouncer.c_str());
		return 1;
	}
	static int GetPreferredModifiers(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sPreferredModifiers.c_str());
		return 1;
	}
	static int GetStageModifiers(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sStageModifiers.c_str());
		return 1;
	}

	DEFINE_METHOD(GetDifficulty, m_dc)
	DEFINE_METHOD(GetSortOrder, m_SortOrder)

	LunaGameCommand()
	{
		ADD_METHOD(GetName);
		ADD_METHOD(GetText);
		ADD_METHOD(GetIndex);
		ADD_METHOD(GetMultiPlayer);
		ADD_METHOD(GetStyle);
		ADD_METHOD(GetDifficulty);
		ADD_METHOD(GetScreen);
		ADD_METHOD(GetProfileID);
		ADD_METHOD(GetSong);
		ADD_METHOD(GetSteps);
		ADD_METHOD(GetSongGroup);
		ADD_METHOD(GetSortOrder);
		ADD_METHOD(GetUrl);
		ADD_METHOD(GetAnnouncer);
		ADD_METHOD(GetPreferredModifiers);
		ADD_METHOD(GetStageModifiers);
	}
};

LUA_REGISTER_CLASS(GameCommand)
// lua end
