#include "IWindowBackend.hpp"

/**
 * This IWindowBackend abstract class implementation is for functions which have identical functionality
 * across all possible IWindowBackends.
 */
namespace Core::Platform::Window {

    IWindowBackend::IWindowBackend(std::string_view title, const Dimensions &size) : title(title), size(size) {}

    #pragma region Event Setters

    void IWindowBackend::setWindowCloseCallback(std::function<void()> function){
        this->onClose = std::move(function);
    }

    void IWindowBackend::setWindowFocusGainCallback(std::function<void()> function){
        this->onFocusGain = std::move(function);
    }

    void IWindowBackend::setWindowFocusLostCallback(std::function<void()> function){
        this->onFocusLost = std::move(function);
    }

    void IWindowBackend::setWindowResizeCallback(std::function<void(Dimensions)> function){
        this->onResize = std::move(function);
    }

    #pragma endregion

    #pragma region Getters and Setters

    std::string IWindowBackend::getTitle() {
        return this->title;
    }

    Dimensions IWindowBackend::getWindowDimensions() {
        return this->size;
    }

#pragma endregion
}
