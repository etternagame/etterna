#include "IWindowBackend.hpp"

namespace Core::Platform::Window {

    IWindowBackend::IWindowBackend(std::string_view title, const Dimensions &size) : title(title), size(size) {
    }


    const std::string& IWindowBackend::getTitle() const {
        return title;
    }

    Dimensions IWindowBackend::getDimensions() const {
        return size;
    }

    void IWindowBackend::registerOnMoved(const ActionDelegate<int, int>::CallbackFunc& func) {
        this->onMoved += func;
    }

    void IWindowBackend::registerOnResized(const ActionDelegate<Dimensions>::CallbackFunc& func) {
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

    void IWindowBackend::registerOnWindowMinimized(const ActionDelegate<>::VoidFunc& func) {
        this->onWindowMinimized += func;
    }

    void IWindowBackend::registerOnWindowMaximized(const ActionDelegate<>::VoidFunc& func) {
        this->onWindowMaximized += func;
    }

}