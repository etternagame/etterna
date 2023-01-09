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
						 std::vector<TrackRowTapNote>& vTN) override;
	void Init(const std::string& sType,
			  PlayerState* pPlayerState,
			  PlayerStageStats* pPlayerStageStats,
			  LifeMeter* pLM,
			  ScoreKeeper* pPrimaryScoreKeeper) override;
	void Load() override;
	void Reload() override;
	void Update(float fDeltaTime) override;
	void CrossedRows(int iLastrowCrossed,
					 const std::chrono::steady_clock::time_point& now) override;
	void Step(int col,
			  int steppedRow,
			  const std::chrono::steady_clock::time_point& tm,
			  bool bHeld,
			  bool bRelease,
			  float padStickSeconds = 0.0F,
			  int rowToJudge = -1,
			  float forcedSongPositionSeconds = 0.0F);

	void UpdateLoadedReplay(int startRow = 0);

	std::map<int, std::vector<PlaybackEvent>>& GetPlaybackEvents() {
		return playbackEvents;
	}
	void SetPlaybackEvents(const std::map<int, std::vector<PlaybackEvent>>& v);

	std::map<int, std::set<int>>& GetDroppedHolds() {
		return droppedHolds;
	}
	void SetDroppedHolds(const std::map<int, std::set<int>>& v) {
		droppedHolds = v;
	}

  protected:
	void UpdateHoldsAndRolls(
	  float fDeltaTime,
	  const std::chrono::steady_clock::time_point& now) override;
	void HandleTapRowScore(unsigned row) override;
	void UpdateTapNotesMissedOlderThan(float fMissIfOlderThanSeconds) override;
	void UpdatePressedFlags() override;
	void CheckForSteps(const std::chrono::steady_clock::time_point& tm);

	std::map<int, std::vector<PlaybackEvent>> playbackEvents{};
	std::map<int, std::set<int>> droppedHolds{};
	std::set<int> holdingColumns{};
};

#endif
