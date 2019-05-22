#ifndef STEPS_UTIL_H
#define STEPS_UTIL_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "RageUtil/Utils/RageUtil_CachedObject.h"

class Steps;
class Song;
class Profile;
class XNode;



/** @brief A Song and one of its Steps. */
class SongAndSteps
{
  public:
	/** @brief the Song we're using. */
	Song* pSong{ NULL };
	/** @brief the Steps we're using. */
	Steps* pSteps{ NULL };
	/** @brief Set up a blank Song and
	 * <a class="el" href="class_steps.html">Step</a>. */
	SongAndSteps() = default;
	/**
	 * @brief Set up the specified Song and
	 * <a class="el" href="class_steps.html">Step</a>.
	 * @param pSong_ the new Song.
	 * @param pSteps_ the new <a class="el" href="class_steps.html">Step</a>. */
	SongAndSteps(Song* pSong_, Steps* pSteps_)
	  : pSong(pSong_)
	  , pSteps(pSteps_)
	{
	}
	/**
	 * @brief Compare two sets of Songs and Steps to see if they are equal.
	 * @param other the other set of SongAndSteps.
	 * @return true if the two sets of Songs and Steps are equal, false
	 * otherwise. */
	bool operator==(const SongAndSteps& other) const
	{
		return pSong == other.pSong && pSteps == other.pSteps;
	}
	/**
	 * @brief Compare two sets of Songs and Steps to see if they are not equal.
	 * @param other the other set of SongAndSteps.
	 * @return true if the two sets of Songs and Steps are not equal, false
	 * otherwise. */
	bool operator<(const SongAndSteps& other) const
	{
		if (pSong != other.pSong)
			return pSong < other.pSong;
		return pSteps < other.pSteps;
	}
};

/** @brief Utility functions for working with Steps. */
namespace StepsUtil {
bool
CompareNotesPointersByRadarValues(const Steps* pSteps1, const Steps* pSteps2);
bool
CompareNotesPointersByMeter(const Steps* pSteps1, const Steps* pSteps2);
bool
CompareNotesPointersByDifficulty(const Steps* pSteps1, const Steps* pSteps2);
void
SortNotesArrayByDifficulty(std::vector<Steps*>& vpStepsInOut);
bool
CompareStepsPointersByTypeAndDifficulty(const Steps* pStep1,
										const Steps* pStep2);
void
SortStepsByTypeAndDifficulty(std::vector<Steps*>& vpStepsInOut);
void
SortStepsPointerArrayByNumPlays(std::vector<Steps*>& vpStepsInOut,
								ProfileSlot slot,
								bool bDescending);
void
SortStepsPointerArrayByNumPlays(std::vector<Steps*>& vpStepsInOut,
								const Profile* pProfile,
								bool bDescending);
bool
CompareStepsPointersByDescription(const Steps* pStep1, const Steps* pStep2);
void
SortStepsByDescription(std::vector<Steps*>& vpStepsInOut);
};

class StepsID
{
	StepsType st{ StepsType_Invalid };
	Difficulty dc{ Difficulty_Invalid };
	RString ck;
	RString sDescription;
	unsigned uHash{ 0 };
	mutable CachedObjectPointer<Steps> m_Cache;

  public:
	/**
	 * @brief Set up the StepsID with default values.
	 *
	 * This used to call Unset(), which set the variables to
	 * the same thing. */
	StepsID()
	  : sDescription("")
	  , uHash(0)
	  , m_Cache()
	{
	}
	void Unset() { FromSteps(nullptr); }
	void FromSteps(const Steps* p);
	Steps* ToSteps(const Song* p, bool bAllowNull) const;
	// FIXME: (interferes with unlimited charts per song)
	// When performing comparisons, the hash value 0 is considered equal to
	// all other values.  This is because the hash value for a Steps is
	// discarded immediately after loading and figuring out a way to preserve
	// and cache it turned into a mess that still didn't work.
	// When scores are looked up by the theme, the theme passes in a Steps
	// which is transformed into a StepsID, which is then used to index into a
	// map.  So when the theme goes to look up the scores for a Steps, the
	// Steps has a hash value of 0, unless it has been played.  But when the
	// scores are saved, the Steps has a correct hash value.  So before a
	// Steps is played, the scores could not be correctly accessed.  This was
	// only visible on Edit charts because scores on all other difficulties
	// are saved without a hash value.
	// Making operator< and operator== treat 0 as equal to all other hash
	// values allows the theme to fetch the scores even though the Steps has
	// a cleared hash value, but is not a good long term solution because the
	// description field isn't always going to be unique.
	// -Kyz
	bool operator<(const StepsID& rhs) const;
	bool operator==(const StepsID& rhs) const;
	bool MatchesStepsType(StepsType s) const { return st == s; }

	XNode* CreateNode() const;
	void LoadFromNode(const XNode* pNode);
	RString ToString() const;
	bool IsValid() const;

	StepsType GetStepsType() const { return st; }
	Difficulty GetDifficulty() const { return dc; }
	RString GetKey() const { return ck; }
	RString GetDescription() const
	{
		return (dc == Difficulty_Edit ? sDescription : RString());
	}
	unsigned GetHash() const { return uHash; }
};

#endif

/**
 * @file
 * @author Chris Danford, Glenn Maynard (c) 2001-2004
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
