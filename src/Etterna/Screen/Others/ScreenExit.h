#ifndef SCREEN_EXIT_H
#define SCREEN_EXIT_H

#include "RageUtil/Misc/RageTimer.h"
#include "Screen.h"

class ScreenExit : public Screen
{
  public:
	void Init() override;

  private:
	bool m_Exited;
};

#endif
