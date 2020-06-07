#pragma once
#include "Etterna/Globals/MinaCalcHelpers.h"
#include "HD_MetaSequencing.h"

// used by ranmen mod, for ranmen sequencing (doesn't have a sequence struct and
// probably should?? this should just be logic only)
struct RM_Sequencing
{
	// params.. loaded by runningman and then set from there
	int max_oht_len = 0;
	int max_off_spacing = 0;
	int max_burst_len = 0;
	int max_jack_len = 0;

	inline void set_params(const float& moht,
						   const float& moff,
						   const float& mburst,
						   const float& mjack)
	{
		max_oht_len = static_cast<int>(moht);
		max_off_spacing = static_cast<int>(moff);
		max_burst_len = static_cast<int>(mburst);
		max_jack_len = static_cast<int>(mjack);
	}

	col_type anchor_col = col_init;
	col_type now_col = col_init;

	// sequencing counters
	// only allow this rm's anchor col to start sequences
	bool in_the_nineties = false;
	// try to allow 1 burst?
	bool is_bursting = false;
	bool had_burst = false;

	int ran_taps = 0;
	int anchor_len = 0;

	int off_taps_same = 0;
	int oht_taps = 0;
	int oht_len = 0;
	int off_taps = 0;
	int off_len = 0;

	int jack_taps = 0;
	int jack_len = 0;

	float max_ms = ms_init;
	float now = 0.F;
	float temp_ms = 0.F;
	float last_anchor_time = s_init;

#pragma region functions

	inline void reset()
	{
		// don't touch anchor col

		// now_col and now don't need to be reset either

		in_the_nineties = false;
		is_bursting = false;
		had_burst = false;

		ran_taps = 0;
		anchor_len = 0;

		off_taps_same = 0;
		oht_taps = 0;
		oht_len = 0;
		off_taps = 0;
		off_len = 0;

		jack_taps = 0;
		jack_len = 0;

		max_ms = ms_init;
		last_anchor_time = ms_init;

		// if we are resetting and this column is the anchor col, restart again
		if (anchor_col == now_col) {
			handle_anchor_progression();
		}
	}

	inline void full_reset()
	{
		// don't touch anchor col

		reset();
		now = 0.F;
		now_col = col_init;
	}

	inline void handle_off_tap()
	{
		if (!in_the_nineties) {
			return;
		}

		++ran_taps;
		++off_taps;
		++off_len;

		// offnote, reset jack length & oht length
		jack_len = 0;

		// handle progression for increasing off_len
		handle_off_tap_progression();
	}

	inline void handle_off_tap_completion()
	{
		// if we end while bursting due to hitting an anchor, complete it
		if (is_bursting) {
			is_bursting = false;
			had_burst = true;
		}
		// reset off_len counter
		off_len = 0;
	}

	inline void handle_off_tap_progression()
	{
		// resume off tap progression caused by another consecutive off tap
		// normal behavior if we have already allowed for 1 burst, reset if the
		// offtap sequence exceeds the spacing limit; this will also catch
		// bursts that exceed the max burst length
		if (had_burst) {
			if (off_len > max_off_spacing) {
				reset();
				return;
			}
			// don't care about any other behavior here
			return;
		}

		// if we are in a burst, allow it to finish and when it does set the
		// had_burst flag rather than resetting, if the burst continues beyond
		// the max burst length then it will be reset via other means
		// (we must be in a burst if off_len == max_burst_len)
		if (off_len == max_burst_len) {
			handle_off_tap_completion();
			return;
		}

		// haven't had or started a burst yet, if we exceed max_off_spacing, set
		// is_bursting to true and allow it to continue, otherwise, do nothing
		if (off_len > max_off_spacing) {
			is_bursting = true;
		}
	}

	inline void handle_anchor_progression()
	{
		// start a sequence whenever we hit this rm's anchor col, if we aren't
		// already in one
		if (in_the_nineties) {
			// break the anchor if the next note is too much slower than the
			// lowest one, but only after we've already set the initial anchor
			temp_ms = ms_from(now, last_anchor_time);
			// account for float precision error and small bpm flux
			if (temp_ms > max_ms + 5.F) {
				reset();
			} else {
				max_ms = temp_ms;
			}
		} else {
			// set first anchor val
			max_ms = 5000.F;
			in_the_nineties = true;
		}

		last_anchor_time = now;
		++ran_taps;
		++anchor_len;

		// handle completion of off tap progression
		handle_off_tap_completion();
	}

	inline void handle_jack_progression()
	{
		++ran_taps;
		//++anchor_len; // do this for jacks?
		++jack_len;
		++jack_taps;

		// handle completion of off tap progression
		handle_off_tap_completion();

		// make sure to set the anchor col when resetting if we exceed max jack
		// len
		if (jack_len > max_jack_len) {
			reset();
		}
	}

	inline void handle_cross_column_branching()
	{
		// we are comparing 2 different enum types here, but this is what we
		// want. base_left_right is 0, col_left is 0. if we are base_left_right
		// then we have landed on the right column, so if we have cc (0) ==
		// anchor_col (0), we are entering the off column (right) of the anchor
		// (left). perhaps left_right and right_left should be flipped in the
		// base_pattern_type enum to make this more intuitive (but probably not)

		// NOT an anchor
		if (anchor_col != now_col && in_the_nineties) {
			handle_off_tap();
			// same hand offtap
			++off_taps_same;
			return;
		}
		handle_anchor_progression();
	}

	inline void handle_oht_progression()
	{
		// we only care about ohts that end off-anchor
		if (now_col != anchor_col) {
			++oht_len;
			++oht_taps;
			if (oht_len > max_oht_len) {
				reset();
			}
		}
	}

	inline void operator()(const col_type& ct,
						   const base_pattern_type& bt,
						   const meta_type& mt,
						   const float& row_time,
						   const int& offhand_taps)
	{

		now_col = ct;
		now = row_time;

		// play catch up, treat offhand jumps like 2 offtaps
		if (in_the_nineties && offhand_taps > 0) {
			// reset oht len if we hit this (not really robust buuuut)
			oht_len = 0;

			for (int i = 0; i < offhand_taps; ++i) {
				handle_off_tap();
			}
		}

		// cosmic brain
		if (mt == meta_cccccc) {
			handle_oht_progression();
		}

		switch (bt) {
			case base_left_right:
			case base_right_left:
				handle_cross_column_branching();
				break;
			case base_jump_single:
				if (offhand_taps > 0) {
					// if we have a jump -> single, and the last
					// note was an offhand tap, and the single
					// is the anchor col, then we have an anchor
					if ((now_col == col_left && anchor_col == col_left) ||
						(now_col == col_right && anchor_col == col_right)) {
						handle_anchor_progression();
					} else {
						// otherwise we have an off anchor tap
						handle_off_tap();
						// same hand offtap
						++off_taps_same;
					}
				} else {
					// if we are jump -> single and the last
					// note was not an offhand hand tap, we have
					// a jack
					handle_jack_progression();
				}
				break;
			case base_single_single:
			case base_single_jump:
				// if last note was an offhand tap, this is by
				// definition part of the anchor
				if (offhand_taps > 0) {
					handle_anchor_progression();
				} else {
					// if not, a jack
					handle_jack_progression();
				}
				break;
			case base_jump_jump:
				// this is kind of a gray area, given that
				// the difficulty of runningmen comes from
				// the tight turns on the same hand... we
				// will treat this as a jack even though
				// technically it's an "anchor" when the
				// last tap was an offhand tap
				handle_jack_progression();
				break;
			case base_type_init:
				if (now_col == anchor_col) {
					handle_anchor_progression();
				}
				break;
			default:
				assert(0);
				break;
		}
	}
#pragma endregion
};
