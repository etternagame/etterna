/** @brief SongUtil - Utility functions that deal with Song. */

#ifndef SONG_UTIL_H
#define SONG_UTIL_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "RageUtil/Utils/RageUtil_CachedObject.h"
#include <utility>

using std::vector;

class Song;
class Steps;
class Profile;
class XNode;

void
AppendOctal(int n, int digits, std::string& out);

/** @brief A set of song utilities to make working with songs easier. */
namespace SongUtil {
void
GetSteps(const Song* pSong,
		 vector<Steps*>& arrayAddTo,
		 StepsType st = StepsType_Invalid,
		 Difficulty dc = Difficulty_Invalid,
		 int iMeterLow = -1,
		 int iMeterHigh = -1,
		 bool filteringSteps = false,
		 const std::string& sDescription = "",
		 const std::string& sCredit = "",
		 bool bIncludeAutoGen = true,
		 unsigned uHash = 0,
		 int iMaxToGet = -1);
auto
GetOneSteps(const Song* pSong,
			StepsType st = StepsType_Invalid,
			Difficulty dc = Difficulty_Invalid,
			int iMeterLow = -1,
			int iMeterHigh = -1,
			bool filteringSteps = false,
			const std::string& sDescription = "",
			const std::string& sCredit = "",
			unsigned uHash = 0,
			bool bIncludeAutoGen = true) -> Steps*;
auto
GetStepsByDifficulty(const Song* pSong,
					 StepsType st,
					 Difficulty dc,
					 bool bIncludeAutoGen = true) -> Steps*;
auto
GetStepsByMeter(const Song* pSong, StepsType st, int iMeterLow, int iMeterHigh)
  -> Steps*;
auto
GetStepsByDescription(const Song* pSong,
					  StepsType st,
					  const std::string& sDescription) -> Steps*;
auto
GetStepsByCredit(const Song* pSong, StepsType st, const std::string& sCredit)
  -> Steps*;
auto
GetClosestNotes(const Song* pSong,
				StepsType st,
				Difficulty dc,
				bool bIgnoreLocked = false) -> Steps*;

void
AdjustDuplicateSteps(Song* pSong); // part of TidyUpData
void
DeleteDuplicateSteps(Song* pSong, vector<Steps*>& vSteps);

void
MakeSortString(std::string& s);
auto
MakeSortString(const std::string& in) -> std::string;
void
SortSongPointerArrayByTitle(vector<Song*>& vpSongsInOut);
void
SortSongPointerArrayByBPM(vector<Song*>& vpSongsInOut);
void
SortSongPointerArrayByWifeScore(vector<Song*>& v);
void
SortSongPointerArrayByArtist(vector<Song*>& vpSongsInOut);
void
SortSongPointerArrayByDisplayArtist(vector<Song*>& vpSongsInOut);
void
SortSongPointerArrayByGenre(vector<Song*>& vpSongsInOut);
void
SortSongPointerArrayByGroupAndTitle(vector<Song*>& vpSongsInOut);
void
SortSongPointerArrayByGroupAndMSD(vector<Song*>& vpSongsInOut, Skillset ss);
void
SortSongPointerArrayByNumPlays(vector<Song*>& vpSongsInOut,
							   ProfileSlot slot,
							   bool bDescending);
void
SortSongPointerArrayByNumPlays(vector<Song*>& vpSongsInOut,
							   const Profile* pProfile,
							   bool bDescending);
void
SortSongPointerArrayByStepsTypeAndMeter(vector<Song*>& vpSongsInOut,
										StepsType st,
										Difficulty dc);
auto
GetSectionNameFromSongAndSort(const Song* pSong, SortOrder so) -> std::string;
void
SortSongPointerArrayBySectionName(vector<Song*>& vpSongsInOut, SortOrder so);
void
SortSongPointerArrayByLength(vector<Song*>& vpSongsInOut);

auto
CompareSongPointersByGroup(const Song* pSong1, const Song* pSong2) -> int;

/**
 * @brief Determine if the requested description for an edit is unique.
 * @param pSong the song the edit is for.
 * @param st the steps type for the edit.
 * @param sPreferredDescription the requested description.
 * @param pExclude the steps that want the description.
 * @return true if it is unique, false otherwise.
 */
auto
IsEditDescriptionUnique(const Song* pSong,
						StepsType st,
						const std::string& sPreferredDescription,
						const Steps* pExclude) -> bool;
auto
IsChartNameUnique(const Song* pSong,
				  StepsType st,
				  const std::string& name,
				  const Steps* pExclude) -> bool;
auto
MakeUniqueEditDescription(const Song* pSong,
						  StepsType st,
						  const std::string& sPreferredDescription)
  -> std::string;
auto
ValidateCurrentEditStepsDescription(const std::string& sAnswer,
									std::string& sErrorOut) -> bool;
auto
ValidateCurrentStepsDescription(const std::string& sAnswer,
								std::string& sErrorOut) -> bool;
auto
ValidateCurrentStepsCredit(const std::string& sAnswer, std::string& sErrorOut)
  -> bool;
auto
ValidateCurrentStepsChartName(const std::string& answer, std::string& error)
  -> bool;
auto
ValidateCurrentSongPreview(const std::string& answer, std::string& error)
  -> bool;
auto
ValidateCurrentStepsMusic(const std::string& answer, std::string& error)
  -> bool;

void
GetAllSongGenres(vector<std::string>& vsOut);
void
GetPlayableStepsTypes(const Song* pSong, std::set<StepsType>& vOut);
void
GetPlayableSteps(const Song* pSong, vector<Steps*>& vOut, bool filteringSteps = false);
auto
IsStepsTypePlayable(Song* pSong, StepsType st) -> bool;
auto
IsStepsPlayable(Song* pSong, Steps* pSteps) -> bool;

/**
 * @brief Determine if the song has any playable steps in the present game.
 * @param s the current song.
 * @return true if the song has playable steps, false otherwise. */
auto
IsSongPlayable(Song* s) -> bool;

auto
GetStepsTypeAndDifficultyFromSortOrder(SortOrder so,
									   StepsType& st,
									   Difficulty& dc) -> bool;
} // namespace SongUtil

class SongID
{
	std::string sDir;
	mutable CachedObjectPointer<Song> m_Cache;

  public:
	/**
	 * @brief Set up the SongID with default values.
	 *
	 * This used to call Unset() to do the same thing. */
	SongID()
	  : sDir("")
	{
		m_Cache.Unset();
	}
	void Unset() { FromSong(nullptr); }
	void FromSong(const Song* p);
	auto ToSong() const -> Song*;
	auto operator<(const SongID& other) const -> bool
	{
		return sDir < other.sDir;
	}
	auto operator==(const SongID& other) const -> bool
	{
		return sDir == other.sDir;
	}

	auto CreateNode() const -> XNode*;
	void LoadFromNode(const XNode* pNode);
	void LoadFromString(const char* dir);
	void FromString(std::string _sDir) { sDir = std::move(_sDir); }
	auto ToString() const -> std::string;
	auto IsValid() const -> bool;
};

#endif
