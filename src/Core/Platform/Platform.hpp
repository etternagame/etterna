#ifndef CORE_ARCH_ARCH_HPP
#define CORE_ARCH_ARCH_HPP

#include <cstddef>
#include <string>
#include <ghc/filesystem.hpp>

/**
 * A namespace to hold all system specific tasks. Intended to be implemented once
 * for each operating system/kernel.
 *
 * All std::string types returned must be UTF-8.
 */
namespace Core::Platform {

    /** @brief Platform-independent struct to store screen dimensions */
    struct ScreenDimensions { unsigned width; unsigned height; };

    /**
     * Run any desired initialization processes specific
     * to a particular operating system.
     */
    void init();

    /**
     * @brief Determine the system name with the version number.
     * - Unix should return it's distributions name (Ubuntu 18.04, Debian 9, ArchLinux, Xubuntu...)
     * - Windows should return "Windows 10"
     * - Apple systems should return "macOS 10.15.4"
     */
    std::string getSystem();

    /**
     * @brief Determine if the current system is 64bit or 32bit.
     * @return If 32bit, return "i386". If 64bit, return "x86".
     */
    std::string getArchitecture();

    /**
     * @brief Determine the current systems kernel type and version
     * - Windows should return "Windows NT"
     * - Linux should return "Linux Kernel <version number>"
     * - Mac should return "Darwin"
     */
    std::string getKernel();

    /**
     * @brief Determine the total amount of memory available on a system.
     * @return RAM in bytes
     */
    std::size_t getSystemMemory();

    /**
     * @brief Get the system CPU information
     * String should include CPU Manufacturer, Model, and Frequency
     */
    std::string getSystemCPU();

    /**
     * @brief Get the primary system GPU information
     * String should include
     * - GPU Manufacturer
     * - GPU Model
     * - GPU Video RAM
     */
    std::string getSystemGPU();

    /**
     * @brief Get the screen dimensions of the screen the game window is currently displayed on.
     * @return A pair with the first being the width, and second, height
     */
    ScreenDimensions getScreenDimensions();

    /**
     * @brief Get the dimensions of the game window.
     * @return A pair with the first being the width, and second, height
     */
    ScreenDimensions getWindowDimensions();

    /**
     * @brief Get the users preferred language.
     * @return A 2-letter RFC-639 language code, using "en" as the default value.
     */
    std::string getLanguage();

    /**
     * @brief Send a URL to be opened with the system's default web browser
     * @param url URL to open
     * @return true if successful, false if unsuccessful.
     */
    bool openWebsite(const std::string& url);

    /**
     * @brief Send a path to be opened with the system's default file browser
     * @param path Path to open
     * @return true if successful, false if unsuccessful.
     */
    bool openFolder(const ghc::filesystem::path& path);

    /**
     * @brief Get contents of the system clipboard.
     * @return UTF-8 clipboard contents, or an empty string if the the clipboard is a non-text type.
     */
    std::string getClipboard();

    /**
     * Set if the mouse is visible or invisible
     * @param value True if visible mouse desired, false if not.
     */
    void setCursorVisible(bool value);

    /**
     * @brief Get the location of the binary
     *
     * @return A UTF-8 string of the directory containing the binary.
     */
    ghc::filesystem::path getExecutableDirectory();

    /**
     * @brief Get the base game directory. "The etterna folder".
     *
     * This is the folder where the user chose to install the application.
     * NOTE: This directory is retrieved based on the binary location.
     *
     * - Windows: Will return one directory up from binary location.
     * - macOS and Linux: Will return same directory as getExecutableDirectory.
     *
     * If the binary locations are changed, the functions and documentation must be changed accordingly.
     *
     * @return A UTF-8 string of the directory containing the binary.
     */
    ghc::filesystem::path getAppDirectory();

    /**
     * Check if another instance of this application is already running
     * @param argc argc from main
     * @param argv argv from main
     * @return True if other instance is running, False otherwise
     */
    bool isOtherInstanceRunning(int argc, char** argv);

    namespace Time {

        /**
         * @return Get microseconds since computer boot time.
         *
         * This function is defined in the universal "Platform.cpp", as after testing on each
         * platform, std::chrono was able to return a value similar to the previously used
         * GetMicrosecondsSinceStart. The API used on each platform is the same, or a better
         * API that was wrapped around std::chrono
         */
        std::chrono::milliseconds GetChronoDurationSinceStart();
    } // namespace Time
} // namespace Core::Platform

#endif //CORE_ARCH_ARCH_HPP
