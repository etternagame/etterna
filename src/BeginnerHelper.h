#ifndef BEGINNER_HELPER_H
#define BEGINNER_HELPER_H

#include "ActorFrame.h"
#include "Character.h"
#include "Sprite.h"
#include "AutoActor.h"
#include "PlayerNumber.h"
#include "NoteData.h"
#include "ThemeMetric.h"
class Model;
/** @brief A dancing character that follows the steps of the Song. */
class BeginnerHelper : public ActorFrame
{
public:
	BeginnerHelper();
	~BeginnerHelper() override;

	bool Init( int iDancePadType );
	bool IsInitialized() { return m_bInitialized; }
	static bool CanUse(PlayerNumber pn);
	void AddPlayer( PlayerNumber pn, const NoteData &nd );
	void ShowStepCircle( PlayerNumber pn, int CSTEP );
	bool m_bShowBackground;

	void Update( float fDeltaTime ) override;
	void DrawPrimitives() override;

protected:
	void Step( PlayerNumber pn, int CSTEP );

	NoteData m_NoteData[NUM_PLAYERS];
	bool m_bPlayerEnabled[NUM_PLAYERS];
	Model *m_pDancer[NUM_PLAYERS];
	Model *m_pDancePad;
	Sprite	m_sFlash;
	AutoActor	m_sBackground;
	Sprite	m_sStepCircle[NUM_PLAYERS][4];	// More memory, but much easier to manage

	int	m_iLastRowChecked;
	int	m_iLastRowFlashed;
	bool m_bInitialized;

	ThemeMetric<bool> SHOW_DANCE_PAD;
};
#endif

/**
 * @file
 * @author Kevin Slaughter, Thad Ward (c) 2003
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
