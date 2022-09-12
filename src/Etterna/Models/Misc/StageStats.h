#ifndef StageStats_H
#define StageStats_H

#include "PlayerNumber.h"
#include "PlayerStageStats.h"
class Song;
class Style;
struct lua_State;

/**
 * @brief Contains statistics for one stage of play.
 *
 * This is either one song, or a whole course. */
class StageStats
{
  public:
	StageStats();
	void Init();

	/**
	 * @brief Ensure that the Player is valid.
	 * @param pn the PlayerNumber to check. */
	void AssertValid(PlayerNumber pn) const;

	/**
	 * @brief Ensure that the Player is valid.
	 * @param mp the Multiplayer to check. */
	void AssertValid(MultiPlayer pn) const;

	void AddStats(const StageStats& other); // accumulate

	[[nodiscard]] auto Failed() const -> bool;

	[[nodiscard]] auto GetAverageMeter(PlayerNumber pn) const -> int;

	Stage m_Stage;
	int m_iStageIndex;
	std::vector<Song*> m_vpPlayedSongs;
	std::vector<Song*> m_vpPossibleSongs;

	/** @brief Was the gameplay exited by the Player giving up? */
	bool m_bGaveUp;
	/** @brief Did the PLayer use Autoplay at any point during gameplay? */
	bool m_bUsedAutoplay;

	/** @brief How fast was the music going compared to normal? */
	float m_fMusicRate;

	// Total number of seconds between first beat and last beat for every song.
	[[nodiscard]] auto GetTotalPossibleStepsSeconds() const -> float;

	PlayerStageStats m_player;
	PlayerStageStats m_multiPlayer[NUM_MultiPlayer];

	void FinalizeScores();
	std::string mostrecentscorekey;

	// Show that this StageStats was a live play or is merely a reproduction
	// using a Replay
	bool m_bLivePlay = false;

	[[nodiscard]] auto GetMinimumMissCombo() const -> unsigned int;

	// Lua
	void PushSelf(lua_State* L);

  private:
	// TODO(Sam): Implement the copy and assignment operators on our own.
};

#endif
