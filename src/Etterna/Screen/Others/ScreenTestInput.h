/* ScreenTestInput - Display pressed keys. */

#ifndef SCREEN_TEST_INPUT_H
#define SCREEN_TEST_INPUT_H

#include "ScreenWithMenuElements.h"

class ScreenTestInput : public ScreenWithMenuElements
{
  public:
	bool Input(const InputEventPlus& input) override;

	bool MenuStart(const InputEventPlus& input) override;
	bool MenuBack(const InputEventPlus& input) override;
};

#endif
