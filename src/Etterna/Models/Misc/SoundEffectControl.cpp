#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "PlayerState.h"
#include "RageUtil/Sound/RageSoundReader.h"
#include "RageUtil/Sound/RageSoundReader.h"
#include "SoundEffectControl.h"

SoundEffectControl::SoundEffectControl()
{
	m_bLocked = false;
	m_fSample = 0.F;
	m_fLastLevel = 0.F;
	m_pPlayerState = nullptr;
	m_pNoteData = nullptr;
	m_pSoundReader = nullptr;
}

void
SoundEffectControl::Load(const std::string& sType,
						 PlayerState* pPlayerState,
						 const NoteData* pNoteData)
{
	SOUND_PROPERTY.Load(sType, "SoundProperty");
	LOCK_TO_HOLD.Load(sType, "LockToHold");
	PROPERTY_MIN.Load(sType, "PropertyMin");
	PROPERTY_CENTER.Load(sType, "PropertyCenter");
	PROPERTY_MAX.Load(sType, "PropertyMax");

	m_pPlayerState = pPlayerState;
	m_pNoteData = pNoteData;
}

void
SoundEffectControl::SetSoundReader(RageSoundReader* pPlayer)
{
	m_pSoundReader = pPlayer;
}

void
SoundEffectControl::Update(float fDeltaTime)
{
	if (SOUND_PROPERTY == "")
		return;

	auto fLevel = INPUTMAPPER->GetLevel(GAME_BUTTON_EFFECT_UP,
										m_pPlayerState->m_PlayerNumber);
	fLevel -= INPUTMAPPER->GetLevel(GAME_BUTTON_EFFECT_DOWN,
									m_pPlayerState->m_PlayerNumber);
	CLAMP(fLevel, -1.0f, +1.0f);

	if (LOCK_TO_HOLD) {
		auto iRow = BeatToNoteRow(GAMESTATE->m_Position.m_fSongBeat);
		int iHoldsHeld, iHoldsLetGo;
		HoldsBeingHeld(iRow, iHoldsHeld, iHoldsLetGo);

		/* If no holds are being held, or any have been missed, lock the effect
		 * off until the button has been released. */
		if (iHoldsLetGo > 0 || iHoldsHeld == 0)
			m_bLocked = true;

		/* If the button is released, unlock it when the level crosses or
		 * reaches 0. */
		if (m_bLocked) {
			if ((fLevel <= 0.0f && m_fLastLevel >= 0.0f) ||
				(fLevel >= 0.0f && m_fLastLevel <= 0.0f))
				m_bLocked = false;
		}
	}

	m_fLastLevel = fLevel;

	if (m_bLocked)
		fLevel = 0.0f;

	m_fSample = fLevel;
	m_pPlayerState->m_EffectHistory.AddSample(m_fSample, fDeltaTime);

	float fPropertyMin = PROPERTY_MIN;
	float fPropertyCenter = PROPERTY_CENTER;
	float fPropertyMax = PROPERTY_MAX;

	float fCurrent;
	if (m_fSample < 0)
		fCurrent = SCALE(m_fSample, 0.0f, -1.0f, fPropertyCenter, fPropertyMin);
	else
		fCurrent = SCALE(m_fSample, 0.0f, +1.0f, fPropertyCenter, fPropertyMax);

	if (m_pSoundReader)
		m_pSoundReader->SetProperty(SOUND_PROPERTY, fCurrent);
}

/* Return false if any holds have been LetGo.  Otherwise, return true if at
 * least one hold is active. */
void
SoundEffectControl::HoldsBeingHeld(int iRow,
								   int& iHoldsHeld,
								   int& iHoldsLetGo) const
{
	iHoldsHeld = iHoldsLetGo = 0;
	for (auto c = 0; c < m_pNoteData->GetNumTracks(); ++c) {
		NoteData::TrackMap::const_iterator begin, end;
		m_pNoteData->GetTapNoteRangeInclusive(c, iRow, iRow + 1, begin, end);
		if (begin == end)
			continue;

		const auto& tn = begin->second;
		if (tn.type != TapNoteType_HoldHead)
			continue;
		if (tn.HoldResult.bActive)
			++iHoldsHeld;
		else if (tn.HoldResult.hns == HNS_LetGo)
			++iHoldsLetGo;
	}
}
