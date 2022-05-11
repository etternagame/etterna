/* RageSound - High-level sound object. */

#ifndef RAGE_SOUND_H
#define RAGE_SOUND_H

#include "RageSoundPosMap.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Misc/RageTimer.h"
#include "Etterna/Models/Lua/LuaReference.h"
#include "fft.h"

class RageSoundReader;
struct lua_State;

struct Butter
{
	RageTimer tm;
	RageTimer hwTime;
	RageTimer syncTime;
	float hwPosition;
	float syncPosition;
	float acc;
};

/* Driver interface for sounds: this is what drivers see. */
class RageSoundBase
{
  public:
	virtual ~RageSoundBase() = default;
	virtual void SoundIsFinishedPlaying() = 0;
	virtual auto GetDataToPlay(float* buffer,
							   int size,
							   int64_t& iStreamFrame,
							   int& got_bytes) -> int = 0;
	virtual void CommitPlayingPosition(int64_t iFrameno,
									   int64_t iPosition,
									   int iBytesRead) = 0;
	[[nodiscard]] virtual auto GetStartTime() const -> RageTimer
	{
		return RageZeroTimer;
	}
	[[nodiscard]] virtual auto GetLoadedFilePath() const -> std::string = 0;
};

/**
 * @brief The parameters to play a sound.
 *
 * These are normally changed before playing begins,
 * and are constant from then on. */
struct RageSoundParams
{
	RageSoundParams();

	// The amount of data to play (or loop):
	float m_StartSecond{ 0 };
	float m_LengthSeconds{ -1 };

	// Number of seconds to spend fading in.
	float m_fFadeInSeconds{ 0 };

	// Number of seconds to spend fading out.
	float m_fFadeOutSeconds{ 0 };

	float m_Volume{ 1.0F };
	float m_fAttractVolume{ 1.0F }; // multiplies with m_Volume

	/* Number of samples input and output when changing speed.
	 * Currently, this is either 1/1, 5/4 or 4/5. */
	float m_fPitch{ 1.0F };
	float m_fSpeed{ 1.0F };

	// Accurate Sync (for now only useful for MP3s)
	bool m_bAccurateSync{ false };

	/* Optional driver feature: time to actually start playing sounds.
	 * If zero, or if not supported, the sound will start immediately. */
	RageTimer m_StartTime;

	/** @brief How does the sound stop itself, if it does? */
	enum StopMode_t
	{
		M_STOP,		/**< The sound is stopped at the end. */
		M_LOOP,		/**< The sound restarts itself. */
		M_CONTINUE, /**< Silence is fed at the end to continue timing longer
					   than the sound. */
		M_AUTO /**< The default, the sound stops while obeying filename hints.
				*/
	} /** @brief How does the sound stop itself, if it does? */ StopMode{
		M_AUTO
	};

	bool m_bIsCriticalSound{
		false
	}; // "is a sound that should be played even during attract"
};

struct RageSoundLoadParams
{
	RageSoundLoadParams();

	/* If true, speed and pitch changes will be supported for this sound, at a
	 * small memory penalty if not used. */
	bool m_bSupportRateChanging{ false };

	// If true, panning will be supported for this sound.
	bool m_bSupportPan{ false };
};

template <class T>
class MufftAllocator
{
  public:
	typedef T value_type;
	MufftAllocator() noexcept {};

	T* allocate(size_t n) {	return static_cast<T*>(mufft_alloc(n * sizeof(T)));	}
	void deallocate(T* p, size_t n) { mufft_free(p); }

	template<typename U>
	MufftAllocator(const MufftAllocator<U>& other) throw(){};
};

struct cfloat
{
	float real;
	float imag;
};

class RageSound : public RageSoundBase
{
  public:
	RageSound();
	~RageSound() override;
	RageSound(const RageSound& cpy);
	auto operator=(const RageSound& cpy) -> RageSound&;

	/* If bPrecache == true, we'll preload the entire file into memory if
	 * small enough.  If this is done, a large number of copies of the sound
	 * can be played without much performance penalty.  This is useful for
	 * efficiently playing keyed sounds, and for rapidly-repeating sound
	 * effects, such as the music wheel.
	 *
	 * If cache == false, we'll always stream the sound on demand, which
	 * makes loads much faster.
	 *
	 * If the file failed to load, false is returned, Error() is set
	 * and a null sample will be loaded.  This makes failed loads nonfatal;
	 * they can be ignored most of the time, so we continue to work if a file
	 * is broken or missing.
	 */
	auto Load(const std::string& sFile,
			  bool bPrecache,
			  const RageSoundLoadParams* pParams = nullptr) -> bool;

	/* Using this version means the "don't care" about caching. Currently,
	 * this always will not cache the sound; this may become a preference. */
	auto Load(const std::string& sFile) -> bool;

	/* Load a RageSoundReader that you've set up yourself. Sample rate
	 * conversion will be set up only if needed. Doesn't fail. */
	void LoadSoundReader(RageSoundReader* pSound);

	// Get the loaded RageSoundReader. While playing, only properties can be
	// set.
	auto GetSoundReader() -> RageSoundReader* { return m_pSource; }

	void Unload();
	auto IsLoaded() const -> bool;
	void DeleteSelfWhenFinishedPlaying();

	void StartPlaying(float fGiven = 0, bool forcedTime = false);
	void StopPlaying();

	auto GetError() const -> std::string { return m_sError; }

	void Play(bool is_action, const RageSoundParams* params = nullptr);
	void PlayCopy(bool is_action,
				  const RageSoundParams* pParams = nullptr) const;
	void Stop();

	/* Cleanly pause or unpause the sound. If the sound wasn't already playing,
	 * return true and do nothing. */
	auto Pause(bool bPause) -> bool;
	bool m_bPaused{ false };

	auto GetLengthSeconds() -> float;
	auto GetPositionSeconds(bool* approximate = nullptr,
							RageTimer* Timestamp = nullptr) -> float;
	auto GetLoadedFilePath() const -> std::string override
	{
		return m_sFilePath;
	}
	auto IsPlaying() const -> bool { return m_bPlaying; }

	auto GetPlaybackRate() const -> float;
	auto GetStartTime() const -> RageTimer override;
	void SetParams(const RageSoundParams& p);
	auto GetParams() const -> const RageSoundParams& { return m_Param; }
	auto SetProperty(const std::string& sProperty, float fValue) -> bool;
	void SetStopModeFromString(const std::string& sStopMode);
	void SetPositionSeconds(float fGiven);

	void SetPlayBackCallback(const std::shared_ptr<LuaReference>& f,
							 unsigned int bufSize = 1024);
	std::atomic<bool> pendingPlayBackCall{ false };
	void ExecutePlayBackCallback(Lua* L);

	// Lua
	virtual void PushSelf(lua_State* L);

  private:
	mutable RageMutex m_Mutex;

	RageSoundReader* m_pSource;

	// We keep track of sound blocks we've sent out recently through
	// GetDataToPlay.
	pos_map_queue m_HardwareToStreamMap;
	pos_map_queue m_StreamToSourceMap;

	std::string m_sFilePath;

	void ApplyParams();
	RageSoundParams m_Param;

	/* Current position of the output sound, in frames. If < 0, nothing will
	 * play until it becomes positive. */
	int64_t m_iStreamFrame;

	// For all operations related to sound play callbacks
	std::mutex recentSamplesMutex; 
	unsigned int recentPCMSamplesBufferSize{ 1024 };
	std::shared_ptr<LuaReference> soundPlayCallback;
	std::vector<float, MufftAllocator<float>> recentPCMSamples;
	std::vector<cfloat, MufftAllocator<cfloat>> fftBuffer;
	mufft_plan_1d *fftPlan{ nullptr };

	/* Hack: When we stop a playing sound, we can't ask the driver the position
	 * (we're not playing); and we can't seek back to the current playing
	 * position when we stop (too slow), but we want to be able to report the
	 * position we were at when we stopped without jumping to the last position
	 * we buffered. Keep track of the position after a seek or stop, so we can
	 * return a sane position when stopped, and when playing but pos_map hasn't
	 * yet been filled. */
	int m_iStoppedSourceFrame{ 0 };
	bool m_bPlaying{ false };
	bool m_bDeleteWhenFinished{ false };

	std::string m_sError;

	Butter m_Pasteurizer{};

	auto GetSourceFrameFromHardwareFrame(int64_t iHardwareFrame,
										 bool* bApproximate = nullptr) const
	  -> int;

	auto SetPositionFrames(int frames = -1) -> bool;
	auto GetStopMode() const -> RageSoundParams::StopMode_t; // resolves M_AUTO

	void SoundIsFinishedPlaying() override; // called by sound drivers

  public:
	// These functions are called only by sound drivers.

	/* Returns the number of bytes actually put into pBuffer. If 0 is returned,
	 * it signals the stream to stop; once it's flushed, SoundStopped will be
	 * called. Until then, SOUNDMAN->GetPosition can still be called; the sound
	 * is still playing. */
	auto GetDataToPlay(float* pBuffer,
					   int iSize,
					   int64_t& iStreamFrame,
					   int& iBytesRead) -> int override;
	void CommitPlayingPosition(int64_t iHardwareFrame,
							   int64_t iStreamFrame,
							   int iGotFrames) override;
};

#endif
