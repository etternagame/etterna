/** @brief PlayerState - Holds per-player game state. */

#ifndef PlayerState_H
#define PlayerState_H

#include "ModsGroup.h"
#include "PlayerNumber.h"
#include "PlayerOptions.h"
#include "SampleHistory.h"
#include "Etterna/Models/Songs/SongPosition.h"

struct lua_State;

struct CacheDisplayedBeat
{
	float beat;
	float displayedBeat;
	float velocity;
};

struct CacheNoteStat
{
	float beat;
	int notesLower;
	int notesUpper;
};

/** @brief The player's indivdual state. */
class PlayerState
{
  public:
	/** @brief Set up the PlayerState with initial values. */
	PlayerState();
	/** @brief Reset the PlayerState with the initial values. */
	void Reset();
	/**
	 * @brief Update the PlayerState based on the present time.
	 * @param fDelta the current time. */
	void Update(float fDelta);

	void SetPlayerNumber(PlayerNumber pn);

	/**
	 * @brief The PlayerNumber assigned to this Player: usually 1 or 2.
	 *
	 * TODO: Remove use of PlayerNumber.  All data about the player should live
	 * in PlayerState and callers should not use PlayerNumber to index into
	 * GameState. */
	PlayerNumber m_PlayerNumber;
	/**
	 * @brief The MultiPlayer number assigned to this Player, typically 1-32.
	 */
	MultiPlayer m_mp;

	// This is used by ArrowEffects and the NoteField to zoom both appropriately
	// to fit in the space available. -Kyz
	float m_NotefieldZoom;

	// Music statistics:
	SongPosition m_Position;

	const SongPosition& GetDisplayedPosition() const;
	const TimingData& GetDisplayedTiming() const;

	/**
	 * @brief Holds a vector sorted by real beat, the beat that would be
	 * displayed in the NoteField (because they are affected by scroll
	 * segments), and also the velocity. This vector will be populated on
	 * Player::Load() be used a lot in ArrowEffects to determine the
	 * target beat in O(log N).
	 */
	vector<CacheDisplayedBeat> m_CacheDisplayedBeat;

	/**
	 * @brief Holds a vector sorted by beat, the cumulative number of notes from
	 *        the start of the song. This will be used by [insert more
	 * description here]
	 */
	vector<CacheNoteStat> m_CacheNoteStat;

	/**
	 * @brief Change the PlayerOptions to their default.
	 * @param l the level of mods to reset.
	 */
	void ResetToDefaultPlayerOptions(ModsLevel l);
	/** @brief The PlayerOptions presently in use by the Player. */
	ModsGroup<PlayerOptions> m_PlayerOptions;

	/**
	 * @brief Used to push note-changing modifiers back so that notes don't pop.
	 *
	 * This is used during gameplay and set by NoteField. */
	mutable float m_fLastDrawnBeat;
	/** @brief The Player's HealthState in general terms. */
	HealthState m_HealthState;

	/** @brief The type of person/machine controlling the Player. */
	PlayerController m_PlayerController;

	SampleHistory m_EffectHistory;

	int m_iCpuSkill; // only used when m_PlayerController is PC_CPU

	// Stores the bpm that was picked for reading the chart if the player is
	// using an mmod.
	float m_fReadBPM;

	/* why is the slow getstyles function called every time to get
	number of columns in places where it can't change? - Mina */
	float playertargetgoal = 0.93f;
	int m_NumCols;
	void SetNumCols(int ncol) { m_NumCols = ncol; };
	int GetNumCols() { return m_NumCols; };
	float wtFFF = 1.f; // lol dont ask - mina

	// Lua
	void PushSelf(lua_State* L);
};

#endif

/**
 * @file
 * @author Chris Danford, Chris Gomez (c) 2001-2004
 * @section LICENSE
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
