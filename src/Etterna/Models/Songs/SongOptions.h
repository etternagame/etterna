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
const RString&
AutosyncTypeToString(AutosyncType cat);
const RString&
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
const RString&
SoundEffectTypeToString(SoundEffectType cat);
const RString&
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
	bool m_bSaveReplay{ false };

	/**
	 * @brief Set up the SongOptions with reasonable defaults.
	 *
	 * This is taken from Init(), but uses the intended
	 * initialization lists. */
	SongOptions() = default;
	void Init();
	void Approach(const SongOptions& other, float fDeltaSeconds);
	void GetMods(vector<RString>& AddTo) const;
	void GetLocalizedMods(vector<RString>& AddTo) const;
	RString GetString() const;
	RString GetLocalizedString() const;
	void FromString(const RString& sOptions);
	bool FromOneModString(const RString& sOneMod,
						  RString& sErrorDetailOut); // On error, return false
													 // and optionally set
													 // sErrorDetailOut

	bool operator==(const SongOptions& other) const;
	bool operator!=(const SongOptions& other) const
	{
		return !operator==(other);
	}

	// Lua
	void PushSelf(lua_State* L);
};

#endif
