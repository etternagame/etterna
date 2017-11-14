/* RageSoundReader_WAV - WAV reader. */

#ifndef RAGE_SOUND_READER_WAV_H
#define RAGE_SOUND_READER_WAV_H

#include "RageSoundReader_FileReader.h"
#include "RageFile.h"

struct WavReader;

RString ReadString( RageFileBasic &f, int iSize, RString &sError );

class RageSoundReader_WAV: public RageSoundReader_FileReader
{
public:
	OpenResult Open( RageFileBasic *pFile ) override;
	void Close();
	int GetLength() const override;
	int SetPosition( int iFrame ) override;
	int Read( float *pBuf, int iFrames ) override;
	int GetSampleRate() const override { return m_WavData.m_iSampleRate; }
	unsigned GetNumChannels() const override { return m_WavData.m_iChannels; }
	int GetNextSourceFrame() const override;
	RageSoundReader_WAV();
	~RageSoundReader_WAV() override;
	RageSoundReader_WAV( const RageSoundReader_WAV & ); /* not defined; don't use */
	RageSoundReader_WAV *Copy() const override;

	struct WavData
	{
		int32_t m_iDataChunkPos, m_iDataChunkSize, m_iExtraFmtPos, m_iSampleRate, m_iFormatTag;
		int16_t m_iChannels, m_iBitsPerSample, m_iBlockAlign, m_iExtraFmtBytes;
	};

private:
	WavData m_WavData;

	WavReader *m_pImpl;
};

#endif

/*
 * (c) 2004 Glenn Maynard
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
