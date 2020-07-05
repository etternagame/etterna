#include "Etterna/Globals/global.h"
#include "RageSoundReader_PostBuffering.h"
#include "RageSoundUtil.h"

/*
 * This filter is normally inserted after extended buffering, implementing
 * properties that do not seek the sound, allowing these properties to be
 * changed with low latency.
 */

RageSoundReader_PostBuffering::RageSoundReader_PostBuffering(
  RageSoundReader* pSource)
  : RageSoundReader_Filter(pSource)
{
	m_fVolume = 1.0f;
}

int
RageSoundReader_PostBuffering::Read(float* pBuf, int iFrames)
{
	iFrames = m_pSource->Read(pBuf, iFrames);
	if (iFrames < 0)
		return iFrames;

	if (m_fVolume != 1.0f)
		RageSoundUtil::Attenuate(
		  pBuf, iFrames * this->GetNumChannels(), m_fVolume);

	return iFrames;
}

bool
RageSoundReader_PostBuffering::SetProperty(const std::string& sProperty,
										   float fValue)
{
	if (sProperty == "Volume") {
		m_fVolume = fValue;
		return true;
	}

	return RageSoundReader_Filter::SetProperty(sProperty, fValue);
}
