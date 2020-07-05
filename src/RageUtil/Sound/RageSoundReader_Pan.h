/* RageSoundReader_Pan - Pan a sound left and right. */

#ifndef RAGE_SOUND_READER_PAN_H
#define RAGE_SOUND_READER_PAN_H

#include "RageSoundReader_Filter.h"

class RageSoundReader_Pan : public RageSoundReader_Filter
{
  public:
	RageSoundReader_Pan(RageSoundReader* pSource);
	RageSoundReader_Pan* Copy() const override
	{
		return new RageSoundReader_Pan(*this);
	}
	unsigned GetNumChannels() const override;
	int Read(float* pBuf, int iFrames) override;
	bool SetProperty(const std::string& sProperty, float fValue) override;

  private:
	float m_fPan;
};

#endif
