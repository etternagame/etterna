/* RageSoundReader_PitchChange - change the pitch and speed of an audio stream
 * independently. */

#ifndef RAGE_SOUND_READER_PITCH_CHANGE_H
#define RAGE_SOUND_READER_PITCH_CHANGE_H

#include "RageSoundReader_Filter.h"

class RageSoundReader_SpeedChange;
class RageSoundReader_Resample_Good;

class RageSoundReader_PitchChange : public RageSoundReader_Filter
{
  public:
	RageSoundReader_PitchChange(RageSoundReader* pSource);
	RageSoundReader_PitchChange(const RageSoundReader_PitchChange& cpy);

	int Read(float* pBuf, int iFrames) override;
	bool SetProperty(const std::string& sProperty, float fValue) override;

	void SetSpeedRatio(float fRatio) { m_fSpeedRatio = fRatio; }
	void SetPitchRatio(float fRatio) { m_fPitchRatio = fRatio; }

	RageSoundReader_PitchChange* Copy() const override
	{
		return new RageSoundReader_PitchChange(*this);
	}

  private:
	RageSoundReader_SpeedChange*
	  m_pSpeedChange; // freed by RageSoundReader_Filter
	RageSoundReader_Resample_Good*
	  m_pResample; // freed by RageSoundReader_Filter

	float m_fSpeedRatio;
	float m_fPitchRatio;
	float m_fLastSetSpeedRatio;
	float m_fLastSetPitchRatio;
	// Swallow up warnings. If they must be used, define them.
	RageSoundReader_PitchChange& operator=(
	  const RageSoundReader_PitchChange& rhs) = delete;
};

#endif
