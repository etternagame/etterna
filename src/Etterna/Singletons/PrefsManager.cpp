#include "Etterna/Globals/global.h"
#include "Etterna/FileTypes/IniFile.h"
#include "LuaManager.h"
#include "Etterna/Models/Misc/Preference.h"
#include "PrefsManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Globals/SpecialFiles.h"

#include <map>

PrefsManager* PREFSMAN =
  nullptr; // global and accessible from anywhere in our program

static const char* MusicWheelUsesSectionsNames[] = {
	"Never",
	"Always",
	"ABCOnly",
};
XToString(MusicWheelUsesSections);
StringToX(MusicWheelUsesSections);
LuaXType(MusicWheelUsesSections);

static const char* MaybeNames[] = {
	"Ask",
	"No",
	"Yes",
};
XToString(Maybe);
StringToX(Maybe);
LuaXType(Maybe);

static const char* GetRankingNameNames[] = {
	"Off",
	"On",
	"List",
};
XToString(GetRankingName);
StringToX(GetRankingName);
LuaXType(GetRankingName);

static const char* RandomBackgroundModeNames[] = {
	"Off",
	"Animations",
};
XToString(RandomBackgroundMode);
StringToX(RandomBackgroundMode);
LuaXType(RandomBackgroundMode);

static const char* ImageCacheModeNames[] = { "Off",
											 "LowResPreload",
											 "LowResLoadOnDemand",
											 "Full" };
XToString(ImageCacheMode);
StringToX(ImageCacheMode);
LuaXType(ImageCacheMode);

static const char* HighResolutionTexturesNames[] = {
	"Auto",
	"ForceOff",
	"ForceOn",
};
XToString(HighResolutionTextures);
StringToX(HighResolutionTextures);
LuaXType(HighResolutionTextures);

static const char* AttractSoundFrequencyNames[] = {
	"Never",	   "EveryTime",	  "Every2Times",
	"Every3Times", "Every4Times", "Every5Times",
};
XToString(AttractSoundFrequency);
StringToX(AttractSoundFrequency);
LuaXType(AttractSoundFrequency);

static const char* BackgroundFitModeNames[] = {
	"CoverDistort",			"CoverPreserve",		"FitInside",
	"FitInsideAvoidLetter", "FitInsideAvoidPillar",
};
XToString(BackgroundFitMode);
StringToX(BackgroundFitMode);
LuaXType(BackgroundFitMode);

#ifdef DEBUG
#define TRUE_IF_DEBUG true
#else
#define TRUE_IF_DEBUG false
#endif

void
ValidateDisplayAspectRatio(float& val)
{
	if (val < 0)
		val = 16 / 9.f;
}

PrefsManager::PrefsManager()
  : m_sCurrentGame("CurrentGame", "")
  , m_sAnnouncer("Announcer", "")
  , m_sDefaultModifiers("DefaultModifiers", "")
  , m_sTheme("Theme", SpecialFiles::BASE_THEME_NAME)
  , m_bFullscreenIsBorderlessWindow("FullscreenIsBorderlessWindow", false)


  , m_bWindowed("Windowed", true)
  , m_sDisplayId("DisplayId", "")
  , m_iDisplayWidth("DisplayWidth", 800)
  , m_iDisplayHeight("DisplayHeight", 600)
  , m_fDisplayAspectRatio("DisplayAspectRatio",
						  16 / 9.f,
						  ValidateDisplayAspectRatio)
  , m_iDisplayColorDepth("DisplayColorDepth", 32)
  , m_iTextureColorDepth("TextureColorDepth", 32)
  , m_iMovieColorDepth("MovieColorDepth", 32)
  , m_bStretchBackgrounds("StretchBackgrounds", false)
  , m_BGFitMode("BackgroundFitMode", BFM_CoverPreserve)
  , m_HighResolutionTextures("HighResolutionTextures",
							 HighResolutionTextures_Auto)
  , m_iMaxTextureResolution("MaxTextureResolution", 1024)
  , m_iRefreshRate("RefreshRate", REFRESH_DEFAULT)
  , m_bAllowMultitexture("AllowMultitexture", true)
  , m_bAllowedLag("AllowedLag", 0.001f)
  , m_bShowStats("ShowStats", TRUE_IF_DEBUG)
  , m_bShowSkips("ShowSkips", true)
  , m_bShowMouseCursor("ShowMouseCursor", true)
  , m_bVsync("Vsync", false)
  , m_FastNoteRendering("FastNoteRendering", true)
  , m_bInterlaced("Interlaced", false)
  , m_bPAL("PAL", false)
  , m_bDelayedTextureDelete("DelayedTextureDeletion", true)
  , m_bDelayedModelDelete("DelayedModelDelete", false)
  , m_ImageCache("CacheImages", IMGCACHE_OFF)
  , m_bFastLoad("FastLoad", true)
  , m_bBlindlyTrustCache("BlindlyTrustCache", true)
  , m_bShrinkSongCache("RemoveCacheEntriesForDeletedSongs", false)
  , m_NeverCacheList("NeverCacheList", "")

  , m_bOnlyDedicatedMenuButtons("OnlyDedicatedMenuButtons", false)
  , m_bMenuTimer("MenuTimer", false)

  , m_fLifeDifficultyScale("LifeDifficultyScale", 1.0f)
  , m_fBGBrightness("BGBrightness", 0.2f)
  , m_bShowBackgrounds("ShowBackgrounds", true)

  , m_bDelayedBack("DelayedBack", false)
  , m_AllowStartToGiveUp("AllowStartToGiveUp", true)
  , m_AllowHoldForOptions("AllowHoldForOptions", true)
  , m_bShowNativeLanguage("ShowNativeLanguage", true)
  , m_bFullTapExplosions("FullTapExplosions", true)
  , m_bNoGlow("NoGlow", false)
  , m_bReplaysUseScoreMods("ReplaysUseScoreMods", true)
  , m_iArcadeOptionsNavigation("ArcadeOptionsNavigation", 0)
  , m_ThreeKeyNavigation("ThreeKeyNavigation", false)
  , m_MusicWheelUsesSections("MusicWheelUsesSections",
							 MusicWheelUsesSections_ALWAYS)
  , m_iMusicWheelSwitchSpeed("MusicWheelSwitchSpeed", 15)
  , m_bSortBySSRNorm("SortBySSRNormPercent", false)
  , m_bPackProgressInWheel("PackProgressInWheel", false)
  , m_bEventMode("EventMode", true)
  , m_MinTNSToHideNotes("MinTNSToHideNotes", TNS_W3)

  , m_ShowSongOptions("ShowSongOptions", Maybe_NO)
  , m_fMinPercentToSaveScores("MinPercentToSaveScores", -1.0f)
  , m_fGlobalOffsetSeconds("GlobalOffsetSeconds", 0)
  , m_sLanguage("Language", "")
  , m_iCenterImageTranslateX("CenterImageTranslateX", 0)
  , m_iCenterImageTranslateY("CenterImageTranslateY", 0)
  , m_fCenterImageAddWidth("CenterImageAddWidth", 0)
  , m_fCenterImageAddHeight("CenterImageAddHeight", 0)
  , EnablePitchRates("EnablePitchRates", true)
  , LiftsOnOsuHolds("LiftsOnOsuHolds", false)
  , m_bEasterEggs("EasterEggs", true)
  , m_AllowMultipleToasties("MultiToasty", false)
  , m_bUseMidGrades("UseMidGrades", false)

  , m_fPadStickSeconds("PadStickSeconds", 0)

  , m_bForceMipMaps("ForceMipMaps", false)
  , m_bTrilinearFiltering("TrilinearFiltering", false)
  , m_bAnisotropicFiltering("AnisotropicFiltering", false)

  , m_sAdditionalSongFolders("AdditionalSongFolders", "")
  , m_sAdditionalFolders("AdditionalFolders", "")

  , m_sDefaultTheme("DefaultTheme", "Rebirth")

  , m_sLastSeenVideoDriver("LastSeenVideoDriver", "")
  , m_sVideoRenderers("VideoRenderers", "")

  , m_bSmoothLines("SmoothLines", false)
  , m_iSoundWriteAhead("SoundWriteAhead", 0)
  , m_iSoundDevice("SoundDevice", "")
  , m_iSoundPreferredSampleRate("SoundPreferredSampleRate", 0)
  , m_bAllowUnacceleratedRenderer("AllowUnacceleratedRenderer", false)
  , m_bThreadedInput("ThreadedInput", true)
  , m_bThreadedMovieDecode("ThreadedMovieDecode", true)
  , m_MuteActions("MuteActions", false)
  , ThreadsToUse("ThreadsToUse", 0)
  , m_bLogToDisk("LogToDisk", true)

  , m_bForceLogFlush("ForceLogFlush", TRUE_IF_DEBUG)
  , m_bShowLogOutput("ShowLogOutput", TRUE_IF_DEBUG)
  , m_bLogSkips("LogSkips", false)
  , m_bShowLoadingWindow("ShowLoadingWindow", true)
  , m_bPseudoLocalize("PseudoLocalize", false)
  , m_show_theme_errors("ShowThemeErrors", false)
  , m_bAlwaysLoadCalcParams("AlwaysLoadCalcParams", false)
  , m_UnfocusedSleepMillisecs("UnfocusedSleepMilliseconds", 10)
  , m_logging_level("LoggingLevel", 2)
  , m_bEnableScoreboard("EnableScoreboard", true)
  , m_bEnableCrashUpload("EnableMinidumpUpload", false)
  , m_bShowMinidumpUploadDialogue("ShowMinidumpUploadDialogue", true)

{
	Init();
	ReadPrefsFromDisk();

	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "PREFSMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}
#undef TRUE_IF_DEBUG

void
PrefsManager::Init()
{
	IPreference::LoadAllDefaults();

	m_mapGameNameToGamePrefs.clear();
}

PrefsManager::~PrefsManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("PREFSMAN");
}

void
PrefsManager::SetCurrentGame(const std::string& sGame)
{
	if (m_sCurrentGame.Get() == sGame)
		return; // redundant

	if (!m_sCurrentGame.Get().empty())
		StoreGamePrefs();

	m_sCurrentGame.Set(sGame);

	RestoreGamePrefs();
}

void
PrefsManager::StoreGamePrefs()
{
	ASSERT(!m_sCurrentGame.Get().empty());

	// save off old values
	GamePrefs& gp = m_mapGameNameToGamePrefs[m_sCurrentGame.ToString()];
	gp.m_sAnnouncer = m_sAnnouncer.Get();
	gp.m_sDefaultModifiers = m_sDefaultModifiers.Get();
}

void
PrefsManager::RestoreGamePrefs()
{
	ASSERT(!m_sCurrentGame.Get().empty());

	// load prefs
	GamePrefs gp;
	const std::map<std::string, GamePrefs>::const_iterator iter =
	  m_mapGameNameToGamePrefs.find(m_sCurrentGame);
	if (iter != m_mapGameNameToGamePrefs.end())
		gp = iter->second;

	m_sAnnouncer.Set(gp.m_sAnnouncer);
	m_sDefaultModifiers.Set(gp.m_sDefaultModifiers);

	// give Static.ini a chance to clobber the saved game prefs
	ReadPrefsFromFile(
	  SpecialFiles::STATIC_INI_PATH, GetPreferencesSection(), true);
}

PrefsManager::GamePrefs::GamePrefs()
  : m_sAnnouncer("")
  , m_sDefaultModifiers("")
{
}

void
PrefsManager::ReadPrefsFromDisk()
{
	ReadDefaultsFromFile(SpecialFiles::DEFAULTS_INI_PATH,
						 GetPreferencesSection());
	IPreference::LoadAllDefaults();

	ReadPrefsFromFile(SpecialFiles::PREFERENCES_INI_PATH, "Options", false);
	ReadGamePrefsFromIni(SpecialFiles::PREFERENCES_INI_PATH);
	ReadPrefsFromFile(
	  SpecialFiles::STATIC_INI_PATH, GetPreferencesSection(), true);

	if (!m_sCurrentGame.Get().empty())
		RestoreGamePrefs();
}

void
PrefsManager::ResetToFactoryDefaults()
{
	// clobber the users prefs by initing then applying defaults
	Init();
	IPreference::LoadAllDefaults();
	ReadPrefsFromFile(
	  SpecialFiles::STATIC_INI_PATH, GetPreferencesSection(), true);

	SavePrefsToDisk();
}

void
PrefsManager::ReadPrefsFromFile(const std::string& sIni,
								const std::string& sSection,
								bool bIsStatic)
{
	IniFile ini;
	if (!ini.ReadFile(sIni))
		return;

	ReadPrefsFromIni(ini, sSection, bIsStatic);
}

static const std::string GAME_SECTION_PREFIX = "Game-";

void
PrefsManager::ReadPrefsFromIni(const IniFile& ini,
							   const std::string& sSection,
							   bool bIsStatic)
{
	// Apply our fallback recursively (if any) before applying ourself.
	static int s_iDepth = 0;
	s_iDepth++;
	ASSERT(s_iDepth < 100);
	std::string sFallback;
	if (ini.GetValue(sSection, "Fallback", sFallback)) {
		ReadPrefsFromIni(ini, sFallback, bIsStatic);
	}
	s_iDepth--;

	const XNode* pChild = ini.GetChild(sSection);
	if (pChild != nullptr)
		IPreference::ReadAllPrefsFromNode(pChild, bIsStatic);
}

void
PrefsManager::ReadGamePrefsFromIni(const std::string& sIni)
{
	IniFile ini;
	if (!ini.ReadFile(sIni))
		return;

	FOREACH_CONST_Child(&ini, section)
	{
		const std::string& section_name = section->GetName();
		if (!BeginsWith(section_name, GAME_SECTION_PREFIX))
			continue;

		std::string sGame = tail(
		  section_name, section_name.length() - GAME_SECTION_PREFIX.length());
		GamePrefs& gp = m_mapGameNameToGamePrefs[sGame];

		ini.GetValue(section_name, "Announcer", gp.m_sAnnouncer);
		ini.GetValue(section_name, "DefaultModifiers", gp.m_sDefaultModifiers);
	}
}

void
PrefsManager::ReadDefaultsFromFile(const std::string& sIni,
								   const std::string& sSection)
{
	IniFile ini;
	if (!ini.ReadFile(sIni))
		return;

	ReadDefaultsFromIni(ini, sSection);
}

void
PrefsManager::ReadDefaultsFromIni(const IniFile& ini,
								  const std::string& sSection)
{
	// Apply our fallback recursively (if any) before applying ourself.
	// TODO: detect circular?
	std::string sFallback;
	if (ini.GetValue(sSection, "Fallback", sFallback))
		ReadDefaultsFromIni(ini, sFallback);

	IPreference::ReadAllDefaultsFromNode(ini.GetChild(sSection));
}

void
PrefsManager::SavePrefsToDisk()
{
	IniFile ini;
	SavePrefsToIni(ini);
	ini.WriteFile(SpecialFiles::PREFERENCES_INI_PATH);
}

void
PrefsManager::SavePrefsToIni(IniFile& ini)
{
	if (!m_sCurrentGame.Get().empty())
		StoreGamePrefs();

	XNode* pNode = ini.GetChild("Options");
	if (pNode == nullptr)
		pNode = ini.AppendChild("Options");
	IPreference::SavePrefsToNode(pNode);

	for (auto& iter : m_mapGameNameToGamePrefs) {
		std::string sSection = "Game-" + std::string(iter.first);

		// todo: write more values here? -aj
		ini.SetValue(sSection, "Announcer", iter.second.m_sAnnouncer);
		ini.SetValue(
		  sSection, "DefaultModifiers", iter.second.m_sDefaultModifiers);
	}
}

std::string
PrefsManager::GetPreferencesSection() const
{
	std::string sSection = "Options";

	// OK if this fails
	GetFileContents(SpecialFiles::TYPE_TXT_FILE, sSection, true);

	// OK if this fails
	if (!GetCommandlineArgument("Type", &sSection))
		Locator::getLogger()->trace("Failed to find Type commandline argument (Not required)");

	return sSection;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the PrefsManager. */
class LunaPrefsManager : public Luna<PrefsManager>
{
  public:
	static int GetPreference(T* p, lua_State* L)
	{
		const std::string sName = SArg(1);
		IPreference* pPref = IPreference::GetPreferenceByName(sName);
		if (pPref == nullptr) {
			LuaHelpers::ReportScriptErrorFmt(
			  "GetPreference: unknown preference \"%s\"", sName.c_str());
			lua_pushnil(L);
			return 1;
		}

		pPref->PushValue(L);
		return 1;
	}
	static int SetPreference(T* p, lua_State* L)
	{
		const std::string sName = SArg(1);

		IPreference* pPref = IPreference::GetPreferenceByName(sName);
		if (pPref == nullptr) {
			LuaHelpers::ReportScriptErrorFmt(
			  "SetPreference: unknown preference \"%s\"", sName.c_str());
			COMMON_RETURN_SELF;
		}

		lua_pushvalue(L, 2);
		pPref->SetFromStack(L);
		COMMON_RETURN_SELF;
	}
	static int SetPreferenceToDefault(T* p, lua_State* L)
	{
		const std::string sName = SArg(1);

		IPreference* pPref = IPreference::GetPreferenceByName(sName);
		if (pPref == nullptr) {
			LuaHelpers::ReportScriptErrorFmt(
			  "SetPreferenceToDefault: unknown preference \"%s\"",
			  sName.c_str());
			COMMON_RETURN_SELF;
		}

		pPref->LoadDefault();
		Locator::getLogger()->info("Restored preference \"{}\" to default \"{}\"",
				   sName.c_str(), pPref->ToString().c_str());
		COMMON_RETURN_SELF;
	}
	static int PreferenceExists(T* p, lua_State* L)
	{
		const std::string sName = SArg(1);

		IPreference* pPref = IPreference::GetPreferenceByName(sName);
		if (pPref == nullptr) {
			lua_pushboolean(L, 0);
			return 1;
		}
		lua_pushboolean(L, 1);
		return 1;
	}

	static int SavePreferences(T* p, lua_State* L)
	{
		p->SavePrefsToDisk();
		COMMON_RETURN_SELF;
	}

	LunaPrefsManager()
	{
		ADD_METHOD(GetPreference);
		ADD_METHOD(SetPreference);
		ADD_METHOD(SetPreferenceToDefault);
		ADD_METHOD(PreferenceExists);
		ADD_METHOD(SavePreferences);
	}
};

LUA_REGISTER_CLASS(PrefsManager)
// lua end
