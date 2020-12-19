#pragma once
#include <vector>
#include <string>

/* PRAISE ULBU FOR IT IS ITS GLORY THAT GIVES OUR LIVES MEANING */

/* since we are no longer using the normalizer system we need to lower the base
 * difficulty for each skillset and then detect pattern types to push down OR
 * up, rather than just down and normalizing to a differential since chorded
 * patterns have lower enps than streams, streams default to 1 and chordstreams
 * start lower, stam is a special case and may use normalizers again */
static const std::array<float, NUM_Skillset> basescalers = {
	0.F, 0.93F, 0.885F, 0.84F, 0.925F, 0.8833277F, 0.8F, 0.83F
};

static const std::string calc_params_xml = "Save/calc params.xml";
static const std::array<unsigned, num_hands> hand_col_ids = { 3, 12 };
constexpr float interval_span = 0.5F;

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
	Stream, JS, HS, CJ, CJDensity, FlamJam, TheThing, TheThing2,
};

static const std::vector<CalcPatternMod> dependent_mods = {
	OHJumpMod,		  Balance,		 Roll,
	OHTrill,		  VOHTrill,		 Chaos,
	WideRangeBalance, WideRangeRoll, WideRangeJumptrill,
	WideRangeAnchor,  RanMan,		 CJOHJump
};

struct PatternMods
{
	static void set_agnostic(const CalcPatternMod& pmod,
							 const float& val,
							 const int& pos,
							 Calc& calc)
	{
		calc.doot.at(left_hand).at(pmod).at(pos) = val;
	}

	static void set_dependent(const int& hand,
							  const CalcPatternMod& pmod,
							  const float& val,
							  const int& pos,
							  Calc& calc)
	{
		calc.doot.at(hand).at(pmod).at(pos) = val;
	}

	static void run_agnostic_smoothing_pass(const int& end_itv, Calc& calc)
	{
		for (const auto& pmod : agnostic_mods) {
			Smooth(calc.doot.at(left_hand).at(pmod), neutral, end_itv);
		}
	}

	static void run_dependent_smoothing_pass(const int& end_itv, Calc& calc)
	{
		for (const auto& pmod : dependent_mods) {
			for (auto& h : calc.doot) {
				Smooth(h.at(pmod), neutral, end_itv);
			}
		}
	}

	static void bruh_they_the_same(const int& end_itv, Calc& calc)
	{
		for (const auto& pmod : agnostic_mods) {
			for (auto i = 0; i < end_itv; i++) {
				calc.doot.at(right_hand).at(pmod).at(i) =
				  calc.doot.at(left_hand).at(pmod).at(i);
			}
		}
	}
};

// converts time to interval index, if there's an offset to add or a rate to
// scale by, it should be done prior
inline auto
time_to_itv_idx(const float& time) -> int
{
	return static_cast<int>(time / interval_span);
}

// checks to see if the noteinfo will fit in our static arrays, if it won't it's
// some garbage joke file and we can throw it out, setting values to 0
inline auto
fast_walk_and_check_for_skip(const std::vector<NoteInfo>& ni,
							 const float& rate,
							 Calc& calc,
							 const float& offset = 0.F) -> bool
{
	/* add 1 to convert index to size, we're just using this to guess due to
	 * potential float precision differences, the actual numitv will be set at
	 * the end */
	calc.numitv = time_to_itv_idx(ni.back().rowTime / rate) + 1;

	// are there more intervals than our emplaced max
	if (calc.numitv >= calc.itv_size.size()) {
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
		if (ri.notes < 0 || ri.notes > 0b1111) {
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

		auto left = 0;
		auto right = 0;

		if ((ri.notes & 1U) != 0U) {
			++left;
		}
		if ((ri.notes & 2U) != 0U) {
			++left;
		}
		if ((ri.notes & 4U) != 0U) {
			++right;
		}
		if ((ri.notes & 8U) != 0U) {
			++right;
		}

		assert(left + right == nri.row_count);

		nri.hand_counts[left_hand] = left;
		nri.hand_counts[right_hand] = right;

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
