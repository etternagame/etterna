#ifndef ScreenGameplaySpectate_H
#define ScreenGameplaySpectate_H

#include "ScreenGameplay.h"

class ScreenGameplaySpectate : public ScreenGameplay
{
  public:
	void FillPlayerInfo(PlayerInfo* playerInfoOut) override;
	ScreenGameplaySpectate();
	void Init() override;
	~ScreenGameplaySpectate() override;

	void Update(float fDeltaTime) override;
	bool Input(const InputEventPlus& input) override;

	void HandleScreenMessage(const ScreenMessage& SM) override;

	// Lua
	void PushSelf(lua_State* L) override;
	PlayerInfo* GetPlayerInfo(PlayerNumber pn);

  protected:
	void SaveStats() override;
	void StageFinished(bool bBackedOut) override;
	void LoadPlayer() override;
	void LoadScoreKeeper() override;
};

#endif
