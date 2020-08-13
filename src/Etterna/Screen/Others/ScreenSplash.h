/* ScreenSplash - A loading screen. */

#ifndef ScreenSplash_H
#define ScreenSplash_H

#include "ScreenWithMenuElements.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

class ScreenSplash : public ScreenWithMenuElements
{
  public:
	void Init() override;
	void BeginScreen() override;

	void HandleScreenMessage(const ScreenMessage& SM) override;
	bool MenuBack(const InputEventPlus& input) override;
	bool MenuStart(const InputEventPlus& input) override;

  protected:
	ThemeMetric<bool> ALLOW_START_TO_SKIP;
	ThemeMetric<bool> PREPARE_SCREEN;
};

#endif
