/* RageSound - High-level sound object. */

#ifndef RAGE_SOUND_H
#define RAGE_SOUND_H

#include "RageSoundPosMap.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Misc/RageTimer.h"
#include "Etterna/Models/Lua/LuaReference.h"

class RageSoundReader;
struct lua_State;

/* Driver interface for sounds: this is what drivers see. */
class RageSoundBase
{
  public:
	virtual ~RageSoundBase() = default;
	virtual void SoundIsFinishedPlaying() = 0;
	virtual int GetDataToPlay(float* buffer,
							  int size,
							  int64_t& iStreamFrame,
							  int& got_bytes) = 0;
	virtual void CommitPlayingPosition(int64_t iFrameno,
									   int64_t iPosition,
									   int iBytesRead) = 0;
	virtual RageTimer GetStartTime() const { return RageZeroTimer; }
	virtual RString GetLoadedFilePath() const = 0;
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

	float m_Volume{ 1.0f };			// multiplies with SOUNDMAN->GetMixVolume()
	float m_fAttractVolume{ 1.0f }; // multiplies with m_Volume

	/* Number of samples input and output when changing speed.
	 * Currently, this is either 1/1, 5/4 or 4/5. */
	float m_fPitch{ 1.0f };
	float m_fSpeed{ 1.0f };

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

class RageSound : public RageSoundBase
{
  public:
	RageSound();
	~RageSound() override;
	RageSound(const RageSound& cpy);
	RageSound& operator=(const RageSound& cpy);

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
	bool Load(const RString& sFile,
			  bool bPrecache,
			  const RageSoundLoadParams* pParams = nullptr);

	/* Using this version means the "don't care" about caching. Currently,
	 * this always will not cache the sound; this may become a preference. */
	bool Load(const RString& sFile);

	/* Load a RageSoundReader that you've set up yourself. Sample rate
	 * conversion will be set up only if needed. Doesn't fail. */
	void LoadSoundReader(RageSoundReader* pSound);

	// Get the loaded RageSoundReader. While playing, only properties can be
	// set.
	RageSoundReader* GetSoundReader() { return m_pSource; }

	void Unload();
	bool IsLoaded() const;
	void DeleteSelfWhenFinishedPlaying();

	void StartPlaying(float fGiven = 0, bool forcedTime = false);
	void StopPlaying();

	RString GetError() const { return m_sError; }

	void Play(bool is_action, const RageSoundParams* params = nullptr);
	void PlayCopy(bool is_action,
				  const RageSoundParams* pParams = nullptr) const;
	void Stop();

	/* Cleanly pause or unpause the sound. If the sound wasn't already playing,
	 * return true and do nothing. */
	bool Pause(bool bPause);

	float GetLengthSeconds();
	float GetPositionSeconds(bool* approximate = nullptr,
							 RageTimer* Timestamp = nullptr) const;
	RString GetLoadedFilePath() const override { return m_sFilePath; }
	bool IsPlaying() const { return m_bPlaying; }

	float GetPlaybackRate() const;
	RageTimer GetStartTime() const override;
	void SetParams(const RageSoundParams& p);
	const RageSoundParams& GetParams() const { return m_Param; }
	bool SetProperty(const RString& sProperty, float fValue);
	void SetStopModeFromString(const RString& sStopMode);
	void SetPositionSeconds(float fGiven);

	void SetPlayBackCallback(shared_ptr<LuaReference> f,
							 unsigned int bufSize = 1024);
	atomic<bool> pendingPlayBackCall{ false };
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

	RString m_sFilePath;

	void ApplyParams();
	RageSoundParams m_Param;

	/* Current position of the output sound, in frames. If < 0, nothing will
	 * play until it becomes positive. */
	int64_t m_iStreamFrame;

	void* fftwBuffer{ nullptr };
	void ActuallySetPlayBackCallback(shared_ptr<LuaReference> f,
									 unsigned int bufSize);
	std::atomic<bool> inPlayCallback{ false };
	std::mutex
	  recentSamplesMutex; // For all operations related to sound play callbacks
	unsigned int recentPCMSamplesBufferSize{ 1024 };
	shared_ptr<LuaReference> soundPlayCallback;
	vector<float> recentPCMSamples;

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

	RString m_sError;

	int GetSourceFrameFromHardwareFrame(int64_t iHardwareFrame,
										bool* bApproximate = nullptr) const;

	bool SetPositionFrames(int frames = -1);
	RageSoundParams::StopMode_t GetStopMode() const; // resolves M_AUTO

	void SoundIsFinishedPlaying() override; // called by sound drivers

  public:
	// These functions are called only by sound drivers.

	/* Returns the number of bytes actually put into pBuffer. If 0 is returned,
	 * it signals the stream to stop; once it's flushed, SoundStopped will be
	 * called. Until then, SOUNDMAN->GetPosition can still be called; the sound
	 * is still playing. */
	int GetDataToPlay(float* pBuffer,
					  int iSize,
					  int64_t& iStreamFrame,
					  int& iBytesRead) override;
	void CommitPlayingPosition(int64_t iHardwareFrame,
							   int64_t iStreamFrame,
							   int iGotFrames) override;
};

#endif
