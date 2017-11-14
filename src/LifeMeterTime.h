#ifndef LifeMeterTime_H
#define LifeMeterTime_H

#include "LifeMeter.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "RageSound.h"
#include "PercentageDisplay.h"
#include "AutoActor.h"
#include "MeterDisplay.h"
#include "Quad.h"
class StreamDisplay;
/** @brief Battery life meter used in Survival. */
class LifeMeterTime : public LifeMeter
{
public:
	LifeMeterTime();

	~LifeMeterTime() override;

	void Load( const PlayerState *pPlayerState, PlayerStageStats *pPlayerStageStats ) override;

	void Update( float fDeltaTime ) override;

	void OnLoadSong() override;
	void ChangeLife( TapNoteScore score ) override;
	void ChangeLife( HoldNoteScore score, TapNoteScore tscore ) override;
	void ChangeLife(float delta) override;
	void SetLife(float value) override;
	void HandleTapScoreNone() override;
	bool IsInDanger() const override;
	bool IsHot() const override;
	bool IsFailing() const override;
	float GetLife() const override;

private:
	float GetLifeSeconds() const;
	void SendLifeChangedMessage( float fOldLife, TapNoteScore tns, HoldNoteScore hns );

	AutoActor		m_sprBackground;
	Quad			m_quadDangerGlow;
	StreamDisplay*	m_pStream;
	AutoActor		m_sprFrame;

	float		m_fLifeTotalGainedSeconds;
	float		m_fLifeTotalLostSeconds;
	/** @brief The sound played when time is gained at the start of each Song. */
	RageSound	m_soundGainLife;
};


#endif

/**
 * @file
 * @author Chris Danford (c) 2001-2004
 * @section LICENSE
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
