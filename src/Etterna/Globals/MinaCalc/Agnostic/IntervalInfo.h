#pragma once
#include <array>
/* simple struct to accumulate raw note info across an interval as it's
 * processed by row, will be reset at the end of the interval */

enum tap_size
{
	single,
	jump,
	hand,
	quad,
	num_tap_size
};

struct ItvInfo
{
	int total_taps = 0;
	int chord_taps = 0;
	std::array<int, num_tap_size> taps_by_size = { 0, 0, 0, 0 };
	int mixed_hs_density_tap_bonus = 0;

	// resets all the stuff that accumulates across intervals
	void handle_interval_end()
	{
		// self explanatory
		total_taps = 0;

		// number of non-single taps
		chord_taps = 0;
		mixed_hs_density_tap_bonus = 0;

		taps_by_size.fill(0);
	}

	void update_tap_counts(const int& row_count)
	{
		total_taps += row_count;

		// ALWAYS COUNT NUMBER OF TAPS IN CHORDS
		if (row_count > 1) {
			chord_taps += row_count;
		}

		// ALWAYS COUNT NUMBER OF TAPS IN CHORDS
		taps_by_size.at(row_count - 1) += row_count;

		// maybe move this to metaitvinfo?
		// we want mixed hs/js to register as hs, even at relatively sparse hand
		// density
		if (taps_by_size[hand] > 0) {
			// this seems kinda extreme? it'll add the number of jumps in the
			// whole interval every hand? maybe it needs to be that extreme?
			mixed_hs_density_tap_bonus += taps_by_size[jump];
		}
	}
};
