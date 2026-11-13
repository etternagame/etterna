/* RageSoundReader_Extend - Add looping, delay and truncation and fading. */

#ifndef RAGE_SOUND_READER_EXTEND
#define RAGE_SOUND_READER_EXTEND

#include "RageSoundReader_Filter.h"

class RageSoundReader_Extend : public RageSoundReader_Filter
{
  public:
	RageSoundReader_Extend(RageSoundReader* pSource);
	int SetPosition(int64_t iFrame) override;
	int64_t Read(float* pBuffer, int64_t iFrames) override;
	int64_t GetNextSourceFrame() const override;
	bool SetProperty(const std::string& sProperty, float fValue) override;

	RageSoundReader_Extend* Copy() const override
	{
		return new RageSoundReader_Extend(*this);
	}
	~RageSoundReader_Extend() override = default;

  private:
	int64_t GetEndFrame() const;
	int64_t GetData(float* pBuffer, int64_t iFrames);

	int64_t m_iPositionFrames;

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
