#ifndef GameplayAssist_H
#define GameplayAssist_H

#include "RageUtil/Sound/RageSound.h"

class PlayerState;
class NoteData;

/** @brief The handclaps and metronomes ready to assist the player. */
class GameplayAssist
{
  public:
	/** @brief Load the sounds. */
	void Init();
	/**
	 * @brief Play the sounds in question for the particular chart.
	 * @param nd the note data used for playing the ticks.
	 * @param ps the player's state (and number) for Split Timing. */
	void PlayTicks(const NoteData& nd, const PlayerState* ps);
	/** @brief Stop playing the sounds. */
	void StopPlaying();

  private:
	/** @brief the sound made when a note is to be hit. */
	RageSound m_soundAssistClap;
	/** @brief the sound made when crossing a new measure. */
	RageSound m_soundAssistMetronomeMeasure;
	/** @brief the sound made when crossing a new beat. */
	RageSound m_soundAssistMetronomeBeat;
};

#endif
