/* RageSoundReader_Extend - Add looping, delay and truncation and fading. */

#ifndef RAGE_SOUND_READER_EXTEND
#define RAGE_SOUND_READER_EXTEND

#include "RageSoundReader_Filter.h"

class RageSoundReader_Extend : public RageSoundReader_Filter
{
  public:
	RageSoundReader_Extend(RageSoundReader* pSource);
	int SetPosition(int iFrame) override;
	int Read(float* pBuffer, int iFrames) override;
	int GetNextSourceFrame() const override;
	bool SetProperty(const std::string& sProperty, float fValue) override;

	RageSoundReader_Extend* Copy() const override
	{
		return new RageSoundReader_Extend(*this);
	}
	~RageSoundReader_Extend() override = default;

  private:
	int GetEndFrame() const;
	int GetData(float* pBuffer, int iFrames);

	int m_iPositionFrames;

	enum StopMode
	{
		M_LOOP,
		M_STOP,
		M_CONTINUE
	};
	StopMode m_StopMode;
	int m_iStartFrames;
	int m_iLengthFrames;
	int m_iFadeInFrames;
	int m_iFadeOutFrames;
	bool m_bIgnoreFadeInFrames;
};

#endif
