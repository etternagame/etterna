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
	[[nodiscard]] int GetRendererCount() const
	{
		if (m_renderers != nullptr)
			return m_renderers->size();
		return 0;
	}

	[[nodiscard]] int GetReceptorCount() const
	{
		return m_ReceptorArrow.size();
	}

	// Initialization happens before Loading
	[[nodiscard]] bool isInitialized() const { return GetReceptorCount() > 0; }
	[[nodiscard]] bool isLoaded() const { return GetRendererCount() > 0; }

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
