#include "Etterna/Globals/global.h"
#include "RageUtil/Utils/RageUtil.h"
#include "ScrollBar.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Core/Services/Locator.hpp"

ScrollBar::ScrollBar()
{
	const std::string sMetricsGroup = "ScrollBar";

	m_sprMiddle.Load(THEME->GetPathG(sMetricsGroup, "middle"));
	this->AddChild(m_sprMiddle);

	m_sprTop.Load(THEME->GetPathG(sMetricsGroup, "top"));
	m_sprTop->SetVertAlign(VertAlign_Bottom);
	this->AddChild(m_sprTop);

	m_sprBottom.Load(THEME->GetPathG(sMetricsGroup, "bottom"));
	m_sprBottom->SetVertAlign(VertAlign_Top);
	this->AddChild(m_sprBottom);

	m_sprScrollTickThumb.Load(THEME->GetPathG(sMetricsGroup, "TickThumb"));
	this->AddChild(m_sprScrollTickThumb);

	for (auto& i : m_sprScrollStretchThumb) {
		i.Load(THEME->GetPathG(sMetricsGroup, "StretchThumb"));
		this->AddChild(i);
	}

	SetBarHeight(100);
}

void
ScrollBar::SetBarHeight(int iHeight)
{
	m_iBarHeight = iHeight;
	m_sprMiddle->SetZoomY(m_iBarHeight / m_sprMiddle->GetUnzoomedHeight());
	m_sprTop->SetY(-m_iBarHeight / 2.0f);
	m_sprBottom->SetY(+m_iBarHeight / 2.0f);
	m_sprScrollTickThumb->SetY(0);
	for (auto& i : m_sprScrollStretchThumb)
		i->SetY(0);
}

void
ScrollBar::SetPercentage(float fCenterPercent, float fSizePercent)
{
	wrap(fCenterPercent, 1.0f);

	const auto iBarContentHeight =
	  static_cast<int>(m_sprMiddle->GetZoomedHeight());
	ASSERT(iBarContentHeight != 0);

	/* Set tick thumb */
	{
		float fY = SCALE(fCenterPercent,
						 0.0f,
						 1.0f,
						 -iBarContentHeight / 2.0f,
						 iBarContentHeight / 2.0f);
		fY = roundf(fY);
		m_sprScrollTickThumb->SetY(fY);
	}

	/* Set stretch thumb */
	float fStartPercent = fCenterPercent - fSizePercent;
	float fEndPercent = fCenterPercent + fSizePercent;

	// make sure the percent numbers are between 0 and 1
	fStartPercent = fmodf(fStartPercent + 1, 1);
	fEndPercent = fmodf(fEndPercent + 1, 1);

	float fPartTopY[2], fPartBottomY[2];

	if (fStartPercent < fEndPercent) // we only need to one 1 stretch thumb part
	{
		fPartTopY[0] = SCALE(fStartPercent,
							 0.0f,
							 1.0f,
							 -iBarContentHeight / 2.0f,
							 +iBarContentHeight / 2.0f);
		fPartBottomY[0] = SCALE(fEndPercent,
								0.0f,
								1.0f,
								-iBarContentHeight / 2.0f,
								+iBarContentHeight / 2.0f);
		fPartTopY[1] = 0;
		fPartBottomY[1] = 0;
	} else // we need two stretch thumb parts
	{
		fPartTopY[0] = SCALE(0.0f,
							 0.0f,
							 1.0f,
							 -iBarContentHeight / 2.0f,
							 +iBarContentHeight / 2.0f);
		fPartBottomY[0] = SCALE(fEndPercent,
								0.0f,
								1.0f,
								-iBarContentHeight / 2.0f,
								+iBarContentHeight / 2.0f);
		fPartTopY[1] = SCALE(fStartPercent,
							 0.0f,
							 1.0f,
							 -iBarContentHeight / 2.0f,
							 +iBarContentHeight / 2.0f);
		fPartBottomY[1] = SCALE(1.0f,
								0.0f,
								1.0f,
								-iBarContentHeight / 2.0f,
								+iBarContentHeight / 2.0f);
	}

	for (unsigned i = 0; i < ARRAYLEN(m_sprScrollStretchThumb); i++) {
		RectF rect(-m_sprScrollStretchThumb[i]->GetUnzoomedWidth() / 2,
				   fPartTopY[i],
				   +m_sprScrollStretchThumb[i]->GetUnzoomedWidth() / 2,
				   fPartBottomY[i]);
		m_sprScrollStretchThumb[i]->StretchTo(rect);
	}
}
