#pragma once
#include <cassert>

/* everything needed for basic sequencing for hand dependent patterns, this file
 * is dependent on nothing else and only operates on unsigned ints, and things
 * it definess */

// note: this are clearly restricted to 4k

// refrence notemap
// "----", "1---", "-1--", "11--",
// "--1-", "1-1-", "-11-", "111-",
// "---1", "1--1", "-1-1", "11-1",
// "--11", "1-11", "-111", "1111" };

// col_type is determined from serialized notedata, successive column types are
// used to build base patterns, sometimes we care only about left and right,
// sometimes we care about left and right and jumps, it's not perfect
enum col_type
{
	col_left,
	col_right,
	col_ohjump,
	num_col_types,
	col_empty,
	col_init
};

static const int num_cols_per_hand = 2;
static const std::array<col_type, num_col_types> ct_loop = { col_left,
															 col_right,
															 col_ohjump };
static const std::array<col_type, num_cols_per_hand> ct_loop_no_jumps = {
	col_left,
	col_right
};

static inline auto
determine_col_type(const unsigned& notes, const unsigned& hand_id) -> col_type
{
	const unsigned shirt = notes & hand_id;
	if (shirt == 0) {
		return col_empty;
	}

	if (hand_id == 3) {
		if (shirt == 3) {
			return col_ohjump;
		}
		if (shirt == 1) {
			return col_left;
		}
		if (shirt == 2) {
			return col_right;
		}
	} else if (hand_id == 12) {
		if (shirt == 12) {
			return col_ohjump;
		}
		if (shirt == 8) {
			return col_right;
		}
		if (shirt == 4) {
			return col_left;
		}
	}
	assert(0);
	return col_init;
}

// inverting ct type for col_left or col_right only, col name here is very much
// intentional
inline auto
invert_col(const col_type& col) -> col_type
{
	assert(col == col_left || col == col_right);
	return col == col_left ? col_right : col_left;
}

// basic pattern permutations are based on two successive non-empty col types as
// noted above. this is important, for basic pattern structure we do not care
// about any notes on the other hand, we only care about sequences of notes on
// this hand, this means that no sequencing done by the row by row sequencer
// should take place if col == col_empty (nothing on this hand)
enum base_type
{
	base_left_right,
	base_right_left,
	base_jump_single,
	base_single_single,
	base_single_jump,
	base_jump_jump,
	num_base_types,
	base_type_init,
};

inline auto
determine_base_pattern_type(const col_type& now, const col_type& last)
  -> base_type
{
	if (last == col_init) {
		return base_type_init;
	}

	const bool single_tap = now == col_left || now == col_right;
	if (last == col_ohjump) {
		if (single_tap) {
			return base_jump_single;
		}
		{ // can't be anything else
			return base_jump_jump;
		}
	} else if (!single_tap) {
		return base_single_jump;
		// if we are on left col _now_, we are right to left
	} else if (now == col_left && last == col_right) {
		return base_right_left;
	} else if (now == col_right && last == col_left) {
		return base_left_right;
	} else if (now == last) {
		// anchor/jack
		return base_single_single;
	}

	// makes no logical sense
	assert(0);
	return base_type_init;
}

// note, base_left_right and base_right_left will be referred to as cross column
// hits, as in, successive single notes that cross columns and are exclusive of
// ohjumps
inline auto
is_cc_tap(const base_type& bt) -> bool
{
	return bt == base_left_right || bt == base_right_left;
}
