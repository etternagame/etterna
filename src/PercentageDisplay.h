#ifndef PERCENTAGE_DISPLAY_H
#define PERCENTAGE_DISPLAY_H

#include "ActorFrame.h"
#include "PlayerNumber.h"
#include "BitmapText.h"
#include "StageStats.h"
#include "ThemeMetric.h"

class PlayerState;

/** @brief An Actor that displays a percentage. */
class PercentageDisplay: public ActorFrame
{
public:
	PercentageDisplay();
	void Load( const PlayerState *pPlayerState, const PlayerStageStats *pPlayerStageStats );
	void Load( const PlayerState *pPlayerState, const PlayerStageStats *pPlayerStageStats, const RString &sMetricsGroup, bool bAutoRefresh );
	void Update( float fDeltaTime ) override;
	void LoadFromNode( const XNode* pNode ) override;
	PercentageDisplay *Copy() const override;

	// Lua
	void PushSelf( lua_State *L ) override;

private:
	int m_iDancePointsDigits;
	bool m_bUseRemainder;

	void Refresh();
	const PlayerState *m_pPlayerState;
	const PlayerStageStats *m_pPlayerStageStats;
	bool m_bAutoRefresh;
	int m_Last;
	int m_LastMax;
	BitmapText	m_textPercent;
	BitmapText	m_textPercentRemainder;
	RString m_sPercentFormat;
	RString m_sRemainderFormat;

	LuaReference m_FormatPercentScore;
};

#endif

/*
 * (c) 2001-2003 Chris Danford
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
