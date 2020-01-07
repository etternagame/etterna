#include "Etterna/Globals/global.h"
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
}

void
ReceptorArrow::Load(const PlayerState* pPlayerState,
					int iColNo,
					std::string Type)
{
	m_pPlayerState = pPlayerState;
	m_iColNo = iColNo;

	const PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
	std::vector<GameInput> GameI;
	GAMESTATE->GetCurrentStyle(pn)->StyleInputToGameInput(iColNo, pn, GameI);
	NOTESKIN->SetPlayerNumber(pn);
	// FIXME?  Does this cause a problem when game inputs on different
	// controllers are mapped to the same column?  Such a thing could be set
	// up in a style that uses two controllers and has a mapping that fits the
	// requirements. -Kyz
	NOTESKIN->SetGameController(GameI[0].controller);

	std::string sButton =
	  GAMESTATE->GetCurrentStyle(pn)->ColToButtonName(iColNo);
	m_pReceptor.Load(NOTESKIN->LoadActor(sButton, Type));
	this->AddChild(m_pReceptor);

	bool bReverse =
	  m_pPlayerState->m_PlayerOptions.GetCurrent().GetReversePercentForColumn(
		m_iColNo) > 0.5f;
	m_pReceptor->PlayCommand(bReverse ? "ReverseOn" : "ReverseOff");
	m_bWasReverse = bReverse;
}

void
ReceptorArrow::Update(float fDeltaTime)
{
	ActorFrame::Update(fDeltaTime);

	bool bReverse =
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

	std::string sJudge = TapNoteScoreToString(score);
	m_pReceptor->PlayCommand(Capitalize(sJudge));
	Message msg("ReceptorJudgment");
	msg.SetParam("TapNoteScore", score);
	msg.SetParam("Color", NOTESKIN->GetLastSeenColor());
	m_pReceptor->HandleMessage(msg);
}

void
ReceptorArrow::SetNoteUpcoming(int iCol, int iRow, bool b)
{
	m_pReceptor->PlayCommand(b ? "ShowNoteUpcoming" : "HideNoteUpcoming");
	Message msg("ReceptorUpcoming");
	msg.SetParam("Column", iCol);
	msg.SetParam("Row", iRow);
	m_pReceptor->HandleMessage(msg);
}
