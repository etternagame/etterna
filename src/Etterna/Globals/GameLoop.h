#ifndef GAME_LOOP_H
#define GAME_LOOP_H
/** @brief Main rendering and update loop. */
namespace GameLoop {
    void SetUpdateRate(float fUpdateRate);
    void ChangeTheme(const std::string& sNewTheme);
    void ChangeGame(const std::string& new_game, const std::string& new_theme = "");
    void RunGameLoop();

    bool hasUserQuit(); /** @return Return true if the user has attempted to quit the game. */
    void setUserQuit(); /** @brief Called when the user attempted to quit the game. */
};

#endif
