#ifndef SONGPOSITION_H
#define SONGPOSITION_H

#include "RageUtil/Misc/RageTimer.h"
#include "Etterna/Models/Misc/TimingData.h"
// XXX: where does this come from? might need another include
struct lua_State;

class SongPosition
{
  public:
	// Arcade - the current stage (one song).
	// Oni/Endless - a single song in a course.
	// Let a lot of classes access this info here so they don't have to keep
	// their own copies.
	float
	  m_fMusicSeconds{}; // time into the current song, not scaled by music rate
	float m_fSongBeat{};
	float m_fSongBeatNoOffset{};
	float m_fCurBPS{};
	// bool		m_bStop;	// in the middle of a stop (freeze or delay)
	/** @brief A flag to determine if we're in the middle of a freeze/stop. */
	bool m_bFreeze{};
	/** @brief A flag to determine if we're in the middle of a delay (Pump style
	 * stop). */
	bool m_bDelay{};
	/** @brief The row used to start a warp. */
	int m_iWarpBeginRow{};
	/** @brief The beat to warp to afterwards. */
	float m_fWarpDestination{};
	RageTimer m_LastBeatUpdate; // time of last m_fSongBeat, etc. update
	float m_fMusicSecondsVisible{};
	float m_fSongBeatVisible{};

	void Reset();
	void UpdateSongPosition(float fPositionSeconds,
							const TimingData& timing,
							const RageTimer& timestamp = RageZeroTimer);

	// Lua
	void PushSelf(lua_State* L);
};

#endif
