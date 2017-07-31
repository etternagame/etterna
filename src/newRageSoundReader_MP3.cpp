#include "global.h"
#include "newRageSoundReader_MP3.h"
#include "RageUtil.h"
#include "RageLog.h"


//Copied from RageSoundReader_WAV.cpp
namespace
{
	/* pBuf contains iSamples 8-bit samples; convert to 16-bit.  pBuf must
	* have enough storage to hold the resulting data. */
	void Convert8bitToFloat(void *pBuf, int iSamples)
	{
		/* Convert in reverse, so we can do it in-place. */
		const uint8_t *pIn = (uint8_t *)pBuf;
		float *pOut = (float *)pBuf;
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
		const int16_t *pIn = (int16_t *)pBuf;
		float *pOut = (float *)pBuf;
		for (int i = iSamples - 1; i >= 0; --i)
		{
			int16_t iSample = Swap16LE(pIn[i]);
			pOut[i] = iSample / 32768.0f;
		}
	}

	void ConvertLittleEndian24BitToFloat(void *pBuf, int iSamples)
	{
		/* Convert in reverse, so we can do it in-place. */
		const unsigned char *pIn = (unsigned char *)pBuf;
		float *pOut = (float *)pBuf;
		pIn += iSamples * 3;
		for (int i = iSamples - 1; i >= 0; --i)
		{
			pIn -= 3;

			int32_t iSample =
				(int(pIn[0]) << 0) |
				(int(pIn[1]) << 8) |
				(int(pIn[2]) << 16);

			/* Sign-extend 24-bit to 32-bit: */
			if (iSample & 0x800000)
				iSample |= 0xFF000000;

			pOut[i] = iSample / 8388608.0f;
		}
	}

	void ConvertLittleEndian32BitToFloat(void *pBuf, int iSamples)
	{
		/* Convert in reverse, so we can do it in-place. */
		const int32_t *pIn = (int32_t *)pBuf;
		float *pOut = (float *)pBuf;
		for (int i = iSamples - 1; i >= 0; --i)
		{
			int32_t iSample = Swap32LE(pIn[i]);
			pOut[i] = iSample / 2147483648.0f;
		}
	}
};

newRageSoundReader_MP3::newRageSoundReader_MP3()
{
	codec=NULL;
	codecCtx = NULL;
	decodedFrame=NULL;
	IOCtx = NULL;
	formatCtx = NULL;
	length = 0;
	numChannels = 0;
	dataSize = 0;
	numSamples = 0;
	sampleRate = 1;
	for (int x = 0; x < MP3_BUFFER_PADDING; x++)
		buffer[MP3_BUFFER_SIZE + x] = 0x00000000;
}


newRageSoundReader_MP3::~newRageSoundReader_MP3()
{
	//Free everything if it isn't already
	if (codecCtx)
		avcodec::avcodec_close(codecCtx);
	if (formatCtx)
		avcodec::avformat_close_input(&formatCtx);  // AVFormatContext is released by avformat_close_input
	if (IOCtx)
		avcodec::av_free(IOCtx);             // AVIOContext is released by av_free
	if (codec)
		avcodec::av_free(codec);
	if (decodedFrame)
		avcodec::av_frame_free(&decodedFrame);
}

void newRageSoundReader_MP3::RegisterProtocols()
{
	static bool Done = false;
	if (Done)
		return;
	Done = true;

	avcodec::avcodec_register_all();
	avcodec::av_register_all();
}
int ReadFunc(void* ptr, uint8_t* buf, int buf_size)
{
	HiddenPtr<RageFileBasic>* ppFile = reinterpret_cast<HiddenPtr<RageFileBasic>*>(ptr);
	//RageFileBasic *pFile = reinterpret_cast<RageFileBasic *>(ptr);
	return (*ppFile)->Read(buf, buf_size);
}
// whence: SEEK_SET, SEEK_CUR, SEEK_END (like fseek) and AVSEEK_SIZE
int64_t SeekFunc(void* ptr, int64_t pos, int whence)
{
	HiddenPtr<RageFileBasic>* ppFile = reinterpret_cast<HiddenPtr<RageFileBasic>*>(ptr);
	//RageFileBasic *pFile = reinterpret_cast<RageFileBasic *>(ptr);
	if (whence == AVSEEK_SIZE)
		return -1;
	return (*ppFile)->Seek(static_cast<int>(pos), whence);
}
RageSoundReader_FileReader::OpenResult newRageSoundReader_MP3::Open(RageFileBasic *pFile)
{
	RegisterProtocols();
	m_pFile = pFile;

	//Free everything if it isn't already
	if (formatCtx)
		avcodec::avformat_close_input(&formatCtx);  // AVFormatContext is released by avformat_close_input
	if (IOCtx)
		avcodec::av_free(IOCtx);             // AVIOContext is released by av_free
	if (codecCtx)
		avcodec::avcodec_close(codecCtx);
	if (codec)
		avcodec::av_free(codec);
	if (decodedFrame)
		avcodec::av_frame_free(&decodedFrame);

	IOCtx = avcodec::avio_alloc_context(buffer, MP3_BUFFER_SIZE,  // internal Buffer and its size
		0,                  // bWriteable (1=true,0=false) 
		&m_pFile,          // user data ; will be passed to our callback functions
		ReadFunc,
		0,                  // Write callback function 
		SeekFunc);

	formatCtx = avcodec::avformat_alloc_context();
	formatCtx->pb = IOCtx;


	// Determining the input format
	/* I'm commenting this. I read a probe should be done to prevent crashes, but it seems the probe is making us crash randomly
	unsigned char buff[1024+64];
	for (int x = 0; x < 64; x++)
		buff[1024 + x] = 0x00000000;
	int readBytes = 0;
	readBytes += m_pFile->Read(buff, 1024);
	if(readBytes <= 0){
		SetError("Error probing (read)");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}
	if(m_pFile->Seek(0, SEEK_SET)<0) {
		SetError("Error probing (seek)");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}
	avcodec::AVProbeData probeData;
	probeData.buf = buff;
	probeData.buf_size = readBytes;
	probeData.filename = "";

	// Determine the input-format:
	formatCtx->iformat = av_probe_input_format(&probeData, 1);
	if(!formatCtx->iformat) {
		SetError("Error probing (Interpreting)");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}
	*/
	formatCtx->flags = AVFMT_FLAG_CUSTOM_IO;
	if (avcodec::avformat_open_input(&formatCtx, "", 0, 0) != 0) {
		SetError("Error opening file");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}

	// Retrieve stream information
	if (avcodec::avformat_find_stream_info(formatCtx, NULL)<0) {
		SetError("Couldn't find stream information");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}

	// Find the first audio stream
	audioStream = -1;
	int i = 0;
	int nbStreams = formatCtx->nb_streams;
	for (i = 0; i<nbStreams; i++) {
		if (formatCtx->streams[i]->codecpar->codec_type == avcodec::AVMEDIA_TYPE_AUDIO) {
			audioStream = i;
			break;
		}
	}
	if (audioStream == -1) {
		SetError("Didn't find a audio stream");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}
	// Find the decoder for the audio stream
	codec = avcodec::avcodec_find_decoder(formatCtx->streams[audioStream]->codecpar->codec_id);
	if (codec == NULL) {
		SetError("Codec not found\n");
		return OPEN_UNKNOWN_FILE_FORMAT;
	}
	// Get a pointer to the codec context for the audio stream
	avcodec::AVCodecParameters * codecParams = formatCtx->streams[audioStream]->codecpar;
	//deprecated
	//codecCtx = formatCtx->streams[audioStream]->codec; 
	codecCtx = avcodec::avcodec_alloc_context3(codec);
	avcodec::avcodec_parameters_to_context(codecCtx, codecParams);

	if (avcodec::avcodec_open2(codecCtx, codec, NULL) < 0) {
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
		break;
	case -2:
		SetError("EOF");
		return OPEN_UNKNOWN_FILE_FORMAT;
		break;
	};
	return OPEN_OK;
}


newRageSoundReader_MP3 *newRageSoundReader_MP3::Copy() const 
{
	newRageSoundReader_MP3 *ret = new newRageSoundReader_MP3;
	RageFileBasic *pFile = m_pFile->Copy();
	pFile->Seek(0);
	ret->Open(pFile);
	return ret;
}

//0=> EOF. -1=> Error . >=0 => Properly SetPosition
int newRageSoundReader_MP3::SetPosition(int iFrame)
{
	curFrame = iFrame;
	//Free the last read frame if there is one (So when we read after this we read from the frame we seeked)
	if (decodedFrame)
		avcodec::av_frame_free(&decodedFrame);
	avcodec::AVStream * stream = formatCtx->streams[audioStream];
	//Calculate the second to seek to (No idea why the *100 seems to work)
	double sec = ((static_cast<double>(iFrame)) / sampleRate * 100);
	//Calculate what we need to pass to the seek function (In the stream's time units)
	timeBase = ((stream->time_base.den) / (stream->time_base.num));
	int seekFrame = static_cast<int>(sec * timeBase);
	const int flags = AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD;
	int ret=-1;
	if (seekFrame >= 0 && seekFrame<= timeBase*length/1000)
		ret = avcodec::av_seek_frame(formatCtx, audioStream, seekFrame, flags);
	else
		ret = avcodec::av_seek_frame(formatCtx, audioStream, 0, flags);
	return ret;
}

//I think this is supposed to read samples, not frames
//Each sample being a float to be stored in the buffer
//So we need to read and decode a frame(Each frame has 1 or more channels and each has samples) if needed
//Then read samples from the frame until the frame finishes or we read enough samples
//Then either return the samples read or read and decode another frame
int newRageSoundReader_MP3::Read(float *pBuf, int iFrames)
{
	uint8_t* buf = reinterpret_cast<uint8_t*>(pBuf);//Increases by 1 byte
	int samplesRead = 0;
	if (iFrames <= 0)
		return iFrames;
	if (decodedFrame != NULL) {
		samplesRead += WriteSamplesForAllChannels(buf + samplesRead*numChannels*dataSize, iFrames - samplesRead);
	}
	while (samplesRead < iFrames) {
		if (!decodedFrame)
			switch (ReadAFrame()) {
			case -1:
				return ERROR;
				break;
			case -2:
				SetError("EOF");
				return END_OF_FILE;
				break;
			};
		int read = WriteSamplesForAllChannels(buf + samplesRead*numChannels*dataSize, iFrames - samplesRead);
		if (!read)
			return ERROR;
		samplesRead += read;
	}
	
	//Translate the raw data into something SM or whatever it is that uses it can understand
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

//Write samplesToRead samples for each channel.
//Also, the data is stored as follows: [sample1ch1,sample1ch2,sample1ch2,..]
int newRageSoundReader_MP3::WriteSamplesForAllChannels(void *pBuf, int samplesToRead)
{
	uint8_t *buf = reinterpret_cast<uint8_t*>(pBuf);
	int samplesWritten = 0; //For all channels(If 2 written and 2 channels then it's 1. samplesRead/numChannels)
	if ((numSamples - curSample) <= samplesToRead) {
		for (; curSample < numSamples; curSample++) {
			for (curChannel=0; curChannel < numChannels; curChannel++)
				memcpy(buf + (samplesWritten*numChannels + curChannel)*dataSize, decodedFrame->data[curChannel] + dataSize*curSample, (unsigned int)dataSize);
			samplesWritten++;
		}
		//Free the frame since we've read it all
		avcodec::av_frame_free(&decodedFrame);
		decodedFrame = NULL;
		curSample = 0;
	}
	else {
		int end = samplesToRead + curSample;
		for (; curSample < end; curSample++) {
			for (curChannel=0; curChannel < numChannels; curChannel++)
				memcpy(buf + (samplesWritten*numChannels + curChannel)*dataSize, decodedFrame->data[curChannel] + dataSize*curSample, (unsigned int)dataSize);
			samplesWritten++;
		}
	}
	curFrame += samplesWritten;
	return samplesWritten;

}

int newRageSoundReader_MP3::GetNextSourceFrame() const
{
	return curFrame;
}
//Return: -1 => Error already set. -2 => EOF. >=0 => bytesRead
int newRageSoundReader_MP3::ReadAFrame()
{
	avcodec::AVPacket avpkt;
	avcodec::av_init_packet(&avpkt);
	avpkt.data = buffer;
	if (decodedFrame)
		avcodec::av_frame_free(&decodedFrame);
	if (!(decodedFrame = avcodec::av_frame_alloc())) {
		SetError("Error allocating memory for frame");
		return -1;
	}
	while (avcodec::av_read_frame(formatCtx, &avpkt) >= 0) {
	//while (avpkt.size > 0) {
		avpkt.dts =
			avpkt.pts = AV_NOPTS_VALUE;
		if (avpkt.stream_index == audioStream) {
			
			int ret = avcodec::avcodec_send_packet(codecCtx, &avpkt);
			if (ret < 0) {
				SetError("Error submitting the packet to the decoder\n");
				return -1;
			}
			while (ret >= 0) {
				ret = avcodec::avcodec_receive_frame(codecCtx, decodedFrame);
				if (ret == AVERROR_EOF)
					return -2;
				if (ret == AVERROR(EAGAIN)) {
					ret = avcodec::avcodec_send_packet(codecCtx, &avpkt);
					if (ret < 0) {
						SetError("Error submitting the packet to the decoder\n");
						return -1;
					}
					break;
				}
				else if (ret < 0) {
					SetError("Error during decoding\n");
					return -1;
				}
				numChannels = avcodec::av_get_channel_layout_nb_channels(decodedFrame->channel_layout);
				dataSize = avcodec::av_get_bytes_per_sample(codecCtx->sample_fmt);
				numSamples = decodedFrame->nb_samples;
				curChannel = 0;
				curSample = 0;
				avcodec::av_packet_unref(&avpkt);
				return numSamples*numChannels*dataSize;
			}
			break;
			/* avcodec_decode_audio4 deprecated
			int size = avpkt.size;
			while (size > 0 && avpkt.stream_index == audioStream) {
				int gotFrame = 0;
				int len = avcodec::avcodec_decode_audio4(codecCtx, decodedFrame, &gotFrame, &avpkt);
				if (len == -1) {
					SetError("Error while decoding\n");
					return -1;
				}
				size -= len;
				if (gotFrame) {
					curChannel = 0;
					curSample = 0;
					numSamples = decodedFrame->nb_samples;
					dataSize = avcodec::av_get_bytes_per_sample(codecCtx->sample_fmt);
					numChannels = codecCtx->channels;
					int read = avpkt.size;
					avcodec::av_packet_unref(&avpkt);
					return read;
				}
			}
			break;
			*/
		}
	}
	avcodec::av_packet_unref(&avpkt);
	return -2;
}
