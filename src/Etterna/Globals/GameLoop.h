#ifndef GAME_LOOP_H
#define GAME_LOOP_H
/** @brief Main rendering and update loop. */
namespace GameLoop {
    bool hasUserQuit(); /** @return Return true if the user has attempted to quit the game. */
    void setUserQuit(); /** @brief Called when the user attempted to quit the game. */
    void setGameFocused(bool bAppHasFocus); /* Platform-specific code calls this to indicate focus changes.*/
    bool isGameFocused(); /* @return True if the application has input focus. */
    bool didFocusChange();  /* @return True if the application's focus has changed since last called */
    void setToggleWindowed();
    bool GetAndClearToggleWindowed();

    void SetUpdateRate(float fUpdateRate);
    void ChangeTheme(const std::string& sNewTheme);
    void ChangeGame(const std::string& new_game, const std::string& new_theme = "");
    void RunGameLoop();
};

#endif
