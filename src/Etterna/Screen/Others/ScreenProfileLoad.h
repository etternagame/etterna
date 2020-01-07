#ifndef SCREEN_PROFILE_LOAD_H
#define SCREEN_PROFILE_LOAD_H

#include "ScreenWithMenuElements.h"

class ScreenProfileLoad : public ScreenWithMenuElements
{
  public:
	void Init() override;
	void BeginScreen() override;
	bool Input(const InputEventPlus& input) override;
	void Continue();

	void PushSelf(lua_State* L) override;

	bool m_bHaveProfileToLoad;

	ThemeMetric<bool> LOAD_EDITS;
};

#endif
