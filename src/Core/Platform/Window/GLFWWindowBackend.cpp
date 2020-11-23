#include "GLFWWindowBackend.hpp"

#include <GLFW/glfw3.h>

namespace Core::Platform::Window {

    GLFWWindowBackend::GLFWWindowBackend(std::string_view title, const Dimensions &size) : IWindowBackend(title, size) {
        glfwInit();
    }

    GLFWWindowBackend::~GLFWWindowBackend() {
        glfwDestroyWindow(this->windowHandle);
        glfwTerminate();
    }

    /**
     * Create the window and initialized all callbacks.
     * TODO: Callback for minimize, restore, maximize
     */
    void GLFWWindowBackend::create() {
        // Create Window
        this->windowHandle = glfwCreateWindow(
                static_cast<int>(size.width),static_cast<int>(size.height),
                title.data(),nullptr, nullptr);

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
            if(backend->onResize != nullptr)
                std::invoke(backend->onResize, newSize);
        });

        // Window Close Callback
        glfwSetWindowCloseCallback(this->windowHandle, [](GLFWwindow *window){
            auto backend = static_cast<GLFWWindowBackend*>(glfwGetWindowUserPointer(window));
            if(backend->onClose != nullptr) std::invoke(backend->onClose);
        });

        // Window Focus Callback
        glfwSetWindowFocusCallback(this->windowHandle, [](GLFWwindow *window, int focus){
            auto backend = static_cast<GLFWWindowBackend*>(glfwGetWindowUserPointer(window));
            if(focus == GLFW_TRUE && backend->onFocusGain != nullptr){
                std::invoke(backend->onFocusGain);
            } else if(focus == GLFW_FALSE && backend->onFocusLost != nullptr){
                std::invoke(backend->onFocusLost);
            }
        });

        // Set as render context
        glfwMakeContextCurrent(this->windowHandle);
    }

    /**
     * Update function to be called every frame
     */
    void GLFWWindowBackend::update() {
        glfwSwapBuffers(this->windowHandle);
        glfwPollEvents();
    }

    /** @return True if the close flag is set for the window, otherwise false. */
    bool GLFWWindowBackend::exited() {
        return static_cast<bool>(glfwWindowShouldClose(this->windowHandle));
    }

}