/* RageSoundReader_Filter - simplify the creation of filter RageSoundReaders. */

#ifndef RAGE_SOUND_READER_FILTER_H
#define RAGE_SOUND_READER_FILTER_H

#include "RageSoundReader.h"
#include "RageUtil/Utils/RageUtil_AutoPtr.h"

class RageSoundReader_Filter : public RageSoundReader
{
  public:
	RageSoundReader_Filter(RageSoundReader* pSource)
	  : m_pSource(pSource)
	{
	}

	int GetLength() const override { return m_pSource->GetLength(); }
	int GetLength_Fast() const override { return m_pSource->GetLength_Fast(); }
	int SetPosition(int iFrame) override
	{
		return m_pSource->SetPosition(iFrame);
	}
	int Read(float* pBuf, int iFrames) override
	{
		return m_pSource->Read(pBuf, iFrames);
	}
	int GetSampleRate() const override { return m_pSource->GetSampleRate(); }
	unsigned GetNumChannels() const override
	{
		return m_pSource->GetNumChannels();
	}
	bool SetProperty(const std::string& sProperty, float fValue) override
	{
		return m_pSource->SetProperty(sProperty, fValue);
	}
	int GetNextSourceFrame() const override
	{
		return m_pSource->GetNextSourceFrame();
	}
	float GetStreamToSourceRatio() const override
	{
		return m_pSource->GetStreamToSourceRatio();
	}
	RageSoundReader* GetSource() override { return &*m_pSource; }
	std::string GetError() const override { return m_pSource->GetError(); }

  protected:
	HiddenPtr<RageSoundReader> m_pSource;
};

#endif
