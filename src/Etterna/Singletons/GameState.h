#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/Grade.h"
#include "MessageManager.h"
#include "Etterna/Models/Misc/ModsGroup.h"
#include "Etterna/Models/Songs/SongPosition.h"
#include "discord_rpc.h"

#include <deque>

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

SortOrder
GetDefaultSort();

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
	void ApplyGameCommand(const RString& sCommand,
						  PlayerNumber pn = PLAYER_INVALID);
	/** @brief Start the game when the first player joins in. */
	void BeginGame();
	void JoinPlayer(PlayerNumber pn);
	void UnjoinPlayer(PlayerNumber pn);
	bool JoinInput(PlayerNumber pn);
	bool JoinPlayers();
	void LoadProfiles(bool bLoadEdits = true);
	void SavePlayerProfiles();
	void SavePlayerProfile(PlayerNumber pn);
	bool HaveProfileToLoad();
	bool HaveProfileToSave();
	void SaveLocalData();
	void AddStageToPlayer(PlayerNumber pn);
	void LoadCurrentSettingsFromProfile(PlayerNumber pn);
	/**
	 * @brief Save the specified player's settings to his/her profile.
	 *
	 * This is called at the beginning of each stage.
	 * @param pn the PlayerNumber to save the stats to. */
	void SaveCurrentSettingsToProfile(PlayerNumber pn);
	Song* GetDefaultSong() const;

	bool CanSafelyEnterGameplay(RString& reason);
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
	BroadcastOnChange<PlayMode>
	  m_PlayMode; // many screens display different info depending on this value

	bool m_bPlayingMulti = false;
	int m_iNumMultiplayerNoteFields;
	bool DifficultiesLocked() const;
	bool ChangePreferredDifficultyAndStepsType(PlayerNumber pn,
											   Difficulty dc,
											   StepsType st);
	bool ChangePreferredDifficulty(PlayerNumber pn, int dir);
	Difficulty GetClosestShownDifficulty(PlayerNumber pn) const;
	Difficulty GetEasiestStepsDifficulty() const;
	Difficulty GetHardestStepsDifficulty() const;
	RageTimer m_timeGameStarted; // from the moment the first
								 // player pressed Start
	LuaTable* m_Environment;

	// This is set to a random number per-game/round; it can be used for a
	// random seed.
	int m_iGameSeed, m_iStageSeed;
	RString m_sStageGUID;

	void SetNewStageSeed();

	/**
	 * @brief Determine if a second player can join in at this time.
	 * @return true if a player can still enter the game, false otherwise. */
	bool PlayersCanJoin() const;

	const Game* GetCurrentGame() const;
	const Style* GetCurrentStyle(PlayerNumber pn) const;
	void SetCurrentStyle(const Style* style, PlayerNumber pn);
	bool SetCompatibleStyle(StepsType stype, PlayerNumber pn);

	void GetPlayerInfo(PlayerNumber pn, bool& bIsEnabledOut, bool& bIsHumanOut);
	bool IsPlayerEnabled(PlayerNumber pn) const;
	bool IsMultiPlayerEnabled(MultiPlayer mp) const;
	bool IsPlayerEnabled(const PlayerState* pPlayerState) const;
	int GetNumPlayersEnabled() const;

	/**
	 * @brief Is the specified Player a human Player?
	 * @param pn the numbered Player to check.
	 * @return true if it's a human Player, or false otherwise. */
	bool IsHumanPlayer(PlayerNumber pn) const;
	int GetNumHumanPlayers() const;
	PlayerNumber GetFirstHumanPlayer() const;
	PlayerNumber GetFirstDisabledPlayer() const;
	bool IsCpuPlayer(PlayerNumber pn) const;
	bool AnyPlayersAreCpu() const;

	/**
	 * @brief Retrieve the present master player number.
	 * @return The master player number. */
	PlayerNumber GetMasterPlayerNumber() const;

	/**
	 * @brief Set the master player number.
	 * @param p the master player number. */
	void SetMasterPlayerNumber(const PlayerNumber p);

	/**
	 * @brief Retrieve the present timing data being processed.
	 * @return the timing data pointer. */
	TimingData* GetProcessedTimingData() const;

	/**
	 * @brief Set the timing data to be used with processing.
	 * @param t the timing data. */
	void SetProcessedTimingData(TimingData* t);

	/**
	 * @brief Do we show the W1 timing judgment?
	 * @return true if we do, or false otherwise. */
	bool ShowW1() const;

	BroadcastOnChange<RString>
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

	RString sExpandedSectionName;

	static int GetNumStagesMultiplierForSong(const Song* pSong);
	static int GetNumStagesForSongAndStyleType(const Song* pSong, StyleType st);
	int GetNumStagesForCurrentSongAndStepsOrCourse() const;

	void BeginStage();
	void CancelStage();
	void CommitStageStats();
	void FinishStage();
	int GetCourseSongIndex() const;
	RString GetPlayerDisplayName(PlayerNumber pn) const;

	bool m_bLoadingNextSong;
	int GetLoadingCourseSongIndex() const;
	
	RString GetEtternaVersion() { return "0.70.-323"; }

	// is this the best place for this? it's not exactly a pref, and we shouldn't
	// be copying and pasting these values everywhere as needed j1-j4 are now all 1.f
	// to remove j1-3 without having to mess with expected array sizes in other areas
	// yes i know this is lazy
	vector<float> timingscales = { 1.00f, 1.00f, 1.00f, 1.00f, 0.84f, 0.66f, 0.50f, 0.33f, 0.20f };
	bool isplaylistcourse = false;
	bool IsPlaylistCourse() { return isplaylistcourse; }
	bool CountNotesSeparately();

	// State Info used during gameplay

	// NULL on ScreenSelectMusic if the currently selected wheel item isn't a
	// Song.
	BroadcastOnChangePtr<Song> m_pCurSong;
	// The last Song that the user manually changed to.
	Song* m_pPreferredSong;
	BroadcastOnChangePtr<Steps> m_pCurSteps;

	// Music statistics:
	SongPosition m_Position;

	BroadcastOnChange<bool> m_bGameplayLeadIn;

	// if re-adding noteskin changes in courses, add functions and such here -aj
	void GetAllUsedNoteSkins(vector<RString>& out) const;

	static const float MUSIC_SECONDS_INVALID;

	void ResetMusicStatistics(); // Call this when it's time to play a new song.
								 // Clears the values above.
	void SetPaused(bool p) { m_paused = p; }
	bool GetPaused() { return m_paused; }
	void UpdateSongPosition(float fPositionSeconds,
							const TimingData& timing,
							const RageTimer& timestamp = RageZeroTimer);
	float GetSongPercent(float beat) const;

	bool AllAreInDangerOrWorse() const;
	bool OneIsHot() const;

	// used by themes that support heart rate entry.
	RageTimer m_DanceStartTime;
	float m_DanceDuration;
	PlayerNumber GetBestPlayer() const;

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
	void ApplyPreferredModifiers(PlayerNumber pn, const RString& sModifiers);
	void ApplyStageModifiers(PlayerNumber pn, const RString& sModifiers);
	void ResetOptions();

	bool CurrentOptionsDisqualifyPlayer(PlayerNumber pn);
	bool PlayerIsUsingModifier(PlayerNumber pn, const RString& sModifier);

	FailType GetPlayerFailType(const PlayerState* pPlayerState) const;

	int GetNumSidesJoined() const;
	// PlayerState
	/** @brief Allow access to each player's PlayerState. */
	PlayerState* m_pPlayerState;
	PlayerState* m_pMultiPlayerState[NUM_MultiPlayer];

	int GetNumCols(int pn);

	// Preferences
	static Preference<bool> m_bAutoJoin;
	static Preference<bool> DisableChordCohesion;

	// These options have weird interactions depending on m_bEventMode,
	// so wrap them.
	bool m_bTemporaryEventMode;
	bool IsEventMode() const;

	// Edit stuff

	/**
	 * @brief Is the game right now using Song timing or Steps timing?
	 *
	 * Different options are available depending on this setting. */
	bool m_bIsUsingStepTiming{ true };
	BroadcastOnChange<RString> m_sEditLocalProfileID;
	Profile* GetEditLocalProfile();
	bool m_bIsChartPreviewActive;

	// Current mode of Gameplay
	BroadcastOnChange<GameplayMode> m_gameplayMode;
	GameplayMode GetGameplayMode() { return m_gameplayMode; }
	void TogglePracticeModeSafe(bool set);
	void TogglePracticeMode(bool set);
	bool IsPracticeMode();

	// A "persistent" way to know if we restarted gameplay (hack)
	bool m_bRestartedGameplay;

	// Discord Rich Presence
	void discordInit();
	void updateDiscordPresence(const RString& largeImageText,
							   const RString& details,
							   const RString& state,
							   const int64_t endTime);
	void updateDiscordPresenceMenu(const RString& largeImageText);

	// Lua
	void PushSelf(lua_State* L);

	// Keep extra stage logic internal to GameState.
  private:
	// Timing position corrections
	RageTimer m_LastPositionTimer;
	float m_LastPositionSeconds;
	bool m_paused;

	GameState(const GameState& rhs);
	GameState& operator=(const GameState& rhs);
};

extern GameState*
  GAMESTATE; // global and accessible from anywhere in our program

#endif
