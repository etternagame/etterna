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

	// 3 columns: (6k 212 or 454 is hard)
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

/// tracks hand movements within an interval generically
/// in other words, tracks trills, jacks, chords on a hand
struct metaItvGenericHandInfo
{
	unsigned last_row = 0u;

	/// handle end of interval
	void interval_end()
	{
		_base_types.fill(0);
	}

	/// zero everything out for end of hand loop so the trailing values from the
	/// left hand don't end up in the start of the right
	void zero()
	{
		_base_types.fill(0);
	}

	void handle_row(const unsigned& new_row, const unsigned& hand_mask){
		auto pattern_type = determine_generic_base_pattern_type(
		  new_row, last_row, hand_mask);
		last_row = new_row;

		if (pattern_type >= num_gbase_types) {
			return;
		}

		_base_types[pattern_type]++;
	}

	std::array<int, num_gbase_types> _base_types = { 0, 0, 0, 0, 0, 0, 0, 0 };
};

