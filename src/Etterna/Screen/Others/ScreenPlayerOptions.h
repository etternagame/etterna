#ifndef SCREENPLAYEROPTIONS_H
#define SCREENPLAYEROPTIONS_H

#include "Etterna/Screen/Options/ScreenOptionsMaster.h"

struct lua_State;

class ScreenPlayerOptions : public ScreenOptionsMaster
{
  public:
	void Init() override;
	void BeginScreen() override;

	bool Input(const InputEventPlus& input) override;
	void HandleScreenMessage(const ScreenMessage& SM) override;
	bool GetGoToOptions() const { return m_bGoToOptions; }

	// Lua
	void PushSelf(lua_State* L) override;

  private:
	vector<bool> m_bRowCausesDisqualified;
	void UpdateDisqualified(int row, PlayerNumber pn);

	bool m_bAcceptedChoices;
	bool m_bGoToOptions;
	bool m_bAskOptionsMessage;

	// show if the current selections will disqualify a high score
	AutoActor m_sprDisqualify;
};

#endif
