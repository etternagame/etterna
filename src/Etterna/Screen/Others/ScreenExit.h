#ifndef SCREEN_EXIT_H
#define SCREEN_EXIT_H

#include "Screen.h"

class ScreenExit : public Screen
{
  public:
	void Init() override;

  private:
	bool m_Exited = false;
};

#endif
