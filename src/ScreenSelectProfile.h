/* ScreenSelectProfile - Screen that allows to select and load profile to use. */

#ifndef SCREEN_SELECT_PROFILE_H
#define SCREEN_SELECT_PROFILE_H

#include "ScreenWithMenuElements.h"

class ScreenSelectProfile : public ScreenWithMenuElements
{
public:
	void Init() override;
	bool Input( const InputEventPlus &input ) override;
	bool MenuLeft( const InputEventPlus &input ) override;
	bool MenuRight( const InputEventPlus &input ) override;
	bool MenuUp( const InputEventPlus &input ) override;
	bool MenuDown( const InputEventPlus &input ) override;
	void HandleScreenMessage( const ScreenMessage SM ) override;

	GameButton m_TrackingRepeatingInput;

	// Lua
	void PushSelf( lua_State *L ) override;
	bool SetProfileIndex( PlayerNumber pn, int iProfileIndex );
	int GetProfileIndex( PlayerNumber pn ) { return m_iSelectedProfiles[pn]; }
	bool Finish();

protected:
	int m_iSelectedProfiles[NUM_PLAYERS];
};

#endif

/*
 * Copyright (c) 2007 vdl
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
