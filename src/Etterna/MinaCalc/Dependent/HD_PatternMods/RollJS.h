#pragma once
#include "../IntervalHandInfo.h"
#include "../HD_Sequencers/GenericSequencing.h"

struct RollJSMod
{
	const CalcPatternMod _pmod = RollJS;
	const std::string name = "RollJSMod";

#pragma region params

	float min_mod = 0.85F;
	float max_mod = 1.F;
	float base = 0.1F;
	float jj_scaler = 2.F;

	// ms apart for 2 taps to be considered a jumpjack
	// 0.075 is 200 bpm 16th trills
	// 0.050 is 300 bpm
	// 0.037 is 400 bpm
	// 0.020 is 750 bpm (375 bpm 64th)
	float ms_threshold = 0.0701F;

	// changes the direction and sharpness of the result curve
	// as the jumpjack width is between 0 and ms_threshold
	// a higher number here makes numbers closer to ms_threshold
	// worth more -- the falloff occurs late
	float diff_falloff_power = 1.F;

	float required_notes_before_nerf = 6.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "jj_scaler", &jj_scaler },
		{ "ms_threshold", &ms_threshold },
		{ "diff_falloff_power", &diff_falloff_power },
		{ "required_notes_before_nerf", &required_notes_before_nerf },
	};
#pragma endregion params and param map

	// indices
	int lc = 0;
	int rc = 0;

	// a "problem" is a rough value of jumpjackyness
	// whereas a 1 is 1 jump and the worst possible flam is nearly 0
	// this tracks amount of "problems" in consecutive intervals
	float current_problems = 0.F;

	float pmod = neutral;

	// timestamps of notes in columns
	std::array<float, max_rows_for_single_interval> _left_times{};
	std::array<float, max_rows_for_single_interval> _right_times{};

#pragma region generic functions

	void full_reset()
	{
		_left_times.fill(s_init);
		_right_times.fill(s_init);
		current_problems = 0.F;
		lc = 0;
		rc = 0;

		pmod = neutral;
	}

	void setup() {}

#pragma endregion

	void check()
	{
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

			if (diff <= ms_threshold) {
				lindex++;
				rindex++;

				// given time_scaler = 1
				// diff of ms_threshold gives a v of 1
				// meaning "1 jumpjack" or "1 problem"
				// but a flammy one, is worth not so much of a jumpjack
				// (this function at x=[0,1] begins slow and drops fast)
				// using std::pow for accuracy here
				const auto x = std::pow(diff / std::max(ms_threshold, 0.00001F),
									   diff_falloff_power);
				const auto v = 1 + (x / (x - 2));
				current_problems += v;
			} else {
				// failed case
				// throw the oldest value and try again...
				if (l > r) {
					rindex++;
				} else if (r > l) {
					lindex++;
				} else {
					// this case exists to prevent infinite loops
					// it should never happen unless you put bad values in
					// params
					lindex++;
					rindex++;
				}
			}
		}
	}

	void advance_sequencing(const col_type& ct, const float& time_s)
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
		if (itvhi.get_taps_nowi() == 0 || current_problems == 0.F) {
			pmod = neutral;
			return;
		}

		if (itvhi.get_taps_nowf() < required_notes_before_nerf) {
			pmod = neutral;
			return;
		}

		pmod = itvhi.get_taps_nowf() / ((current_problems * 2.F) * jj_scaler);
		pmod = std::clamp(base + pmod, min_mod, max_mod);
	}

	auto operator()(const ItvHandInfo& itvhi) -> float
	{
		check();
		set_pmod(itvhi);

		interval_end();
		return pmod;
	}

	void interval_end()
	{
		// reset every interval when finished
		current_problems = 0.F;
		_left_times.fill(s_init);
		_right_times.fill(s_init);
		lc = 0;
		rc = 0;
	}
};
#pragma once
