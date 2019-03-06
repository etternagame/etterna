#include "Etterna/Globals/global.h"
#include "DualScrollBar.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ThemeManager.h"

DualScrollBar::DualScrollBar()
{
	m_fBarHeight = 100;
	m_fBarTime = 1;
}

void
DualScrollBar::Load(const RString& sType)
{
	m_sprScrollThumbUnderHalf.Load(
		THEME->GetPathG(sType, ssprintf("thumb p%i", PLAYER_1 + 1)));
	m_sprScrollThumbUnderHalf->SetName(ssprintf("ThumbP%i", PLAYER_1 + 1));
	this->AddChild(m_sprScrollThumbUnderHalf);

	m_sprScrollThumbOverHalf.Load(
		THEME->GetPathG(sType, ssprintf("thumb p%i", PLAYER_1 + 1)));
	m_sprScrollThumbOverHalf->SetName(ssprintf("ThumbP%i", PLAYER_1 + 1));
	this->AddChild(m_sprScrollThumbOverHalf);

	m_sprScrollThumbUnderHalf->SetCropLeft(.5f);
	m_sprScrollThumbUnderHalf->SetCropRight(.5f);

	m_sprScrollThumbOverHalf->SetCropRight(.5f);
	m_sprScrollThumbOverHalf->SetCropLeft(.5f);

	SetPercentage(PLAYER_1, 0);

	FinishTweening();
}

void
DualScrollBar::EnablePlayer(PlayerNumber pn, bool on)
{
	m_sprScrollThumbUnderHalf->SetVisible(on);
	m_sprScrollThumbOverHalf->SetVisible(on);
}

void
DualScrollBar::SetPercentage(PlayerNumber pn, float fPercent)
{
	const float bottom =
	  m_fBarHeight / 2 - m_sprScrollThumbUnderHalf->GetZoomedHeight() / 2;
	const float top = -bottom;

	/* Position both thumbs. */
	m_sprScrollThumbUnderHalf->StopTweening();
	m_sprScrollThumbUnderHalf->BeginTweening(m_fBarTime);
	m_sprScrollThumbUnderHalf->SetY(SCALE(fPercent, 0, 1, top, bottom));

	m_sprScrollThumbOverHalf->StopTweening();
	m_sprScrollThumbOverHalf->BeginTweening(m_fBarTime);
	m_sprScrollThumbOverHalf->SetY(SCALE(fPercent, 0, 1, top, bottom));
}

/*
 * (c) 2001-2004 Glenn Maynard, Chris Danford
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
