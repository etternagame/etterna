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

#include "arch/ArchHooks/ArchHooks.h"

#include <chrono>

// this is hereby the difference between a second and a microsecond
#define INT_TIMESTAMP_RESOLUTION 1000000
#define FLOAT_TIMESTAMP_RESOLUTION 1000000.f

const RageTimer RageZeroTimer(0);
static std::chrono::microseconds g_iStartTime =
  ArchHooks::GetChronoDurationSinceStart();

static uint64_t
GetTime()
{
	return ArchHooks::GetMicrosecondsSinceStart();
}

static std::chrono::microseconds
GetChronoTime()
{
	return ArchHooks::GetChronoDurationSinceStart();
}

float
RageTimer::GetTimeSinceStart()
{
	auto usecs = GetChronoTime();
	std::chrono::microseconds g = usecs - g_iStartTime;

	return g.count() / FLOAT_TIMESTAMP_RESOLUTION;
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
	if (c_dur != rhs.c_dur)
		return c_dur < rhs.c_dur;
	return c_dur < rhs.c_dur;
}

RageTimer
RageTimer::Sum(const RageTimer& lhs, float tm)
{
	uint64_t usecs = static_cast<uint64_t>(tm * INT_TIMESTAMP_RESOLUTION);
	std::chrono::microseconds period(usecs);

	RageTimer ret(0); // Prevent unnecessarily checking the time
	ret.c_dur = period + lhs.c_dur;

	return ret;
}

float
RageTimer::Difference(const RageTimer& lhs, const RageTimer& rhs)
{
	std::chrono::microseconds diff = lhs.c_dur - rhs.c_dur;

	return static_cast<float>(diff.count()) / FLOAT_TIMESTAMP_RESOLUTION;
}

#include "Etterna/Singletons/LuaManager.h"
LuaFunction(GetTimeSinceStart, RageTimer::GetTimeSinceStart())

  /*
   * Copyright (c) 2001-2003 Chris Danford, Glenn Maynard
   * All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, and/or sell copies of the Software, and to permit persons to
   * whom the Software is furnished to do so, provided that the above
   * copyright notice(s) and this permission notice appear in all copies of
   * the Software and that both the above copyright notice(s) and this
   * permission notice appear in supporting documentation.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
   * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
   * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
   * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
   * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
   * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
   * PERFORMANCE OF THIS SOFTWARE.
   */
