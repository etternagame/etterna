/* SongOptions - Options that apply to an entire song (not per-player). */

#ifndef SONG_OPTIONS_H
#define SONG_OPTIONS_H

#include "Etterna/Models/Misc/EnumHelper.h"

enum AutosyncType
{
	AutosyncType_Off,
	AutosyncType_Song,
	AutosyncType_Machine,
	NUM_AutosyncType,
	AutosyncType_Invalid
};
const std::string&
AutosyncTypeToString(AutosyncType cat);
const std::string&
AutosyncTypeToLocalizedString(AutosyncType cat);
LuaDeclareType(AutosyncType);

enum SoundEffectType
{
	SoundEffectType_Off,
	SoundEffectType_Speed,
	SoundEffectType_Pitch,
	NUM_SoundEffectType,
	SoundEffectType_Invalid
};
const std::string&
SoundEffectTypeToString(SoundEffectType cat);
const std::string&
SoundEffectTypeToLocalizedString(SoundEffectType cat);
LuaDeclareType(SoundEffectType);

class SongOptions
{
  public:
	bool m_bAssistClap{ false };
	bool m_bAssistMetronome{ false };
	float m_fMusicRate{ 1.0f }, m_SpeedfMusicRate{ 1.0f };
	AutosyncType m_AutosyncType{ AutosyncType_Off };
	SoundEffectType m_SoundEffectType{ SoundEffectType_Off };
	bool m_bStaticBackground{ false };
	bool m_bRandomBGOnly{ false };
	bool m_bSaveScore{ true };

	/**
	 * @brief Set up the SongOptions with reasonable defaults.
	 *
	 * This is taken from Init(), but uses the intended
	 * initialization lists. */
	SongOptions() = default;
	void Init();
	void Approach(const SongOptions& other, float fDeltaSeconds);
	void GetMods(std::vector<std::string>& AddTo) const;
	void GetLocalizedMods(std::vector<std::string>& AddTo) const;
	std::string GetString() const;
	std::string GetLocalizedString() const;
	void FromString(const std::string& sOptions);
	bool FromOneModString(const std::string& sOneMod,
						  std::string& sErrorDetailOut); // On error, return
														 // false and optionally
														 // set sErrorDetailOut

	bool operator==(const SongOptions& other) const;
	bool operator!=(const SongOptions& other) const
	{
		return !operator==(other);
	}

	// Lua
	void PushSelf(lua_State* L);
};

#endif
