#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Singletons/GameState.h"
#include "RageUtil/Utils/RageUtil.h"
#include "SongOptions.h"
#include "Etterna/Singletons/DownloadManager.h"

#include "Etterna/Models/Misc/Foreach.h"

static const char* AutosyncTypeNames[] = {
	"Off",
	"Song",
	"Machine",
};
XToString(AutosyncType);
XToLocalizedString(AutosyncType);
LuaXType(AutosyncType);

static const char* SoundEffectTypeNames[] = {
	"Off",
	"Speed",
	"Pitch",
};
XToString(SoundEffectType);
XToLocalizedString(SoundEffectType);
LuaXType(SoundEffectType);

void
SongOptions::Init()
{
	m_bAssistClap = false;
	m_bAssistMetronome = false;
	m_fMusicRate = 1.0f;
	m_SpeedfMusicRate = 1.0f;
	m_AutosyncType = AutosyncType_Off;
	m_SoundEffectType = SoundEffectType_Off;
	m_bStaticBackground = false;
	m_bRandomBGOnly = false;
	m_bSaveScore = true;
}

void
SongOptions::Approach(const SongOptions& other, float fDeltaSeconds)
{
#define APPROACH(opt)                                                          \
	fapproach(m_##opt, other.m_##opt, fDeltaSeconds* other.m_Speed##opt);
#define DO_COPY(x) x = other.x;

	APPROACH(fMusicRate);
	DO_COPY(m_bAssistClap);
	DO_COPY(m_bAssistMetronome);
	DO_COPY(m_AutosyncType);
	DO_COPY(m_SoundEffectType);
	DO_COPY(m_bStaticBackground);
	DO_COPY(m_bRandomBGOnly);
	DO_COPY(m_bSaveScore);
#undef APPROACH
#undef DO_COPY
}

void
SongOptions::GetMods(std::vector<std::string>& AddTo) const
{
	if (m_fMusicRate != 1) {
		auto s = ssprintf("%2.2f", m_fMusicRate);
		if (s[s.size() - 1] == '0')
			s.erase(s.size() - 1);
		AddTo.push_back(s + "xMusic");
	}

	switch (m_AutosyncType) {
		case AutosyncType_Off:
			break;
		case AutosyncType_Song:
			AddTo.push_back("AutosyncSong");
			break;
		case AutosyncType_Machine:
			AddTo.push_back("AutosyncMachine");
			break;
		default:
			FAIL_M(ssprintf("Invalid autosync type: %i", m_AutosyncType));
	}

	switch (m_SoundEffectType) {
		case SoundEffectType_Off:
			break;
		case SoundEffectType_Speed:
			AddTo.push_back("EffectSpeed");
			break;
		case SoundEffectType_Pitch:
			AddTo.push_back("EffectPitch");
			break;
		default:
			FAIL_M(
			  ssprintf("Invalid sound effect type: %i", m_SoundEffectType));
	}

	if (m_bAssistClap)
		AddTo.push_back("Clap");
	if (m_bAssistMetronome)
		AddTo.push_back("Metronome");

	if (m_bStaticBackground)
		AddTo.push_back("StaticBG");
	if (m_bRandomBGOnly)
		AddTo.push_back("RandomBG");
}

void
SongOptions::GetLocalizedMods(std::vector<std::string>& v) const
{
	GetMods(v);
	for (auto& s : v) {
		s = CommonMetrics::LocalizeOptionItem(s, true);
	}
}

std::string
SongOptions::GetString() const
{
	std::vector<std::string> v;
	GetMods(v);
	return join(", ", v);
}

std::string
SongOptions::GetLocalizedString() const
{
	std::vector<std::string> v;
	GetLocalizedMods(v);
	return join(", ", v);
}

/* Options are added to the current settings; call Init() beforehand if
 * you don't want this. */
void
SongOptions::FromString(const std::string& sMultipleMods)
{
	const auto sTemp = sMultipleMods;
	std::vector<std::string> vs;
	split(sTemp, ",", vs, true);
	std::string sThrowAway;
	for (auto& s : vs) {
		FromOneModString(s, sThrowAway);
	}
}

bool
SongOptions::FromOneModString(const std::string& sOneMod,
							  std::string& sErrorOut)
{
	auto sBit = make_lower(sOneMod);
	Trim(sBit);

	Regex mult("^([0-9]+(\\.[0-9]+)?)xmusic$");
	std::vector<std::string> matches;
	if (mult.Compare(sBit, matches)) {
		m_fMusicRate = StringToFloat(matches[0]);
		MESSAGEMAN->Broadcast("RateChanged");
		return true;
	}

	matches.clear();

	std::vector<std::string> asParts;
	split(sBit, " ", asParts, true);
	auto on = true;
	if (asParts.size() > 1) {
		sBit = asParts[1];
		if (asParts[0] == "no")
			on = false;
	}

	if (sBit == "clap")
		m_bAssistClap = on;
	else if (sBit == "metronome")
		m_bAssistMetronome = on;
	else if (sBit == "autosync" || sBit == "autosyncsong")
		m_AutosyncType = on ? AutosyncType_Song : AutosyncType_Off;
	else if (sBit == "autosyncmachine")
		m_AutosyncType = on ? AutosyncType_Machine : AutosyncType_Off;
	else if (sBit == "effect" && !on)
		m_SoundEffectType = SoundEffectType_Off;
	else if (sBit == "effectspeed")
		m_SoundEffectType = on ? SoundEffectType_Speed : SoundEffectType_Off;
	else if (sBit == "effectpitch")
		m_SoundEffectType = on ? SoundEffectType_Pitch : SoundEffectType_Off;
	else if (sBit == "staticbg")
		m_bStaticBackground = on;
	else if (sBit == "randombg")
		m_bRandomBGOnly = on;
	else if (sBit == "savescore")
		m_bSaveScore = on;
	else
		return false;

	return true;
}

bool
SongOptions::operator==(const SongOptions& other) const
{
#define COMPARE(x)                                                             \
	{                                                                          \
		if ((x) != other.x)                                                    \
			return false;                                                      \
	}
	COMPARE(m_fMusicRate);
	COMPARE(m_bAssistClap);
	COMPARE(m_bAssistMetronome);
	COMPARE(m_AutosyncType);
	COMPARE(m_SoundEffectType);
	COMPARE(m_bStaticBackground);
	COMPARE(m_bRandomBGOnly);
	COMPARE(m_bSaveScore);
#undef COMPARE
	return true;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Globals/OptionsBinding.h"

/** @brief Allow Lua to have access to SongOptions. */
class LunaSongOptions : public Luna<SongOptions>
{
  public:
	ENUM_INTERFACE(AutosyncSetting, AutosyncType, AutosyncType);
	// ENUM_INTERFACE(SoundEffectSetting, SoundEffectType, SoundEffectType);
	// Broken, SoundEffectType_Speed disables rate mod, other settings have no
	// effect. -Kyz
	BOOL_INTERFACE(AssistClap, AssistClap);
	BOOL_INTERFACE(AssistMetronome, AssistMetronome);
	BOOL_INTERFACE(StaticBackground, StaticBackground);
	BOOL_INTERFACE(RandomBGOnly, RandomBGOnly);
	SECBOOL_INTERFACE(SaveScore, SaveScore);
	static int MusicRate(T* p, lua_State* L)
	{
		const auto original_top = lua_gettop(L);
		lua_pushnumber(L, p->m_fMusicRate);
		lua_pushnumber(L, p->m_SpeedfMusicRate);
		if (lua_isnumber(L, 1) && original_top >= 1) {
			if (DLMAN->gameplay) {
				Locator::getLogger()->warn(
				  "Attempted to set mod illegally - MusicRate");
				OPTIONAL_RETURN_SELF(original_top);
				return 1;
			}
			const auto v = FArg(1);
			if (!(v > 0.0f && v <= 3.0f)) {
				luaL_error(L, "Invalid value %f", v);
			} else {
				p->m_fMusicRate = v;
				MESSAGEMAN->Broadcast("RateChanged");
			}
		}
		if (original_top >= 2 && lua_isnumber(L, 2)) {
			if (DLMAN->gameplay) {
				Locator::getLogger()->warn(
				  "Attempted to set mod illegally - MusicRate");
				OPTIONAL_RETURN_SELF(original_top);
				return 1;
			}
			p->m_SpeedfMusicRate = FArgGTEZero(L, 2);
		}
		OPTIONAL_RETURN_SELF(original_top);
		return 2;
	}
	LunaSongOptions()
	{
		ADD_METHOD(AutosyncSetting);
		// ADD_METHOD(SoundEffectSetting);
		ADD_METHOD(AssistClap);
		ADD_METHOD(AssistMetronome);
		ADD_METHOD(StaticBackground);
		ADD_METHOD(RandomBGOnly);
		ADD_METHOD(SaveScore);
		ADD_METHOD(MusicRate);
	}
};

LUA_REGISTER_CLASS(SongOptions)
// lua end
