#ifndef GAME_LOOP_H
#define GAME_LOOP_H
/** @brief Main rendering and update loop. */
namespace GameLoop {
    void RunGameLoop();
    void SetUpdateRate(float fUpdateRate);
    void ChangeTheme(const std::string& sNewTheme);
    void ChangeGame(const std::string& new_game, const std::string& new_theme = "");
};

#endif
