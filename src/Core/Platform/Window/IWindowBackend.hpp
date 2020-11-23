#ifndef CORE_PLATFORM_WINDOW_HPP
#define CORE_PLATFORM_WINDOW_HPP

#include "Core/Platform/Model/Dimensions.hpp"

#include <functional>
#include <string>
#include <utility>

namespace Core::Platform::Window {

/**
 * An abstract class to be implemented as an interface with the
 * windowing system backend.
 *
 */
class IWindowBackend {

public:
    // De/Constructor
    IWindowBackend(std::string_view title, const Dimensions &size);
    ~IWindowBackend() = default;

    // Virtual Functions
    virtual void create() = 0;
    virtual void update() = 0;
    virtual bool exited() = 0;

    // Getters and Setters
    std::string getTitle();
    Dimensions getWindowDimensions();

    // Window Event Callback Setters
    void setWindowCloseCallback(std::function<void()> function);
    void setWindowFocusGainCallback(std::function<void()> function);
    void setWindowFocusLostCallback(std::function<void()> function);
    void setWindowResizeCallback(std::function<void(Dimensions)> function);

protected:
    // Window properties
    std::string title; /** @brief Title of the window */
    Dimensions size; /** @brief Window dimensions */

    // Event Function Callbacks
    std::function<void()> onClose; /** @brief Triggered just before when window closes */
    std::function<void()> onFocusLost; /** @brief Triggered when the windows loses focus*/
    std::function<void()> onFocusGain; /** @brief Triggered when the window gains focus */
    std::function<void(Dimensions)> onResize; /** @brief Triggered when the window changes size */
};
}

#endif //CORE_PLATFORM_WINDOW_HPP
