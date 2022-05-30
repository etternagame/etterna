/** @brief NoteTypes - Types for holding tap notes and scores. */

#ifndef NOTE_TYPES_H
#define NOTE_TYPES_H

#include "GameConstantsAndTypes.h"
#include "Core/Services/Locator.hpp"

class XNode;

/** @brief The result of hitting (or missing) a tap note. */
struct TapNoteResult
{
	/** @brief Set up the TapNoteResult with default values. The default offset
	 * value should be 1 not 0 as to diffrentiate from 0 offset hits, idiots
	 * -Mina*/
	TapNoteResult() = default;
	/** @brief The TapNoteScore that was achieved by the player. */
	TapNoteScore tns{ TNS_None };

	/**
	 * @brief Offset, in seconds, for a tap grade.
	 *
	 * Negative numbers mean the note was hit early; positive numbers mean
	 * it was hit late. These values are only meaningful for graded taps
	 * (tns >= TNS_W5). */
	float fTapNoteOffset{ 1.F };

	/** @brief If the whole row has been judged, all taps on the row will be set
	 * to hidden. */
	bool bHidden{ false };

	// XML
	[[nodiscard]] auto CreateNode() const -> XNode*;
	void LoadFromNode(const XNode* pNode);

	// Lua
	void PushSelf(lua_State* L);
};
/** @brief The result of holding (or letting go of) a hold note. */
struct HoldNoteResult
{
	HoldNoteResult() = default;
	[[nodiscard]] auto GetLastHeldBeat() const -> float;

	HoldNoteScore hns{ HNS_None };

	/**
	 * @brief the current life of the hold.
	 *
	 * 1.0 means this HoldNote has full life.
	 *
	 * 0.0 means this HoldNote is dead.
	 *
	 * When this value hits 0.0 for the first time, m_HoldScore becomes
	 * HNS_LetGo. If the life is > 0.0 when the HoldNote ends, then m_HoldScore
	 * becomes HNS_Held. */
	float fLife{ 1.F };

	/** @brief The number of seconds the hold note has overlapped the current
	 * beat.
	 *
	 * This value is 0 if it doesn't overlap. */
	float fOverlappedTime{ 0 };

	/** @brief Last index where fLife was greater than 0. If the tap was missed,
	 * this will be the first index of the hold. */
	int iLastHeldRow{ 0 };

	/** @brief If checkpoint holds are enabled, the number of checkpoints hit.
	 */
	int iCheckpointsHit{ 0 };
	/** @brief If checkpoint holds are enabled, the number of checkpoints
	 * missed. */
	int iCheckpointsMissed{ 0 };

	/** @brief Was the button held during the last update? */
	bool bHeld{ false };
	/** @brief Is there life in the hold and does it overlap the current beat?
	 */
	bool bActive{ false };

	// XML
	[[nodiscard]] auto CreateNode() const -> XNode*;
	void LoadFromNode(const XNode* pNode);

	// Lua
	void PushSelf(lua_State* L);
};

/** @brief What is the TapNote's core type? */
enum TapNoteType
{
	TapNoteType_Empty,	  /**< There is no note here. */
	TapNoteType_Tap,	  /**< The player simply steps on this. */
	TapNoteType_HoldHead, /**< This is graded like the Tap type, but should be
							 held. */
	TapNoteType_HoldTail, /**< In 2sand3s mode, holds are deleted and hold_tail
							 is added. */
	TapNoteType_Mine, /**< In most modes, it is suggested to not step on these
						 mines. */
	TapNoteType_Lift, /**< Lift your foot up when it crosses the target area. */
	TapNoteType_AutoKeysound, /**< A special sound is played when this note
								 crosses the target area. */
	TapNoteType_Fake, /**< This arrow can't be scored for or against the player.
					   */
	NUM_TapNoteType,
	TapNoteType_Invalid
};
auto
TapNoteTypeToString(TapNoteType tnt) -> const std::string&;
auto
TapNoteTypeToLocalizedString(TapNoteType tnt) -> const std::string&;
LuaDeclareType(TapNoteType);

/** @brief The list of a TapNote's sub types. */
enum TapNoteSubType
{
	TapNoteSubType_Hold, /**< The start of a traditional hold note. */
	TapNoteSubType_Roll, /**< The start of a roll note that must be hit
							repeatedly. */
	// TapNoteSubType_Mine,
	NUM_TapNoteSubType,
	TapNoteSubType_Invalid
};
auto
TapNoteSubTypeToString(TapNoteSubType tnst) -> const std::string&;
auto
TapNoteSubTypeToLocalizedString(TapNoteSubType tnst) -> const std::string&;
LuaDeclareType(TapNoteSubType);

/** @brief The different places a TapNote could come from. */
enum TapNoteSource
{
	TapNoteSource_Original, /**< This note is part of the original NoteData. */
	TapNoteSource_Addition, /**< This note is additional note added by a
							   transform. */
	NUM_TapNoteSource,
	TapNoteSource_Invalid
};
auto
TapNoteSourceToString(TapNoteSource tns) -> const std::string&;
auto
TapNoteSourceToLocalizedString(TapNoteSource tns) -> const std::string&;
LuaDeclareType(TapNoteSource);

/** @brief The various properties of a tap note. */
struct TapNote
{
	/** @brief The core note type that is about to cross the target area. */
	TapNoteType type{ TapNoteType_Empty };
	/** @brief The sub type of the note. This is only used if the type is
	 * hold_head. */
	TapNoteSubType subType{ TapNoteSubType_Invalid };
	/** @brief The originating source of the TapNote. */
	TapNoteSource source{ TapNoteSource_Original };
	/** @brief The result of hitting or missing the TapNote. */
	TapNoteResult result;

	// Index into Song's vector of keysound files if nonnegative:
	int iKeysoundIndex{ -1 };

	// also used for hold_head only:
	int iDuration{ 0 };
	HoldNoteResult HoldResult;

	// XML
	[[nodiscard]] auto CreateNode() const -> XNode*;
	void LoadFromNode(const XNode* pNode);

	// Lua
	void PushSelf(lua_State* L);

	// So I'm not repeatedly typing this out - Mina
	[[nodiscard]] auto IsNote() const -> bool
	{
		return type == TapNoteType_Tap || type == TapNoteType_HoldHead;
	}

	TapNote() = default;
	void Init()
	{
		type = TapNoteType_Empty;
		subType = TapNoteSubType_Invalid;
		source = TapNoteSource_Original;
		iDuration = 0;
	}
	TapNote(TapNoteType type_,
			TapNoteSubType subType_,
			TapNoteSource source_,
			int iKeysoundIndex_)
	  : type(type_)
	  , subType(subType_)
	  , source(source_)
	  , iKeysoundIndex(iKeysoundIndex_)
	{
		if (type_ > TapNoteType_Fake) {
			Locator::getLogger()->trace("Invalid tap note type {} (most likely) due to random "
					   "vanish issues. Assume it doesn't need judging.",
					   TapNoteTypeToString(type_).c_str());
			type = TapNoteType_Empty;
		}
	}

	/**
	 * @brief Determine if the two TapNotes are equal to each other.
	 * @param other the other TapNote we're checking.
	 * @return true if the two TapNotes are equal, or false otherwise. */
	auto operator==(const TapNote& other) const -> bool
	{
#define COMPARE(x)                                                             \
	if ((x) != other.x)                                                        \
	return false
		COMPARE(type);
		COMPARE(subType);
		COMPARE(source);
		COMPARE(iKeysoundIndex);
		COMPARE(iDuration);
#undef COMPARE
		return true;
	}
	/**
	 * @brief Determine if the two TapNotes are not equal to each other.
	 * @param other the other TapNote we're checking.
	 * @return true if the two TapNotes are not equal, or false otherwise. */
	auto operator!=(const TapNote& other) const -> bool
	{
		return !operator==(other);
	}
};

extern TapNote TAP_EMPTY;				   // '0'
extern TapNote TAP_ORIGINAL_TAP;		   // '1'
extern TapNote TAP_ORIGINAL_HOLD_HEAD;	   // '2'
extern TapNote TAP_ORIGINAL_ROLL_HEAD;	   // '4'
extern TapNote TAP_ORIGINAL_MINE;		   // 'M'
extern TapNote TAP_ORIGINAL_LIFT;		   // 'L'
extern TapNote TAP_ORIGINAL_ATTACK;		   // 'A'
extern TapNote TAP_ORIGINAL_AUTO_KEYSOUND; // 'K'
extern TapNote TAP_ORIGINAL_FAKE;		   // 'F'
// extern TapNote TAP_ORIGINAL_MINE_HEAD;	// 'N' (tentative, we'll see when
// iDance gets ripped.)
extern TapNote TAP_ADDITION_TAP;
extern TapNote TAP_ADDITION_MINE;

/**
 * @brief The number of tracks allowed.
 *
 * TODO: Don't have a hard-coded track limit.
 */
const int MAX_NOTE_TRACKS = 16;

/**
 * @brief The number of rows per beat.
 *
 * This is a divisor for our "fixed-point" time/beat representation. It must be
 * evenly divisible by 2, 3, and 4, to exactly represent 8th, 12th and 16th
 * notes.
 *
 * XXX: Some other forks try to keep this flexible by putting this in the
 * simfile. Is this a recommended course of action? -Wolfman2000 */
const int ROWS_PER_BEAT = 48;

/** @brief The max number of rows allowed for a Steps pattern. */
const int MAX_NOTE_ROW = (1 << 30);

/** @brief The list of quantized note types allowed at present. */
enum NoteType
{
	NOTE_TYPE_4TH,	 /**< quarter note */
	NOTE_TYPE_8TH,	 /**< eighth note */
	NOTE_TYPE_12TH,	 /**< quarter note triplet */
	NOTE_TYPE_16TH,	 /**< sixteenth note */
	NOTE_TYPE_24TH,	 /**< eighth note triplet */
	NOTE_TYPE_32ND,	 /**< thirty-second note */
	NOTE_TYPE_48TH,	 /**< sixteenth note triplet */
	NOTE_TYPE_64TH,	 /**< sixty-fourth note */
	NOTE_TYPE_192ND, /**< sixty-fourth note triplet */
	NUM_NoteType,
	NoteType_Invalid
};
auto
NoteTypeToString(NoteType nt) -> const std::string&;
auto
NoteTypeToLocalizedString(NoteType nt) -> const std::string&;
LuaDeclareType(NoteType);
auto
NoteTypeToBeat(NoteType nt) -> float;
auto
NoteTypeToRow(NoteType nt) -> int;
auto
GetNoteType(int row) -> NoteType;
auto
BeatToNoteType(float fBeat) -> NoteType;
auto
IsNoteOfType(int row, NoteType t) -> bool;

/* This is more accurate: by computing the integer and fractional parts
separately, we
 * can avoid storing very large numbers in a float and possibly losing
precision.  It's
 * slower; use this once less stuff uses BeatToNoteRow.
 *
 *	This function rounds up.
inline int   BeatToNoteRow( float fBeatNum )
{
	float fraction = fBeatNum - truncf(fBeatNum);
	int integer = int(fBeatNum) * ROWS_PER_BEAT;
	return integer + lrintf(fraction * ROWS_PER_BEAT);
}
*/

/**
 * @brief Convert the beat into a note row.
 * @param fBeatNum the beat to convert.
 * @return the note row. */
inline auto
BeatToNoteRow(float fBeatNum) -> int
{
	return lround(fBeatNum * 48.F);
}
/**
 * @brief Convert the note row to a beat.
 * @param iRow the row to convert.
 * @return the beat. */
inline auto
NoteRowToBeat(int iRow) -> float
{
	return iRow / static_cast<float>(ROWS_PER_BEAT);
}

// These functions can be useful for function templates,
// where both rows and beats can be specified.

/**
 * @brief Convert the note row to note row (returns itself).
 * @param row the row to convert.
 */
static inline auto
ToNoteRow(int row) -> int
{
	return row;
}

/**
 * @brief Convert the beat to note row.
 * @param beat the beat to convert.
 */
static inline auto
ToNoteRow(float beat) -> int
{
	return BeatToNoteRow(beat);
}

/**
 * @brief Convert the note row to beat.
 * @param row the row to convert.
 */
static inline auto
ToBeat(int row) -> float
{
	return NoteRowToBeat(row);
}

/**
 * @brief Convert the beat row to beat (return itself).
 * @param beat the beat to convert.
 */
static inline auto
ToBeat(float beat) -> float
{
	return beat;
}

/**
 * @brief Scales the position.
 * @param T start - the starting row of the scaling region
 * @param T length - the length of the scaling region
 * @param T newLength - the new length of the scaling region
 * @param T position - the position to scale
 * @return T the scaled position
 */
template<typename T>
auto
ScalePosition(T start, T length, T newLength, T position) -> T
{
	if (position < start)
		return position;
	if (position >= start + length)
		return position - length + newLength;
	return start + (position - start) * newLength / length;
}

#endif
