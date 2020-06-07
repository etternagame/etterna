#pragma once
#include "Etterna/Globals/MinaCalc/Dependent/HD_MetaSequencing.h"
#include "Etterna/Globals/MinaCalc/CalcWindow.h"

#include <array>
#include <initializer_list>

/* Contains generic sequencers passed to metahandinfo to be advanced in the row
 * loop */

// ok this has expanded into more than i bargained for an should be
// appropriately renamed and commented when i figure out what it is

/* important note about timing names used throughout the calc. anything being
 * updated with the ms value of this row to the last row with a note in it
 * independent of any other considerations should use the var name "any_ms", the
 * name of the ms value from the current row to the last row that contains a
 * note on the same column should be "sc_ms", for same column ms, and the only
 * other option is "cc_ms", for cross column ms */

// bpm flux float precision etc
static const float anchor_buffer_ms = 10.F;

// individual anchors, 2 objects per hand on 4k
struct Anchor_Sequencing
{
	col_type _ct = col_init;

	float _sc_ms = 0.F;

	// if we exceed this + buffer, break the anchor sequence
	float _max_ms = ms_init;

	// row_time of last note on this col
	float _last = s_init;

	int _len = 0;

	inline void full_reset()
	{
		// we don't need to reset col_type
		_sc_ms = 0.F;
		_max_ms = ms_init;
		_last = s_init;
		_len = 0;
	}

	inline void operator()(const col_type ct, const float& now)
	{
		assert(ct == _ct);
		_sc_ms = ms_from(now, _last);

		// break the anchor if the next note is too much slower than the
		// lowest one in the sequence
		if (_sc_ms > _max_ms + anchor_buffer_ms) {
			_len = 1;
			_max_ms = ms_init;
		} else {
			// increase anchor length and set new cutoff point
			++_len;
			_max_ms = _sc_ms;
		}
		_last = now;
	}
};

// CURRENTLY ALSO BEING USED TO STORE THE OLD CC/TC MS VALUES IN MHI...
// not that this is a great idea but it's appropriate for doing so
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

	// derives sc_ms, which sequencer general will pull for its moving window
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
				anch.at(c)(c, row_time);

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
	/* should maybe have this contain a struct that just handles timing, or is
	 * that overboard? */

	// moving window of ms_any values, practically speaking, row with something
	// in it on this hand to last row with something in it on the current hand
	CalcMovingWindow<float> _mw_any_ms;

	// moving window of cc_ms values
	CalcMovingWindow<float> _mw_cc_ms;

	// moving window of sc_ms values, not used but we will probably want
	// eventually (maybe? maybe move it? idk???)
	std::array<CalcMovingWindow<float>, num_cols_per_hand> _mw_sc_ms;

	/* basic sequencers */
	AnchorSequencer _as;

	// sc_ms is the time from the current note to the last note in the same
	// column, we've already updated the anchor sequencer and it will already
	// contain that value in _sc_ms
	inline void set_sc_ms(const col_type& ct)
	{
		// single notes are simple
		if (ct == col_left || ct == col_right)
			_mw_sc_ms.at(ct)(_as.anch.at(ct)._sc_ms);

		// oh jumps mean we do both, we will allow whatever is querying for the
		// value to choose which column value they want (lower by default)
		if (ct == col_ohjump) {
			for (auto& c : ct_loop_no_jumps) {
				_mw_sc_ms.at(c)(_as.anch.at(c)._sc_ms);
			}
		}
	}

	// cc_ms is the time from the current note to the last note in the cross
	// column, for this we need to take the last row_time on the cross column,
	// (anchor sequencer has it as _last) and derive a new ms value from it and
	// the current row_time
	inline void set_cc_ms(const col_type& ct, const float& row_time)
	{
		// single notes are simple, grab the _last of ct inverted
		if (ct == col_left || ct == col_right) {
			_mw_cc_ms(ms_from(row_time, _as.anch.at(invert_col(ct))._last));
		}

		// jumps are tricky, tehcnically we have 2 cc_ms values, but also
		// technically values are simply the sc_ms values we already calculated,
		// but inverted, given that the goal however is to provide general
		// values that various pattern mods can use such that they don't have to
		// all track their own custom sequences, we should place the lower sc_ms
		// value in here, since that's the most common use case, if something
		// needs to specifically handle ohjumps differently, it can do so
		// we do actually need to set this value so the calcwindow internal cv
		// checks will work, we can't just shortcut and make a get function
		// which swaps where it returns from
		if (ct == col_ohjump) {
			_mw_cc_ms(get_sc_ms_now(col_ohjump));
		}
	}

	// stuff
	inline void advance_sequencing(const col_type& ct,
								   const float& row_time,
								   const float& ms_now)
	{ // update sequencers
		_as(ct, row_time);

		// i guess we keep track of ms sequencing here instead of mhi, or
		// somewhere new?

		// sc ms needs to be set first, cc ms will reference it for ohjumps
		set_sc_ms(ct);
		set_cc_ms(ct, row_time);
		_mw_any_ms(ms_now);
	}

	inline auto get_sc_ms_now(const col_type& ct, bool lower = true) -> float
	{
		if (ct == col_init)
			return ms_init;

		// if ohjump, grab the smaller value by default
		if (ct == col_ohjump) {
			if (lower) {
				return _mw_sc_ms[col_left].get_now() <
						   _mw_sc_ms[col_right].get_now()
						 ? _mw_sc_ms[col_left].get_now()
						 : _mw_sc_ms[col_right].get_now();
			}
			{
				// return the higher value instead (dunno if we'll ever need
				// this but it's good to have the option)
				return _mw_sc_ms[col_left].get_now() >
						   _mw_sc_ms[col_right].get_now()
						 ? _mw_sc_ms[col_left].get_now()
						 : _mw_sc_ms[col_right].get_now();
			}
		}

		// simple
		return _mw_sc_ms[ct].get_now();
	}

	inline auto get_any_ms_now() -> float { return _mw_any_ms.get_now(); }

	inline void full_reset()
	{
		for (auto& c : ct_loop_no_jumps) {
			_mw_sc_ms[c].zero();
		}

		_as.full_reset();
	}
};
