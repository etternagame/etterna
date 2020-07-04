#ifndef SOUND_EFFECT_CONTROL_H
#define SOUND_EFFECT_CONTROL_H

#include "ThemeMetric.h"

class RageSoundReader;
class PlayerState;
class NoteData;
/** @brief Control a sound property through user input. */
class SoundEffectControl
{
  public:
	SoundEffectControl();
	void Load(const std::string& sType,
			  PlayerState* pPlayerState,
			  const NoteData* pNoteData);

	void SetSoundReader(RageSoundReader* pPlayer);
	void ReleaseSound() { SetSoundReader(nullptr); }

	void Update(float fDeltaTime);

  private:
	void HoldsBeingHeld(int iRow, int& iHoldsHeld, int& iHoldsLetGo) const;

	ThemeMetric<std::string> SOUND_PROPERTY;
	ThemeMetric<bool> LOCK_TO_HOLD;
	ThemeMetric<float> PROPERTY_MIN;
	ThemeMetric<float> PROPERTY_CENTER;
	ThemeMetric<float> PROPERTY_MAX;

	PlayerState* m_pPlayerState;

	bool m_bLocked;

	float m_fSample;
	float m_fLastLevel;

	const NoteData* m_pNoteData;
	RageSoundReader* m_pSoundReader;
};

#endif
