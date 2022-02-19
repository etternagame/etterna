#ifndef CORE_PLATFORM_DISPLAYMODE
#define CORE_PLATFORM_DISPLAYMODE

namespace Core::Platform {

    /**
     * Each DisplayMode struct represents the settings which the users monitors is capable of displaying.
     * Exists as a compatibility layer between `Core/` and `RageUtil/`. Without this, includes to `RageUtil`
     * would be necessary in `Core`.
     */
    struct DisplayMode {
        DisplayMode() = default;
        int width{0};
        int height{0};
        int refreshRate{0};
    };

}

#endif //CORE_PLATFORM_DISPLAYMODE