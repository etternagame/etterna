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
	bool SetProperty(const RString& sProperty, float fValue) override;
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

/*
 * Copyright (c) 2006 Glenn Maynard
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
