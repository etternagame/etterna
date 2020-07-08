/* ScreenSelectProfile - Screen that allows to select and load profile to use.
 */

#ifndef SCREEN_SELECT_PROFILE_H
#define SCREEN_SELECT_PROFILE_H

#include "ScreenWithMenuElements.h"

class ScreenSelectProfile : public ScreenWithMenuElements
{
  public:
	void Init() override;
	bool Input(const InputEventPlus& input) override;
	bool MenuLeft(const InputEventPlus& input) override;
	bool MenuRight(const InputEventPlus& input) override;
	bool MenuUp(const InputEventPlus& input) override;
	bool MenuDown(const InputEventPlus& input) override;
	void HandleScreenMessage(const ScreenMessage& SM) override;

	GameButton m_TrackingRepeatingInput;

	// Lua
	void PushSelf(lua_State* L) override;
	bool SetProfileIndex(PlayerNumber pn, int iProfileIndex);
	int GetProfileIndex(PlayerNumber pn) { return m_iSelectedProfiles; }
	bool Finish();

  protected:
	int m_iSelectedProfiles;
};

#endif
