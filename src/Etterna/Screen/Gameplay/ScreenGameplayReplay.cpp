#include "Etterna/Globals/global.h"
#include "ScreenGameplayReplay.h"
#include "Etterna/Models/Misc/Difficulty.h"

REGISTER_SCREEN_CLASS(ScreenGameplayReplay);

void
ScreenGameplayReplay::FillPlayerInfo(PlayerInfo* playerInfoOut)
{
	playerInfoOut->Load(
	  PLAYER_1, MultiPlayer_Invalid, true, Difficulty_Invalid);
}
