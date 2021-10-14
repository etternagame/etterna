#ifndef CORE_PLATFORM_WINDOW_GLFWWINDOWBACKEND_HPP
#define CORE_PLATFORM_WINDOW_GLFWWINDOWBACKEND_HPP

#include <RageUtil/Misc/RageInputDevice.h>
#include "Core/Platform/Window/IWindowBackend.hpp"

struct GLFWwindow;

namespace Core::Platform::Window {

class GLFWWindowBackend : public IWindowBackend {
public:
    explicit GLFWWindowBackend(const VideoMode& params);
    ~GLFWWindowBackend() override;

    // Context Related
    void create() override;
    void update() const override;
    bool exited() const override;
    void swapBuffers() const override;
    void setContext() const override;
    void clearContext() const override;

    // Data Related
    void *getNativeWindow() const override;
    void setTitle(const std::string &title) override;
    Dimensions getFrameBufferSize() const override;
    int getRefreshRate() const override;

    static void setWindowHint(int hint, int value);

private:
    GLFWwindow *windowHandle{nullptr}; /** @brief A reference to the window backend*/
    static DeviceButton convertKeyToLegacy(int keycode, int mods);

    // TODO(james): Fix me? IWindowBackend is not the place for this game-specific function, but is currently
    //              the only reasonable place to implement the feature. Only used in GLFWWindowBackend.
    //              To be moved to an appropriate location for a more complete input rework.
    bool nextInputFirst{false};
};

}



#endif //CORE_PLATFORM_WINDOW_GLFWWINDOWBACKEND_HPP
