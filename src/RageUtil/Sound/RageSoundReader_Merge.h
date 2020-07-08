#ifndef RAGE_SOUND_READER_MERGE
#define RAGE_SOUND_READER_MERGE

#include "RageSoundReader.h"

/** @brief Chain different sounds together. */
class RageSoundReader_Merge : public RageSoundReader
{
  public:
	RageSoundReader_Merge();
	~RageSoundReader_Merge() override;
	RageSoundReader_Merge(const RageSoundReader_Merge& cpy);
	RageSoundReader_Merge* Copy() const override
	{
		return new RageSoundReader_Merge(*this);
	}

	int GetLength() const override;
	int GetLength_Fast() const override;
	int SetPosition(int iFrame) override;
	int Read(float* pBuf, int iFrames) override;
	int GetSampleRate() const override { return m_iSampleRate; }
	unsigned GetNumChannels() const override { return m_iChannels; }
	bool SetProperty(const std::string& sProperty, float fValue) override;
	int GetNextSourceFrame() const override { return m_iNextSourceFrame; }
	float GetStreamToSourceRatio() const override
	{
		return m_fCurrentStreamToSourceRatio;
	}
	std::string GetError() const override { return ""; }

	void AddSound(RageSoundReader* pSound);

	/**
	 * @brief Finish adding sounds.
	 * @param iPreferredSampleRate the sample rate for the sounds. */
	void Finish(int iPreferredSampleRate);

  private:
	int GetSampleRateInternal() const;

	int m_iSampleRate;
	unsigned m_iChannels;

	vector<RageSoundReader*> m_aSounds;

	/* Read state: */
	int m_iNextSourceFrame;
	float m_fCurrentStreamToSourceRatio;
};

#endif
