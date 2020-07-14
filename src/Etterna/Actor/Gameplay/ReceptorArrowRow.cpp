#include "Etterna/Globals/global.h"
#include "ArrowEffects.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "RageUtil/Utils/RageUtil.h"
#include "ReceptorArrowRow.h"
#include "Etterna/Models/StepsAndStyles/Style.h"

#include <map>

std::map<int, std::map<int, bool>> NoteUpcoming;

ReceptorArrowRow::ReceptorArrowRow()
{
	NoteUpcoming.clear();
	m_pPlayerState = nullptr;
	m_fYReverseOffsetPixels = 0;
	m_fFadeToFailPercent = 0;
	m_renderers = nullptr;
}

void
ReceptorArrowRow::Load(const PlayerState* pPlayerState, float fYReverseOffset)
{
	m_pPlayerState = pPlayerState;
	m_fYReverseOffsetPixels = fYReverseOffset;

	const auto* const pStyle =
	  GAMESTATE->GetCurrentStyle(pPlayerState->m_PlayerNumber);

	for (auto c = 0; c < pStyle->m_iColsPerPlayer; c++) {
		m_ReceptorArrow.push_back(new ReceptorArrow);
		m_OverlayReceptorArrow.push_back(new ReceptorArrow);
		m_ReceptorArrow[c]->SetName("ReceptorArrow");
		m_OverlayReceptorArrow[c]->SetName("OverlayReceptor");
		m_ReceptorArrow[c]->Load(m_pPlayerState, c, "Receptor");
		m_OverlayReceptorArrow[c]->Load(m_pPlayerState, c, "Ovceptor");
		this->AddChild(m_ReceptorArrow[c]);
		this->AddChild(m_OverlayReceptorArrow[c]);
	}
}

void
ReceptorArrowRow::SetColumnRenderers(vector<NoteColumnRenderer>& renderers)
{
	ASSERT_M(renderers.size() == m_ReceptorArrow.size(),
			 "Notefield has different number of columns than receptor row.");
	for (size_t c = 0; c < m_ReceptorArrow.size(); ++c) {
		m_ReceptorArrow[c]->SetFakeParent(&(renderers[c]));
	}
	m_renderers = &renderers;
}

ReceptorArrowRow::~ReceptorArrowRow()
{
	for (auto& i : m_ReceptorArrow)
		delete i;
}

void
ReceptorArrowRow::Update(float fDeltaTime)
{
	ActorFrame::Update(fDeltaTime);
	// If we're on gameplay, then the notefield will take care of updating
	// ArrowEffects.  But if we're on ScreenNameEntry, there is no notefield,
	// Checking whether m_renderers is null is a proxy for checking whether
	// there is a notefield. -Kyz
	if (m_renderers == nullptr) {
		ArrowEffects::Update();
	}

	for (unsigned c = 0; c < m_ReceptorArrow.size(); c++) {
		// m_fDark==1 or m_fFadeToFailPercent==1 should make fBaseAlpha==0
		auto fBaseAlpha =
		  (1 - m_pPlayerState->m_PlayerOptions.GetCurrent().m_fDark);
		if (m_fFadeToFailPercent != -1) {
			fBaseAlpha *= (1 - m_fFadeToFailPercent);
		}
		CLAMP(fBaseAlpha, 0.0f, 1.0f);
		m_ReceptorArrow[c]->SetBaseAlpha(fBaseAlpha);
		m_OverlayReceptorArrow[c]->SetBaseAlpha(fBaseAlpha);

		if (m_renderers != nullptr) {
			// set arrow XYZ
			(*m_renderers)[c].UpdateReceptorGhostStuff(m_ReceptorArrow[c]);
			(*m_renderers)[c].UpdateReceptorGhostStuff(
			  m_OverlayReceptorArrow[c]);
		} else {
			// ScreenNameEntry uses ReceptorArrowRow but doesn't have or need
			// column renderers.  Just do the lazy thing and offset x. -Kyz
			const auto* style =
			  GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber);
			m_ReceptorArrow[c]->SetX(style->m_ColumnInfo[c].fXOffset);
			m_OverlayReceptorArrow[c]->SetX(style->m_ColumnInfo[c].fXOffset);
		}
	}
}

void
ReceptorArrowRow::DrawPrimitives()
{
	const auto* pStyle =
	  GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber);
	for (unsigned i = 0; i < m_ReceptorArrow.size(); i++) {
		const auto c = pStyle->m_iColumnDrawOrder[i];
		m_ReceptorArrow[c]->Draw();
	}
}

void
ReceptorArrowRow::DrawOverlay()
{
	const auto* pStyle =
	  GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber);
	for (unsigned i = 0; i < m_ReceptorArrow.size(); i++) {
		const auto c = pStyle->m_iColumnDrawOrder[i];
		m_OverlayReceptorArrow[c]->Draw();
	}
}

void
ReceptorArrowRow::Step(int iCol, TapNoteScore score)
{
	ASSERT(iCol >= 0 && iCol < static_cast<int>(m_ReceptorArrow.size()));
	m_ReceptorArrow[iCol]->Step(score);
}

void
ReceptorArrowRow::SetPressed(int iCol)
{
	ASSERT(iCol >= 0 && iCol < static_cast<int>(m_ReceptorArrow.size()));
	m_ReceptorArrow[iCol]->SetPressed();
}
