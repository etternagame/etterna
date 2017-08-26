#ifndef RAGE_SOUND_READER_MP3_H
#define RAGE_SOUND_READER_MP3_H

#include <array>
#include "RageSoundReader_FileReader.h"

constexpr unsigned int MP3_BUFFER_SIZE = 4096;

namespace avcodec
{
	extern "C"
	{
		#include <libavformat/avformat.h>
	}
}; // namespace avcodec

class RageSoundReader_MP3: public RageSoundReader_FileReader
{
public:
	OpenResult Open(RageFileBasic *pFile) override; 
	void Close();
	int GetLength() const override {  return length; }
	int SetPosition(int iFrame) override;
	int aux{};
	int Read(float *pBuf, int iFrames) override;
	int GetSampleRate() const override {  return sampleRate; }
	unsigned GetNumChannels() const override { return numChannels; }
	int GetNextSourceFrame() const override;
	RageSoundReader_MP3();
	~RageSoundReader_MP3() override;
	RageSoundReader_MP3(const RageSoundReader_MP3 &) = delete;
	RageSoundReader_MP3 *Copy() const override;
private:
	int ReadAFrame();
	int WriteSamplesForAllChannels(void *pBuf, int samplesToRead);
	int sampleRate{1};
	int length{};
	uint8_t *buffer{nullptr};
	int audioStream{};
	int numChannels{};
	int numSamples{};
	int bitrate{};
	int dataSize{};
	int curFrame{};
	int curSample{}; 
	double timeBase{};
	int curChannel{};
	avcodec::AVIOContext* IOCtx{nullptr};
	avcodec::AVFormatContext* formatCtx{nullptr};
	avcodec::AVCodec *codec{nullptr}; 
	avcodec::AVCodecContext *codecCtx{nullptr};
	avcodec::AVFrame *decodedFrame{nullptr};
};

#endif

/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard 
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