#include "GLFWWindowBackend.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Core::Platform::Window {

    GLFWWindowBackend::GLFWWindowBackend(std::string_view title, const Dimensions &size)
    : IWindowBackend(title, size) {
        glfwInit();
    }

    /**
     * Create the window and initialized all callbacks.
     * TODO: Callback for minimize, restore, maximize
     */
    void GLFWWindowBackend::create() {
        // Set Window Hints
        // This set of window hints is only required on macOS as that platform no
        // longer prefers OpenGL and would generate a OpenGL 2.0 graphics context.
        // Commented out since the existing GL only works with a 2.0 context.
//#ifdef __APPLE__
//        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
//        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
//        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
//#endif

        // Create Window
        this->windowHandle = glfwCreateWindow(static_cast<int>(size.width),static_cast<int>(size.height),
                                              title.data(), nullptr, nullptr);

        // Tell the GLFWwindow to store a pointer to this GLFWWindowBackend object
        glfwSetWindowUserPointer(this->windowHandle, this);

        // Initialize Callbacks
        // Functions are checked to be not null first. If the function is null, it was never set to an
        // invokable function, and should not be called.
        // Window Resize Callback
        glfwSetWindowSizeCallback(this->windowHandle, [](GLFWwindow *window, int width, int height){
            Dimensions newSize{static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
            auto backend = static_cast<GLFWWindowBackend*>(glfwGetWindowUserPointer(window));
            backend->size = newSize;
            backend->onResized(newSize);
        });

        // Window Focus Callback
        glfwSetWindowFocusCallback(this->windowHandle, [](GLFWwindow *window, int focus){
            auto backend = static_cast<GLFWWindowBackend*>(glfwGetWindowUserPointer(window));
            if(focus == GLFW_TRUE){
                backend->onFocusGain();
            } else if(focus == GLFW_FALSE){
                backend->onFocusLost();
            }
        });

        // Window Close Callback
        glfwSetWindowCloseCallback(this->windowHandle, [](GLFWwindow *window){
            auto backend = static_cast<GLFWWindowBackend*>(glfwGetWindowUserPointer(window));
            backend->onCloseRequested();
        });

        // Windows min/max Callback
        glfwSetWindowMaximizeCallback(this->windowHandle, [](GLFWwindow* window, int maximized){
            auto backend = static_cast<GLFWWindowBackend*>(glfwGetWindowUserPointer(window));
            if(maximized == GLFW_TRUE){
                backend->onWindowMaximized();
            } else if(maximized == GLFW_FALSE){
                backend->onWindowMinimized();
            }
        });

        // Set as render context
        glfwMakeContextCurrent(this->windowHandle);
    }

    void GLFWWindowBackend::update() const {
        glfwPollEvents();
    }

    bool GLFWWindowBackend::exited() const {
        return glfwWindowShouldClose(this->windowHandle);
    }

    void GLFWWindowBackend::swapBuffers() const {
        glfwSwapBuffers(this->windowHandle);
    }

    void *GLFWWindowBackend::getNativeWindow() const {
        return static_cast<void*>(this->windowHandle);
    }

    void GLFWWindowBackend::setTitle(const std::string &title) {
        glfwSetWindowTitle(this->windowHandle, title.data());
    }

}