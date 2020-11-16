#include "Arch.hpp"

/** This file is where functions which have a cross-platform implementation may be defined. */
namespace Core::Arch::Time {

    /** TODO: Move time related functions to their own class/namespace */
    std::chrono::microseconds GetChronoDurationSinceStart(){
        return std::chrono::microseconds(std::chrono::steady_clock::now().time_since_epoch().count() / 1000);
    }

} // namespace Core::Arch::Time