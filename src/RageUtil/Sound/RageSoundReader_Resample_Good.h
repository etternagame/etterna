/* RageSoundReader_Resample_Good - fast audio resampling. */

#ifndef RAGE_SOUND_READER_RESAMPLE_GOOD_H
#define RAGE_SOUND_READER_RESAMPLE_GOOD_H

#include "RageSoundReader_Filter.h"

class RageSoundResampler_Polyphase;

/** @brief This class changes the sampling rate of a sound. */
class RageSoundReader_Resample_Good : public RageSoundReader_Filter
{
  public:
	/* We own source. */
	RageSoundReader_Resample_Good(RageSoundReader* pSource, int iSampleRate);
	RageSoundReader_Resample_Good(const RageSoundReader_Resample_Good& cpy);
	int SetPosition(int iFrame) override;
	int Read(float* pBuf, int iFrames) override;
	~RageSoundReader_Resample_Good() override;
	RageSoundReader_Resample_Good* Copy() const override;
	bool SetProperty(const std::string& sProperty, float fValue) override;
	int GetNextSourceFrame() const override;
	float GetStreamToSourceRatio() const override;

	/**
	 * @brief Change the rate of a sound without changing the sample rate.
	 * @param fRatio the ratio for changing. */
	void SetRate(float fRatio);

	/**
	 * @brief Retrieve the exact rate.
	 * @return the exact rate. */
	float GetRate() const;

	int GetSampleRate() const override { return m_iSampleRate; }

  private:
	void Reset();
	void ReopenResampler();
	void GetFactors(int& iDownFactor, int& iUpFactor) const;

	vector<RageSoundResampler_Polyphase*> m_apResamplers; /* one per channel */

	int m_iSampleRate;
	float m_fRate;
};

#endif
