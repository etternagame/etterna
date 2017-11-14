#ifndef ScoreDisplayLifeTime_H
#define ScoreDisplayLifeTime_H

#include "ScoreDisplay.h"
#include "BitmapText.h"
#include "AutoActor.h"

/**
 * @brief Display a updating count of life time remaining.
 * 
 * This is mainly used for Survival mode. */
class ScoreDisplayLifeTime : public ScoreDisplay
{
public:
	ScoreDisplayLifeTime();

	void Init( const PlayerState* pPlayerState, const PlayerStageStats* pPlayerStageStats ) override;

	void Update( float fDelta ) override;

	void OnLoadSong() override;
	void OnJudgment( TapNoteScore score ) override;
	void OnJudgment( HoldNoteScore score, TapNoteScore tscore ) override;

protected:
	AutoActor	m_sprFrame;
	BitmapText	m_textTimeRemaining;
	BitmapText	m_textDeltaSeconds;
};

#endif

/*
 * (c) 2001-2004 Chris Danford
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
