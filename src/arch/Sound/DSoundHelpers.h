#ifndef DSOUND_HELPERS
#define DSOUND_HELPERS 1

#ifdef _WIN32
#include <windows.h>
#include <wtypes.h>
#endif

struct IDirectSound;
struct IDirectSoundBuffer;

class DSound
{
  public:
	IDirectSound* GetDS() const { return m_pDS; }
	bool IsEmulated() const;

	DSound();
	~DSound();
	RString Init();

  private:
	IDirectSound* m_pDS;
	static BOOL CALLBACK EnumCallback(LPGUID lpGuid,
									  LPCSTR lpcstrDescription,
									  LPCSTR lpcstrModule,
									  LPVOID lpContext);

	void SetPrimaryBufferMode();
};

class DSoundBuf
{
  public:
	enum hw
	{
		HW_HARDWARE,
		HW_SOFTWARE,
		HW_DONT_CARE
	};

	/* If samplerate is DYNAMIC_SAMPLERATE, then call SetSampleRate before
	 * you use the sample. */
	enum
	{
		DYNAMIC_SAMPLERATE = -1
	};

	DSoundBuf();
	RString Init(DSound& ds,
				 hw hardware,
				 int iChannels,
				 int iSampleRate,
				 int iSampleBits,
				 int iWriteAhead);

	bool get_output_buf(char** pBuffer, unsigned* iBuffersize, int iChunksize);
	void release_output_buf(char* pBuffer, unsigned iBuffersize);

	void Play();
	void Stop();
	void SetVolume(float fVolume);
	void SetSampleRate(int iRate);
	int GetSampleRate() const { return m_iSampleRate; }

	~DSoundBuf();
	int64_t GetPosition() const;
	int64_t GetOutputPosition() const { return m_iWriteCursorPos; }

  private:
	int buffersize_frames() const { return m_iBufferSize / bytes_per_frame(); }
	int bytes_per_frame() const { return m_iChannels * m_iSampleBits / 8; }

	void CheckWriteahead(int iCursorStart, int iCursorEnd);
	void CheckUnderrun(int iCursorStart, int iCursorEnd);

	IDirectSoundBuffer* m_pBuffer;

	int m_iChannels, m_iSampleRate, m_iSampleBits, m_iWriteAhead;
	int m_iVolume;

	int m_iBufferSize;

	int m_iWriteCursor, m_iBufferBytesFilled; /* bytes */
	int m_iExtraWriteahead;
	int64_t m_iWriteCursorPos; /* frames */
	mutable int64_t m_iLastPosition;
	bool m_bPlaying;

	bool m_bBufferLocked;
	char *m_pLockedBuf1, *m_pLockedBuf2;
	int m_iLockedSize1, m_iLockedSize2;
	char* m_pTempBuffer;

	int m_iLastCursors[4][2];
};

#endif
