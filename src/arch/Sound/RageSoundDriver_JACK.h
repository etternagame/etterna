#ifndef RAGE_SOUND_JACK
#define RAGE_SOUND_JACK

#include "RageSoundDriver.h"
#include <jack/jack.h>

#define USE_RAGE_SOUND_JACK

class RageSoundDriver_JACK : public RageSoundDriver
{
  public:
	RageSoundDriver_JACK();
	~RageSoundDriver_JACK();

	std::string Init();

	int GetSampleRate() const;
	int64_t GetPosition() const;

  private:
	jack_client_t* client;
	jack_port_t* port_l;
	jack_port_t* port_r;

	int sample_rate;

	// Helper for Init()
	std::string ConnectPorts();

	// JACK callbacks and trampolines
	int ProcessCallback(jack_nframes_t nframes);
	static int ProcessTrampoline(jack_nframes_t nframes, void* arg);
	int SampleRateCallback(jack_nframes_t nframes);
	static int SampleRateTrampoline(jack_nframes_t nframes, void* arg);
};

#endif
