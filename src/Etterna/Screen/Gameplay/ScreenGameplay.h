#ifndef SCREEN_GAMEPLAY_H
#define SCREEN_GAMEPLAY_H

#include "Etterna/Models/Misc/AutoKeysounds.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/GameplayAssist.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Actor/Gameplay/LyricDisplay.h"
#include "Etterna/Models/Misc/PlayerStageStats.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "RageUtil/Sound/RageSound.h"
#include "Etterna/Screen/Others/ScreenWithMenuElements.h"
#include "Etterna/Models/Misc/SoundEffectControl.h"
#include "Etterna/Actor/Base/Sprite.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/Actor/GameplayAndMenus/Transition.h"

class LyricsLoader;
class Player;
class LifeMeter;
class StepsDisplay;
class ScoreKeeper;
class Background;
class Foreground;

AutoScreenMessage(SM_NotesEnded);
AutoScreenMessage(SM_BeginFailed);
AutoScreenMessage(SM_LeaveGameplay);

class PlayerInfo
{
  public:
	PlayerInfo();
	~PlayerInfo();

	void Load(PlayerNumber pn,
			  MultiPlayer mp,
			  bool bShowNoteField,
			  int iAddToDifficulty);
	void LoadDummyP1(int iDummyIndex, int iAddToDifficulty);

	/**
	 * @brief Retrieve the player's state and stage stats index.
	 * @return the player's state and stage stats index.
	 */
	MultiPlayer GetPlayerStateAndStageStatsIndex()
	{
		return m_pn == PLAYER_INVALID ? m_mp : static_cast<MultiPlayer>(m_pn);
	}
	PlayerState* GetPlayerState();
	PlayerStageStats* GetPlayerStageStats();
	PlayerNumber GetStepsAndTrailIndex()
	{
		return m_pn == PLAYER_INVALID ? PLAYER_1 : m_pn;
	}
	/**
	 * @brief Determine if the player information is enabled.
	 * @return its success or failure. */
	bool IsEnabled();
	/**
	 * @brief Determine if we're in MultiPlayer.
	 * @return true if it is MultiPlayer, false otherwise. */
	bool IsMultiPlayer() const { return m_mp != MultiPlayer_Invalid; }
	/**
	 * @brief Retrieve the name of the Player based on the mode.
	 * @return the name of the Player. */
	RString GetName() const
	{
		if (m_bIsDummy)
			return ssprintf("Dummy%d", m_iDummyIndex);
		if (IsMultiPlayer())
			return MultiPlayerToString(m_mp);
		else
			return PlayerNumberToString(m_pn);
	}

	// Lua
	void PushSelf(lua_State* L);

	/** @brief The present Player's number. */
	PlayerNumber m_pn;
	/** @brief The present Player's multiplayer number. */
	MultiPlayer m_mp{ MultiPlayer_Invalid };
	bool m_bIsDummy{ false };
	int m_iDummyIndex{ 0 };
	int m_iAddToDifficulty{ 0 };	// if > 0, use the Nth harder Steps
	bool m_bPlayerEnabled{ false }; // IsEnabled cache for iterators
	PlayerState m_PlayerStateDummy;
	PlayerStageStats m_PlayerStageStatsDummy;
	SoundEffectControl m_SoundEffectControl;

	/**
	 * @brief The list of Steps a player has to go through in this set.
	 *
	 * The size may be greater than 1 if playing a course. */
	std::vector<Steps*> m_vpStepsQueue;

	/** @brief The LifeMeter showing a Player's health. */
	LifeMeter* m_pLifeMeter;
	/** @brief The description of the current Steps. */
	BitmapText* m_ptextStepsDescription;
	/** @brief The primary ScoreKeeper for keeping track of the score. */
	ScoreKeeper* m_pPrimaryScoreKeeper;
	/** @brief The secondary ScoreKeeper. Formerly used in PLAY_MODE_RAVE. */
	ScoreKeeper* m_pSecondaryScoreKeeper;
	/** @brief The current PlayerOptions that are activated. */
	BitmapText* m_ptextPlayerOptions;
	/** @brief The current attack modifiers that are in play for the moment. */

	/** @brief The NoteData the Player has to get through. */
	NoteData m_NoteData;
	/** @brief The specific Player that is going to play. */
	Player* m_pPlayer;

	StepsDisplay* m_pStepsDisplay;
};

/** @brief The music plays, the notes scroll, and the Player is pressing
 * buttons. */
class ScreenGameplay : public ScreenWithMenuElements
{
  public:
	ScreenGameplay();
	void Init() override;
	~ScreenGameplay() override;
	void BeginScreen() override;

	void Update(float fDeltaTime) override;
	bool Input(const InputEventPlus& input) override;
	void HandleScreenMessage(ScreenMessage SM) override;
	void HandleMessage(const Message& msg) override;
	void Cancel(ScreenMessage smSendWhenDone) override;

	void DrawPrimitives() override;

	/**
	 * @brief Retrieve the current ScreenType.
	 * @return the gameplay ScreenType. */
	ScreenType GetScreenType() const override { return gameplay; }

	/**
	 * @brief Determine if we are to center the columns for just one player.
	 * @return true if we center the solo player, false otherwise. */
	bool Center1Player() const;

	// Lua
	void PushSelf(lua_State* L) override;
	LifeMeter* GetLifeMeter(PlayerNumber pn);
	PlayerInfo* GetPlayerInfo(PlayerNumber pn);
	PlayerInfo* GetDummyPlayerInfo(int iDummyIndex);

	void FailFadeRemovePlayer(PlayerInfo* pi);
	void FailFadeRemovePlayer(PlayerNumber pn);
	void BeginBackingOutFromGameplay();

	// Set the playback rate in the middle of gameplay
	float SetRate(float newRate);
	// Move the current position of the song in the middle of gameplay
	void SetSongPosition(float newPositionSeconds);
	// Set the playback rate in the middle of gameplay, in practice mode only
	float AddToPracticeRate(float amountAdded);
	// Move the current position of the song in the middle of gameplay, in practice mode only
	void SetPracticeSongPosition(float newPositionSeconds);
	// Get current position of the song during gameplay
	const float GetSongPosition();
	// Toggle pause. Don't use this outside of replays.
	// Please.
	// I'm serious.
	void ToggleReplayPause();
	// Toggle pause. It's for practice mode.
	// ignore the comment above
	void TogglePracticePause();
	float m_fReplayBookmarkSeconds;

  protected:
	virtual void UpdateStageStats(
	  MultiPlayer /* mp */){}; // overridden for multiplayer

	virtual bool UseSongBackgroundAndForeground() const { return true; }

	ThemeMetric<RString> PLAYER_TYPE;
	ThemeMetric<RString> SCORE_DISPLAY_TYPE;
	ThemeMetric<apActorCommands> PLAYER_INIT_COMMAND;
	LocalizedString GIVE_UP_START_TEXT;
	LocalizedString GIVE_UP_BACK_TEXT;
	LocalizedString GIVE_UP_ABORTED_TEXT;
	LocalizedString SKIP_SONG_TEXT;
	ThemeMetric<float> GIVE_UP_SECONDS;
	ThemeMetric<float> MUSIC_FADE_OUT_SECONDS;
	ThemeMetric<float> OUT_TRANSITION_LENGTH;
	ThemeMetric<float> BEGIN_FAILED_DELAY;
	ThemeMetric<float> MIN_SECONDS_TO_STEP;
	ThemeMetric<float> MIN_SECONDS_TO_MUSIC;
	ThemeMetric<float> MIN_SECONDS_TO_STEP_NEXT_SONG;
	ThemeMetric<bool> START_GIVES_UP;
	ThemeMetric<bool> BACK_GIVES_UP;
	ThemeMetric<bool> SELECT_SKIPS_SONG;
	ThemeMetric<bool> GIVING_UP_GOES_TO_PREV_SCREEN;
	/** @brief The miss combo a player needs to fail out of a song. */
	ThemeMetric<int> FAIL_ON_MISS_COMBO;
	ThemeMetric<bool> ALLOW_CENTER_1_PLAYER;
	ThemeMetric<RString> SONG_NUMBER_FORMAT;

	void SetupSong(int iSongIndex);
	void ReloadCurrentSong();
	virtual void LoadNextSong();
	void StartPlayingSong(float fMinTimeToNotes, float fMinTimeToMusic);
	void GetMusicEndTiming(float& fSecondsToStartFadingOutMusic,
						   float& fSecondsToStartTransitioningOut);
	void PlayAnnouncer(const RString& type,
					   float fSeconds,
					   float* fDeltaSeconds);
	void PlayAnnouncer(const RString& type, float fSeconds)
	{
		PlayAnnouncer(type, fSeconds, &m_fTimeSinceLastDancingComment);
	}
	void SendCrossedMessages();

	void PlayTicks();
	// Used to update some pointers
	void UpdateSongPosition(float fDeltaTime);
	void UpdateLyrics(float fDeltaTime);
	void SongFinished();
	virtual void SaveStats();
	virtual void StageFinished(bool bBackedOut);
	void SaveReplay();
	// bool LoadReplay();
	bool AllAreFailing();

	virtual void InitSongQueues();

	/** @brief The different game states of ScreenGameplay. */
	enum DancingState
	{
		STATE_INTRO =
		  0, /**< The starting state, pressing Back isn't allowed here. */
		STATE_DANCING, /**< The main state where notes have to be pressed. */
		STATE_OUTRO, /**< The ending state, pressing Back isn't allowed here. */
		NUM_DANCING_STATES
	} /** @brief The specific point within ScreenGameplay. */ m_DancingState;

  private:
  protected:
	/**
	 * @brief The songs left to play.
	 *
	 * The size can be greater than 1 if playing a course. */
	std::vector<Song*> m_apSongsQueue;
	std::vector<float> ratesqueue;
	std::vector<string> playlistscorekeys;

	float m_fTimeSinceLastDancingComment; // this counter is only running while
										  // STATE_DANCING

	LyricDisplay m_LyricDisplay;

	Background* m_pSongBackground;
	Foreground* m_pSongForeground;

	/** @brief Used between songs in a course to show the next song. */
	Transition m_NextSong;

	BitmapText m_textSongOptions;
	BitmapText m_Scoreboard[NUM_NSScoreBoardColumn]; // for NSMAN, so we can
													 // have a scoreboard

	bool m_bShowScoreboard;

	BitmapText m_textDebug;

	RageTimer m_GiveUpTimer;
	bool m_gave_up;
	RageTimer m_SkipSongTimer;
	bool m_skipped_song;
	void AbortGiveUpText(bool show_abort_text);
	void AbortSkipSong(bool show_text);
	void AbortGiveUp(bool bShowText);
	void ResetGiveUpTimers(bool show_text);

	Transition m_Ready;
	Transition m_Go;
	/** @brief The transition to use when all players have failed. */
	Transition m_Failed;
	/** @brief The transition to use when one player earns an easter egg. */
	Transition m_Toasty;

	AutoKeysounds m_AutoKeysounds;

	RageSound m_soundBattleTrickLevel1;
	RageSound m_soundBattleTrickLevel2;
	RageSound m_soundBattleTrickLevel3;

	bool m_bZeroDeltaOnNextUpdate;

	GameplayAssist m_GameplayAssist;
	RageSound* m_pSoundMusic;


	std::vector<PlayerInfo>
	  m_vPlayerInfo; // filled by SGameplay derivatives in FillPlayerInfo
	virtual void FillPlayerInfo(std::vector<PlayerInfo>& vPlayerInfoOut) = 0;
	virtual PlayerInfo& GetPlayerInfoForInput(const InputEventPlus& iep)
	{
		return m_vPlayerInfo[iep.pn];
	}

	RageTimer m_timerGameplaySeconds;

	// m_delaying_ready_announce is for handling a case where the ready
	// announcer sound needs to be delayed.  See HandleScreenMessage for more.
	// -Kyz
	bool m_delaying_ready_announce;

	// True when the player hit RestartGameplay
	bool m_bRestarted = false;
};

std::vector<PlayerInfo>::iterator
GetNextEnabledPlayerInfo(std::vector<PlayerInfo>::iterator iter,
						 std::vector<PlayerInfo>& v);
std::vector<PlayerInfo>::iterator
GetNextEnabledPlayerInfoNotDummy(std::vector<PlayerInfo>::iterator iter,
								 std::vector<PlayerInfo>& v);
std::vector<PlayerInfo>::iterator
GetNextEnabledPlayerNumberInfo(std::vector<PlayerInfo>::iterator iter,
							   std::vector<PlayerInfo>& v);
std::vector<PlayerInfo>::iterator
GetNextPlayerNumberInfo(std::vector<PlayerInfo>::iterator iter,
						std::vector<PlayerInfo>& v);
std::vector<PlayerInfo>::iterator
GetNextVisiblePlayerInfo(std::vector<PlayerInfo>::iterator iter,
						 std::vector<PlayerInfo>& v);

/** @brief Get each enabled Player's info. */
#define FOREACH_EnabledPlayerInfo(v, pi)                                       \
	for (std::vector<PlayerInfo>::iterator pi =                                     \
		   GetNextEnabledPlayerInfo(v.begin(), v);                             \
		 pi != v.end();                                                        \
		 pi = GetNextEnabledPlayerInfo(++pi, v))
/** @brief Get each enabled Player's info as long as it's not a dummy player. */
#define FOREACH_EnabledPlayerInfoNotDummy(v, pi)                               \
	for (std::vector<PlayerInfo>::iterator pi =                                     \
		   GetNextEnabledPlayerInfoNotDummy(v.begin(), v);                     \
		 pi != v.end();                                                        \
		 pi = GetNextEnabledPlayerInfoNotDummy(++pi, v))
/** @brief Get each enabled Player Number's info. */
#define FOREACH_EnabledPlayerNumberInfo(v, pi)                                 \
	for (std::vector<PlayerInfo>::iterator pi =                                     \
		   GetNextEnabledPlayerNumberInfo(v.begin(), v);                       \
		 pi != v.end();                                                        \
		 pi = GetNextEnabledPlayerNumberInfo(++pi, v))
/** @brief Get each Player Number's info, regardless of whether it's enabled or
 * not. */
#define FOREACH_PlayerNumberInfo(v, pi)                                        \
	for (std::vector<PlayerInfo>::iterator pi =                                     \
		   GetNextPlayerNumberInfo(v.begin(), v);                              \
		 pi != v.end();                                                        \
		 pi = GetNextPlayerNumberInfo(++pi, v))
/** @brief Get each visible Player's info. */
#define FOREACH_VisiblePlayerInfo(v, pi)                                       \
	for (std::vector<PlayerInfo>::iterator pi =                                     \
		   GetNextVisiblePlayerInfo(v.begin(), v);                             \
		 pi != v.end();                                                        \
		 pi = GetNextVisiblePlayerInfo(++pi, v))

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
