#include "Etterna/Globals/global.h"
#include "ScreenGameplayPractice.h"
#include "Etterna/Models/Misc/Difficulty.h"

REGISTER_SCREEN_CLASS(ScreenGameplayPractice);

void
ScreenGameplayPractice::FillPlayerInfo(PlayerInfo* playerInfoOut)
{
	playerInfoOut->Load(
	  PLAYER_1, MultiPlayer_Invalid, true, Difficulty_Invalid);
}
