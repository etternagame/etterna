/*
 * This class handles two things: auto-play preload, and runtime auto-play
 * sounds.
 *
 * On song start, all autoplay sounds and the main BGM track (if any) are
 * combined into a single chain, which is used as the song background.  Sounds
 * added this way are removed from the NoteData.
 *
 * Any sounds not added to the sound chain and any autoplay sounds added to the
 * NoteData during play (usually due to battle mode mods) are played
 * dynamically, via Update().
 *
 * Note that autoplay sounds which are played before the BGM starts will never
 * be placed in the sound chain, since the sound chain becomes the BGM; the BGM
 * can't play sound before it starts.  These sounds will be left in the
 * NoteData, and played as dynamic sounds; this means that they don't get robust
 * sync.  This isn't a problem for imported BMS files, which don't have an
 * offset value, but it's annoying.
 */

#include "Etterna/Globals/global.h"
#include "AutoKeysounds.h"
#include "Etterna/Singletons/GameState.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/Sound/RageSoundReader_Chain.h"
#include "RageUtil/Sound/RageSoundReader_ChannelSplit.h"
#include "RageUtil/Sound/RageSoundReader_Extend.h"
#include "RageUtil/Sound/RageSoundReader_FileReader.h"
#include "RageUtil/Sound/RageSoundReader_Merge.h"
#include "RageUtil/Sound/RageSoundReader_Pan.h"
#include "RageUtil/Sound/RageSoundReader_PitchChange.h"
#include "RageUtil/Sound/RageSoundReader_PostBuffering.h"
#include "RageUtil/Sound/RageSoundReader_ThreadedBuffer.h"
#include "Etterna/Models/Songs/Song.h"

#include <algorithm>

void
AutoKeysounds::Load(PlayerNumber pn, const NoteData& ndAutoKeysoundsOnly)
{
	m_ndAutoKeysoundsOnly = ndAutoKeysoundsOnly;
}

void
AutoKeysounds::LoadAutoplaySoundsInto(RageSoundReader_Chain* pChain)
{
	//
	// Load sounds.
	//
	Song* pSong = GAMESTATE->m_pCurSong;
	auto sSongDir = pSong->GetSongDir();

	/*
	 * Add all current autoplay sounds in both players to the chain.
	 */
	auto iNumTracks = m_ndAutoKeysoundsOnly.GetNumTracks();
	for (auto t = 0; t < iNumTracks; t++) {
		auto iRow = -1;
		for (;;) {
			/* Find the next row that either player has a note on. */
			auto iNextRow = INT_MAX;
			// XXX Hack. Enabled players need not have their own note data.
			if (!(t >= m_ndAutoKeysoundsOnly.GetNumTracks())) {
				auto iNextRowForPlayer = iRow;
				/* XXX: If a BMS file only has one tap note per track,
				 * this will prevent any keysounds from loading.
				 * This leads to failure later on.
				 * We need a better way to prevent this. */
				if (m_ndAutoKeysoundsOnly.GetNextTapNoteRowForTrack(
					  t, iNextRowForPlayer))
					iNextRow = std::min(iNextRow, iNextRowForPlayer);
			}

			if (iNextRow == INT_MAX)
				break;
			iRow = iNextRow;

			TapNote tn;
			tn = m_ndAutoKeysoundsOnly.GetTapNote(t, iRow);

			if (tn == TAP_EMPTY)
				continue;

			ASSERT(tn.type == TapNoteType_AutoKeysound);
			if (tn.iKeysoundIndex >= 0) {
				auto sKeysoundFilePath =
				  sSongDir + pSong->m_vsKeysoundFile[tn.iKeysoundIndex];
				auto fSeconds =
				  GAMESTATE->m_pCurSteps->GetTimingData()->WhereUAtBroNoOffset(
					NoteRowToBeat(iRow)) +
				  SOUNDMAN->GetPlayLatency();

				float fPan = 0;
				auto iIndex = pChain->LoadSound(sKeysoundFilePath);
				pChain->AddSound(iIndex, fSeconds, fPan);
			}
		}
	}
}

void
AutoKeysounds::LoadTracks(const Song* pSong,
						  RageSoundReader*& pShared,
						  RageSoundReader*& pPlayer1)
{
	pPlayer1 = nullptr;
	pShared = nullptr;

	vector<std::string> vsMusicFile;
	const auto sMusicPath = GAMESTATE->m_pCurSteps->GetMusicPath();

	if (!sMusicPath.empty())
		vsMusicFile.push_back(sMusicPath);

	vector<RageSoundReader*> vpSounds;
	for (auto& s : vsMusicFile) {
		std::string sError;
		RageSoundReader* pSongReader =
		  RageSoundReader_FileReader::OpenFile(s, sError);
		vpSounds.push_back(pSongReader);
	}

	if (vpSounds.size() == 1) {
		auto pSongReader = vpSounds[0];

		// Load the buffering filter before the effects filters, so effects
		// aren't delayed.
		pSongReader = new RageSoundReader_Extend(pSongReader);
		pSongReader = new RageSoundReader_ThreadedBuffer(pSongReader);
		pShared = pSongReader;
	} else if (!vpSounds.empty()) {
		auto* pMerge = new RageSoundReader_Merge;

		for (auto& so : vpSounds)
			pMerge->AddSound(so);
		pMerge->Finish(SOUNDMAN->GetDriverSampleRate());

		RageSoundReader* pSongReader = pMerge;

		// Load the buffering filter before the effects filters, so effects
		// aren't delayed.
		pSongReader = new RageSoundReader_Extend(pSongReader);
		pSongReader = new RageSoundReader_ThreadedBuffer(pSongReader);
		pShared = pSongReader;
	}

	return;
}

void
AutoKeysounds::FinishLoading()
{
	m_sSound.Unload();

	Song* pSong = GAMESTATE->m_pCurSong;

	vector<RageSoundReader*> apSounds;
	LoadTracks(pSong, m_pSharedSound, m_pPlayerSounds);

	// Load autoplay sounds, if any.
	{
		auto* pChain = new RageSoundReader_Chain;
		pChain->SetPreferredSampleRate(SOUNDMAN->GetDriverSampleRate());
		LoadAutoplaySoundsInto(pChain);

		if (pChain->GetNumSounds() > 0 || (m_pSharedSound == nullptr)) {
			if (m_pSharedSound != nullptr) {
				auto iIndex = pChain->LoadSound(m_pSharedSound);
				pChain->AddSound(iIndex, 0.0f, 0);
			}
			pChain->Finish();
			m_pSharedSound = new RageSoundReader_Extend(pChain);
		} else {
			delete pChain;
		}
	}
	ASSERT_M(m_pSharedSound != nullptr,
			 ssprintf("No keysounds were loaded for the song %s!",
					  pSong->m_sMainTitle.c_str()));

	m_pSharedSound = new RageSoundReader_PitchChange(m_pSharedSound);
	m_pSharedSound = new RageSoundReader_PostBuffering(m_pSharedSound);
	m_pSharedSound = new RageSoundReader_Pan(m_pSharedSound);
	apSounds.push_back(m_pSharedSound);

	if (m_pPlayerSounds != nullptr) {
		m_pPlayerSounds = new RageSoundReader_PitchChange(m_pPlayerSounds);
		m_pPlayerSounds = new RageSoundReader_PostBuffering(m_pPlayerSounds);
		m_pPlayerSounds = new RageSoundReader_Pan(m_pPlayerSounds);
		apSounds.push_back(m_pPlayerSounds);
	}

	if (apSounds.size() > 1) {
		auto* pMerge = new RageSoundReader_Merge;

		for (auto& ps : apSounds) {
			pMerge->AddSound(ps);
		}

		pMerge->Finish(SOUNDMAN->GetDriverSampleRate());

		m_pChain = pMerge;
	} else {
		ASSERT(!apSounds.empty());
		m_pChain = apSounds[0];
	}

	m_sSound.LoadSoundReader(m_pChain);
}

void
AutoKeysounds::Update(float fDelta)
{
	// Play keysounds for crossed rows.
	/*
		bool bCrossedABeat = false;
		{
			float fPositionSeconds = GAMESTATE->m_fMusicSeconds;
			float fSongBeat = GAMESTATE->m_pCurSong->GetBeatFromElapsedTime(
	   fPositionSeconds );

			int iRowNow = BeatToNoteRow( fSongBeat );
			iRowNow = max( 0, iRowNow );
			static int iRowLastCrossed = 0;

			float fBeatLast = roundf(NoteRowToBeat(iRowLastCrossed));
			float fBeatNow = roundf(NoteRowToBeat(iRowNow));

			bCrossedABeat = fBeatLast != fBeatNow;

			FOREACH_EnabledPlayer( pn )
			{
				const NoteData &nd = m_ndAutoKeysoundsOnly[pn];

				for( int t=0; t<nd.GetNumTracks(); t++ )
				{
					FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( nd, t, r,
	   iRowLastCrossed+1, iRowNow )
					{
						const TapNote &tn = nd.GetTapNote( t, r );
						ASSERT( tn.type == TapNoteType_AutoKeysound );
						if( tn.bKeysound )
							m_vKeysounds[tn.iKeysoundIndex].Play();
					}
				}
			}

			iRowLastCrossed = iRowNow;
		}
	*/
}
