#ifndef Profile_H
#define Profile_H

#include "Etterna/Models/Misc/DateTime.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/Grade.h"
#include "Etterna/Models/HighScore/HighScore.h"
#include "Etterna/Models/Lua/LuaReference.h"
#include "Etterna/Models/Songs/SongUtil.h"			 // for SongID
#include "Etterna/Models/StepsAndStyles/StepsUtil.h" // for StepsID
#include "Etterna/Models/StepsAndStyles/StyleUtil.h" // for StyleID
#include "Etterna/Models/Misc/XMLProfile.h"
#include "Etterna/Models/Misc/DBProfile.h"
#include "arch/LoadingWindow/LoadingWindow.h"

#include <map>
#include <set>

#include <unordered_map>
using std::string;

class XNode;
struct lua_State;
struct Playlist;

// Current file versions
extern const std::string ETT_XML;

/**
 * @brief The filename where one can edit their personal profile data.
 *
 * Editable data is an INI because the default INI file association on Windows
 * systems will open the ini file in an editor.  The default association for
 * XML will open in IE.  Users have a much better chance of discovering how to
 * edit this data if they don't have to fight against the file associations. */
extern const std::string EDITABLE_INI;

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
extern const std::string DONT_SHARE_SIG;

extern const std::string PUBLIC_KEY_FILE;
extern const std::string SCREENSHOTS_SUBDIR;
extern const std::string REPLAY_SUBDIR;
extern const std::string EDIT_STEPS_SUBDIR;
extern const std::string LASTGOOD_SUBDIR;
// extern const std::string RIVAL_SUBDIR;

/** @brief The max number of characters that can be used in a profile. */
const unsigned int PROFILE_MAX_DISPLAY_NAME_LENGTH = 64;

class Style;

class Song;
class Steps;
struct Game;

// future goalman stuff - Mina
class ScoreGoal
{
  public:
	float rate = 1.F;
	float percent = .93F;
	int priority = 1;
	bool achieved = false;
	DateTime timeassigned;
	DateTime timeachieved;
	std::string comment;
	std::string chartkey;

	// which specific score was this goal achieved by, reminder to consider
	// what happens when individual score deletion is possibly added -mina
	std::string scorekey;

	[[nodiscard]] auto CreateNode() const -> XNode*;
	void LoadFromNode(const XNode* pNode);

	[[nodiscard]] auto GetPBUpTo() const -> HighScore*;

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
	auto Get() -> std::vector<ScoreGoal>& { return goals; }
	std::vector<ScoreGoal> goals;

	[[nodiscard]] auto CreateNode() const -> XNode*;
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
	  : m_sGuid(MakeGuid())
	{
		m_lastSong.Unset();
		m_fPlayerRating = 0.F;
		FOREACH_ENUM(Skillset, ss)
		m_fPlayerSkillsets[ss] = 0.F;

		m_LastPlayedDate.Init();

		FOREACH_ENUM(Difficulty, i)
		m_iNumSongsPlayedByDifficulty[i] = 0;
		for (auto& i : m_iNumSongsPlayedByMeter) {
			i = 0;
		}

		ZERO(m_iNumStagesPassedByGrade);
		m_UserTable.Unset();
	}

	// smart accessors
	auto GetDisplayNameOrHighScoreName() const -> std::string;
	auto GetDefaultModifiers(const Game* pGameType,
							 std::string& sModifiersOut) const -> bool;
	void SetDefaultModifiers(const Game* pGameType,
							 const std::string& sModifiers);

	void AddStepTotals(int iNumTapsAndHolds,
					   int iNumJumps,
					   int iNumHolds,
					   int iNumRolls,
					   int iNumMines,
					   int iNumHands,
					   int iNumLifts);

	// Profiles of the same type and priority are sorted by dir name.
	int m_ListPriority{ 0 };
	// Profile Playlists
	std::map<std::string, Playlist> allplaylists;

	// Editable data
	std::string m_sDisplayName;
	// Dont edit this. Should be unique (Is it?)
	std::string m_sProfileID;
	/**
	 * @brief The last used name for high scoring purposes.
	 *
	 * This really shouldn't be in "editable", but it's needed in the smaller
	 * editable file so that it can be ready quickly. */
	std::string m_sLastUsedHighScoreName;

	// General data
	static auto MakeGuid() -> std::string;
	auto GetGuid() -> std::string* { return &m_sGuid; }
	std::string m_sGuid;
	std::map<std::string, std::string> m_sDefaultModifiers;
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
	float m_fPlayerSkillsets[NUM_Skillset]{};
	/** @brief Is this a brand new profile? */
	bool m_bNewProfile{ false };

	// seriously why is this not a thing -mina
	std::string profiledir;
	bool IsEtternaProfile{ false };
	/**
	 * @brief Which machine did we play on last, based on the Guid?
	 *
	 * This is mutable because it's overwritten on save, but is usually
	 * const everywhere else. It was decided to keep const on the whole
	 * save chain and keep this mutable. -Chris */
	mutable std::string m_sLastPlayedMachineGuid;
	mutable DateTime m_LastPlayedDate;
	/* These stats count twice in the machine profile if two players are
	 * playing; that's the only approach that makes sense for ByDifficulty and
	 * ByMeter. */
	std::map<StyleID, int> m_iNumSongsPlayedByStyle;
	int m_iNumSongsPlayedByDifficulty[NUM_Difficulty]{};
	int m_iNumSongsPlayedByMeter[MAX_METER + 1]{};
	/**
	 * @brief Count the total number of songs played.
	 *
	 * This stat counts once per song, even if two players are active. */
	int m_iNumTotalSongsPlayed{ 0 };
	int m_iNumStagesPassedByGrade[NUM_Grade]{};

	// if anymore of these are added they should be enum'd to reduce copy pasta
	// -mina and also should be sets
	void AddToFavorites(const std::string& ck) { FavoritedCharts.emplace(ck); }
	void AddToPermaMirror(const std::string& ck)
	{
		PermaMirrorCharts.emplace(ck);
	}
	void RemoveFromFavorites(const std::string& ck);
	void RemoveFromPermaMirror(const std::string& ck);
	std::set<std::string> FavoritedCharts;
	std::set<std::string> PermaMirrorCharts;

	// more future goalman stuff -mina
	bool AddGoal(const std::string& ck);
	void RemoveGoal(const std::string& ck, DateTime assigned);
	std::unordered_map<std::string, GoalsForChart> goalmap;
	void FillGoalTable();
	std::vector<ScoreGoal*> goaltable;
	int sortmode = 1;	// 1=date 2=rate 3=name 4=priority 5=diff, init to name
						// because that's the default- mina
	int filtermode = 1; // 1=all, 2=completed, 3=uncompleted
	bool asc = false;

	auto HasGoal(const std::string& ck) const -> bool
	{
		return goalmap.count(ck) == 1;
	}
	auto GetLowestGoalForRate(const std::string& ck, float rate) -> ScoreGoal&;
	void SetAnyAchievedGoals(const std::string& ck,
							 float& rate,
							 const HighScore& pscore);

	/* store arbitrary data for the theme within a profile */
	LuaTable m_UserTable;

	// this actually does use scoreman atm
	auto GetBestGrade(const Song* song, StepsType st) const -> Grade;
	auto GetBestWifeScore(const Song* song, StepsType st) const -> float;

	// Screenshot Data
	std::vector<Screenshot> m_vScreenshots;
	void AddScreenshot(const Screenshot& screenshot);
	int GetNextScreenshotIndex() const { return static_cast<int>(m_vScreenshots.size()); }

	// Init'ing
	void InitAll()
	{
		InitEditableData();
		InitGeneralData();
		InitScreenshotData();
	}
	void InitEditableData();
	void InitGeneralData();
	void InitScreenshotData();
	void ClearStats();

	void swap(Profile& other);

	// Loading and saving
	void HandleStatsPrefixChange(std::string dir);
	auto LoadAllFromDir(const std::string& sDir, LoadingWindow* ld)
	  -> ProfileLoadResult;
	auto LoadStatsFromDir(std::string dir, bool require_signature)
	  -> ProfileLoadResult;
	void LoadTypeFromDir(const std::string& dir);
	void LoadCustomFunction(const std::string& sDir);
	auto SaveAllToDir(const std::string& sDir) const -> bool;

	auto LoadEditableDataFromDir(const std::string& sDir) -> ProfileLoadResult;

	void SaveTypeToDir(const std::string& dir) const;
	void SaveEditableDataToDir(const std::string& sDir) const;

	void CalculateStatsFromScores(LoadingWindow* ld);
	void CalculateStatsFromScores();

	void SaveStatsWebPageToDir(const std::string& sDir) const;

	static void MoveBackupToDir(const std::string& sFromDir,
								const std::string& sToDir);
	static auto MakeUniqueFileNameNoExtension(
	  const std::string& sDir,
	  const std::string& sFileNameBeginning) -> std::string;
	static auto MakeFileNameNoExtension(const std::string& sFileNameBeginning,
										int iIndex) -> std::string;

	// Lua
	void PushSelf(lua_State* L);

  private:
	XMLProfile XMLProf;
	DBProfile DBProf;
};

#endif
