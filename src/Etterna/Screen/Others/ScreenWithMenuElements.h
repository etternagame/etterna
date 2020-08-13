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

	void HandleScreenMessage(const ScreenMessage& SM) override;
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
	std::string HandleLuaMusicFile(std::string const& path);
	virtual void StartPlayingMusic();
	void SetHelpText(const std::string& s);

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
	ThemeMetric<std::string> TIMER_METRICS_GROUP;
	ThemeMetric<bool> RESET_GAMESTATE;

  private:
	std::string m_sPathToMusic;
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
