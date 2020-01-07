/* ScreenGameplay - The music plays, the notes scroll, and the Player is
 * pressing buttons. */

#ifndef ScreenGameplayNormal_H
#define ScreenGameplayNormal_H

#include "ScreenGameplay.h"

class ScreenGameplayNormal : public ScreenGameplay
{
  public:
	virtual void FillPlayerInfo(PlayerInfo* vPlayerInfoOut);
};

#endif
