/* RageSoundReader_PostBuffering - Apply low-latency effects. */

#ifndef RAGE_SOUND_READER_POST_BUFFERING_H
#define RAGE_SOUND_READER_POST_BUFFERING_H

#include "RageSoundReader_Filter.h"

class RageSoundReader_PostBuffering : public RageSoundReader_Filter
{
  public:
	RageSoundReader_PostBuffering(RageSoundReader* pSource);
	RageSoundReader_PostBuffering* Copy() const override
	{
		return new RageSoundReader_PostBuffering(*this);
	}
	int Read(float* pBuf, int iFrames) override;
	bool SetProperty(const std::string& sProperty, float fValue) override;
	static void SetMasterVolume(float fVolume);
	static void SetBGMVolume(float fVol);
	static void SetActionVolume(float fVol);
	static float GetMasterVolume();

  private:

	float GetActionVolume() const;
	float GetBGMVolume() const;

	float m_fVolume;
	bool m_bIsAction{ false };
	bool m_bIsBGM{ false };
};

#endif
