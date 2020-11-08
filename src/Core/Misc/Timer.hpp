#ifndef CORE_MISC_TIMER_HPP
#define CORE_MISC_TIMER_HPP

#include <chrono>

namespace Core::Timer {

    /**
     * By using std::chrono::steady_clock, we get a consistent clock, forward moving clock (monotonic),
     * that is not affected by time zones. std::chrono::system_clock is better for getting the wall-clock
     * time, and std::chrono::high_resolution_clock is usually an alias for one of the two previous clocks.
     */
    using timer_clock = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<timer_clock>;

    /**
     * All duration values will be returned as a standard_duration type, to keep consistency among all .count()
     * functions (regardless of how much their used). It can be changed for each template function.
     */
    using default_duration = std::chrono::milliseconds;

    /**
     * A time point storing the initialization time of the application. Most times in Etterna aren't concerned
     * with wall-clock time, but are concerned with delta time. This variable acts are a started point to track
     * deltas from.
     */
    extern time_point APP_START_TIME;

    /**
     * Get how long has passed since the program has started in the desired duration type.
     *
     * @tparam Duration std::chrono duration to return result in.
     *         Defaults to seconds to match RageTimer::GetTimeSinceStart()
     * @return The duration in desired units since application start.
     */
    template<class Duration=std::chrono::seconds> Duration getDeltaSinceStart() {
        return std::chrono::duration_cast<Duration>(timer_clock::now() - APP_START_TIME);
    }

    /**
     * Get how long has elapsed since the inputTime parameter
     * @tparam Duration std::chrono duration to return result in
     * @param inputTime Desired time to compare with
     * @return The duration from inputTime in the desired units
     */
    template<class Duration=default_duration> Duration getDuration(time_point inputTime) {
        return std::chrono::duration_cast<Duration>(timer_clock::now() - inputTime);
    }

    /**
     * Get a time_point representing the time the function was called.
     * @return timer_clock::now() of type std::chrono::time_point<timer_clock>
     */
    time_point getCurrentTime();

}


#endif //CORE_MISC_TIMER_HPP
