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

#include <chrono>

const RageTimer RageZeroTimer(0);
static auto g_iStartTime = std::chrono::steady_clock::now();

float
RageTimer::GetTimeSinceStart()
{
	std::chrono::duration<float> t = std::chrono::steady_clock::now() - g_iStartTime;
	return t.count();
}

static double
GetTimeSinceStart64()
{
	std::chrono::duration<double> t = std::chrono::steady_clock::now() - g_iStartTime;
	return t.count();
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
	return tm < rhs.tm;
}

RageTimer
RageTimer::Sum(const RageTimer& lhs, float tm)
{
	const std::chrono::steady_clock::time_point ret =
		lhs.tm + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>(tm));
	return RageTimer(ret);
}

float
RageTimer::Difference(const RageTimer& lhs, const RageTimer& rhs)
{
	const std::chrono::duration<float> diff = lhs.tm - rhs.tm;
	return diff.count();
}

#include "Etterna/Singletons/LuaManager.h"
LuaFunction(GetTimeSinceStart, GetTimeSinceStart64())
