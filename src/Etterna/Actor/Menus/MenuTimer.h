/* MenuTimer - A timer on the menu that ticks down. */

#ifndef MENU_TIMER_H
#define MENU_TIMER_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "RageUtil/Sound/RageSound.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

std::string
WARNING_COMMAND_NAME(size_t i);

class MenuTimer : public ActorFrame
{
  public:
	MenuTimer();
	~MenuTimer() override;
	void Load(const std::string& sMetricsGroup);

	void Update(float fDeltaTime) override;

	void SetSeconds(float fSeconds);
	float GetSeconds() const { return m_fSecondsLeft; }
	void Start();	// resume countdown from paused
	void Pause();	// don't count down
	void Stop();	// set to "00" and pause
	void Disable(); // set to "99" and pause
	void Stall();	// pause countdown for a sec
	void EnableSilent(bool bSilent)
	{
		m_bSilent = bSilent;
	}								   // make timer silent
	void EnableStealth(bool bStealth); // make timer invisible and silent

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	float m_fSecondsLeft = 0.f;
	float m_fStallSeconds, m_fStallSecondsLeft;
	bool m_bPaused;

	void SetText(float fSeconds);

#define NUM_MENU_TIMER_TEXTS 2

	bool m_bSilent;

	BitmapText m_text[NUM_MENU_TIMER_TEXTS];

	LuaReference m_exprFormatText[NUM_MENU_TIMER_TEXTS];

	AutoActor m_sprFrame;

	RageSound m_soundBeep;

	ThemeMetric<int> WARNING_START;
	ThemeMetric<int> WARNING_BEEP_START;
	ThemeMetric<float> MAX_STALL_SECONDS;
	ThemeMetric<float> HURRY_UP_TRANSITION;
	ThemeMetric1D<apActorCommands>* WARNING_COMMAND;
};

#endif
