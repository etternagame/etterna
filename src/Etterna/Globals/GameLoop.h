#ifndef GAME_LOOP_H
#define GAME_LOOP_H
/** @brief Main rendering and update loop. */
namespace GameLoop {
void
RunGameLoop();
void
SetUpdateRate(float fUpdateRate);
void
ChangeTheme(const RString& sNewTheme);
void
ChangeGame(const RString& new_game, const RString& new_theme = "");
void
StartConcurrentRendering();
void
FinishConcurrentRendering();
};

#endif
