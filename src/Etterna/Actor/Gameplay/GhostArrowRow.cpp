#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "GhostArrowRow.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/StepsAndStyles/Style.h"

void
GhostArrowRow::Load(const PlayerState* pPlayerState, float fYReverseOffset)
{
	m_pPlayerState = pPlayerState;
	m_fYReverseOffsetPixels = fYReverseOffset;

	const PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
	const Style* pStyle = GAMESTATE->GetCurrentStyle(pn);
	NOTESKIN->SetPlayerNumber(pn);

	// init arrows
	for (int c = 0; c < pStyle->m_iColsPerPlayer; c++) {
		const std::string& sButton =
		  GAMESTATE->GetCurrentStyle(pn)->ColToButtonName(c);

		std::vector<GameInput> GameI;
		GAMESTATE->GetCurrentStyle(pn)->StyleInputToGameInput(c, GameI);
		NOTESKIN->SetGameController(GameI[0].controller);

		m_bHoldShowing.push_back(TapNoteSubType_Invalid);
		m_bLastHoldShowing.push_back(TapNoteSubType_Invalid);

		m_Ghost.push_back(NOTESKIN->LoadActor(sButton, "Explosion", this));
		m_Ghost[c]->SetName("GhostArrow");
	}
}

void
GhostArrowRow::SetColumnRenderers(std::vector<NoteColumnRenderer>& renderers)
{
	ASSERT_M(renderers.size() == m_Ghost.size(),
			 "Notefield has different number of columns than ghost row.");
	for (size_t c = 0; c < m_Ghost.size(); ++c) {
		m_Ghost[c]->SetFakeParent(&(renderers[c]));
	}
	m_renderers = &renderers;
}

GhostArrowRow::~GhostArrowRow()
{
	for (auto& i : m_Ghost)
		delete i;
}

void
GhostArrowRow::Update(float fDeltaTime)
{
	for (unsigned c = 0; c < m_Ghost.size(); c++) {
		m_Ghost[c]->Update(fDeltaTime);
		(*m_renderers)[c].UpdateReceptorGhostStuff(m_Ghost[c]);
	}

	for (unsigned i = 0; i < m_bHoldShowing.size(); ++i) {
		if (m_bLastHoldShowing[i] != m_bHoldShowing[i]) {
			if (m_bLastHoldShowing[i] == TapNoteSubType_Hold)
				m_Ghost[i]->PlayCommand("HoldingOff");
			else if (m_bLastHoldShowing[i] == TapNoteSubType_Roll)
				m_Ghost[i]->PlayCommand("RollOff");
			/*
			else if( m_bLastHoldShowing[i] == TapNoteSubType_Mine )
				m_Ghost[i]->PlayCommand( "MinefieldOff" );
			*/

			if (m_bHoldShowing[i] == TapNoteSubType_Hold)
				m_Ghost[i]->PlayCommand("HoldingOn");
			else if (m_bHoldShowing[i] == TapNoteSubType_Roll)
				m_Ghost[i]->PlayCommand("RollOn");
			/*
			else if( m_bHoldShowing[i] == TapNoteSubType_Mine )
				m_Ghost[i]->PlayCommand( "MinefieldOn" );
			*/
			m_bLastHoldShowing[i] = m_bHoldShowing[i];
		}
		m_bHoldShowing[i] = TapNoteSubType_Invalid;
	}
}

void
GhostArrowRow::DrawPrimitives()
{
	const Style* pStyle =
	  GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber);
	for (unsigned i = 0; i < m_Ghost.size(); i++) {
		const int c = pStyle->m_iColumnDrawOrder[i];
		m_Ghost[c]->Draw();
	}
}

void
GhostArrowRow::DidTapNote(int iCol, TapNoteScore tns, bool bBright)
{
	ASSERT_M(
	  iCol >= 0 && iCol < static_cast<int>(m_Ghost.size()),
	  ssprintf(
		"assert(iCol %i >= 0  && iCol %i < (int)m_Ghost.size() %i) failed",
		iCol,
		iCol,
		(int)m_Ghost.size()));

	Message msg("ColumnJudgment");
	msg.SetParam("TapNoteScore", tns);
	// This may be useful for popn styled judgment :) -DaisuMaster
	msg.SetParam("Column", iCol);
	msg.SetParam("Color", NOTESKIN->GetLastSeenColor());
	if (bBright)
		msg.SetParam("Bright", true);
	m_Ghost[iCol]->HandleMessage(msg);

	m_Ghost[iCol]->PlayCommand("Judgment");
	if (bBright)
		m_Ghost[iCol]->PlayCommand("Bright");
	else
		m_Ghost[iCol]->PlayCommand("Dim");
	m_Ghost[iCol]->PlayCommand(Capitalize(TapNoteScoreToString(tns)));
}

void
GhostArrowRow::DidHoldNote(int iCol, HoldNoteScore hns, bool bBright)
{
	ASSERT(iCol >= 0 && iCol < static_cast<int>(m_Ghost.size()));
	Message msg("ColumnJudgment");
	msg.SetParam("HoldNoteScore", hns);
	msg.SetParam("Column", iCol);
	msg.SetParam("Color", NOTESKIN->GetLastSeenColor());
	if (bBright)
		msg.SetParam("Bright", true);
	m_Ghost[iCol]->HandleMessage(msg);

	m_Ghost[iCol]->PlayCommand("Judgment");
	if (bBright)
		m_Ghost[iCol]->PlayCommand("Bright");
	else
		m_Ghost[iCol]->PlayCommand("Dim");
	m_Ghost[iCol]->PlayCommand(Capitalize(HoldNoteScoreToString(hns)));
}

void
GhostArrowRow::SetHoldShowing(int iCol, const TapNote& tn)
{
	ASSERT(iCol >= 0 && iCol < static_cast<int>(m_Ghost.size()));
	m_bHoldShowing[iCol] = tn.subType;
}
