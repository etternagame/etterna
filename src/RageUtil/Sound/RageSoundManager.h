/* RageSoundManager - A global singleton to interface RageSound and
 * RageSoundDriver. */

#ifndef RAGE_SOUND_MANAGER_H
#define RAGE_SOUND_MANAGER_H

#include <map>

class RageSound;
class RageSoundBase;
class RageSoundDriver;
struct RageSoundParams;
class RageSoundReader;
class RageSoundReader_Preload;
class RageTimer;

class RageSoundManager
{
  public:
	RageSoundManager();
	~RageSoundManager();

	/* This may be called when shutting down, in order to stop all sounds.  This
	 * should be called before shutting down threads that may have running
	 * sounds, in order to prevent DirectSound delays and glitches.  Further
	 * attempts to start sounds will do nothing, and threads may be shut down.
	 */
	void Shutdown();

	void Init();

	float GetMixVolume() const { return m_fMixVolume; }
	void SetMixVolume();
	float GetVolumeOfNonCriticalSounds() const
	{
		return m_fVolumeOfNonCriticalSounds;
	}
	void SetVolumeOfNonCriticalSounds(float fVolumeOfNonCriticalSounds);

	void Update();
	void StartMixing(RageSoundBase* snd);		  /* used by RageSound */
	void StopMixing(RageSoundBase* snd);		  /* used by RageSound */
	bool Pause(RageSoundBase* snd, bool bPause);  /* used by RageSound */
	int64_t GetPosition(RageTimer* pTimer) const; /* used by RageSound */
	float GetPlayLatency() const;
	int GetDriverSampleRate() const;

	RageSoundReader* GetLoadedSound(const std::string& sPath);
	void AddLoadedSound(const std::string& sPath,
						RageSoundReader_Preload* pSound);

	void fix_bogus_sound_driver_pref(std::string const& valid_setting);

  private:
	std::map<std::string, RageSoundReader_Preload*> m_mapPreloadedSounds;

	RageSoundDriver* m_pDriver;

	/* Prefs: */
	float m_fMixVolume{ 1.0f };
	float m_fVolumeOfNonCriticalSounds{ 1.0f };
	// Swallow up warnings. If they must be used, define them.
	RageSoundManager& operator=(const RageSoundManager& rhs);
	RageSoundManager(const RageSoundManager& rhs);
};

extern RageSoundManager* SOUNDMAN;

#endif
