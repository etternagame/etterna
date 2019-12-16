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
	bool Input(const InputEventPlus& input) override;

	// Lua
	void PushSelf(lua_State* L) override;
	LifeMeter* GetLifeMeter(PlayerNumber pn);
	PlayerInfo* GetPlayerInfo(PlayerNumber pn);

	void FailFadeRemovePlayer(PlayerInfo* pi);
	void FailFadeRemovePlayer(PlayerNumber pn);
	// void BeginBackingOutFromGameplay();

	// Set the playback rate in the middle of gameplay
	float SetRate(float newRate);
	// Add to the playback rate in the middle of gameplay
	float AddToRate(float amountAdded);
	// Move the current position of the song in the middle of gameplay
	void SetSongPosition(float newSongPositionSeconds,
						 float noteDelay = 0.f,
						 bool hardSeek = false,
						 bool unpause = false);
	// Toggle pause
	void TogglePause();

	// Practice Looper
	void SetLoopRegion(float start, float end);
	void ResetLoopRegion();

  protected:
	void SetupNoteDataFromRow(Steps* pSteps,
							  int minRow = 0,
							  int maxrow = MAX_NOTE_ROW);

  private:
	float loopStart = -2000.f;
	float loopEnd = -2000.f;
	float lastReportedSeconds = -2000.f;
};

#endif
