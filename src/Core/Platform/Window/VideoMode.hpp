#ifndef CORE_PLATFORM_WINDOW_VIDEOMODE_HPP
#define CORE_PLATFORM_WINDOW_VIDEOMODE_HPP

#include "Core/Misc/AppInfo.hpp"

namespace Core::Platform::Window {

    /**
     * VideoMode - A struct to pass along parameters about how the display should be configured
     */
    struct VideoMode {
        VideoMode() = default;
        std::string windowTitle{AppInfo::APP_TITLE};        /** @brief The title of the window */
        std::string windowIcon;                             /** @brief The path of the icon */
        int width{960};                                     /** @brief Window Height */
        int height{540};                                    /** @brief Window Width */
        int refreshRate{0};                                 /** @brief Window Refresh Rate */
        bool isVsyncEnabled{false};                         /** @brief If VSync is enabled */
        bool isBorderless{false};                           /** @brief If the windows has decorations, or if it's borderless. */
        bool isFullscreen{false};                           /** @brief If the windows is fullscreen. */
    };
}

#endif //CORE_PLATFORM_WINDOW_VIDEOMODE_HPP