/* RageSoundReader_SpeedChange_Good - change the speed of an audio stream without
 * changing its pitch. */

#ifndef RAGE_SOUND_READER_SPEED_CHANGE_GOOD_H
#define RAGE_SOUND_READER_SPEED_CHANGE_GOOD_H

#include <string>
#include <vector>

#include "RageSoundReader_Filter.h"

struct SpeedChangeFFT;

class RageSoundReader_SpeedChange_Good : public RageSoundReader_Filter
{
  public:
	RageSoundReader_SpeedChange_Good(RageSoundReader* pSource);

	virtual int SetPosition(int iFrame);
	virtual int Read(float* pBuf, int iFrames);
	virtual RageSoundReader_SpeedChange_Good* Copy() const
	{
		return new RageSoundReader_SpeedChange_Good(*this);
	}
	virtual bool SetProperty(const std::string& sProperty, float fValue);
	virtual int GetNextSourceFrame() const;
	virtual float GetStreamToSourceRatio() const;

	void SetSpeedRatio(float fRatio);
	float GetRatio() const { return m_fRate; }

  protected:

	float m_fRate = 1.0f;
	double m_dPos = 0.0;
	bool m_bDraining = false;

	struct AudioBuffer {
		int64_t iNumChannels = 1;
		int64_t iReadPosition = 0;
		int64_t iWritePosition = 0;
		std::vector<float> Samples;

		int64_t Frames() const
		{
			return iWritePosition - iReadPosition;
		}

		void Extend(int64_t iFrames) {
			uint64_t iSamples = uint64_t((iWritePosition + iFrames) * iNumChannels);
			Samples.resize(iSamples);
		}

		void Shift()
		{
			int64_t iSamples = iReadPosition * iNumChannels;
			Samples.erase(Samples.begin(), Samples.begin() + iSamples);

			iWritePosition -= iReadPosition;
			iReadPosition -= iReadPosition;
		}
	};

	AudioBuffer m_ReadAhead;
	AudioBuffer m_Mixed;
	std::vector<float> m_Scale;
	std::vector<float> m_Copy;

	struct Window
	{
		std::vector<float> Buffer;
		int64_t iSize = 0;
		int64_t iSourceStep = 0;
		double dDestStep = 0.0;
		std::shared_ptr<SpeedChangeFFT> Junk;

		static Window Make(int iSampleRate, double dRate);
	};

	Window m_Window;
};

#endif
