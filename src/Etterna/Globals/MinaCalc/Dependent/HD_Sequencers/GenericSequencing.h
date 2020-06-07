#pragma once
#include "Etterna/Globals/MinaCalc/Dependent/HD_MetaSequencing.h"
#include "Etterna/Globals/MinaCalc/CalcWindow.h"

#include <array>
#include <initializer_list>

/* Contains generic sequencers passed to metahandinfo to be advanced in the row
 * loop */

// ok this has expanded into more than i bargained for an should be
// appropriately renamed and commented when i figure out what it is

// bpm flux float precision etc
static const float anchor_buffer_ms = 10.F;

// individual anchors, 2 objects per hand on 4k
struct Anchor_Sequencing
{
	col_type _ct = col_init;

	float _now_ms = 0.F;

	// if we exceed this + buffer, break the anchor sequence
	float _max_ms = ms_init;

	// row_time of last note on this col
	float _last = s_init;

	int _len = 0;

	inline void full_reset()
	{
		// we don't need to reset col_type
		_now_ms = 0.F;
		_max_ms = ms_init;
		_last = s_init;
		_len = 0;
	}

	inline auto col_check(const col_type ct) -> bool
	{
		return ct == _ct || ct == col_ohjump;
	}

	inline void operator()(const col_type ct, const float& now)
	{
		if (col_check(ct)) {
			_now_ms = ms_from(now, _last);

			// break the anchor if the next note is too much slower than the
			// lowest one in the sequence
			if (_now_ms > _max_ms + anchor_buffer_ms) {
				_len = 1;
				_max_ms = ms_init;
			} else {
				// increase anchor length and set new cutoff point
				++_len;
				_max_ms = _now_ms;
			}
		}
		_last = now;
	}
};

struct AnchorSequencer
{
	std::array<Anchor_Sequencing, num_cols_per_hand> anch;
	std::array<int, num_cols_per_hand> max_seen = { 0, 0 };

	// track windows of highest anchor per col seen during an interval
	std::array<CalcMovingWindow<int>, num_cols_per_hand> _mw_max;

	AnchorSequencer()
	{
		anch[col_left]._ct = col_left;
		anch[col_right]._ct = col_right;
		full_reset();
	}

	inline void full_reset()
	{
		max_seen.fill(0);

		for (auto& c : ct_loop_no_jumps) {
			anch.at(c).full_reset();
			_mw_max.at(c).zero();
		}
	}

	// new plan, this will be updated from mhi, which will then use it to set
	// its tcms values
	inline void operator()(const col_type ct, const float& row_time)
	{
		// update the one
		if (ct == col_left || ct == col_right) {
			anch.at(ct)(ct, row_time);

			// set max seen for this col for this interval
			max_seen[ct] =
			  anch[ct]._len > max_seen[ct] ? anch[ct]._len : max_seen[ct];
		} else if (ct == col_ohjump) {

			// update both
			for (auto& c : ct_loop_no_jumps) {
				anch.at(c)(ct, row_time);

				// set max seen
				max_seen.at(c) =
				  anch.at(c)._len > max_seen[c] ? anch[c]._len : max_seen[c];
			}
		}
	}

	// returns max anchor length seen for the requested window
	[[nodiscard]] inline auto get_max_for_window_and_col(
	  const col_type& ct,
	  const int& window) const -> int
	{
		assert(ct < num_cols_per_hand);
		return _mw_max[ct].get_max_for_window(window);
	}

	inline void handle_interval_end()
	{
		for (auto& c : ct_loop_no_jumps) {
			_mw_max[c](max_seen[c]);
			max_seen[c] = 0;
		}
	}
};


/* keep timing stuff here instead of in mhi, use mhi exclusively for pattern
 * detection */

// every note has at least 2 ms values associated with it, the
// ms value from the last cross column note (on the same hand),
// and the ms value from the last note on it's/this column both
// are useful for different things, and we want to track both.
// for ohjumps, we will track the ms from the last non-jump on
// either finger, there are situations where we may want to
// consider jumps as having a cross column ms value of 0 with
// itself, not sure if they should be set to this or left at the
// init values of 5000 though

// more stuff could/should be moved here? the only major issue with moving _all_
// sequencers here is loading/setting their params
struct SequencerGeneral
{
	// moving window of ms_any values, practically speaking, row with something
	// in it on this hand to last row with something in it on the current hand
	CalcMovingWindow<float> _mw_ms_any;

	// moving window of tc_ms values, not used but we will probably want
	// eventually (maybe? maybe move it? idk???)
	std::array<CalcMovingWindow<float>, num_cols_per_hand> _mw_tc_ms;

	/* basic sequencers */
	AnchorSequencer _as;

	// stuff
	inline void advance_sequencing(const col_type ct, const float& row_time, const float& ms_now) {

		// update sequencers
		_as(ct, row_time);

		// set tcs ms for time now, for each column
		for (auto& c : ct_loop_no_jumps) {
			_mw_tc_ms[c](_as.anch[c]._now_ms);
		}

		// i guess we do this here instead??
		_mw_ms_any(ms_now);
	}

	inline void full_reset()
	{
		for (auto& c : ct_loop_no_jumps) {
			_mw_tc_ms[c].zero();
		}

		_as.full_reset();
	}
};
