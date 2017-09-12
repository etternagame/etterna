/* ScreenNetSelectMusic - A method for Online/Net song selection */

#ifndef SCREEN_NET_SELECT_MUSIC_H
#define SCREEN_NET_SELECT_MUSIC_H

#include "BPMDisplay.h"
#include "Difficulty.h"
#include "DifficultyIcon.h"
#include "ModIconRow.h"
#include "MusicWheel.h"
#include "ScreenNetSelectBase.h"
#include "ScreenWithMenuElements.h"
#include "Sprite.h"
#include "StepsDisplay.h"

class ScreenNetSelectMusic : public ScreenNetSelectBase
{
public:
	void Init() override;

	bool Input( const InputEventPlus &input ) override;
	void HandleScreenMessage( ScreenMessage SM ) override;

	void StartSelectedSong();
	bool SelectCurrent();

	MusicWheel* GetMusicWheel();
	// Lua
	void PushSelf(lua_State *L) override;
	
protected:
	bool MenuStart( const InputEventPlus &input ) override;
	bool MenuBack( const InputEventPlus &input ) override;
	bool MenuLeft( const InputEventPlus &input ) override;
	bool MenuRight( const InputEventPlus &input ) override;
	bool MenuUp( const InputEventPlus &input ) override;
	bool MenuDown( const InputEventPlus &input ) override;
	bool LeftAndRightPressed( PlayerNumber pn );

	void Update( float fDeltaTime ) override;

	void MusicChanged();

	void TweenOffScreen() override;

	ThemeMetric<SampleMusicPreviewMode> SAMPLE_MUSIC_PREVIEW_MODE;
	RString m_sSectionMusicPath;
	RString m_sRouletteMusicPath;
	RString m_sRandomMusicPath;

	ThemeMetric<RString>	MUSIC_WHEEL_TYPE;
	ThemeMetric<RString>	PLAYER_OPTIONS_SCREEN;
	
	ThemeMetric<float>		SAMPLE_MUSIC_FALLBACK_FADE_IN_SECONDS;
	ThemeMetric<float>		SAMPLE_MUSIC_FADE_OUT_SECONDS;
	ThemeMetric<bool>		ALIGN_MUSIC_BEATS;

private:
	MusicWheel m_MusicWheel;

	StepsDisplay m_StepsDisplays[NUM_PLAYERS];
	Difficulty m_DC[NUM_PLAYERS];

	void UpdateDifficulties( PlayerNumber pn );

	RageSound m_soundChangeOpt;
	RageSound m_soundChangeSel;

	// todo: do this theme-side instead. -aj
	ModIconRow m_ModIconRow[NUM_PLAYERS];

	Song * m_cSong;

	bool m_bInitialSelect;
	bool m_bAllowInput;
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
