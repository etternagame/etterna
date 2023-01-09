#include "global.h"
#include "StepMania.h"
#include "Etterna/Globals/rngthing.h"

// Rage global classes
#include "Core/Services/Locator.hpp"
#include "Core/Misc/PlogLogger.hpp"
#include "Core/Crash/CrashpadHandler.hpp"
#include "Core/Misc/AppInfo.hpp"
#include "Core/Platform/Platform.hpp"

#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Misc/RageInput.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Misc/RageTimer.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "arch/Dialog/Dialog.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "Etterna/Models/Misc/CodeDetector.h"
#include "Etterna/Singletons/CommandLineActions.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "RageUtil/Graphics/RageSurface.h"
#include "RageUtil/Graphics/RageSurface_Load.h"
#include "Etterna/Screen/Others/Screen.h"
#include "Etterna/Globals/GameLoop.h"

#if !defined(SUPPORT_OPENGL) && !defined(SUPPORT_D3D)
#define SUPPORT_OPENGL
#endif

// StepMania global classes
#include "Etterna/Singletons/AnnouncerManager.h"
#include "Etterna/Singletons/FilterManager.h"
#include "Etterna/Singletons/FontManager.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/InputFilter.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Singletons/InputQueue.h"
#include "Etterna/Models/Songs/SongCacheIndex.h"
#include "Etterna/Models/Misc/ImageCache.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/ReplayManager.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Actor/Base/ModelManager.h"
#include "Etterna/Singletons/CryptManager.h"
#include "GameLoop.h"
#include "Etterna/Singletons/MessageManager.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "discord_rpc.h"

#include <ctime>

#ifdef _WIN32
#include <windows.h>
int(WINAPIV* __vsnprintf)(char*, size_t, const char*, va_list) = _vsnprintf;
#endif

bool noWindow;

void
ShutdownGame();
bool
HandleGlobalInputs(const InputEventPlus& input);
void
HandleInputEvents(float fDeltaTime);

static Preference<bool> g_bAllowMultipleInstances("AllowMultipleInstances",
												  false);

void
StepMania::GetPreferredVideoModeParams(VideoModeParams& paramsOut)
{
	// resolution handling code that probably needs fixing
	int iWidth = PREFSMAN->m_iDisplayWidth;
	if (PREFSMAN->m_bWindowed) {
		// float fRatio = PREFSMAN->m_iDisplayHeight;
		// iWidth = PREFSMAN->m_iDisplayHeight * fRatio;
		iWidth = static_cast<int>(
		  ceilf(PREFSMAN->m_iDisplayHeight * PREFSMAN->m_fDisplayAspectRatio));
		// ceilf causes the width to come out odd when it shouldn't.
		// 576 * 1.7778 = 1024.0128, which is rounded to 1025. -Kyz
		iWidth -= iWidth % 2;
	}

	paramsOut = VideoModeParams(
	  PREFSMAN->m_bWindowed || PREFSMAN->m_bFullscreenIsBorderlessWindow,
	  PREFSMAN->m_sDisplayId,
	  iWidth,
	  PREFSMAN->m_iDisplayHeight,
	  PREFSMAN->m_iDisplayColorDepth,
	  PREFSMAN->m_iRefreshRate,
	  PREFSMAN->m_bVsync,
	  PREFSMAN->m_bInterlaced,
	  PREFSMAN->m_bSmoothLines,
	  PREFSMAN->m_bTrilinearFiltering,
	  PREFSMAN->m_bAnisotropicFiltering,
	  !PREFSMAN->m_bWindowed && PREFSMAN->m_bFullscreenIsBorderlessWindow,
	  CommonMetrics::WINDOW_TITLE,
	  THEME->GetPathG("Common", "window icon"),
	  PREFSMAN->m_bPAL,
	  PREFSMAN->m_fDisplayAspectRatio);
}

static LocalizedString COLOR("Etterna", "color");
static LocalizedString TEXTURE("Etterna", "texture");
static LocalizedString WINDOWED("Etterna", "Windowed");
static LocalizedString FULLSCREEN("Etterna", "Fullscreen");
static LocalizedString ANNOUNCER_("Etterna", "Announcer");
static LocalizedString VSYNC("Etterna", "Vsync");
static LocalizedString NO_VSYNC("Etterna", "NoVsync");
static LocalizedString SMOOTH_LINES("Etterna", "SmoothLines");
static LocalizedString NO_SMOOTH_LINES("Etterna", "NoSmoothLines");

static std::string
GetActualGraphicOptionsString()
{
	const VideoModeParams& params = (*DISPLAY->GetActualVideoModeParams());
	std::string sFormat = "%s %s %dx%d %d " + COLOR.GetValue() + " %d " +
						  TEXTURE.GetValue() + " %dHz %s %s";
	std::string sLog =
	  ssprintf(sFormat,
			   DISPLAY->GetApiDescription().c_str(),
			   (params.windowed ? WINDOWED : FULLSCREEN).GetValue().c_str(),
			   params.width,
			   params.height,
			   params.bpp,
			   static_cast<int>(PREFSMAN->m_iTextureColorDepth),
			   params.rate,
			   (params.vsync ? VSYNC : NO_VSYNC).GetValue().c_str(),
			   (PREFSMAN->m_bSmoothLines ? SMOOTH_LINES : NO_SMOOTH_LINES)
				 .GetValue()
				 .c_str());
	return sLog;
}

static void
StoreActualGraphicOptions()
{
	/* Store the settings that RageDisplay was actually able to use so that
	 * we don't go through the process of auto-detecting a usable video mode
	 * every time. */
	const VideoModeParams& params = (*DISPLAY->GetActualVideoModeParams());
	PREFSMAN->m_bWindowed.Set(params.windowed &&
							  !params.bWindowIsFullscreenBorderless);
	if (!params.windowed && !params.bWindowIsFullscreenBorderless) {
		// In all other cases, want to preserve the value of this preference,
		// but if DISPLAY decides to go fullscreen exclusive, we'll persist that
		// decision
		PREFSMAN->m_bFullscreenIsBorderlessWindow.Set(false);
	}
	/* If we're windowed, we may have tweaked the width based on the aspect
	 * ratio. Don't save this new value over the preferred value. */
	if (!PREFSMAN->m_bWindowed) {
		PREFSMAN->m_iDisplayWidth.Set(params.width);
		PREFSMAN->m_iDisplayHeight.Set(params.height);
	}
	PREFSMAN->m_iDisplayColorDepth.Set(params.bpp);
	if (PREFSMAN->m_iRefreshRate != REFRESH_DEFAULT)
		PREFSMAN->m_iRefreshRate.Set(params.rate);
	PREFSMAN->m_bVsync.Set(params.vsync);

	Dialog::SetWindowed(params.windowed);
}

static RageDisplay*
CreateDisplay();

bool
StepMania::GetHighResolutionTextures()
{
	switch (PREFSMAN->m_HighResolutionTextures) {
		default:
		case HighResolutionTextures_Auto: {
			int height = PREFSMAN->m_iDisplayHeight;
			return height > THEME->GetMetricI("Common", "ScreenHeight");
		}
		case HighResolutionTextures_ForceOn:
			return true;
		case HighResolutionTextures_ForceOff:
			return false;
	}
}

static void
update_centering()
{
	DISPLAY->ChangeCentering(PREFSMAN->m_iCenterImageTranslateX,
							 PREFSMAN->m_iCenterImageTranslateY,
							 PREFSMAN->m_fCenterImageAddWidth,
							 PREFSMAN->m_fCenterImageAddHeight);
}

static void
StartDisplay()
{
	if (DISPLAY != nullptr)
		return; // already started

	DISPLAY = CreateDisplay();

	update_centering();

	TEXTUREMAN = new RageTextureManager;
	TEXTUREMAN->SetPrefs(
	  RageTextureManagerPrefs(PREFSMAN->m_iTextureColorDepth,
							  PREFSMAN->m_iMovieColorDepth,
							  PREFSMAN->m_bDelayedTextureDelete,
							  PREFSMAN->m_iMaxTextureResolution,
							  StepMania::GetHighResolutionTextures(),
							  PREFSMAN->m_bForceMipMaps));

	MODELMAN = new ModelManager;
	MODELMAN->SetPrefs(ModelManagerPrefs(PREFSMAN->m_bDelayedModelDelete));
}

void
StepMania::ApplyGraphicOptions()
{
	bool bNeedReload = false;

	VideoModeParams params;
	GetPreferredVideoModeParams(params);
	std::string sError = DISPLAY->SetVideoMode(std::move(params), bNeedReload);
	if (!sError.empty())
		RageException::Throw("%s", sError.c_str());

	update_centering();

	bNeedReload |= TEXTUREMAN->SetPrefs(
	  RageTextureManagerPrefs(PREFSMAN->m_iTextureColorDepth,
							  PREFSMAN->m_iMovieColorDepth,
							  PREFSMAN->m_bDelayedTextureDelete,
							  PREFSMAN->m_iMaxTextureResolution,
							  StepMania::GetHighResolutionTextures(),
							  PREFSMAN->m_bForceMipMaps));

	bNeedReload |=
	  MODELMAN->SetPrefs(ModelManagerPrefs(PREFSMAN->m_bDelayedModelDelete));

	if (bNeedReload)
		TEXTUREMAN->ReloadAll();

	StoreActualGraphicOptions();
	if (SCREENMAN)
		SCREENMAN->SystemMessage(GetActualGraphicOptionsString());

	// Give the input handlers a chance to re-open devices as necessary.
	INPUTMAN->WindowReset();
}

static bool
CheckVideoDefaultSettings();

void
StepMania::ResetPreferences()
{
	PREFSMAN->ResetToFactoryDefaults();
	SOUNDMAN->SetMixVolume();
	CheckVideoDefaultSettings();
	ApplyGraphicOptions();
}

/* Shutdown all global singletons. Note that this may be called partway through
 * initialization, due to an object failing to initialize, in which case some of
 * these may still be NULL. */
void
ShutdownGame()
{
	/* First, tell SOUNDMAN that we're shutting down. This signals sound drivers
	 * to stop sounds, which we want to do before any threads that may have
	 * started sounds are closed; this prevents annoying DirectSound glitches
	 * and delays. */
	if (SOUNDMAN != nullptr)
		SOUNDMAN->Shutdown();

	SAFE_DELETE(SCREENMAN);
	SAFE_DELETE(STATSMAN);
	SAFE_DELETE(MESSAGEMAN);
	SAFE_DELETE(NSMAN);
	/* Delete INPUTMAN before the other INPUTFILTER handlers, or an input
	 * driver may try to send a message to INPUTFILTER after we delete it. */
	SAFE_DELETE(INPUTMAN);
	SAFE_DELETE(INPUTQUEUE);
	SAFE_DELETE(INPUTMAPPER);
	SAFE_DELETE(INPUTFILTER);
	SAFE_DELETE(MODELMAN);
	SAFE_DELETE(PROFILEMAN); // PROFILEMAN needs the songs still loaded
	SAFE_DELETE(CRYPTMAN);
	SAFE_DELETE(SONGMAN);
	SAFE_DELETE(IMAGECACHE);
	SAFE_DELETE(SONGINDEX);
	SAFE_DELETE(SOUND); // uses GAMESTATE, PREFSMAN
	SAFE_DELETE(PREFSMAN);
	SAFE_DELETE(GAMESTATE);
	SAFE_DELETE(GAMEMAN);
	SAFE_DELETE(NOTESKIN);
	SAFE_DELETE(THEME);
	SAFE_DELETE(ANNOUNCER);
	SAFE_DELETE(SOUNDMAN);
	SAFE_DELETE(FONT);
	SAFE_DELETE(TEXTUREMAN);
	SAFE_DELETE(DISPLAY);
	Dialog::Shutdown();
	DLMAN.reset();
	SAFE_DELETE(FILEMAN);
	SAFE_DELETE(LUA);
	Discord_Shutdown();
}

static void
HandleException(const std::string& sError)
{

	// Shut down first, so we exit graphics mode before trying to open a dialog.
	ShutdownGame();

	// Throw up a pretty error dialog.
	Dialog::Error(sError);
	Dialog::Shutdown(); // Shut it back down.
}

void
StepMania::ResetGame()
{
	GAMESTATE->Reset();

	// if somehow the current theme loaded does not exist anymore
	// reset to a real one
	if (!THEME->DoesThemeExist(THEME->GetCurThemeName())) {
		std::string sGameName = GAMESTATE->GetCurrentGame()->m_szName;
		if (!THEME->DoesThemeExist(sGameName))
			sGameName =
			  PREFSMAN->m_sDefaultTheme.Get(); // was previously "default" -aj
		THEME->SwitchThemeAndLanguage(
		  sGameName, THEME->GetCurLanguage(), PREFSMAN->m_bPseudoLocalize);
		TEXTUREMAN->DoDelayedDelete();
	}

	PREFSMAN->SavePrefsToDisk();
}

ThemeMetric<std::string> INITIAL_SCREEN("Common", "InitialScreen");
std::string
StepMania::GetInitialScreen()
{
	std::string screen_name = INITIAL_SCREEN.GetValue();
	if (!SCREENMAN->IsScreenNameValid(screen_name)) {
		screen_name = "ScreenInitialScreenIsInvalid";
	}
	return screen_name;
}
ThemeMetric<std::string> SELECT_MUSIC_SCREEN("Common", "SelectMusicScreen");
std::string
StepMania::GetSelectMusicScreen()
{
	return SELECT_MUSIC_SCREEN.GetValue();
}

#ifdef _WIN32
static Preference<int> g_iLastSeenMemory("LastSeenMemory", 0);
#endif

static void
AdjustForChangedSystemCapabilities()
{
#ifdef _WIN32
	// Has the amount of memory changed?
	MEMORYSTATUS mem;
	GlobalMemoryStatus(&mem);

	const int Memory = mem.dwTotalPhys / (1024 * 1024);

	if (g_iLastSeenMemory == Memory)
		return;

	Locator::getLogger()->trace("Memory changed from {} to {}; settings changed",
			   g_iLastSeenMemory.Get(),
			   Memory);
	g_iLastSeenMemory.Set(Memory);

	// is this assumption outdated? -aj
	/* Let's consider 128-meg systems low-memory, and 256-meg systems
	 * high-memory. Cut off at 192. This is pretty conservative; many 128-meg
	 * systems can deal with higher memory profile settings, but some can't.
	 *
	 * Actually, Windows lops off a meg or two; cut off a little lower to treat
	 * 192-meg systems as high-memory. */
	const bool HighMemory = (Memory >= 190);

	/* Two memory-consuming features that we can disable are texture caching and
	 * preloaded banners. Texture caching can use a lot of memory; disable it
	 * for low-memory systems. */
	PREFSMAN->m_bDelayedTextureDelete.Set(HighMemory);

	PREFSMAN->SavePrefsToDisk();
#endif
}

#ifdef _WIN32
#include "RageUtil/Graphics/RageDisplay_D3D.h"
#include "archutils/Win32/VideoDriverInfo.h"
#endif

#if defined(SUPPORT_OPENGL)
#include "RageUtil/Graphics/RageDisplay_OGL.h"
#endif

#if defined(SUPPORT_GLES2)
#include "RageUtil/Graphics/RageDisplay_GLES2.h"
#endif

#include "RageUtil/Graphics/RageDisplay_Null.h"

struct VideoCardDefaults
{
	std::string sDriverRegex;
	std::string sVideoRenderers;
	int iWidth;
	int iHeight;
	int iDisplayColor;
	int iTextureColor;
	int iMovieColor;
	int iTextureSize;
	bool bSmoothLines;

	VideoCardDefaults() = default;
	VideoCardDefaults(std::string sDriverRegex_,
					  std::string sVideoRenderers_,
					  int iWidth_,
					  int iHeight_,
					  int iDisplayColor_,
					  int iTextureColor_,
					  int iMovieColor_,
					  int iTextureSize_,
					  bool bSmoothLines_)
	{
		sDriverRegex = sDriverRegex_;
		sVideoRenderers = sVideoRenderers_;
		iWidth = iWidth_;
		iHeight = iHeight_;
		iDisplayColor = iDisplayColor_;
		iTextureColor = iTextureColor_;
		iMovieColor = iMovieColor_;
		iTextureSize = iTextureSize_;
		bSmoothLines = bSmoothLines_;
	}
} const g_VideoCardDefaults[] = {
#ifdef _WINDOWS
	VideoCardDefaults("", "d3d, opengl", 800, 600, 32, 32, 32, 1024, false)
#else
	VideoCardDefaults(
	  "Voodoo *5",
	  "d3d,opengl", // received 3 reports of opengl crashing. -Chris
	  640,
	  480,
	  32,
	  32,
	  32,
	  1024,
	  false // accelerated
	  ),
	VideoCardDefaults(
	  "Voodoo|3dfx", // all other Voodoos: some drivers don't identify which one
	  "d3d,opengl",
	  640,
	  480,
	  16,
	  16,
	  16,
	  256,
	  false // broken, causes black screen
	  ),
	VideoCardDefaults(
	  "Radeon.* 7|Wonder 7500|ArcadeVGA", // Radeon 7xxx, RADEON Mobility 7500
	  "d3d,opengl", // movie texture performance is terrible in OpenGL, but fine
					// in D3D.
	  640,
	  480,
	  16,
	  16,
	  16,
	  1024,
	  false // accelerated
	  ),
	VideoCardDefaults("GeForce|Radeon|Wonder 9|Quadro",
					  "opengl,d3d",
					  640,
					  480,
					  32,
					  32,
					  32, // 32 bit textures are faster to load
					  1024,
					  false // hardware accelerated
					  ),
	VideoCardDefaults("TNT|Vanta|M64",
					  "opengl,d3d",
					  640,
					  480,
					  16,
					  16,
					  16, // Athlon 1.2+TNT demonstration w/ movies: 70fps w/
						  // 32bit textures, 86fps w/ 16bit textures
					  1024,
					  false // hardware accelerated
					  ),
	VideoCardDefaults("G200|G250|G400",
					  "d3d,opengl",
					  640,
					  480,
					  16,
					  16,
					  16,
					  1024,
					  false // broken, causes black screen
					  ),
	VideoCardDefaults("Savage",
					  "d3d",
					  // OpenGL is unusable on my Savage IV with even the latest
					  // drivers. It draws 30 frames of gibberish then crashes.
					  // This happens even with simple NeHe demos. -Chris
					  640,
					  480,
					  16,
					  16,
					  16,
					  1024,
					  false),
	VideoCardDefaults(
	  "XPERT@PLAY|IIC|RAGE PRO|RAGE LT PRO", // Rage Pro chip, Rage IIC chip
	  "d3d",
	  // OpenGL is not hardware accelerated, despite the fact that the
	  // drivers come with an ICD.  Also, the WinXP driver performance
	  // is terrible and supports only 640. The ATI driver is usable.
	  // -Chris
	  320,
	  240, // lower resolution for 60fps. In-box WinXP driver doesn't support
		   // 400x300.
	  16,
	  16,
	  16,
	  256,
	  false),
	VideoCardDefaults(
	  "RAGE MOBILITY-M1",
	  "d3d,opengl", // Vertex alpha is broken in OpenGL, but not D3D. -Chris
	  400,
	  300, // lower resolution for 60fps
	  16,
	  16,
	  16,
	  256,
	  false),
	VideoCardDefaults("Mobility M3", // ATI Rage Mobility 128 (AKA "M3")
					  "d3d,opengl",	 // bad movie texture performance in opengl
					  640,
					  480,
					  16,
					  16,
					  16,
					  1024,
					  false),
	VideoCardDefaults("Intel.*82810|Intel.*82815",
					  "opengl,d3d", // OpenGL is 50%+ faster than D3D w/ latest
									// Intel drivers.  -Chris
					  512,
					  384, // lower resolution for 60fps
					  16,
					  16,
					  16,
					  512,
					  false),
	VideoCardDefaults("Intel*Extreme Graphics",
					  "d3d", // OpenGL blue screens w/ XP drivers from 6-21-2002
					  640,
					  480,
					  16,
					  16,
					  16, // slow at 32bpp
					  1024,
					  false),
	VideoCardDefaults("Intel.*", /* fallback: all unknown Intel cards to D3D,
									since Intel is notoriously bad at OpenGL */
					  "d3d,opengl",
					  640,
					  480,
					  16,
					  16,
					  16,
					  1024,
					  false),
	VideoCardDefaults(
	  // Cards that have problems with OpenGL:
	  // ASSERT fail somewhere in RageDisplay_OpenGL "Trident Video Accelerator
	  // CyberBlade" bug 764499: ASSERT fail after glDeleteTextures for "SiS
	  // 650_651_740" bug 764830: ASSERT fail after glDeleteTextures for "VIA
	  // Tech VT8361/VT8601 Graphics Controller" bug 791950: AV in
	  // glsis630!DrvSwapBuffers for "SiS 630/730"
	  "Trident Video Accelerator CyberBlade|VIA.*VT|SiS 6*",
	  "d3d,opengl",
	  640,
	  480,
	  16,
	  16,
	  16,
	  1024,
	  false),
	VideoCardDefaults(
	  /* Unconfirmed texture problems on this; let's try D3D, since it's
	   * a VIA/S3 chipset. */
	  "VIA/S3G KM400/KN400",
	  "d3d,opengl",
	  640,
	  480,
	  16,
	  16,
	  16,
	  1024,
	  false),
	VideoCardDefaults(
	  "OpenGL", // This matches all drivers in Mac and Linux. -Chris
	  "opengl",
	  640,
	  480,
	  16,
	  16,
	  16,
	  1024,
	  true // Right now, they've got to have NVidia or ATi Cards anyway..
	  ),
	VideoCardDefaults(
	  // Default graphics settings used for all cards that don't match above.
	  // This must be the very last entry!
	  "",
	  "opengl,d3d",
	  640,
	  480,
	  32,
	  32,
	  32,
	  1024,
	  false // AA is slow on some cards, so let's selectively enable HW
			// accelerated cards.
	  )
#endif
};

static std::string
GetVideoDriverName()
{
#ifdef _WIN32
	return GetPrimaryVideoDriverName();
#else
	return "OpenGL";
#endif
}

bool
CheckVideoDefaultSettings()
{
	// Video card changed since last run
	std::string sVideoDriver = GetVideoDriverName();

	Locator::getLogger()->info("Last seen video driver: {}",
				PREFSMAN->m_sLastSeenVideoDriver.Get().c_str());

	// allow players to opt out of the forced reset when a new video card is
	// detected - mina
	static Preference<bool> TKGP("ResetVideoSettingsWithNewGPU", true);

	VideoCardDefaults defaults;

	unsigned i;
	for (i = 0; i < ARRAYLEN(g_VideoCardDefaults); i++) {
		defaults = g_VideoCardDefaults[i];

		std::string sDriverRegex = defaults.sDriverRegex;
		Regex regex(sDriverRegex);
		if (regex.Compare(sVideoDriver)) {
			Locator::getLogger()->trace("Card matches '{}'.", sDriverRegex.size() ? sDriverRegex.c_str() : "(unknown card)");
			break;
		}
	}
	if (i >= ARRAYLEN(g_VideoCardDefaults)) {
		FAIL_M("Failed to match video driver");
	}

	bool bSetDefaultVideoParams = false;
	if (PREFSMAN->m_sVideoRenderers.Get().empty()) {
		bSetDefaultVideoParams = true;
		Locator::getLogger()->trace("Applying defaults for {}.", sVideoDriver.c_str());
	} else if (PREFSMAN->m_sLastSeenVideoDriver.Get() != sVideoDriver) {
		bSetDefaultVideoParams = true;
		Locator::getLogger()->trace("Video card has changed from {} to {}.  Applying new defaults.",
		  PREFSMAN->m_sLastSeenVideoDriver.Get().c_str(),
		  sVideoDriver.c_str());
	}

	if (bSetDefaultVideoParams) {
		if (TKGP) {
			PREFSMAN->m_sVideoRenderers.Set(defaults.sVideoRenderers);
			PREFSMAN->m_iDisplayWidth.Set(defaults.iWidth);
			PREFSMAN->m_iDisplayHeight.Set(defaults.iHeight);
			PREFSMAN->m_iDisplayColorDepth.Set(defaults.iDisplayColor);
			PREFSMAN->m_iTextureColorDepth.Set(defaults.iTextureColor);
			PREFSMAN->m_iMovieColorDepth.Set(defaults.iMovieColor);
			PREFSMAN->m_iMaxTextureResolution.Set(defaults.iTextureSize);
			PREFSMAN->m_bSmoothLines.Set(defaults.bSmoothLines);
			PREFSMAN->m_fDisplayAspectRatio.Set(
			  PREFSMAN->m_fDisplayAspectRatio);
		}

		// Update last seen video card
		PREFSMAN->m_sLastSeenVideoDriver.Set(GetVideoDriverName());
	} else if (CompareNoCase(PREFSMAN->m_sVideoRenderers.Get(),
							 defaults.sVideoRenderers)) {
		Locator::getLogger()->warn("Video renderer list has been changed from '{}' to '{}'",
				  defaults.sVideoRenderers.c_str(), PREFSMAN->m_sVideoRenderers.Get().c_str());
	}

	Locator::getLogger()->info("Video renderers: '{}'", PREFSMAN->m_sVideoRenderers.Get().c_str());
	return bSetDefaultVideoParams;
}

static LocalizedString ERROR_INITIALIZING_CARD(
  "Etterna",
  "There was an error while initializing your video card.");
static LocalizedString ERROR_DONT_FILE_BUG("Etterna",
										   "Please do not file this error as a "
										   "bug!  Use the web page below to "
										   "troubleshoot this problem.");
static LocalizedString ERROR_VIDEO_DRIVER("Etterna", "Video Driver: %s");
static LocalizedString ERROR_NO_VIDEO_RENDERERS(
  "Etterna",
  "No video renderers attempted.");
static LocalizedString ERROR_INITIALIZING("Etterna", "Initializing %s...");
static LocalizedString ERROR_UNKNOWN_VIDEO_RENDERER(
  "Etterna",
  "Unknown video renderer value: %s");

RageDisplay*
CreateDisplay()
{
	/* We never want to bother users with having to decide which API to use.
	 *
	 * Some cards simply are too troublesome with OpenGL to ever use it, eg.
	 * Voodoos. If D3D8 isn't installed on those, complain and refuse to run (by
	 * default). For others, always use OpenGL.  Allow forcing to D3D as an
	 * advanced option.
	 *
	 * If we're missing acceleration when we load D3D8 due to a card being in
	 * the D3D list, it means we need drivers and that they do exist.
	 *
	 * If we try to load OpenGL and we're missing acceleration, it may mean:
	 *  1. We're missing drivers, and they just need upgrading.
	 *  2. The card doesn't have drivers, and it should be using D3D8.
	 *     In other words, it needs an entry in this table.
	 *  3. The card doesn't have drivers for either.  (Sorry, no S3 868s.)
	 *     Can't play.
	 * In this case, fail to load; don't silently fall back on D3D.  We don't
	 * want people unknowingly using D3D8 with old drivers (and reporting
	 * obscure bugs due to driver problems).  We'll probably get bug reports for
	 * all three types. #2 is the only case that's actually a bug.
	 *
	 * Actually, right now we're falling back. I'm not sure which behavior is
	 * better.
	 */

	// bool bAppliedDefaults = CheckVideoDefaultSettings();
	CheckVideoDefaultSettings();

	VideoModeParams params;
	StepMania::GetPreferredVideoModeParams(params);

	std::string error =
	  ERROR_INITIALIZING_CARD.GetValue() + "\n\n" +
	  ERROR_DONT_FILE_BUG.GetValue() + "\n\n" +
	  ssprintf(ERROR_VIDEO_DRIVER.GetValue(), GetVideoDriverName().c_str()) +
	  "\n\n";

	std::vector<std::string> asRenderers;
	split(PREFSMAN->m_sVideoRenderers, ",", asRenderers, true);

	if (asRenderers.empty())
		RageException::Throw("%s", ERROR_NO_VIDEO_RENDERERS.GetValue().c_str());

	RageDisplay* pRet = nullptr;
	if (noWindow) {
		return new RageDisplay_Null;
	} else {
		for (unsigned i = 0; i < asRenderers.size(); i++) {
			std::string sRenderer = asRenderers[i];

			if (CompareNoCase(sRenderer, "opengl") == 0) {
#if defined(SUPPORT_OPENGL)
				pRet = new RageDisplay_Legacy;
#endif
			} else if (CompareNoCase(sRenderer, "gles2") == 0) {
#if defined(SUPPORT_GLES2)
				pRet = new RageDisplay_GLES2;
#endif
			} else if (CompareNoCase(sRenderer, "d3d") == 0) {
// TODO: ANGLE/RageDisplay_Modern
#if defined(SUPPORT_D3D)
				pRet = new RageDisplay_D3D;
#endif
			} else if (CompareNoCase(sRenderer, "null") == 0) {
				return new RageDisplay_Null;
			} else {
				RageException::Throw(
				  ERROR_UNKNOWN_VIDEO_RENDERER.GetValue().c_str(),
				  sRenderer.c_str());
			}

			if (pRet == nullptr)
				continue;

			std::string sError =
			  pRet->Init(std::move(params), PREFSMAN->m_bAllowUnacceleratedRenderer);
			if (!sError.empty()) {
				error +=
				  ssprintf(ERROR_INITIALIZING.GetValue(), sRenderer.c_str()) +
				  "\n" + sError;
				SAFE_DELETE(pRet);
				error += "\n\n\n";
				continue;
			}

			break; // the display is ready to go
		}
	}

	if (pRet == nullptr)
		RageException::Throw("%s", error.c_str());

	return pRet;
}

static void
SwitchToLastPlayedGame()
{
	const Game* pGame = GAMEMAN->StringToGame(PREFSMAN->GetCurrentGame());

	// If the active game type isn't actually available, revert to the default.
	if (pGame == nullptr)
		pGame = GAMEMAN->GetDefaultGame();

	if (!GAMEMAN->IsGameEnabled(pGame) && pGame != GAMEMAN->GetDefaultGame()) {
		pGame = GAMEMAN->GetDefaultGame();
		Locator::getLogger()->warn(R"(Default NoteSkin for "{}" missing, reverting to "{}")",
				  pGame->m_szName, GAMEMAN->GetDefaultGame()->m_szName);
	}

	ASSERT(GAMEMAN->IsGameEnabled(pGame));

	StepMania::InitializeCurrentGame(pGame);
}

// This function is meant to only be called during start up.
void
StepMania::InitializeCurrentGame(const Game* g)
{
	ASSERT(g != NULL);
	ASSERT(GAMESTATE != NULL);
	ASSERT(ANNOUNCER != NULL);
	ASSERT(THEME != NULL);

	GAMESTATE->SetCurGame(g);

	std::string sAnnouncer = PREFSMAN->m_sAnnouncer;
	std::string sTheme = PREFSMAN->m_sTheme;
	std::string sGametype = GAMESTATE->GetCurrentGame()->m_szName;
	std::string sLanguage = PREFSMAN->m_sLanguage;

	if (sAnnouncer.empty())
		sAnnouncer = GAMESTATE->GetCurrentGame()->m_szName;
	std::string argCurGame;
	if (GetCommandlineArgument("game", &argCurGame) &&
		argCurGame != sGametype) {
		Game const* new_game = GAMEMAN->StringToGame(argCurGame);
		if (new_game == nullptr) {
			Locator::getLogger()->warn("{} is not a known game type, ignoring.", argCurGame.c_str());
		} else {
			PREFSMAN->SetCurrentGame(sGametype);
			GAMESTATE->SetCurGame(new_game);
		}
	}

	// It doesn't matter if sTheme is blank or invalid, THEME->STAL will set
	// a selectable theme for us. -Kyz

	// process gametype, theme and language command line arguments;
	// these change the preferences in order for transparent loading -aj
	std::string argTheme;
	if (GetCommandlineArgument("theme", &argTheme) && argTheme != sTheme) {
		sTheme = argTheme;
		// set theme in preferences too for correct behavior  -aj
		PREFSMAN->m_sTheme.Set(sTheme);
	}

	std::string argLanguage;
	if (GetCommandlineArgument("language", &argLanguage)) {
		sLanguage = argLanguage;
		// set language in preferences too for correct behavior -aj
		PREFSMAN->m_sLanguage.Set(sLanguage);
	}

	// it's OK to call these functions with names that don't exist.
	ANNOUNCER->SwitchAnnouncer(sAnnouncer);
	THEME->SwitchThemeAndLanguage(
	  sTheme, sLanguage, PREFSMAN->m_bPseudoLocalize);

	// Set the input scheme for the new game, and load keymaps.
	if (INPUTMAPPER != nullptr) {
		INPUTMAPPER->SetInputScheme(&g->m_InputScheme);
		INPUTMAPPER->ReadMappingsFromDisk();
	}
}

static void
WriteLogHeader()
{
	if (g_argc > 1) {
		std::string args;
		for (int i = 1; i < g_argc; ++i) {
			if (i > 1)
				args += " ";

			// surround all params with some marker, as they might have
			// whitespace. using [[ and ]], as they are not likely to be in the
			// params.
			args += ssprintf("[[%s]]", g_argv[i]);
		}
		Locator::getLogger()->info("Command line args (count={}): {}", (g_argc - 1), args.c_str());
	}
}

static LocalizedString COULDNT_OPEN_LOADING_WINDOW(
  "LoadingWindow",
  "Couldn't open any loading windows.");

int
sm_main(int argc, char* argv[])
{
	g_RandomNumberGenerator.seed(static_cast<unsigned int>(time(nullptr)));
	seed_lua_prng();

	// Initialize Logging
    Locator::provide(std::make_unique<PlogLogger>());

    // Init Crash Handling
	bool success = Core::Crash::initCrashpad();
	if(!success)
	    Locator::getLogger()->warn("Crash Handler could not be initialized. Crash reports will not be created.");

    // Log App and System Information
    Locator::getLogger()->info("{} v{} - Build {}",
                               Core::AppInfo::APP_TITLE,
                               Core::AppInfo::APP_VERSION,
                               Core::AppInfo::GIT_HASH);
    Locator::getLogger()->info("System: {}", Core::Platform::getSystem());
    Locator::getLogger()->info("CPU: {}", Core::Platform::getSystemCPU());
	Locator::getLogger()->info("System Architecture: {}", Core::Platform::getArchitecture());
	Locator::getLogger()->info("Total Memory: {}GB", Core::Platform::getSystemMemory() / pow(1024, 3));

    // Run Platform Initialization
    Core::Platform::init();

	RageThreadRegister thread("Main thread");
	RageException::SetCleanupHandler(HandleException);

	SetCommandlineArguments(argc, argv);

	LUA = new LuaManager;

	MESSAGEMAN = new MessageManager;

	// Initialize the file extension type lists so everything can ask ActorUtil
	// what the type of a file is.
	ActorUtil::InitFileTypeLists();

	// Almost everything uses this to read and write files.  Load this early.
	FILEMAN = new RageFileManager(argv[0]);
	FILEMAN->Mount("dir", Core::Platform::getAppDirectory(), "/");

	// load preferences and mount any alternative trees.
	PREFSMAN = new PrefsManager;

	/* Allow ArchHooks to check for multiple instances.  We need to do this after
	 * PREFS is initialized, so ArchHooks can use a preference to turn this off.
	 * We want to do this before ApplyLogPreferences, so if we exit because of
	 * another instance, we don't try to clobber its log.  We also want to do
	 * this before opening the loading window, so if we give focus away, we
	 * don't flash the window. */
	if (!g_bAllowMultipleInstances.Get() && Core::Platform::isOtherInstanceRunning(argc, argv)) {
	    Locator::getLogger()->warn("Multiple instances are disabled. Other instance detected. Shutting down...");
		ShutdownGame();
		return 0;
	}

	WriteLogHeader();

	// Set up alternative filesystem trees.
	if (!PREFSMAN->m_sAdditionalFolders.Get().empty()) {
		std::vector<std::string> dirs;
		split(PREFSMAN->m_sAdditionalFolders, ",", dirs, true);
		for (unsigned i = 0; i < dirs.size(); i++)
			FILEMAN->Mount("dir", dirs[i], "/");
	}
	if (!PREFSMAN->m_sAdditionalSongFolders.Get().empty()) {
		std::vector<std::string> dirs;
		split(PREFSMAN->m_sAdditionalSongFolders, ",", dirs, true);
		for (unsigned i = 0; i < dirs.size(); i++)
			FILEMAN->Mount("dir", dirs[i], "/AdditionalSongs");
	}

	/* One of the above filesystems might contain files that affect preferences
	 * (e.g. Data/Static.ini). Re-read preferences. */
	PREFSMAN->ReadPrefsFromDisk();

    // Setup options that require preference variables
    // Used to be contents of ApplyLogPreferences
    Core::Crash::setShouldUpload(PREFSMAN->m_bEnableCrashUpload);
    Core::Platform::setConsoleEnabled(PREFSMAN->m_bShowLogOutput);
	Locator::getLogger()->info("Logging level {} (0 - TRACE | 5 - FATAL)",
							   PREFSMAN->m_logging_level.Get());
	Locator::getLogger()->setLogLevel(
	  static_cast<Core::ILogger::Severity>(PREFSMAN->m_logging_level.Get()));

	// This needs PREFSMAN.
	Dialog::Init();

	// Create game objects

	GAMESTATE = new GameState;

	std::vector<std::string> arguments(argv + 1, argv + argc);
	noWindow = std::any_of(arguments.begin(), arguments.end(), [](string str) {
		return str == "notedataCache";
	});

	// This requires PREFSMAN, for PREFSMAN->m_bShowLoadingWindow.
	LoadingWindow* pLoadingWindow = nullptr;
	if (!noWindow) {
		pLoadingWindow = LoadingWindow::Create();
		if (pLoadingWindow == nullptr)
			RageException::Throw(
			  "%s", COULDNT_OPEN_LOADING_WINDOW.GetValue().c_str());
	}

#if defined(HAVE_TLS)
	Locator::getLogger()->info("TLS is {}available", RageThread::GetSupportsTLS() ? "" : "not ");
#endif

	AdjustForChangedSystemCapabilities();

	GAMEMAN = new GameManager;
	THEME = new ThemeManager;
	ANNOUNCER = new AnnouncerManager;
	NOTESKIN = new NoteSkinManager;

	// Switch to the last used game type, and set up the theme and announcer.
	SwitchToLastPlayedGame();

	CommandLineActions::Handle(pLoadingWindow);

	if (!noWindow) {
		/* Now that THEME is loaded, load the icon and splash for the current
		 * theme into the loading window. */
		std::string sError;
		RageSurface* pSurface = RageSurfaceUtils::LoadFile(
		  THEME->GetPathG("Common", "window icon"), sError);
		if (pSurface != nullptr)
			pLoadingWindow->SetIcon(pSurface);
		delete pSurface;
		pSurface = RageSurfaceUtils::LoadFile(
		  THEME->GetPathG("Common", "splash"), sError);
		if (pSurface != nullptr)
			pLoadingWindow->SetSplash(pSurface);
		delete pSurface;
	}

	if (PREFSMAN->m_iSoundWriteAhead)
		Locator::getLogger()->info("Sound writeahead has been overridden to {}", PREFSMAN->m_iSoundWriteAhead.Get());

	SONGINDEX = new SongCacheIndex;
	SOUNDMAN = new RageSoundManager;
	SOUNDMAN->Init();
	SOUNDMAN->SetMixVolume();
	SOUND = new GameSoundManager;
	INPUTFILTER = new InputFilter;
	INPUTMAPPER = new InputMapper;

	StepMania::InitializeCurrentGame(GAMESTATE->GetCurrentGame());

	INPUTQUEUE = new InputQueue;
	IMAGECACHE = new ImageCache;

	// depends on SONGINDEX:
	SONGMAN = new SongManager;
	SONGINDEX->StartTransaction();
	SONGMAN->InitAll(pLoadingWindow); // this takes a long time
	SONGINDEX->FinishTransaction();
	CRYPTMAN = new CryptManager; // need to do this before ProfileMan
	SCOREMAN = new ScoreManager;
	DLMAN = std::make_shared<DownloadManager>();
	REPLAYS = std::make_shared<ReplayManager>();
	PROFILEMAN = new ProfileManager;
	PROFILEMAN->Init(pLoadingWindow); // must load after SONGMAN
	SONGMAN->CalcTestStuff();		  // must be after profileman init

	NSMAN = new NetworkSyncManager(pLoadingWindow);
	STATSMAN = new StatsManager;

	FILTERMAN = new FilterManager;

	/* If the user has tried to quit during the loading, do it before creating
	 * the main window. This prevents going to full screen just to quit. */
	if (GameLoop::hasUserQuit()) {
		ShutdownGame();
		return 0;
	}
	if (!noWindow)
		SAFE_DELETE(pLoadingWindow);
	StartDisplay();

	StoreActualGraphicOptions();
	Locator::getLogger()->info(GetActualGraphicOptionsString().c_str());

	/* Input handlers can have dependences on the video system so
	 * INPUTMAN must be initialized after DISPLAY. */
	INPUTMAN = new RageInput;

	// These things depend on the TextureManager, so do them after!
	FONT = new FontManager;
	SCREENMAN = new ScreenManager;

	StepMania::ResetGame();

	/* Now that GAMESTATE is reset, tell SCREENMAN to update the theme (load
	 * overlay screens and global sounds), and load the initial screen. */
	SCREENMAN->ThemeChanged();
	SCREENMAN->SetNewScreen(StepMania::GetInitialScreen());

	// Do this after ThemeChanged so that we can show a system message
	std::string sMessage;
	if (INPUTMAPPER->CheckForChangedInputDevicesAndRemap(sMessage))
		SCREENMAN->SystemMessage(sMessage);

	CodeDetector::RefreshCacheItems();

	if (GetCommandlineArgument("netip"))
		NSMAN->DisplayStartupStatus(); // If we're using networking show what
									   // happened

	// Run the main loop.
	GameLoop::RunGameLoop();

	PREFSMAN->SavePrefsToDisk();

	ShutdownGame();

	return 0;
}

std::string
StepMania::SaveScreenshot(const std::string& Dir,
						  bool SaveCompressed,
						  const std::string& NamePrefix,
						  const std::string& NameSuffix)
{
	/* As of sm-ssc v1.0 rc2, screenshots are no longer named by an arbitrary
	 * index. This was causing naming issues for some unknown reason, so we have
	 * changed the screenshot names to a non-blocking format: date and time.
	 * As before, we ignore the extension. -aj */
	std::string FileNameNoExtension =
	  NamePrefix + DateTime::GetNowDateTime().GetString() + NameSuffix;
	// replace space with underscore.
	s_replace(FileNameNoExtension, " ", "_");
	// colons are illegal in filenames.
	s_replace(FileNameNoExtension, ":", "");

	// Save the screenshot. If writing lossy to a memcard, use
	// SAVE_LOSSY_LOW_QUAL, so we don't eat up lots of space.
	RageDisplay::GraphicsFileFormat fmt;
	if (SaveCompressed)
		fmt = RageDisplay::SAVE_LOSSY_HIGH_QUAL;
	else
		fmt = RageDisplay::SAVE_LOSSLESS_SENSIBLE;

	std::string FileName =
	  FileNameNoExtension + "." + (SaveCompressed ? "jpg" : "png");
	std::string Path = Dir + FileName;
	bool Result = DISPLAY->SaveScreenshot(Path, fmt);
	if (!Result) {
		SCREENMAN->PlayInvalidSound();
		return std::string();
	}

	SCREENMAN->PlayScreenshotSound();

	return FileName;
}


/* Returns true if the key has been handled and should be discarded, false if
 * the key should be sent on to screens. */
static LocalizedString SERVICE_SWITCH_PRESSED("Etterna",
											  "Service switch pressed");
static LocalizedString RELOADED_METRICS("ThemeManager", "Reloaded metrics");
static LocalizedString RELOADED_METRICS_AND_TEXTURES(
  "ThemeManager",
  "Reloaded metrics and textures");
static LocalizedString RELOADED_SCRIPTS("ThemeManager", "Reloaded scripts");
static LocalizedString RELOADED_OVERLAY_SCREENS("ThemeManager",
												"Reloaded overlay screens");
bool
HandleGlobalInputs(const InputEventPlus& input)
{
	// None of the globals keys act on types other than FIRST_PRESS
	if (input.type != IET_FIRST_PRESS)
		return false;

	switch (input.MenuI) {
		case GAME_BUTTON_OPERATOR:
			/* Global operator key, to get quick access to the options menu.
			 * Don't do this if we're on a "system menu", which includes the
			 * editor (to prevent quitting without storing changes). */
			if (SCREENMAN->AllowOperatorMenuButton()) {
				bool bIsCtrlHeld =
				  INPUTFILTER->IsBeingPressed(
					DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL),
					&input.InputList) ||
				  INPUTFILTER->IsBeingPressed(
					DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL), &input.InputList);
				if (bIsCtrlHeld) // Operator is rebound to OPERATOR + Ctrl
				{
					SCREENMAN->SystemMessage(SERVICE_SWITCH_PRESSED);
					SCREENMAN->PopAllScreens();
					SCREENMAN->set_input_redirected(PLAYER_1, false);
					GAMESTATE->Reset();
					SCREENMAN->SetNewScreen(
					  CommonMetrics::OPERATOR_MENU_SCREEN);
					return true;
				}
			}
		default:
			break;
	}

	/* Re-added for StepMania 3.9 theming veterans, plus it's just faster than
	 * the debug menu. The Shift button only reloads the metrics, unlike in 3.9
	 * (where it saved bookkeeping and machine profile). -aj */
	bool bIsShiftHeld =
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT),
								  &input.InputList) ||
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT),
								  &input.InputList);
	bool bIsCtrlHeld =
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL),
								  &input.InputList) ||
	  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL),
								  &input.InputList);
	if (input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F2)) {
		if (bIsShiftHeld && !bIsCtrlHeld) {
			// Shift+F2: refresh metrics,noteskin cache and CodeDetector cache
			// only
			THEME->ReloadMetrics();
			NOTESKIN->RefreshNoteSkinData(GAMESTATE->m_pCurGame);
			CodeDetector::RefreshCacheItems();
			SCREENMAN->SystemMessage(RELOADED_METRICS);
		} else if (bIsCtrlHeld && !bIsShiftHeld) {
			// Ctrl+F2: reload scripts only
			THEME->UpdateLuaGlobals();
			SCREENMAN->SystemMessage(RELOADED_SCRIPTS);
		} else if (bIsCtrlHeld && bIsShiftHeld) {
			// Shift+Ctrl+F2: reload overlay screens (and metrics, since themers
			// are likely going to do this after changing metrics.)
			THEME->ReloadMetrics();
			SCREENMAN->ReloadOverlayScreens();
			SCREENMAN->SystemMessage(RELOADED_OVERLAY_SCREENS);
		} else {
			// F2 alone: refresh metrics, textures, noteskins, codedetector
			// cache
			THEME->ReloadMetrics();
			TEXTUREMAN->ReloadAll();
			NOTESKIN->RefreshNoteSkinData(GAMESTATE->m_pCurGame);
			CodeDetector::RefreshCacheItems();
			SCREENMAN->SystemMessage(RELOADED_METRICS_AND_TEXTURES);
		}

		return true;
	}

	if (input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_PAUSE)) {
		Message msg("ToggleConsoleDisplay");
		MESSAGEMAN->Broadcast(msg);
		return true;
	}

#if !defined(__APPLE__)
	if (input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F4)) {
		if (INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RALT),
										&input.InputList) ||
			INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LALT),
										&input.InputList)) {
			// pressed Alt+F4
			GameLoop::setUserQuit();
			return true;
		}
	}
#else
	if (input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_Cq) &&
		(INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LMETA),
									 &input.InputList) ||
		 INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RMETA),
									 &input.InputList))) {
		/* The user quit is handled by the menu item so we don't need to set it
		 * here; however, we do want to return that it has been handled since
		 * this will happen first. */
		return true;
	}
#endif

	bool bDoScreenshot =
#ifdef __APPLE__
	  // Notebooks don't have F13. Use cmd-F12 as well.
	  input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_PRTSC) ||
	  input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F13) ||
	  (input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F12) &&
	   (INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LMETA),
									&input.InputList) ||
		INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RMETA),
									&input.InputList)));
#else
	  /* The default Windows message handler will capture the desktop window
	   * upon pressing PrntScrn, or will capture the foreground with focus upon
	   * pressing Alt+PrntScrn. Windows will do this whether or not we save a
	   * screenshot ourself by dumping the frame buffer. */
	  // "if pressing PrintScreen and not pressing Alt"
	  input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_PRTSC) &&
	  !INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LALT),
								   &input.InputList) &&
	  !INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RALT),
								   &input.InputList);
#endif
	if (bDoScreenshot) {
		// If holding Shift save resized, else save normally
		bool bHoldingShift = (INPUTFILTER->IsBeingPressed(
								DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)) ||
							  INPUTFILTER->IsBeingPressed(
								DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT)));
		bool bSaveCompressed = bHoldingShift;
		RageTimer timer;
		StepMania::SaveScreenshot("Screenshots/", bSaveCompressed, "", "");
		Locator::getLogger()->debug("Screenshot took {} seconds.", timer.GetDeltaTime());
		return true; // handled
	}

	if (input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_ENTER) &&
		(INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RALT),
									 &input.InputList) ||
		 INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LALT),
									 &input.InputList)) &&
		!(SCREENMAN->GetTopScreen()->GetScreenType() == gameplay)) {
		// alt-enter
		/* In OS X, this is a menu item and will be handled as such. This will
		 * happen first and then the lower priority GUI thread will happen
		 * second, causing the window to toggle twice. Another solution would be
		 * to put a timer in ArchHooks::SetToggleWindowed() and just not set the
		 * bool it if it's been less than, say, half a second. */
#if !defined(__APPLE__)
		GameLoop::setToggleWindowed();
#endif
		return true;
	}

	return false;
}

void StepMania::HandleInputEvents(float fDeltaTime) {
	INPUTFILTER->Update(fDeltaTime);

	/* Hack: If the topmost screen hasn't been updated yet, don't process input,
	 * since we must not send inputs to a screen that hasn't at least had one
	 * update yet. (The first Update should be the very first thing a screen
	 * gets.) We'll process it next time. Call Update above, so the inputs are
	 * read and timestamped. */
	if (SCREENMAN->GetTopScreen()->IsFirstUpdate())
		return;

	std::vector<InputEvent> ieArray;
	INPUTFILTER->GetInputEvents(ieArray);

	// If we don't have focus, discard input.
	if (!GameLoop::isGameFocused())
		return;

	for (unsigned i = 0; i < ieArray.size(); i++) {
		InputEventPlus input;
		input.DeviceI = ieArray[i].di;
		input.type = ieArray[i].type;
		swap(input.InputList, ieArray[i].m_ButtonState);

		INPUTMAPPER->DeviceToGame(input.DeviceI, input.GameI);

		input.mp = MultiPlayer_Invalid;

		{
			// Translate input to the appropriate MultiPlayer. Assume that all
			// joystick devices are mapped the same as the master player.
			if (input.DeviceI.IsJoystick()) {
				DeviceInput diTemp = input.DeviceI;
				diTemp.device = DEVICE_JOY1;
				GameInput gi;

				// LOG->Trace( "device %d, %d", diTemp.device, diTemp.button );
				if (INPUTMAPPER->DeviceToGame(diTemp, gi)) {

					input.mp = InputMapper::InputDeviceToMultiPlayer(
					  input.DeviceI.device);
					// LOG->Trace( "multiplayer %d", input.mp );
					ASSERT(input.mp >= 0 && input.mp < NUM_MultiPlayer);
				}
			}
		}

		if (input.GameI.IsValid()) {
			input.MenuI =
			  INPUTMAPPER->GameButtonToMenuButton(input.GameI.button);
			input.pn =
			  INPUTMAPPER->ControllerToPlayerNumber(input.GameI.controller);
		}

		INPUTQUEUE->RememberInput(input);

		// When a GameButton is pressed, stop repeating other keys on the same
		// controller.
		if (input.type == IET_FIRST_PRESS &&
			input.MenuI != GameButton_Invalid) {
			FOREACH_ENUM(GameButton, m)
			{
				if (input.MenuI != m)
					INPUTMAPPER->RepeatStopKey(m, input.pn);
			}
		}

		if (HandleGlobalInputs(input))
			continue; // skip

		SCREENMAN->Input(input);
	}

	if (GameLoop::GetAndClearToggleWindowed()) {
		PREFSMAN->m_bWindowed.Set(!PREFSMAN->m_bWindowed);
		StepMania::ApplyGraphicOptions();
	}
}

#include "Etterna/Singletons/LuaManager.h"

int
LuaFunc_SaveScreenshot(lua_State* L)
{
	// If pn is provided, save to that player's profile.
	// Otherwise, save to the machine.
	PlayerNumber pn = Enum::Check<PlayerNumber>(L, 1, true);
	bool compress = lua_toboolean(L, 2) != 0;
	bool sign =
	  lua_toboolean(L, 3) != 0; // Legacy, unused. This should be removed later.
	std::string prefix = luaL_optstring(L, 4, "");
	std::string suffix = luaL_optstring(L, 5, "");
	std::string dir;
	if (pn == PlayerNumber_Invalid) {
		dir = "Screenshots/";
	} else {
		dir = PROFILEMAN->GetProfileDir(static_cast<ProfileSlot>(pn)) +
			  "Screenshots/";
	}
	std::string filename =
	  StepMania::SaveScreenshot(dir, compress, prefix, suffix);
	if (pn != PlayerNumber_Invalid) {
	}
	std::string path = dir + filename;
	lua_pushboolean(L, !filename.empty());
	lua_pushstring(L, path.c_str());
	return 2;
}
void
LuaFunc_Register_SaveScreenshot(lua_State* L);
void
LuaFunc_Register_SaveScreenshot(lua_State* L)
{
	lua_register(L, "SaveScreenshot", LuaFunc_SaveScreenshot);
}
REGISTER_WITH_LUA_FUNCTION(LuaFunc_Register_SaveScreenshot);

static int
LuaFunc_update_centering(lua_State* L)
{
	update_centering();
	return 0;
}
LUAFUNC_REGISTER_COMMON(update_centering);
