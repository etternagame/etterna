/* RageSoundReader_Chain - Chain sounds together */

#ifndef RAGE_SOUND_READER_CHAIN
#define RAGE_SOUND_READER_CHAIN

#include "RageSoundReader.h"

#include <map>

class RageSoundReader_Chain : public RageSoundReader
{
  public:
	RageSoundReader_Chain();
	~RageSoundReader_Chain() override;
	RageSoundReader_Chain* Copy() const override;

	/* Set the preferred sample rate.  This will only be used if the source
	 * sounds use different sample rates. */
	void SetPreferredSampleRate(int iSampleRate)
	{
		m_iPreferredSampleRate = iSampleRate;
	}

	int LoadSound(std::string sPath);
	int LoadSound(RageSoundReader* pSound);

	/* Add the given sound to play after fOffsetSecs seconds.  Takes ownership
	 * of pSound. */
	void AddSound(int iIndex, float fOffsetSecs, float fPan);

	/* Finish adding sounds. */
	void Finish();

	/* Return the number of added sounds. */
	int GetNumSounds() const { return m_aSounds.size(); }

	int GetLength() const override;
	int GetLength_Fast() const override;
	int SetPosition(int iFrame) override;
	int Read(float* pBuf, int iFrames) override;
	int GetSampleRate() const override { return m_iActualSampleRate; }
	unsigned GetNumChannels() const override { return m_iChannels; }
	bool SetProperty(const std::string& sProperty, float fValue) override;
	int GetNextSourceFrame() const override;
	float GetStreamToSourceRatio() const override;
	std::string GetError() const override { return ""; }

  private:
	int GetSampleRateInternal() const;

	int m_iPreferredSampleRate;
	int m_iActualSampleRate;
	unsigned m_iChannels;

	std::map<std::string, RageSoundReader*> m_apNamedSounds;
	vector<RageSoundReader*> m_apLoadedSounds;

	struct Sound
	{
		int iIndex; // into m_apLoadedSounds
		int iOffsetMS;
		float fPan;
		RageSoundReader* pSound; // NULL if not activated

		int GetOffsetFrame(int iSampleRate) const
		{
			return int(int64_t(iOffsetMS) * iSampleRate / 1000);
		}
		bool operator<(const Sound& rhs) const
		{
			return iOffsetMS < rhs.iOffsetMS;
		}
	};
	vector<Sound> m_aSounds;

	/* Read state: */
	int m_iCurrentFrame;
	unsigned m_iNextSound;
	vector<Sound*> m_apActiveSounds;

	void ActivateSound(Sound* s);
	void ReleaseSound(Sound* s);
};

#endif
