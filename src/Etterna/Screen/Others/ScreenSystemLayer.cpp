#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenSystemLayer.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

namespace {
LocalizedString CREDITS_PRESS_START("ScreenSystemLayer", "CreditsPressStart");
LocalizedString CREDITS_INSERT_CARD("ScreenSystemLayer", "CreditsInsertCard");
LocalizedString CREDITS_CARD_TOO_LATE("ScreenSystemLayer",
									  "CreditsCardTooLate");
LocalizedString CREDITS_CARD_NO_NAME("ScreenSystemLayer", "CreditsCardNoName");
LocalizedString CREDITS_CARD_READY("ScreenSystemLayer", "CreditsCardReady");
LocalizedString CREDITS_CARD_CHECKING("ScreenSystemLayer",
									  "CreditsCardChecking");
LocalizedString CREDITS_CARD_REMOVED("ScreenSystemLayer", "CreditsCardRemoved");
LocalizedString CREDITS_FREE_PLAY("ScreenSystemLayer", "CreditsFreePlay");
LocalizedString CREDITS_CREDITS("ScreenSystemLayer", "CreditsCredits");
LocalizedString CREDITS_MAX("ScreenSystemLayer", "CreditsMax");
LocalizedString CREDITS_NOT_PRESENT("ScreenSystemLayer", "CreditsNotPresent");
LocalizedString CREDITS_LOAD_FAILED("ScreenSystemLayer", "CreditsLoadFailed");
LocalizedString CREDITS_LOADED_FROM_LAST_GOOD_APPEND(
  "ScreenSystemLayer",
  "CreditsLoadedFromLastGoodAppend");

ThemeMetric<bool> CREDITS_JOIN_ONLY("ScreenSystemLayer", "CreditsJoinOnly");

RString
GetCreditsMessage(PlayerNumber pn)
{
	if ((bool)CREDITS_JOIN_ONLY && !GAMESTATE->PlayersCanJoin())
		return RString();

	bool bShowCreditsMessage;
	if ((SCREENMAN != nullptr) && (SCREENMAN->GetTopScreen() != nullptr) &&
		SCREENMAN->GetTopScreen()->GetScreenType() == system_menu)
		bShowCreditsMessage = true;
	else
		bShowCreditsMessage = !GAMESTATE->m_bSideIsJoined;

	if (!bShowCreditsMessage) {
		const Profile* pProfile = PROFILEMAN->GetProfile(pn);
		// this is a local machine profile
		if (PROFILEMAN->LastLoadWasFromLastGood(pn))
			return pProfile->GetDisplayNameOrHighScoreName() +
				   CREDITS_LOADED_FROM_LAST_GOOD_APPEND.GetValue();
		else if (PROFILEMAN->LastLoadWasTamperedOrCorrupt(pn))
			return CREDITS_LOAD_FAILED.GetValue();
		// Prefer the name of the profile over the name of the card.
		else if (PROFILEMAN->IsPersistentProfile(pn))
			return pProfile->GetDisplayNameOrHighScoreName();
		else if (GAMESTATE->PlayersCanJoin())
			return CREDITS_INSERT_CARD.GetValue();
		else
			return RString();
	}
	return RString();
}
};

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

namespace {
int
GetCreditsMessage(lua_State* L)
{
	PlayerNumber pn = PLAYER_1;
	RString sText = GetCreditsMessage(pn);
	LuaHelpers::Push(L, sText);
	return 1;
}

const luaL_Reg ScreenSystemLayerHelpersTable[] = { LIST_METHOD(
													 GetCreditsMessage),
												   { NULL, NULL } };
} // namespace

LUA_REGISTER_NAMESPACE(ScreenSystemLayerHelpers)

REGISTER_SCREEN_CLASS(ScreenSystemLayer);
void
ScreenSystemLayer::Init()
{
	Screen::Init();

	m_sprOverlay.Load(THEME->GetPathB(m_sName, "overlay"));
	this->AddChild(m_sprOverlay);
	m_errLayer.Load(THEME->GetPathB(m_sName, "error"));
	this->AddChild(m_errLayer);
}
