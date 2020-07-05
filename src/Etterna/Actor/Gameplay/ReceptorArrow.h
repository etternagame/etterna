#ifndef RECEPTOR_ARROW_H
#define RECEPTOR_ARROW_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"

class PlayerState;
/** @brief A gray arrow that "receives" the note arrows. */
class ReceptorArrow : public ActorFrame
{
  public:
	ReceptorArrow();
	void Load(const PlayerState* pPlayerState,
			  int iColNo,
			  const std::string& Type);

	void DrawPrimitives() override;
	void Update(float fDeltaTime) override;
	void Step(TapNoteScore score);
	void SetPressed() { m_bIsPressed = true; };

  private:
	const PlayerState* m_pPlayerState;
	int m_iColNo;

	AutoActor m_pReceptor;

	bool m_bIsPressed;
	bool m_bWasPressed; // set in Update
	bool m_bWasReverse;
};

#endif
