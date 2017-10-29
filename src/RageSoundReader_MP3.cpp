#include "global.h"
#include "RageFileBasic.h"
#include "RageSoundReader_MP3.h"
#include "RageUtil.h"

// Copied from RageSoundReader_WAV.cpp	
namespace
{
	/* pBuf contains iSamples 8-bit samples; convert to 16-bit.  pBuf must
	* have enough storage to hold the resulting data. */
	void Convert8bitToFloat(void *pBuf, int iSamples)
	{
		/* Convert in reverse, so we can do it in-place. */
		const uint8_t *pIn = static_cast<uint8_t *>(pBuf);
		auto pOut = static_cast<float *>(pBuf);
		for (int i = iSamples - 1; i >= 0; --i)
		{
			int iSample = pIn[i];
			iSample -= 128; /* 0..255 -> -128..127 */
			pOut[i] = iSample / 128.0f;
		}
	}

	/* Flip 16-bit samples if necessary.  On little-endian systems, this will
	* optimize out. */
	void ConvertLittleEndian16BitToFloat(void *pBuf, int iSamples)
	{
		/* Convert in reverse, so we can do it in-place. */
		const int16_t *pIn = static_cast<int16_t *>(pBuf);
		auto pOut = static_cast<float *>(pBuf);
		for (int i = iSamples - 1; i >= 0; --i)
		{
			int16_t iSample = Swap16LE(pIn[i]);
			pOut[i] = iSample / 32768.0f;
		}
	}

	void ConvertLittleEndian24BitToFloat(void *pBuf, int iSamples)
	{
		/* Convert in reverse, so we can do it in-place. */
		const unsigned char *pIn = static_cast<unsigned char *>(pBuf);
		auto pOut = static_cast<float *>(pBuf);
		pIn += iSamples * 3;
		for (int i = iSamples - 1; i >= 0; --i)
		{
			pIn -= 3;

			int32_t iSample = (int(pIn[0]) << 0) 
					| (int(pIn[1]) << 8)
					| (int(pIn[2]) << 16);

			/* Sign-extend 24-bit to 32-bit: */
			if (iSample & 0x800000)
				iSample |= 0xFF000000;

			pOut[i] = iSample / 8388608.0f;
		}
	}

	void ConvertLittleEndian32BitToFloat(void *pBuf, int iSamples)
	{
		/* Convert in reverse, so we can do it in-place. */
		const int32_t *pIn = static_cast<int32_t *>(pBuf);
		auto pOut = static_cast<float *>(pBuf);
		for (int i = iSamples - 1; i >= 0; --i)
		{
			int32_t iSample = Swap32LE(pIn[i]);
			pOut[i] = iSample / 2147483648.0f;
		}
	}
}; // namespace

RageSoundReader_MP3::RageSoundReader_MP3()
{
	buffer = static_cast<uint8_t *>(av_malloc(MP3_BUFFER_SIZE));
}

RageSoundReader_MP3::~RageSoundReader_MP3()
{
	//Free everything if it isn't already
	avformat_close_input(&formatCtx);  // AVFormatContext is released by avformat_close_input
	if (IOCtx) {
		av_freep(&IOCtx->buffer);
		av_freep(&IOCtx);
	}
	avcodec_close(codecCtx);
	av_frame_free(&decodedFrame);
}

int ReadFunc(void* ptr, uint8_t* buf, int buf_size)
{
	auto ppFile = static_cast<HiddenPtr<RageFileBasic>*>(ptr);
	return (*ppFile)->Read(buf, buf_size);
}

// whence: SEEK_SET, SEEK_CUR, SEEK_END (like fseek) and AVSEEK_SIZE
int64_t SeekFunc(void* ptr, int64_t pos, int whence)
{
	auto ppFile = static_cast<HiddenPtr<RageFileBasic>*>(ptr);
	if (whence == AVSEEK_SIZE)
		return -1;
	return (*ppFile)->Seek(static_cast<int>(pos), whence);
}

RageSoundReader_FileReader::OpenResult RageSoundReader_MP3::Open(RageFileBasic *pFile)
{
	av_register_all();
	m_pFile = pFile;

	//Free everything if it isn't already
	avformat_close_input(&formatCtx);  // AVFormatContext is released by avformat_close_input
	if (IOCtx) {
		av_freep(&IOCtx->buffer);
		av_freep(&IOCtx);
	}
	avcodec_close(codecCtx);
	av_frame_free(&decodedFrame);

	IOCtx = avio_alloc_context(buffer, MP3_BUFFER_SIZE,  // internal Buffer and its size
		0, // bWriteable (1 = true, 0 = false) 
		&m_pFile, // user data -- will be passed to our callback functions
		ReadFunc,
		nullptr, // Write callback function 
		SeekFunc);

	formatCtx = avformat_alloc_context();
	formatCtx->pb = IOCtx;
	formatCtx->flags = AVFMT_FLAG_CUSTOM_IO;

	if (avformat_open_input(&formatCtx, nullptr, nullptr, nullptr) != 0) {
		SetError("Error opening file");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}

	// Retrieve stream information
	if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
		SetError("Couldn't find stream information");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}

	// Find the first audio stream
	audioStream = -1;
	int nbStreams = formatCtx->nb_streams;
	for (int i = 0; i<nbStreams; i++) {
		if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStream = i;
			break;
		}
	}
	if (audioStream == -1) {
		SetError("Didn't find a audio stream");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}
	// Find the decoder for the audio stream
	codec = avcodec_find_decoder(formatCtx->streams[audioStream]->codecpar->codec_id);
	if (codec == nullptr) {
		SetError("Codec not found\n");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}
	// Get a pointer to the codec context for the audio stream
	AVCodecParameters * codecParams = formatCtx->streams[audioStream]->codecpar;
	codecCtx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codecCtx, codecParams);

	if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
		SetError("Error opening decoder");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}
	curFrame = 0;
	curChannel = 0;
	curSample = 0;
	numChannels = codecCtx->channels;
	bitrate = static_cast<int>(codecCtx->bit_rate);
	sampleRate = codecCtx->sample_rate;
	//Length is being calculated to be in miliseconds(Not sure if that's what SM wants to read. Edit GetLength accordingly once we know)
	//I have no idea what's wrong with calculating length as formatCtx->duration/1000
	//But it makes it so an error with RageFile's Seek leads to a crash(Probably accessing memory we shouldn't be accessing)
	//And if i add breakpoints in the debug in that line and the ones inmediately above(4) they're never reached
	//I have even less of an idea why multiplying by timeBase makes it not do that, 
	//I have tried doing something like 
	//length = static_cast<double>(formatCtx->duration)*0.001;
	//length = length / 1000;
	//But it doesn't work either
	//Anyways, this works, and then we can just divide it by that and properly calculate the ms
	//Come back to this mess later
	//timeBase = (double)codecCtx->time_base.num  / (double)codecCtx->time_base.den;
	//length = formatCtx->duration * timeBase;
	//length = (length/timeBase) /1000;

	//After much trial and error it seems using another buffer for the probe
	//and placing 0's in the padding at the end of the buffer make this work
	//I have no idea why that nonesense made it work
	//I'm keeping these comments for now in case this starts giving trouble again
	length = static_cast<int>(static_cast<double>(formatCtx->duration)*0.001);
	switch (ReadAFrame()) {
	case -1:
		return OPEN_UNKNOWN_FILE_FORMAT;;
	case -2:
		SetError("EOF");
		return OPEN_UNKNOWN_FILE_FORMAT;
	};
	return OPEN_OK;
}

RageSoundReader_MP3 *RageSoundReader_MP3::Copy() const 
{
	auto ret = new RageSoundReader_MP3;
	RageFileBasic *pFile = m_pFile->Copy();
	pFile->Seek(0);
	ret->Open(pFile);
	return ret;
}

// 0 => EOF. -1 => Error. >= 0 => Properly SetPosition.
int RageSoundReader_MP3::SetPosition(int iFrame)
{
	curFrame = iFrame;
	int ret = -1;

	// Free the last read frame if there is one (So when we read after this we read from the frame we seeked)
	av_frame_free(&decodedFrame);
	
	AVStream * stream = formatCtx->streams[audioStream];
	// Calculate what we need to pass to the seek function (In the stream's time units)
	timeBase = ((stream->time_base.den) / (stream->time_base.num));
	double sec = (static_cast<double>(iFrame) / sampleRate);
	auto seekFrame = static_cast<unsigned int>(sec * timeBase);

	if (seekFrame >= 0 && sec <= stream->duration) {
		const int flags = AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD;
		ret = av_seek_frame(formatCtx, audioStream, seekFrame, flags);
		avcodec_flush_buffers(codecCtx);
	}
	return ret;
}

// I think this is supposed to read samples, not frames
// Each sample being a float to be stored in the buffer
// So we need to read and decode a frame(Each frame has 1 or more channels and each has samples) if needed
// Then read samples from the frame until the frame finishes or we read enough samples
// Then either return the samples read or read and decode another frame
int RageSoundReader_MP3::Read(float *pBuf, int iFrames)
{
	if (iFrames <= 0)
		return iFrames;

	auto buf = reinterpret_cast<uint8_t*>(pBuf); // Increases by 1 byte

	int samplesRead = 0;
	while (samplesRead < iFrames) {
		if (!decodedFrame)
			switch (ReadAFrame()) {
			case -1:
				return ERROR;
			case -2:
				SetError("EOF");
				return END_OF_FILE;
			};

		if(decodedFrame->nb_samples<=0)
			return ERROR;

		int read = WriteSamplesForAllChannels(buf + samplesRead*numChannels*dataSize, iFrames - samplesRead);
		if (read == 0)
			return ERROR;

		samplesRead += read;
	}
	
	// Translate the raw data into something SM or whatever it is that uses it can understand
	switch (dataSize)
	{
	case 1:
		Convert8bitToFloat(buf, samplesRead*numChannels);
		break;
	case 2:
		ConvertLittleEndian16BitToFloat(buf, samplesRead*numChannels);
		break;
	case 3:
		ConvertLittleEndian24BitToFloat(buf, samplesRead*numChannels);
		break;
	case 4:
		ConvertLittleEndian32BitToFloat(buf, samplesRead*numChannels);
		// otherwise 3; already a float 
		break;
	}
	
	return samplesRead;
}

// Write samplesToRead samples for each channel.
// Also, the data is stored as follows: [sample1ch1,sample1ch2,sample1ch2,..]
int RageSoundReader_MP3::WriteSamplesForAllChannels(void *pBuf, int samplesToRead)
{
	auto buf = static_cast<uint8_t*>(pBuf);

	int samplesWritten = 0; // For all channels (if 2 written and 2 channels then it's 1. samplesRead / numChannels)
	if ((numSamples - curSample) <= samplesToRead) {
		for (; curSample < numSamples; curSample++) {
			for (curChannel=0; curChannel < numChannels; curChannel++)
				memcpy(buf + (samplesWritten*numChannels + curChannel)*dataSize, decodedFrame->data[curChannel] + dataSize*curSample, static_cast<unsigned int>(dataSize));
			samplesWritten++;
		}
		// Free the frame since we've read it all
		av_frame_free(&decodedFrame);
		decodedFrame = nullptr;
		curSample = 0;
	}
	else {
		int end = samplesToRead + curSample;
		for (; curSample < end; curSample++) {
			for (curChannel=0; curChannel < numChannels; curChannel++)
				memcpy(buf + (samplesWritten*numChannels + curChannel)*dataSize, decodedFrame->data[curChannel] + dataSize*curSample, static_cast<unsigned int>(dataSize));
			samplesWritten++;
		}
	}

	curFrame += samplesWritten;
	return samplesWritten;

}

int RageSoundReader_MP3::GetNextSourceFrame() const
{
	return curFrame + codecCtx->frame_size;
}

// Return: -1 => Error already set. -2 => EOF. >=0 => bytesRead
int RageSoundReader_MP3::ReadAFrame()
{
	AVPacket avpkt;
	av_init_packet(&avpkt);
	avpkt.data = buffer;

	av_frame_free(&decodedFrame);
	if (!(decodedFrame = av_frame_alloc())) {
		SetError("Error allocating memory for frame");
		return -1;
	}

	while (av_read_frame(formatCtx, &avpkt) >= 0) {
		avpkt.dts = avpkt.pts = AV_NOPTS_VALUE;
		if (avpkt.stream_index == audioStream) {
			int ret = avcodec_send_packet(codecCtx, &avpkt);
			if (ret < 0) {
				SetError("Error submitting the packet to the decoder\n");
				return -1;
			}

			while (ret >= 0) {
				ret = avcodec_receive_frame(codecCtx, decodedFrame);
				if (ret == AVERROR_EOF)
					return -2;
				if (ret == AVERROR(EAGAIN)) {
					ret = avcodec_send_packet(codecCtx, &avpkt);
					if (ret < 0) {
						SetError("Error submitting the packet to the decoder\n");
						return -1;
					}
					break;
				}
				if (ret < 0) {
					SetError("Error during decoding\n");
					return -1;
				}

				numChannels = av_get_channel_layout_nb_channels(decodedFrame->channel_layout);
				dataSize = av_get_bytes_per_sample(codecCtx->sample_fmt);
				numSamples = decodedFrame->nb_samples;
				curChannel = 0;
				curSample = 0;
				av_packet_unref(&avpkt);
				return numSamples*numChannels*dataSize;
			}
			break;
		}
	}

	av_packet_unref(&avpkt);
	return -2;
}

/*
 * Copyright (c) 2004 Glenn Maynard
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
