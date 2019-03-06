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

/*
 * (c) 2004-2005 Charles Lohr
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
