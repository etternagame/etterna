#pragma once
#include "HD_BasicSequencing.h"
#include "HD_MetaSequencing.h"
#include "Etterna/Globals/MinaCalc/Dependent/HD_Sequencers/GenericSequencing.h"
#include "Etterna/Globals/MinaCalc/CalcWindow.h"

/* this is a row by row sequencer that constructs basic and advanced hand based
 * patterns given noteinfo input for a current row, and its own output of the
 * last row */

// perhaps this should contain no timing information, only pattern information?

// potentially outdated comment below
/// this should contain most everything needed for the generic pattern mods,
/// extremely specific sequencing will take place in separate areas like with
/// rm_seuqencing, and widerange scalers should track their own interval queues
/// metanoteinfo is generated per row, from current noteinfo and the previous
/// metanoteinfo object, each metanoteinfo stores some basic information from the
/// last object, allowing us to look back 3-4 rows into the past without having
/// to explicitly store more than 2 mni objects, and we can recycle the pointers
/// as we generate the info
/// metanoteinfo is generated per _hand_, not per note or column. it contains the
/// relevant information for determining what the column configuation of each
/// hand is for any row, and it contains timestamp arrays for each column, so it
/// is unnecessary to generate information per note, even though in some ways it
/// might be more convenient or clearer

struct metaHandInfo
{
	// time (s) of the last seen note in each column
	float row_time = s_init;
	unsigned row_notes = 0U;

	float col_time[num_cols_per_hand] = { s_init, s_init };
	float col_time_no_jumps[num_cols_per_hand] = { s_init, s_init };

	// col
	col_type col = col_init;
	col_type last_col = col_init;

	// type of cross column hit
	base_pattern_type cc = base_type_init;
	base_pattern_type last_cc = base_type_init;

	// needed for the BIGGEST BRAIN PLAYS
	base_pattern_type last_last_cc = base_type_init;

	// whomst've
	meta_type mt = meta_type_init;
	meta_type last_mt = meta_type_init;

	// number of offhand taps before this row
	int offhand_taps = 0;
	int offhand_ohjumps = 0;

	// ms from last cross column note
	float cc_ms_any = ms_init;

	// ms from last cross column note, ignoring any oh jumps
	float cc_ms_no_jumps = ms_init;

	// ms from last note in this column
	float tc_ms = ms_init;

	// we need to reset everything between hands or the trailing values from the
	// end of one will carry over into the start of the other, not a huge
	// practical deal but it could theoretically be abused and it's good
	// practice to reset anyway
	inline void full_reset()
	{
		row_time = s_init;
		row_notes;

		for (auto& v : col_time) {
			v = s_init;
		}
		for (auto& v : col_time_no_jumps) {
			v = s_init;
		}

		col = col_init;
		last_col = col_init;

		cc = base_type_init;
		last_cc = base_type_init;
		last_last_cc = base_type_init;

		mt = meta_type_init;
		last_mt = meta_type_init;

		offhand_taps = 0;
		offhand_ohjumps = 0;

		cc_ms_any = ms_init;
		cc_ms_no_jumps = ms_init;
		tc_ms = ms_init;
	}

	inline void update_col_times(const float& val)
	{
		// update both
		if (col == col_ohjump) {
			col_time[col_left] = val;
			col_time[col_right] = val;
			return;
		}
		col_time[col] = val;
		col_time_no_jumps[col] = val;
	}

	// sets time from last note in the same column, and last note in the
	// opposite column, handling for jumps is not completely fleshed out yet
	// maybe, i think any case specific handling of their timings can be done
	// with the information already given
	inline void set_timings(const float last[], const float last_no_jumps[])
	{
		switch (cc) {
			case base_type_init:
			case base_left_right:
			case base_right_left:
			case base_single_single:
			case base_jump_single:
				// either we know the end col so we know the start col, or the
				// start col doesn't matter
				cc_ms_any = ms_from(col_time[col], last[invert_col(col)]);
				cc_ms_no_jumps =
				  ms_from(col_time[col], last_no_jumps[invert_col(col)]);

				// technically doesn't matter if we use last_col to index, if
				// it's single -> single we know it's an anchor so it's more
				// intuitive to use col twice
				tc_ms = ms_from(col_time[col], last[col]);
				break;
			case base_single_jump:
				// tracking this for now, use the higher value of the array
				// (lower ms time, i.e. the column closest to this jump)
				if (last[col_left] < last[col_right]) {
					cc_ms_any = ms_from(col_time[col_left], last[col_right]);
				} else {
					cc_ms_any = ms_from(col_time[col_right], last[col_left]);
				}

				// make sure this doesn't make sense
				cc_ms_no_jumps = ms_init;

				// logically the same as cc_ms_any in 1[12] 1 is the anchor
				// timing with 1 and also the cross column timing with 2
				tc_ms = cc_ms_any;
				break;
			case base_jump_jump:
				cc_ms_any = ms_init;
				// make sure this doesn't make sense
				cc_ms_no_jumps = ms_init;

				// indexes don't matter-- except that we can't use col or
				// last_col (because index 2 is outside array size)
				tc_ms = ms_from(col_time[0], last[0]);
				break;
			default:
				assert(0);
				break;
		}
	}

	inline void operator()(const metaHandInfo& last,
						   CalcMovingWindow<float>& ms_any,
						   const float& now,
						   const col_type& ct,
						   const unsigned& notes)
	{
		// this should never ever be called on col_empty
		assert(ct != col_empty);

		col = ct;
		row_time = now;
		row_notes = notes;

		// set older values, yeah yeah, i know
		last_col = last.col;

		last_last_cc = last.last_cc;
		last_cc = last.cc;

		last_mt = last.mt;

		col_time[col_left] = last.col_time[col_left];
		col_time[col_right] = last.col_time[col_right];
		col_time_no_jumps[col_left] = last.col_time_no_jumps[col_left];
		col_time_no_jumps[col_right] = last.col_time_no_jumps[col_right];

		/* ok now actually do stuff */

		// update this hand's cc type for this row
		cc = determine_base_pattern_type(col, last_col);

		// now that we have determined base_pattern_type, we can look for more complex
		// patterns
		mt = determine_meta_type(cc, last_cc, last_last_cc, last.last_last_cc);

		// every note has at least 2 ms values associated with it, the
		// ms value from the last cross column note (on the same hand),
		// and the ms value from the last note on it's/this column both
		// are useful for different things, and we want to track both.
		// for ohjumps, we will track the ms from the last non-jump on
		// either finger, there are situations where we may want to
		// consider jumps as having a cross column ms value of 0 with
		// itself, not sure if they should be set to this or left at the
		// init values of 5000 though

		// we will need to update time for one or both cols
		update_col_times(now);
		set_timings(last.col_time, last.col_time_no_jumps);

		// keep track of these ms values here so we aren't doing it in
		// potentially 5 different pattern mods
		ms_any(cc_ms_any);
	}
};
