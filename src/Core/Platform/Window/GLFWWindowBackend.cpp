#include "GLFWWindowBackend.hpp"

#include "Etterna/Singletons/InputFilter.h"
#include "Core/Services/Locator.hpp"

#include <chrono>
#include <glad/glad.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <archutils/Win32/GraphicsWindow.h>

#endif

#include <GLFW/glfw3.h>

namespace Core::Platform::Window {

    GLFWWindowBackend::GLFWWindowBackend(std::string_view title, const Dimensions &size)
    : IWindowBackend(title, size) {
        glfwInit();
    }

    GLFWWindowBackend::~GLFWWindowBackend(){
        glfwDestroyWindow(this->windowHandle);
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

        // Reset window hints after window creating in-case other hints have been made.
        glfwDefaultWindowHints();

        // Tell the GLFWwindow to store a pointer to this GLFWWindowBackend object
        glfwSetWindowUserPointer(this->windowHandle, this);

#ifdef _WIN32
		GraphicsWindow::SetHwnd(glfwGetWin32Window(this->windowHandle));
#endif

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

        // Window key callback
        glfwSetKeyCallback(this->windowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods){
            auto time = std::chrono::steady_clock::now();
            if(action == GLFW_REPEAT) return; // We do our own repeat
            auto legacy_key = GLFWWindowBackend::convertKeyToLegacy(key, mods);
            DeviceInput di(DEVICE_KEYBOARD, legacy_key, action == GLFW_PRESS ? 1 : 0, time);
            if(INPUTFILTER)
                INPUTFILTER->ButtonPressed(di);
        });

        // Window mouse callback
        glfwSetMouseButtonCallback(this->windowHandle, [](GLFWwindow* window, int button, int action, int mods){
            auto time = std::chrono::steady_clock::now();
            if(action == GLFW_REPEAT) return; // We do our own repeat
            auto legacy_key = GLFWWindowBackend::convertKeyToLegacy(button, 0);
            DeviceInput di(DEVICE_MOUSE, legacy_key, action == GLFW_PRESS ? 1 : 0, time);
            if(INPUTFILTER)
                INPUTFILTER->ButtonPressed(di);
        });

        // Window specific mouse position callback
        glfwSetCursorPosCallback(this->windowHandle, [](GLFWwindow* window, double xpos, double ypos){
            INPUTFILTER->UpdateCursorLocation(static_cast<float>(xpos), static_cast<float>(ypos));
        });

        glfwMakeContextCurrent(this->windowHandle); // Set as render context
		glfwSwapInterval(0); // Don't wait for vsync
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

    Dimensions GLFWWindowBackend::getFrameBufferSize() const {
        int width, height;
        glfwGetFramebufferSize(this->windowHandle, &width, &height);
        return {static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
    }

    DeviceButton GLFWWindowBackend::convertKeyToLegacy(int keycode, int mods){
        // GLFW keycodes are all uppercase ascii. If we're in that ascii, determine if uppercase
        // or lowercase, and convert if necessary.
        if(65 <= keycode && keycode <= 90) {
            char asChar = static_cast<char>(keycode);
            if (mods & GLFW_MOD_SHIFT)
                return DeviceButton(asChar);
            else
                return DeviceButton(tolower(asChar));
        }

        // Check other keys if not ascii.
        switch (keycode) {
            case GLFW_KEY_BACKSPACE:        return KEY_BACK;
            case GLFW_KEY_TAB:              return KEY_TAB;
            case GLFW_KEY_PAUSE:            return KEY_PAUSE;
            case GLFW_KEY_ESCAPE:           return KEY_ESC;
            case GLFW_KEY_KP_0:             return KEY_KP_C0;
            case GLFW_KEY_KP_1:             return KEY_KP_C1;
            case GLFW_KEY_KP_2:             return KEY_KP_C2;
            case GLFW_KEY_KP_3:             return KEY_KP_C3;
            case GLFW_KEY_KP_4:             return KEY_KP_C4;
            case GLFW_KEY_KP_5:             return KEY_KP_C5;
            case GLFW_KEY_KP_6:             return KEY_KP_C6;
            case GLFW_KEY_KP_7:             return KEY_KP_C7;
            case GLFW_KEY_KP_8:             return KEY_KP_C8;
            case GLFW_KEY_KP_9:             return KEY_KP_C9;
            case GLFW_KEY_PERIOD:           return KEY_KP_PERIOD;
            case GLFW_KEY_SLASH:            return KEY_KP_SLASH;
            case GLFW_KEY_KP_MULTIPLY:      return KEY_KP_ASTERISK;
            case GLFW_KEY_KP_SUBTRACT:      return KEY_KP_HYPHEN;
            case GLFW_KEY_KP_ADD:           return KEY_KP_PLUS;
            case GLFW_KEY_KP_EQUAL:         return KEY_KP_EQUAL;
            case GLFW_KEY_KP_ENTER:         return KEY_KP_ENTER;
            case GLFW_KEY_KP_DECIMAL:       return KEY_KP_PERIOD;
            case GLFW_KEY_UP:               return KEY_UP;
            case GLFW_KEY_DOWN:             return KEY_DOWN;
            case GLFW_KEY_RIGHT:            return KEY_RIGHT;
            case GLFW_KEY_LEFT:             return KEY_LEFT;
            case GLFW_KEY_INSERT:           return KEY_INSERT;
            case GLFW_KEY_HOME:             return KEY_HOME;
            case GLFW_KEY_DELETE:           return KEY_DEL;
            case GLFW_KEY_END:              return KEY_END;
            case GLFW_KEY_PAGE_UP:          return KEY_PGUP;
            case GLFW_KEY_PAGE_DOWN:        return KEY_PGDN;
            case GLFW_KEY_F1:               return KEY_F1;
            case GLFW_KEY_F2:               return KEY_F2;
            case GLFW_KEY_F3:               return KEY_F3;
            case GLFW_KEY_F4:               return KEY_F4;
            case GLFW_KEY_F5:               return KEY_F5;
            case GLFW_KEY_F6:               return KEY_F6;
            case GLFW_KEY_F7:               return KEY_F7;
            case GLFW_KEY_F8:               return KEY_F8;
            case GLFW_KEY_F9:               return KEY_F9;
            case GLFW_KEY_F10:              return KEY_F10;
            case GLFW_KEY_F11:              return KEY_F11;
            case GLFW_KEY_F12:              return KEY_F12;
            case GLFW_KEY_F13:              return KEY_F13;
            case GLFW_KEY_F14:              return KEY_F14;
            case GLFW_KEY_F15:              return KEY_F15;

            case GLFW_KEY_NUM_LOCK:         return KEY_NUMLOCK;
            case GLFW_KEY_CAPS_LOCK:        return KEY_CAPSLOCK;
            case GLFW_KEY_SCROLL_LOCK:      return KEY_SCRLLOCK;
            case GLFW_KEY_ENTER:            return KEY_ENTER;
            case GLFW_KEY_PRINT_SCREEN:     return KEY_PRTSC;
            case GLFW_KEY_RIGHT_SHIFT:      return KEY_RSHIFT;
            case GLFW_KEY_LEFT_SHIFT:       return KEY_LSHIFT;
            case GLFW_KEY_RIGHT_CONTROL:    return KEY_RCTRL;
            case GLFW_KEY_LEFT_CONTROL:     return KEY_LCTRL;
            case GLFW_KEY_RIGHT_ALT:        return KEY_RALT;
            case GLFW_KEY_LEFT_ALT:         return KEY_LALT;
            case GLFW_KEY_RIGHT_SUPER:      return KEY_RSUPER;
            case GLFW_KEY_LEFT_SUPER:       return KEY_LSUPER;
            case GLFW_KEY_MENU:             return KEY_MENU;

            // mouse
            case GLFW_MOUSE_BUTTON_LEFT:    return MOUSE_LEFT;
            case GLFW_MOUSE_BUTTON_MIDDLE:  return MOUSE_MIDDLE;
            case GLFW_MOUSE_BUTTON_RIGHT:   return MOUSE_RIGHT;
            default:                        return DeviceButton(keycode);
        }
    }

    /**
     * Set a glfw window hint. Only needed for loading window.
     * @param hint GLFW hint to replace
     * @param value GLFW value to set
     */
    void GLFWWindowBackend::setWindowHint(int hint, int value){
        glfwWindowHint(hint, value);
    }
}