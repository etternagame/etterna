#pragma once
#include "IntervalHandInfo.h"

// this _may_ prove to be overkill
struct metaItvHandInfo
{
	ItvHandInfo _itvhi;

	// handle end of interval
	void interval_end()
	{
		_base_types.fill(0);
		_meta_types.fill(0);

		_itvhi.interval_end();
	}

	// zero everything out for end of hand loop so the trailing values from the
	// left hand don't end up in the start of the right (not that it would make
	// a huge difference, but it might be abusable
	void zero()
	{
		_base_types.fill(0);
		_meta_types.fill(0);

		_itvhi.zero();
	}

	std::array<int, num_base_types> _base_types = { 0, 0, 0, 0, 0, 0 };
	std::array<int, num_meta_types> _meta_types = { 0, 0, 0, 0, 0, 0 };
};
