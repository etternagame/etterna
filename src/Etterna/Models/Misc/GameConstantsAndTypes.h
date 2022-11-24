/* GameConstantsAndTypes - Things used in many places that don't change often.
 */

#ifndef GAME_CONSTANTS_AND_TYPES_H
#define GAME_CONSTANTS_AND_TYPES_H

#include "EnumHelper.h"
#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include <cfloat> // need the max for default.

// Note definitions
/** @brief Define the mininum difficulty value allowed. */
const int MIN_METER = 1;
/**
 * @brief Define the maximum difficulty value allowed.
 *
 * 35 is used rather than 13 due to a variety of Profile data.
 * For more examples, see Profile::InitGeneralData(). -aj
 */
const int MAX_METER = 35;

// some components of the game care about some negative number
// and break if you pass it
// so this defines the point at which behavior is undefined
// usually you only reach this if you have a huge starting bpm
const int ARBITRARY_MIN_GAMEPLAY_NUMBER = -200000;

// the hard end of the boo window and start of the miss window
// dont let anyone hit a note outside of this
const float MISS_WINDOW_BEGIN_SEC = 0.18F;

// 75ms in both directions, around a j5 great
const float MINE_WINDOW_SEC = 0.075F;

// taps within alive holds/rolls must be this far away from 
// the next note so that they do not count.
// this prevents unwanted cbs from roll tapping
const float TAP_IN_HOLD_REQ_SEC = 0.135F;

enum GameplayMode
{
	GameplayMode_Normal,
	GameplayMode_Practice,
	GameplayMode_Replay,
	NUM_GameplayMode,
	GameplayMode_Invalid
};

auto
SkillsetToString(Skillset ss) -> const std::string&;

auto CalcPatternModToString(CalcPatternMod) -> const std::string&;
auto CalcDiffValueToString(CalcDiffValue) -> const std::string&;
auto CalcDebugMiscToString(CalcDebugMisc) -> const std::string&;

enum NSScoreBoardColumn
{
	NSSB_NAMES = 0,
	NSSB_COMBO,
	NSSB_GRADE,
	NUM_NSScoreBoardColumn,
	NSScoreBoardColumn_Invalid
};

enum ValidationKey
{
	ValidationKey_Brittle,
	ValidationKey_Weak,
	NUM_ValidationKey,
	ValidationKey_Invalid,
};

auto
ValidationKeyToString(ValidationKey ss) -> const std::string&;

/**
 * @brief The various radar categories available.
 *
 * This is just cached song data. Not all of it may actually be displayed
 * in the radar. */
enum RadarCategory
{
	RadarCategory_Notes,		/**< How many notes are in the song? */
	RadarCategory_TapsAndHolds, /**< How many taps and holds are in the song? */
	RadarCategory_Jumps,		/**< How many jumps are in the song? */
	RadarCategory_Holds,		/**< How many holds are in the song? */
	RadarCategory_Mines,		/**< How many mines are in the song? */
	RadarCategory_Hands,		/**< How many hands are in the song? */
	RadarCategory_Rolls,		/**< How many rolls are in the song? */
	RadarCategory_Lifts,		/**< How many lifts are in the song? */
	RadarCategory_Fakes,		/**< How many fakes are in the song? */
	NUM_RadarCategory,			/**< The number of radar categories. */
	RadarCategory_Invalid
};
/**
 * @brief Turn the radar category into a proper string.
 * @param cat the radar category.
 * @return the string version of the radar category.
 */
auto
RadarCategoryToString(RadarCategory cat) -> const std::string&;
/**
 * @brief Turn the radar category into a proper localized string.
 * @param cat the radar category.
 * @return the localized string version of the radar category.
 */
auto
RadarCategoryToLocalizedString(RadarCategory cat) -> const std::string&;
LuaDeclareType(RadarCategory);

/** @brief The different game categories available to play. */
enum StepsTypeCategory
{
	StepsTypeCategory_Single, /**< One person plays on one side. */
	StepsTypeCategory_Double, /**< One person plays on both sides. */
};

/** @brief The different steps types for playing. */
enum StepsType
{
	StepsType_dance_single = 0,
	StepsType_dance_double,
	StepsType_dance_solo,
	StepsType_dance_threepanel,
	StepsType_pump_single,
	StepsType_pump_halfdouble,
	StepsType_pump_double,
	StepsType_kb7_single,
	StepsType_ez2_single,
	StepsType_ez2_double,
	StepsType_ez2_real,
	StepsType_ds3ddx_single,
	StepsType_beat_single5,
	StepsType_beat_double5,
	StepsType_beat_single7,
	StepsType_beat_double7,
	StepsType_maniax_single,
	StepsType_maniax_double,
	StepsType_popn_five,
	StepsType_popn_nine,
	NUM_StepsType, // leave this at the end
	StepsType_Invalid,
};
LuaDeclareType(StepsType);

/**
 * @brief The list of ways to sort songs
 */
enum SortOrder
{
	SORT_GROUP,		 /**< Sort by the groups the Songs are in. */
	SORT_TITLE,		 /**< Sort by the Song's title. */
	SORT_BPM,		 /**< Sort by the Song's BPM. */
	SORT_TOP_GRADES, /**< Sort by the highest grades earned on a Song. */
	SORT_ARTIST,	 /**< Sort by the name of the artist of the Song. */
	SORT_GENRE,		 /**< Sort by the Song's genre. */
	SORT_MODE_MENU,	 /**< Have access to the menu for choosing the sort. */
	SORT_FAVORITES,
	SORT_Overall,
	SORT_Stream,
	SORT_Jumpstream,
	SORT_Handstream,
	SORT_Stamina,
	SORT_JackSpeed,
	SORT_Chordjack,
	SORT_Technical,
	SORT_LENGTH,
	SORT_Ungrouped, /**< Sort by the song title, putting all songs into single
					   group. */
	NUM_SortOrder,
	SortOrder_Invalid
};

/**
 * @brief Turn the sort order into a proper string.
 * @param so the sort order.
 * @return the string version of the sort order.
 */
auto
SortOrderToString(SortOrder so) -> const std::string&;
/**
 * @brief Turn the sort order into a proper localized string.
 * @param so the sort order.
 * @return the localized string version of the sort order.
 */
auto
SortOrderToLocalizedString(SortOrder so) -> const std::string&;
/**
 * @brief Turn the string into the proper sort order.
 * @param str the string.
 * @return the sort order based on the string.
 */
auto
StringToSortOrder(const std::string& str) -> SortOrder;
LuaDeclareType(SortOrder);
/**
 * @brief Determine if the sort order in question is for songs or not.
 *
 * This function is mainly used for saving sort order to the profile. -aj
 */
inline auto
IsSongSort(SortOrder so) -> bool
{
	return so >= SORT_GROUP && so <= SORT_GENRE;
}

/** @brief The list of tap note scores available during play. */
enum TapNoteScore
{
	TNS_None,			/**< There is no score involved with this one. */
	TNS_HitMine,		/**< A mine was hit successfully. */
	TNS_AvoidMine,		/**< A mine was avoided successfully. */
	TNS_CheckpointMiss, /**< A checkpoint was missed during a hold. */
	TNS_Miss,			/**< A note was missed entirely. */
	TNS_W5,				/**< A note was almost missed, but not quite. */
	TNS_W4,				/**< A note was hit either a bit early or a bit late. */
	TNS_W3, /**< A note was hit with decent accuracy, but not the best. */
	TNS_W2, /**< A note was hit off by just a miniscule amount. This used to be
			   the best rating. */
	TNS_W1, /**< A note was hit perfectly. */
	TNS_CheckpointHit, /**< A checkpoint was held during a hold. */
	NUM_TapNoteScore,  /**< The number of Tap Note Scores available. */
	TapNoteScore_Invalid,
};
/**
 * @brief Turn the tap note score into a proper string.
 * @param tns the tap note score.
 * @return the string version of the tap note score.
 */
auto
TapNoteScoreToString(TapNoteScore tns) -> const std::string&;
/**
 * @brief Turn the tap note score into a proper localized string.
 * @param tns the tap note score.
 * @return the localized string version of the tap note score.
 */
auto
TapNoteScoreToLocalizedString(TapNoteScore tns) -> const std::string&;
/**
 * @brief Turn the string into the proper tap note score.
 * @param str the string.
 * @return the tap note score based on the string.
 */
auto
StringToTapNoteScore(const std::string& str) -> TapNoteScore;
LuaDeclareType(TapNoteScore);

/** @brief The list of hold note scores available during play. */
enum HoldNoteScore
{
	HNS_None,	/**< The HoldNote was not scored yet. */
	HNS_LetGo,	/**< The HoldNote has passed, but the player missed it. */
	HNS_Held,	/**< The HoldNote has passed, and was successfully held all the
				   way. */
	HNS_Missed, /**< The HoldNote has passed, and was never initialized. */
	NUM_HoldNoteScore, /**< The number of hold note scores. */
	HoldNoteScore_Invalid,
};
/**
 * @brief Turn the hold note score into a proper string.
 * @param hns the hold note score.
 * @return the string version of the hold note score.
 */
auto
HoldNoteScoreToString(HoldNoteScore hns) -> const std::string&;

/**
 * @brief Turn the hold note score into a proper localized string.
 * @param hns the hold note score.
 * @return the localized string version of the hold note score.
 */
auto
HoldNoteScoreToLocalizedString(HoldNoteScore hns) -> const std::string&;
/**
 * @brief Turn the string into the proper hold note score.
 * @param str the string.
 * @return the hold note score based on the string.
 */
auto
StringToHoldNoteScore(const std::string& str) -> HoldNoteScore;
LuaDeclareType(HoldNoteScore);

/** @brief The list of timing windows to deal with when playing. */
enum TimingWindow
{
	TW_W1,
	TW_W2,
	TW_W3,
	TW_W4,
	TW_W5,
	TW_Mine,
	TW_Hold,
	TW_Roll,
	TW_Checkpoint,
	NUM_TimingWindow
};
auto
TimingWindowToString(TimingWindow tw) -> const std::string&;

/** @brief The list of score events that can take place while playing. */
enum ScoreEvent
{
	SE_CheckpointHit,
	SE_W1,
	SE_W2,
	SE_W3,
	SE_W4,
	SE_W5,
	SE_Miss,
	SE_HitMine,
	SE_CheckpointMiss,
	SE_Held,
	SE_LetGo,
	SE_Missed,
	NUM_ScoreEvent
};
auto
ScoreEventToString(ScoreEvent se) -> const std::string&;

/** @brief The list of game button types available for all game modes. */
enum GameButtonType
{
	GameButtonType_Step,
	GameButtonType_Menu
};

/** @brief The list of judge types for the tap note scores. */
enum TapNoteScoreJudgeType
{
	TapNoteScoreJudgeType_MinimumScore,
	TapNoteScoreJudgeType_LastScore,
	NUM_TapNoteScoreJudgeType,
	TapNoteScoreJudgeType_Invalid,
};
auto
TapNoteScoreJudgeTypeToString(TapNoteScoreJudgeType jt) -> const std::string&;
LuaDeclareType(TapNoteScoreJudgeType);

/** @brief The profile slots available. This is mainly for Profiles and Memory
 * Cards. */
enum ProfileSlot
{
	ProfileSlot_Player1,
	ProfileSlot_Player2,
	NUM_ProfileSlot,
	ProfileSlot_Invalid
};
auto
ProfileSlotToString(ProfileSlot ps) -> const std::string&;
LuaDeclareType(ProfileSlot);

extern const std::string RANKING_TO_FILL_IN_MARKER;
inline auto
IsRankingToFillIn(const std::string& sName) -> bool
{
	return !sName.empty() && sName[0] == '#';
}

// Group stuff
extern const std::string GROUP_ALL;

/** @brief The different types of players in the game. */
enum PlayerController
{
	PC_HUMAN,
	PC_AUTOPLAY,
	PC_CPU,
	PC_REPLAY,
	NUM_PlayerController,
	PlayerController_Invalid
};
auto
PlayerControllerToString(PlayerController pc) -> const std::string&;
LuaDeclareType(PlayerController);

/** @brief The different health bar states. */
enum HealthState
{
	HealthState_Hot,	/**< The health bar is very full. */
	HealthState_Alive,	/**< The health bar is at a decent size. */
	HealthState_Danger, /**< The health bar is about to run out. */
	HealthState_Dead,	/**< The health bar is drained completely. */
	NUM_HealthState,
	HealthState_Invalid
};
LuaDeclareType(HealthState);

/** @brief The list of BPMs to display */
struct DisplayBpms
{
	/**
	 * @brief Add a BPM to the list.
	 * @param f the BPM to add.
	 */
	void Add(float f);
	/**
	 * @brief Retrieve the minimum BPM of the set.
	 * @return the minimum BPM.
	 */
	[[nodiscard]] auto GetMin() const -> float;
	/**
	 * @brief Retrieve the maximum BPM of the set.
	 * @return the maximum BPM.
	 */
	[[nodiscard]] auto GetMax() const -> float;
	/**
	 * @brief Retrieve the maximum BPM of the set,
	 * but no higher than a certain value.
	 * @param highest the highest BPM to use.
	 * @return the maximum BPM.
	 */
	[[nodiscard]] auto GetMaxWithin(float highest = FLT_MAX) const -> float;
	/**
	 * @brief Determine if the BPM is really constant.
	 * @return Whether the BPM is constant or not.
	 */
	[[nodiscard]] auto BpmIsConstant() const -> bool;
	/**
	 * @brief Determine if the BPM is meant to be a secret.
	 * @return Whether the BPM is a secret or not.
	 */
	[[nodiscard]] auto IsSecret() const -> bool;
	/**
	 * @brief The list of the BPMs for the song.
	 */
	std::vector<float> vfBpms;
};

/** @brief The various style types available. */
enum StyleType
{
	StyleType_OnePlayerOneSide,	 /**< Single style */
	StyleType_OnePlayerTwoSides, /**< Double style */
	NUM_StyleType,
	StyleType_Invalid
};
auto
StyleTypeToString(StyleType s) -> const std::string&;
auto
StringToStyleType(const std::string& s) -> StyleType;
LuaDeclareType(StyleType);

/**
 * @brief The different types of sample music previews available.
 *
 * These were originally from the deleted screen ScreenEz2SelectMusic.
 * (if no confirm type is mentioned, there is none.)
 *
 * 0 = play music as you select;				SampleMusicPreviewMode_Normal
 * 1 = no music plays, select 1x to play preview music, select again to confirm
 * 2 = no music plays at all (SampleMusicPreviewMode_ScreenMusic
 * + redir to silent) 3 = play music as select, 2x to confirm
 * (SampleMusicPreviewMode_Normal + [SSMusic] TwoPartConfirmsOnly) 4 = screen
 * music plays;					SampleMusicPreviewMode_ScreenMusic
 */
enum SampleMusicPreviewMode
{
	SampleMusicPreviewMode_Normal, /**< Music is played as the song is
									  highlighted. */
	SampleMusicPreviewMode_StartToPreview,
	SampleMusicPreviewMode_ScreenMusic, /**< No music plays. Select it once to
										 * preview the music, then once more to
										 * select the song. */
	SampleMusicPreviewMode_LastSong,	/**< continue playing the last song */
	SampleMusicPreviewMode_Nothing, // music never plays
	NUM_SampleMusicPreviewMode,
	SampleMusicPreviewMode_Invalid,
};
auto SampleMusicPreviewModeToString(SampleMusicPreviewMode)
  -> const std::string&;
auto
StringToSampleMusicPreviewMode(const std::string& s) -> SampleMusicPreviewMode;
LuaDeclareType(SampleMusicPreviewMode);

/**
 * @brief The different kinds of Stages available.
 *
 * These are shared stage values shown in StageDisplay. These are not
 * per-player.
 */
enum Stage
{
	Stage_1st,	  /**< The first stage. */
	Stage_2nd,	  /**< The second stage. */
	Stage_3rd,	  /**< The third stage. */
	Stage_4th,	  /**< The fourth stage. */
	Stage_5th,	  /**< The fifth stage. */
	Stage_6th,	  /**< The sixth stage. */
	Stage_Next,	  /**< Somewhere between the sixth and final stage.
				   * This won't normally happen because 7 stages is the max in the
				   * UI. */
	Stage_Final,  /**< The last stage. */
	Stage_Extra1, /**< The first bonus stage, AKA the extra stage. */
	Stage_Extra2, /**< The last bonus stage, AKA the encore extra stage. */
	Stage_Event,  /**< Playing in event mode. */
	Stage_Demo,	  /**< Playing the demonstration. */
	NUM_Stage,	  /**< The number of stage types. */
	Stage_Invalid,
};
auto
StageToString(Stage s) -> const std::string&;
LuaDeclareType(Stage);
auto
StageToLocalizedString(Stage i) -> const std::string&;

/** @brief The different results of loading a profile. */
enum ProfileLoadResult
{
	ProfileLoadResult_Success,
	ProfileLoadResult_FailedNoProfile,
	ProfileLoadResult_FailedTampered
};

/** @brief The different statuses for multiplayer. */
enum MultiPlayerStatus
{
	MultiPlayerStatus_Joined,
	MultiPlayerStatus_NotJoined,
	MultiPlayerStatus_Unplugged,
	MultiPlayerStatus_MissingMultitap,
	NUM_MultiPlayerStatus,
	MultiPlayerStatus_Invalid
};
auto
MultiPlayerStatusToString(MultiPlayerStatus i) -> const std::string&;

/** @brief How can the Player fail a song? */
enum FailType
{
	FailType_Immediate,			/**< fail immediately when life touches 0 */
	FailType_ImmediateContinue, /**< Same as above, but allow playing the rest
								   of the song */
	FailType_Off,				/**< never fail */
	NUM_FailType,
	FailType_Invalid
};

auto
FailTypeToString(FailType cat) -> const std::string&;
auto
FailTypeToLocalizedString(FailType cat) -> const std::string&;
LuaDeclareType(FailType);

#endif
