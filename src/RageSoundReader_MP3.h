
#ifndef RAGE_SOUND_READER_MP3_H
#define RAGE_SOUND_READER_MP3_H
#define MP3_BUFFER_SIZE 5120
#define MP3_BUFFER_PADDING 64

#include "global.h"
#include "RageSoundReader_FileReader.h"
#include "RageFile.h"

namespace avcodec
{
	extern "C"
	{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/rational.h>
#include <libavcodec/avcodec.h>
	}
};




class RageSoundReader_MP3 :
	public RageSoundReader_FileReader
{
public:
	static void RegisterProtocols();
	OpenResult Open(RageFileBasic *pFile); 
	void Close();
	int GetLength() const {  return length; }
	int SetPosition(int iFrame);
	int aux;
	int Read(float *pBuf, int iSample);
	int GetSampleRate() const {  return sampleRate; }
	unsigned GetNumChannels() const { return numChannels; }
	int GetNextSourceFrame() const ;
	RageSoundReader_MP3();
	~RageSoundReader_MP3();
	RageSoundReader_MP3(const RageSoundReader_MP3 &); /* not defined; don't use */
	RageSoundReader_MP3 *Copy() const;
private:
	int sampleRate;
	int length;
	uint8_t buffer[MP3_BUFFER_SIZE + MP3_BUFFER_PADDING];
	int audioStream;
	int numChannels;
	avcodec::AVIOContext* IOCtx;
	avcodec::AVFormatContext* formatCtx;
	int numSamples; 
	int WriteSamplesForAllChannels(void *pBuf, int samplesToRead);
	int bitrate;
	int dataSize;
	int curFrame;
	int curSample; 
	double timeBase;
	int curChannel;
	avcodec::AVCodec *codec; 
	avcodec::AVCodecContext *codecCtx;
	avcodec::AVFrame *decodedFrame;
	int ReadAFrame();
};

#endif

/*
 * Copyright (c) 2017 Nicolas (Nickito12), RGates94
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