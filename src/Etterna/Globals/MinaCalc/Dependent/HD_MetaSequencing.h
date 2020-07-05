#pragma once
#include "HD_BasicSequencing.h"

/* meta patterns are formed by chains of base patterns, we could pick up
 * triplets (left_right, right_left), or triple jacks (single_single,
 * single_single), but these aren't particularly useful for our use case, meta
 * patterns types are largely used in conjunction with timing checks to pick up
 * mashable sequences of notes and depress them so they do not inflate certain
 * files, however there is potential to try to pick up broader sequences of
 * tricky patterns and upscale them, it's just not done at the moment */
enum meta_type
{
	meta_cccccc,
	meta_ccacc,
	meta_acca,
	meta_ccsjjscc,
	meta_ccsjjscc_inverted,
	meta_enigma,
	meta_meta_enigma,
	meta_unknowable_enigma,
	num_meta_types,
	meta_type_init,
};

// 1212, 2121, etc
// cccccc is cross column, cross column, cross column more colloquially known as
// either an oht, or a roll, depending on the timing, since the definition is
// timing dependent we will simply use the pattern configuration to refer to it,
// until timing checks are run only called if both now and last_last are
// base_left_right || base_right_lef,  then, if it's not cccccc, it's ccacc by
// definition
inline auto
detecc_cccccc(const base_type& now, const base_type& last_last) -> bool
{
	// wow it was actually cabbage brain LUL
	return now == last_last;
}

inline auto
detecc_acca(const base_type& a, const base_type& b, const base_type& c) -> bool
{
	// 1122, 2211, etc
	return a == base_single_single && is_cc_tap(b) && c == base_single_single;
}

// WHOMST'D'VE
// 12[12]12, we'll check now for cc before entering this, so we can then
// determine whether this is an inverted ccsjjscc or not (12[12]21)
inline auto
detecc_sjjscc(const base_type& last,
			  const base_type& last_last,
			  const base_type& last_last_last) -> bool
{
	// check last_last_last first, if it's not cc, throw it out
	if (!is_cc_tap(last_last_last)) {
		{
			{
				return false;
			}
		}
	}

	// last is exiting the jump, last_last is entering it
	// note: we don't care about the single/jump jump/single column order
	// because it can always be inferred from existing data
	return last == base_jump_single && last_last == base_single_jump;
}

inline auto
determine_meta_type(const base_type& now,
					const base_type& last,
					const base_type& last_last,
					const base_type& last_last_last,
					const meta_type& last_mt) -> meta_type
{
	// this is either cccccc or ccacc
	if (is_cc_tap(now) && is_cc_tap(last_last)) {

		if (detecc_cccccc(now, last_last)) {
			// 1212, 2121, etc
			return meta_cccccc;
		}
		{
			// 1221, 2112, etc
			return meta_ccacc;
		}
	}

	/* 1122, 2211, these are generally tricky and we wouldn't be interseted in
	 * using them for downscaling in most contexts, however there are
	 * jumptrillable patterns that exist for which acca is an axiomatic
	 * transition, namely, chains of ccacc, or, ccaccaccaccaccacc, or,
	 * 1221221221221221221, scanning down rows will produce alternating meta
	 * types of ccacc and acca, so we can use this to do very robust detection,
	 * particularly since any other transition type in sequences of these
	 * patterns will actually make them significantly harder to manipulate */
	if (detecc_acca(now, last, last_last)) {
		return meta_acca;
	}

	// we need to be on a cc to have ccsjjscc
	if (is_cc_tap(now)) {

		// this is a 5 row pattern, so we have to go deep
		if (detecc_sjjscc(last, last_last, last_last_last)) {

			if (now == last_last_last) {
				// 12 [12] 12
				return meta_ccsjjscc;
			}
			{
				// 12 [12] 21
				return meta_ccsjjscc_inverted;
			}
		}
	}

	// past the point of our current largest meta pattern definitions, so if we
	// see this, we can stop waiting for something like ccsjjscc
	if (last_mt == meta_enigma) {
		return meta_meta_enigma;
	}

	// there are probably entire packs where we won't even see one of these
	if (last_mt == meta_meta_enigma) {
		return meta_unknowable_enigma;
	}

	// meta_enigma is commonplace for transitions between two meta types, so we
	// often use it to wait and see what comes next
	return meta_enigma;
}
