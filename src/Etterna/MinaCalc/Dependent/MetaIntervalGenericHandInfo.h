#pragma once
#include <bit>

enum generic_base_type
{
	gbase_hard_trill, // '12' in 6k (ring-middle or pinky-ring)
	gbase_easy_trill, // '23' in 6k (index-middle or opposite fingers of hand)
	gbase_single_single,	  // jack on any column alone
	gbase_single_chord_jack,  // single into a chord forming a jack
	gbase_single_chord_trill, // single into a chord forming a trill
	gbase_chord_chord_jack,	  // 2 chords forming a jack
	gbase_chord_chord_trill,  // 2 chords forming no jack
	gbase_chord_weird,		  // 2 chords with a jack and a trill
	num_gbase_types,
	gbase_type_init,
};

// return the mask of notes that would be hard to hit
// given the notes that were just hit
inline auto
hard_trill_mask(const unsigned& last_notes, const unsigned& hand_mask)
  -> unsigned
{
	const auto keycount_on_hand = std::popcount(hand_mask);
	const auto is_left_hand = (hand_mask & 0b1) > 0u;
	auto result_mask = 0b0;
	if (keycount_on_hand <= 2 || keycount_on_hand > 5) {
		// for 2 columns on this hand, it's always "easy"
		// and for more than 5... dont care
		return result_mask;
	}

	// 3 columns: (6k 212 or 565 is hard)
	// for 3 columns on left hand (binary is reversed)
	// 010 returns 001
	// 001 returns 010 ..
	// 3 columns on other hand:
	// 100 returns 010
	// 010 returns 100
	if (keycount_on_hand == 3u) {
		result_mask = is_left_hand ? 0b011 : 0b110;
	}

	// 4 columns: (8k 232 or 565 is hard)
	// 0100 returns 0010
	// 0010 returns 0100
	else if (keycount_on_hand == 4u) {
		result_mask = 0b0110;
	}

	// 5 columns: (10k 232 or 898 is hard)
	// 01000 returns 00100
	// 00100 returns 01000
	// opposite hand
	// 00010 returns 00100
	// 00100 returns 00010
	else {
		result_mask = is_left_hand ? 0b01100 : 0b00110;
	}

	if ((result_mask & last_notes) > 0u) {
		return result_mask & ~last_notes;
	} else {
		return 0b0;
	}
}

inline auto
determine_generic_base_pattern_type(const unsigned& notes_now,
									const unsigned& last_notes,
									const unsigned& hand_mask)
  -> generic_base_type
{
	if (last_notes == 0u || notes_now == 0u) {
		return gbase_type_init;
	}

	const bool now_single = is_only_1_bit(notes_now);
	const bool last_single = is_only_1_bit(last_notes);
	const bool has_jack = (notes_now & last_notes) > 0u;

	if (now_single && last_single) {
		if (has_jack) {
			return gbase_single_single;
		}
		if ((notes_now & hard_trill_mask(last_notes, hand_mask)) > 0u) {
			return gbase_hard_trill;
		}
		return gbase_easy_trill;
	}

	// only one single
	if (now_single || last_single) {
		if (has_jack) {
			return gbase_single_chord_jack;
		} else {
			return gbase_single_chord_trill;
		}
	}

	// at this point only consecutive chords are possible
	if (has_jack) {
		const bool has_trill = (notes_now ^ last_notes) > 0u;
		if (has_trill) {
			// a one handed anchored jack with a trill
			// [12][13]...
			return gbase_chord_weird;
		} else {
			return gbase_chord_chord_jack;
		}
	} else {
		return gbase_chord_chord_trill;
	}
}

inline auto
is_bracket(const unsigned& notes_row,
		   const unsigned& last_notes,
		   const unsigned& lastlast_notes) -> bool
{
	// firstsec and secthird rows must form some trill
	// firstthird rows must "jack" if put next to each other
	return (notes_row ^ last_notes) > 0 && (last_notes ^ lastlast_notes) > 0 &&
		   (notes_row & lastlast_notes) > 0;
}

inline auto
basetype_stops_bracket(const generic_base_type& bt)
{
	switch(bt) {
		case gbase_single_single:
		case gbase_chord_chord_jack:
		case gbase_single_chord_jack:
		case gbase_easy_trill:
		case gbase_chord_weird:
			return true;
		default:
			return false;
	}
}

/// tracks hand movements within an interval generically
/// in other words, tracks trills, jacks, chords on a hand
struct metaItvGenericHandInfo
{
	unsigned lastlast_row = 0u;
	unsigned last_row = 0u;
	generic_base_type last_type = gbase_type_init;

	int total_taps = 0;
	int chord_taps = 0;

	// total number of taps involved in a bracket
	// classically, a bracket is something like [13]2[13]2
	// a chord trilling into something that splits it
	int taps_bracketing = 0;
	bool bracketing = false;

	/// handle end of interval
	void interval_end()
	{
		_base_types.fill(0);
		taps_by_size.fill(0);
		total_taps = 0;
		chord_taps = 0;
		taps_bracketing = 0;
		bracketing = false;
		last_type = gbase_type_init;
	}

	/// zero everything out for end of hand loop so the trailing values from the
	/// left hand don't end up in the start of the right
	void zero()
	{
		_base_types.fill(0);
		taps_by_size.fill(0);
		total_taps = 0;
		chord_taps = 0;
		taps_bracketing = 0;
		bracketing = false;
		last_type = gbase_type_init;
	}

	void handle_row(const unsigned& new_row, const unsigned& hand_mask){
		auto pattern_type = determine_generic_base_pattern_type(
		  new_row, last_row, hand_mask);

		const auto taps_in_row = std::popcount(new_row);
		total_taps += taps_in_row;
		if (taps_in_row > 1) {
			chord_taps += taps_in_row;
		}

		if (pattern_type >= num_gbase_types) {
			lastlast_row = last_row;
			last_row = new_row;
			return;
		}

		// stop the bracketing
		if (basetype_stops_bracket(pattern_type)) {
			bracketing = false;
		}

		// we might be able to start bracketing
		else if	(!basetype_stops_bracket(last_type)) {
			if (is_bracket(new_row, last_row, lastlast_row)) {
				if (bracketing) {
					taps_bracketing += taps_in_row;
				} else {
					bracketing = true;
					taps_bracketing += taps_in_row + std::popcount(last_row) +
									   std::popcount(lastlast_row);
				}
			}
		}

		lastlast_row = last_row;
		last_row = new_row;
		last_type = pattern_type;
		_base_types[pattern_type]++;
		taps_by_size[taps_in_row - 1] += taps_in_row;
	}

	int possible_brackets() const
	{
		return _base_types[gbase_hard_trill] +
			   _base_types[gbase_single_chord_trill] +
			   _base_types[gbase_chord_chord_trill];
	}

	std::array<int, num_gbase_types> _base_types = { 0, 0, 0, 0, 0, 0, 0, 0 };
	std::array<int, num_tap_size> taps_by_size = { 0, 0, 0, 0, 0, 0, 0, 0,
												   0, 0, 0, 0, 0, 0, 0, 0 };

};

