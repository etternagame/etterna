#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Core/Services/Locator.hpp"
#include "RageSoundMixBuffer.h"
#include "RageSoundReader_Chain.h"
#include "RageSoundReader_FileReader.h"
#include "RageSoundReader_Pan.h"
#include "RageSoundReader_Preload.h"
#include "RageSoundReader_Resample_Good.h"
#include "RageUtil/Utils/RageUtil.h"

#include <algorithm>

using std::max;
using std::min;

/*
 * Keyed sounds should pass this object to SoundReader_Preload, to preprocess
 * it. Streaming more than two or three sounds is too expensive (keyed games can
 * play two dozen), and reading from disk is too latent.
 *
 * This can also be used for chained background music, which should always
 * stream, so we don't do the preloading in here.
 */
RageSoundReader_Chain::RageSoundReader_Chain()
{
	m_iPreferredSampleRate = 44100;
	m_iActualSampleRate = -1;
	m_iChannels = 0;
	m_iCurrentFrame = 0;
	m_iNextSound = 0;
}

RageSoundReader_Chain::~RageSoundReader_Chain()
{
	/* Clear m_apActiveSounds. */
	while (!m_apActiveSounds.empty())
		ReleaseSound(m_apActiveSounds.front());

	FOREACH(RageSoundReader*, m_apLoadedSounds, it)
	delete *it;
}

RageSoundReader_Chain*
RageSoundReader_Chain::Copy() const
{
	// XXX
	FAIL_M("unimplemented");
	return 0;
}

/* The same sound may be used several times, and by several different chains.
 * Avoid loading the same sound multiple times.  We need to make a Copy() if we
 * need to read it more than once at a time. */
void
RageSoundReader_Chain::AddSound(int iIndex, float fOffsetSecs, float fPan)
{
	if (iIndex == -1)
		return;

	Sound s;
	s.iIndex = iIndex;
	s.iOffsetMS = lround(fOffsetSecs * 1000);
	s.fPan = fPan;
	s.pSound = nullptr;
	m_aSounds.push_back(s);
}

int
RageSoundReader_Chain::LoadSound(std::string sPath)
{
	sPath = make_lower(sPath);

	std::map<std::string, RageSoundReader*>::const_iterator it =
	  m_apNamedSounds.find(sPath);
	if (it != m_apNamedSounds.end()) {
		const RageSoundReader* pReader = it->second;

		for (int i = 0; i < (int)m_apLoadedSounds.size(); ++i)
			if (m_apLoadedSounds[i] == pReader)
				return i;
		FAIL_M(sPath);
	}

	std::string sError;
	bool bPrebuffer;
	RageSoundReader* pReader =
	  RageSoundReader_FileReader::OpenFile(sPath, sError, &bPrebuffer);
	if (pReader == nullptr) {
		Locator::getLogger()->warn("RageSoundReader_Chain: error opening sound \"{}\": {}",
				  sPath.c_str(), sError.c_str());
		return -1;
	}

	m_apNamedSounds[sPath] = pReader;

	m_apLoadedSounds.push_back(m_apNamedSounds[sPath]);
	return m_apLoadedSounds.size() - 1;
}

int
RageSoundReader_Chain::LoadSound(RageSoundReader* pSound)
{
	m_apLoadedSounds.push_back(pSound);
	return m_apLoadedSounds.size() - 1;
}

/* If every sound has the same sample rate, return it.  Otherwise, return -1. */
int
RageSoundReader_Chain::GetSampleRateInternal() const
{
	if (m_apLoadedSounds.empty())
		return m_iPreferredSampleRate;

	int iRate = -1;
	FOREACH_CONST(RageSoundReader*, m_apLoadedSounds, it)
	{
		if (iRate == -1)
			iRate = (*it)->GetSampleRate();
		else if (iRate != (*it)->GetSampleRate())
			return -1;
	}
	return iRate;
}

void
RageSoundReader_Chain::Finish()
{
	/* Figure out how many channels we have.  All sounds must either have 1 or 2
	 * channels, which will be converted as needed, or have the same number of
	 * channels. */
	m_iChannels = 1;
	FOREACH(RageSoundReader*, m_apLoadedSounds, it)
	m_iChannels = std::max(m_iChannels, (*it)->GetNumChannels());

	if (m_iChannels > 2) {
		FOREACH(RageSoundReader*, m_apLoadedSounds, it)
		{
			if ((*it)->GetNumChannels() != m_iChannels) {
				Locator::getLogger()->warn("Discarded sound with {} channels, not {}",
						  (*it)->GetNumChannels(),
						  m_iChannels);
				delete (*it);
				(*it) = nullptr;
			}
		}
	}

	/* Remove any sounds that don't have corresponding RageSoundReaders. */
	for (unsigned i = 0; i < m_aSounds.size();) {
		Sound& sound = m_aSounds[i];

		if (m_apLoadedSounds[sound.iIndex] == nullptr) {
			m_aSounds.erase(m_aSounds.begin() + i);
			continue;
		}

		++i;
	}

	/*
	 * We might get different sample rates from our sources.  If they're all the
	 * same sample rate, just leave it alone, so the whole sound can be
	 * resampled as a group. If not, resample eveything to the preferred rate.
	 * (Using the preferred rate should avoid redundant resampling later.)
	 */
	m_iActualSampleRate = GetSampleRateInternal();
	if (m_iActualSampleRate == -1) {
		for (auto& it : m_apLoadedSounds) {
			RageSoundReader*& pSound = it;

			RageSoundReader_Resample_Good* pResample =
			  new RageSoundReader_Resample_Good(pSound, m_iPreferredSampleRate);
			pSound = pResample;
		}

		m_iActualSampleRate = m_iPreferredSampleRate;
	}

	/* Attempt to preload all sounds. */
	FOREACH(RageSoundReader*, m_apLoadedSounds, it)
	{
		RageSoundReader*& pSound = (*it);
		RageSoundReader_Preload::PreloadSound(pSound);
	}

	/* Sort the sounds by start time. */
	sort(m_aSounds.begin(), m_aSounds.end());
}

int
RageSoundReader_Chain::SetPosition(int iFrame)
{
	/* Clear m_apActiveSounds. */
	while (!m_apActiveSounds.empty())
		ReleaseSound(m_apActiveSounds.front());

	m_iCurrentFrame = iFrame;

	/* Run through all sounds in the chain, and activate all sounds which have
	 * data at iFrame. */
	for (m_iNextSound = 0; m_iNextSound < m_aSounds.size(); ++m_iNextSound) {
		Sound* pSound = &m_aSounds[m_iNextSound];
		int iOffsetFrame = pSound->GetOffsetFrame(GetSampleRate());

		/* If this sound is in the future, skip it. */
		if (iOffsetFrame > iFrame)
			break;

		/* Find the RageSoundReader. */
		ActivateSound(pSound);
		RageSoundReader* pReader = pSound->pSound;

		int iOffsetFrames = iFrame - iOffsetFrame;
		if (pReader->SetPosition(iOffsetFrames) == 0) {
			/* We're past the end of this sound. */
			ReleaseSound(pSound);
			continue;
		}
	}

	/* If no sounds were started, and we have no sounds ahead of us, we've
	 * seeked past EOF. */
	if (m_apActiveSounds.empty() && m_iNextSound == m_aSounds.size())
		return 0;

	return 1;
}

void
RageSoundReader_Chain::ActivateSound(Sound* s)
{
	RageSoundReader* pSound = m_apLoadedSounds[s->iIndex];
	s->pSound = pSound->Copy();

	/* Add a balance filter.  If this source has the same number of channels
	 * as this sound, and does not need to be panned, we can omit this. */
	if (s->fPan != 0.0f ||
		s->pSound->GetNumChannels() != this->GetNumChannels()) {
		s->pSound = new RageSoundReader_Pan(s->pSound);
		s->pSound->SetProperty("Pan", s->fPan);
	}

	m_apActiveSounds.push_back(s);
}

void
RageSoundReader_Chain::ReleaseSound(Sound* s)
{
	std::vector<Sound*>::iterator it =
	  find(m_apActiveSounds.begin(), m_apActiveSounds.end(), s);
	ASSERT(it != m_apActiveSounds.end());
	RageSoundReader*& pSound = s->pSound;

	delete pSound;
	pSound = nullptr;

	m_apActiveSounds.erase(it);
}

bool
RageSoundReader_Chain::SetProperty(const std::string& sProperty, float fValue)
{
	bool bRet = false;
	for (unsigned i = 0; i < m_apActiveSounds.size(); ++i) {
		if (m_apActiveSounds[i]->pSound->SetProperty(sProperty, fValue))
			bRet = true;
	}
	return bRet;
}

int
RageSoundReader_Chain::GetNextSourceFrame() const
{
	return m_iCurrentFrame;
	/*	if( m_apActiveSounds.empty() )
			return m_iCurrentFrame;

		int iPosition = m_apActiveSounds[0]->pSound->GetNextSourceFrame();
		iPosition += m_apActiveSounds[0]->GetOffsetFrame( GetSampleRate() );

		for( unsigned i = 1; i < m_apActiveSounds.size(); ++i )
		{
			int iThisPosition =
	   m_apActiveSounds[i]->pSound->GetNextSourceFrame(); iThisPosition +=
	   m_apActiveSounds[i]->GetOffsetFrame( GetSampleRate() ); if( iThisPosition
	   != iPosition ) LOG->Warn( "RageSoundReader_Chain: sound positions moving
	   at different rates" );
		}

		return iPosition;
	*/
}

float
RageSoundReader_Chain::GetStreamToSourceRatio() const
{
	if (m_apActiveSounds.empty())
		return 1.0f;

	float iRate = m_apActiveSounds[0]->pSound->GetStreamToSourceRatio();
	for (unsigned i = 1; i < m_apActiveSounds.size(); ++i) {
		if (m_apActiveSounds[i]->pSound->GetStreamToSourceRatio() != iRate)
			Locator::getLogger()->warn("RageSoundReader_Chain: sound rates changing differently");
	}

	return iRate;
}

/* As we iterate through the sound tree, we'll find that we need data from
 * different sounds; a sound may be needed by more than one other sound. */
int
RageSoundReader_Chain::Read(float* pBuffer, int iFrames)
{
	while (m_iNextSound < m_aSounds.size() &&
		   m_iCurrentFrame ==
			 m_aSounds[m_iNextSound].GetOffsetFrame(m_iActualSampleRate)) {
		Sound* pSound = &m_aSounds[m_iNextSound];
		ActivateSound(pSound);
		++m_iNextSound;
	}

	if (m_iNextSound == m_aSounds.size() && m_apActiveSounds.empty())
		return END_OF_FILE;

	/* Clamp iFrames to the beginning of the next sound we need to start. */
	if (m_iNextSound < m_aSounds.size()) {
		int iOffsetFrame =
		  m_aSounds[m_iNextSound].GetOffsetFrame(m_iActualSampleRate);
		ASSERT_M(iOffsetFrame >= m_iCurrentFrame,
				 ssprintf("%i %i", iOffsetFrame, m_iCurrentFrame));
		int iFramesToRead = iOffsetFrame - m_iCurrentFrame;
		iFrames = std::min(iFramesToRead, iFrames);
	}

	if (m_apActiveSounds.size() == 1 &&
		m_apActiveSounds.front()->pSound->GetNumChannels() == m_iChannels &&
		m_apActiveSounds.front()->pSound->GetSampleRate() ==
		  m_iActualSampleRate) {
		/* We have only one source, and it matches our target.  Don't mix; read
		 * directly from the source into the destination.  This is to optimize
		 * the common case of having one BGM track and no autoplay sounds. */
		iFrames = m_apActiveSounds.front()->pSound->Read(pBuffer, iFrames);
		if (iFrames < 0)
			ReleaseSound(m_apActiveSounds.front());
		if (iFrames > 0)
			m_iCurrentFrame += iFrames;
		return iFrames;
	}

	if (m_apActiveSounds.empty()) {
		/* If we have more sounds ahead of us, pretend we read the entire block,
		 * since there's silence in between.  Otherwise, we're at EOF. */
		memset(pBuffer, 0, iFrames * m_iChannels * sizeof(float));
		m_iCurrentFrame += iFrames;
		return iFrames;
	}

	RageSoundMixBuffer mix;
	/* Read iFrames from each sound. */
	float Buffer[2048];
	iFrames = std::min(iFrames, (int)(ARRAYLEN(Buffer) / m_iChannels));
	for (unsigned i = 0; i < m_apActiveSounds.size();) {
		RageSoundReader* pSound = m_apActiveSounds[i]->pSound;
		ASSERT(pSound->GetNumChannels() ==
			   m_iChannels); // guaranteed by ActivateSound and Finish

		/* If we receive less than we were asked for, keep asking for more data.
		 * Most filters would simply return what they have in this situation,
		 * but we want to deal transparently with separate sounds returning
		 * differently-sized partial blocks. */
		int iFramesRead = 0;
		while (iFramesRead < iFrames) {
			int iGotFrames = pSound->RetriedRead(Buffer, iFrames - iFramesRead);
			if (iGotFrames < 0) {
				iFramesRead = iGotFrames;
				break;
			}

			mix.SetWriteOffset(iFramesRead * pSound->GetNumChannels());
			mix.write(Buffer, iGotFrames * pSound->GetNumChannels());
			iFramesRead += iGotFrames;
		}

		if (iFramesRead < 0)
			ReleaseSound(m_apActiveSounds[i]);
		else
			++i;
	}

	/* Read mixed frames into the output buffer. */
	int iMaxFramesRead = mix.size() / m_iChannels;
	mix.read(pBuffer);
	m_iCurrentFrame += iMaxFramesRead;

	return iMaxFramesRead;
}

int
RageSoundReader_Chain::GetLength() const
{
	int iLength = 0;
	for (unsigned i = 0; i < m_aSounds.size(); ++i) {
		const Sound& sound = m_aSounds[i];
		const RageSoundReader* pSound = m_apLoadedSounds[sound.iIndex];
		int iThisLength = pSound->GetLength();
		if (iThisLength)
			iLength = std::max(iLength, iThisLength + sound.iOffsetMS);
	}
	return iLength;
}

int
RageSoundReader_Chain::GetLength_Fast() const
{
	int iLength = 0;
	for (unsigned i = 0; i < m_aSounds.size(); ++i) {
		const Sound& sound = m_aSounds[i];
		const RageSoundReader* pSound = m_apLoadedSounds[sound.iIndex];
		int iThisLength = pSound->GetLength_Fast();
		if (iThisLength)
			iLength = std::max(iLength, iThisLength + sound.iOffsetMS);
	}
	return iLength;
}
