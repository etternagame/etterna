#ifndef RAGE_SOUND_PULSEAUDIO_H
#define RAGE_SOUND_PULSEAUDIO_H

#include "RageUtil/Sound/RageSound.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageSoundDriver.h"
#include <pulse/pulseaudio.h>

class RageSoundDriver_PulseAudio : public RageSoundDriver
{
  public:
	RageSoundDriver_PulseAudio();
	virtual ~RageSoundDriver_PulseAudio();

	std::string Init();

	inline int64_t GetPosition() const;
	inline int GetSampleRate() const { return m_SampleRate; };

  protected:
	int64_t m_LastPosition;
	int m_SampleRate;
	char* m_Error;

	void m_InitStream();
	RageSemaphore m_Sem;

	pa_threaded_mainloop* m_PulseMainLoop;
	pa_context* m_PulseCtx;
	pa_stream* m_PulseStream;

  public:
	void CtxStateCb(pa_context* c);
	void StreamStateCb(pa_stream* s);
	void StreamWriteCb(pa_stream* s, size_t length);

	static void StaticCtxStateCb(pa_context* c, void* user);
	static void StaticStreamStateCb(pa_stream* s, void* user);
	static void StaticStreamWriteCb(pa_stream* s, size_t length, void* user);
};

#endif /* RAGE_SOUND_PULSEAUDIO_H */
