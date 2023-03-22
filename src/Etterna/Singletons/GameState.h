#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "MessageManager.h"
#include "Etterna/Models/Misc/ModsGroup.h"
#include "Etterna/Models/Songs/SongPosition.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Models/Misc/PlayerNumber.h"

struct Game;
struct lua_State;
class LuaTable;
class PlayerState;
class PlayerOptions;
class Profile;
class Song;
class Steps;
class StageStats;
class Style;
class TimingData;
class SongOptions;

auto
GetDefaultSort() -> SortOrder;

extern int mina_calc_version;

/** @brief Holds game data that is not saved between sessions. */
class GameState
{
	/** @brief The player number used with Styles where one player controls both
	 * sides. */
	PlayerNumber masterPlayerNumber;
	/** @brief The TimingData that is used for processing certain functions. */
	TimingData* processedTiming;

  public:
	/** @brief Set up the GameState with initial values. */
	GameState();
	~GameState();
	/** @brief Reset the GameState back to initial values. */
	void Reset();
	void ResetPlayer(PlayerNumber pn);
	void ResetPlayerOptions(PlayerNumber pn);
	void ApplyCmdline(); // called by Reset
	void ApplyGameCommand(const std::string& sCommand,
						  PlayerNumber pn = PLAYER_INVALID);
	/** @brief Start the game when the first player joins in. */
	void BeginGame();
	void JoinPlayer(PlayerNumber pn);
	void UnjoinPlayer(PlayerNumber pn);
	auto JoinInput(PlayerNumber pn) -> bool;
	auto JoinPlayers() -> bool;
	void LoadProfiles(bool bLoadEdits = true);
	void SavePlayerProfile();
	auto HaveProfileToLoad() -> bool;
	auto HaveProfileToSave() -> bool;
	void AddStageToPlayer(PlayerNumber pn);
	void LoadCurrentSettingsFromProfile(PlayerNumber pn);
	/**
	 * @brief Save the specified player's settings to his/her profile.
	 *
	 * This is called at the beginning of each stage.
	 * @param pn the PlayerNumber to save the stats to. */
	void SaveCurrentSettingsToProfile(PlayerNumber pn);
	[[nodiscard]] auto GetDefaultSong() const -> Song*;

	auto CanSafelyEnterGameplay(std::string& reason) -> bool;
	void SetCompatibleStylesForPlayers();
	void ForceOtherPlayersToCompatibleSteps(PlayerNumber main);

	void Update(float fDelta);

	// Main state info

	/**
	 * @brief State what the current game is.
	 *
	 * Call this instead of m_pCurGame.Set to make sure that
	 * PREFSMAN->m_sCurrentGame stays in sync.
	 * @param pGame the game to start using. */
	void SetCurGame(const Game* pGame);
	BroadcastOnChangePtr<const Game> m_pCurGame;

  private: // DO NOT access directly.  Use Get/SetCurrentStyle.
	BroadcastOnChangePtr<const Style> m_pCurStyle;
	// Only used if the Game specifies that styles are separate.
	Style const* m_SeparatedStyles[NUM_PlayerNumber];

  public:
	/** @brief Determine which side is joined.
	 *
	 * The left side is player 1, and the right side is player 2. */
	bool m_bSideIsJoined; // left side, right side
	MultiPlayerStatus m_MultiPlayerStatus[NUM_MultiPlayer];

	bool m_bPlayingMulti = false;
	int m_iNumMultiplayerNoteFields;
	auto ChangePreferredDifficultyAndStepsType(PlayerNumber pn,
											   Difficulty dc,
											   StepsType st) -> bool;
	auto ChangePreferredDifficulty(PlayerNumber pn, int dir) -> bool;
	[[nodiscard]] auto GetClosestShownDifficulty(PlayerNumber pn) const
	  -> Difficulty;
	[[nodiscard]] auto GetEasiestStepsDifficulty() const -> Difficulty;
	[[nodiscard]] auto GetHardestStepsDifficulty() const -> Difficulty;
	RageTimer m_timeGameStarted; // from the moment the first
								 // player pressed Start
	LuaTable* m_Environment;

	// This is set to a random number per-game/round; it can be used for a
	// random seed.
	int m_iGameSeed, m_iStageSeed;

	void SetNewStageSeed();

	/**
	 * @brief Determine if a second player can join in at this time.
	 * @return true if a player can still enter the game, false otherwise. */
	[[nodiscard]] auto PlayersCanJoin() const -> bool;

	[[nodiscard]] auto GetCurrentGame() const -> const Game*;
	[[nodiscard]] auto GetCurrentStyle(PlayerNumber pn) const -> const Style*;
	void SetCurrentStyle(const Style* style, PlayerNumber pn);
	auto SetCompatibleStyle(StepsType stype, PlayerNumber pn) -> bool;

	[[nodiscard]] auto IsPlayerEnabled(PlayerNumber pn) const -> bool;
	[[nodiscard]] auto IsMultiPlayerEnabled(MultiPlayer mp) const -> bool;
	auto IsPlayerEnabled(const PlayerState* pPlayerState) const -> bool;
	[[nodiscard]] auto GetNumPlayersEnabled() const -> int;

	/**
	 * @brief Is the specified Player a human Player?
	 * @param pn the numbered Player to check.
	 * @return true if it's a human Player, or false otherwise. */
	[[nodiscard]] auto IsHumanPlayer(PlayerNumber pn) const -> bool;
	[[nodiscard]] auto GetNumHumanPlayers() const -> int;
	[[nodiscard]] auto GetFirstHumanPlayer() const -> PlayerNumber;
	[[nodiscard]] auto GetFirstDisabledPlayer() const -> PlayerNumber;
	[[nodiscard]] auto IsCpuPlayer(PlayerNumber pn) const -> bool;
	[[nodiscard]] auto AnyPlayersAreCpu() const -> bool;

	/**
	 * @brief Retrieve the present master player number.
	 * @return The master player number. */
	[[nodiscard]] auto GetMasterPlayerNumber() const -> PlayerNumber;

	/**
	 * @brief Set the master player number.
	 * @param p the master player number. */
	void SetMasterPlayerNumber(PlayerNumber p);

	/**
	 * @brief Retrieve the present timing data being processed.
	 * @return the timing data pointer. */
	[[nodiscard]] auto GetProcessedTimingData() const -> TimingData*;

	/**
	 * @brief Set the timing data to be used with processing.
	 * @param t the timing data. */
	void SetProcessedTimingData(TimingData* t);

	BroadcastOnChange<std::string>
	  m_sPreferredSongGroup;		  // GROUP_ALL denotes no preferred group
	bool m_bFailTypeWasExplicitlySet; // true if FailType was changed in the
									  // song options screen
	BroadcastOnChange<StepsType> m_PreferredStepsType;
	BroadcastOnChange<Difficulty> m_PreferredDifficulty;
	BroadcastOnChange<SortOrder> m_SortOrder; // set by MusicWheel
	SortOrder m_PreferredSortOrder;			  // used by MusicWheel

	int m_iNumStagesOfThisSong;
	// Used by GameplayScreen to know if it needs to call NSMAN
	bool m_bInNetGameplay = false;

	/**
	 * @brief Increase this every stage while not resetting on a continue.
	 *
	 * This is cosmetic: it's not use for Stage or Screen branching logic. */
	int m_iCurrentStageIndex;
	/**
	 * @brief The number of stages available for the players.
	 *
	 * This resets whenever a player joins or continues. */
	int m_iPlayerStageTokens;

	std::string sExpandedSectionName;

	static auto GetNumStagesMultiplierForSong(const Song* pSong) -> int;
	[[nodiscard]] auto GetNumStagesForCurrentSongAndStepsOrCourse() const
	  -> int;

	void BeginStage();
	void CancelStage();
	void CommitStageStats();
	void FinishStage();
	[[nodiscard]] auto GetCourseSongIndex() const -> int;
	[[nodiscard]] auto GetPlayerDisplayName(PlayerNumber pn) const
	  -> std::string;

	bool m_bLoadingNextSong;
	[[nodiscard]] auto GetLoadingCourseSongIndex() const -> int;

	static auto GetEtternaVersion() -> std::string { return "0.72.3"; }

	/* is this the best place for this? it's not exactly a pref, and we
	 * shouldn't be copying and pasting these values everywhere as needed j1-j4
	 * are now all 1.f to remove j1-3 without having to mess with expected array
	 * sizes in other areas yes i know this is lazy */
	std::vector<float> timingscales = { 1.00F, 1.00F, 1.00F, 1.00F, 0.84F,
										0.66F, 0.50F, 0.33F, 0.20F };
	bool isplaylistcourse = false;
	[[nodiscard]] auto IsPlaylistCourse() const -> bool
	{
		return isplaylistcourse;
	}
	auto CountNotesSeparately() -> bool;

	// State Info used during gameplay

	// NULL on ScreenSelectMusic if the currently selected wheel item isn't a
	// Song.
	BroadcastOnChangePtrWithSelf<Song> m_pCurSong;
	// The last Song that the user manually changed to.
	Song* m_pPreferredSong;
	BroadcastOnChangePtrWithSelf<Steps> m_pCurSteps;

	// Music statistics:
	SongPosition m_Position;

	BroadcastOnChange<bool> m_bGameplayLeadIn;

	// if re-adding noteskin changes in courses, add functions and such here -aj
	void GetAllUsedNoteSkins(std::vector<std::string>& out) const;

	static const float MUSIC_SECONDS_INVALID;

	void ResetMusicStatistics(); // Call this when it's time to play a new song.
								 // Clears the values above.
	void SetPaused(bool p) { m_paused = p; }
	[[nodiscard]] auto GetPaused() const -> bool { return m_paused; }
	void UpdateSongPosition(float fPositionSeconds,
							const TimingData& timing,
							const RageTimer& timestamp = RageZeroTimer);
	[[nodiscard]] auto GetSongPercent(float beat) const -> float;

	[[nodiscard]] auto AllAreInDangerOrWorse() const -> bool;
	[[nodiscard]] auto OneIsHot() const -> bool;

	// used by themes that support heart rate entry.
	RageTimer m_DanceStartTime;
	float m_DanceDuration;

	/** @brief Call this function when it's time to play a new stage. */
	void ResetStageStatistics();

	// Options stuff
	ModsGroup<SongOptions> m_SongOptions;

	/**
	 * @brief Did the current game mode change the default Noteskin?
	 *
	 * This is true if it has: see Edit/Sync Songs for a common example.
	 * Note: any mode that wants to use this must set it explicitly. */
	bool m_bDidModeChangeNoteSkin{ false };

	void GetDefaultPlayerOptions(PlayerOptions& po);
	void GetDefaultSongOptions(SongOptions& so);
	void ResetToDefaultSongOptions(ModsLevel l);
	void ApplyPreferredModifiers(PlayerNumber pn,
								 const std::string& sModifiers);
	void ApplyStageModifiers(PlayerNumber pn, const std::string& sModifiers);

	auto CurrentOptionsDisqualifyPlayer(PlayerNumber pn) -> bool;
	auto PlayerIsUsingModifier(PlayerNumber pn, const std::string& sModifier)
	  -> bool;

	auto GetPlayerFailType(const PlayerState* pPlayerState) const -> FailType;

	[[nodiscard]] auto GetNumSidesJoined() const -> int;
	// PlayerState
	/** @brief Allow access to each player's PlayerState. */
	PlayerState* m_pPlayerState;
	PlayerState* m_pMultiPlayerState[NUM_MultiPlayer];

	auto GetNumCols(int pn) -> int;

	// Preferences
	static Preference<bool> m_bAutoJoin;
	static Preference<bool> DisableChordCohesion;

	// These options have weird interactions depending on m_bEventMode,
	// so wrap them.
	bool m_bTemporaryEventMode;
	[[nodiscard]] auto IsEventMode() const -> bool;

	// Edit stuff

	/**
	 * @brief Is the game right now using Song timing or Steps timing?
	 *
	 * Different options are available depending on this setting. */
	bool m_bIsUsingStepTiming{ true };
	BroadcastOnChange<std::string> m_sEditLocalProfileID;
	auto GetEditLocalProfile() -> Profile*;

	// Current mode of Gameplay
	BroadcastOnChange<GameplayMode> m_gameplayMode;
	[[nodiscard]] auto GetGameplayMode() const -> GameplayMode
	{
		return m_gameplayMode;
	}
	void TogglePracticeModeSafe(bool set);
	void TogglePracticeMode(bool set);
	auto IsPracticeMode() -> bool;

	// A "persistent" way to know if we restarted gameplay (hack)
	bool m_bRestartedGameplay;

	// Discord Rich Presence
	void discordInit();
	void updateDiscordPresence(const std::string& largeImageText,
							   const std::string& details,
							   const std::string& state,
							   int64_t endTime);
	void updateDiscordPresenceMenu(const std::string& largeImageText);

	// Lua
	void PushSelf(lua_State* L);

	// Keep extra stage logic internal to GameState.
  private:
	// Timing position corrections
	RageTimer m_LastPositionTimer;
	float m_LastPositionSeconds;
	bool m_paused;

	GameState(const GameState& rhs);
};

extern GameState*
  GAMESTATE; // global and accessible from anywhere in our program

#endif
