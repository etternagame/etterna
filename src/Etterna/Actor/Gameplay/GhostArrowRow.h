#ifndef GHOSTARROWROW_H
#define GHOSTARROWROW_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "NoteDisplay.h"

class PlayerState;
/** @brief Row of GhostArrow Actors. */
class GhostArrowRow : public ActorFrame
{
  public:
	~GhostArrowRow() override;
	void Update(float fDeltaTime) override;
	void DrawPrimitives() override;

	void Load(const PlayerState* pPlayerState, float fYReverseOffset);
	void SetColumnRenderers(vector<NoteColumnRenderer>& renderers);
	[[nodiscard]] int GetRendererCount() const
	{
		if (m_renderers != nullptr)
			return m_renderers->size();
		return 0;
	}

	// alias
	[[nodiscard]] int GetGhostCount() const { return GetReceptorCount(); }
	[[nodiscard]] int GetReceptorCount() const
	{
		return m_Ghost.size();
	}

	// Initialization happens before Loading
	[[nodiscard]] bool isInitialized() const { return GetGhostCount() > 0; }
	[[nodiscard]] bool isLoaded() const { return GetRendererCount() > 0; }

	void DidTapNote(int iCol, TapNoteScore tns, bool bBright);
	void DidHoldNote(int iCol, HoldNoteScore hns, bool bBright);
	void SetHoldShowing(int iCol, const TapNote& tn);

  protected:
	float m_fYReverseOffsetPixels = 0.F;
	const PlayerState* m_pPlayerState{};

	vector<NoteColumnRenderer> const* m_renderers{};
	vector<Actor*> m_Ghost;
	vector<TapNoteSubType> m_bHoldShowing;
	vector<TapNoteSubType> m_bLastHoldShowing;
};

#endif
