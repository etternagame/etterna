#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageSoundReader_SpeedChange.h"
#include "RageUtil/Utils/RageUtil.h"

#include <algorithm>

RageSoundReader_SpeedChange::RageSoundReader_SpeedChange(
  RageSoundReader* pSource, bool bStepMania)
  : RageSoundReader_Filter(pSource)
  , m_iWindowSize(bStepMania ? 30 : 70)
  , m_bMidSideEncoding(!bStepMania)
{
	m_Channels.resize(pSource->GetNumChannels());
	m_fSpeedRatio = m_fTrailingSpeedRatio = 1.0f;
	m_iDataBufferAvailFrames = 0;
	SetSpeedRatio(1.0f);
	Reset();
}

void
RageSoundReader_SpeedChange::SetSpeedRatio(float fRatio)
{
	m_fSpeedRatio = fRatio;

	/* If we haven't read any data yet, put the new delta into effect
	 * immediately. */
	if (m_iDataBufferAvailFrames == 0)
		m_fTrailingSpeedRatio = m_fSpeedRatio;
}

int
RageSoundReader_SpeedChange::GetWindowSizeFrames() const
{
	return (m_iWindowSize * GetSampleRate()) / 1000;
}

void
RageSoundReader_SpeedChange::Reset()
{
	m_fTrailingSpeedRatio = m_fSpeedRatio;
	m_iDataBufferAvailFrames = 0;
	for (size_t i = 0; i < m_Channels.size(); ++i) {
		ChannelInfo& c = m_Channels[i];
		c.m_iCorrelatedPos = 0;
		c.m_iLastCorrelatedPos = 0;
	}
	m_iUncorrelatedPos = 0;
	m_iPos = 0;
	m_fErrorFrames = 0;
}

static int
FindClosestMatch(const float* pBuffer,
				 int iBufferSize,
				 const float* pCorrelateBuffer,
				 int iCorrelateBufferSize,
				 int iStride)
{
	if (iBufferSize <= iCorrelateBufferSize)
		return 0;

	/* Optimization: if the buffers are the same, they always corellate at 0. */
	if (pBuffer == pCorrelateBuffer)
		return 0;

	int iBufferDistanceToSearch = iBufferSize - iCorrelateBufferSize;
	int iBestOffset = 0;
	float fBestScore = 0;
	for (int i = 0; i < iBufferDistanceToSearch; i += iStride) {
		float fScore = 0;
		const float* pFrames = pBuffer + i;
		for (int j = 0; j < iCorrelateBufferSize; j += iStride) {
			float fDiff = pFrames[j] - pCorrelateBuffer[j];
			fScore += fabsf(fDiff);
		}

		if (i == 0 || fScore < fBestScore) {
			fBestScore = fScore;
			iBestOffset = i;
		}
	}
	return iBestOffset;
}

int
RageSoundReader_SpeedChange::FillData(int iMaxFrames)
{
	/* XXX: If the rate or source frame offset changes in the source, we should
	 * stop at that point, so the changes propagate upward.  That's tricky,
	 * since we want to process a block at a time.  We could just flush what we
	 * have, and maybe tweak m_iDeltaFrames for the next block based on how much
	 * we processed, so we average out the short block. */
	while (iMaxFrames > 0) {
		int iFramesToRead = iMaxFrames - m_iDataBufferAvailFrames;
		int iBytesToRead = iFramesToRead * m_Channels.size() * sizeof(float);
		if (iBytesToRead <= 0)
			return m_iDataBufferAvailFrames;

		auto* pTempBuffer = new float[iBytesToRead / sizeof(float)];
		int iGotFrames = m_pSource->Read(pTempBuffer, iFramesToRead);
		if (iGotFrames < 0) {
			delete[] pTempBuffer;
			if (iGotFrames == END_OF_FILE && m_iDataBufferAvailFrames)
				return m_iDataBufferAvailFrames;
			return iGotFrames;
		}

		for (size_t i = 0; i < m_Channels.size(); ++i) {
			ChannelInfo& c = m_Channels[i];

			if ((int)c.m_DataBuffer.size() < iMaxFrames)
				c.m_DataBuffer.resize(iMaxFrames);

			const float* pIn = pTempBuffer + i;
			float* pOut = &c.m_DataBuffer[m_iDataBufferAvailFrames];
			for (int j = 0; j < iGotFrames; ++j) {
				*pOut = *pIn;
				pIn += m_Channels.size();
				++pOut;
			}
		}
		delete[] pTempBuffer;

		if (m_Channels.size() == 2 && m_bMidSideEncoding) {
			// Encode as mid/side
			ChannelInfo& left = m_Channels[0];
			ChannelInfo& right = m_Channels[1];
			float* pLeft = &left.m_DataBuffer[m_iDataBufferAvailFrames];
			float* pRight = &right.m_DataBuffer[m_iDataBufferAvailFrames];
			for (int j = 0; j < iGotFrames; ++j) {
				float mid = *pLeft + *pRight;
				float side = *pLeft - *pRight;
				*pLeft = mid;
				*pRight = side;
				pLeft++;
				pRight++;
			}			
		}

		m_iDataBufferAvailFrames += iGotFrames;
	}
	return m_iDataBufferAvailFrames;
}

void
RageSoundReader_SpeedChange::EraseData(int iFramesToDelete)
{
	ASSERT(iFramesToDelete <= m_iDataBufferAvailFrames);
	ASSERT(m_iDataBufferAvailFrames >= iFramesToDelete);
	ASSERT(m_iUncorrelatedPos >= iFramesToDelete);

	int iFramesToMove = m_iDataBufferAvailFrames - iFramesToDelete;
	m_iDataBufferAvailFrames -= iFramesToDelete;
	m_iUncorrelatedPos -= iFramesToDelete;
	for (size_t i = 0; i < m_Channels.size(); ++i) {
		ChannelInfo& c = m_Channels[i];
		if (iFramesToMove)
			memmove(&c.m_DataBuffer[0],
					&c.m_DataBuffer[iFramesToDelete],
					iFramesToMove * sizeof(float));
		ASSERT(c.m_iCorrelatedPos >= iFramesToDelete);
		c.m_iCorrelatedPos -= iFramesToDelete;
	}
}

int
RageSoundReader_SpeedChange::Step()
{
	if (m_iDataBufferAvailFrames == 0)
		return FillData(GetWindowSizeFrames());

	/* If m_iPos is non-zero, we just finished playing a previous block, so
	 * advance forward. */
	if (m_iPos) {
		/* Advance m_iCorrelatedPos past the data that was just copied, to point
		 * to the sound that we would have played if we had continued copying at
		 * that point. */
		for (size_t i = 0; i < m_Channels.size(); ++i) {
			ASSERT(m_Channels[i].m_iCorrelatedPos + m_iPos <=
				   m_iDataBufferAvailFrames);
			m_Channels[i].m_iCorrelatedPos += m_iPos;
		}

		/* Advance m_iUncorrelatedPos to the position we'd prefer to continue
		 * playing from. This rounds the position, making the ratio imprecise.
		 * Record the amount of error introduced here.  If we want to advance
		 * by 2.3 frames, we'll advance by 2.0 frames, and advance by 0.3 more
		 * the next time around. */
		float fAdvanceFrames = GetWindowSizeFrames() * m_fTrailingSpeedRatio;
		fAdvanceFrames += m_fErrorFrames;
		int iTrailingDeltaFrames = lround(fAdvanceFrames);
		m_fErrorFrames = fAdvanceFrames - iTrailingDeltaFrames;
		m_iUncorrelatedPos += iTrailingDeltaFrames;

		m_iPos = 0;
	}

	/* Update m_fTrailingSpeedRatio.  Do this after advancing
	 * m_iUncorrelatedPos, so changes to m_iUncorrelatedPos line up with
	 * GetNextSourceFrame. */
	m_fTrailingSpeedRatio = m_fSpeedRatio;

	/* We don't need any data before the earlier of m_iUncorrelatedPos or
	 * m_iCorrelatedPos. */
	int iToDelete = m_iUncorrelatedPos;
	for (size_t i = 0; i < m_Channels.size(); ++i) {
		ChannelInfo& c = m_Channels[i];
		ASSERT(c.m_iCorrelatedPos <= m_iDataBufferAvailFrames);
		iToDelete = std::min(iToDelete, c.m_iCorrelatedPos);
		// iToDelete = min( iToDelete, m_iDataBufferAvailFrames );
	}
	EraseData(iToDelete);

	/* Fill as much data as we might need to do the search and use the result.
	 */
	{
		int iMaxPositionNeeded =
		  m_iUncorrelatedPos + GetToleranceFrames() + GetWindowSizeFrames();
		for (size_t i = 0; i < m_Channels.size(); ++i)
			iMaxPositionNeeded =
			  std::max(iMaxPositionNeeded,
					   m_Channels[i].m_iCorrelatedPos + GetWindowSizeFrames());

		int iGot = FillData(iMaxPositionNeeded);
		if (iGot < 0)
			return iGot;

		if (iMaxPositionNeeded > m_iDataBufferAvailFrames) {
			/* We're at EOF.  Flush the remaining data, if any. */
			m_iUncorrelatedPos = m_Channels[0].m_iCorrelatedPos;
			return m_iDataBufferAvailFrames;
		}
	}

	/* Starting at our preferred position (m_iUncorrelatedPos), within
	 * GetToleranceFrames(), search for data that looks like the sound we would
	 * have continued playing if we kept going from the old position
	 * (m_iCorrelatedPos). */
	int iCorrelatedToMatch = GetWindowSizeFrames() / 4;
	int iUncorrelatedToMatch =
	  GetToleranceFrames() + iCorrelatedToMatch; // maximum distance to search

	for (size_t i = 0; i < m_Channels.size(); ++i) {
		ChannelInfo& c = m_Channels[i];
		ASSERT(c.m_iCorrelatedPos >= 0);
		ASSERT(c.m_iCorrelatedPos < m_iDataBufferAvailFrames);

		int iBest = FindClosestMatch(&c.m_DataBuffer[m_iUncorrelatedPos],
									 iUncorrelatedToMatch,
									 &c.m_DataBuffer[c.m_iCorrelatedPos],
									 iCorrelatedToMatch,
									 m_Channels.size());
		c.m_iLastCorrelatedPos = c.m_iCorrelatedPos;
		c.m_iCorrelatedPos = iBest + m_iUncorrelatedPos;
		ASSERT(m_Channels[i].m_iCorrelatedPos + GetWindowSizeFrames() <=
			   m_iDataBufferAvailFrames);
	}
	return m_iDataBufferAvailFrames;
}

int
RageSoundReader_SpeedChange::GetCursorAvail() const
{
	int iCursorAvail = GetWindowSizeFrames() - m_iPos;
	for (size_t i = 0; i < m_Channels.size(); ++i) {
		int iCursorAvailForChannel =
		  (m_iDataBufferAvailFrames - m_Channels[i].m_iCorrelatedPos) - m_iPos;
		iCursorAvail = std::min(iCursorAvail, iCursorAvailForChannel);
	}

	return iCursorAvail;
}

int
RageSoundReader_SpeedChange::Read(float* pBuf, int iFrames)
{
	for (;;) {
		if (m_iDataBufferAvailFrames == 0 &&
			m_fTrailingSpeedRatio == m_fSpeedRatio && m_fSpeedRatio == 1.0f) {
			/* Fast path: the buffer is empty, and we're not scaling the audio.
			 * Read directly into the output buffer, to eliminate memory and
			 * copying overhead. */
			return m_pSource->Read(pBuf, iFrames);
		}

		int iCursorAvail = GetCursorAvail();

		if (iCursorAvail == 0) {
			int iRet = Step();
			if (iRet < 0) {
				return iRet;
			}
			if (!GetCursorAvail()) {
				return END_OF_FILE;
			}
			continue;
		}

		/* copy GetWindowSizeFrames() from iCorrelatedPos */
		int iFramesLen = iFrames;
		int iFramesAvail = std::min(iCursorAvail, iFramesLen);

		iFrames -= iFramesAvail;
		int iFramesRead = iFramesAvail;

		int iWindowSizeFrames = GetWindowSizeFrames();
		float *pBufLeftRight = pBuf;

		while (iFramesAvail--) {
			for (size_t i = 0; i < m_Channels.size(); ++i) {
				ChannelInfo& c = m_Channels[i];
				float i1 = c.m_DataBuffer[c.m_iCorrelatedPos + m_iPos];
				float i2 = c.m_DataBuffer[c.m_iLastCorrelatedPos + m_iPos];
				if (m_bMidSideEncoding) {
					float t = (float)m_iPos / (float)iWindowSizeFrames;		
					// Approx quick cosine fade to minimise swhipping sounds on percussion
					t = 1.0f - t*t*(3 - 2*t); 
					t = 1.0f - t*t*t*t;		  
					*pBuf++ = lerp(t, i2, i1);
				} else {
					*pBuf++ = SCALE(m_iPos, 0, iWindowSizeFrames, i2, i1);
				}
			}

			++m_iPos;
		}

		if (m_Channels.size() == 2 && m_bMidSideEncoding) {
			// Decode back to left/right
			while (pBufLeftRight != pBuf) {
				float left = pBufLeftRight[0] + pBufLeftRight[1];
				float right = pBufLeftRight[0] - pBufLeftRight[1];
				pBufLeftRight[0] = 0.5f * left;
				pBufLeftRight[1] = 0.5f * right;
				pBufLeftRight += 2;
			}
		}
	
		return iFramesRead;
	}
}

/* We prefer to be able to seek precisely, so seeking to a position produces
 * data equal to what you'd get if you read data up to that point.  This filter
 * can't do that, because the exact selection of slices is dependent on the
 * previous selection. */
int
RageSoundReader_SpeedChange::SetPosition(int iFrame)
{
	Reset();
	return RageSoundReader_Filter::SetPosition(iFrame);
}

bool
RageSoundReader_SpeedChange::SetProperty(const std::string& sProperty,
										 float fValue)
{
	if (sProperty == "Speed") {
		SetSpeedRatio(fValue);
		return true;
	}

	return RageSoundReader_Filter::SetProperty(sProperty, fValue);
}

int
RageSoundReader_SpeedChange::GetNextSourceFrame() const
{
	float fRatio = m_fTrailingSpeedRatio;

	int iSourceFrame = RageSoundReader_Filter::GetNextSourceFrame();
	int iPos = lround(m_iPos * fRatio);

	iSourceFrame -= m_iDataBufferAvailFrames;
	iSourceFrame += m_iUncorrelatedPos + iPos;
	return iSourceFrame;
}

float
RageSoundReader_SpeedChange::GetStreamToSourceRatio() const
{
	/* If the ratio has changed and GetCursorAvail() is 0, the new ratio will
	 * become the real ratio on the next read.  Otherwise, we'll continue using
	 * our old ratio for a bit longer. */
	if (GetCursorAvail() == 0)
		return m_fSpeedRatio * RageSoundReader_Filter::GetStreamToSourceRatio();
	else
		return m_fTrailingSpeedRatio *
			   RageSoundReader_Filter::GetStreamToSourceRatio();
}
