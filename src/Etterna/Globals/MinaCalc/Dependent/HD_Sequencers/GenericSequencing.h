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
static const float anchor_speed_increase_cutoff_factor = 2.1F;

enum anch_status
{
	reset_too_slow,
	reset_too_fast,

	// _len > 2, otherwise we would be at the start of a file, or just reset due
	// to being too fast/slow
	anchoring,
	anch_init,
};

// individual anchors, 2 objects per hand on 4k
// if you are struggling with the notion that a 300 bpm oht is considered 2
// anchors of roughly equivalent lengths (depending on where you sample) and
// equivalent times, the difference between an anchor and a jack is that one
// cares about the existence of notes on other columns while the other doesn't,
// as in, it would make far less sense to call them jacks
struct Anchor_Sequencing
{
	// what column this anchor is on (will be set on startup by the sequencer)
	col_type _ct = col_init;
	anch_status _status = anch_init;

	// note: aside from the first note, _len is always at least 2
	/* outside of note 1 we are always in a 2 note anchor of some description.
	 * given 50, 500, 50, (4 notes), we have 2 notes, 1 and 2, 50 ms apart, and
	 * we are in an anchor. the 500 ms then breaks it due to being too much
	 * slower and starts a new anchor with notes 2 and 3, a 500 ms anchor. then
	 * the same thing happens again on reaching note 4, where the 50 breaks the
	 * 500 anchor due to being too fast, and again starts a new sequence with
	 * 3-4, this may seem like needless quibbling but if we are using anchor
	 * sequencing as the base for jack difficulty we want to ensure that cutoff
	 * points are reasonable, and that any point may be queried for a jack
	 * difficulty regardless of whether or not a human would consider it to be a
	 * jack
	 */

	int _len = 1;
	float _sc_ms = 0.F;

	// if we exceed this + buffer, break the anchor sequence
	float _max_ms = ms_init;

	// row_time of last note on this col
	float _last = s_init;
	float _start = s_init;

	inline void full_reset()
	{
		// never reset col_type
		_sc_ms = 0.F;
		_max_ms = ms_init;
		_last = s_init;
		_len = 1;
		_status = anch_init;
	}

	inline void operator()(const col_type ct, const float& now)
	{
		assert(ct == _ct);
		_sc_ms = ms_from(now, _last);

		// break the anchor if the next note is too much slower than the
		// lowest one in the sequence, remember, if we reset the start of the
		// new anchor was the last row_time, and the new max_ms should be the
		// current ms value

		if (_sc_ms > _max_ms + anchor_buffer_ms) {
			_status = reset_too_slow;
		} else if (_sc_ms * 2.5F < _max_ms) {
			_status = reset_too_fast;
		} else {
			_status = anchoring;
		}

		switch (_status) {
			case reset_too_slow:
			case reset_too_fast:
				//  i don't like hard cutoffs much but in the interest of
				//  fairness
				//  if the current ms value is vastly lower than the _max_ms,
				//  set the start time of the anchor to now and reset, i can't
				//  really think of any way this can be abused in a way that
				//  inflates files, just lots of ways it can underdetect

				// we're resetting because we've started on something much
				// faster or slower, so we know the start of this anchor was
				// actually the, last note, directly reset _max_ms to the
				// current ms and len to 2

				_start = _last;
				_max_ms = _sc_ms;
				_len = 2;
				break;
			case anchoring:
				// increase anchor length and set new cutoff point
				++_len;
				_max_ms = _sc_ms;
				break;
			case anch_init:
				// nothing to do
				break;
		}

		// update row time after any potential resets
		_last = now;
	}

	inline auto get_difficulty() -> float
	{
		float flool = ms_from(_last, _start);
		float pule = (flool + 360.F) / static_cast<float>(_len);
		float drool = ms_to_scaled_nps(pule);
		return drool;
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
				max_seen.at(c) = anch.at(c)._len > max_seen.at(c)
								   ? anch.at(c)._len
								   : max_seen.at(c);
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
			_mw_max.at(c)(max_seen.at(c));
			max_seen.at(c) = 0;
		}
	}

	inline auto get_highest_anchor_difficulty() -> float
	{
		return max(anch.at(col_left).get_difficulty(),
				   anch.at(col_right).get_difficulty());
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

	int itv_row_counter = 0;
	float cv_check_sum = 0.F;

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

		cccccc_check();
		cv_check_sum += _cv_cv();
		++itv_row_counter;
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
	inline auto get_cc_ms_now() -> float { return _mw_cc_ms.get_now(); }

	inline void full_reset()
	{
		for (auto& c : ct_loop_no_jumps) {
			_mw_sc_ms.at(c).zero();
		}

		_as.full_reset();
	}

	inline void interval_end()
	{
		cv_check_sum = 0.F;
		itv_row_counter = 0;
	}

	std::array<float, 6> _cv_check = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
	CalcMovingWindow<float> _cv_check2;

	[[nodiscard]] inline auto cccccc_check() -> float
	{
		float ms_now = _mw_any_ms.get_now();
		_cv_check[5] = _mw_sc_ms[col_left].get_now();
		_cv_check[4] = _mw_sc_ms[col_right].get_now();
		// we should be doing this maybe but idk if it made things better or
		// worse, need to look more closely
		//_cv_check[3] = ms_now * 3.f;

		float test1 = check_cv(3);
		return test1;
	}

	inline auto _cv_cv() -> float { return _cv_check2.get_cv_of_window(3); }

	[[nodiscard]] inline auto get_cv_mean(const int& window) -> float
	{
		float o = 0.f;

		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o += _cv_check.at(i);
		}
		_cv_check2(o);
		return static_cast<float>(o) / static_cast<float>(window);
	}

	[[nodiscard]] inline auto check_cv(const int& window) -> float
	{
		float sd = 0.F;
		float avg = get_cv_mean(window);
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			sd += (static_cast<float>(_cv_check.at(i)) - avg) *
				  (static_cast<float>(_cv_check.at(i)) - avg);
		}

		return fastsqrt(sd / static_cast<float>(window)) / avg;
	}
};
