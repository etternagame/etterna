#pragma once
/* hand agnostic pattern sequencing is a little less developed than its
 * counterpart and just contains some basic bitwise helpers for the moment */

// bitwise operations on noteinfo.notes, they must be unsigned ints, and
// shouldn't be called on enums or row counts or anything like that
inline auto
is_single_tap(const unsigned& a) -> bool
{
	return (a & (a - 1)) == 0;
}

// between two successive rows usually... but i suppose this could be called
// outside of that limitation
inline auto
is_jack_at_col(const unsigned& id,
			   const unsigned& row_notes,
			   const unsigned& last_row_notes) -> bool
{
	return ((id & row_notes) != 0U) && ((id & last_row_notes) != 0U);
}

// doesn't check for jacks
inline auto
is_alternating_chord_single(const unsigned& a, const unsigned& b) -> bool
{
	return (a > 1 && b == 1) || (a == 1 && b > 1);
}

// ok lets stop being bad, find 1[n]1 or [n]1[n] with no jacks between first and
// second and second and third elements
inline auto
is_alternating_chord_stream(const unsigned& a,
							const unsigned& b,
							const unsigned& c) -> bool
{
	if (is_single_tap(a)) {
		if (is_single_tap(b)) {
			// single single, don't care, bail
			return false;
		}
		if (!is_single_tap(c)) { // single, chord, chord, bail
			return false;
		}
	} else {
		if (!is_single_tap(b)) {
			// chord chord, don't care, bail
			return false;
		}
		if (is_single_tap(c)) { // chord, single, single, bail
			return false;
		}
	}
	// we have either 1[n]1 or [n]1[n], check for any jacks
	return static_cast<int>(((a & b) != 0U) && ((b & c) != 0U)) == 0;
}
