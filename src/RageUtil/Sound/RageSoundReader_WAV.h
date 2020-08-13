/* RageSoundReader_WAV - WAV reader. */

#ifndef RAGE_SOUND_READER_WAV_H
#define RAGE_SOUND_READER_WAV_H

#include "RageUtil/File/RageFile.h"
#include "RageSoundReader_FileReader.h"

struct WavReader;

std::string
ReadString(RageFileBasic& f, int iSize, std::string& sError);

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
		int32_t m_iDataChunkPos = 0, m_iDataChunkSize = 0, m_iExtraFmtPos = 0,
				m_iSampleRate = 0, m_iFormatTag = 0;
		int16_t m_iChannels = 0, m_iBitsPerSample = 0, m_iBlockAlign = 0,
				m_iExtraFmtBytes = 0;
	};

  private:
	WavData m_WavData;

	WavReader* m_pImpl;
};

#endif
