#ifndef RAGE_SOUND_DRIVER_AU_H
#define RAGE_SOUND_DRIVER_AU_H

#include "RageSoundDriver.h"
#include "RageUtil/Misc/RageThreads.h"
#include <AudioUnit/AudioUnit.h>

class RageSoundDriver_AU : public RageSoundDriver
{
  public:
	RageSoundDriver_AU();
	std::string Init();
	~RageSoundDriver_AU();
	float GetPlayLatency() const;
	int GetSampleRate() const { return m_iSampleRate; }
	int64_t GetPosition() const;

  protected:
	void SetupDecodingThread();

  private:
	static OSStatus Render(void* inRefCon,
						   AudioUnitRenderActionFlags* ioActionFlags,
						   const AudioTimeStamp* inTimeStamp,
						   UInt32 inBusNumber,
						   UInt32 inNumberFrames,
						   AudioBufferList* ioData);
	static void NameHALThread(CFRunLoopObserverRef,
							  CFRunLoopActivity activity,
							  void* inRefCon);

	double m_TimeScale;
	AudioUnit m_OutputUnit;
	int m_iSampleRate;
	bool m_bDone;
	bool m_bStarted;
	RageThreadRegister* m_pIOThread;
	RageThreadRegister* m_pNotificationThread;
	RageSemaphore m_Semaphore;
};

#endif
