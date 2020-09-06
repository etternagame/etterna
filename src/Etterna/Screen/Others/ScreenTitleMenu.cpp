#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenTitleMenu.h"

#define COIN_MODE_CHANGE_SCREEN                                                \
	THEME->GetMetric(m_sName, "CoinModeChangeScreen")

REGISTER_SCREEN_CLASS(ScreenTitleMenu);
ScreenTitleMenu::ScreenTitleMenu()
{
	/* XXX We really need two common calls:
	 * 1) something run when exiting from gameplay (to do this reset),
	 * and 2) something run when entering gameplay, to apply default options.
	 * Having special cases in attract screens and the title menu to reset
	 * things stinks ...
	 * [aj] Well... Can't #2 be done in Lua nowadays? and #1 as well? (hi shake)
	 */
	GAMESTATE->Reset();
}

void
ScreenTitleMenu::Init()
{
	ScreenSelectMaster::Init();

	SOUND->PlayOnceFromAnnouncer("title menu game name");
}

static LocalizedString THEME_("ScreenTitleMenu", "Theme");
static LocalizedString ANNOUNCER_("ScreenTitleMenu", "Announcer");
bool
ScreenTitleMenu::Input(const InputEventPlus& input)
{
#if defined(DEBUG)
	Locator::getLogger()->trace("ScreenTitleMenu::Input( {}-{} )",
			   input.DeviceI.device, input.DeviceI.button); // debugging gameport joystick problem
#endif

	if (m_In.IsTransitioning() || m_Cancel.IsTransitioning()) /* not m_Out */
		return false;

	bool bHandled = false;

	return ScreenSelectMaster::Input(input) || bHandled;
}

void
ScreenTitleMenu::HandleMessage(const Message& msg)
{
	ScreenSelectMaster::HandleMessage(msg);
}
