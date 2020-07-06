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
	void AssertValid(MultiPlayer mp) const;

	void AddStats(const StageStats& other); // accumulate

	[[nodiscard]] bool OnePassed() const;
	[[nodiscard]] bool AllFailed() const;

	[[nodiscard]] int GetAverageMeter(PlayerNumber pn) const;

	Stage m_Stage;
	int m_iStageIndex;
	PlayMode m_playMode;
	vector<Song*> m_vpPlayedSongs;
	vector<Song*> m_vpPossibleSongs;

	/** @brief Was the gameplay exited by the Player giving up? */
	bool m_bGaveUp;
	/** @brief Did the PLayer use Autoplay at any point during gameplay? */
	bool m_bUsedAutoplay;

	// TODO: These are updated in ScreenGameplay::Update based on fDelta.
	// They should be made more accurate.
	/**
	 * @brief How many seconds were there before gameplay ended?
	 *
	 * This is updated by Gameplay, and not scaled by the music rate. */
	float m_fGameplaySeconds;
	/**
	 * @brief How many seconds are we in a song?
	 *
	 * This is equivalent to m_fGameplaySeconds unless the song has steps past
	 * the end. */
	float m_fStepsSeconds;
	/** @brief How fast was the music going compared to normal? */
	float m_fMusicRate;

	// Total number of seconds between first beat and last beat for every song.
	[[nodiscard]] float GetTotalPossibleStepsSeconds() const;

	PlayerStageStats m_player;
	PlayerStageStats m_multiPlayer[NUM_MultiPlayer];

	void FinalizeScores(bool bSummary);
	string mostrecentscorekey;

	// Show that this StageStats was a live play or is merely a reproduction
	// using a Replay
	bool m_bLivePlay = false;

	/**
	 * @brief Determine if the PlayerNumber has a high score.
	 * @param pn the PlayerNumber in question.
	 * @return true if the PlayerNumber has a high score, false otherwise. */
	[[nodiscard]] bool PlayerHasHighScore(PlayerNumber pn) const;
	[[nodiscard]] unsigned int GetMinimumMissCombo() const;

	// Lua
	void PushSelf(lua_State* L);

  private:
	// TODO: Implement the copy and assignment operators on our own.
};

#endif
