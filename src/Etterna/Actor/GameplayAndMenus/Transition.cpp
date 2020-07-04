#include "Etterna/Singletons/ScreenManager.h"
#include "Transition.h"

Transition::Transition()
{
	m_State = waiting;
}

void
Transition::Load(const std::string& sBGAniDir)
{
	this->RemoveAllChildren();

	m_sprTransition.Load(sBGAniDir);
	this->AddChild(m_sprTransition);

	m_State = waiting;
}

void
Transition::UpdateInternal(float fDeltaTime)
{
	if (m_State != transitioning)
		return;

	// Check this before running Update, so we draw the last frame of the
	// finished transition before sending m_MessageToSendWhenDone.
	if (m_sprTransition->GetTweenTimeLeft() == 0.F) // over
	{
		m_State = finished;
		SCREENMAN->SendMessageToTopScreen(m_MessageToSendWhenDone);
	}

	ActorFrame::UpdateInternal(fDeltaTime);
}

void
Transition::Reset()
{
	m_State = waiting;
	m_bFirstUpdate = true;

	if (m_sprTransition.IsLoaded())
		m_sprTransition->FinishTweening();
}

bool
Transition::EarlyAbortDraw() const
{
	return m_State == waiting;
}

void
Transition::StartTransitioning(ScreenMessage send_when_done)
{
	if (m_State != waiting)
		return; // ignore

	// If transition isn't loaded don't set state to transitioning.
	// We assume elsewhere that m_sprTransition is loaded.
	if (!m_sprTransition.IsLoaded())
		return;

	m_sprTransition->PlayCommand("StartTransitioning");

	m_MessageToSendWhenDone = send_when_done;
	m_State = transitioning;
}

float
Transition::GetTweenTimeLeft() const
{
	if (m_State != transitioning)
		return 0;

	return m_sprTransition->GetTweenTimeLeft();
}
