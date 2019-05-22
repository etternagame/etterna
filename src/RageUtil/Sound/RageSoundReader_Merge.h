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
	bool SetProperty(const RString& sProperty, float fValue) override;
	int GetNextSourceFrame() const override { return m_iNextSourceFrame; }
	float GetStreamToSourceRatio() const override
	{
		return m_fCurrentStreamToSourceRatio;
	}
	RString GetError() const override { return ""; }

	void AddSound(RageSoundReader* pSound);

	/**
	 * @brief Finish adding sounds.
	 * @param iPreferredSampleRate the sample rate for the sounds. */
	void Finish(int iPreferredSampleRate);

  private:
	int GetSampleRateInternal() const;

	int m_iSampleRate;
	unsigned m_iChannels;

	std::vector<RageSoundReader*> m_aSounds;

	/* Read state: */
	int m_iNextSourceFrame;
	float m_fCurrentStreamToSourceRatio;
};

#endif

/**
 * @file
 * @author Glenn Maynard (c) 2004-2006
 * @section LICENSE
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
