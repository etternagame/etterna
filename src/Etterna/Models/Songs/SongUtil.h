/** @brief SongUtil - Utility functions that deal with Song. */

#ifndef SONG_UTIL_H
#define SONG_UTIL_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "RageUtil/Utils/RageUtil_CachedObject.h"
#include <set>

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
		 const std::string& sDescription = "",
		 const std::string& sCredit = "",
		 bool bIncludeAutoGen = true,
		 unsigned uHash = 0,
		 int iMaxToGet = -1);
Steps*
GetOneSteps(const Song* pSong,
			StepsType st = StepsType_Invalid,
			Difficulty dc = Difficulty_Invalid,
			int iMeterLow = -1,
			int iMeterHigh = -1,
			const std::string& sDescription = "",
			const std::string& sCredit = "",
			unsigned uHash = 0,
			bool bIncludeAutoGen = true);
Steps*
GetStepsByDifficulty(const Song* pSong,
					 StepsType st,
					 Difficulty dc,
					 bool bIncludeAutoGen = true);
Steps*
GetStepsByMeter(const Song* pSong, StepsType st, int iMeterLow, int iMeterHigh);
Steps*
GetStepsByDescription(const Song* pSong,
					  StepsType st,
					  const std::string& sDescription);
Steps*
GetStepsByCredit(const Song* pSong, StepsType st, const std::string& sCredit);
Steps*
GetClosestNotes(const Song* pSong,
				StepsType st,
				Difficulty dc,
				bool bIgnoreLocked = false);

void
AdjustDuplicateSteps(Song* pSong); // part of TidyUpData
void
DeleteDuplicateSteps(Song* pSong, vector<Steps*>& vSteps);

void
MakeSortString(std::string& s);
std::string
MakeSortString(const string& in);
void
SortSongPointerArrayByTitle(vector<Song*>& vpSongsInOut);
void
SortSongPointerArrayByBPM(vector<Song*>& vpSongsInOut);
void
SortSongPointerArrayByGrades(vector<Song*>& vpSongsInOut, bool bDescending);
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
std::string
GetSectionNameFromSongAndSort(const Song* pSong, SortOrder so);
void
SortSongPointerArrayBySectionName(vector<Song*>& vpSongsInOut, SortOrder so);
void
SortSongPointerArrayByLength(vector<Song*>& vpSongsInOut);

int
CompareSongPointersByGroup(const Song* pSong1, const Song* pSong2);

/**
 * @brief Determine if the requested description for an edit is unique.
 * @param pSong the song the edit is for.
 * @param st the steps type for the edit.
 * @param sPreferredDescription the requested description.
 * @param pExclude the steps that want the description.
 * @return true if it is unique, false otherwise.
 */
bool
IsEditDescriptionUnique(const Song* pSong,
						StepsType st,
						const std::string& sPreferredDescription,
						const Steps* pExclude);
bool
IsChartNameUnique(const Song* pSong,
				  StepsType st,
				  const std::string& name,
				  const Steps* pExclude);
std::string
MakeUniqueEditDescription(const Song* pSong,
						  StepsType st,
						  const std::string& sPreferredDescription);
bool
ValidateCurrentEditStepsDescription(const std::string& sAnswer,
									std::string& sErrorOut);
bool
ValidateCurrentStepsDescription(const std::string& sAnswer,
								std::string& sErrorOut);
bool
ValidateCurrentStepsCredit(const std::string& sAnswer, std::string& sErrorOut);
bool
ValidateCurrentStepsChartName(const std::string& answer, std::string& error);
bool
ValidateCurrentSongPreview(const std::string& answer, std::string& error);
bool
ValidateCurrentStepsMusic(const std::string& answer, std::string& error);

void
GetAllSongGenres(vector<std::string>& vsOut);
void
GetPlayableStepsTypes(const Song* pSong, set<StepsType>& vOut);
void
GetPlayableSteps(const Song* pSong, vector<Steps*>& vOut);
bool
IsStepsTypePlayable(Song* pSong, StepsType st);
bool
IsStepsPlayable(Song* pSong, Steps* pSteps);

/**
 * @brief Determine if the song has any playable steps in the present game.
 * @param s the current song.
 * @return true if the song has playable steps, false otherwise. */
bool
IsSongPlayable(Song* s);

bool
GetStepsTypeAndDifficultyFromSortOrder(SortOrder so,
									   StepsType& st,
									   Difficulty& dc);
}

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
	  , m_Cache()
	{
		m_Cache.Unset();
	}
	void Unset() { FromSong(nullptr); }
	void FromSong(const Song* p);
	Song* ToSong() const;
	bool operator<(const SongID& other) const { return sDir < other.sDir; }
	bool operator==(const SongID& other) const { return sDir == other.sDir; }

	XNode* CreateNode() const;
	void LoadFromNode(const XNode* pNode);
	void LoadFromString(const char* dir);
	void FromString(std::string _sDir) { sDir = _sDir; }
	std::string ToString() const;
	bool IsValid() const;
};

#endif
