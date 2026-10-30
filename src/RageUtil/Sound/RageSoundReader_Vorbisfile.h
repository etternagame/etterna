/* RageSoundReader_Vorbisfile - Read from a Vorbisfile interface. */

#ifndef RAGE_SOUND_READER_VORBISFILE_H
#define RAGE_SOUND_READER_VORBISFILE_H

#include "RageSoundReader_FileReader.h"

using OggVorbis_File = struct OggVorbis_File;
class RageFileBasic;

class RageSoundReader_Vorbisfile : public RageSoundReader_FileReader
{
  public:
	OpenResult Open(RageFileBasic* pFile) override;

	int GetLength() const override;
	int SetPosition(int64_t iFrame) override;
	int64_t Read(float* pBuf, int64_t iFrames) override;
	int GetSampleRate() const override;
	unsigned GetNumChannels() const override { return channels; }
	int64_t GetNextSourceFrame() const override;
	RageSoundReader_Vorbisfile();
	~RageSoundReader_Vorbisfile() override;
	RageSoundReader_Vorbisfile* Copy() const override;

  private:
	OggVorbis_File* vf;
	bool eof = false;
	bool FillBuf();
	std::string filename;
	int64_t read_offset = 0;
	unsigned channels = 0;
};

#endif
