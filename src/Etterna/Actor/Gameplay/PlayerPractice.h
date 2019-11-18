#ifndef PlayerPractice_H
#define PlayerPractice_H

#include "Player.h"

// Player derivative meant to ignore useless stuff
class PlayerPractice : public Player
{
  public:
	PlayerPractice(NoteData& nd, bool bVisibleParts = true);
	~PlayerPractice() override;

	void Init(const std::string& sType,
			  PlayerState* pPlayerState,
			  PlayerStageStats* pPlayerStageStats,
			  LifeMeter* pLM,
			  ScoreKeeper* pPrimaryScoreKeeper) override;
	void Update(float fDeltaTime) override;

	void Step(int col,
			  int row,
			  const std::chrono::steady_clock::time_point& tm,
			  bool bHeld,
			  bool bRelease,
			  float padStickSeconds = 0.0f) override;

	// When called, resets stage stats and necessary things
	// Also sets countStats to false.
	void PositionReset();

  private:
	// Becomes true immediately on first button press.
	// Allows notes to pass without counting for anything.
	bool countStats = false;
};

#endif
