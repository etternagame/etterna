#include "IWindowBackend.hpp"

#include <utility>

namespace Core::Platform::Window {

    IWindowBackend::IWindowBackend(VideoMode  params) : videoMode(std::move(params)) {
    }


    const VideoMode& IWindowBackend::getVideoMode() const {
        return videoMode;
    }

    void IWindowBackend::registerOnMoved(const ActionDelegate<int, int>::CallbackFunc& func) {
        this->onMoved += func;
    }

    void IWindowBackend::registerOnWindowResized(const ActionDelegate<int, int>::CallbackFunc& func) {
        this->onResized += func;
    }

    void IWindowBackend::registerOnCloseRequested(const ActionDelegate<>::VoidFunc& func) {
        this->onCloseRequested += func;
    }

    void IWindowBackend::registerOnFocusLost(const ActionDelegate<>::VoidFunc& func) {
        this->onFocusLost += func;
    }

    void IWindowBackend::registerOnFocusGain(const ActionDelegate<>::VoidFunc& func) {
        this->onFocusGain += func;
    }

    void IWindowBackend::registerOnMaximized(const ActionDelegate<>::VoidFunc& func) {
        this->onWindowMaximized += func;
    }

    void IWindowBackend::registerOnMinimized(const ActionDelegate<>::VoidFunc& func) {
        this->onWindowMinimized += func;
    }

    void IWindowBackend::registerOnFrameBufferResized(const ActionDelegate<int, int>::CallbackFunc &func) {
        this->onFrameBufferResize += func;
    }

}