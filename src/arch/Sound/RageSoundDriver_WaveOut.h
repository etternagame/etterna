#ifndef RAGE_SOUND_WAVEOUT_H
#define RAGE_SOUND_WAVEOUT_H

#include "RageSoundDriver.h"
#include "RageUtil/Misc/RageThreads.h"
#include <windows.h>
#include <mmsystem.h>

class RageSoundDriver_WaveOut : public RageSoundDriver
{
  public:
	RageSoundDriver_WaveOut();
	~RageSoundDriver_WaveOut();
	std::string Init();

	int64_t GetPosition() const;
	float GetPlayLatency() const;
	int GetSampleRate() const { return m_iSampleRate; }

  private:
	static int MixerThread_start(void* p);
	void MixerThread();
	RageThread MixingThread;
	bool GetData();
	void SetupDecodingThread();

	HWAVEOUT m_hWaveOut;
	HANDLE m_hSoundEvent;
	WAVEHDR m_aBuffers[8];
	int m_iSampleRate;
	bool m_bShutdown;
	int m_iLastCursorPos;
};

#endif
