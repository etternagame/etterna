#include "Etterna/Globals/global.h"
#include "ScreenGameplayNormal.h"
#include "Etterna/Models/Misc/Difficulty.h"

REGISTER_SCREEN_CLASS(ScreenGameplayNormal);

void
ScreenGameplayNormal::FillPlayerInfo(std::vector<PlayerInfo>& vPlayerInfoOut)
{
	vPlayerInfoOut.clear();

	vPlayerInfoOut.push_back(PlayerInfo());

	vPlayerInfoOut[0].Load(
	  PLAYER_1, MultiPlayer_Invalid, true, Difficulty_Invalid);
};

