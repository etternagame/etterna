/* ScreenSelectMusic - Choose a Song and Steps. */

#ifndef SCREEN_SELECT_MUSIC_H
#define SCREEN_SELECT_MUSIC_H

#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/GameInput.h"
#include "Etterna/Actor/Menus/MusicWheel.h"
#include "Etterna/Actor/Menus/OptionsList.h"
#include "ScreenWithMenuElements.h"
#include "Etterna/Actor/Base/Sprite.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/Models/Misc/TimingData.h"
#include "Etterna/Models/HighScore/HighScore.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Actor/Gameplay/NoteField.h"

enum SelectionState
{
	SelectionState_SelectingSong,
	SelectionState_SelectingSteps,
	SelectionState_Finalized,
	NUM_SelectionState,
};
const std::string&
SelectionStateToString(SelectionState ss);

class ScreenSelectMusic : public ScreenWithMenuElements
{
  public:
	~ScreenSelectMusic() override;
	void Init() override;
	void BeginScreen() override;

	void Update(float fDeltaTime) override;
	bool Input(const InputEventPlus& input) override;
	void HandleMessage(const Message& msg) override;
	void HandleScreenMessage(const ScreenMessage& SM) override;
	bool AllowLateJoin() const override { return true; }

	bool MenuStart(const InputEventPlus& input) override;
	bool MenuBack(const InputEventPlus& input) override;
	bool SelectCurrent(PlayerNumber pn,
					   GameplayMode mode = GameplayMode_Normal);

	// ScreenWithMenuElements override: never play music here; we do it ourself.
	void StartPlayingMusic() override {}

	virtual void OpenOptions();
	bool GetGoToOptions() const { return m_bGoToOptions; }
	MusicWheel* GetMusicWheel() { return &m_MusicWheel; }

	void OpenOptionsList(PlayerNumber pn);

	bool can_open_options_list(PlayerNumber pn);

	virtual void DifferentialReload();

	void PlayReplay(HighScore* score);

	int GetSelectionState();

	void SetSampleMusicPosition(float);
	void PauseSampleMusic();
	bool ReloadCurrentSong();
	bool ReloadCurrentPack();
	bool ToggleCurrentFavorite();
	bool ToggleCurrentPermamirror();
	bool GoalFromCurrentChart();
	bool AddCurrentChartToActivePlaylist();
	void PlayCurrentSongSampleMusic(bool bForcePlay,
									bool bForceAccurate = false,
									bool bExtended = false);

	void ChangeSteps(PlayerNumber pn, int dir);
	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	virtual bool GenericTweenOn() const { return true; }
	virtual bool GenericTweenOff() const { return true; }
	void UpdateSelectButton(PlayerNumber pn, bool bBeingPressed);

	void AfterStepsOrTrailChange();
	void SwitchToPreferredDifficulty();
	void AfterMusicChange();

	void CheckBackgroundRequests(bool bForce);
	bool DetectCodes(const InputEventPlus& input);

	std::vector<Steps*> m_vpSteps;
	int m_iSelection;

	RageTimer m_timerIdleComment;
	ThemeMetric<float> IDLE_COMMENT_SECONDS;

	ThemeMetric<std::string> PLAYER_OPTIONS_SCREEN;
	ThemeMetric<float> SAMPLE_MUSIC_DELAY_INIT;
	ThemeMetric<float> SAMPLE_MUSIC_DELAY;
	ThemeMetric<bool> SAMPLE_MUSIC_LOOPS;
	ThemeMetric<SampleMusicPreviewMode> SAMPLE_MUSIC_PREVIEW_MODE;
	ThemeMetric<float> SAMPLE_MUSIC_FALLBACK_FADE_IN_SECONDS;
	ThemeMetric<float> SAMPLE_MUSIC_FADE_OUT_SECONDS;
	ThemeMetric<bool> DO_ROULETTE_ON_MENU_TIMER;
	ThemeMetric<float> ROULETTE_TIMER_SECONDS;
	ThemeMetric<bool> ALIGN_MUSIC_BEATS;
	ThemeMetric<std::string> CODES;
	ThemeMetric<std::string> MUSIC_WHEEL_TYPE;
	ThemeMetric<bool> OPTIONS_MENU_AVAILABLE;
	ThemeMetric<bool> SELECT_MENU_AVAILABLE;
	ThemeMetric<bool> MODE_MENU_AVAILABLE;
	ThemeMetric<bool> USE_OPTIONS_LIST;
	ThemeMetric<float> OPTIONS_LIST_TIMEOUT;
	ThemeMetric<bool> USE_PLAYER_SELECT_MENU;
	ThemeMetric<std::string> SELECT_MENU_NAME;
	ThemeMetric<bool> SELECT_MENU_CHANGES_DIFFICULTY;
	ThemeMetric<bool> WRAP_CHANGE_STEPS;
	ThemeMetric<bool> CHANGE_STEPS_WITH_GAME_BUTTONS;
	ThemeMetric<bool> CHANGE_GROUPS_WITH_GAME_BUTTONS;
	ThemeMetric<std::string> NULL_SCORE_STRING;
	ThemeMetric<bool> PLAY_SOUND_ON_ENTERING_OPTIONS_MENU;

	bool CanChangeSong() const
	{
		return m_SelectionState == SelectionState_SelectingSong;
	}
	SelectionState GetNextSelectionState() const
	{
		switch (m_SelectionState) {
			case SelectionState_SelectingSong:
				return SelectionState_Finalized;
			default:
				return SelectionState_Finalized;
		}
	}

	GameButton m_GameButtonPreviousSong;
	GameButton m_GameButtonNextSong;
	GameButton m_GameButtonPreviousDifficulty;
	GameButton m_GameButtonNextDifficulty;
	GameButton m_GameButtonPreviousGroup;
	GameButton m_GameButtonNextGroup;

	std::string m_sSectionMusicPath;
	std::string m_sSortMusicPath;
	std::string m_sRouletteMusicPath;
	std::string m_sRandomMusicPath;
	std::string m_sCourseMusicPath;
	std::string m_sLoopMusicPath;
	std::string m_sFallbackCDTitlePath;

	MusicWheel m_MusicWheel;
	OptionsList m_OptionsList;

	SelectionState m_SelectionState;
	bool m_bStepsChosen; // only used in SelectionState_SelectingSteps
	bool m_bGoToOptions;
	std::string m_sSampleMusicToPlay;
	TimingData* m_pSampleMusicTimingData;
	float m_fSampleStartSeconds, m_fSampleLengthSeconds;
	bool m_bAllowOptionsMenu, m_bAllowOptionsMenuRepeat;
	bool m_bSelectIsDown;
	bool m_bAcceptSelectRelease;

	RageSound m_soundStart;
	RageSound m_soundDifficultyEasier;
	RageSound m_soundDifficultyHarder;
	RageSound m_soundOptionsChange;
	RageSound m_soundLocked;
};

#endif
