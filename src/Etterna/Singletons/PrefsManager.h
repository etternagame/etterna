#ifndef PREFSMANAGER_H
#define PREFSMANAGER_H

#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/Preference.h"

class IniFile;

void
ValidateDisplayAspectRatio(float& val);

enum MusicWheelUsesSections
{
	MusicWheelUsesSections_NEVER,
	MusicWheelUsesSections_ALWAYS,
	MusicWheelUsesSections_ABC_ONLY,
	NUM_MusicWheelUsesSections,
	MusicWheelUsesSections_Invalid
};
enum Maybe
{
	Maybe_ASK,
	Maybe_NO,
	Maybe_YES,
	NUM_Maybe,
	Maybe_Invalid
};
enum GetRankingName
{
	RANKING_OFF,
	RANKING_ON,
	RANKING_LIST,
	NUM_GetRankingName,
	GetRankingName_Invalid
};
enum RandomBackgroundMode
{
	BGMODE_OFF,
	BGMODE_ANIMATIONS,
	NUM_RandomBackgroundMode,
	RandomBackgroundMode_Invalid
};
enum ImageCacheMode
{
	IMGCACHE_OFF,
	IMGCACHE_LOW_RES_PRELOAD,		 // preload low-res on start
	IMGCACHE_LOW_RES_LOAD_ON_DEMAND, // preload low-res on screen load
	IMGCACHE_FULL,
	NUM_ImageCacheMode,
	ImageCacheMode_Invalid
};
enum HighResolutionTextures
{
	HighResolutionTextures_Auto,
	HighResolutionTextures_ForceOff,
	HighResolutionTextures_ForceOn,
	NUM_HighResolutionTextures,
	HighResolutionTextures_Invalid,
};
enum AttractSoundFrequency
{
	ASF_NEVER,
	ASF_EVERY_TIME,
	ASF_EVERY_2_TIMES,
	ASF_EVERY_3_TIMES,
	ASF_EVERY_4_TIMES,
	ASF_EVERY_5_TIMES,
	NUM_AttractSoundFrequency,
	AttractSoundFrequency_Invalid
};
enum BackgroundFitMode
{
	BFM_CoverDistort,
	BFM_CoverPreserve,
	BFM_FitInside,
	BFM_FitInsideAvoidLetter,
	BFM_FitInsideAvoidPillar,
	NUM_BackgroundFitMode,
	BackgroundFitMode_Invalid
};

/** @brief Holds user-chosen preferences that are saved between sessions. */
class PrefsManager
{
  public:
	PrefsManager();
	~PrefsManager();

	void Init();

	void SetCurrentGame(const std::string& sGame);
	auto GetCurrentGame() -> std::string { return m_sCurrentGame; }

  protected:
	Preference<std::string> m_sCurrentGame;

  public:
	// Game-specific prefs.  Copy these off and save them every time the game
	// changes
	Preference<std::string> m_sAnnouncer;
	Preference<std::string> m_sDefaultModifiers;

  protected:
	void StoreGamePrefs();
	void RestoreGamePrefs();
	struct GamePrefs
	{
		// See GamePrefs::GamePrefs in PrefsManager.cpp for some default
		// settings
		GamePrefs();

		std::string m_sAnnouncer;
		std::string m_sDefaultModifiers;
	};
	std::map<std::string, GamePrefs> m_mapGameNameToGamePrefs;

  public:
	Preference<std::string> m_sTheme;
	Preference<bool> m_bFullscreenIsBorderlessWindow;
	Preference<bool> m_bWindowed;
	Preference<std::string> m_sDisplayId;
	Preference<int> m_iDisplayWidth;
	Preference<int> m_iDisplayHeight;
	Preference<float> m_fDisplayAspectRatio;
	Preference<int> m_iDisplayColorDepth;
	Preference<int> m_iTextureColorDepth;
	Preference<int> m_iMovieColorDepth;
	Preference<bool> m_bStretchBackgrounds;
	Preference<BackgroundFitMode> m_BGFitMode;
	Preference<HighResolutionTextures> m_HighResolutionTextures;
	Preference<int> m_iMaxTextureResolution;
	Preference<int> m_iRefreshRate;
	Preference<bool> m_bAllowMultitexture;
	Preference<float> m_bAllowedLag;
	Preference<bool> m_bShowStats;
	Preference<bool> m_bShowSkips;
	Preference<bool> m_bShowMouseCursor;
	Preference<bool> m_bVsync;
	Preference<bool> m_FastNoteRendering;
	Preference<bool> m_bInterlaced;
	Preference<bool> m_bPAL;
	Preference<bool> m_bDelayedTextureDelete;
	Preference<bool> m_bDelayedModelDelete;
	Preference<ImageCacheMode> m_ImageCache;
	Preference<bool> m_bFastLoad;
	Preference<bool> m_bBlindlyTrustCache;
	Preference<bool> m_bShrinkSongCache;
	Preference<std::string> m_NeverCacheList;

	Preference<bool> m_bOnlyDedicatedMenuButtons;
	Preference<bool> m_bMenuTimer;

	Preference<float> m_fLifeDifficultyScale;
	Preference<float> m_fBGBrightness;
	Preference<bool> m_bShowBackgrounds;

	Preference<bool> m_bDelayedBack;
	Preference<bool> m_AllowStartToGiveUp;
	Preference<bool> m_AllowHoldForOptions;
	Preference<bool> m_bShowNativeLanguage;
	Preference<bool> m_bFullTapExplosions;
	Preference<bool> m_bNoGlow;
	Preference<bool> m_bReplaysUseScoreMods;
	Preference<int> m_iArcadeOptionsNavigation;
	Preference<bool> m_ThreeKeyNavigation;
	Preference<MusicWheelUsesSections> m_MusicWheelUsesSections;
	Preference<int> m_iMusicWheelSwitchSpeed;
	Preference<bool> m_bSortBySSRNorm;
	Preference<bool> m_bPackProgressInWheel;
	Preference<bool> m_bEventMode;
	Preference<TapNoteScore> m_MinTNSToHideNotes;

	Preference<Maybe> m_ShowSongOptions;
	Preference<float> m_fMinPercentToSaveScores;
	Preference<float> m_fGlobalOffsetSeconds;
	Preference<std::string> m_sLanguage;
	Preference<int> m_iCenterImageTranslateX;
	Preference<int> m_iCenterImageTranslateY;
	Preference<int> m_fCenterImageAddWidth;
	Preference<int> m_fCenterImageAddHeight;
	Preference<bool> EnablePitchRates;
	Preference<bool> LiftsOnOsuHolds;
	Preference<bool> m_bEasterEggs;
	Preference<bool> m_AllowMultipleToasties;
	Preference<bool> m_bUseMidGrades;

	// Number of seconds it takes for a button on the controller to release
	// after pressed.
	Preference<float> m_fPadStickSeconds;

	// Useful for non 4:3 displays and resolutions < 640x480 where texels don't
	// std::map directly to pixels.
	Preference<bool> m_bForceMipMaps;
	Preference<bool> m_bTrilinearFiltering; // has no effect without mipmaps on
	Preference<bool> m_bAnisotropicFiltering; // has no effect without mipmaps
											  // on.  Not mutually exclusive
											  // with trilinear.

	Preference<std::string> m_sAdditionalSongFolders;
	Preference<std::string> m_sAdditionalFolders;

	// failsafe
	Preference<std::string> m_sDefaultTheme;

	Preference<std::string> m_sLastSeenVideoDriver;
	Preference<std::string> m_sVideoRenderers; // StepMania.cpp sets these on
											   // first run based on the card
	Preference<bool> m_bSmoothLines;
	Preference<int> m_iSoundWriteAhead;
	Preference<std::string> m_iSoundDevice;
	Preference<int> m_iSoundPreferredSampleRate;
	Preference<bool> m_bAllowUnacceleratedRenderer;
	Preference<bool> m_bThreadedInput;
	Preference<bool> m_bThreadedMovieDecode;
	Preference<bool> m_MuteActions;
	Preference<int> ThreadsToUse;

	// Debug:
	Preference<bool> m_bLogToDisk;
	Preference<bool> m_bForceLogFlush;
	Preference<bool> m_bShowLogOutput;
	Preference<bool> m_bLogSkips;
	Preference<bool> m_bShowLoadingWindow;
	Preference<bool> m_bPseudoLocalize;
	Preference<bool> m_show_theme_errors;
	Preference<bool> m_bAlwaysLoadCalcParams;
	Preference<int> m_UnfocusedSleepMillisecs;

	// logging level 0 - 5
	// 0 = TRACE (all the logging)
	// 1 = DEBUG
	// 2 = INFO
	// 3 = WARN
	// 4 = ERR
	// 5 = FATAL (almost no logging)
	Preference<int> m_logging_level;

	Preference<bool>
	  m_bEnableScoreboard; // Alows disabling of scoreboard in network play

	Preference<bool> m_bEnableCrashUpload;
	Preference<bool> m_bShowMinidumpUploadDialogue;

	void ReadPrefsFromIni(const IniFile& ini,
						  const std::string& sSection,
						  bool bIsStatic);
	void ReadGamePrefsFromIni(const std::string& sIni);
	void ReadDefaultsFromIni(const IniFile& ini, const std::string& sSection);
	void SavePrefsToIni(IniFile& ini);

	void ReadPrefsFromDisk();
	void SavePrefsToDisk();

	void ResetToFactoryDefaults();

	[[nodiscard]] auto GetPreferencesSection() const -> std::string;

	// Lua
	void PushSelf(lua_State* L);

  protected:
	void ReadPrefsFromFile(const std::string& sIni,
						   const std::string& sSection,
						   bool bIsStatic);
	void ReadDefaultsFromFile(const std::string& sIni,
							  const std::string& sSection);
};

extern PrefsManager* PREFSMAN; // global and accessible from anywhere in our program

#endif
