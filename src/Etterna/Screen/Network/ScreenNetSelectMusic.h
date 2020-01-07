/* ScreenNetSelectMusic - A method for Online/Net song selection */

#ifndef SCREEN_NET_SELECT_MUSIC_H
#define SCREEN_NET_SELECT_MUSIC_H

#include "Etterna/Actor/Menus/BPMDisplay.h"
#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Actor/Menus/ModIconRow.h"
#include "Etterna/Actor/Menus/MusicWheel.h"
#include "Etterna/Screen/Others/ScreenSelectMusic.h"
#include "Etterna/Screen/Others/ScreenWithMenuElements.h"
#include "Etterna/Actor/Base/Sprite.h"
#include "Etterna/Actor/GameplayAndMenus/StepsDisplay.h"

class ScreenNetSelectMusic : public ScreenSelectMusic
{
  public:
	~ScreenNetSelectMusic() override;
	void Init() override;
	void BeginScreen() override;

	void DifferentialReload();

	bool Input(const InputEventPlus& input) override;
	void HandleScreenMessage(ScreenMessage SM) override;

	void StartSelectedSong();
	bool SelectCurrent();

	void OpenOptions() override;

	MusicWheel* GetMusicWheel();
	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	bool MenuStart(const InputEventPlus& input) override;
	bool MenuBack(const InputEventPlus& input) override;
	bool MenuLeft(const InputEventPlus& input) override;
	bool MenuRight(const InputEventPlus& input) override;

	void Update(float fDeltaTime) override;

	void TweenOffScreen() override;

  private:
	RageSound m_soundChangeOpt;
	RageSound m_soundChangeSel;

	bool m_bInitialSelect = false;
	bool m_bAllowInput = false;
};

#endif
