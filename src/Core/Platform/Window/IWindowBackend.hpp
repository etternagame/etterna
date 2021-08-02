#ifndef CORE_PLATFORM_WINDOW_WINDOW_HPP
#define CORE_PLATFORM_WINDOW_WINDOW_HPP

#include "Core/Utility/ActionDelegate.hpp"

#include <string>

namespace Core::Platform::Window {
    struct Dimensions {
        unsigned int width;
        unsigned int height;
    };

    class IWindowBackend {
    public:
        IWindowBackend(std::string_view title, const Dimensions &size);
        virtual ~IWindowBackend() = default;

    public:
        virtual void create() = 0;
        virtual void update() const = 0;
        virtual bool exited() const = 0;
        virtual void swapBuffers() const = 0;
        virtual void *getNativeWindow() const = 0;
        virtual void setTitle(const std::string& title) = 0;
        virtual Dimensions getFrameBufferSize() const = 0;
        virtual int getRefreshRate() const = 0;

        const std::string& getTitle() const;
        Dimensions getDimensions() const;

        // Callback Registration
        void registerOnMoved(const ActionDelegate<int, int>::CallbackFunc& func);
        void registerOnResized(const ActionDelegate<Dimensions>::CallbackFunc& func);
        void registerOnCloseRequested(const ActionDelegate<>::VoidFunc& func);
        void registerOnFocusLost(const ActionDelegate<>::VoidFunc& func);
        void registerOnFocusGain(const ActionDelegate<>::VoidFunc& func);
        void registerOnWindowMinimized(const ActionDelegate<>::VoidFunc& func);
        void registerOnWindowMaximized(const ActionDelegate<>::VoidFunc& func);

    protected:
        // Window data
        std::string title;
        Dimensions size;

        // Window Callbacks
        ActionDelegate<int, int> onMoved;         /** @brief Triggered when window position changes */
        ActionDelegate<Dimensions> onResized;     /** @brief Triggered when window changes size */
        ActionDelegate<> onCloseRequested;        /** @brief Triggered when window close button is clicked */
        ActionDelegate<> onFocusLost;             /** @brief Triggered when windows loses focus*/
        ActionDelegate<> onFocusGain;             /** @brief Triggered when window gains focus */
        ActionDelegate<> onWindowMinimized;       /** @brief Triggered when window is minimized */
        ActionDelegate<> onWindowMaximized;       /** @brief Triggered when window is maximized */
    };

}
#endif //CORE_PLATFORM_WINDOW_WINDOW_HPP