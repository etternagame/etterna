#pragma once
#include "../IntervalHandInfo.h"

/// Hand-Dependent PatternMod detecting jump jacks but considering flammyness
struct WideRangeJJMod
{
	const CalcPatternMod _pmod = { WideRangeJJ };
	const std::string name = "WideRangeJJMod";

#pragma region params

	float window_param = 6.F;
	// how many jumpjacks are required for the pmod to not be neutral
	// it considers the entire combined moving window set by window_param
	float jj_required = 26.F;

	float min_mod = 0.25F;
	float max_mod = 1.F;
	float total_scaler = 2.F;

	// ms apart for 2 taps to be considered a jumpjack
	float ms_threshold = 0.017F;

	// the scaler will change how quickly a non-ohj flam is worth nothing
	// scaler = 1 means a flam of ms_threshold span is worth 0
	// scaler > 1 will lower the ms_threshold artificially
	// this can be useful if we want to "accept" values
	// but throw away some that are close but not worth caring about
	float time_scaler = 1.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "intervals_to_consider", &window_param },
		{ "jumpjacks_required_in_combined_window", &jj_required },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "total_scaler", &total_scaler },

		{ "ms_threshold", &ms_threshold },
		{ "time_scaler", &time_scaler },
	};
#pragma endregion params and param map

	int window = 0;

	// indices
	int lc = 0;
	int rc = 0;

	// moving window of "problems"
	// a "problem" is a rough value of jumpjackyness
	// whereas a 1 is 1 jump and the worst possible flam is nearly 0
	// this tracks amount of "problems" in consecutive intervals
	CalcMovingWindow<float> _mw_problems;
	float interval_problems = 0.F;

	float pmod = neutral;

	// timestamps of notes in columns
	std::array<float, max_rows_for_single_interval> _left_times;
	std::array<float, max_rows_for_single_interval> _right_times;

#pragma region generic functions

	void full_reset()
	{
		_mw_problems.zero();
		_left_times.fill(s_init);
		_right_times.fill(s_init);
		interval_problems = 0.F;
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
		while (lindex < lc && rindex < rc) {
			const auto& l = _left_times.at(lindex);
			const auto& r = _right_times.at(rindex);
			const auto diff = fabsf(l - r);

			if (diff < ms_threshold) {
				lindex++;
				rindex++;

				// given time_scaler = 1
				// diff of ms_threshold gives a value of 1
				// meaning "1 jumpjack" or "1 problem"
				// but a flammy one, is worth not so much of a jumpjack
				const auto v =
				  1 - ((diff / std::max(0.1F, time_scaler)) / ms_threshold);
				interval_problems += v;
			} else {
				// failed case
				// throw the oldest value and try again...
				if (l > r) {
					rindex++;
				} else if (r > l) {
					lindex++;
				} else {
					// this case exists to prevent infinite loops
					// it should never happen unless you put bad values in params
					lindex++;
					rindex++;
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
		// no taps, no jj
		if (itvhi.get_taps_windowi(window) == 0 ||
			_mw_problems.get_total_for_window(window) == 0.F) {
			pmod = neutral;
			return;
		}

		if (_mw_problems.get_total_for_window(window) < jj_required) {
			pmod = neutral;
			return;
		}

		pmod =
		  itvhi.get_taps_windowf(window) /
		  ((_mw_problems.get_total_for_windowf(window) * 2.F) * total_scaler);

		pmod = std::clamp(pmod, min_mod, max_mod);
	}

	auto operator()(const ItvHandInfo& itvhi) -> float
	{
		check();
		_mw_problems(interval_problems);

		set_pmod(itvhi);

		interval_end();
		return pmod;
	}

	void interval_end()
	{
		// we could count these in metanoteinfo but let's do it here for now,
		// reset every interval when finished
		interval_problems = 0.F;
		_left_times.fill(s_init);
		_right_times.fill(s_init);
		lc = 0;
		rc = 0;
	}
};
