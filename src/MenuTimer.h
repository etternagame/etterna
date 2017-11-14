/* MenuTimer - A timer on the menu that ticks down. */

#ifndef MENU_TIMER_H
#define MENU_TIMER_H

#include "ActorFrame.h"
#include "BitmapText.h"
#include "RageSound.h"
#include "ThemeMetric.h"
#include "AutoActor.h"

RString WARNING_COMMAND_NAME( size_t i );

class MenuTimer : public ActorFrame
{
public:
	MenuTimer();
	~MenuTimer() override;
	void Load( const RString &sMetricsGroup );
	
	void Update( float fDeltaTime ) override; 

	void SetSeconds( float fSeconds );
	float GetSeconds() const { return m_fSecondsLeft; }
	void Start();		// resume countdown from paused
	void Pause();		// don't count down
	void Stop();		// set to "00" and pause
	void Disable();	// set to "99" and pause
	void Stall();		// pause countdown for a sec
	void EnableSilent( bool bSilent ) { m_bSilent = bSilent; } // make timer silent
	void EnableStealth( bool bStealth ); // make timer invisible and silent


	// Lua
	void PushSelf( lua_State *L ) override;

protected:
	float m_fSecondsLeft;
	float m_fStallSeconds, m_fStallSecondsLeft;
	bool m_bPaused;

	void SetText( float fSeconds );

#define NUM_MENU_TIMER_TEXTS 2

	bool m_bSilent;

	BitmapText	m_text[NUM_MENU_TIMER_TEXTS];

	LuaReference	m_exprFormatText[NUM_MENU_TIMER_TEXTS];

	AutoActor	m_sprFrame;

	RageSound	m_soundBeep;

	ThemeMetric<int>		WARNING_START;
	ThemeMetric<int>		WARNING_BEEP_START;
	ThemeMetric<float>		MAX_STALL_SECONDS;
	ThemeMetric<float>		HURRY_UP_TRANSITION;
	ThemeMetric1D<apActorCommands>	*WARNING_COMMAND;
};

#endif

/*
 * (c) 2002-2004 Chris Danford
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
