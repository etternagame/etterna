#ifndef RAGE_SOUND_WAVEOUT_H
#define RAGE_SOUND_WAVEOUT_H

#include "RageSoundDriver.h"
#include "RageUtil/Misc/RageThreads.h"
#include <windows.h>

struct WinWdmStream;
struct WinWdmFilter;

class RageSoundDriver_WDMKS : public RageSoundDriver
{
  public:
	RageSoundDriver_WDMKS();
	~RageSoundDriver_WDMKS();
	std::string Init();

	int64_t GetPosition() const;
	float GetPlayLatency() const;
	int GetSampleRate() const;

  private:
	static int MixerThread_start(void* p);
	void MixerThread();
	bool Fill(int iPacket, std::string& sError);
	void Read(void* pData, int iFrames, int iLastCursorPos, int iCurrentFrame);

	RageThread MixingThread;
	void SetupDecodingThread();

	bool m_bShutdown;
	int m_iLastCursorPos;

	HANDLE m_hSignal;
	WinWdmStream* m_pStream;
	WinWdmFilter* m_pFilter;
};

#endif
