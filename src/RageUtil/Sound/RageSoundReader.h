/* RageSoundReader - Data source for a RageSound. */

#ifndef RAGE_SOUND_READER_H
#define RAGE_SOUND_READER_H

class RageSoundReader
{
  public:
	virtual int GetLength() const = 0;						   /* ms */
	virtual int GetLength_Fast() const { return GetLength(); } /* ms */
	virtual int SetPosition(int iFrame) = 0;
	virtual int Read(float* pBuf, int iFrames) = 0;
	virtual ~RageSoundReader() = default;
	virtual RageSoundReader* Copy() const = 0;
	virtual int GetSampleRate() const = 0;
	virtual unsigned GetNumChannels() const = 0;
	virtual bool SetProperty(const std::string& sProperty, float fValue)
	{
		return false;
	}
	virtual RageSoundReader* GetSource() { return NULL; }

	enum
	{

		RSRERROR = -1,
		END_OF_FILE = -2,

		/* A nonblocking buffer in the filter chain has underrun, and no data is
		 * currently available. */
		WOULD_BLOCK = -3,

		/* The source position has changed in an expected way, such as looping.
		 * Seeking manually will not cause this. */
		STREAM_LOOPED = -4,
	};

	/* GetNextSourceFrame() provides the source frame associated with the next
	 * frame that will be read via Read().  GetStreamToSourceRatio() returns the
	 * ratio for extrapolating the source frames of the remainder of the block.
	 * These values are valid so long as no parameters are changed before the
	 * next Read(). */
	virtual int GetNextSourceFrame() const = 0;
	virtual float GetStreamToSourceRatio() const = 0;

	virtual std::string GetError() const = 0;
	int RetriedRead(float* pBuffer,
					int iFrames,
					int* iSourceFrame = NULL,
					float* fRate = NULL);
};

#endif
