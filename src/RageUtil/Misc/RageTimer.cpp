/*
 * This can be used in two ways: as a timestamp or as a timer.
 *
 * As a timer,
 * RageTimer Timer;
 * for(;;) {
 *   printf( "Will be approximately: %f", Timer.PeekDeltaTime()) ;
 *   float fDeltaTime = Timer.GetDeltaTime();
 * }
 *
 * or as a timestamp:
 * void foo( RageTimer &timestamp ) {
 *     if( timestamp.IsZero() )
 *         printf( "The timestamp isn't set." );
 *     else
 *         printf( "The timestamp happened %f ago", timestamp.Ago() );
 *     timestamp.Touch();
 *     printf( "Near zero: %f", timestamp.Age() );
 * }
 */

#include "Etterna/Globals/global.h"

#include "RageTimer.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Core/Platform/Platform.hpp"

#include <chrono>

// let this henceforth be referred to as the difference between a second and a
// microsecond *kingly fanfare*
#define TIMESTAMP_RESOLUTION 1000000

const RageTimer RageZeroTimer(0);
static std::chrono::microseconds g_iStartTime = Core::Platform::Time::GetChronoDurationSinceStart();

static uint64_t GetTime() {
    return Core::Platform::Time::GetChronoDurationSinceStart().count();
}

static std::chrono::microseconds GetChronoTime() {
	return Core::Platform::Time::GetChronoDurationSinceStart();
}

float
RageTimer::GetTimeSinceStart()
{
	const auto usecs = GetChronoTime();
	const std::chrono::microseconds g = usecs - g_iStartTime;

	return static_cast<float>(g.count()) / TIMESTAMP_RESOLUTION;
}

uint64_t
RageTimer::GetUsecsSinceStart()
{
	return GetTime() - g_iStartTime.count();
}

void
RageTimer::Touch()
{
	this->c_dur = GetChronoTime();
}

float
RageTimer::Ago() const
{
	const RageTimer Now;
	return Now - *this;
}

float
RageTimer::GetDeltaTime()
{
	const RageTimer Now;
	const float diff = Difference(Now, *this);
	*this = Now;
	return diff;
}

/*
 * Get a timer representing half of the time ago as this one.  This is
 * useful for averaging time.  For example,
 *
 * RageTimer tm;
 * ... do stuff ...
 * RageTimer AverageTime = tm.Half();
 * printf( "Something happened approximately %f seconds ago.\n", tm.Ago() );
 */
RageTimer
RageTimer::Half() const
{
	const float fProbableDelay = Ago() / 2;
	return *this + fProbableDelay;
}

RageTimer
RageTimer::operator+(float tm) const
{
	return Sum(*this, tm);
}

float
RageTimer::operator-(const RageTimer& rhs) const
{
	return Difference(*this, rhs);
}

bool
RageTimer::operator<(const RageTimer& rhs) const
{
	return c_dur < rhs.c_dur;
}

RageTimer
RageTimer::Sum(const RageTimer& lhs, float tm)
{
	const uint64_t usecs = static_cast<uint64_t>(tm * TIMESTAMP_RESOLUTION);
	const std::chrono::microseconds period(usecs);

	RageTimer ret(0); // Prevent unnecessarily checking the time
	ret.c_dur = period + lhs.c_dur;

	return ret;
}

float
RageTimer::Difference(const RageTimer& lhs, const RageTimer& rhs)
{
	const std::chrono::microseconds diff = lhs.c_dur - rhs.c_dur;

	return static_cast<float>(diff.count()) / TIMESTAMP_RESOLUTION;
}

#include "Etterna/Singletons/LuaManager.h"
LuaFunction(GetTimeSinceStart, RageTimer::GetTimeSinceStart())
