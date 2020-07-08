#ifndef RECEPTOR_ARROW_ROW_H
#define RECEPTOR_ARROW_ROW_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "NoteDisplay.h"
#include "ReceptorArrow.h"

class PlayerState;
/** @brief A row of ReceptorArrow objects. */
class ReceptorArrowRow : public ActorFrame
{
  public:
	ReceptorArrowRow();
	~ReceptorArrowRow() override;
	void Update(float fDeltaTime) override;
	void DrawPrimitives() override;
	void DrawOverlay();

	void Load(const PlayerState* pPlayerState, float fYReverseOffset);
	void SetColumnRenderers(vector<NoteColumnRenderer>& renderers);

	void Step(int iCol, TapNoteScore score);
	void SetPressed(int iCol);

	void SetFadeToFailPercent(float fFadeToFailPercent)
	{
		m_fFadeToFailPercent = fFadeToFailPercent;
	}

  protected:
	const PlayerState* m_pPlayerState;
	float m_fYReverseOffsetPixels;
	float m_fFadeToFailPercent;

	std::vector<NoteColumnRenderer> const* m_renderers;
	std::vector<ReceptorArrow*> m_ReceptorArrow;
	std::vector<ReceptorArrow*> m_OverlayReceptorArrow;
};

#endif
