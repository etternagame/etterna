#include "Etterna/Globals/global.h"
#include "ArchHooks.h"

/*
 * This is a helper for GetMicrosecondsSinceStart on systems with a system
 * timer that may loop or move backwards.
 *
 * The time may decrease last for at least two reasons:
 *
 * 1. The underlying timer may be 32-bit and use millisecs internally, in which
 * case the timer will loop every 2^32 ms.
 *
 * 2. The underlying clock may have moved backwards (eg. system clock and ntpd).
 *
 * If the system clock moves backwards, we can't just clamp the time; if it
 * moved back an hour, we'd sit around for an hour until it catches up.
 *
 * Keep track of an offset: the amount of time to add to the result.  If we move
 * back by 100ms, the offset will be increased by 100ms.  If we loop, the offset
 * will be increased by the duration 2^32 ms.
 *
 * This helper only needs to be used if one or both of the above conditions can
 * occur. If the underlying timer is reliable, this doesn't need to be used (for
 * a small efficiency bonus).  Also, you may omit this for
 * GetMicrosecondsSinceStart() when bAccurate == false.
 */

int64_t
ArchHooks::FixupTimeIfLooped(int64_t usecs)
{
	static int64_t last = 0;
	static int64_t offset_us = 0;

	/* The time has wrapped if the last time was very high and the current time
	 * is very low. */
	const int64_t i32BitMaxMs = uint64_t(1) << 32;
	const int64_t i32BitMaxUs = i32BitMaxMs * 1000;
	const int64_t one_day = uint64_t(24 * 60 * 60) * 1000000;
	if (last > (i32BitMaxUs - one_day) && usecs < one_day)
		offset_us += i32BitMaxUs;

	last = usecs;

	return usecs + offset_us;
}

int64_t
ArchHooks::FixupTimeIfBackwards(int64_t usecs)
{
	static int64_t last = 0;
	static int64_t offset_us = 0;

	if (usecs < last) {
		/* The time has moved backwards.  Increase the offset by the amount we
		 * moved. */
		offset_us += last - usecs;
	}

	last = usecs;

	return usecs + offset_us;
}
