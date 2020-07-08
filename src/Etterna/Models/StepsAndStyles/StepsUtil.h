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
	Song* pSong{ nullptr };
	/** @brief the Steps we're using. */
	Steps* pSteps{ nullptr };
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
	auto operator==(const SongAndSteps& other) const -> bool
	{
		return pSong == other.pSong && pSteps == other.pSteps;
	}
	/**
	 * @brief Compare two sets of Songs and Steps to see if they are not equal.
	 * @param other the other set of SongAndSteps.
	 * @return true if the two sets of Songs and Steps are not equal, false
	 * otherwise. */
	auto operator<(const SongAndSteps& other) const -> bool
	{
		if (pSong != other.pSong) {
			return pSong < other.pSong;
		}
		return pSteps < other.pSteps;
	}
};

/** @brief Utility functions for working with Steps. */
namespace StepsUtil {
auto
CompareNotesPointersByRadarValues(const Steps* pSteps1, const Steps* pSteps2)
  -> bool;
auto
CompareNotesPointersByMeter(const Steps* pSteps1, const Steps* pSteps2) -> bool;
auto
CompareNotesPointersByDifficulty(const Steps* pSteps1, const Steps* pSteps2)
  -> bool;
void
SortNotesArrayByDifficulty(vector<Steps*>& vpStepsInOut);
auto
CompareStepsPointersByTypeAndDifficulty(const Steps* pStep1,
										const Steps* pStep2) -> bool;
void
SortStepsByTypeAndDifficulty(vector<Steps*>& vpStepsInOut);
void
SortStepsPointerArrayByNumPlays(vector<Steps*>& vpStepsInOut,
								ProfileSlot slot,
								bool bDescending);
void
SortStepsPointerArrayByNumPlays(vector<Steps*>& vpStepsInOut,
								const Profile* pProfile,
								bool bDescending);
auto
CompareStepsPointersByDescription(const Steps* pStep1, const Steps* pStep2)
  -> bool;
void
SortStepsByDescription(vector<Steps*>& vpStepsInOut);
} // namespace StepsUtil

class StepsID
{
	StepsType st{ StepsType_Invalid };
	Difficulty dc{ Difficulty_Invalid };
	std::string ck;
	std::string sDescription;
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
	{
	}
	void Unset() { FromSteps(nullptr); }
	void FromSteps(const Steps* p);
	auto ToSteps(const Song* p, bool bAllowNull) const -> Steps*;
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
	auto operator<(const StepsID& rhs) const -> bool;
	auto operator==(const StepsID& rhs) const -> bool;
	auto MatchesStepsType(StepsType s) const -> bool { return st == s; }

	auto CreateNode() const -> XNode*;
	void LoadFromNode(const XNode* pNode);
	auto ToString() const -> std::string;
	auto IsValid() const -> bool;

	auto GetStepsType() const -> StepsType { return st; }
	auto GetDifficulty() const -> Difficulty { return dc; }
	auto GetKey() const -> std::string { return ck; }
	auto GetDescription() const -> std::string
	{
		return (dc == Difficulty_Edit ? sDescription : std::string());
	}
	auto GetHash() const -> unsigned { return uHash; }
};

#endif
