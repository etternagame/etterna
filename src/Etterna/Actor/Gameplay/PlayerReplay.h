#ifndef PlayerReplay_H
#define PlayerReplay_H

#include "Player.h"

// Player derivative meant to ignore useless stuff
class PlayerReplay : public Player
{
  public:
	PlayerReplay(NoteData& nd, bool bVisibleParts = true);
	~PlayerReplay() override;

	void UpdateHoldNotes(int iSongRow,
						 float fDeltaTime,
						 vector<TrackRowTapNote>& vTN) override;
	void Init(const std::string& sType,
			  PlayerState* pPlayerState,
			  PlayerStageStats* pPlayerStageStats,
			  LifeMeter* pLM,
			  ScoreKeeper* pPrimaryScoreKeeper) override;
	void Load() override;
	void Update(float fDeltaTime) override;
	void CrossedRows(int iLastrowCrossed,
					 const std::chrono::steady_clock::time_point& now) override;
	void Step(int col,
			  int row,
			  const std::chrono::steady_clock::time_point& tm,
			  bool bHeld,
			  bool bRelease,
			  float padStickSeconds = 0.0f,
			  int rowToJudge = -1);

  protected:
	void UpdateHoldsAndRolls(float fDeltaTime,
							 const std::chrono::steady_clock::time_point& now);
	void HandleTapRowScore(unsigned row);
	void UpdateTapNotesMissedOlderThan(float fMissIfOlderThanSeconds);
};

#endif
