/* RageSoundReader_MP3 - An interface to read MP3s via MAD. */

#ifndef RAGE_SOUND_READER_MP3_H
#define RAGE_SOUND_READER_MP3_H

#include "RageSoundReader_FileReader.h"
#include "RageUtil/File/RageFile.h"

struct madlib_t;

typedef unsigned long id3_length_t;

signed long
id3_tag_query(const unsigned char* data, id3_length_t length);
void
fill_frame_index_cache(madlib_t* mad);

class RageSoundReader_MP3 : public RageSoundReader_FileReader
{
  public:
	OpenResult Open(RageFileBasic* pFile) override;
	void Close();
	int GetLength() const override { return GetLengthConst(false); }
	int GetLength_Fast() const override { return GetLengthConst(true); }
	int SetPosition(int iSample) override;
	int Read(float* pBuf, int iFrames) override;
	unsigned GetNumChannels() const override { return Channels; }
	int GetSampleRate() const override { return SampleRate; }
	int GetNextSourceFrame() const override;
	bool SetProperty(const std::string& sProperty, float fValue) override;

	RageSoundReader_MP3();
	~RageSoundReader_MP3();
	RageSoundReader_MP3(
	  const RageSoundReader_MP3&); /* not defined; don't use */
	RageSoundReader_MP3* Copy() const override;

  private:
	int SampleRate;
	int Channels;
	bool m_bAccurateSync;

	madlib_t* mad;

	bool MADLIB_rewind();
	int SetPosition_toc(int iSample, bool Xing);
	int SetPosition_hard(int iSample);
	int SetPosition_estimate(int iSample);

	int fill_buffer();
	int do_mad_frame_decode(bool headers_only = false);
	int resync();
	void synth_output();
	int seek_stream_to_byte(int byte);
	bool handle_first_frame();
	int GetLengthInternal(bool fast);
	int GetLengthConst(bool fast) const;
};

#endif
