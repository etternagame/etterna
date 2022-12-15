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
	void ResetCacheInfo(/*const NoteData& notes*/);

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

	auto GetDisplayedTiming() const -> const TimingData&;

	/**
	 * @brief Holds a vector sorted by real beat, the beat that would be
	 * displayed in the NoteField (because they are affected by scroll
	 * segments), and also the velocity. This vector will be populated on
	 * Player::Load() be used a lot in ArrowEffects to determine the
	 * target beat in O(log N).
	 */
	std::vector<CacheDisplayedBeat> m_CacheDisplayedBeat;

	/**
	 * @brief Holds a vector sorted by beat, the cumulative number of notes from
	 *        the start of the song. This will be used by [insert more
	 * description here]
	 */
	std::vector<CacheNoteStat> m_CacheNoteStat;

	/**
	 * @brief Change the PlayerOptions to their default.
	 * @param l the level of mods to reset.
	 */
	void ResetToDefaultPlayerOptions(ModsLevel l);
	/** @brief The PlayerOptions presently in use by the Player. */
	ModsGroup<PlayerOptions> m_PlayerOptions{};

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
	int m_NumCols;
	void SetNumCols(int ncol) { m_NumCols = ncol; };
	auto GetNumCols() -> int { return m_NumCols; };

	float playertargetgoal = 0.93F;

	// Lua
	void PushSelf(lua_State* L);
};

#endif
