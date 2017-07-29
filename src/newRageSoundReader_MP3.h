
#ifndef RAGE_SOUND_READER_MP3_H
#define RAGE_SOUND_READER_MP3_H
#define MP3_BUFFERSIZE 40960

#include "RageSoundReader_FileReader.h"
#include "RageFile.h"

namespace avcodec
{
	extern "C"
	{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
	}
};


void RegisterProtocols();

class newRageSoundReader_MP3 :
	public RageSoundReader_FileReader
{
public:
	OpenResult Open(RageFileBasic *pFile); 
	void Close();
	int GetLength() const {  return length; }
	int SetPosition(int iFrame);
	int aux;
	int Read(float *pBuf, int iSample);
	int GetSampleRate() const {  return sampleRate; }
	unsigned GetNumChannels() const { return numChannels; }
	int GetNextSourceFrame() const ;
	newRageSoundReader_MP3();
	~newRageSoundReader_MP3();
	newRageSoundReader_MP3(const newRageSoundReader_MP3 &); /* not defined; don't use */
	newRageSoundReader_MP3 *Copy() const;
private:
	int sampleRate;
	int length;
	uint8_t buffer[MP3_BUFFERSIZE + 64];
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
