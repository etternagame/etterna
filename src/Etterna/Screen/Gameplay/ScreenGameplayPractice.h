#ifndef ScreenGameplayPractice_H
#define ScreenGameplayPractice_H

#include "ScreenGameplay.h"

class ScreenGameplayPractice : public ScreenGameplay
{
  public:
	virtual void FillPlayerInfo(PlayerInfo* playerInfoOut);
};

#endif
