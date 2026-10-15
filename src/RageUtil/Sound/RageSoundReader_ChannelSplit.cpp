/*
 * Output a sound from the channels of another sound.
 *
 * This is intended for splitting a FileReader, and assumes the
 * GetStreamToSourceRatio of the source is 1.0.
 *
 * If multiple readers are created, they should be read in parallel.  If their
 * source position drifts apart, the data between will be buffered to prevent
 * seeking the source.
 *
 * The resulting sounds have as many channels as the largest destination channel
 * specified; multiple sounds on the same channel are mixed together; empty
 * channels are silent.
 *
 * Create two mono sounds from one stereo sound, one playing the left channel
 * and one playing the right: RageSoundSplitter split(pSource);
 *   RageSoundReader_Split *pLeft = split.CreateSound();
 *   pLeft->AddSourceChannelToSound( 0, 0 );
 *   RageSoundReader_Split *pRight = split.CreateSound();
 *   pRight->AddSourceChannelToSound( 1, 0 );
 *
 * Convert a stereo sound to mono:
 *   RageSoundSplitter split(pSource);
 *   RageSoundReader_Split *pMono = split.CreateSound();
 *   pMono->AddSourceChannelToSound( 0, 0 );
 *   pMono->AddSourceChannelToSound( 1, 0 );
 *
 * Convert a mono sound to stereo (RageSoundReader_Pan will do this faster):
 *   RageSoundSplitter split(pSource);
 *   RageSoundReader_Split *pStereo = split.CreateSound();
 *   pStereo->AddSourceChannelToSound( 0, 0 );
 *   pStereo->AddSourceChannelToSound( 0, 1 );
 */

#include "Etterna/Globals/global.h"
#include "RageSoundMixBuffer.h"
#include "RageSoundReader_ChannelSplit.h"
#include "RageUtil/Utils/RageUtil.h"

#include <climits>
#include <limits>
#include <set>
#include <algorithm>

class RageSoundReader_Split;

/* This class is refcounted, freed when all RageSoundReader_Split and
 * RageSoundSplitter classes release it. */
class RageSoundSplitterImpl
{
  public:
	explicit RageSoundSplitterImpl(RageSoundReader* pSource)
	{
		m_iRefCount = 1;
		m_pSource = pSource;
		m_iBufferPositionFrames = 0;
	}

	~RageSoundSplitterImpl() { delete m_pSource; }

	static void Release(RageSoundSplitterImpl* pImpl)
	{
		--pImpl->m_iRefCount;
		if (pImpl->m_iRefCount == 0)
			delete pImpl;
	}

	/* Request that m_sBuffer contain frames [iStartFrame,iStartFrame+iFrames).
	 */
	int ReadBuffer();
	int m_iRefCount;

	RageSoundReader* m_pSource;

	std::set<RageSoundReader_Split*> m_apSounds;

	/* m_sBuffer[0] corresponds to frame number m_iBufferPositionFrames. */
	int m_iBufferPositionFrames;
	std::vector<float> m_sBuffer;
};

int
RageSoundReader_Split::GetLength() const
{
	return m_pImpl->m_pSource->GetLength();
}
int
RageSoundReader_Split::GetLength_Fast() const
{
	return m_pImpl->m_pSource->GetLength_Fast();
}
int
RageSoundReader_Split::GetSampleRate() const
{
	return m_pImpl->m_pSource->GetSampleRate();
}
unsigned
RageSoundReader_Split::GetNumChannels() const
{
	return m_iNumOutputChannels;
}
int64_t
RageSoundReader_Split::GetNextSourceFrame() const
{
	return m_iPositionFrame;
}
float
RageSoundReader_Split::GetStreamToSourceRatio() const
{
	return 1.0f;
}
std::string
RageSoundReader_Split::GetError() const
{
	return m_pImpl->m_pSource->GetError();
}

RageSoundReader_Split::RageSoundReader_Split(RageSoundSplitterImpl* pImpl)
{
	m_pImpl = pImpl;
	++m_pImpl->m_iRefCount;
	m_pImpl->m_apSounds.insert(this);
	m_iNumOutputChannels = 0;
	m_iPositionFrame = 0;
	m_iRequestFrames = 0;
}

RageSoundReader_Split::RageSoundReader_Split(const RageSoundReader_Split& cpy)
  : RageSoundReader(cpy)
{
	m_pImpl = cpy.m_pImpl;
	++m_pImpl->m_iRefCount;
	m_pImpl->m_apSounds.insert(this);
	m_aChannels = cpy.m_aChannels;
	m_iNumOutputChannels = cpy.m_iNumOutputChannels;
	m_iPositionFrame = cpy.m_iPositionFrame;
	m_iRequestFrames = cpy.m_iRequestFrames;
}

RageSoundReader_Split::~RageSoundReader_Split()
{
	m_pImpl->m_apSounds.erase(this);
	RageSoundSplitterImpl::Release(m_pImpl);
}

int
RageSoundReader_Split::SetPosition(int64_t iFrame)
{
	m_iPositionFrame = iFrame;

	/* We can't tell whether we're past EOF.  We can't ReadBuffer here, because
	 * the other sounds using this same source probably haven't seeked yet, so
	 * seeking to the beginning of the file would buffer between there and the
	 * position of those sounds.  Just return success, and we'll return EOF in
	 * Read if needed. */
	return 1;
}

bool
RageSoundReader_Split::SetProperty(const std::string& sProperty, float fValue)
{
	return m_pImpl->m_pSource->SetProperty(sProperty, fValue);
}

int64_t
RageSoundReader_Split::Read(float* pBuf, int64_t iFrames)
{
	m_iRequestFrames = iFrames;
	int iRet = m_pImpl->ReadBuffer();

	int64_t iSamplesAvailable = static_cast<int64_t>(m_pImpl->m_sBuffer.size());
	const float* pSrc = &m_pImpl->m_sBuffer[0];
	if (m_pImpl->m_iBufferPositionFrames < m_iPositionFrame) {
		int64_t iSkipFrames = m_iPositionFrame - m_pImpl->m_iBufferPositionFrames;
		int64_t iSkipSamples = iSkipFrames * m_pImpl->m_pSource->GetNumChannels();
		pSrc += iSkipSamples;
		iSamplesAvailable -= iSkipSamples;
	}

	int64_t iFramesWanted = iFrames;
	int64_t iFramesAvailable =
	  iSamplesAvailable / (m_pImpl->m_pSource->GetNumChannels());

	/* Report any errors from Read() if we don't have any data buffered to
	 * return.  If we do have data, finish returning it first. */
	if (iFramesAvailable == 0 && iRet < 0)
		return iRet;

	iFramesAvailable = std::min(iFramesAvailable, iFramesWanted);

	{
		RageSoundMixBuffer mix;
		for (int i = 0; i < static_cast<int>(m_aChannels.size()); ++i) {
			const ChannelMap& chan = m_aChannels[i];
			mix.SetWriteOffset(chan.m_iToChannel);
			mix.write(pSrc + chan.m_iFromChannel,
					  iFramesAvailable,
					  m_pImpl->m_pSource->GetNumChannels(),
					  m_iNumOutputChannels);
		}

		mix.read(pBuf);
	}

	m_iPositionFrame += iFramesAvailable;

	/* We no longer need the data we requested.  Clear our request, so the
	 * memory can be freed. */
	m_iRequestFrames = 0;
	m_pImpl->ReadBuffer();
	return iFramesAvailable;
}

int
RageSoundSplitterImpl::ReadBuffer()
{
	/* Discard any bytes that are no longer requested by any sound. */
	int64_t iMinFrameRequested = std::numeric_limits<int64_t>::max();
	int64_t iMaxFrameRequested = std::numeric_limits<int64_t>::min();
	for (auto& snd : m_apSounds) {
		iMinFrameRequested =
		  std::min(iMinFrameRequested, snd->m_iPositionFrame);
		iMaxFrameRequested = std::max(
		  iMaxFrameRequested, snd->m_iPositionFrame + snd->m_iRequestFrames);
	}

	if (iMinFrameRequested > m_iBufferPositionFrames) {
		int64_t iEraseFrames = iMinFrameRequested - m_iBufferPositionFrames;
		iEraseFrames =
		  std::min(iEraseFrames, static_cast<int64_t>(m_sBuffer.size()));
		m_sBuffer.erase(m_sBuffer.begin(),
						m_sBuffer.begin() +
						  iEraseFrames * m_pSource->GetNumChannels());
		m_iBufferPositionFrames += iEraseFrames;
	}

	if (iMinFrameRequested != m_iBufferPositionFrames) {
		m_pSource->SetPosition(iMinFrameRequested);
		m_iBufferPositionFrames = iMinFrameRequested;
		m_sBuffer.clear();
	}

	int64_t iFramesBuffered = static_cast<int64_t>(m_sBuffer.size()) / m_pSource->GetNumChannels();

	int64_t iFramesToRead =
	  iMaxFrameRequested - (m_iBufferPositionFrames + iFramesBuffered);
	if (iFramesToRead <= 0)
		return 1; // requested data already buffered

	int64_t iSamplesToRead = iFramesToRead * m_pSource->GetNumChannels();
	int64_t iOldSizeSamples = static_cast<int64_t>(m_sBuffer.size());
	m_sBuffer.resize(static_cast<size_t>(iOldSizeSamples + iSamplesToRead));
	int64_t iGotFrames =
	  m_pSource->Read(&m_sBuffer[0] + iOldSizeSamples, iFramesToRead);
	if (iGotFrames < 0) {
		m_sBuffer.resize(iOldSizeSamples);
		return iGotFrames;
	}

	int64_t iGotSamples = iGotFrames * m_pSource->GetNumChannels();
	m_sBuffer.resize(static_cast<size_t>(iOldSizeSamples + iGotSamples));
	return 1;
}

void
RageSoundReader_Split::AddSourceChannelToSound(int iFromChannel, int iToChannel)
{
	m_aChannels.push_back(ChannelMap(iFromChannel, iToChannel));
	m_iNumOutputChannels = std::max(m_iNumOutputChannels, iToChannel + 1);
}

RageSoundSplitter::RageSoundSplitter(RageSoundReader* pSource)
{
	m_pImpl = new RageSoundSplitterImpl(pSource);
}

RageSoundSplitter::~RageSoundSplitter()
{
	RageSoundSplitterImpl::Release(m_pImpl);
}

RageSoundReader_Split*
RageSoundSplitter::CreateSound()
{
	return new RageSoundReader_Split(m_pImpl);
}
