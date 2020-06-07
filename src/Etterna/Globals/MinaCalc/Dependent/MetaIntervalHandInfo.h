#pragma once
#include "IntervalHandInfo.h"

// this _may_ prove to be overkill
struct metaItvHandInfo
{
	ItvHandInfo _itvhi;

	// handle end of interval
	inline void interval_end()
	{
		for (auto& v : _base_pattern_types) {
			v = 0;
		}
		for (auto& v : _base_pattern_types) {
			v = 0;
		}

		_itvhi.interval_end();
	}

	// zero everything out for end of hand loop so the trailing values from the
	// left hand don't end up in the start of the right (not that it would make
	// a huge difference, but it might be abusable
	inline void zero()
	{
		for (auto& v : _base_pattern_types) {
			v = 0;
		}
		for (auto& v : _base_pattern_types) {
			v = 0;
		}

		_itvhi.zero();
	}

	int _base_pattern_types[num_base_types] = { 0, 0, 0, 0, 0, 0 };
	int _meta_types[num_meta_types] = { 0, 0, 0, 0, 0, 0 };
};
