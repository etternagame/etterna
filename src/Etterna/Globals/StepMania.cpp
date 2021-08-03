#include "global.h"
#include "StepMania.h"
#include "Etterna/Globals/rngthing.h"

// Core headers
#include "Core/Services/Locator.hpp"
#include "Core/Misc/PlogLogger.hpp"
#include "Core/Crash/CrashpadHandler.hpp"
#include "Core/Misc/AppInfo.hpp"
#include "Core/Platform/Platform.hpp"

// Rage global classes
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

#include "RageUtil/Graphics/RageDisplay_OGL.h"
#include "RageUtil/Graphics/RageDisplay_Null.h"

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
#include "Etterna/Singletons/ScoreManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Actor/Base/ModelManager.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Singletons/MessageManager.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Globals/GameLoop.h"

#include "discord_rpc.h"

#include <ctime>

#ifdef _WIN32
#include <windows.h>
int(WINAPIV* __vsnprintf)(char*, size_t, const char*, va_list) = _vsnprintf;

#include "RageUtil/Graphics/RageDisplay_D3D.h"
#include "archutils/Win32/VideoDriverInfo.h"
static Preference<int> g_iLastSeenMemory("LastSeenMemory", 0);
#endif

// Misc
bool noWindow;
struct VideoCardDefaults {
    std::string sDriverRegex;
	std::string sVideoRenderers;
	int iWidth;
	int iHeight;
	int iDisplayColor;
	int iTextureColor;
	int iMovieColor;
	int iTextureSize;
	bool bSmoothLines;
};

//// Function Declarations //////////////////////////
#pragma region Function Declarations
static void AdjustForChangedSystemCapabilities();
static bool CheckVideoDefaultSettings();
static std::string GetActualGraphicOptionsString();
static std::string GetVideoDriverName();
static void HandleException(const std::string& sError);
bool HandleGlobalInputs(const InputEventPlus& input);
static void StoreActualGraphicOptions();
static void SwitchToLastPlayedGame();
static void update_centering();
static void WriteLogHeader();

static void StartDisplay();
static RageDisplay* CreateDisplay();
#pragma endregion

//// Localized Strings Variables ////////////////////
#pragma region LocalizedString
static LocalizedString COLOR("Etterna", "color");
static LocalizedString TEXTURE("Etterna", "texture");
static LocalizedString WINDOWED("Etterna", "Windowed");
static LocalizedString FULLSCREEN("Etterna", "Fullscreen");
static LocalizedString ANNOUNCER_("Etterna", "Announcer");
static LocalizedString VSYNC("Etterna", "Vsync");
static LocalizedString NO_VSYNC("Etterna", "NoVsync");
static LocalizedString SMOOTH_LINES("Etterna", "SmoothLines");
static LocalizedString NO_SMOOTH_LINES("Etterna", "NoSmoothLines");
static LocalizedString ERROR_INITIALIZING_CARD( "Etterna", "There was an error while initializing your video card.");
static LocalizedString ERROR_DONT_FILE_BUG("Etterna","Please do not file this error as a bug!  Use the web page below to troubleshoot this problem.");
static LocalizedString ERROR_VIDEO_DRIVER("Etterna", "Video Driver: %s");
static LocalizedString ERROR_NO_VIDEO_RENDERERS("Etterna","No video renderers attempted.");
static LocalizedString ERROR_INITIALIZING("Etterna", "Initializing %s...");
static LocalizedString ERROR_UNKNOWN_VIDEO_RENDERER("Etterna","Unknown video renderer value: %s");
static LocalizedString SERVICE_SWITCH_PRESSED("Etterna","Service switch pressed");
static LocalizedString RELOADED_METRICS("ThemeManager", "Reloaded metrics");
static LocalizedString RELOADED_METRICS_AND_TEXTURES("ThemeManager","Reloaded metrics and textures");
static LocalizedString RELOADED_SCRIPTS("ThemeManager", "Reloaded scripts");
static LocalizedString RELOADED_OVERLAY_SCREENS("ThemeManager","Reloaded overlay screens");
static LocalizedString COULDNT_OPEN_LOADING_WINDOW("LoadingWindow","Couldn't open any loading windows.");
#pragma endregions

//// Preferences & Theme Metrics ////////////////////

#pragma region Preferences & Theme Metrics
static Preference<bool> g_bAllowMultipleInstances("AllowMultipleInstances",false);
static Preference<bool> TKGP("ResetVideoSettingsWithNewGPU", true);
ThemeMetric<std::string> INITIAL_SCREEN("Common", "InitialScreen");
ThemeMetric<std::string> SELECT_MUSIC_SCREEN("Common", "SelectMusicScreen");
#pragma endregion

//// Translation Unit Specific Functions ////////////
#pragma region Translation Unit Specific Functions
static void AdjustForChangedSystemCapabilities() {
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
	const bool LowMemory = (Memory < 100); // 64 and 96-meg systems

	/* Two memory-consuming features that we can disable are texture caching and
	 * preloaded banners. Texture caching can use a lot of memory; disable it
	 * for low-memory systems. */
	PREFSMAN->m_bDelayedTextureDelete.Set(HighMemory);

	PREFSMAN->SavePrefsToDisk();
#endif
}

bool CheckVideoDefaultSettings() {
    // Video card changed since last run
    std::string sVideoDriver = GetVideoDriverName();

    Locator::getLogger()->trace("Last seen video driver: {}", PREFSMAN->m_sLastSeenVideoDriver.Get().c_str());

    // allow players to opt out of the forced reset when a new video card is
    // detected - mina

    VideoCardDefaults defaults{"", "d3d,opengl", 800, 600, 32, 32, 32, 1024, false};
    Locator::getLogger()->trace("Card matches '{}'.",!defaults.sDriverRegex.empty() ? defaults.sDriverRegex.c_str() : "(unknown card)");


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

    if (PREFSMAN->m_verbose_log > 0)
        Locator::getLogger()->info("Video renderers: '{}'", PREFSMAN->m_sVideoRenderers.Get().c_str());
    return bSetDefaultVideoParams;
}

static std::string GetActualGraphicOptionsString() {
	auto params = DISPLAY->getVideoMode();
	std::string sFormat = "%s %s %dx%d " + COLOR.GetValue() + " %d " +
						  TEXTURE.GetValue() + " %dHz %s %s";
	std::string sLog = ssprintf(sFormat,
			   GetVideoDriverName().c_str(),
			   (params.isFullscreen ? FULLSCREEN : WINDOWED).GetValue().c_str(),
			   params.width, params.height,
//			   params.bpp,
			   static_cast<int>(PREFSMAN->m_iTextureColorDepth),
			   params.refreshRate,
			   (params.isVsyncEnabled ? VSYNC : NO_VSYNC).GetValue().c_str(),
			   (PREFSMAN->m_bSmoothLines ? SMOOTH_LINES : NO_SMOOTH_LINES).GetValue().c_str());
	return sLog;
}

static std::string GetVideoDriverName() {
#ifdef _WIN32
	return GetPrimaryVideoDriverName();
#else
	return "OpenGL";
#endif
}

static void HandleException(const std::string& sError) {
	StepMania::ShutdownGame(); // Shut down first, so we exit graphics mode before trying to open a dialog.
	Dialog::Error(sError); // Throw up a pretty error dialog.
	Dialog::Shutdown(); // Shut it back down.
}

bool HandleGlobalInputs(const InputEventPlus& input) {
    /* Returns true if the key has been handled and should be discarded, false if
       the key should be sent on to screens. */


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
				}
			}
			return true;
			return false; // Attract needs to know because it goes to TitleMenu
						  // on > 1 credit
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
		Locator::getLogger()->trace("Screenshot took {} seconds.", timer.GetDeltaTime());
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

static void StoreActualGraphicOptions() {
	/* Store the settings that RageDisplay was actually able to use so that
	 * we don't go through the process of auto-detecting a usable video mode
	 * every time. */
	auto params = DISPLAY->getVideoMode();
	PREFSMAN->m_bWindowed.Set(!params.isFullscreen);
	if (!params.isFullscreen) {
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

	if (PREFSMAN->m_iRefreshRate != REFRESH_DEFAULT)
		PREFSMAN->m_iRefreshRate.Set(params.refreshRate);

	PREFSMAN->m_bVsync.Set(params.isVsyncEnabled);

	Dialog::SetWindowed(!params.isFullscreen);
}

static void SwitchToLastPlayedGame() {

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

static void update_centering() {
	DISPLAY->ChangeCentering(PREFSMAN->m_iCenterImageTranslateX, PREFSMAN->m_iCenterImageTranslateY,
							 PREFSMAN->m_fCenterImageAddWidth, PREFSMAN->m_fCenterImageAddHeight);
}

static void WriteLogHeader() {
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

#pragma endregion


//// Display Related ////////////////////////////////
#pragma region Display Related
static void StartDisplay() {
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
RageDisplay* CreateDisplay() {
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

    if (noWindow) return new RageDisplay_Null; // Determine if display should be created

    // Set default error value
    std::string error = ERROR_INITIALIZING_CARD.GetValue() + "\n\n" + ERROR_DONT_FILE_BUG.GetValue() + "\n\n" +
                        ssprintf(ERROR_VIDEO_DRIVER.GetValue(), GetVideoDriverName().c_str()) + "\n\n";

	// Load VideoMode settings
    CheckVideoDefaultSettings();
    Core::Platform::Window::VideoMode videoMode;
    StepMania::GetPreferredVideoModeParams(videoMode);

	// Load list of backends to use
    std::vector<std::string> renderers;
    split(PREFSMAN->m_sVideoRenderers, ",", renderers, true);
    if(renderers.empty()) {
        Locator::getLogger()->warn("'VideoRenderers' list in Preferences.ini is empty. Resetting to default: {}",
                                   PREFSMAN->m_sVideoRenderers.GetDefault());
        PREFSMAN->m_sVideoRenderers.LoadDefault();
        split(PREFSMAN->m_sVideoRenderers, ",", renderers, true);
    }

	// Determine Backend
    RageDisplay* display = nullptr;
    for (const auto& backend : renderers) {
        if(CompareNoCase(backend, "opengl") == 0){
            display = new RageDisplay_Legacy; break;
        }
        else if(CompareNoCase(backend, "d3d") == 0) {
            #if defined(SUPPORT_D3D)
                display = new RageDisplay_D3D; break;
            #endif
        }
        else if(CompareNoCase(backend, "null") == 0){
            display = new RageDisplay_Null; break;
        }
    }

    // If display is still null, throw error
    if(display == nullptr) RageException::Throw("%s", error.c_str());

    // Run display init process
    display->Init(videoMode, PREFSMAN->m_bAllowUnacceleratedRenderer);

    return display;
}
#pragma endregion

//// StepMania Namespace ////////////////////////////
namespace StepMania {

    int sm_main(int argc, char* argv[]) {
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

#ifdef __unix__
        /* Mount the root filesystem, so we can read files in /proc, /etc, and so
         * on. This is /rootfs, not /root, to avoid confusion with root's home
         * directory. */
        FILEMAN->Mount("dir", "/", "/rootfs");
#endif
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
            StepMania::ShutdownGame();
            return 0;
        }

        WriteLogHeader();

        // Set up alternative filesystem trees.
        if (!PREFSMAN->m_sAdditionalFolders.Get().empty()) {
            vector<std::string> dirs;
            split(PREFSMAN->m_sAdditionalFolders, ",", dirs, true);
            for (unsigned i = 0; i < dirs.size(); i++)
                FILEMAN->Mount("dir", dirs[i], "/");
        }
        if (!PREFSMAN->m_sAdditionalSongFolders.Get().empty()) {
            vector<std::string> dirs;
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
        Locator::getLogger()->setLogLevel(static_cast<Core::ILogger::Severity>(PREFSMAN->m_verbose_log.Get()));

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
        PROFILEMAN = new ProfileManager;
        PROFILEMAN->Init(pLoadingWindow); // must load after SONGMAN
        SONGMAN->CalcTestStuff();		  // must be after profileman init

        NSMAN = new NetworkSyncManager(pLoadingWindow);
        STATSMAN = new StatsManager;

        FILTERMAN = new FilterManager;

        DLMAN = std::make_shared<DownloadManager>();

        /* If the user has tried to quit during the loading, do it before creating
         * the main window. This prevents going to full screen just to quit. */
        if (GameLoop::hasUserQuit()) {
            StepMania::ShutdownGame();
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

        StepMania::ShutdownGame();

        return 0;
    }

    void HandleInputEvents(float fDeltaTime) {
        INPUTFILTER->Update(fDeltaTime);

        /* Hack: If the topmost screen hasn't been updated yet, don't process input,
         * since we must not send inputs to a screen that hasn't at least had one
         * update yet. (The first Update should be the very first thing a screen
         * gets.) We'll process it next time. Call Update above, so the inputs are
         * read and timestamped. */
        if (SCREENMAN->GetTopScreen()->IsFirstUpdate())
            return;

        vector<InputEvent> ieArray;
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

    void GetPreferredVideoModeParams(VideoMode& paramsOut) {
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

        paramsOut.windowTitle = CommonMetrics::WINDOW_TITLE;
        paramsOut.windowIcon = THEME->GetPathG("Common", "window icon");
        paramsOut.width = iWidth;
        paramsOut.height = PREFSMAN->m_iDisplayHeight;
        paramsOut.refreshRate = PREFSMAN->m_iRefreshRate;
        paramsOut.isFullscreen = PREFSMAN->m_bFullscreenIsBorderlessWindow;
        paramsOut.isBorderless = PREFSMAN->m_bFullscreenIsBorderlessWindow;
        paramsOut.isVsyncEnabled = PREFSMAN->m_bVsync;
    }

    bool GetHighResolutionTextures() {
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

    void ApplyGraphicOptions(){
        bool bNeedReload = false;

//        VideoModeParams params;
//        GetPreferredVideoModeParams(params);
        std::string sError = "";//DISPLAY->SetVideoMode(params, bNeedReload);
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

    }

    void ResetGame() {
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

    void ResetPreferences() {
        PREFSMAN->ResetToFactoryDefaults();
        SOUNDMAN->SetMixVolume();
        CheckVideoDefaultSettings();
        ApplyGraphicOptions();
    }

    std::string GetInitialScreen() {
        std::string screen_name = INITIAL_SCREEN.GetValue();
        if (!SCREENMAN->IsScreenNameValid(screen_name)) {
            screen_name = "ScreenInitialScreenIsInvalid";
        }
        return screen_name;
    }

    std::string GetSelectMusicScreen() {
        return SELECT_MUSIC_SCREEN.GetValue();
    }

    // This function is meant to only be called during start up.
    void InitializeCurrentGame(const Game* g) {
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

    std::string SaveScreenshot(const std::string& Dir, bool SaveCompressed, const std::string& NamePrefix,
                               const std::string& NameSuffix) {
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
    /* Shutdown all global singletons. Note that this may be called partway through
     * initialization, due to an object failing to initialize, in which case some of
     * these may still be NULL. */
    void ShutdownGame() {
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

}

//// Lua ////////////////////////////////////////////
#pragma region Lua

#include "Etterna/Singletons/LuaManager.h"
int LuaFunc_SaveScreenshot(lua_State* L)
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
void LuaFunc_Register_SaveScreenshot(lua_State* L);
void LuaFunc_Register_SaveScreenshot(lua_State* L)
{
	lua_register(L, "SaveScreenshot", LuaFunc_SaveScreenshot);
}
static int LuaFunc_update_centering(lua_State* L)
{
	update_centering();
	return 0;
}
REGISTER_WITH_LUA_FUNCTION(LuaFunc_Register_SaveScreenshot);
LUAFUNC_REGISTER_COMMON(update_centering);

#pragma endregion
