#pragma once
#include "../IntervalHandInfo.h"

/// Hand-Dependent PatternMod detecting jump jacks but considering flammyness.
/// Collects the number of consecutive jumpjacks, largest sequence of them
struct WideRangeJJMod
{
	const CalcPatternMod _pmod = { WideRangeJJ };
	const std::string name = "WideRangeJJMod";

#pragma region params

	float window_param = 3.F;
	// how many jumpjacks are required for the pmod to not be neutral
	// it considers the entire combined moving window set by window_param
	float jj_required = 30.F;

	float min_mod = 0.25F;
	float max_mod = 1.F;
	float total_scaler = 2.5F;
	float cur_interval_tap_scaler = 1.2F;

	// ms apart for 2 taps to be considered a jumpjack
	// 0.075 is 200 bpm 16th trills
	// 0.050 is 300 bpm
	// 0.037 is 400 bpm
	// 0.020 is 750 bpm (375 bpm 64th)
	float ms_threshold = 0.065F;

	// add this much to the pmod before sqrt when below threshold
	float calming_comp = 0.05F;

	// changes the direction and sharpness of the result curve
	// as the jumpjack width is between 0 and ms_threshold
	// a higher number here makes numbers closer to ms_threshold
	// worth more -- the falloff occurs late
	float diff_falloff_power = 6.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "intervals_to_consider", &window_param },
		{ "jumpjacks_required_in_combined_window", &jj_required },
		{ "jumpjack_total_scaler", &total_scaler },
		{ "cur_interval_tap_scaler", &cur_interval_tap_scaler },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "calming_comp", &calming_comp },

		{ "ms_threshold", &ms_threshold },
		{ "diff_falloff_power", &diff_falloff_power },
	};
#pragma endregion params and param map

	int window = 0;

	// indices
	int lc = 0;
	int rc = 0;

	// moving window of "problems"
	// specifically, the longest consecutive series of "problems"
	// a "problem" is a rough value of jumpjackyness
	// whereas a 1 is 1 jump and the worst possible flam is nearly 0
	// this tracks amount of "problems" in consecutive intervals
	CalcMovingWindow<float> _mw_max_problems{};
	float current_problems = 0.F;
	float max_interval_problems = 0.F;

	float pmod = neutral;

	// timestamps of notes in columns
	std::array<float, max_rows_for_single_interval> _left_times{};
	std::array<float, max_rows_for_single_interval> _right_times{};

#pragma region generic functions

	void full_reset()
	{
		_mw_max_problems.zero();
		_left_times.fill(s_init);
		_right_times.fill(s_init);
		current_problems = 0.F;
		max_interval_problems = 0.F;
		lc = 0;
		rc = 0;

		pmod = neutral;
	}

	void setup()
	{
		window =
		  std::clamp(static_cast<int>(window_param), 1, max_moving_window_size);
	}

#pragma endregion

	void check() {
		// check times in parallel
		// any times within the window count as the jumpishjack
		// just ... determine the degree of jumpy the jumpyjack is
		// using ms ... or something
		auto lindex = 0;
		auto rindex = 0;
		auto jumpJacking = false;
		auto failedLeft = 0;
		auto failedRight = 0;
		while (lindex < lc && rindex < rc) {
			const auto& l = _left_times.at(lindex);
			const auto& r = _right_times.at(rindex);
			const auto diff = fabsf(l - r);

			if (diff < ms_threshold) {
				lindex++;
				rindex++;

				// werent previously jumpjacking, restart at 0
				if (!jumpJacking) {
					current_problems = 0.F;
				}

				// given time_scaler = 1
				// diff of ms_threshold gives a value of 1
				// meaning "1 jumpjack" or "1 problem"
				// but a flammy one, is worth not so much of a jumpjack
				// x=0 would be y=1, a jump
				// using std::pow for accuracy here
				const auto x = std::pow(diff / std::max(ms_threshold, 0.00001F),
									   diff_falloff_power);
				const auto v = 1 + (x / (x - 2));
				current_problems += v;
				if (current_problems > max_interval_problems) {
					max_interval_problems = current_problems;
				}
				jumpJacking = true;
			} else {
				// failed case
				// throw the oldest value and try again...
				if (l > r) {
					rindex++;
					if (failedRight) {
						jumpJacking = false;
					}
					failedRight = true;
				} else if (r > l) {
					lindex++;
					if (failedLeft) {
						jumpJacking = false;
					}
					failedLeft = true;
				} else {
					// this case exists to prevent infinite loops
					// it should never happen unless you put bad values in params
					lindex++;
					rindex++;

					if (failedLeft || failedRight) {
						jumpJacking = false;
					}
					failedLeft = true;
					failedRight = true;
				}
			}
		}
	}

	void advance_sequencing(const col_type& ct,
							const float& time_s)
	{
		if (lc >= max_rows_for_single_interval ||
			rc >= max_rows_for_single_interval) {
			// completely impossible condition
			// checking for sanity and safety
			return;
		}

		switch (ct) {
			case col_left: {
				_left_times.at(lc++) = time_s;
				break;
			}
			case col_right: {
				_right_times.at(rc++) = time_s;
				break;
			}
			case col_ohjump: {
				_left_times.at(lc++) = time_s;
				_right_times.at(rc++) = time_s;
				break;
			}
			default:
				break;
		}
	}

	void set_pmod(const ItvHandInfo& itvhi)
	{
		const auto taps_in_window =
		  itvhi.get_taps_windowf(window) * cur_interval_tap_scaler;
		const auto problems_in_window =
		  _mw_max_problems.get_total_for_windowf(window) * total_scaler;

		// no taps or below threshold, or actionable condition
		if (taps_in_window == 0.F || problems_in_window < jj_required) {
			// when below threshold, the pmod will drift back to neutral
			// ideally take less than 5 intervals to drift
			pmod = fastsqrt(pmod + std::clamp(calming_comp, 0.F, 1.F));
		} else {
			pmod = taps_in_window / problems_in_window * 0.75;
		}

		pmod = std::clamp(pmod, min_mod, max_mod);
	}

	auto operator()(const ItvHandInfo& itvhi) -> float
	{
		check();
		_mw_max_problems(max_interval_problems);

		set_pmod(itvhi);

		interval_end();
		return pmod;
	}

	void interval_end()
	{
		// reset every interval when finished
		current_problems = 0.F;
		max_interval_problems = 0.F;
		_left_times.fill(s_init);
		_right_times.fill(s_init);
		lc = 0;
		rc = 0;
	}
};
