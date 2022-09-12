#pragma once

#include "../../Dependent/HD_BasicSequencing.h"
#include "../../CalcWindow.h"
#include <cassert>

struct twohandtrill
{
	// trill scenarios
	enum trillnario
	{
		the_lack_thereof, // empty state
		need_opposite,
	};

	// for the trill to continue, the next note must be on the opposite hand
	hands current_hand = left_hand;
	// for the trill to continue, the left hand notes must be on this column
	// allow for ohj
	col_type left_hand_state = col_init;
	// for the trill to continue, the right hand notes must be on this column
	// allow for ohj
	col_type right_hand_state = col_init;

	trillnario current_scenario = the_lack_thereof;

	/// a trill length 1 is 2 notes long and has "completed"
	int cur_length = 0;
	int jump_count = 0;
	
	CalcMovingWindow<float> trill_ms;
	int trills_in_interval = 0;
	int total_taps = 0;

	// trill ms cv greater than this is rejected
	float cv_threshold = 0.5F;

	unsigned last_notes = 0b0000;
	unsigned last_last_notes = 0b0000;

	// true if is a hand or two hand jump
	bool two_hand_jump(const unsigned& notes) {
		return ((notes & 0b1100) != 0U) && ((notes & 0b0011) != 0U);
	}

	// drive the sequencer
	void process(const float& ms_now, const unsigned& notes) {

		hands notes_hand;
		col_type notes_coltype;
		const unsigned prev_notes = last_notes;
		const unsigned prev_prev_notes = last_last_notes;
		last_last_notes = last_notes;
		last_notes = notes;

		if (two_hand_jump(notes)) {
			if (two_hand_jump(prev_notes)) {
				// absolutely not
				dead_trill();
				return;
			}
			if (two_hand_jump(prev_prev_notes)) {
				// basically not a two hand trill
				dead_trill();
				return;
			}

			if ((notes & 0b1111) == 0b1111) {
				// quad
				dead_trill();
				return;
			} else if ((notes & 0b1100) == 0b1100) {
				// jump on left
				//notes_coltype = determine_col_type(notes, 0b0011);
				notes_hand = left_hand;
				notes_coltype = col_ohjump;
			} else if ((notes & 0b0011) == 0b0011) {
				// jump on right
				//notes_coltype = determine_col_type(notes, 0b1100);
				notes_hand = right_hand;
				notes_coltype = col_ohjump;
			} else {
				// actual two hand jump
				if (((notes & prev_prev_notes) != 0U) && ((notes & prev_notes) == 0U)) {
					// a jack separated by a note
					// current row doesnt form a jack with previous row
					// (23[24])
					// find the jack column and hand
					const unsigned ppn = notes & prev_prev_notes;
					if ((ppn & 0b1100) != 0U) {
						notes_hand = left_hand;
						notes_coltype = determine_col_type(notes, 0b1100);
					} else if ((ppn & 0b0011) != 0U) {
						notes_hand = right_hand;
						notes_coltype = determine_col_type(notes, 0b0011);
					} else {
						// preposterous
						dead_trill();
						return;
					}
				} else {
					// what (232[24])
					dead_trill();
					return;
				}
			}

		} else if ((notes & 0b1100) != 0) {
			notes_hand = left_hand;
			notes_coltype = determine_col_type(notes, 0b1100);
		} else if ((notes & 0b0011) != 0) {
			notes_hand = right_hand;
			notes_coltype = determine_col_type(notes, 0b0011);
		} else {
			// there is a lack of taps.
			// this typically doesnt happen
			reset();
			return;
		}

		switch (current_scenario) {
			case the_lack_thereof: {
				// begin a trill anywhere
				current_hand = notes_hand;
				set_hand_states(notes_coltype);
				current_scenario = need_opposite;
				break;
			}
			case need_opposite: {
				if (current_hand == notes_hand) {
					// trill broken
					dead_trill();
				} else {
					if (notes_coltype == col_ohjump) {
						jump_count++;
					}
					if (current_hand == left_hand) {
						if (right_hand_state == col_init ||
							right_hand_state == notes_coltype) {
							right_hand_state = notes_coltype;
							calc_trill(ms_now);
							cur_length++;
							total_taps++;
						} else {
							// trill broken
							dead_trill();
							right_hand_state = notes_coltype;
							current_scenario = need_opposite;
						}
					} else {
						if (left_hand_state == col_init ||
							left_hand_state == notes_coltype) {
							left_hand_state = notes_coltype;
							calc_trill(ms_now);
							cur_length++;
							total_taps++;
						} else {
							// trill broken
							dead_trill();
							left_hand_state = notes_coltype;
							current_scenario = need_opposite;
						}
					}
				}
				break;
			}
			default:
				assert(0);
		}
	}

	void set_hand_states(col_type coltype)
	{
		if (current_hand == left_hand) {
			left_hand_state = coltype;
		} else {
			right_hand_state = coltype;
		}
	}

	bool calc_trill(const float& ms_now)
	{
		const int window = std::min(cur_length, max_moving_window_size);
		trill_ms(ms_now);
		// float cv = trill_ms.get_cv_of_window(window);
		// for high cv, this may be a flam
		return true;
		//return cv < cv_threshold;
	}

	void reset() {
		trills_in_interval = 0;
		total_taps = 0;
		cur_length = 0;
		trill_ms.zero();
		current_scenario = the_lack_thereof;
		left_hand_state = col_init;
		right_hand_state = col_init;
		last_notes = 0b0000;
		last_last_notes = 0b0000;
	}

	void dead_trill() {
		cur_length = 0;
		trill_ms.zero();
		current_scenario = the_lack_thereof;
		left_hand_state = col_init;
		right_hand_state = col_init;
		last_notes = 0b0000;
		last_last_notes = 0b0000;
	}
};


/// Two Hand Trill Sequencing
struct THT_Sequencing
{
	twohandtrill trill;

	const int _tap_size = 1;
	const int _jump_size = 2;

	float trill_buffer = 0.F;
	float trill_scaler = 1.F;
	float jump_buffer = 0.F;
	float jump_scaler = 1.F;
	float jump_weight = 0.5F;

	float min_val = 0.F;
	float max_val = 1.5F;

	void set_params(const float& cv,
					const float& tbuffer,
					const float& tscaler,
					const float& jbuffer,
					const float& jscaler,
					const float& jweight,
					const float& min,
					const float& max)
	{
		trill.cv_threshold = cv;
		trill_buffer = tbuffer;
		trill_scaler = tscaler;
		jump_buffer = jbuffer;
		jump_scaler = jscaler;
		jump_weight = jweight;
		min_val = min;
		max_val = max;
	}

	// advance sequencing
	void operator()(const float& ms_now, const unsigned& notes)
	{
		trill.process(ms_now, notes);
	}

	void reset() {
		trill.reset();
	}

	/// numerical output (kind of like a proportion. 1 is "all trills")
	float get(const metaItvInfo& mitvi) {
		const float trill_taps = static_cast<float>(trill.total_taps);
		const float trill_jumps = static_cast<float>(trill.jump_count);
		const auto& itvi = mitvi._itvi;
		const float taps = static_cast<float>(itvi.total_taps);

		if (taps == 0.F) {
			return 0.F;
		}

		const float jumps = static_cast<float>(itvi.taps_by_size.at(_jump_size));

		float trill_proportion = (trill_taps + trill_buffer) /
								 std::max(taps - trill_buffer, 1.F) *
								 trill_scaler;

		float jump_proportion = (jumps - trill_jumps + jump_buffer) /
								std::max(taps - jump_buffer, 1.F) * jump_scaler;

		// jump_weight = 0 will make it all jump_proportion
		float prop = weighted_average(trill_proportion,
									  jump_proportion,
									  std::clamp(1 - jump_weight, 0.F, 1.F),
									  1.F);

		return std::clamp(prop, min_val, max_val);
	}
};
