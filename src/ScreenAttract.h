/* ScreenAttract - Base class for all attraction screens. This class handles input and coin logic. */

#ifndef ScreenAttract_H
#define ScreenAttract_H

#include "ScreenWithMenuElements.h"

AutoScreenMessage( SM_GoToStartScreen );

class ScreenAttract : public ScreenWithMenuElements
{
public:
	void Init() override;
	void BeginScreen() override;

	static bool AttractInput( const InputEventPlus &input, ScreenWithMenuElements *pScreen );
	static void GoToStartScreen( RString sScreenName );
	static void SetAttractVolume( bool bInAttract );

	bool Input( const InputEventPlus &input ) override;
	void HandleScreenMessage( const ScreenMessage SM ) override;
	void Cancel( ScreenMessage smSendWhenDone ) override;

	ScreenType GetScreenType() const override { return attract; }

	// Lua
	void PushSelf( lua_State *L ) override;

protected:
	void StartPlayingMusic() override;
	ThemeMetric<bool> RESET_GAME_STATE;
	ThemeMetric<bool> ATTRACT_VOLUME;
};

#endif

/*
 * (c) 2003-2004 Chris Danford
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
