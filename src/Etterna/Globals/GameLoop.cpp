#include "global.h"
#include "GameLoop.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/InputFilter.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "RageUtil/Misc/RageInput.h"
#include "Etterna/Singletons/ScreenManager.h"

#include <chrono>
#include <mutex>

#include "Core/Platform/Platform.hpp"

// Static Variables
//// On the next update, change themes, and load sNewScreen.
static std::mutex archMutex;
static bool toggleWindowed, focusChanged;
static bool hasFocus = true;

static bool userQuit = false;
static std::string g_NewTheme;
static std::string g_NewGame;
static auto g_AccurateGameplayTimer = std::chrono::steady_clock::now();
static float g_fUpdateRate = 1;
static Preference<bool> g_bNeverBoostAppPriority("NeverBoostAppPriority",false);
/* experimental: force a specific update rate. This prevents big  animation jumps on frame skips. 0 to disable. */
static Preference<float> g_fConstantUpdateDeltaSeconds("ConstantUpdateDeltaSeconds",0);

// Static Functions
static void CheckGameLoopTimerSkips(float fDeltaTime) {
	if (!PREFSMAN->m_bLogSkips)
		return;

	static int iLastFPS = 0;
	int iThisFPS = DISPLAY->GetFPS();

	/* If vsync is on, and we have a solid framerate (vsync == refresh and we've
	 * sustained this for at least one second), we expect the amount of time for
	 * the last frame to be 1/FPS. */
	if (iThisFPS != (*DISPLAY->GetActualVideoModeParams()).rate ||
		iThisFPS != iLastFPS) {
		iLastFPS = iThisFPS;
		return;
	}

	const float fExpectedTime = 1.0f / iThisFPS;
	const float fDifference = fDeltaTime - fExpectedTime;
	if (fabsf(fDifference) > 0.002f && fabsf(fDifference) < 0.100f)
		Locator::getLogger()->trace("GameLoop timer skip: {} FPS, expected {:.3f}, got {:.3f} ({:.3f} difference)",
				   iThisFPS, fExpectedTime, fDeltaTime, fDifference);
}

static void CheckFocus() {
	if (!GameLoop::didFocusChange())
		return;
	// If we lose focus, we may lose input events, especially key releases.
	INPUTFILTER->Reset();

	// Maintain the Application priority at Above-Normal
	// This helps to mitigate game stutter caused by CPU scheduling between frames
	if (hasFocus)
		Core::Platform::boostPriority();
	else
		Core::Platform::unboostPriority();
}

// Anonymous Namespace
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Singletons/GameManager.h"
#include "StepMania.h" // XXX
namespace {
    void DoChangeTheme() {
        SAFE_DELETE(SCREENMAN);
        TEXTUREMAN->DoDelayedDelete();

        // In case the previous theme overloaded class bindings, reinitialize them.
        LUA->RegisterTypes();

        // We always need to force the theme to reload because we cleared the lua
        // state by calling RegisterTypes so the scripts in Scripts/ need to run.
        THEME->SwitchThemeAndLanguage(
          g_NewTheme, THEME->GetCurLanguage(), PREFSMAN->m_bPseudoLocalize, true);
        PREFSMAN->m_sTheme.Set(g_NewTheme);

        // Apply the new window title, icon and aspect ratio.
        StepMania::ApplyGraphicOptions();

        SCREENMAN = new ScreenManager();

        StepMania::ResetGame();
        SCREENMAN->ThemeChanged();
        // The previous system for changing the theme fetched the "NextScreen"
        // metric from the current theme, then changed the theme, then tried to
        // set the new screen to the name that had been fetched.
        // If the new screen didn't exist in the new theme, there would be a
        // crash.
        // So now the correct thing to do is for a theme to specify its entry
        // point after a theme change, ensuring that we are going to a valid
        // screen and not crashing. -Kyz
        std::string new_screen = THEME->GetMetric("Common", "InitialScreen");
        if (THEME->HasMetric("Common", "AfterThemeChangeScreen")) {
            std::string after_screen =
              THEME->GetMetric("Common", "AfterThemeChangeScreen");
            if (SCREENMAN->IsScreenNameValid(after_screen)) {
                new_screen = after_screen;
            }
        }
        if (!SCREENMAN->IsScreenNameValid(new_screen)) {
            new_screen = "ScreenInitialScreenIsInvalid";
        }
        SCREENMAN->SetNewScreen(new_screen);

        g_NewTheme = std::string();
    }
    void DoChangeGame() {
        const Game* g = GAMEMAN->StringToGame(g_NewGame);
        ASSERT(g != nullptr);
        GAMESTATE->SetCurGame(g);

		bool theme_changing = false;
		// The prefs allow specifying a different default theme to use for each
		// game type.  So if a theme name isn't passed in, fetch from the prefs.
		if (g_NewTheme.empty()) {
			g_NewTheme = PREFSMAN->m_sTheme.Get();
		}
		if (g_NewTheme != THEME->GetCurThemeName() &&
			THEME->IsThemeSelectable(g_NewTheme)) {
			theme_changing = true;
		}

		if (theme_changing) {
			SAFE_DELETE(SCREENMAN);
			TEXTUREMAN->DoDelayedDelete();
			LUA->RegisterTypes();
			THEME->SwitchThemeAndLanguage(
			  g_NewTheme, THEME->GetCurLanguage(), PREFSMAN->m_bPseudoLocalize);
			PREFSMAN->m_sTheme.Set(g_NewTheme);
			StepMania::ApplyGraphicOptions();
			SCREENMAN = new ScreenManager();
		}

        // reset gamestate to deal with new Game
        StepMania::ResetGame();

        // point us to the new Screen to end up on after Game change
        // either the initialscreen or something else
        std::string new_screen = THEME->GetMetric("Common", "InitialScreen");
        std::string after_screen;
		if (theme_changing) {
			SCREENMAN->ThemeChanged();
			if (THEME->HasMetric("Common", "AfterGameAndThemeChangeScreen")) {
				after_screen =
				  THEME->GetMetric("Common", "AfterGameAndThemeChangeScreen");
			}
		} else {
			if (THEME->HasMetric("Common", "AfterGameChangeScreen")) {
				after_screen =
				  THEME->GetMetric("Common", "AfterGameChangeScreen");
			}
		}
        if (SCREENMAN->IsScreenNameValid(after_screen)) {
            new_screen = after_screen;
        }
        SCREENMAN->SetNewScreen(new_screen);

        // Set the input scheme for the new game, and load keymaps.
        if (INPUTMAPPER != nullptr) {
            INPUTMAPPER->SetInputScheme(&g->m_InputScheme);
            INPUTMAPPER->ReadMappingsFromDisk();
        }
        // aj's comment transplanted from ScreenOptionsMasterPrefs.cpp:GameSel. -Kyz
        /* Reload metrics to force a refresh of CommonMetrics::DIFFICULTIES_TO_SHOW,
         * mainly if we're not switching themes. I'm not sure if this was the
         * case going from theme to theme, but if it was, it should be fixed
         * now. There's probably be a better way to do it, but I'm not sure
         * what it'd be. -aj */
        THEME->UpdateLuaGlobals();
		SCREENMAN->ReloadOverlayScreens();
        THEME->ReloadMetrics();
        g_NewGame = std::string();
        g_NewTheme = std::string();
    }
} // namespace

namespace GameLoop {
    bool hasUserQuit(){
        return userQuit;
    }

    void setUserQuit(){
        userQuit = true;
    }

    void setGameFocused(bool isFocused){
        if(isFocused == hasFocus)
            return;
        hasFocus = isFocused;

        Locator::getLogger()->trace("App {} focus", isFocused ? "has" : "doesn't have");
        std::lock_guard<std::mutex> lock(archMutex);
        focusChanged = true;
    }

    bool isGameFocused(){
        return hasFocus;
    }

    bool didFocusChange(){
        std::lock_guard<std::mutex> lock(archMutex);
        bool temp = focusChanged;
        focusChanged = false;
        return temp;
    }

    void setToggleWindowed(){
        std::lock_guard<std::mutex> lock(archMutex);
        toggleWindowed = true;
    }

    bool GetAndClearToggleWindowed(){
        std::lock_guard<std::mutex> lock(archMutex);
        bool temp = toggleWindowed;
        toggleWindowed = false;
        return temp;
    }

    void SetUpdateRate(float fUpdateRate) {
        g_fUpdateRate = fUpdateRate;
    }

	float GetUpdateRate() {
		return g_fUpdateRate;
    }

    void ChangeTheme(const std::string& sNewTheme) {
        g_NewTheme = sNewTheme;
    }

    void ChangeGame(const std::string& new_game, const std::string& new_theme) {
        g_NewGame = new_game;
        g_NewTheme = new_theme;
    }

    void RunGameLoop() {
		Core::Platform::boostPriority();
    	
        while (!GameLoop::hasUserQuit()) {
            if (!g_NewGame.empty()) {
                DoChangeGame();
            }
            if (!g_NewTheme.empty()) {
                DoChangeTheme();
            }

            // Update
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> frameStart = now - g_AccurateGameplayTimer;
            float fDeltaTime = frameStart.count();
            g_AccurateGameplayTimer = now;

            if (g_fConstantUpdateDeltaSeconds > 0)
                fDeltaTime = g_fConstantUpdateDeltaSeconds;

            CheckGameLoopTimerSkips(fDeltaTime);

            fDeltaTime *= g_fUpdateRate;

            CheckFocus();

            // Update SOUNDMAN early (before any RageSound::GetPosition calls), to
            // flush position data.
            SOUNDMAN->Update();

            /* Update song beat information -before- calling update on all the
             * classes that depend on it. If you don't do this first, the classes
             * are all acting on old information and will lag. (but no longer
             * fatally, due to timestamping -glenn) */
            SOUND->Update(fDeltaTime);
            TEXTUREMAN->Update(fDeltaTime);
            GAMESTATE->Update(fDeltaTime);
            SCREENMAN->Update(fDeltaTime);
            NSMAN->Update(fDeltaTime);
            DLMAN->Update(fDeltaTime);

            /* Important: Process input AFTER updating game logic, or input will be
             * acting on song beat from last frame */
            StepMania::HandleInputEvents(fDeltaTime);

            static float deviceCheckWait = 0.f;
            deviceCheckWait += fDeltaTime;

            if (deviceCheckWait >= 1.0f) {
                deviceCheckWait = 0.f;

                if (INPUTMAN->DevicesChanged()) {
                    INPUTFILTER->Reset(); // fix "buttons stuck" if button held
                                          // while unplugged
                    INPUTMAN->LoadDrivers();
                    std::string sMessage;
                    if (INPUTMAPPER->CheckForChangedInputDevicesAndRemap(sMessage))
                        SCREENMAN->SystemMessage(sMessage);
                }
            }

            // Render
            SCREENMAN->Draw();

			// Don't burn CPU while unfocused
			if (!GameLoop::isGameFocused() &&
				PREFSMAN->m_UnfocusedSleepMillisecs > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(
				  PREFSMAN->m_UnfocusedSleepMillisecs));
        }

    	Core::Platform::unboostPriority();
    }

}





