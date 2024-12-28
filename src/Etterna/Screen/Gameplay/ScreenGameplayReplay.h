#ifndef ScreenGameplayReplay_H
#define ScreenGameplayReplay_H

#include "ScreenGameplay.h"

class ScreenGameplayReplay : public ScreenGameplay
{
  public:
	void FillPlayerInfo(PlayerInfo* playerInfoOut) override;
	ScreenGameplayReplay();
	void Init() override;
	~ScreenGameplayReplay() override;

	void Update(float fDeltaTime) override;
	bool Input(const InputEventPlus& input) override;

	// Lua
	void PushSelf(lua_State* L) override;
	PlayerInfo* GetPlayerInfo(PlayerNumber pn);

	// void BeginBackingOutFromGameplay();

	// Set the playback rate in the middle of gameplay
	float SetRate(float newRate);
	// Move the current position of the song in the middle of gameplay
	void SetSongPosition(float newPositionSeconds);
	// Toggle pause
	void TogglePause();
	float m_fReplayBookmarkSeconds;

  protected:
	void SaveStats() override;
	void StageFinished(bool bBackedOut) override;
	void LoadPlayer() override;
	void ReloadPlayer() override;
	void LoadScoreKeeper() override;
};

#endif
