/* RageSoundReader_Preload - Preload sounds from another reader into memory. */

#ifndef RAGE_SOUND_READER_PRELOAD
#define RAGE_SOUND_READER_PRELOAD

#include "RageSoundReader.h"
#include "RageUtil_AutoPtr.h"

class RageSoundReader_Preload: public RageSoundReader
{
public:
	RageSoundReader_Preload();
	/* Return true if the sound has been preloaded, in which case source will
	 * be deleted.  Otherwise, return false. */
	bool Open( RageSoundReader *pSource );
	int GetLength() const override;
	int GetLength_Fast() const override;
	int SetPosition( int iFrame ) override;
	int Read( float *pBuffer, int iLength ) override;
	int GetSampleRate() const override { return m_iSampleRate; }
	unsigned GetNumChannels() const override { return m_iChannels; }
	int GetNextSourceFrame() const override;
	float GetStreamToSourceRatio() const override { return m_fRate; }
	RString GetError() const override { return ""; }

	/* Return the total number of copies of this sound.  (If 1 is returned,
	 * this is the last copy.) */
	int GetReferenceCount() const;

	RageSoundReader_Preload *Copy() const override;
	~RageSoundReader_Preload() override = default;

	/* Attempt to preload a sound.  pSound must be rewound. */
	static bool PreloadSound( RageSoundReader *&pSound );

private:
	AutoPtrCopyOnWrite<RString> m_Buffer;
	bool m_bBufferIs16Bit{false};

	/* Bytes: */
	int m_iPosition{0};

	int GetTotalFrames() const;

	int m_iSampleRate{0};
	unsigned m_iChannels{0};
	float m_fRate{0.0f};
};

#endif

/*
 * Copyright (c) 2003 Glenn Maynard
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
