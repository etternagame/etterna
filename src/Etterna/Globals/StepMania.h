#ifndef STEP_MANIA_H
#define STEP_MANIA_H

struct Game;
class RageTimer;
class VideoModeParams;

int
sm_main(int argc, char* argv[]);

/** @brief Utility functions for controlling the whole game. */
namespace StepMania {
void
ApplyGraphicOptions();
void
ResetPreferences();
void
ResetGame();
std::string
GetInitialScreen();
std::string
GetSelectMusicScreen();
void
InitializeCurrentGame(const Game* g);

// If successful, return filename of screenshot in sDir, else return ""
std::string
SaveScreenshot(const std::string& Dir,
			   bool SaveCompressed,
			   bool MakeSignature,
			   const std::string& NamePrefix,
			   const std::string& NameSuffix);

void
InsertCoin(int iNum = 1, bool bCountInBookkeeping = true);
void
InsertCredit();
void
ClearCredits();

void
GetPreferredVideoModeParams(VideoModeParams& paramsOut);
bool
GetHighResolutionTextures();
}

#endif
