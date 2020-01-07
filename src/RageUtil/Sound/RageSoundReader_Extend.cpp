#include "Etterna/Globals/global.h"
#include "RageSoundReader_Extend.h"
#include "RageSoundUtil.h"
#include "RageUtil/Utils/RageUtil.h"

/*
 * Add support for negative seeks (adding a delay), extending a sound
 * beyond its end (m_LengthSeconds and M_CONTINUE), looping and fading.
 * This filter is normally inserted before extended buffering, implementing
 * properties that can seek the sound; this results in buffered seeks, but
 * changes to these properties are delayed.
 */

RageSoundReader_Extend::RageSoundReader_Extend(RageSoundReader* pSource)
  : RageSoundReader_Filter(pSource)
{
	ASSERT_M(pSource != NULL,
			 "The music file was not found! Was it deleted or moved while the "
			 "game was on?");
	m_iPositionFrames = pSource->GetNextSourceFrame();

	m_StopMode = M_STOP;
	m_iStartFrames = 0;
	m_iLengthFrames = -1;
	m_iFadeOutFrames = 0;
	m_iFadeInFrames = 0;
	m_bIgnoreFadeInFrames = false;
}

int
RageSoundReader_Extend::SetPosition(int iFrame)
{
	m_bIgnoreFadeInFrames = false;

	m_iPositionFrames = iFrame;
	int iRet = m_pSource->SetPosition(std::max(iFrame, 0));
	if (iRet < 0)
		return iRet;

	if (m_iLengthFrames != -1)
		return m_iPositionFrames < GetEndFrame();

	/* If we're in CONTINUE and we seek past the end of the file, don't return
	 * EOF. */
	if (m_StopMode == M_CONTINUE)
		return 1;

	return iRet;
}

int
RageSoundReader_Extend::GetEndFrame() const
{
	if (m_iLengthFrames == -1)
		return -1;

	return m_iStartFrames + m_iLengthFrames;
}

int
RageSoundReader_Extend::GetData(float* pBuffer, int iFrames)
{
	int iFramesToRead = iFrames;
	if (m_iLengthFrames != -1) {
		int iFramesLeft = GetEndFrame() - m_iPositionFrames;
		iFramesLeft = std::max(0, iFramesLeft);
		iFramesToRead = std::min(iFramesToRead, iFramesLeft);
	}

	if (iFrames && !iFramesToRead)
		return RageSoundReader::END_OF_FILE;

	if (m_iPositionFrames < 0) {
		iFramesToRead = std::min(iFramesToRead, -m_iPositionFrames);
		memset(
		  pBuffer, 0, iFramesToRead * sizeof(float) * this->GetNumChannels());
		return iFramesToRead;
	}

	int iNewPositionFrames = m_pSource->GetNextSourceFrame();
	int iRet = RageSoundReader_Filter::Read(pBuffer, iFramesToRead);

	/* Update the position from the source.  If the source is at EOF, skip this,
	 * so we'll extrapolate in M_CONTINUE. */
	if (iRet != RageSoundReader::END_OF_FILE)
		m_iPositionFrames = iNewPositionFrames;
	return iRet;
}

int
RageSoundReader_Extend::Read(float* pBuffer, int iFrames)
{
	int iFramesRead = GetData(pBuffer, iFrames);
	if (iFramesRead == RageSoundReader::END_OF_FILE) {
		if ((m_iLengthFrames != -1 && m_iPositionFrames < GetEndFrame()) ||
			m_StopMode == M_CONTINUE) {
			iFramesRead = iFrames;
			if (m_StopMode != M_CONTINUE)
				iFramesRead =
				  std::min(GetEndFrame() - m_iPositionFrames, iFramesRead);
			memset(
			  pBuffer, 0, iFramesRead * sizeof(float) * this->GetNumChannels());
		}
	}

	if (iFramesRead > 0) {
		int iFullVolumePositionFrames = 0;
		int iSilencePositionFrames = 0;
		if (m_iFadeInFrames != 0 && !m_bIgnoreFadeInFrames) {
			iSilencePositionFrames = 0;
			iFullVolumePositionFrames = m_iFadeInFrames;
		}

		/* We want to fade when there's m_iFadeFrames frames left, but if
		 * m_LengthFrames is -1, we don't know the length we're playing.
		 * (m_LengthFrames is the length to play, not the length of the
		 * source.)  If we don't know the length, don't fade. */
		if (m_iFadeOutFrames != 0 && m_iLengthFrames != -1) {
			iSilencePositionFrames = GetEndFrame();
			iFullVolumePositionFrames =
			  iSilencePositionFrames - m_iFadeOutFrames;
		}

		if (iSilencePositionFrames != iFullVolumePositionFrames) {
			const int iStartSecond = m_iPositionFrames;
			const int iEndSecond = m_iPositionFrames + iFramesRead;
			const float fStartVolume = SCALE(iStartSecond,
											 iFullVolumePositionFrames,
											 iSilencePositionFrames,
											 1.0f,
											 0.0f);
			const float fEndVolume = SCALE(iEndSecond,
										   iFullVolumePositionFrames,
										   iSilencePositionFrames,
										   1.0f,
										   0.0f);
			RageSoundUtil::Fade(pBuffer,
								iFramesRead,
								this->GetNumChannels(),
								fStartVolume,
								fEndVolume);
		}

		m_iPositionFrames += iFramesRead;
	}

	if (iFramesRead == RageSoundReader::END_OF_FILE && m_StopMode == M_LOOP) {
		this->SetPosition(m_iStartFrames);

		/* If we're not fading out at the end, then only fade in once.  Ignore
		 * m_iFadeInFrames until seeked, so we only fade in once. */
		if (m_iFadeOutFrames == 0)
			m_bIgnoreFadeInFrames = true;
		return STREAM_LOOPED;
	}

	return iFramesRead;
}

int
RageSoundReader_Extend::GetNextSourceFrame() const
{
	return m_iPositionFrames;
}

bool
RageSoundReader_Extend::SetProperty(const RString& sProperty, float fValue)
{
	if (sProperty == "StartSecond") {
		m_iStartFrames = lround(fValue * this->GetSampleRate());
		return true;
	}

	if (sProperty == "LengthSeconds") {
		if (fValue == -1)
			m_iLengthFrames = -1;
		else
			m_iLengthFrames = lround(fValue * this->GetSampleRate());
		return true;
	}

	if (sProperty == "Loop") {
		m_StopMode = M_LOOP;
		return true;
	}

	if (sProperty == "Stop") {
		m_StopMode = M_STOP;
		return true;
	}

	if (sProperty == "Continue") {
		m_StopMode = M_CONTINUE;
		return true;
	}

	if (sProperty == "FadeInSeconds") {
		m_iFadeInFrames = lround(fValue * this->GetSampleRate());
		return true;
	}

	if (sProperty == "FadeSeconds" || sProperty == "FadeOutSeconds") {
		m_iFadeOutFrames = lround(fValue * this->GetSampleRate());
		return true;
	}

	return RageSoundReader_Filter::SetProperty(sProperty, fValue);
}
