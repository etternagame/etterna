#include "Etterna/Globals/global.h"
#include "RageSoundReader_Pan.h"
#include "RageSoundUtil.h"

#include <algorithm>

RageSoundReader_Pan::RageSoundReader_Pan(RageSoundReader* pSource)
  : RageSoundReader_Filter(pSource)
{
	m_fPan = 0;
}

int
RageSoundReader_Pan::Read(float* pBuf, int iFrames)
{
	iFrames = m_pSource->Read(pBuf, iFrames);
	if (iFrames < 0)
		return iFrames;

	int iSamples = iFrames * m_pSource->GetNumChannels();

	if (m_pSource->GetNumChannels() == 1) {
		RageSoundUtil::ConvertMonoToStereoInPlace(pBuf, iSamples);
		iSamples *= 2;
	}

	/* This block goes from iStreamFrame to iStreamFrame+iGotFrames. */
	if (GetNumChannels() == 2 && m_fPan != 0.0)
		RageSoundUtil::Pan(pBuf, iFrames, m_fPan);

	return iFrames;
}

unsigned
RageSoundReader_Pan::GetNumChannels() const
{
	return std::max(2u, RageSoundReader_Filter::GetNumChannels());
}

bool
RageSoundReader_Pan::SetProperty(const std::string& sProperty, float fValue)
{
	if (sProperty == "Pan") {
		m_fPan = fValue;
		return true;
	}

	return RageSoundReader_Filter::SetProperty(sProperty, fValue);
}
