#ifndef RAGE_SOUND_WAVEOUT
#define RAGE_SOUND_WAVEOUT

#include "RageSoundDriver.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Misc/RageTimer.h"

class RageSoundDriver_OSS : public RageSoundDriver
{
	int fd;

	bool shutdown;
	int last_cursor_pos;
	int samplerate;

	static int MixerThread_start(void* p);
	void MixerThread();
	RageThread MixingThread;

	static std::string CheckOSSVersion(int fd);

  public:
	bool GetData();
	int GetSampleRate() const { return samplerate; }

	/* virtuals: */
	int64_t GetPosition() const;
	float GetPlayLatency() const;
	void SetupDecodingThread();

	RageSoundDriver_OSS();
	std::string Init();
	~RageSoundDriver_OSS();
};

#endif
