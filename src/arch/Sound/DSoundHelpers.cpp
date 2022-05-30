#include "Etterna/Globals/global.h"
#include "DSoundHelpers.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Core/Services/Locator.hpp"
#include "archutils/Win32/DirectXHelpers.h"
#include "archutils/Win32/GetFileInformation.h"

#include <algorithm>

#ifdef _WIN32
#include <mmsystem.h>
#endif
#define DIRECTSOUND_VERSION 0x0700
#include <dsound.h>

#if defined(_MSC_VER)
#pragma comment(lib, "dsound.lib")
#endif

BOOL CALLBACK
DSound::EnumCallback(LPGUID lpGuid,
					 LPCSTR lpcstrDescription,
					 LPCSTR lpcstrModule,
					 LPVOID lpContext)
{
	std::string sLine = ssprintf("DirectSound Driver: %s", lpcstrDescription);
	if (lpcstrModule[0]) {
		sLine += ssprintf(" %s", lpcstrModule);

		std::string sPath = FindSystemFile(lpcstrModule);
		if (sPath != "") {
			std::string sVersion;
			if (GetFileVersion(sPath, sVersion))
				sLine += ssprintf(" %s", sVersion.c_str());
		}
	}

	Locator::getLogger()->info("{}", sLine.c_str());

	return TRUE;
}

void
DSound::SetPrimaryBufferMode()
{
	DSBUFFERDESC format;
	memset(&format, 0, sizeof(format));
	format.dwSize = sizeof(format);
	format.dwFlags = DSBCAPS_PRIMARYBUFFER;
	format.dwBufferBytes = 0;
	format.lpwfxFormat = nullptr;

	IDirectSoundBuffer* pBuffer;
	HRESULT hr = this->GetDS()->CreateSoundBuffer(&format, &pBuffer, nullptr);
	if (FAILED(hr)) {
		Locator::getLogger()->warn(
		  "{}", hr_ssprintf(hr, "Couldn't create primary buffer"));
		return;
	}

	WAVEFORMATEX waveformat;
	memset(&waveformat, 0, sizeof(waveformat));
	waveformat.cbSize = 0;
	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.wBitsPerSample = 16;
	waveformat.nChannels = 2;
	waveformat.nSamplesPerSec = 44100;
	waveformat.nBlockAlign = 4;
	waveformat.nAvgBytesPerSec =
	  waveformat.nSamplesPerSec * waveformat.nBlockAlign;

	// Set the primary buffer's format
	hr = IDirectSoundBuffer_SetFormat(pBuffer, &waveformat);
	if (FAILED(hr))
		Locator::getLogger()->warn(
		  "{}", hr_ssprintf(hr, "SetFormat on primary buffer"));

	DWORD got;
	hr = pBuffer->GetFormat(&waveformat, sizeof(waveformat), &got);
	if (FAILED(hr))
		Locator::getLogger()->warn(
		  "{}", hr_ssprintf(hr, "GetFormat on primary buffer"));
	else if (waveformat.nSamplesPerSec != 44100)
		Locator::getLogger()->warn("Primary buffer set to {} instead of 44100",
								   waveformat.nSamplesPerSec);

	/*
	 * MS docs:
	 *
	 * When there are no sounds playing, DirectSound stops the mixer engine and
	 * halts DMA (direct memory access) activity. If your application has
	 * frequent short intervals of silence, the overhead of starting and
	 * stopping the mixer each time a sound is played may be worse than the DMA
	 * overhead if you kept the mixer active. Also, some sound hardware or
	 * drivers may produce unwanted audible artifacts from frequent starting and
	 * stopping of playback. If your application is playing audio almost
	 * continuously with only short breaks of silence, you can force the mixer
	 * engine to remain active by calling the IDirectSoundBuffer::Play method
	 * for the primary buffer. The mixer will continue to run silently.
	 *
	 * However, I just added the above code and I don't want to change more
	 * until it's tested.
	 */
	//	pBuffer->Play( 0, 0, DSBPLAY_LOOPING );

	pBuffer->Release();
}

DSound::DSound()
{
	HRESULT hr;
	if (FAILED(hr = CoInitialize(NULL)))
		RageException::Throw(hr_ssprintf(hr, "CoInitialize").c_str());
	m_pDS = nullptr;
}

std::string
DSound::Init()
{
	HRESULT hr;
	if (FAILED(hr = DirectSoundCreate(NULL, &m_pDS, NULL)))
		return hr_ssprintf(hr, "DirectSoundCreate");

	static bool bShownInfo = false;
	if (!bShownInfo) {
		bShownInfo = true;
		DirectSoundEnumerate(EnumCallback, nullptr);

		DSCAPS Caps;
		Caps.dwSize = sizeof(Caps);
		HRESULT hr;
		if (FAILED(hr = m_pDS->GetCaps(&Caps))) {
			Locator::getLogger()->warn(
			  "{}", hr_ssprintf(hr, "m_pDS->GetCaps failed"));
		} else {
			Locator::getLogger()->info("DirectSound sample rates: {}..{} {}",
					  Caps.dwMinSecondarySampleRate,
					  Caps.dwMaxSecondarySampleRate,
					  (Caps.dwFlags & DSCAPS_CONTINUOUSRATE) ? "(continuous)"
															 : "");
		}
	}

	/* Try to set primary mixing privileges */
	hr = m_pDS->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);

	SetPrimaryBufferMode();

	return std::string();
}

DSound::~DSound()
{
	if (m_pDS != nullptr)
		m_pDS->Release();
	CoUninitialize();
}

bool
DSound::IsEmulated() const
{
	/* Don't bother wasting time trying to create buffers if we're
	 * emulated.  This also gives us better diagnostic information. */
	DSCAPS Caps;
	Caps.dwSize = sizeof(Caps);
	HRESULT hr;
	if (FAILED(hr = m_pDS->GetCaps(&Caps))) {
		Locator::getLogger()->warn("{}", hr_ssprintf(hr, "m_pDS->GetCaps failed"));
		/* This is strange, so let's be conservative. */
		return true;
	}

	return !!(Caps.dwFlags & DSCAPS_EMULDRIVER);
}

DSoundBuf::DSoundBuf()
{
	m_pBuffer = nullptr;
	m_pTempBuffer = nullptr;
}

std::string
DSoundBuf::Init(DSound& ds,
				DSoundBuf::hw hardware,
				int iChannels,
				int iSampleRate,
				int iSampleBits,
				int iWriteAhead)
{
	m_iChannels = iChannels;
	m_iSampleRate = iSampleRate;
	m_iSampleBits = iSampleBits;
	m_iWriteAhead = iWriteAhead * bytes_per_frame();
	m_iVolume = -1; /* unset */
	m_bBufferLocked = false;
	m_iWriteCursorPos = m_iWriteCursor = m_iBufferBytesFilled = 0;
	m_iExtraWriteahead = 0;
	m_iLastPosition = 0;
	m_bPlaying = false;
	ZERO(m_iLastCursors);

	/* The size of the actual DSound buffer.  This can be large; we generally
	 * won't fill it completely. */
	m_iBufferSize = 1024 * 64;
	m_iBufferSize = std::max(m_iBufferSize, m_iWriteAhead);

	WAVEFORMATEX waveformat;
	memset(&waveformat, 0, sizeof(waveformat));
	waveformat.cbSize = 0;
	waveformat.wFormatTag = WAVE_FORMAT_PCM;

	bool bNeedCtrlFrequency = false;
	if (m_iSampleRate == DYNAMIC_SAMPLERATE) {
		m_iSampleRate = 44100;
		bNeedCtrlFrequency = true;
	}

	int bytes = m_iSampleBits / 8;
	waveformat.wBitsPerSample = WORD(m_iSampleBits);
	waveformat.nChannels = WORD(m_iChannels);
	waveformat.nSamplesPerSec = DWORD(m_iSampleRate);
	waveformat.nBlockAlign = WORD(bytes * m_iChannels);
	waveformat.nAvgBytesPerSec = m_iSampleRate * bytes * m_iChannels;

	/* Try to create the secondary buffer */
	DSBUFFERDESC format;
	memset(&format, 0, sizeof(format));
	format.dwSize = sizeof(format);
	format.dwFlags =
	  DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME;

	/* Don't use DSBCAPS_STATIC.  It's meant for static buffers, and we
	 * only use streaming buffers. */
	if (hardware == HW_HARDWARE)
		format.dwFlags |= DSBCAPS_LOCHARDWARE;
	else
		format.dwFlags |= DSBCAPS_LOCSOFTWARE;

	if (bNeedCtrlFrequency)
		format.dwFlags |= DSBCAPS_CTRLFREQUENCY;

	format.dwBufferBytes = m_iBufferSize;

	format.dwReserved = 0;

	format.lpwfxFormat = &waveformat;

	HRESULT hr = ds.GetDS()->CreateSoundBuffer(&format, &m_pBuffer, nullptr);
	if (FAILED(hr))
		return hr_ssprintf(
		  hr, "CreateSoundBuffer failed (%i hz)", m_iSampleBits);

	/* I'm not sure this should ever be needed, but ... */
	DSBCAPS bcaps;
	bcaps.dwSize = sizeof(bcaps);
	hr = m_pBuffer->GetCaps(&bcaps);
	if (FAILED(hr))
		return hr_ssprintf(hr, "m_pBuffer->GetCaps");
	if (static_cast<int>(bcaps.dwBufferBytes) != m_iBufferSize) {
		Locator::getLogger()->warn("bcaps.dwBufferBytes ({}) != m_iBufferSize({}); adjusting",
				  bcaps.dwBufferBytes,
				  m_iBufferSize);
		m_iBufferSize = bcaps.dwBufferBytes;
		m_iWriteAhead = std::min(m_iWriteAhead, m_iBufferSize);
	}

	if (!(bcaps.dwFlags & DSBCAPS_CTRLVOLUME))
		Locator::getLogger()->warn("Sound channel missing DSBCAPS_CTRLVOLUME");
	if (!(bcaps.dwFlags & DSBCAPS_GETCURRENTPOSITION2))
		Locator::getLogger()->warn("Sound channel missing DSBCAPS_GETCURRENTPOSITION2");

	DWORD got;
	hr = m_pBuffer->GetFormat(&waveformat, sizeof(waveformat), &got);
	if (FAILED(hr))
		Locator::getLogger()->warn(
		  "{}", hr_ssprintf(hr, "GetFormat on secondary buffer"));
	else if (static_cast<int>(waveformat.nSamplesPerSec) != m_iSampleRate)
		Locator::getLogger()->warn("Secondary buffer set to {} instead of {}",
				  waveformat.nSamplesPerSec,
				  m_iSampleRate);

	m_pTempBuffer = new char[m_iBufferSize];

	return std::string();
}

void
DSoundBuf::SetSampleRate(int hz)
{
	m_iSampleRate = hz;
	HRESULT hr = m_pBuffer->SetFrequency(hz);
	if (FAILED(hr))
		RageException::Throw(
		  hr_ssprintf(hr, "m_pBuffer->SetFrequency(%i)", hz).c_str());
}

void
DSoundBuf::SetVolume(float fVolume)
{
	ASSERT_M(fVolume >= 0 && fVolume <= 1, ssprintf("%f", fVolume));

	if (fVolume == 0)
		fVolume = 0.001f;							 // fix log10f(0) == -INF
	float iVolumeLog2 = log10f(fVolume) / log10f(2); /* vol log 2 */

	/* Volume is a multiplier; SetVolume wants attenuation in hundredths of a
	 * decibel. */
	const int iNewVolume =
	  std::max(static_cast<int>(1000 * iVolumeLog2), DSBVOLUME_MIN);

	if (m_iVolume == iNewVolume)
		return;

	HRESULT hr = m_pBuffer->SetVolume(iNewVolume);
	if (FAILED(hr)) {
		static bool bWarned = false;
		if (!bWarned)
			Locator::getLogger()->warn("{}", hr_ssprintf(
			  hr, "DirectSoundBuffer::SetVolume(%i) failed", iNewVolume));
		bWarned = true;
		return;
	}

	m_iVolume = iNewVolume;
}

/* Determine if "pos" is between "start" and "end", for a circular buffer.  Note
 * that a start/end pos is ambiguous when start == end; it can mean that the
 * buffer is completely full or completely empty; this function treats it as
 * completely empty. */
static bool
contained(int iStart, int iEnd, int iPos)
{
	if (iEnd >= iStart) /* iStart ... iPos ... iEnd */
		return iStart <= iPos && iPos < iEnd;
	else
		return iPos >= iStart || iPos < iEnd;
}

DSoundBuf::~DSoundBuf()
{
	if (m_pBuffer != nullptr)
		m_pBuffer->Release();
	delete[] m_pTempBuffer;
}

/* Check to make sure that, given the current writeahead and chunksize, we're
 * capable of filling the prefetch region entirely.  If we aren't, increase
 * the writeahead.  If this happens, we're underruning. */
void
DSoundBuf::CheckWriteahead(int iCursorStart, int iCursorEnd)
{
	/* If we're in a recovering-from-underrun state, stop. */
	if (m_iExtraWriteahead)
		return;

	/* If the driver is requesting an unreasonably large prefetch, ignore it
	 * entirely. Some drivers seem to give broken write cursors sporadically,
	 * requesting that almost the entire buffer be filled.  There's no reason a
	 * driver should ever need more than 8k frames of writeahead. */
	int iPrefetch = iCursorEnd - iCursorStart;
	wrap(iPrefetch, m_iBufferSize);

	if (iPrefetch >= 1024 * 32) {
		static bool bLogged = false;
		if (bLogged)
			return;
		bLogged = true;

		Locator::getLogger()->warn("Sound driver is requesting an overly large prefetch: wants "
				  "%i (cursor at {}..{}), writeahead not adjusted",
				  iPrefetch / bytes_per_frame(),
				  iCursorStart,
				  iCursorEnd);
		return;
	}

	if (m_iWriteAhead >= iPrefetch)
		return;

	/* We need to increase the writeahead. */
	Locator::getLogger()->trace("insufficient writeahead: wants {} (cursor at {}..{}), "
			   "writeahead adjusted from {} to {}",
			   iPrefetch / bytes_per_frame(), iCursorStart,
			   iCursorEnd, m_iWriteAhead, iPrefetch);

	m_iWriteAhead = iPrefetch;
}

/* Figure out if we've underrun, and act if appropriate. */
void
DSoundBuf::CheckUnderrun(int iCursorStart, int iCursorEnd)
{
	/* If the buffer is full, we can't be underrunning. */
	if (m_iBufferBytesFilled >= m_iBufferSize)
		return;

	/* If nothing is expected to be filled, we can't underrun. */
	if (iCursorStart == iCursorEnd)
		return;

	/* If we're already in a recovering-from-underrun state, stop. */
	if (m_iExtraWriteahead)
		return;

	int iFirstByteFilled = m_iWriteCursor - m_iBufferBytesFilled;
	wrap(iFirstByteFilled, m_iBufferSize);

	/* If the end of the play cursor has data, we haven't underrun. */
	if (m_iBufferBytesFilled > 0 &&
		contained(iFirstByteFilled, m_iWriteCursor, iCursorEnd))
		return;

	/* Extend the writeahead to force fill as much as required to stop
	 * underrunning. This has a major benefit: if we havn't skipped so long
	 * we've passed a whole buffer (64k = ~350ms), this doesn't break stride.
	 * We'll skip forward, but the beat won't be lost, which is a lot easier to
	 * recover from in play. */
	/* XXX: If this happens repeatedly over a period of time, increase
	 * writeahead. */
	/* XXX: What was I doing here?  This isn't working.  We want to know the
	 * writeahead value needed to fill from the current iFirstByteFilled all the
	 * way to iCursorEnd. */
	// int iNeededWriteahead = (iCursorStart + writeahead) - m_iWriteCursor;
	int iNeededWriteahead = iCursorEnd - iFirstByteFilled;
	wrap(iNeededWriteahead, m_iBufferSize);
	if (iNeededWriteahead > m_iWriteAhead) {
		m_iExtraWriteahead = iNeededWriteahead - m_iWriteAhead;
		m_iWriteAhead = iNeededWriteahead;
	}

	int iMissedBy = iCursorEnd - m_iWriteCursor;
	wrap(iMissedBy, m_iBufferSize);

	std::string s = ssprintf(
	  "underrun: %i..%i (%i) filled but cursor at %i..%i; missed it by %i",
	  iFirstByteFilled,
	  m_iWriteCursor,
	  m_iBufferBytesFilled,
	  iCursorStart,
	  iCursorEnd,
	  iMissedBy);

	if (m_iExtraWriteahead)
		s += ssprintf("; extended writeahead by %i to %i",
					  m_iExtraWriteahead,
					  m_iWriteAhead);

	s += "; last: ";
	for (auto& m_iLastCursor : m_iLastCursors)
		s += ssprintf("%i, %i; ", m_iLastCursor[0], m_iLastCursor[1]);

	Locator::getLogger()->trace("{}", s);
}

bool
DSoundBuf::get_output_buf(char** pBuffer, unsigned* pBufferSize, int iChunksize)
{
	ASSERT(!m_bBufferLocked);

	iChunksize *= bytes_per_frame();

	DWORD iCursorStart, iCursorEnd;

	HRESULT result;

	/* It's easiest to think of the cursor as a block, starting and ending at
	 * the two values returned by GetCurrentPosition, that we can't write to. */
	result = m_pBuffer->GetCurrentPosition(&iCursorStart, &iCursorEnd);
	if (result == DSERR_BUFFERLOST) {
		m_pBuffer->Restore();
		result = m_pBuffer->GetCurrentPosition(&iCursorStart, &iCursorEnd);
	}
	if (result != DS_OK) {
		Locator::getLogger()->warn(
		  "{}", hr_ssprintf(result, "DirectSound::GetCurrentPosition failed"));
		return false;
	}

	memmove(&m_iLastCursors[0][0], &m_iLastCursors[1][0], sizeof(int) * 6);
	m_iLastCursors[3][0] = iCursorStart;
	m_iLastCursors[3][1] = iCursorEnd;

	/* Some cards (Creative AudioPCI) have a no-write area even when not
	 * playing.  I'm not sure what that means, but it breaks the assumption that
	 * we can fill the whole writeahead when prebuffering. */
	if (!m_bPlaying)
		iCursorEnd = iCursorStart;

	/*
	 * Some cards (Game Theater XP 7.1 hercwdm.sys 5.12.01.4101 [466688b,
	 * 01-10-2003]) have odd behavior when starting a sound: the start/end
	 * cursors go:
	 *
	 * 0,0             end cursor forced equal to start above (normal)
	 * 4608, 1764      end cursor trailing the write cursor; except with old
	 * emulated WaveOut devices, this shouldn't happen; it indicates that the
	 *                   driver expects almost the whole buffer to be filled.
	 * Also, the play cursor is too far ahead from the last call for the amount
	 *                   of actual time passed.
	 * 704, XXX        start cursor moves back to where it should be.  I don't
	 * have an exact end cursor position, but in general from now on it stays
	 * about 5kb ahead of start (which is where it should be).
	 *
	 * The second call is completely wrong; both the start and end cursors are
	 * meaningless. Detect this: if the end cursor is close behind the start
	 * cursor, don't do anything. (We can't; we have no idea what the cursors
	 * actually are.)
	 */
	{
		int iPrefetch = iCursorEnd - iCursorStart;
		wrap(iPrefetch, m_iBufferSize);

		if (m_iBufferSize - iPrefetch < 1024 * 4) {
			Locator::getLogger()->trace("Strange DirectSound cursor ignored: {}..{}",
					   iCursorStart, iCursorEnd);
			return false;
		}
	}

	/* Update m_iBufferBytesFilled. */
	{
		int iFirstByteFilled = m_iWriteCursor - m_iBufferBytesFilled;
		wrap(iFirstByteFilled, m_iBufferSize);

		/* The number of bytes that have been played since the last time we got
		 * here: */
		int bytes_played = iCursorStart - iFirstByteFilled;
		wrap(bytes_played, m_iBufferSize);

		m_iBufferBytesFilled -= bytes_played;
		m_iBufferBytesFilled = std::max(0, m_iBufferBytesFilled);

		if (m_iExtraWriteahead) {
			int used = std::min(m_iExtraWriteahead, bytes_played);
			std::string s = ssprintf("used %i of %i (%i..%i)",
									 used,
									 m_iExtraWriteahead,
									 iCursorStart,
									 iCursorEnd);
			s += "; last: ";
			for (auto& m_iLastCursor : m_iLastCursors)
				s += ssprintf("%i, %i; ", m_iLastCursor[0], m_iLastCursor[1]);
			Locator::getLogger()->trace("{}", s);
			m_iWriteAhead -= used;
			m_iExtraWriteahead -= used;
		}
	}

	CheckWriteahead(iCursorStart, iCursorEnd);
	CheckUnderrun(iCursorStart, iCursorEnd);

	/* If we already have enough bytes written ahead, stop. */
	if (m_iBufferBytesFilled >= m_iWriteAhead)
		return false;

	/* If we don't have enough free space in the buffer to fill a whole chunk,
	 * stop. */
	if (m_iBufferSize - m_iBufferBytesFilled < iChunksize)
		return false;

	int iNumBytesEmpty = m_iWriteAhead - m_iBufferBytesFilled;
	iNumBytesEmpty = QuantizeUp(iNumBytesEmpty, iChunksize);

	//	LOG->Trace("gave %i at %i (%i, %i) %i filled", iNumBytesEmpty,
	// m_iWriteCursor, cursor, write, m_iBufferBytesFilled);

	/* Lock the audio buffer. */
	result = m_pBuffer->Lock(m_iWriteCursor,
							 iNumBytesEmpty,
							 (LPVOID*)&m_pLockedBuf1,
							 (DWORD*)&m_iLockedSize1,
							 (LPVOID*)&m_pLockedBuf2,
							 (DWORD*)&m_iLockedSize2,
							 0);

	if (result == DSERR_BUFFERLOST) {
		m_pBuffer->Restore();
		result = m_pBuffer->Lock(m_iWriteCursor,
								 iNumBytesEmpty,
								 (LPVOID*)&m_pLockedBuf1,
								 (DWORD*)&m_iLockedSize1,
								 (LPVOID*)&m_pLockedBuf2,
								 (DWORD*)&m_iLockedSize2,
								 0);
	}

	if (result != DS_OK) {
		Locator::getLogger()->warn(
		  "{}", hr_ssprintf(result, "Couldn't lock the DirectSound buffer."));
		return false;
	}

	*pBuffer = m_pTempBuffer;
	*pBufferSize = m_iLockedSize1 + m_iLockedSize2;

	m_iWriteCursor += iNumBytesEmpty;
	if (m_iWriteCursor >= m_iBufferSize)
		m_iWriteCursor -= m_iBufferSize;

	m_iBufferBytesFilled += iNumBytesEmpty;
	m_iWriteCursorPos += iNumBytesEmpty / bytes_per_frame();

	m_bBufferLocked = true;

	return true;
}

void
DSoundBuf::release_output_buf(char* pBuffer, unsigned iBufferSize)
{
	memcpy(m_pLockedBuf1, pBuffer, m_iLockedSize1);
	memcpy(m_pLockedBuf2, pBuffer + m_iLockedSize1, m_iLockedSize2);
	m_pBuffer->Unlock(
	  m_pLockedBuf1, m_iLockedSize1, m_pLockedBuf2, m_iLockedSize2);
	m_bBufferLocked = false;
}

int64_t
DSoundBuf::GetPosition() const
{
	DWORD iCursor, iJunk;
	HRESULT hr = m_pBuffer->GetCurrentPosition(&iCursor, &iJunk);

	if (hr == DSERR_BUFFERLOST) {
		m_pBuffer->Restore();
		hr = m_pBuffer->GetCurrentPosition(&iCursor, &iJunk);
	}
	if (hr != DS_OK) {
		Locator::getLogger()->warn(
		  "{}", hr_ssprintf(hr, "DirectSound::GetPosition failed"));
		iCursor = 0;
	}

	/* This happens occasionally on "Realtek AC97 Audio". */
	if (static_cast<int>(iCursor) == m_iBufferSize)
		iCursor = 0;
	ASSERT_M(static_cast<int>(iCursor) < m_iBufferSize,
			 ssprintf("%i, %i", iCursor, m_iBufferSize));

	int iCursorFrames = static_cast<int>(iCursor) / bytes_per_frame();
	int iWriteCursorFrames = m_iWriteCursor / bytes_per_frame();

	int iFramesBehind = iWriteCursorFrames - iCursorFrames;
	/* iFramesBehind will be 0 if we're called before the buffer starts playing:
	 * both iWriteCursorFrames and iCursorFrames will be 0. */
	if (iFramesBehind < 0)
		iFramesBehind += buffersize_frames(); /* unwrap */

	int64_t iRet = m_iWriteCursorPos - iFramesBehind;

	/* Failsafe: never return a value smaller than we've already returned.
	 * This can happen once in a while in underrun conditions. */
	iRet = std::max(m_iLastPosition, iRet);
	m_iLastPosition = iRet;

	return iRet;
}

void
DSoundBuf::Play()
{
	if (m_bPlaying)
		return;
	m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
	m_bPlaying = true;
}

void
DSoundBuf::Stop()
{
	if (!m_bPlaying)
		return;

	m_pBuffer->Stop();
	m_pBuffer->SetCurrentPosition(0);

	m_iWriteCursorPos = m_iWriteCursor = m_iBufferBytesFilled = 0;
	m_iLastPosition = 0;

	m_iWriteAhead -= m_iExtraWriteahead;
	m_iExtraWriteahead = 0;

	/* When stopped and rewound, the play and write cursors should both be 0. */
	/* This isn't true on some broken cards. */
	//	DWORD iPlay, iWrite;
	//	m_pBuffer->GetCurrentPosition( &iPlay, &iWrite );
	//	ASSERT_M( iPlay == 0 && iWrite == 0, ssprintf("%i, %i", iPlay, iWrite)
	//);

	m_bPlaying = false;
}

/*
 * (c) 2002-2004 Glenn Maynard
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
