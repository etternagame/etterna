#ifndef Profile_H
#define Profile_H

#include "Etterna/Models/Misc/DateTime.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/Grade.h"
#include "Etterna/Models/Misc/HighScore.h"
#include "Etterna/Models/Lua/LuaReference.h"
#include "Etterna/Models/Songs/SongUtil.h"  // for SongID
#include "Etterna/Models/StepsAndStyles/StepsUtil.h" // for StepsID
#include "Etterna/Models/StepsAndStyles/StyleUtil.h" // for StyleID
#include "Etterna/Models/Lua/LuaReference.h"
#include "Etterna/Models/Misc/XMLProfile.h"
#include "Etterna/Models/Misc/DBProfile.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include <map>
#include <set>

#include <unordered_map>

class XNode;
struct lua_State;
class Character;
struct Playlist;

// Current file versions
extern const RString ETT_XML;

/**
 * @brief The filename where one can edit their personal profile data.
 *
 * Editable data is an INI because the default INI file association on Windows
 * systems will open the ini file in an editor.  The default association for
 * XML will open in IE.  Users have a much better chance of discovering how to
 * edit this data if they don't have to fight against the file associations. */
extern const RString EDITABLE_INI;

/**
 * @brief The filename containing the signature for STATS_XML's signature.
 *
 *
 * The "don't share" file is something that the user should always keep private.
 * They can safely share STATS_XML with STATS_XML's signature so that others
 * can authenticate the STATS_XML data.  However, others can't copy that data
 * to their own profile for use in the game unless they also have the "don't
 * share" file.  DontShare contains a piece of information that we can
 * construct using STATS_XML but the user can't construct using STATS_XML. */
extern const RString DONT_SHARE_SIG;

extern const RString PUBLIC_KEY_FILE;
extern const RString SCREENSHOTS_SUBDIR;
extern const RString REPLAY_SUBDIR;
extern const RString EDIT_STEPS_SUBDIR;
extern const RString LASTGOOD_SUBDIR;
// extern const RString RIVAL_SUBDIR;

/** @brief The max number of characters that can be used in a profile. */
const unsigned int PROFILE_MAX_DISPLAY_NAME_LENGTH = 64;

class Style;

class Song;
class Steps;
struct Game;

// Profile types exist for sorting the list of profiles.
// Guest profiles at the top, test at the bottom.
enum ProfileType
{
	ProfileType_Guest,
	ProfileType_Normal,
	ProfileType_Test,
	NUM_ProfileType,
	ProfileType_Invalid
};

// future goalman stuff - Mina
class ScoreGoal
{
  public:
	float rate = 1.f;
	float percent = .93f;
	int priority = 1;
	bool achieved = false;
	DateTime timeassigned;
	DateTime timeachieved;
	RString comment = "";
	RString chartkey = "";

	// which specific score was this goal achieved by, reminder to consider
	// what happens when individual score deletion is possibly added -mina
	RString scorekey = "";

	XNode* CreateNode() const;
	void LoadFromNode(const XNode* pNode);

	HighScore* GetPBUpTo();

	// If the scoregoal has already been completed prior to being assigned, flag
	// it as a vacuous goal
	void CheckVacuity();

	void UploadIfNotVacuous();

	// Vacuous goals will remain in memory for the session but not be written
	// during save -mina
	bool vacuous = false;

	void PushSelf(lua_State* L);
};

struct GoalsForChart
{
  public:
	void Add(ScoreGoal& sg) { goals.emplace_back(sg); }
	std::vector<ScoreGoal>& Get() { return goals; }
	std::vector<ScoreGoal> goals;

	XNode* CreateNode() const;
	void LoadFromNode(const XNode* pNode);
};

/**
 * @brief Player data that persists between sessions.
 *
 * This can be stored on a local disk or on a memory card. */
class Profile
{
  public:
	/**
	 * @brief Set up the Profile with default values.
	 *
	 * Note: there are probably a lot of variables. */
	// When adding new score related data, add logic for handling it to
	// MergeScoresFromOtherProfile. -Kyz
	// When adding any new fields, add them to SwapExceptPriority.  Anything not
	// added to SwapExceptPriority won't be swapped correctly when the user
	// changes the list priority of a profile. -Kyz
	Profile()
	  :

	  m_sDisplayName("")
	  , m_sCharacterID("")
	  , m_sLastUsedHighScoreName("")
	  , m_sGuid(MakeGuid())
	  , m_sDefaultModifiers()
	  , m_lastSong()
	  , m_sLastPlayedMachineGuid("")
	  , m_LastPlayedDate()
	  , m_iNumSongsPlayedByStyle()
	  , m_UserTable()
	  , m_SongHighScores()
	  , m_vScreenshots()
	  , profiledir("")
	{
		m_lastSong.Unset();

		m_LastPlayedDate.Init();

		FOREACH_ENUM(PlayMode, i)
		m_iNumSongsPlayedByPlayMode[i] = 0;
		FOREACH_ENUM(Difficulty, i)
		m_iNumSongsPlayedByDifficulty[i] = 0;
		for (int& i : m_iNumSongsPlayedByMeter)
			i = 0;

		ZERO(m_iNumStagesPassedByPlayMode);
		ZERO(m_iNumStagesPassedByGrade);
		m_UserTable.Unset();
	}

	// smart accessors
	RString GetDisplayNameOrHighScoreName() const;
	Character* GetCharacter() const;
	void SetCharacter(const RString& sCharacterID);
	int GetTotalNumSongsPassed() const;
	int GetTotalStepsWithTopGrade(StepsType st, Difficulty d, Grade g) const;
	float GetSongsPossible(StepsType st, Difficulty dc) const;
	float GetSongsActual(StepsType st, Difficulty dc) const;
	float GetSongsPercentComplete(StepsType st, Difficulty dc) const;
	float GetSongsAndCoursesPercentCompleteAllDifficulties(StepsType st) const;
	bool GetDefaultModifiers(const Game* pGameType,
							 RString& sModifiersOut) const;
	void SetDefaultModifiers(const Game* pGameType, const RString& sModifiers);
	Song* GetMostPopularSong() const;

	void AddStepTotals(int iNumTapsAndHolds,
					   int iNumJumps,
					   int iNumHolds,
					   int iNumRolls,
					   int iNumMines,
					   int iNumHands,
					   int iNumLifts);

	ProfileType m_Type{ ProfileType_Normal };
	// Profiles of the same type and priority are sorted by dir name.
	int m_ListPriority{ 0 };
	// Profile Playlists
	std::map<std::string, Playlist> allplaylists;

	// Editable data
	RString m_sDisplayName;
	RString m_sCharacterID;
	// Dont edit this. Should be unique (Is it?)
	RString m_sProfileID;
	/**
	 * @brief The last used name for high scoring purposes.
	 *
	 * This really shouldn't be in "editable", but it's needed in the smaller
	 * editable file so that it can be ready quickly. */
	RString m_sLastUsedHighScoreName;

	// General data
	static RString MakeGuid();
	RString* GetGuid() { return &m_sGuid; }
	RString m_sGuid;
	std::map<RString, RString> m_sDefaultModifiers;
	SortOrder m_SortOrder{ SortOrder_Invalid };
	Difficulty m_LastDifficulty{ Difficulty_Invalid };
	StepsType m_LastStepsType{ StepsType_Invalid };
	SongID m_lastSong;
	int m_iCurrentCombo{ 0 };
	int m_iTotalSessions{ 0 };
	int m_iTotalSessionSeconds{ 0 };
	int m_iTotalGameplaySeconds{ 0 };
	int m_iTotalDancePoints{ 0 };
	int m_iNumExtraStagesPassed{ 0 };
	int m_iNumExtraStagesFailed{ 0 };
	int m_iNumToasties{ 0 };
	int m_iTotalTapsAndHolds{ 0 };
	int m_iTotalJumps{ 0 };
	int m_iTotalHolds{ 0 };
	int m_iTotalRolls{ 0 };
	int m_iTotalMines{ 0 };
	int m_iTotalHands{ 0 };
	int m_iTotalLifts{ 0 };
	float m_fPlayerRating;
	float m_fPlayerSkillsets[NUM_Skillset];
	/** @brief Is this a brand new profile? */
	bool m_bNewProfile{ false };

	// seriously why is this not a thing -mina
	string profiledir;
	bool IsEtternaProfile{ false };
	/**
	 * @brief Which machine did we play on last, based on the Guid?
	 *
	 * This is mutable because it's overwritten on save, but is usually
	 * const everywhere else. It was decided to keep const on the whole
	 * save chain and keep this mutable. -Chris */
	mutable RString m_sLastPlayedMachineGuid;
	mutable DateTime m_LastPlayedDate;
	/* These stats count twice in the machine profile if two players are
	 * playing; that's the only approach that makes sense for ByDifficulty and
	 * ByMeter. */
	int m_iNumSongsPlayedByPlayMode[NUM_PlayMode];
	std::map<StyleID, int> m_iNumSongsPlayedByStyle;
	int m_iNumSongsPlayedByDifficulty[NUM_Difficulty];
	int m_iNumSongsPlayedByMeter[MAX_METER + 1];
	/**
	 * @brief Count the total number of songs played.
	 *
	 * This stat counts once per song, even if two players are active. */
	int m_iNumTotalSongsPlayed{ 0 };
	int m_iNumStagesPassedByPlayMode[NUM_PlayMode];
	int m_iNumStagesPassedByGrade[NUM_Grade];

	// if anymore of these are added they should be enum'd to reduce copy pasta
	// -mina and also should be sets
	void AddToFavorites(const std::string& ck) { FavoritedCharts.emplace(ck); }
	void AddToPermaMirror(const std::string& ck) { PermaMirrorCharts.emplace(ck); }
	void RemoveFromFavorites(const std::string& ck);
	void RemoveFromPermaMirror(const std::string& ck);
	std::set<string> FavoritedCharts;
	std::set<string> PermaMirrorCharts;

	// more future goalman stuff -mina
	void AddGoal(const std::string& ck);
	void RemoveGoal(const std::string& ck, DateTime assigned);
	std::unordered_map<string, GoalsForChart> goalmap;
	void FillGoalTable();
	std::vector<ScoreGoal*> goaltable;
	int sortmode = 1;   // 1=date 2=rate 3=name 4=priority 5=diff, init to name
						// because that's the default- mina
	int filtermode = 1; // 1=all, 2=completed, 3=uncompleted
	bool asc = false;

	bool HasGoal(const std::string& ck) { return goalmap.count(ck) == 1; }
	ScoreGoal& GetLowestGoalForRate(const std::string& ck, float rate);
	void SetAnyAchievedGoals(const std::string& ck,
							 float& rate,
							 const HighScore& pscore);

	/* store arbitrary data for the theme within a profile */
	LuaTable m_UserTable;

	// Song high scores
	struct HighScoresForASteps
	{
		HighScoreList hsl;
		HighScoresForASteps()
		  : hsl()
		{
		}
	};
	struct HighScoresForASong
	{
		std::map<StepsID, HighScoresForASteps> m_StepsHighScores;
		int GetNumTimesPlayed() const;
		HighScoresForASong()
		  : m_StepsHighScores()
		{
		}
	};
	std::map<SongID, HighScoresForASong> m_SongHighScores;

	void AddStepsHighScore(const Song* pSong,
						   const Steps* pSteps,
						   HighScore hs,
						   int& iIndexOut);
	const HighScoreList& GetStepsHighScoreList(const Song* pSong,
											   const Steps* pSteps) const;
	HighScoreList& GetStepsHighScoreList(const Song* pSong,
										 const Steps* pSteps);
	int GetStepsNumTimesPlayed(const Song* pSong, const Steps* pSteps) const;
	void IncrementStepsPlayCount(const Song* pSong, const Steps* pSteps);
	Grade GetBestGrade(const Song* pSong, StepsType st) const;
	void GetGrades(const Song* pSong,
				   StepsType st,
				   int iCounts[NUM_Grade]) const;
	int GetSongNumTimesPlayed(const Song* pSong) const;
	int GetSongNumTimesPlayed(const SongID& songID) const;
	DateTime GetSongLastPlayedDateTime(const Song* pSong) const;
	bool HasPassedSteps(const Song* pSong, const Steps* pSteps) const;
	bool HasPassedAnyStepsInSong(const Song* pSong) const;

	void GetAllUsedHighScoreNames(std::set<RString>& names);

	void MergeScoresFromOtherProfile(Profile* other,
									 bool skip_totals,
									 RString const& from_dir,
									 RString const& to_dir);

	// Screenshot Data
	std::vector<Screenshot> m_vScreenshots;
	void AddScreenshot(const Screenshot& screenshot);
	int GetNextScreenshotIndex() { return m_vScreenshots.size(); }

	// Init'ing
	void InitAll()
	{
		InitEditableData();
		InitGeneralData();
		InitSongScores();
		InitScreenshotData();
	}
	void InitEditableData();
	void InitGeneralData();
	void InitSongScores();
	void InitScreenshotData();
	void ClearStats();

	void swap(Profile& other);

	// Loading and saving
	void HandleStatsPrefixChange(RString dir, bool require_signature);
	ProfileLoadResult LoadAllFromDir(const RString& sDir,
									 bool bRequireSignature,
									 LoadingWindow* ld);
	ProfileLoadResult LoadStatsFromDir(RString dir, bool require_signature);
	void LoadTypeFromDir(const RString& dir);
	void LoadCustomFunction(const RString& sDir);
	bool SaveAllToDir(const RString& sDir, bool bSignData) const;

	ProfileLoadResult LoadEditableDataFromDir(const RString& sDir);

	void SaveTypeToDir(const RString& dir) const;
	void SaveEditableDataToDir(const RString& sDir) const;

	void CalculateStatsFromScores(LoadingWindow* ld);
	void CalculateStatsFromScores();

	void SaveStatsWebPageToDir(const RString& sDir) const;
	void SaveMachinePublicKeyToDir(const RString& sDir) const;

	static void MoveBackupToDir(const RString& sFromDir, const RString& sToDir);
	static RString MakeUniqueFileNameNoExtension(
	  const RString& sDir,
	  const RString& sFileNameBeginning);
	static RString MakeFileNameNoExtension(const RString& sFileNameBeginning,
										   int iIndex);

	// Lua
	void PushSelf(lua_State* L);

  private:
	const HighScoresForASong* GetHighScoresForASong(const SongID& songID) const;
	XMLProfile XMLProf;
	DBProfile DBProf;
};

#endif

/**
 * @file
 * @author Chris Danford (c) 2001-2004
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
