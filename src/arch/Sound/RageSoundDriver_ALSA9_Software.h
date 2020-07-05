#ifndef RAGE_SOUND_ALSA9_SOFTWARE_H
#define RAGE_SOUND_ALSA9_SOFTWARE_H

#include "RageUtil/Sound/RageSound.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageSoundDriver.h"

#include "ALSA9Helpers.h"

class RageSoundDriver_ALSA9_Software : public RageSoundDriver
{
  public:
	RageSoundDriver_ALSA9_Software();
	~RageSoundDriver_ALSA9_Software();
	std::string Init();

	/* virtuals: */
	int64_t GetPosition() const;
	float GetPlayLatency() const;
	int GetSampleRate() const { return m_iSampleRate; }

	void SetupDecodingThread();

  private:
	static int MixerThread_start(void* p);
	void MixerThread();
	bool GetData();

	bool m_bShutdown;
	int m_iSampleRate;
	Alsa9Buf* m_pPCM;
	RageThread m_MixingThread;
};

#endif
