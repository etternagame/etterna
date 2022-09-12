#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "RageUtil/Utils/RageUtil.h"
#include "ReceptorArrow.h"
#include "Etterna/Models/StepsAndStyles/Style.h"

ReceptorArrow::ReceptorArrow()
{
	m_bIsPressed = false;
	m_bWasPressed = false;
	m_bWasReverse = false;
	m_pPlayerState = nullptr;
	m_iColNo = 0;
}

void
ReceptorArrow::Load(const PlayerState* pPlayerState,
					int iColNo,
					const std::string& Type)
{
	m_pPlayerState = pPlayerState;
	m_iColNo = iColNo;

	const auto pn = m_pPlayerState->m_PlayerNumber;
	std::vector<GameInput> GameI;
	GAMESTATE->GetCurrentStyle(pn)->StyleInputToGameInput(iColNo, GameI);
	NOTESKIN->SetPlayerNumber(pn);
	// FIXME?  Does this cause a problem when game inputs on different
	// controllers are mapped to the same column?  Such a thing could be set
	// up in a style that uses two controllers and has a mapping that fits the
	// requirements. -Kyz
	NOTESKIN->SetGameController(GameI[0].controller);

	const auto sButton =
	  GAMESTATE->GetCurrentStyle(pn)->ColToButtonName(iColNo);
	m_pReceptor.Load(NOTESKIN->LoadActor(sButton, Type));
	this->AddChild(m_pReceptor);

	const auto bReverse =
	  m_pPlayerState->m_PlayerOptions.GetCurrent().GetReversePercentForColumn(
		m_iColNo) > 0.5f;
	m_pReceptor->PlayCommand(bReverse ? "ReverseOn" : "ReverseOff");
	m_bWasReverse = bReverse;
}

void
ReceptorArrow::Update(float fDeltaTime)
{
	ActorFrame::Update(fDeltaTime);

	const auto bReverse =
	  m_pPlayerState->m_PlayerOptions.GetCurrent().GetReversePercentForColumn(
		m_iColNo) > 0.5f;
	if (bReverse != m_bWasReverse) {
		m_pReceptor->PlayCommand(bReverse ? "ReverseOn" : "ReverseOff");
		m_bWasReverse = bReverse;
	}
}

void
ReceptorArrow::DrawPrimitives()
{
	if (m_bWasPressed && !m_bIsPressed)
		m_pReceptor->PlayCommand("Lift");
	else if (!m_bWasPressed && m_bIsPressed)
		m_pReceptor->PlayCommand("Press");

	m_bWasPressed = m_bIsPressed;
	m_bIsPressed = false; // it may get turned back on next update

	ActorFrame::DrawPrimitives();
}

void
ReceptorArrow::Step(TapNoteScore score)
{
	m_bIsPressed = true;
	m_pReceptor->PlayCommand(Capitalize(TapNoteScoreToString(score)));
	Message msg("ReceptorJudgment");
	msg.SetParam("TapNoteScore", score);
	msg.SetParam("Color", NOTESKIN->GetLastSeenColor());
	m_pReceptor->HandleMessage(msg);
}
