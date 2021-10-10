#ifndef CORE_PLATFORM_WINDOW_WINDOW_HPP
#define CORE_PLATFORM_WINDOW_WINDOW_HPP

#include "Core/Utility/ActionDelegate.hpp"
#include "Core/Platform/Window/VideoMode.hpp"

#include <string>

namespace Core::Platform::Window {
    struct Dimensions {
        int width;
        int height;
    };

    class IWindowBackend {
    public:
        explicit IWindowBackend(VideoMode  params);
        virtual ~IWindowBackend() = default;

        // Context Related
        virtual void create() = 0;
        virtual void update() const = 0;
        virtual bool exited() const = 0;
        virtual void swapBuffers() const = 0;
        virtual void setContext() const = 0;
        virtual void clearContext() const = 0;

        // Data Related
        virtual void *getNativeWindow() const = 0;
        virtual void setTitle(const std::string& title) = 0;
        virtual Dimensions getFrameBufferSize() const = 0;
        virtual int getRefreshRate() const = 0;

        const VideoMode& getVideoMode() const;

        // Callback Registration
        void registerOnMoved(const ActionDelegate<int, int>::CallbackFunc& func);
        void registerOnWindowResized(const ActionDelegate<int, int>::CallbackFunc& func);
        void registerOnCloseRequested(const ActionDelegate<>::VoidFunc& func);
		void registerOnFocusGain(const ActionDelegate<>::VoidFunc& func);
        void registerOnFocusLost(const ActionDelegate<>::VoidFunc& func);
		void registerOnMaximized(const ActionDelegate<>::VoidFunc& func);
		void registerOnMinimized(const ActionDelegate<>::VoidFunc& func);
		void registerOnFrameBufferResized(const ActionDelegate<int, int>::CallbackFunc& func);

    protected:
        // Window data
        VideoMode videoMode;

        // Window Callbacks
        ActionDelegate<int, int> onMoved;               /** @brief Triggered when window position changes */
        ActionDelegate<int, int> onResized;             /** @brief Triggered when window changes size */
        ActionDelegate<> onCloseRequested;              /** @brief Triggered when window close button is clicked */
        ActionDelegate<> onFocusLost;                   /** @brief Triggered when windows loses focus*/
        ActionDelegate<> onFocusGain;                   /** @brief Triggered when window gains focus */
        ActionDelegate<> onWindowMinimized;             /** @brief Triggered when window is minimized */
        ActionDelegate<> onWindowMaximized;             /** @brief Triggered when window is maximized */
        ActionDelegate<int, int> onFrameBufferResize;   /** @brief Triggered when framebuffer is resized */
    };

}
#endif //CORE_PLATFORM_WINDOW_WINDOW_HPP