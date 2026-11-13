/* RageSoundReader_Preload - Preload sounds from another reader into memory. */

#ifndef RAGE_SOUND_READER_PRELOAD
#define RAGE_SOUND_READER_PRELOAD

#include "RageSoundReader.h"

class RageSoundReader_Preload : public RageSoundReader
{
  public:
	RageSoundReader_Preload();
	/* Return true if the sound has been preloaded, in which case source will
	 * be deleted.  Otherwise, return false. */
	bool Open(RageSoundReader* pSource);
	int GetLength() const override;
	int GetLength_Fast() const override;
	int SetPosition(int64_t iFrame) override;
	int64_t Read(float* pBuffer, int64_t iLength) override;
	int GetSampleRate() const override { return m_iSampleRate; }
	unsigned GetNumChannels() const override { return m_iChannels; }
	int64_t GetNextSourceFrame() const override;
	float GetStreamToSourceRatio() const override { return m_fRate; }
	std::string GetError() const override { return ""; }

	/* Return the total number of copies of this sound.  (If 1 is returned,
	 * this is the last copy.) */
	int GetReferenceCount() const;

	RageSoundReader_Preload* Copy() const override;
	~RageSoundReader_Preload() override = default;

	/* Attempt to preload a sound.  pSound must be rewound. */
	static bool PreloadSound(RageSoundReader*& pSound);

  private:
	std::shared_ptr<std::string> m_Buffer;
	bool m_bBufferIs16Bit{ false };

	/* Bytes: */
	int64_t m_iPosition{ 0 };

	int64_t GetTotalFrames() const;

	int m_iSampleRate{ 0 };
	unsigned m_iChannels{ 0 };
	float m_fRate{ 0.0f };
};

#endif
