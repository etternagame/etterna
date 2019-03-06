#ifndef ScreenWithMenuElements_H
#define ScreenWithMenuElements_H

#include "Etterna/Actor/Base/ActorUtil.h"
#include "Screen.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/Actor/GameplayAndMenus/Transition.h"

class MenuTimer;

class ScreenWithMenuElements : public Screen
{
  public:
	ScreenWithMenuElements();
	void Init() override;
	void BeginScreen() override;
	~ScreenWithMenuElements() override;

	void HandleScreenMessage(ScreenMessage SM) override;
	void Update(float fDeltaTime) override;
	void UpdateTimedFunctions(float fDeltaTime) override;
	void StartTransitioningScreen(ScreenMessage smSendWhenDone);
	virtual void Cancel(ScreenMessage smSendWhenDone);
	bool IsTransitioning();
	bool AllowCallbackInput() override { return !IsTransitioning(); }

	void StopTimer();
	void ResetTimer();

	// Sub-classes can hook these and do special actions that won't be triggered
	// automatically by an "On"/"Off" command
	virtual void TweenOnScreen();
	virtual void TweenOffScreen();

	// Lua
	void PushSelf(lua_State* L) override;

	bool AllowLateJoin() const override { return m_bShouldAllowLateJoin; }
	bool m_bShouldAllowLateJoin; // So that it can be exposed to Lua.

  protected:
	RString HandleLuaMusicFile(RString const& path);
	virtual void StartPlayingMusic();
	void SetHelpText(const RString& s);

	AutoActor m_sprUnderlay;
	MenuTimer* m_MenuTimer;
	AutoActor m_sprOverlay;
	vector<Actor*> m_vDecorations;

	Transition m_In;
	Transition m_Out;
	Transition m_Cancel;

	ThemeMetric<bool> PLAY_MUSIC;
	ThemeMetric<bool> MUSIC_ALIGN_BEAT;
	ThemeMetric<float> DELAY_MUSIC_SECONDS;
	ThemeMetric<bool> CANCEL_TRANSITIONS_OUT;
	ThemeMetric<float> TIMER_SECONDS;
	ThemeMetric<RString> TIMER_METRICS_GROUP;
	ThemeMetric<bool> RESET_GAMESTATE;

  private:
	RString m_sPathToMusic;
};

class ScreenWithMenuElementsSimple : public ScreenWithMenuElements
{
  public:
	bool MenuStart(const InputEventPlus& input) override;
	bool MenuBack(const InputEventPlus& input) override;

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
};

class ScreenWithMenuElementsBasic : public ScreenWithMenuElements
{
};

#endif

/*
 * (c) 2004 Chris Danford
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
