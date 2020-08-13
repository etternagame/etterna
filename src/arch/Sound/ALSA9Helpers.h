#ifndef ALSA9_HELPERS_H
#define ALSA9_HELPERS_H

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>

class Alsa9Buf
{
  private:
	int channels, samplebits;
	unsigned samplerate;
	int buffersize;
	int64_t last_cursor_pos;

	snd_pcm_uframes_t preferred_writeahead, preferred_chunksize;
	snd_pcm_uframes_t writeahead, chunksize;

	snd_pcm_t* pcm;

	bool Recover(int r);
	bool SetHWParams();
	bool SetSWParams();

	static void ErrorHandler(const char* file,
							 int line,
							 const char* function,
							 int err,
							 const char* fmt,
							 ...);

  public:
	static void InitializeErrorHandler();
	static void GetSoundCardDebugInfo();
	static std::string GetHardwareID(std::string name = "");

	Alsa9Buf();
	std::string Init(int channels,
				 int iWriteahead,
				 int iChunkSize,
				 int iSampleRate);
	~Alsa9Buf();

	int GetNumFramesToFill();
	bool WaitUntilFramesCanBeFilled(int timeout_ms);
	void Write(const int16_t* buffer, int frames);

	void Play();
	void Stop();
	void SetVolume(float vol);
	int GetSampleRate() const { return samplerate; }

	int64_t GetPosition() const;
	int64_t GetPlayPos() const { return last_cursor_pos; }
};
#endif
