#pragma once
#include "SequencingHelpers.h"
#include <array>
#include <unordered_map>

/* MS difficulty bases are going to be sequence constructed row by row here, the
 * nps base may be moved here later but not right now. we'll use statically
 * allocated arrays to built difficulty for each interval, and the output will
 * be placed into statically allocated arrays for base difficulties. (in
 * threaded calc object now, though) */

/// required percentage of average notes to pass
constexpr float min_threshold = 0.65F;
static const float downscale_logbase = std::log(6.2F);

constexpr float scaler_for_ms_base = 1.175F;
// i do not know a proper name for these
constexpr float ms_base_finger_weighter_2 = 9.F;
constexpr float ms_base_finger_weighter = 5.5F;

static auto
CalcMSEstimate(std::vector<float>& input, const int& burp) -> float
{
	// how many ms values we use from here, if there are fewer than this
	// number we'll mock up some values to water down intervals with a
	// single extremely fast minijack, if there are more, we will truncate
	unsigned int num_used = burp;

	if (input.empty()) {
		return 0.F;
	}

	// avoiding this for now because of smoothing
	// single ms value, dunno if we want to do this? technically the tail
	// end of an insanely hard burst that gets lopped off at the last note
	// is still hard? if (input.size() < 2) return 1.f;

	// sort before truncating/filling
	std::sort(input.begin(), input.end());

	// truncate if we have more values than what we care to sample, we're
	// looking for a good estimate of the hardest part of this interval
	// if above 1 and below used_ms_vals, fill up the stuff with dummies
	// my god i was literally an idiot for doing what i was doing before
	static const float ms_dummy = 360.F;

	// mostly try to push down stuff like jumpjacks, not necessarily to push
	// up "complex" stuff (this will push up intervals with few fast ms
	// values kinda hard but it shouldn't matter as their base ms diff
	// should be extremely low
	float cv_yo = cv_trunc_fill(input, burp, ms_dummy) + 0.5F;
	cv_yo = std::clamp(cv_yo, 0.5F, 1.25F);

	// basically doing a jank average, bigger m = lower difficulty
	float m = sum_trunc_fill(input, burp, ms_dummy);

	// add 1 to num_used because some meme about sampling
	// same thing as jack stuff, convert to bpm and then nps
	float bpm_est = ms_to_bpm(m / (num_used + 1));
	float nps_est = bpm_est / 15.F;
	float fdiff = nps_est * cv_yo;
	return fdiff;
}

struct nps
{
	/// determine NPSBase, itv_points, CJBase for this hand
	static void actual_cancer(Calc& calc, const int& hand)
	{
		// finger row times
		auto last_left_row_time = s_init;
		auto last_right_row_time = s_init;

		auto scaly_ms_estimate = [](std::vector<float>& input,
								const float& scaler) {
			float o = CalcMSEstimate(input, 3);
			if (input.size() > 3) {
				o = std::max(o, CalcMSEstimate(input, 4) * scaler);
			}
			if (input.size() > 4) {
				o = std::max(o, CalcMSEstimate(input, 5) * scaler * scaler);
			}
			return o;
		};

		for (auto itv = 0; itv < calc.numitv; ++itv) {
			auto notes = 0;

			// times between rows for this interval for each finger
			std::vector<float> left_finger_ms{};
			std::vector<float> right_finger_ms{};

			for (auto row = 0; row < calc.itv_size.at(itv); ++row) {
				const auto& cur = calc.adj_ni.at(itv).at(row);
				notes += cur.hand_counts.at(hand);

				const auto& crt = cur.row_time;
				switch (determine_col_type(cur.row_notes, hand_col_ids[hand])) {
					case col_left:
						if (last_left_row_time != s_init) {
							const auto left_ms =
							  ms_from(crt, last_left_row_time);
							left_finger_ms.push_back(left_ms);
						}
						last_left_row_time = crt;
						break;
					case col_right:
						if (last_right_row_time != s_init) {
							const auto right_ms =
							  ms_from(crt, last_right_row_time);
							right_finger_ms.push_back(right_ms);
						}
						last_right_row_time = crt;
						break;
					case col_ohjump:
						if (last_right_row_time != s_init) {
							const auto right_ms =
							  ms_from(crt, last_right_row_time);
							right_finger_ms.push_back(right_ms);
						}
						if (last_left_row_time != s_init) {
							const auto left_ms =
							  ms_from(crt, last_left_row_time);
							left_finger_ms.push_back(left_ms);
						}
						last_left_row_time = crt;
						last_right_row_time = crt;
						break;
					case col_empty:
					case col_init:
					default:
						// none
						break;
				}
			}

			// nps for this interval
			const auto nps = static_cast<float>(notes) * finalscaler * 1.6F;
			calc.init_base_diff_vals.at(hand).at(NPSBase).at(itv) = nps;

			// ms base for this interval
			const auto left =
			  scaly_ms_estimate(left_finger_ms, scaler_for_ms_base);
			const auto right =
			  scaly_ms_estimate(right_finger_ms, scaler_for_ms_base);
			float msdiff = 0.F;
			if (left > right) {
				msdiff = weighted_average(left,
										  right,
										  ms_base_finger_weighter,
										  ms_base_finger_weighter_2);
			} else {
				msdiff = weighted_average(right,
										  left,
										  ms_base_finger_weighter,
										  ms_base_finger_weighter_2);
			}
			const auto msbase = finalscaler * msdiff;
			calc.init_base_diff_vals.at(hand).at(MSBase).at(itv) = msbase;
			/////

			// set points for this interval
			calc.itv_points.at(hand).at(itv) = notes * 2;
		}
	}

	/// determine grindscaler using smoothed npsbase
	static void grindscale(Calc& calc) {
		auto populated_intervals = 0;
		auto avg_notes = 0.F;
		for (auto itv = 0; itv < calc.numitv; ++itv) {
			auto notes = 0.F;

			for (auto& hand : both_hands) {
				notes += calc.init_base_diff_vals.at(hand).at(NPSBase).at(itv);
			}

			if (notes > 0) {
				avg_notes += notes;
				populated_intervals++;
			}
		}

		if (populated_intervals > 0) {
			// const auto empty_intervals = static_cast<float>(calc.numitv - populated_intervals);
			avg_notes /= populated_intervals;

			auto failed_intervals = 0;

			for (auto itv = 0; itv < calc.numitv; itv++) {
				auto notes = 0.F;
				for (auto& hand : both_hands)
					notes +=
					  calc.init_base_diff_vals.at(hand).at(NPSBase).at(itv);

				// count only intervals with notes
				if (notes > 0.F && notes < avg_notes * min_threshold)
					failed_intervals++;
			}

			// base grindscaler on how many intervals are passing
			// if the entire file is just single taps or empty:
			//   ask yourself... is it really worth playing?
			const auto file_length =
			  itv_idx_to_time(populated_intervals - failed_intervals);
			// log minimum but if you move this you need to move the log base
			const auto ping = .3F;
			const auto timescaler =
			  (ping * (std::log(file_length + 1) / downscale_logbase)) + ping;

			calc.grindscaler = std::clamp(timescaler, .1F, 1.F);
		} else {
			calc.grindscaler = .1F;
		}
	}
};

struct ceejay
{
	const std::string name = "CJ_Static";

#pragma region params

	float static_ms_weight = .65F;
	float min_ms = 75.F;

	float base_tap_scaler = 3.F;
	float huge_anchor_scaler = 1.15F;
	float small_anchor_scaler = 1.15F;
	float ccj_scaler = 1.25F;
	float cct_scaler = 1.5F;
	float ccn_scaler = 1.15F;
	float ccb_scaler = 1.25F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "static_ms_weight", &static_ms_weight },
		{ "min_ms", &min_ms },
		{ "base_tap_scaler", &base_tap_scaler },
		{ "huge_anchor_scaler", &huge_anchor_scaler },
		{ "small_anchor_scaler", &small_anchor_scaler },
		{ "chord-chord-jack_scaler", &ccj_scaler },
		{ "chord-chord-tap_scaler", &cct_scaler },
		{ "chord-chord-no-anchors_scaler", &ccn_scaler },
		{ "chordjacks_beginning_scaler", &ccb_scaler },
	};

#pragma endregion params and param map


	void update_flags(const unsigned& row_notes, const int& row_count)
	{
		is_cj = last_row_count > 1 && row_count > 1;
		was_cj = last_row_count > 1 && last_last_row_count > 1;

		is_scj = (row_count == 1 && last_row_count > 1) &&
				 ((row_notes & last_row_notes) != 0u);

		is_at_least_3_note_anch =
		  ((row_notes & last_row_notes) & last_last_row_notes) != 0u;

		last_last_row_count = last_row_count;
		last_row_count = row_count;

		last_last_row_notes = last_row_notes;
		last_row_notes = row_notes;

		last_was_3_note_anch = is_at_least_3_note_anch;
	}

	void advance_base(const float& any_ms, Calc& calc)
	{
		if (row_counter >= max_rows_for_single_interval) {
			return;
		}

		// pushing back ms values, so multiply to nerf
		float pewpew = base_tap_scaler;

		if (is_at_least_3_note_anch && last_was_3_note_anch) {
			// biggy boy anchors and beyond
			pewpew = huge_anchor_scaler;
		} else if (is_at_least_3_note_anch) {
			// big boy anchors
			pewpew = small_anchor_scaler;
		} else {
			// single note
			if (!is_cj) {
				if (is_scj) {
					// was cj a little bit ago..
					if (was_cj) {
						// single note jack with 2 chords behind it
						// ccj (chord chord jack)
						pewpew = ccj_scaler;
					} else {
						// single note, not a jack, 2 chords behind
						// cct (chord chord tap)
						pewpew = cct_scaler;
					}
				}
			} else {
				// actual cj
				if (was_cj) {
					// cj now and was cj before, but not necessarily
					// with strong anchors
					// ccn (chord chord no anchor)
					pewpew = ccn_scaler;
				} else {
					// cj now but wasn't even cj before
					// ccb (chordjack beginning)
					pewpew = ccb_scaler;
				}
			}
		}

		// single note streams / regular jacks should retain the base
		// multiplier

		const auto ms = std::max(min_ms, any_ms * pewpew);
		calc.cj_static.at(row_counter) = ms;
		++row_counter;
	}

	// final output difficulty for this interval
	auto get_itv_diff(Calc& calc) const -> float
	{
		if (row_counter == 0) {
			return 0.F;
		}

		// ms vals to counts
		std::unordered_map<int, int> mode;
		std::vector<float> static_ms;
		for (int i = 0; i < row_counter; ++i) {
			const auto v = static_cast<int>(calc.cj_static.at(i));
			static_ms.push_back(calc.cj_static.at(i));
			mode[v]++; // this is safe
		}
		auto modev = 0;
		auto modefreq = 0;
		for (auto it = mode.begin(); it != mode.end(); it++) {
			if (it->second > modefreq) {
				modev = it->first;
				modefreq = it->second;
			}
		}
		for (auto i = 0; i < static_ms.size(); i++) {
			// weight = 0 means all values become modev
			static_ms.at(i) =
			  weighted_average(static_ms.at(i), modev, static_ms_weight, 1.F);
		}

		const auto ms_total = sum(static_ms);
		const auto ms_mean = ms_total / static_cast<float>(row_counter);
		return ms_to_scaled_nps(ms_mean);
	}

	void interval_end() { row_counter = 0; }
	void full_reset()
	{
		is_cj = false;
		was_cj = false;
		is_scj = false;
		is_at_least_3_note_anch = false;
		last_was_3_note_anch = false;

		last_row_count = 0;
		last_last_row_count = 0;

		last_row_notes = 0U;
		last_last_row_notes = 0U;
	}

  private:
	int row_counter = 0;

	bool is_cj = false;
	bool was_cj = false;
	bool is_scj = false;
	bool is_at_least_3_note_anch = false;
	bool last_was_3_note_anch = false;

	int last_row_count = 0;
	int last_last_row_count = 0;

	unsigned last_row_notes = 0U;
	unsigned last_last_row_notes = 0U;
};

/// if this looks ridiculous, that's because it is
struct techyo
{

	const std::string name = "TC_Static";

#pragma region params

	float tc_base_weight = 4.F;
	float nps_base_weight = 9.F;
	float rm_base_weight = 1.F;

	float balance_comp_window = 36.F;
	float chaos_comp_window = 4.F;
	float tc_static_base_window = 2.F;

	// determines steepness of non-1/2 balance ratios
	float balance_power = 2.F;
	float min_balance_ratio = 0.2F;
	float balance_ratio_scaler = 1.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "tc_base_weight", &tc_base_weight },
		{ "nps_base_weight", &nps_base_weight },
		{ "rm_base_weight", &rm_base_weight },

		{ "balance_comp_window", &balance_comp_window },
		{ "chaos_comp_window", &chaos_comp_window },
		{ "tc_static_base_window", &tc_static_base_window },
		{ "balance_power", &balance_power },
		{ "min_balance_ratio", &min_balance_ratio },
		{ "balance_ratio_scaler", &balance_ratio_scaler },
	};

#pragma endregion params and param map

	// moving window of the last 3 {column, time} that showed up
	// we transform col_type ohj into 2 entries with the same time on insertion
	static const unsigned trill_window = 3;
	std::array<std::pair<col_type, float>, techyo::trill_window> mw_dt{};

	void advance_base(const SequencerGeneral& seq,
					  const col_type& ct,
					  Calc& calc, const int& hand, const float& row_time)
	{
		if (row_counter >= max_rows_for_single_interval) {
			return;
		}

		//process_mw_dt(ct, seq.get_any_ms_now());
		//advance_trill_base(calc);
		//increment_column_counters(ct);
		//auto balance_comp = std::max(calc_balance_comp() * balance_ratio_scaler, min_balance_ratio);
		auto chaos_comp = calc_chaos_comp(seq, ct, calc, hand, row_time);
		//insert(balance_ratios, balance_comp);
		teehee(chaos_comp);
		calc.tc_static.at(row_counter) = teehee.get_mean_of_window(tc_static_base_window);
		++row_counter;
	}

	void advance_rm_comp(const float& rm_diff)
	{
		rm_itv_max_diff = std::max(rm_itv_max_diff, rm_diff);
	}

	void advance_jack_comp(const float& hardest_itv_jack_ms) {
		jack_itv_diff =
		  ms_to_scaled_nps(hardest_itv_jack_ms) * basescalers[Skill_JackSpeed];
	}

	// for debug
	[[nodiscard]] auto get_itv_rma_diff() const -> float
	{
		return rm_itv_max_diff;
	}

	// not for debug
	[[nodiscard]] auto get_itv_jack_diff() const -> float
	{
		return jack_itv_diff;
	}

	// final output difficulty for this interval
	// the output of this is officially TechBase for an interval
	[[nodiscard]] auto get_itv_diff(const float& nps_base, Calc& calc) const
	  -> float
	{
		auto rmbase = rm_itv_max_diff;
		const auto nps_biased_chaos_base = weighted_average(
		  get_tc_base(calc), nps_base, tc_base_weight, nps_base_weight);
		if (rmbase >= nps_biased_chaos_base) {
			// for rm dominant intervals, use tc to drag diff down
			// weight should be [0,1]
			// 1 -> all rm
			// 0 -> all tc
			rmbase = weighted_average(
			  rmbase, nps_biased_chaos_base, rm_base_weight, 1.F);
		}
		return std::max(nps_biased_chaos_base, rmbase);

		/*
		const auto trillbase = get_tb_base(calc);
		const auto jackbase = jack_itv_diff;

		// [0,1]
		// 0 = all jack/flam
		// 1 = all trill
		const auto how_flammy = get_itv_flam_factor();

		const auto combinedbase =
		  weighted_average(trillbase, jackbase, how_flammy, 1.F);
		return std::max(rmbase, combinedbase);
		*/
	}

	void interval_end()
	{
		row_counter = 0;
		rm_itv_max_diff = 0.F;
		jack_itv_diff = 0.F;
		balance_ratios.fill(0);
		//count_left.fill(0);
		//count_right.fill(0);
	}

	void full_reset()
	{
		row_counter = 0;
		rm_itv_max_diff = 0.F;
		jack_itv_diff = 0.F;
		teehee.zero();
		count_left.fill(0);
		count_right.fill(0);

		mw_dt.fill({ col_empty, ms_init });
		tb_static.fill(ms_init);
		flammity.fill(0.F);
	}

  private:
	// how many non-empty rows have we seen
	int row_counter = 0;

	// jank stuff.. keep a small moving average of the base diff
	CalcMovingWindow<float> teehee;

	// 1 or 0 continuously
	std::array<int, max_rows_for_single_interval> count_left{};
	std::array<int, max_rows_for_single_interval> count_right{};

	// balance ratio values for an interval
	std::array<float, max_rows_for_single_interval> balance_ratios{};

	// trill base values
	std::array<float, max_rows_for_single_interval> tb_static{};
	std::array<float, max_rows_for_single_interval> flammity{};

	// max value of rm diff for this interval, this will be an exception to the
	// only storing ms rule, rm diff will be stored as a pre-converted diff
	// value because rm_sequencing may adjust the scaled diff using the rm
	// components in ways we can't emulate here (nor should we try)
	float rm_itv_max_diff = 0.F;
	float jack_itv_diff = 0.F;

	// get the interval base diff, which will then be merged via weighted
	// average with npsbase, and then compared to max_rm diff
	[[nodiscard]] auto get_tc_base(Calc& calc) const -> float
	{
		if (row_counter == 0) {
			return 0.F;
		}

		auto ms_total = 0.F;
		for (auto i = 0; i < row_counter; ++i) {
			ms_total += calc.tc_static.at(i);
		}

		const auto ms_mean = ms_total / static_cast<float>(row_counter);
		return ms_to_scaled_nps(ms_mean);
	}

	// produces the average value of the trill ms value for the interval
	// converted to nps
	auto get_tb_base(Calc& calc) const -> float
	{
		if (row_counter < 3) {
			return 0.F;
		}

		auto ms_total = 0.F;
		for (auto i = 0; i < row_counter; ++i) {
			ms_total += tb_static.at(i);
		}
		const auto ms_mean = ms_total / static_cast<float>(row_counter);
		return ms_to_scaled_nps(ms_mean);
	}

	// produces the average value of the flam value for the interval
	// [0,1]
	// 0 = all jacks or all flams
	// 1 = all perfect trills
	auto get_itv_flam_factor() const -> float
	{
		if (row_counter == 0) {
			return 0.F;
		}

		auto total = 0.F;
		for (auto i = 0; i < row_counter; ++i) {
			total += flammity.at(i);
		}

		const auto mean = total / static_cast<float>(row_counter);
		return mean;
	}

	/// find the balance between columns in a hand
	/// max value is found when note count is exactly half the other
	/// min value is at 0/max and max/max
	/// returns [0,1]
	float calc_balance_comp() const {

		const auto left =
		  get_total_for_windowf(count_left, balance_comp_window);
		const auto right = 
		  get_total_for_windowf(count_right, balance_comp_window);

		// for this application of balance, dont care about half empty hands
		// or fully balanced hands
		if (left == 0.F || right == 0.F) {
			return 1.F;
		}
		if (left == right) {
			return 1.F;
		}

		// make sure power is at least 2 and divisible by 2
		// but it cant be greater than 30 due to int overflow
		const auto bal_power =
		  std::clamp(static_cast<int>(std::max(balance_power, 2.F)) % 2 == 0
					   ? balance_power
					   : balance_power - 1.F,
					 2.F,
					 30.F);
		const auto bal_factor = std::pow(2.F, bal_power);

		const auto high = left > right ? left : right;
		const auto low = left > right ? right : left;

		const auto x = low / high;
		const auto y = (-bal_factor * std::pow(x - 0.5F, bal_power)) + 1;
		return y;
	}

	/// in some wildly confusing fashion, find a degree of chaoticness
	/// but using MS->NPS
	/// considers a window of N+1 rows (N ms times)
	float calc_chaos_comp(const SequencerGeneral& seq,
						  const col_type& ct,
						  Calc& calc, const int& hand, const float& row_time)
	{
		const auto a = seq.get_sc_ms_now(ct);
		float b;
		if (ct == col_ohjump) {
			b = seq.get_sc_ms_now(ct, false);
		} else {
			b = seq.get_cc_ms_now();
		}

		// geometric mean of ms times since (last note in this column) and (last note in the other column)
		//const auto c = fastsqrt(a) * fastsqrt(b);

		// arithmetic mean instead
		const auto c = (a + b) / 2;

		// coeff var. of last N ms times on either column
		auto pineapple = seq._mw_any_ms.get_cv_of_window(chaos_comp_window);
		// coeff var. of last N ms times on left column
		auto porcupine =
		  seq._mw_sc_ms[col_left].get_cv_of_window(chaos_comp_window);
		// coeff var. of last N ms times on right column
		auto sequins =
		  seq._mw_sc_ms[col_right].get_cv_of_window(chaos_comp_window);

		// coeff var. is sd divided by mean
		// cv of 0 is 0 sd

		// all of those numbers are clamped to [0.5, 1.5] (or [oioi, ioio+oioi])
		const auto oioi = 0.5F;
		const auto ioio = 0.5F;
		pineapple = std::clamp(pineapple + oioi, oioi, ioio + oioi);
		porcupine = std::clamp(porcupine + oioi, oioi, ioio + oioi);
		sequins = std::clamp(sequins + oioi, oioi, ioio + oioi);

		// get most recent ms time in left column
		const auto scoliosis = seq._mw_sc_ms[col_left].get_now();
		// get most recent ms time in right column
		const auto poliosis = seq._mw_sc_ms[col_right].get_now();

		float obliosis;
		if (ct == col_left) {
			obliosis = poliosis / scoliosis;
		} else {
			obliosis = scoliosis / poliosis;
		}

		// the ratio of most recent ms times between left and right column must be [1,10]
		// 1 = perfect trill or slowing down trill
		// 10 = quickly speeding up trill (flams)
		obliosis = std::clamp(obliosis, 1.F, 10.F);

		// sqrt of ( [1,inf] - 1 ) (NOTE: fastsqrt IS NOT ACCURATE)
		// result = [0,sqrt(inf)]
		// 0 = perfect trill or perfect jumpjack
		// >0 = uneven trill
		// huge = one column is hitting notes faster than the other (maybe minijack in pattern)
		auto pewp = fastsqrt(div_high_by_low(scoliosis, poliosis) - 1.F);

		// [0,inf] divided by [1,10]
		pewp /= obliosis;

		if (calc.debugmode) {
			std::array<float, 4> a;
			a[0] = row_time;
			a[1] = pewp;
			a[2] = obliosis;
			a[3] = c;
			calc.debugTechVals.at(hand).emplace_back(a);
		}

		// average of (cv left, cv right, cv both)
		// note cv clamped to [0.5,1.5]
		// simplifies to [0.5,1.5]
		// output: [0.5, 1.5]
		const auto vertebrae = std::clamp(
		  ((pineapple + porcupine + sequins) / 3.F), oioi, ioio + oioi);

		// result is ms divided by fudgy cv number
		// [0,5000] / [0.5,1.5]
		return c / vertebrae;
	}

	void advance_trill_base(Calc& calc)
	{
		auto flamentation = .5F;
		auto trill_ms_value = ms_init;

		auto& a = mw_dt.at(0);
		if (a.first == col_init) {
			// do nothing special, not a complete trill
		} else {
			auto& b = mw_dt.at(1);
			auto& c = mw_dt.at(2);

			auto flam_of_the_trill = ms_init;
			auto third_tap = ms_init;

			if (b.second == 0.F && a.first != b.first) {
				// first 2 notes form a jump
				flam_of_the_trill = 0.F;
				third_tap = c.second * 2.F;
			} else if (c.second == 0.F && b.first != c.first) {
				// last 2 notes form a jump
				flam_of_the_trill = 0.F;
				third_tap = b.second * 2.F;
			} else {
				// determine ... trillflamjackyness

				if (a.first == c.first && a.first != b.first) {
					// it's a trill (121 or 212)
					flam_of_the_trill = std::min(b.second, c.second);
					third_tap = std::max(b.second, c.second);
				} else if (a.first == c.first) {
					// it's 111
					// the intended behavior is to treat it like a flam
					// thus treating it like a jack
					flam_of_the_trill = 0.F;
					third_tap = c.second;
				} else {
					if (a.first != b.first) {
						// it's 211
						flam_of_the_trill = b.second;
						third_tap = c.second;
					} else {
						// it's 112
						flam_of_the_trill = c.second;
						third_tap = b.second;
					}
				}
			}

			if (flam_of_the_trill == 0.F && third_tap == 0.F) {
				// effectively a forced dropped note
				flamentation = 0.F;
				trill_ms_value = 0.1F;
			} else {
				// ratio = [0,1]
				// 0 = flam involved
				// 1 = straight trill
				const auto flam_ms_depressor = 0.F;
				auto ratio = div_low_by_high(
				  std::max(flam_of_the_trill - flam_ms_depressor, 0.F),
				  third_tap);
				flamentation = std::clamp(std::pow(ratio, 0.125F), 0.F, 1.F);
				trill_ms_value = (flam_of_the_trill + third_tap) / 2.F;
			}
		}

		flammity.at(row_counter) = flamentation;
		tb_static.at(row_counter) = trill_ms_value;
	}


	//////////////////////////////////////////////////////////////
	// util
	void increment_column_counters(const col_type& ct)
	{
		switch (ct) {
			case col_left: {
				insert(count_left, 1);
				insert(count_right, 0);
				break;
			}
			case col_right: {
				insert(count_left, 0);
				insert(count_right, 1);
				break;
			}
			case col_ohjump: {
				insert(count_left, 1);
				insert(count_right, 1);
				break;
			}
			default: {
				insert(count_left, 0);
				insert(count_right, 0);
				break;
			}
		}
	}

	float get_total_for_windowf(
	  const std::array<int, max_rows_for_single_interval>& arr, const unsigned window) const
	{
		float o = 0.F;
		auto i = max_rows_for_single_interval;
		while (i > max_rows_for_single_interval - window) {
			i--;
			o += arr.at(i);
		}
		return o;
	}

	template <typename T>
	void insert(std::array<T, max_rows_for_single_interval>& arr,
				 const T value)
	{
		for (auto i = 1; i < max_rows_for_single_interval; i++) {
			arr.at(i - 1) = arr.at(i);
		}
		arr.at(max_rows_for_single_interval - 1) = value;
	}

	void process_mw_dt(const col_type& ct, const float ms_now)
	{
		auto& arr = mw_dt;
		if (ct == col_ohjump) {
			for (auto i = 1; i < trill_window; i++) {
				arr.at(i - 1) = arr.at(i);
			}
			arr.at(trill_window - 1) = { col_left, ms_now };

			for (auto i = 1; i < trill_window; i++) {
				arr.at(i - 1) = arr.at(i);
			}
			arr.at(trill_window - 1) = { col_right, 0.F };
		} else {
			for (auto i = 1; i < trill_window; i++) {
				arr.at(i - 1) = arr.at(i);
			}
			arr.at(trill_window - 1) = { ct, ms_now };
		}
	}
};

struct diffz
{
	nps _nps;
	techyo _tc;
	ceejay _cj;

	void interval_end()
	{
		_tc.interval_end();
		_cj.interval_end();
	}

	void full_reset()
	{
		interval_end();
		_cj.full_reset();
		_tc.full_reset();
	}
};
