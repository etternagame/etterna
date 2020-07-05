#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "OptionsCursor.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ThemeManager.h"

OptionsCursor::OptionsCursor()
{
	m_iOriginalLeftX = 0;
	m_iOriginalRightX = 0;
	m_iOriginalCanGoLeftX = 0;
	m_iOriginalCanGoRightX = 0;
}

OptionsCursor::OptionsCursor(const OptionsCursor& cpy)
  : ActorFrame(cpy)
  , m_sprMiddle(cpy.m_sprMiddle)
  , m_sprLeft(cpy.m_sprLeft)
  , m_sprRight(cpy.m_sprRight)
  , m_sprCanGoLeft(cpy.m_sprCanGoLeft)
  , m_sprCanGoRight(cpy.m_sprCanGoRight)
  , m_iOriginalLeftX(cpy.m_iOriginalLeftX)
  , m_iOriginalRightX(cpy.m_iOriginalRightX)
  , m_iOriginalCanGoLeftX(cpy.m_iOriginalCanGoLeftX)
  , m_iOriginalCanGoRightX(cpy.m_iOriginalCanGoRightX)
{
	// Re-add children, or m_SubActors will point to cpy's children and not our
	// own.
	m_SubActors.clear();
	this->AddChild(m_sprMiddle);
	this->AddChild(m_sprLeft);
	this->AddChild(m_sprRight);
	if (m_sprCanGoLeft)
		this->AddChild(m_sprCanGoLeft);
	if (m_sprCanGoRight)
		this->AddChild(m_sprCanGoRight);
}

void
OptionsCursor::Load(const std::string& sMetricsGroup, bool bLoadCanGos)
{
#define LOAD_SPR(spr, name)                                                    \
	spr.Load(THEME->GetPathG(sMetricsGroup, name));                            \
	(spr)->SetName(name);                                                      \
	ActorUtil::LoadAllCommandsAndSetXYAndOnCommand(spr, sMetricsGroup);        \
	this->AddChild(spr);

	LOAD_SPR(m_sprMiddle, "Middle");
	LOAD_SPR(m_sprLeft, "Left");
	LOAD_SPR(m_sprRight, "Right");
	if (bLoadCanGos) {
		LOAD_SPR(m_sprCanGoLeft, "CanGoLeft");
		LOAD_SPR(m_sprCanGoRight, "CanGoRight");
	}
#undef LOAD_SPR

	m_iOriginalLeftX = static_cast<int>(m_sprLeft->GetX());
	m_iOriginalRightX = static_cast<int>(m_sprRight->GetX());
	if (bLoadCanGos) {
		m_iOriginalCanGoLeftX = static_cast<int>(m_sprCanGoLeft->GetX());
		m_iOriginalCanGoRightX = static_cast<int>(m_sprCanGoRight->GetX());
	}

	SetCanGo(false, false);
}

void
OptionsCursor::SetCanGo(bool bCanGoLeft, bool bCanGoRight)
{
	if (m_sprCanGoLeft != nullptr) {
		m_sprCanGoLeft->EnableAnimation(bCanGoLeft);
		m_sprCanGoRight->EnableAnimation(bCanGoRight);

		m_sprCanGoLeft->SetDiffuse(bCanGoLeft ? RageColor(1, 1, 1, 1)
											  : RageColor(1, 1, 1, 0));
		m_sprCanGoRight->SetDiffuse(bCanGoRight ? RageColor(1, 1, 1, 1)
												: RageColor(1, 1, 1, 0));
	}
}

void
OptionsCursor::StopTweening()
{
	ActorFrame::StopTweening();

	m_sprMiddle->StopTweening();
	m_sprLeft->StopTweening();
	m_sprRight->StopTweening();

	if (m_sprCanGoLeft != nullptr) {
		m_sprCanGoLeft->StopTweening();
		m_sprCanGoRight->StopTweening();
	}
}

void
OptionsCursor::BeginTweening(float fSecs)
{
	ActorFrame::BeginTweening(fSecs);

	m_sprMiddle->BeginTweening(fSecs);
	m_sprLeft->BeginTweening(fSecs);
	m_sprRight->BeginTweening(fSecs);

	if (m_sprCanGoLeft != nullptr) {
		m_sprCanGoLeft->BeginTweening(fSecs);
		m_sprCanGoRight->BeginTweening(fSecs);
	}
}

void
OptionsCursor::SetBarWidth(int iWidth)
{
	float fWidth =
	  ceilf(iWidth / 2.0f) * 2.0f; // round up to nearest even number

	m_sprMiddle->ZoomToWidth(fWidth);

	m_sprLeft->SetX(m_iOriginalLeftX - fWidth / 2);
	m_sprRight->SetX(m_iOriginalRightX + fWidth / 2);
	if (m_sprCanGoLeft != nullptr) {
		m_sprCanGoLeft->SetX(m_iOriginalCanGoLeftX - fWidth / 2);
		m_sprCanGoRight->SetX(m_iOriginalCanGoRightX + fWidth / 2);
	}
}

int
OptionsCursor::GetBarWidth() const
{
	float fWidth = m_sprMiddle->GetZoomX() * m_sprMiddle->GetUnzoomedWidth();
	return static_cast<int>(fWidth);
}
