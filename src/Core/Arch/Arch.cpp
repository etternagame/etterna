#include "Arch.hpp"

/** This file is where functions which have a cross-platform implementation may be defined. */
namespace Core::Arch::Time {

    /** TODO: Move time related functions to their own class/namespace */
    std::chrono::milliseconds GetChronoDurationSinceStart(){
        return std::chrono::milliseconds(std::chrono::steady_clock::now().time_since_epoch().count() / 1000000);
    }

} // namespace Core::Arch::Time