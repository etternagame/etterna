#ifndef ScreenGameplayReplay_H
#define ScreenGameplayReplay_H

#include "ScreenGameplay.h"

class ScreenGameplayReplay : public ScreenGameplay
{
  public:
	virtual void FillPlayerInfo(PlayerInfo* playerInfoOut);
};

#endif
