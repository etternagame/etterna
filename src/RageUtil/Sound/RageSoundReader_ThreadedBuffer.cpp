#include "Etterna/Globals/global.h"
#include "RageSoundReader_ThreadedBuffer.h"
#include "RageUtil/Misc/RageTimer.h"

#include <algorithm>

#if defined(HAVE_UNISTD_H)
#include <unistd.h> /* correct place with correct definitions */
#endif

/* Implement threaded read-ahead buffering.
 *
 * If a buffer is low on data, keep filling until it has a g_iMinFillFrames.
 * Once beyond that, fill at a rate relative to realtime.
 *
 * This allows a stream to have a large buffer, for higher reliability, without
 * causing major CPU bursts when the stream starts or underruns. Filling 32k
 * takes more CPU than filling 4k frames, and may cause a skip. */

// The amount of data to read at once:
static const unsigned g_iReadBlockSizeFrames = 1024;

// The maximum number of frames to buffer:
static const int g_iStreamingBufferFrames = 1024 * 32;

/* When a sound has fewer than g_iMinFillFrames buffered, buffer at maximum
 * speed. Once beyond that, fill at a limited rate. */
static const int g_iMinFillFrames = 1024 * 4;

RageSoundReader_ThreadedBuffer::RageSoundReader_ThreadedBuffer(
  RageSoundReader* pSource)
  : RageSoundReader_Filter(pSource)
  , m_Event("ThreadedBuffer")
{
	m_iSampleRate = pSource->GetSampleRate();
	m_iChannels = pSource->GetNumChannels();

	int iFrameSize = sizeof(float) * this->GetNumChannels();
	m_DataBuffer.reserve(g_iStreamingBufferFrames * iFrameSize, iFrameSize);

	m_bEOF = false;
	m_bShutdownThread = false;
	m_bEnabled = false;
	m_bFilling = false;

	m_StreamPosition.push_back(Mapping());
	m_StreamPosition.back().iPositionOfFirstFrame =
	  pSource->GetNextSourceFrame();
	m_StreamPosition.back().fRate = pSource->GetStreamToSourceRatio();

	m_Thread.SetName("Buffered Audio Streamer");
	m_Thread.Create(StartBufferingThread, this);
}

RageSoundReader_ThreadedBuffer::RageSoundReader_ThreadedBuffer(
  const RageSoundReader_ThreadedBuffer& cpy)
  : RageSoundReader_Filter(NULL)
  , // don't touch m_pSource before DisableBuffering
  m_Event("ThreadedBuffer")
{
	bool bWasEnabled = cpy.DisableBuffering();

	m_pSource = cpy.m_pSource->Copy();
	m_iSampleRate = cpy.m_iSampleRate;
	m_iChannels = cpy.m_iChannels;
	m_DataBuffer = cpy.m_DataBuffer;
	m_bEOF = cpy.m_bEOF;
	m_bShutdownThread = cpy.m_bShutdownThread;
	m_bEnabled = cpy.m_bEnabled;
	m_bFilling = cpy.m_bFilling;

	m_StreamPosition = cpy.m_StreamPosition;

	m_Thread.Create(StartBufferingThread, this);

	if (bWasEnabled) {
		cpy.EnableBuffering();
		EnableBuffering();
	}
}

RageSoundReader_ThreadedBuffer::~RageSoundReader_ThreadedBuffer()
{
	DisableBuffering();
	m_Event.Lock();
	m_bShutdownThread = true;
	m_Event.Broadcast();
	m_Event.Unlock();

	m_Thread.Wait();
}

void
RageSoundReader_ThreadedBuffer::EnableBuffering()
{
	m_Event.Lock();
	m_bEnabled = true;
	m_Event.Broadcast();
	m_Event.Unlock();
}

bool
RageSoundReader_ThreadedBuffer::DisableBuffering()
{
	m_Event.Lock();
	bool bRet = m_bEnabled;
	m_bEnabled = false;
	m_Event.Broadcast();

	while (m_bFilling)
		m_Event.Wait();

	m_Event.Unlock();

	return bRet;
}

void
RageSoundReader_ThreadedBuffer::WaitUntilFrames(int iWaitUntilFrames)
{
	m_Event.Lock();
	ASSERT(m_bEnabled);
	while (GetFilledFrames() < iWaitUntilFrames)
		m_Event.Wait();
	m_Event.Unlock();
}

int
RageSoundReader_ThreadedBuffer::SetPosition(int iFrame)
{
	bool bWasEnabled = DisableBuffering();

	m_DataBuffer.clear();

	int iRet = RageSoundReader_Filter::SetPosition(iFrame);

	m_StreamPosition.clear();
	m_StreamPosition.push_back(Mapping());
	m_StreamPosition.back().iPositionOfFirstFrame = iFrame;

	m_bEOF = iRet == 0;

	if (bWasEnabled)
		EnableBuffering();

	return iRet;
}

int
RageSoundReader_ThreadedBuffer::GetEmptyFrames() const
{
	int iSamplesPerFrame = this->GetNumChannels();
	if (g_iReadBlockSizeFrames * iSamplesPerFrame > m_DataBuffer.num_writable())
		return 0;
	return m_DataBuffer.num_writable() / iSamplesPerFrame;
}

int
RageSoundReader_ThreadedBuffer::GetFilledFrames() const
{
	int iSamplesPerFrame = this->GetNumChannels();
	return m_DataBuffer.num_readable() / iSamplesPerFrame;
}

int
RageSoundReader_ThreadedBuffer::GetNextSourceFrame() const
{
	m_Event.Lock();
	int iRet = m_StreamPosition.front().iPositionOfFirstFrame;
	m_Event.Unlock();
	return iRet;
}

float
RageSoundReader_ThreadedBuffer::GetStreamToSourceRatio() const
{
	m_Event.Lock();
	float fRet = m_StreamPosition.front().fRate;
	m_Event.Unlock();
	return fRet;
}

int
RageSoundReader_ThreadedBuffer::GetLength() const
{
	bool bWasEnabled = DisableBuffering();
	int iRet = m_pSource->GetLength();
	if (bWasEnabled)
		EnableBuffering();
	return iRet;
}
int
RageSoundReader_ThreadedBuffer::GetLength_Fast() const
{
	bool bWasEnabled = DisableBuffering();
	int iRet = m_pSource->GetLength_Fast();
	if (bWasEnabled)
		EnableBuffering();
	return iRet;
}

bool
RageSoundReader_ThreadedBuffer::SetProperty(const std::string& sProperty,
											float fValue)
{
	return m_pSource->SetProperty(sProperty, fValue);
}

void
RageSoundReader_ThreadedBuffer::BufferingThread()
{
	m_Event.Lock();
	while (!m_bShutdownThread) {
		if (!m_bEnabled) {
			m_Event.Wait();
			continue;
		}

		// Fill some data.
		m_bFilling = true;

		int iFramesToFill = g_iReadBlockSizeFrames;
		if (GetFilledFrames() < g_iMinFillFrames)
			iFramesToFill =
			  std::max(iFramesToFill, g_iMinFillFrames - GetFilledFrames());

		int iRet = FillFrames(iFramesToFill);

		// Release m_bFilling, and signal the event to wake anyone waiting for
		// it.
		m_bFilling = false;
		m_Event.Broadcast();

		// On error or end of file, stop buffering the sound.
		if (iRet < 0) {
			m_bEnabled = false;
			continue;
		}

		/* Sleep proportionately to the amount of data we buffered, so we
		 * fill at a reasonable pace. */
		float fTimeFilled =
		  static_cast<float>(g_iReadBlockSizeFrames) / m_iSampleRate;
		float fTimeToSleep = fTimeFilled / 2;
		if (fTimeToSleep == 0)
			fTimeToSleep =
			  static_cast<float>(g_iReadBlockSizeFrames) / m_iSampleRate;

		if (m_Event.WaitTimeoutSupported()) {
			m_Event.Wait(fTimeToSleep);
		} else {
			m_Event.Unlock();
			usleep(lrintf(fTimeToSleep * 1000000));
			m_Event.Lock();
		}
	}
	m_Event.Unlock();
}

int
RageSoundReader_ThreadedBuffer::FillFrames(int iFrames)
{
	int iFramesFilled = 0;
	while (iFramesFilled < iFrames) {
		int iRet = FillBlock();

		if (iRet == 0)
			break;
		// On error or end of file, stop buffering the sound.
		if (iRet < 0)
			return iRet;

		iFramesFilled += iRet;
	}
	return iFramesFilled;
}

int
RageSoundReader_ThreadedBuffer::FillBlock()
{
	if (GetEmptyFrames() == 0)
		return 0;

	int iSamplesPerFrame = this->GetNumChannels();
	ASSERT(g_iReadBlockSizeFrames * iSamplesPerFrame <=
		   m_DataBuffer.num_writable());

	int iGotFrames;
	int iNextSourceFrame = 0;
	float fRate = 0;

	m_Event.Unlock();

	{
		// We own m_pSource, even after unlocking, because m_bFilling is true.
		unsigned iBufSize;
		float* pBuf = m_DataBuffer.get_write_pointer(&iBufSize);
		ASSERT((iBufSize % iSamplesPerFrame) == 0);
		iGotFrames = m_pSource->RetriedRead(
		  pBuf,
		  std::min(g_iReadBlockSizeFrames, iBufSize / iSamplesPerFrame),
		  &iNextSourceFrame,
		  &fRate);
	}

	m_Event.Lock();

	if (iGotFrames > 0) {
		// Add the data to the buffer.
		m_DataBuffer.advance_write_pointer(iGotFrames * iSamplesPerFrame);
		if (iNextSourceFrame != m_StreamPosition.back().iPositionOfFirstFrame +
								  m_StreamPosition.back().iFramesBuffered ||
			fRate != m_StreamPosition.back().fRate) {
			m_StreamPosition.push_back(Mapping());
			m_StreamPosition.back().iPositionOfFirstFrame = iNextSourceFrame;
			m_StreamPosition.back().fRate = fRate;
		}

		m_StreamPosition.back().iFramesBuffered += iGotFrames;
	}

	m_bEOF = (iGotFrames == END_OF_FILE);

	return iGotFrames;
}

int
RageSoundReader_ThreadedBuffer::Read(float* pBuffer, int iFrames)
{
	if (!m_bEOF)
		EnableBuffering();

	m_Event.Lock();

	{
		/* Delete any empty mappings from the beginning, but don't empty the
		 * list, so we always have the current position and rate. If we delete
		 * an item, the rate or position has probably changed, so return. */
		std::list<Mapping>::iterator it = m_StreamPosition.begin();
		++it;
		if (it != m_StreamPosition.end() &&
			!m_StreamPosition.front().iFramesBuffered) {
			++it;
			m_StreamPosition.pop_front();
			m_Event.Unlock();
			return 0;
		}
	}

	int iRet;
	if (m_StreamPosition.front().iFramesBuffered) {
		Mapping& pos = m_StreamPosition.front();
		int iFramesToRead = std::min(iFrames, pos.iFramesBuffered);
		int iSamplesPerFrame = this->GetNumChannels();
		m_DataBuffer.read(pBuffer, iFramesToRead * iSamplesPerFrame);
		pos.iPositionOfFirstFrame += iFramesToRead;
		pos.iFramesBuffered -= iFramesToRead;
		iRet = iFramesToRead;
	} else if (m_bEOF)
		iRet = END_OF_FILE;
	else
		iRet = WOULD_BLOCK;
	m_Event.Unlock();

	return iRet;
}
