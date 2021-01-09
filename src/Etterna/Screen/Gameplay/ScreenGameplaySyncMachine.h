#ifndef ScreenGameplaySyncMachine_H
#define ScreenGameplaySyncMachine_H

#include "ScreenGameplayNormal.h"
#include "Etterna/Models/Songs/Song.h"
/** @brief A gameplay screen used for syncing the machine's timing. */
class ScreenGameplaySyncMachine : public ScreenGameplayNormal
{
  public:
	void Init() override;

	void Update(float fDelta) override;
	bool Input(const InputEventPlus& input) override;

	ScreenType GetScreenType() const override { return system_menu; }

	void HandleScreenMessage(const ScreenMessage& SM) override;
	void ResetAndRestartCurrentSong();
	void RestartGameplay() override;

  protected:
	bool UseSongBackgroundAndForeground() const override { return false; }
	void RefreshText();

	/** @brief the Song used for this screen. */
	Song m_Song;
	/** @brief the Steps used for this screen. */
	const Steps* m_pSteps;

	BitmapText m_textSyncInfo;
};

#endif
