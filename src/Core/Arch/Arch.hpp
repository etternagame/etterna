#ifndef CORE_ARCH_ARCH_HPP
#define CORE_ARCH_ARCH_HPP

#include <cstddef>
#include <string>

/**
 * A namespace to hold all system specific tasks. Intended to be implemented once
 * for each operating system/kernel.
 *
 * All std::string types returned must be UTF-8.
 */
namespace Core::Arch {

    /** @brief Platform-independent struct to store screen dimensions */
    struct ScreenDimensions { unsigned width; unsigned height; };

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
     * @brief Get the operating system's primary language.
     * @return A 2-letter RFC-639 language code.
     */
    std::string getSystemLanguage();

    /**
     * @brief Send a URL to be opened with the system's default web browser
     * @param url URL to open
     * @return true if successful, false if unsuccessful.
     */
    bool openWebsite(const std::string& url);

    /**
     * @brief Get contents of the system clipboard.
     * @return UTF-8 clipboard contents, or an empty string if the the clipboard is a non-text type.
     */
    std::string getClipboard();

    /**
     * @brief Get the location of the binary
     *
     * Using std::filesystem::path would be preferable, though we must target a minimum
     * version of macOS 10.15 for cross-platform availability.
     *
     * @return A UTF-8 string of the directory containing the binary.
     */
    std::string getExecutableDirectory();
}

#endif //CORE_ARCH_ARCH_HPP
