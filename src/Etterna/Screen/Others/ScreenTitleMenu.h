/* ScreenTitleMenu - The main title screen and menu. */

#ifndef SCREEN_TITLE_MENU_H
#define SCREEN_TITLE_MENU_H

#include "ScreenSelectMaster.h"

class ScreenTitleMenu : public ScreenSelectMaster
{
  public:
	ScreenTitleMenu();
	void Init() override;

	bool Input(const InputEventPlus& input) override;

	void HandleMessage(const Message& msg) override;
};

#endif
