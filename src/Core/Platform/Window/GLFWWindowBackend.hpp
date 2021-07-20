#ifndef CORE_PLATFORM_WINDOW_GLFWWINDOWBACKEND_HPP
#define CORE_PLATFORM_WINDOW_GLFWWINDOWBACKEND_HPP

#include "Core/Platform/Window/IWindowBackend.hpp"

class GLFWwindow;

namespace Core::Platform::Window {

class GLFWWindowBackend : public IWindowBackend {
public:
    GLFWWindowBackend(std::string_view title, const Dimensions &size);

    // Overridden Functions
    void create() override;
    void update() const override;
    bool exited() const override;
    void swapBuffers() const override;
    void *getNativeWindow() const override;
    void setTitle(const std::string &title) override;

private:
    GLFWwindow *windowHandle{nullptr}; /** @brief A reference to the window backend*/
};

}



#endif //CORE_PLATFORM_WINDOW_GLFWWINDOWBACKEND_HPP
