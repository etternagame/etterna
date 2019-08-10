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
#include "Etterna/Models/Misc/PlayerInfo.h"

class LifeMeter;
class Background;
class Foreground;

AutoScreenMessage(SM_NotesEnded);
AutoScreenMessage(SM_BeginFailed);
AutoScreenMessage(SM_LeaveGameplay);

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

	void FailFadeRemovePlayer(PlayerInfo* pi);
	void FailFadeRemovePlayer(PlayerNumber pn);
	void BeginBackingOutFromGameplay();

	// Get current position of the song during gameplay
	const float GetSongPosition();

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
	void SongFinished();
	virtual void SaveStats();
	virtual void StageFinished(bool bBackedOut);
	void SaveReplay();
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
	vector<Song*> m_apSongsQueue;
	vector<float> ratesqueue;
	vector<string> playlistscorekeys;

	float m_fTimeSinceLastDancingComment; // this counter is only running while
										  // STATE_DANCING

	LyricDisplay m_LyricDisplay;

	Background* m_pSongBackground;
	Foreground* m_pSongForeground;

	/** @brief Used between songs in a course to show the next song. */
	Transition m_NextSong;

	BitmapText m_Scoreboard[NUM_NSScoreBoardColumn]; // for NSMAN, so we can
													 // have a scoreboard

	bool m_bShowScoreboard;

	BitmapText m_textDebug;

	RageTimer m_GiveUpTimer;
	bool m_gave_up;
	void AbortGiveUpText(bool show_abort_text);
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

	PlayerInfo
	  m_vPlayerInfo; // filled by SGameplay derivatives in FillPlayerInfo
	virtual void FillPlayerInfo(PlayerInfo* vPlayerInfoOut) = 0;
	virtual PlayerInfo& GetPlayerInfoForInput(const InputEventPlus& iep)
	{
		return m_vPlayerInfo;
	}

	RageTimer m_timerGameplaySeconds;

	// m_delaying_ready_announce is for handling a case where the ready
	// announcer sound needs to be delayed.  See HandleScreenMessage for more.
	// -Kyz
	bool m_delaying_ready_announce;

	// True when the player hit RestartGameplay
	bool m_bRestarted = false;
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
