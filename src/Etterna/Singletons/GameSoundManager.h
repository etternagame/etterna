#ifndef RAGE_SOUNDS_H
#define RAGE_SOUNDS_H

#include <functional>
#include "Etterna/Models/Misc/PlayerNumber.h"
#include "MessageManager.h"

struct RageSoundParams;
class TimingData;
class Screen;
class RageSound;
struct lua_State;
struct MusicToPlay;

auto
MusicThread_start(void* p) -> int;

/** @brief High-level sound utilities. */
class GameSoundManager : MessageSubscriber
{
  public:
	GameSoundManager();
	~GameSoundManager() override;
	void Update(float fDeltaTime);

	struct PlayMusicParams
	{
		PlayMusicParams()
		{
			pTiming = nullptr;
			bForceLoop = false;
			fStartSecond = 0;
			fLengthSeconds = -1;
			fFadeInLengthSeconds = 0;
			fFadeOutLengthSeconds = 0;
			bAlignBeat = true;
			bApplyMusicRate = false;
			bAccurateSync = false;
		}

		std::string sFile;
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
	void PlayMusic(const std::string& sFile,
				   const TimingData* pTiming = nullptr,
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
	[[nodiscard]] auto GetMusicPath() const -> std::string;
	void Flush();

	void PlayOnce(const std::string& sPath);
	void PlayOnceFromDir(const std::string& sDir);
	void PlayOnceFromAnnouncer(const std::string& sFolderName);

	void HandleSongTimer(bool on = true);
	auto GetFrameTimingAdjustment(float fDeltaTime) -> float;

	static auto GetPlayerBalance(PlayerNumber pn) -> float;
	void WithRageSoundPlaying(std::function<void(RageSound*)> f);
	auto GetPlayingMusicTiming() -> TimingData;

	// Set a sound's position given its pointer
	// Meant to avoid blocking the game execution (stutter)
	void SetSoundPosition(RageSound* s, float fSeconds);

	void SetPlayingMusicParams(RageSoundParams p);

	const RageSoundParams& GetPlayingMusicParams();

	void StartMusic(MusicToPlay& ToPlay);
	void DoPlayOnce(std::string sPath);
	void StartQueuedSounds();
	void DoPlayOnceFromDir(std::string sPath);
	auto SoundWaiting() -> bool;
	void HandleSetPosition();

	void HandleSetParams();

	std::shared_ptr<LuaReference> soundPlayCallback;
	unsigned int recentPCMSamplesBufferSize = 1024;
	Screen* callbackOwningScreen{ nullptr };

	void HandleMessage(const Message& msg) override;

	void ResyncMusicPlaying();

	// Lua
	void PushSelf(lua_State* L);
};

extern GameSoundManager* SOUND;
#endif
