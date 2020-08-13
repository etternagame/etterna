/* RageSoundReader_ThreadedBuffer - Buffer sounds into memory. */

#ifndef RAGE_SOUND_READER_THREADED_BUFFER
#define RAGE_SOUND_READER_THREADED_BUFFER

#include "RageSoundReader_Filter.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Utils/RageUtil_CircularBuffer.h"
#include <list>

class RageThread;
class RageSoundReader_ThreadedBuffer : public RageSoundReader_Filter
{
  public:
	RageSoundReader_ThreadedBuffer(RageSoundReader* pSource);
	RageSoundReader_ThreadedBuffer(const RageSoundReader_ThreadedBuffer& cpy);
	~RageSoundReader_ThreadedBuffer() override;
	RageSoundReader_ThreadedBuffer* Copy() const override
	{
		return new RageSoundReader_ThreadedBuffer(*this);
	}

	int SetPosition(int iFrame) override;
	int Read(float* pBuffer, int iLength) override;
	int GetNextSourceFrame() const override;

	int GetLength() const override;
	int GetLength_Fast() const override;
	int GetSampleRate() const override { return m_iSampleRate; }
	unsigned GetNumChannels() const override { return m_iChannels; }
	bool SetProperty(const std::string& sProperty, float fValue) override;
	float GetStreamToSourceRatio() const override;
	RageSoundReader* GetSource() override { return NULL; }

	/* Enable and disable threaded buffering.  Disable buffering before
	 * accessing the underlying sound.  DisableBuffering returns true if
	 * buffering was enabled. */
	void EnableBuffering();
	void EnableBuffering() const
	{
		const_cast<RageSoundReader_ThreadedBuffer*>(this)->EnableBuffering();
	}
	bool DisableBuffering();
	bool DisableBuffering() const
	{
		return const_cast<RageSoundReader_ThreadedBuffer*>(this)
		  ->DisableBuffering();
	}

  private:
	int FillFrames(int iBytes);
	int FillBlock();
	int GetFilledFrames() const;
	int GetEmptyFrames() const;
	void WaitUntilFrames(int iWaitUntilFrames);

	int m_iSampleRate;
	int m_iChannels;

	CircBuf<float> m_DataBuffer;

	struct Mapping
	{
		int iFramesBuffered{ 0 };
		int iPositionOfFirstFrame{ 0 };
		float fRate{ 1.0f };
		Mapping() = default;
	};
	std::list<Mapping> m_StreamPosition;

	bool m_bEOF;

	bool m_bEnabled;

	/* If this is true, the buffering thread owns m_pSource, even
	 * if m_Event is unlocked. */
	bool m_bFilling;

	mutable RageEvent m_Event;

	RageThread m_Thread;
	bool m_bShutdownThread;
	static int StartBufferingThread(void* p)
	{
		(reinterpret_cast<RageSoundReader_ThreadedBuffer*>(p))
		  ->BufferingThread();
		return 0;
	}
	void BufferingThread();
};

#endif
