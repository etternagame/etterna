/* ScreenNetSelectMusic - A method for Online/Net song selection */

#ifndef SCREEN_NET_SELECT_MUSIC_H
#define SCREEN_NET_SELECT_MUSIC_H

#include "Etterna/Actor/Menus/MusicWheel.h"
#include "Etterna/Screen/Others/ScreenSelectMusic.h"

class ScreenNetSelectMusic : public ScreenSelectMusic
{
  public:
	~ScreenNetSelectMusic() override;
	void Init() override;
	void BeginScreen() override;

	bool Input(const InputEventPlus& input) override;
	void HandleScreenMessage(const ScreenMessage& SM) override;

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
