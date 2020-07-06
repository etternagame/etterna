#ifndef AUTO_KEYSOUNDS_H
#define AUTO_KEYSOUNDS_H

#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/Misc/PlayerNumber.h"
#include "RageUtil/Sound/RageSound.h"

class RageSoundReader;
class RageSoundReader_Chain;
class Song;
/** @brief Handle playback of auto keysound notes. */
class AutoKeysounds
{
  public:
	void Load(PlayerNumber pn, const NoteData& ndAutoKeysoundsOnly);
	void Update(float fDelta);
	/** @brief Finish loading the main sounds, and setup the auto keysounds if
	 * any. */
	void FinishLoading();
	RageSound* GetSound() { return &m_sSound; }
	RageSoundReader* GetSharedSound() const { return m_pSharedSound; }
	RageSoundReader* GetPlayerSound(PlayerNumber pn) const
	{
		if (pn == PLAYER_INVALID)
			return nullptr;
		return m_pPlayerSounds;
	}

  protected:
	void LoadAutoplaySoundsInto(RageSoundReader_Chain* pChain);
	static void LoadTracks(const Song* pSong,
						   RageSoundReader*& pGlobal,
						   RageSoundReader*& pPlayer1);

	NoteData m_ndAutoKeysoundsOnly;
	vector<RageSound> m_vKeysounds;
	RageSound m_sSound;
	RageSoundReader* m_pChain;		  // owned by m_sSound
	RageSoundReader* m_pPlayerSounds; // owned by m_sSound
	RageSoundReader* m_pSharedSound;  // owned by m_sSound
};

#endif
