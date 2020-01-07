/* RageSoundReader_WAV - WAV reader. */

#ifndef RAGE_SOUND_READER_WAV_H
#define RAGE_SOUND_READER_WAV_H

#include "RageUtil/File/RageFile.h"
#include "RageSoundReader_FileReader.h"

struct WavReader;

RString
ReadString(RageFileBasic& f, int iSize, RString& sError);

class RageSoundReader_WAV : public RageSoundReader_FileReader
{
  public:
	OpenResult Open(RageFileBasic* pFile) override;
	void Close();
	int GetLength() const override;
	int SetPosition(int iFrame) override;
	int Read(float* pBuf, int iFrames) override;
	int GetSampleRate() const override { return m_WavData.m_iSampleRate; }
	unsigned GetNumChannels() const override { return m_WavData.m_iChannels; }
	int GetNextSourceFrame() const override;
	RageSoundReader_WAV();
	~RageSoundReader_WAV() override;
	RageSoundReader_WAV(
	  const RageSoundReader_WAV&); /* not defined; don't use */
	RageSoundReader_WAV* Copy() const override;

	struct WavData
	{
		int32_t m_iDataChunkPos, m_iDataChunkSize, m_iExtraFmtPos,
		  m_iSampleRate, m_iFormatTag;
		int16_t m_iChannels, m_iBitsPerSample, m_iBlockAlign, m_iExtraFmtBytes;
	};

  private:
	WavData m_WavData;

	WavReader* m_pImpl;
};

#endif
