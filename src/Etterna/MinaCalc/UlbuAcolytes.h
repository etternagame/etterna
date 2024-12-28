#pragma once
#include <vector>
#include <string>
#include <bit>

/* PRAISE ULBU FOR IT IS ITS GLORY THAT GIVES OUR LIVES MEANING */

constexpr float interval_span = 0.5F;

/// smoothing function to reduce spikes and holes in a given vector
/// biases vector beginning towards neutral
/// uses an average of 3 continuous elements
inline void
Smooth(std::vector<float>& input,
	   const float neutral,
	   const int end_interval)
{
	auto f2 = neutral;
	auto f3 = neutral;

	for (auto i = 0; i < end_interval; ++i) {
		const auto f1 = f2;
		f2 = f3;
		f3 = input.at(i);
		input.at(i) = (f1 + f2 + f3) / 3.F;
	}
}

/// smoothing function to reduce spikes and holes in a given vector.
/// biases vector beginning towards neutral
/// uses an average of 2 continuous elements
inline void
MSSmooth(std::vector<float>& input,
		 const float neutral,
		 const int end_interval)
{
	auto f2 = neutral;

	for (auto i = 0; i < end_interval; ++i) {
		const auto f1 = f2;
		f2 = input.at(i);
		input.at(i) = (f1 + f2) / 2.F;
	}
}

static const std::vector<CalcPatternMod> agnostic_mods = {
	Stream,	 JS,	   HS,		  CJ,	   CJDensity,	 HSDensity,
	FlamJam, TheThing, TheThing2, GChordStream,
};

static const std::vector<CalcPatternMod> dependent_mods = {
	OHJumpMod,	   Balance,
	Roll,		   RollJS,
	OHTrill,	   VOHTrill,
	Chaos,		   WideRangeBalance,
	WideRangeRoll, WideRangeJumptrill,
	WideRangeJJ,   WideRangeAnchor,
	RanMan,		   Minijack,
	CJOHJump, GStream, GBracketing,
};

struct PatternMods
{
	static void set_agnostic(const CalcPatternMod& pmod,
							 const float& val,
							 const int& pos,
							 Calc& calc)
	{
		calc.pmod_vals.at(left_hand).at(pmod).at(pos) = val;
	}

	static void set_dependent(const int& hand,
							  const CalcPatternMod& pmod,
							  const float& val,
							  const int& pos,
							  Calc& calc)
	{
		calc.pmod_vals.at(hand).at(pmod).at(pos) = val;
	}

	static void run_agnostic_smoothing_pass(const int& end_itv, Calc& calc)
	{
		for (const auto& pmod : agnostic_mods) {
			Smooth(calc.pmod_vals.at(left_hand).at(pmod), neutral, end_itv);
		}
	}

	static void run_dependent_smoothing_pass(const int& end_itv, Calc& calc)
	{
		for (const auto& pmod : dependent_mods) {
			for (auto& h : calc.pmod_vals) {
				Smooth(h.at(pmod), neutral, end_itv);
			}
		}
	}

	static void bruh_they_the_same(const int& end_itv, Calc& calc)
	{
		for (const auto& pmod : agnostic_mods) {
			for (auto i = 0; i < end_itv; i++) {
				calc.pmod_vals.at(right_hand).at(pmod).at(i) =
				  calc.pmod_vals.at(left_hand).at(pmod).at(i);
			}
		}
	}
};

/// converts time to interval index, if there's an offset to add or a rate to
/// scale by, it should be done prior
inline auto
time_to_itv_idx(const float& time) -> int
{
	// Offset time by half a millisecond to consistently break ties when a row
	// lies on an interval boundary. This is worst on files at bpms like 180
	// where the milliseconds between 16ths cannot be represented exactly by a
	// float but in exact arithemetic regularly align with the interval
	// boundaries. Offsetting here makes the calc more robust to bpm
	// fluctuations, mines at the start of the file, etc, which could move notes
	// into different intervals
	return static_cast<int>((time + 0.0005f) / interval_span);
}

inline auto
itv_idx_to_time(const int& idx) -> float
{
	return static_cast<float>(idx) * interval_span;
}

/// checks to see if the noteinfo will fit in our static arrays, if it won't it's
/// some garbage joke file and we can throw it out, setting values to 0
inline auto
fast_walk_and_check_for_skip(const std::vector<NoteInfo>& ni,
							 const float& rate,
							 Calc& calc,
							 const float& offset = 0.F) -> bool
{
	// an inf rowtime means 0 bpm or some other odd gimmick that may break things
	// skip this file
	// nan/inf can occur before the end of the file
	// but the way these are generated, the last should be the largest
	// therefore if any are inf, this is inf
	if (std::isinf(ni.back().rowTime) || std::isnan(ni.back().rowTime))
		return true;
	
	/* add 1 to convert index to size, we're just using this to guess due to
	 * potential float precision differences, the actual numitv will be set at
	 * the end */
	calc.numitv = time_to_itv_idx(ni.back().rowTime / rate) + 1;

	// are there more intervals than our emplaced max
	if (calc.numitv >= static_cast<int>(calc.itv_size.size())) {
		// hard cap for memory considerations
		if (calc.numitv >= max_intervals)
			return true;
		// accesses can happen way at the end so give it some breathing room
		calc.resize_interval_dependent_vectors(calc.numitv + 2);
	}

	// for various reasons we actually have to do this, scan the file and make
	// sure each successive row time is greater than the last
	for (auto i = 1; i < static_cast<int>(ni.size()); ++i) {
		if (ni.at(i - 1).rowTime >= ni.at(i).rowTime) {
			return true;
		}
	}

	// set up extra keycount information
	const auto max_keycount_notes = keycount_to_bin(calc.keycount);
	auto all_columns_without_middle = max_keycount_notes;
	if (ignore_middle_column) {
		all_columns_without_middle = mask_to_remove_middle_column(calc.keycount);
	}
	auto left_hand_mask = left_mask(calc.keycount) & all_columns_without_middle;
	auto right_hand_mask = right_mask(calc.keycount) & all_columns_without_middle;

	// left, right
	calc.hand_col_masks = { left_hand_mask, right_hand_mask };
	// all columns from left to rightmost
	calc.col_masks.clear();
	calc.col_masks.reserve(calc.keycount);
	for (unsigned i = 0; i < calc.keycount; i++) {
		calc.col_masks.push_back(1 << i);
	}

	/* now we can attempt to construct notinfo that includes column count and
	 * rate adjusted row time, both of which are derived data that both pmod
	 * loops require */
	auto itv = 0;
	auto last_itv = 0;
	auto row_counter = 0;
	auto scaled_time = 0.F;
	for (auto i : ni) {

		// it's at least 25 nps per finger, throw it out
		if (row_counter >= max_rows_for_single_interval) {
			return true;
		}

		const auto& ri = i;

		// either not a 4k file or malformed
		if (ri.notes < 0 || ri.notes > max_keycount_notes) {
			return true;
		}

		// 90000 bpm flams may produce 0s due to float precision, we can ignore
		// this for now, there should be no /0 errors due to it
		/*if (i > 0) {
			assert(zoop > scaled_time);
		}*/

		scaled_time = (i.rowTime + offset) / rate;

		// set current interval and current scaled time
		itv = time_to_itv_idx(scaled_time);

		// new interval, reset row counter and set new last interval
		if (itv > last_itv) {

			// we're using static arrays so if we skip over some empty intervals
			// we have to go back and set their row counts to 0
			if (itv - last_itv > 1) {
				for (auto j = last_itv + 1; j < itv; ++j) {
					calc.itv_size.at(j) = 0;
				}
			}

			calc.itv_size.at(last_itv) = row_counter;

			last_itv = itv;
			row_counter = 0;
		}

		auto& nri = calc.adj_ni.at(itv).at(row_counter);

		nri.row_notes = ri.notes;
		nri.row_count = column_count(ri.notes);
		nri.row_time = scaled_time;

		// how many columns have a note on them per hand
		nri.hand_counts[left_hand] = std::popcount(ri.notes & left_hand_mask);
		nri.hand_counts[right_hand] = std::popcount(ri.notes & right_hand_mask);

		// make sure row_count adds up...
		// this validates that the mask is correct
		assert(nri.hand_counts[left_hand] + nri.hand_counts[right_hand] ==
			   nri.row_count);

		++row_counter;
	}

	// take care to set the proper values for the last row, the set logic block
	// won't be hit on it
	if (itv - last_itv > 1) {
		for (auto j = last_itv + 1; j < itv; ++j) {
			calc.itv_size.at(j) = 0;
		}
	}

	calc.itv_size.at(itv) = row_counter;

	// make sure we only set up to the interval/row we actually use
	calc.numitv = itv + 1;
	return false;
}
