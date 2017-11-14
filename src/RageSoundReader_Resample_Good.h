/* RageSoundReader_Resample_Good - fast audio resampling. */

#ifndef RAGE_SOUND_READER_RESAMPLE_GOOD_H
#define RAGE_SOUND_READER_RESAMPLE_GOOD_H

#include "RageSoundReader_Filter.h"

class RageSoundResampler_Polyphase;

/** @brief This class changes the sampling rate of a sound. */
class RageSoundReader_Resample_Good: public RageSoundReader_Filter
{
public:
	/* We own source. */
	RageSoundReader_Resample_Good( RageSoundReader *pSource, int iSampleRate );
	RageSoundReader_Resample_Good( const RageSoundReader_Resample_Good &cpy );
	int SetPosition( int iFrame ) override;
	int Read( float *pBuf, int iFrames ) override;
	~RageSoundReader_Resample_Good() override;
	RageSoundReader_Resample_Good *Copy() const override;
	bool SetProperty( const RString &sProperty, float fValue ) override;
	int GetNextSourceFrame() const override;
	float GetStreamToSourceRatio() const override;

	/**
	 * @brief Change the rate of a sound without changing the sample rate.
	 * @param fRatio the ratio for changing. */
	void SetRate( float fRatio );

	/**
	 * @brief Retrieve the exact rate.
	 * @return the exact rate. */
	float GetRate() const;

	int GetSampleRate() const override { return m_iSampleRate; }

private:
	void Reset();
	void ReopenResampler();
	void GetFactors( int &iDownFactor, int &iUpFactor ) const;

	vector<RageSoundResampler_Polyphase *> m_apResamplers; /* one per channel */

	int m_iSampleRate;
	float m_fRate;
};

#endif

/*
 * (c) 2006 Glenn Maynard
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
