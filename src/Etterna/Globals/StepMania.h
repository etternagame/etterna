#ifndef STEP_MANIA_H
#define STEP_MANIA_H

#include <string>
#include "Core/Platform/Window/VideoMode.hpp"
using namespace Core::Platform::Window;


struct Game;
class RageTimer;

/** @brief Utility functions for controlling the whole game. */
namespace StepMania {
    int sm_main(int argc, char* argv[]);
    void ApplyGraphicOptions();
    void ResetPreferences();
    void ResetGame();
    std::string GetInitialScreen();
    std::string GetSelectMusicScreen();
    void InitializeCurrentGame(const Game* g);
    void GetPreferredVideoModeParams(VideoMode& paramsOut);
    bool GetHighResolutionTextures();
    void HandleInputEvents(float fDeltaTime);

    // If successful, return filename of screenshot in sDir, else return ""
    std::string SaveScreenshot(const std::string& Dir, bool SaveCompressed,
                   const std::string& NamePrefix, const std::string& NameSuffix);

    void ShutdownGame();
}

#endif
