#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Core/Services/Locator.hpp"
#include "RageSoundMixBuffer.h"
#include "RageSoundReader_Merge.h"
#include "RageSoundReader_Pan.h"
#include "RageSoundReader_Resample_Good.h"
#include "RageUtil/Utils/RageUtil.h"

#include <algorithm>

RageSoundReader_Merge::RageSoundReader_Merge()
{
	m_iSampleRate = -1;
	m_iChannels = 0;
	m_iNextSourceFrame = 0;
	m_fCurrentStreamToSourceRatio = 1.0f;
}

RageSoundReader_Merge::~RageSoundReader_Merge()
{
	FOREACH(RageSoundReader*, m_aSounds, it)
	delete *it;
}

RageSoundReader_Merge::RageSoundReader_Merge(const RageSoundReader_Merge& cpy)
  : RageSoundReader(cpy)
{
	m_iSampleRate = cpy.m_iSampleRate;
	m_iChannels = cpy.m_iChannels;
	m_iNextSourceFrame = cpy.m_iNextSourceFrame;
	m_fCurrentStreamToSourceRatio = cpy.m_fCurrentStreamToSourceRatio;

	for (auto& it : cpy.m_aSounds) {
		m_aSounds.push_back(it->Copy());
	}
}

void
RageSoundReader_Merge::AddSound(RageSoundReader* pSound)
{
	m_aSounds.push_back(pSound);
}

/* If every sound has the same sample rate, return it.  Otherwise, return -1. */
int
RageSoundReader_Merge::GetSampleRateInternal() const
{
	auto iRate = -1;
	for (auto& it : m_aSounds) {
		if (iRate == -1)
			iRate = it->GetSampleRate();
		else if (iRate != it->GetSampleRate())
			return -1;
	}
	return iRate;
}

void
RageSoundReader_Merge::Finish(int iPreferredSampleRate)
{
	/* Figure out how many channels we have.  All sounds must either have 1 or 2
	 * channels, which will be converted as needed, or have the same number of
	 * channels. */
	m_iChannels = 1;
	for (auto& it : m_aSounds)
		m_iChannels = std::max(m_iChannels, it->GetNumChannels());

	/*
	 * We might get different sample rates from our sources.  If they're all the
	 * same sample rate, just leave it alone, so the whole sound can be
	 * resampled as a group. If not, resample eveything to the preferred rate.
	 * (Using the preferred rate should avoid redundant resampling later.)
	 */
	m_iSampleRate = GetSampleRateInternal();
	if (m_iSampleRate == -1) {
		for (auto& it : m_aSounds) {
			auto& pSound = it;

			auto pResample =
			  new RageSoundReader_Resample_Good(pSound, iPreferredSampleRate);
			pSound = pResample;
		}

		m_iSampleRate = iPreferredSampleRate;
	}

	/* If we have two channels, and any sounds have only one, convert them by
	 * adding a Pan filter. */
	for (auto& it : m_aSounds) {
		{
			if (it->GetNumChannels() != this->GetNumChannels())
				it = new RageSoundReader_Pan(it);
		}

		/* If we have more than two channels, then all sounds must have the same
		 * number of channels. */
		if (m_iChannels > 2) {
			vector<RageSoundReader*> aSounds;
			for (auto& it : m_aSounds) {
				if (it->GetNumChannels() != m_iChannels) {
					Locator::getLogger()->warn("Discarded sound with {} channels, not {}",
						  it->GetNumChannels(),
						  m_iChannels);
					delete it;
					it = nullptr;
				} else {
					aSounds.push_back(it);
				}

				m_aSounds = aSounds;
			}
		}
	}
}

int
RageSoundReader_Merge::SetPosition(int iFrame)
{
	m_iNextSourceFrame = iFrame;

	auto iRet = 0;
	for (auto pSound : m_aSounds) {
		auto iThisRet = pSound->SetPosition(iFrame);
		if (iThisRet == -1)
			return -1;
		if (iThisRet > 0)
			iRet = 1;
	}

	return iRet;
}

bool
RageSoundReader_Merge::SetProperty(const std::string& sProperty, float fValue)
{
	auto bRet = false;
	for (auto& m_aSound : m_aSounds) {
		if (m_aSound->SetProperty(sProperty, fValue))
			bRet = true;
	}

	return bRet;
}

static float
Difference(float a, float b)
{
	return fabsf(a - b);
}
static int
Difference(int a, int b)
{
	return abs(a - b);
}

/*
 * If the audio position drifts apart further than
 * ERROR_CORRECTION_THRESHOLD frames, attempt to resync it.
 *
 * Frames are expressed as whole numbers, and the ratio between source
 * and stream frames is floating point.  We can't read a specific number
 * of source frames, only stream frames.  If a stream is early by 15
 * source frames, we'll convert that to stream frames for reading; this
 * rounds back to an integer, so it isn't exact.  (The amount of error
 * should be no more than the ratio; if we have a ratio of 10, then
 * reading 10 stream frames should advance the stream by 100-110 frames.
 * The ratio is normally less than 5.)
 *
 * ERROR_CORRECTION_THRESHOLD should be greater than the maximum rate in
 * use, so we can always resync the stream back to within the tolerance
 * of the threshold.
 *
 * In the pathological case, if this is too low we may never resync
 * properly, each attempt to resync leapfrogging past the previous.
 */

static const int ERROR_CORRECTION_THRESHOLD = 16;

/* As we iterate through the sound tree, we'll find that we need data
 * from different sounds; a sound may be needed by more than one other
 * sound. */
int
RageSoundReader_Merge::Read(float* pBuffer, int iFrames)
{
	if (m_aSounds.empty())
		return END_OF_FILE;

	/*
	 * All sounds which are active should stay aligned; each
	 * GetNextSourceFrame should not come out of sync.  Accomodate small
	 * rounding errors.  A larger inconsistency happens may be a bug,
	 * such as sounds at different speeds.
	 */

	vector<int> aNextSourceFrames;
	vector<float> aRatios;
	aNextSourceFrames.resize(m_aSounds.size());
	aRatios.resize(m_aSounds.size());
	for (unsigned i = 0; i < m_aSounds.size(); ++i) {
		aNextSourceFrames[i] = m_aSounds[i]->GetNextSourceFrame();
		aRatios[i] = m_aSounds[i]->GetStreamToSourceRatio();
	}

	{
		/* GetNextSourceFrame for each active sound should be the same.
		 * If any differ, delay the later sounds until the earlier ones
		 * catch back up to put them back in sync. */
		int iEarliestSound = distance(
		  aNextSourceFrames.begin(),
		  min_element(aNextSourceFrames.begin(), aNextSourceFrames.end()));

		/* Normally, m_iNextSourceFrame should already be aligned with
		 * the GetNextSourceFrame of our sounds.  If it's not, adjust it
		 * and return.
		 */
		if (m_iNextSourceFrame != aNextSourceFrames[iEarliestSound] ||
			m_fCurrentStreamToSourceRatio != aRatios[iEarliestSound]) {
			m_iNextSourceFrame = aNextSourceFrames[iEarliestSound];
			m_fCurrentStreamToSourceRatio = aRatios[iEarliestSound];
			return 0;
		}

		auto iMinPosition = aNextSourceFrames[iEarliestSound];
		for (unsigned i = 0; i < m_aSounds.size(); ++i) {
			if (Difference(aNextSourceFrames[i], iMinPosition) <=
				ERROR_CORRECTION_THRESHOLD)
				continue;

			/* A sound is being delayed to resync it; clamp the number
			 * of frames we read now, so we don't advance past it. */
			auto iMaxSourceFramesToRead = aNextSourceFrames[i] - iMinPosition;
			int iMaxStreamFramesToRead =
			  lround(iMaxSourceFramesToRead / m_fCurrentStreamToSourceRatio);
			iFrames = std::min(iFrames, iMaxStreamFramesToRead);
			//			LOG->Warn( "RageSoundReader_Merge: sound
			// positions
			// moving  at  different rates" );
		}
	}

	if (m_aSounds.size() == 1) {
		/* We have only one source; read directly into the buffer. */
		auto pSound = m_aSounds.front();
		iFrames = pSound->Read(pBuffer, iFrames);
		if (iFrames > 0)
			m_iNextSourceFrame +=
			  lround(iFrames * m_fCurrentStreamToSourceRatio);
		aNextSourceFrames.front() = pSound->GetNextSourceFrame();
		aRatios.front() = pSound->GetStreamToSourceRatio();
		return iFrames;
	}

	RageSoundMixBuffer mix;
	float Buffer[2048];
	iFrames =
	  std::min(iFrames, static_cast<int>((ARRAYLEN(Buffer) / m_iChannels)));

	/* Read iFrames from each sound. */
	for (unsigned i = 0; i < m_aSounds.size(); ++i) {
		auto pSound = m_aSounds[i];
		ASSERT(pSound->GetNumChannels() == m_iChannels);

		auto iFramesRead = 0;
		while (iFramesRead < iFrames) {
			//			if( i == 0 )
			// LOG->Trace( "*** %i", Difference(aNextSourceFrames[i],
			// m_iNextSourceFrame + lround(iFramesRead * aRatios[i])) );

			if (Difference(aNextSourceFrames[i],
						   m_iNextSourceFrame +
							 lround(iFramesRead * aRatios[i])) >
				ERROR_CORRECTION_THRESHOLD) {
				Locator::getLogger()->trace("*** hurk {}",
						   Difference(aNextSourceFrames[i],
									  m_iNextSourceFrame +
										lround(iFramesRead * aRatios[i])));
				break;
			}

			auto iGotFrames = pSound->Read(Buffer, iFrames - iFramesRead);
			if (0 && /*i == 1 && */ iGotFrames > 0) {
				int iAt =
				  aNextSourceFrames[i] + lround(iGotFrames * aRatios[i]);
				if (iAt != m_aSounds[i]->GetNextSourceFrame())
					Locator::getLogger()->trace("{}: at {}, expected {}",
							   i,
							   iAt,
							   m_aSounds[i]->GetNextSourceFrame());
			}
			aNextSourceFrames[i] = m_aSounds[i]->GetNextSourceFrame();
			aRatios[i] = m_aSounds[i]->GetStreamToSourceRatio();
			//	LOG->Trace( "read %i from %i; %i -> %i", iGotFrames, i,
			// oldf,
			// aNextSourceFrames[i] );
			if (iGotFrames < 0) {
				if (i == 0)
					return iGotFrames;
				break;
			}

			mix.SetWriteOffset(iFramesRead * pSound->GetNumChannels());
			mix.write(Buffer, iGotFrames * pSound->GetNumChannels());
			iFramesRead += iGotFrames;

			if (Difference(aRatios[i], m_fCurrentStreamToSourceRatio) > 0.001f)
				break;
		}
	}

	/* Read mixed frames into the output buffer. */
	int iMaxFramesRead = mix.size() / m_iChannels;
	mix.read(pBuffer);

	m_iNextSourceFrame +=
	  lround(iMaxFramesRead * m_fCurrentStreamToSourceRatio);

	return iMaxFramesRead;
}

int
RageSoundReader_Merge::GetLength() const
{
	auto iLength = 0;
	for (auto m_aSound : m_aSounds)
		iLength = std::max(iLength, m_aSound->GetLength());
	return iLength;
}

int
RageSoundReader_Merge::GetLength_Fast() const
{
	auto iLength = 0;
	for (auto m_aSound : m_aSounds)
		iLength = std::max(iLength, m_aSound->GetLength_Fast());
	return iLength;
}
