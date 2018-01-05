#ifndef PREFSMANAGER_H
#define PREFSMANAGER_H

#include "Preference.h"
#include "GameConstantsAndTypes.h"

class IniFile;

void ValidateDisplayAspectRatio( float &val );

enum MusicWheelUsesSections
{ 
	MusicWheelUsesSections_NEVER, 
	MusicWheelUsesSections_ALWAYS, 
	MusicWheelUsesSections_ABC_ONLY, 
	NUM_MusicWheelUsesSections, 
	MusicWheelUsesSections_Invalid
};
/** @brief The options for allowing the W1 timing. */
enum AllowW1
{ 
	ALLOW_W1_NEVER, /**< The W1 timing is not used. */
	ALLOW_W1_COURSES_ONLY, /**< The W1 timing is used for courses only. */
	ALLOW_W1_EVERYWHERE, /**< The W1 timing is used for all modes. */
	NUM_AllowW1, 
	AllowW1_Invalid
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
	BGMODE_RANDOMMOVIES,
	NUM_RandomBackgroundMode,
	RandomBackgroundMode_Invalid
};
enum ShowDancingCharacters
{
	SDC_Off,
	SDC_Random,
	SDC_Select,
	NUM_ShowDancingCharacters,
	ShowDancingCharacters_Invalid
};
enum ImageCacheMode
{
	IMGCACHE_OFF,
	IMGCACHE_LOW_RES_PRELOAD, // preload low-res on start
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

	void SetCurrentGame( const RString &sGame );
	RString	GetCurrentGame() { return m_sCurrentGame; }
protected:
	Preference<RString>	m_sCurrentGame;

public:
	// Game-specific prefs.  Copy these off and save them every time the game changes
	Preference<RString>	m_sAnnouncer;
	Preference<RString>	m_sTheme;
	Preference<RString>	m_sDefaultModifiers;
protected:
	void StoreGamePrefs();
	void RestoreGamePrefs();
	struct GamePrefs
	{
		// See GamePrefs::GamePrefs in PrefsManager.cpp for some default settings
		GamePrefs();

		RString	m_sAnnouncer;
		RString m_sTheme;
		RString	m_sDefaultModifiers;
	};
	map<RString, GamePrefs> m_mapGameNameToGamePrefs;

public:
	Preference<bool>	m_bWindowed;
	Preference<int>	m_iDisplayWidth;
	Preference<int>	m_iDisplayHeight;
	Preference<float>	m_fDisplayAspectRatio;
	Preference<int>	m_iDisplayColorDepth;
	Preference<int>	m_iTextureColorDepth;
	Preference<int>	m_iMovieColorDepth;
	Preference<bool>	m_bStretchBackgrounds;
	Preference<BackgroundFitMode> m_BGFitMode;
	Preference<HighResolutionTextures>	m_HighResolutionTextures;
	Preference<int>	m_iMaxTextureResolution;
	Preference<int>	m_iRefreshRate;
	Preference<bool>	m_bAllowMultitexture;
	Preference<float>	m_bAllowedLag;
	Preference<bool>	m_bShowStats;
	Preference<bool>	m_bShowSkips;
	Preference<bool>	m_bShowBanners;
	Preference<bool>	m_bShowMouseCursor;
	Preference<bool>	m_bVsync;
	Preference<bool>	m_FastNoteRendering;
	Preference<bool>	m_bInterlaced;
	Preference<bool>	m_bPAL;
	Preference<bool>	m_bDelayedTextureDelete;
	Preference<bool>	m_bDelayedModelDelete;
	Preference<ImageCacheMode>		m_ImageCache;
	Preference<bool>	m_bFastLoad;
	Preference<bool>	m_bBlindlyTrustCache;
	Preference<bool>	m_bShrinkSongCache;
	Preference<RString> m_NeverCacheList;

	Preference<bool>	m_bOnlyDedicatedMenuButtons;
	Preference<bool>	m_bMenuTimer;

	Preference<float>	m_fLifeDifficultyScale;

	// Whoever added these: Please add a comment saying what they do. -Chris
	Preference<int>		m_iRegenComboAfterMiss; // combo that must be met after a Miss to regen life
	Preference<int>		m_iMaxRegenComboAfterMiss; // caps RegenComboAfterMiss if multiple Misses occur in rapid succession
	Preference<bool>	m_bDelayedBack;
	Preference<bool>	m_AllowHoldForOptions;
	Preference<bool>	m_bShowInstructions; // how to play a mode
	Preference<bool>	m_bShowCaution;
	Preference<bool>	m_bShowNativeLanguage;
	Preference<int>	m_iArcadeOptionsNavigation;
	Preference<bool>	m_ThreeKeyNavigation;
	Preference<MusicWheelUsesSections>		m_MusicWheelUsesSections;
	Preference<int>	m_iMusicWheelSwitchSpeed;
	Preference<AllowW1>	m_AllowW1; // this should almost always be on, given use cases. -aj
	Preference<bool>	m_bEventMode;
	Preference<bool>	m_bComboContinuesBetweenSongs;
	Preference<TapNoteScore> m_MinTNSToHideNotes;
	Preference<Maybe>	m_ShowSongOptions;
	Preference<float>	m_fMinPercentToSaveScores;
	Preference<bool>	m_bDisqualification;
	Preference<ShowDancingCharacters>		m_ShowDancingCharacters;
	Preference<float>	m_fGlobalOffsetSeconds;
	Preference<bool>	m_bShowBeginnerHelper;
	Preference<RString>	m_sLanguage;
	Preference<int>	m_iCenterImageTranslateX;
	Preference<int>	m_iCenterImageTranslateY;
	Preference<int>	m_fCenterImageAddWidth;
	Preference<int>	m_fCenterImageAddHeight;
	Preference<AttractSoundFrequency>	m_AttractSoundFrequency;
	Preference<bool> m_DisableUploadDir;
	Preference<bool>	m_bCelShadeModels;
	Preference<bool>	m_bPreferredSortUsesGroups;

	// Number of seconds it takes for a button on the controller to release
	// after pressed.
	Preference<float>	m_fPadStickSeconds;

	// Lead in time before recording starts in edit mode.
	Preference<float> m_EditRecordModeLeadIn;

	// Useful for non 4:3 displays and resolutions < 640x480 where texels don't
	// map directly to pixels.
	Preference<bool>	m_bForceMipMaps;
	Preference<bool>	m_bTrilinearFiltering;		// has no effect without mipmaps on
	Preference<bool>	m_bAnisotropicFiltering;	// has no effect without mipmaps on.  Not mutually exclusive with trilinear.

	// If true, then signatures created when writing profile data and verified
	// when reading profile data. Leave this false if you want to use a profile
	// on different machines that don't have the same key, or else the
	// profile's data will be discarded.
	Preference<bool>	m_bSignProfileData;

	Preference<RString>	m_sAdditionalSongFolders;
	Preference<RString>	m_sAdditionalFolders;

	// failsafe
	Preference<RString>	m_sDefaultTheme;

	Preference<RString>	m_sLastSeenVideoDriver;
	Preference<RString>	m_sVideoRenderers; // StepMania.cpp sets these on first run based on the card
	Preference<bool>	m_bSmoothLines;
	Preference<int>	m_iSoundWriteAhead;
	Preference<RString>	m_iSoundDevice;	
	Preference<int>	m_iSoundPreferredSampleRate;
	Preference<bool>	m_bAllowUnacceleratedRenderer;
	Preference<bool>	m_bThreadedInput;
	Preference<bool>	m_bThreadedMovieDecode;
	Preference<RString>	m_sTestInitialScreen;
	Preference<bool> m_MuteActions;
	Preference<bool> m_bAllowSongDeletion; // Allow the user to remove songs from their collection through UI / keyboard shortcut

	/** @brief Enable some quirky behavior used by some older versions of StepMania. */
	Preference<bool>	m_bQuirksMode;

	// Debug:
	Preference<bool>	m_bLogToDisk;
	Preference<bool>	m_bForceLogFlush;
	Preference<bool>	m_bShowLogOutput;
	Preference<bool>	m_bLogSkips;
	Preference<bool>	m_bLogCheckpoints;
	Preference<bool>	m_bShowLoadingWindow;
	Preference<bool>	m_bPseudoLocalize;
	Preference<bool>	m_show_theme_errors;

#if !defined(WITHOUT_NETWORKING)
	Preference<bool>	m_bEnableScoreboard;  //Alows disabling of scoreboard in network play

	// Check for Updates code
	Preference<bool>	m_bUpdateCheckEnable;
	// TODO - Aldo_MX: Use PREFSMAN->m_iUpdateCheckIntervalSeconds & PREFSMAN->m_iUpdateCheckLastCheckedSecond
	//Preference<int>				m_iUpdateCheckIntervalSeconds;
	//Preference<int>				m_iUpdateCheckLastCheckedSecond;

	// TODO - Aldo_MX: Write helpers in LuaManager.cpp to treat unsigned int/long like LUA Numbers
	//Preference<unsigned long>	m_uUpdateCheckLastCheckedBuild;

#endif

	void ReadPrefsFromIni( const IniFile &ini, const RString &sSection, bool bIsStatic );
	void ReadGamePrefsFromIni( const RString &sIni );
	void ReadDefaultsFromIni( const IniFile &ini, const RString &sSection );
	void SavePrefsToIni( IniFile &ini );

	void ReadPrefsFromDisk();
	void SavePrefsToDisk();

	void ResetToFactoryDefaults();

	RString GetPreferencesSection() const;

	// Lua
	void PushSelf( lua_State *L );

protected:
	void ReadPrefsFromFile( const RString &sIni, const RString &sSection, bool bIsStatic );
	void ReadDefaultsFromFile( const RString &sIni, const RString &sSection );
};

/* This is global, because it can be accessed by crash handlers and error handlers
 * that are run after PREFSMAN shuts down (and probably don't want to deref that
 * pointer anyway). */
extern bool			g_bAutoRestart;

extern PrefsManager*	PREFSMAN;	// global and accessible from anywhere in our program

#endif

/**
 * @file
 * @author Chris Danford, Chris Gomez (c) 2001-2004
 * @section LICENSE
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
