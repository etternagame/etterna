/* RageSoundMixBuffer - Simple audio mixing. */

#ifndef RAGE_SOUND_MIX_BUFFER_H
#define RAGE_SOUND_MIX_BUFFER_H

class RageSoundMixBuffer
{
  public:
	RageSoundMixBuffer();
	~RageSoundMixBuffer();

	/* Mix the given buffer of samples. */
	void write(const float* pBuf,
			   unsigned iSize,
			   int iSourceStride = 1,
			   int iDestStride = 1);

	/* Extend the buffer as if write() was called with a buffer of silence. */
	void Extend(unsigned iSamples);

	void read(int16_t* pBuf);
	void read(float* pBuf);
	void read_deinterlace(float** pBufs, int channels);
	float* read() { return m_pMixbuf; }
	unsigned size() const { return m_iBufUsed; }
	void SetWriteOffset(int iOffset);

  private:
	float* m_pMixbuf;
	unsigned m_iBufSize; /* actual allocated samples */
	unsigned m_iBufUsed; /* used samples */
	int m_iOffset;
};

#endif
