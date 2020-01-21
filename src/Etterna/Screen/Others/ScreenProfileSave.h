#ifndef SCREEN_PROFILE_SAVE_H
#define SCREEN_PROFILE_SAVE_H

#include "ScreenWithMenuElements.h"

class ScreenProfileSave : public ScreenWithMenuElements
{
  public:
	void BeginScreen() override;
	bool Input(const InputEventPlus& input) override;
	void Continue();

	void PushSelf(lua_State* L) override;
};

#endif
