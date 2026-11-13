/* This reader simply precaches all of the data from another reader. This
 * reduces CPU usage for sounds that are played several times at once. */

#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Preference.h"
#include "RageSoundReader_Preload.h"
#include "RageSoundUtil.h"
#include "RageUtil/Utils/RageUtil.h"

#include <algorithm>

/* If true, preloaded sounds are stored in 16-bit instead of floats.  Most
 * processing happens after preloading, and it's usually a waste to store high-
 * resolution data for sound effects. */
Preference<bool> g_bSoundPreload16bit("SoundPreload16bit", true);

/* If a sound is smaller than this, we'll load it entirely into memory. */
Preference<int> g_iSoundPreloadMaxSamples("SoundPreloadMaxSamples",
										  1024 * 1024);

#define samplesize (m_bBufferIs16Bit ? sizeof(int16_t) : sizeof(float))
#define framesize (samplesize * m_iChannels)

bool
RageSoundReader_Preload::PreloadSound(RageSoundReader*& pSound)
{
	auto* pPreload = new RageSoundReader_Preload;
	if (!pPreload->Open(pSound)) {
		/* Preload failed.  It read some data, so we need to rewind the reader.
		 */
		pSound->SetPosition(0);
		delete pPreload;
		return false;
	}

	pSound = pPreload;
	return true;
}

RageSoundReader_Preload::RageSoundReader_Preload()
  : m_Buffer(new std::string)
{
	m_bBufferIs16Bit = g_bSoundPreload16bit.Get();
}

int64_t
RageSoundReader_Preload::GetTotalFrames() const
{
	return static_cast<int64_t>(m_Buffer->size()) / framesize;
}

bool
RageSoundReader_Preload::Open(RageSoundReader* pSource)
{
	ASSERT(pSource != NULL);
	m_iSampleRate = pSource->GetSampleRate();
	m_iChannels = pSource->GetNumChannels();
	m_fRate = pSource->GetStreamToSourceRatio();

	int iMaxSamples = g_iSoundPreloadMaxSamples.Get();

	if (m_Buffer.use_count() != 1)
		m_Buffer = std::make_shared<std::string>(*m_Buffer);

	/* Check the length, and see if we think it'll fit in the buffer. */
	int iLen = pSource->GetLength_Fast();
	if (iLen != -1) {
		float fSecs = iLen / 1000.f;

		int64_t iFrames = lround(fSecs * m_iSampleRate);	/* seconds -> frames */
		int64_t iSamples = int64_t(iFrames) * m_iChannels; /* frames -> samples */
		if (iSamples > iMaxSamples)
			return false; /* Don't bother trying to preload it. */

		int64_t iBytes = iSamples * samplesize; /* samples -> bytes */
		m_Buffer->reserve(iBytes);
	}

	for (;;) {
		/* If the rate changes, we won't preload it. */
		if (pSource->GetStreamToSourceRatio() != m_fRate) {
			return false; /* Don't bother trying to preload it. */
		}
		float buffer[1024];
		int64_t iCnt = pSource->Read(buffer, ARRAYLEN(buffer) / m_iChannels);

		if (iCnt == END_OF_FILE) {
			break;
		}
		if (iCnt < 0) {
			return false;
		}
		/* Add the buffer. */
		if (m_bBufferIs16Bit) {
			int16_t buffer16[1024];
			RageSoundUtil::ConvertFloatToNativeInt16(
			  buffer, buffer16, iCnt * m_iChannels);
			m_Buffer->append((char*)buffer16,
								   (char*)(buffer16 + iCnt * m_iChannels));
		} else {
			m_Buffer->append((char*)buffer,
								   (char*)(buffer + iCnt * m_iChannels));
		}

		if (m_Buffer->size() > iMaxSamples * samplesize) {
			return false; /* too big */
		}
	}

	m_iPosition = 0;
	delete pSource;
	return true;
}

int
RageSoundReader_Preload::GetLength() const
{
	return int(float(GetTotalFrames()) * 1000.f / m_iSampleRate);
}

int
RageSoundReader_Preload::GetLength_Fast() const
{
	return GetLength();
}

int
RageSoundReader_Preload::SetPosition(int64_t iFrame)
{
	m_iPosition = iFrame;
	m_iPosition = llround(static_cast<double>(m_iPosition) / m_fRate);

	if (m_iPosition >= static_cast<int64_t>(m_Buffer->size()) / framesize) {
		m_iPosition = static_cast<int64_t>(m_Buffer->size()) / framesize;
		return 0;
	}

	return 1;
}

int64_t
RageSoundReader_Preload::GetNextSourceFrame() const
{
	return llround(static_cast<double>(m_iPosition) * m_fRate);
}

int64_t
RageSoundReader_Preload::Read(float* pBuffer, int64_t iFrames)
{
	const int64_t iSizeFrames = m_Buffer->size() / framesize;
	const int64_t iFramesAvail = iSizeFrames - m_iPosition;

	iFrames = std::min(iFrames, iFramesAvail);
	if (iFrames == 0)
		return END_OF_FILE;
	if (m_bBufferIs16Bit) {
		const int16_t* pIn =
		  (const int16_t*)(m_Buffer->data() + (m_iPosition * framesize));
		RageSoundUtil::ConvertNativeInt16ToFloat(
		  pIn, pBuffer, iFrames * m_iChannels);
	} else {
		memcpy(pBuffer,
			   m_Buffer->data() + (m_iPosition * framesize),
			   iFrames * framesize);
	}
	m_iPosition += iFrames;

	return iFrames;
}

RageSoundReader_Preload*
RageSoundReader_Preload::Copy() const
{
	return new RageSoundReader_Preload(*this);
}

int
RageSoundReader_Preload::GetReferenceCount() const
{
	return m_Buffer.use_count();
}
