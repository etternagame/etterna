#ifndef RAGE_SOUND_GENERIC_TEST
#define RAGE_SOUND_GENERIC_TEST

#include "DSoundHelpers.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageSoundDriver.h"

class RageSoundDriver_DSound_Software : public RageSoundDriver
{
  public:
	RageSoundDriver_DSound_Software();
	virtual ~RageSoundDriver_DSound_Software();
	std::string Init();

	int64_t GetPosition() const;
	float GetPlayLatency() const;
	int GetSampleRate() const;

  protected:
	void SetupDecodingThread();

  private:
	DSound ds;
	DSoundBuf* m_pPCM;
	int m_iSampleRate;

	bool m_bShutdownMixerThread;

	static int MixerThread_start(void* p);
	void MixerThread();
	RageThread m_MixingThread;
};

#endif
