#ifndef RAGE_SOUNDS_H
#define RAGE_SOUNDS_H

#include "Etterna/Models/Misc/PlayerNumber.h"
#include "MessageManager.h"

class TimingData;
class Screen;
class RageSound;
struct lua_State;
struct MusicToPlay;

int
MusicThread_start(void* p);

/** @brief High-level sound utilities. */
class GameSoundManager : MessageSubscriber
{
  public:
	GameSoundManager();
	~GameSoundManager();
	void Update(float fDeltaTime);

	struct PlayMusicParams
	{
		PlayMusicParams()
		{
			pTiming = NULL;
			bForceLoop = false;
			fStartSecond = 0;
			fLengthSeconds = -1;
			fFadeInLengthSeconds = 0;
			fFadeOutLengthSeconds = 0;
			bAlignBeat = true;
			bApplyMusicRate = false;
			bAccurateSync = false;
		}

		RString sFile;
		const TimingData* pTiming;
		bool bForceLoop;
		float fStartSecond;
		float fLengthSeconds;
		float fFadeInLengthSeconds;
		float fFadeOutLengthSeconds;
		bool bAlignBeat;
		bool bApplyMusicRate;
		bool bAccurateSync;
	};
	void PlayMusic(PlayMusicParams params,
				   PlayMusicParams FallbackMusicParams = PlayMusicParams());
	void PlayMusic(const RString& sFile,
				   const TimingData* pTiming = NULL,
				   bool force_loop = false,
				   float start_sec = 0,
				   float length_sec = -1,
				   float fFadeInLengthSeconds = 0,
				   float fade_len = 0,
				   bool align_beat = true,
				   bool bApplyMusicRate = false,
				   bool bAccurateSync = false);
	void StopMusic() { PlayMusic(""); }
	void DimMusic(float fVolume, float fDurationSeconds);
	RString GetMusicPath() const;
	void Flush();

	void PlayOnce(const RString& sPath);
	void PlayOnceFromDir(const RString& sDir);
	void PlayOnceFromAnnouncer(const RString& sFolderName);

	void HandleSongTimer(bool on = true);
	float GetFrameTimingAdjustment(float fDeltaTime);

	static float GetPlayerBalance(PlayerNumber pn);
	void WithRageSoundPlaying(std::function<void(RageSound*)> f);
	TimingData GetPlayingMusicTiming();

	// Set a sound's position given its pointer
	// Meant to avoid blocking the game execution (stutter)
	void SetSoundPosition(RageSound* s, float fSeconds);

	void StartMusic(MusicToPlay& ToPlay);
	void DoPlayOnce(RString sPath);
	void StartQueuedSounds();
	void DoPlayOnceFromDir(RString sPath);
	bool SoundWaiting();
	void HandleSetPosition();

	std::shared_ptr<LuaReference> soundPlayCallback;
	unsigned int recentPCMSamplesBufferSize = 1024;
	Screen* callbackOwningScreen{ nullptr };

	void HandleMessage(const Message& msg) override;

	// Lua
	void PushSelf(lua_State* L);
};

extern GameSoundManager* SOUND;
#endif
