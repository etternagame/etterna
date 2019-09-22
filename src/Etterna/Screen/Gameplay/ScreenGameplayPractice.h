#ifndef ScreenGameplayPractice_H
#define ScreenGameplayPractice_H

#include "ScreenGameplay.h"

class ScreenGameplayPractice : public ScreenGameplay
{
  public:
	virtual void FillPlayerInfo(PlayerInfo* playerInfoOut);
	ScreenGameplayPractice();
	void Init() override;
	~ScreenGameplayPractice() override;

	void Update(float fDeltaTime) override;
	// bool Input(const InputEventPlus& input) override;

	// Lua
	void PushSelf(lua_State* L) override;
	LifeMeter* GetLifeMeter(PlayerNumber pn);
	PlayerInfo* GetPlayerInfo(PlayerNumber pn);

	void FailFadeRemovePlayer(PlayerInfo* pi);
	void FailFadeRemovePlayer(PlayerNumber pn);
	// void BeginBackingOutFromGameplay();

	// Set the playback rate in the middle of gameplay
	float SetRate(float newRate);
	// Set the playback rate in the middle of gameplay, in practice mode only
	float AddToPracticeRate(float amountAdded);
	// Move the current position of the song in the middle of gameplay, in
	// practice mode only
	void SetPracticeSongPosition(float newPositionSeconds);
	// Toggle pause
	void TogglePracticePause();
};

#endif
