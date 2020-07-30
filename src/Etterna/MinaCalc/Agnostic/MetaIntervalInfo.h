#pragma once
#include "IntervalInfo.h"

/* meta info is information that is derived from two or more consecutive
 * noteinfos, the first level of pattern abstraction is generated from noteinfo,
 * the second level of abstraction is generated from the meta info produced by
 * the first level of abstraction, and so on and so forth. meta info is
 * constructed on the fly per row and each row's meta info is able to look back
 * 3-4 rows into the past for relevant pattern information, in that sense meta
 * info contains information that persists beyond the explicit bounds of its
 * generation point, and that information may carry forward into the next
 * generation point, functionally speaking this has the effect of carrying
 * pattern sequence detection through intervals, reducing the error caused by
 * interval splicing*/

// remember this is hand _agnostic_ meaning it operates fully on note info, and
// needs no derived column logic
struct metaItvInfo
{
	ItvInfo _itvi;

	int _idx = 0;
	// meta info for this interval extracted from the base noterow progression
	int seriously_not_js = 0;
	int definitely_not_jacks = 0;
	int actual_jacks = 0;
	int actual_jacks_cj = 0;
	int not_js = 0;
	int not_hs = 0;
	int zwop = 0;
	int shared_chord_jacks = 0;
	bool dunk_it = false;

	// ok new plan instead of a map, keep an array of 3, run a comparison loop
	// that sets 0s to a new value if that value doesn't match any non 0 value,
	// and set a bool flag if we have filled the array with unique values
	std::array<unsigned, 3> row_variations = { 0, 0, 0 };
	int num_var = 0;
	// unique(noteinfos for interval) < 3, or row_variations[2] == 0 by interval
	// end
	bool basically_vibro = true;

	void reset()
	{
		// at the moment this also resets to default
		_itvi.handle_interval_end();

		_idx = 0;
		seriously_not_js = 0;
		definitely_not_jacks = 0;
		actual_jacks = 0;
		actual_jacks_cj = 0;
		not_js = 0;
		not_hs = 0;
		zwop = 0;
		shared_chord_jacks = 0;
		dunk_it = false;
		
		row_variations.fill(0);
		num_var = 0;

		basically_vibro = 0;
	}

	void handle_interval_end()
	{
		// isn't reset, preserve behavior. this essentially just tracks longer
		// sequences of single notes, we don't want it to be reset with
		// intervals, also there's probably a better way to implement this setup
		// seriously_not_js = 0;

		// alternating chordstream detected (used by cj only atm)
		definitely_not_jacks = 0;

		// number of shared jacks between to successive rows, used by js/hs to
		// depress jumpjacks
		actual_jacks = 0;

		// almost same thing as above (see comment in jack_scan)
		actual_jacks_cj = 0;

		// increased by detecting either long runs of single notes
		// (definitely_not_jacks > 3) or by encountering jumptrills, either
		// splithand or two hand, not_js and not_hs are the same thing, this
		// entire operation and setup should probably be split up and made more
		// explicit in each thing it detects and how those things are used
		not_js = 0;
		not_hs = 0;

		// recycle var for any int assignments
		zwop = 0;

		// self explanatory and unused atm
		shared_chord_jacks = 0;

		row_variations.fill(0);
		num_var = 0;

		// see def
		basically_vibro = true;
		dunk_it = false;

		// reset our interval info
		_itvi.handle_interval_end();
	}
};
