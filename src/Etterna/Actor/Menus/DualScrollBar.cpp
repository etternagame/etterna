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
DualScrollBar::Load(const std::string& sType)
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
