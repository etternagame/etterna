#pragma once
#include "../IntervalHandInfo.h"
#include "../HD_Sequencers/GenericSequencing.h"

/// Hand-Dependent PatternMod detecting anchors in general.
/// general mod, should maybe make cj specific one
struct WideRangeAnchorMod
{
	const CalcPatternMod _pmod = WideRangeAnchor;
	const std::string name = "WideRangeAnchorMod";

#pragma region params

	float window_param = 2.F;

	float min_mod = 1.F;
	float max_mod = 1.1F;
	float base = 1.F;

	float diff_min = 4.F;
	float diff_max = 16.F;
	float scaler = 0.5F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "diff_min", &diff_min },
		{ "diff_max", &diff_max },
		{ "scaler", &scaler },
	};
#pragma endregion params and param map

	int window = 0;
	int a = 0;
	int b = 0;
	int diff = 0;

	// set in setup
	float divisor = 0.F;
	float pmod = min_mod;

	void full_reset()
	{
		interval_end();
		pmod = neutral;
	}

	void setup()
	{
		// setup should be run after loading params from disk
		window =
		  std::clamp(static_cast<int>(window_param), 1, max_moving_window_size);
		divisor = static_cast<float>(static_cast<int>(diff_max) -
									 static_cast<int>(diff_min));

		// /0 lul
		if (divisor < 0.1F)
			divisor = 0.1F;
	}

	void set_pmod(const ItvHandInfo& itvhi, const AnchorSequencer& as)
	{
		a = as.get_max_for_window_and_col(col_left, window);
		b = as.get_max_for_window_and_col(col_right, window);

		diff = diff_high_by_low(a, b);

		// nothing here
		if (a == 0 && b == 0) {
			pmod = neutral;
			return;
		}

		// set max mod if either is 0
		if (a == 0 || b == 0) {
			pmod = max_mod;
			return;
		}

		// difference won't matter
		if (diff <= static_cast<int>(diff_min)) {
			pmod = min_mod;
			return;
		}

		// would max anyway
		if (diff > static_cast<int>(diff_max)) {
			pmod = max_mod;
			return;
		}

		pmod =
		  base + (scaler * ((static_cast<float>(diff) - diff_min) / divisor));
		pmod = std::clamp(pmod, min_mod, max_mod);
	}

	auto operator()(const ItvHandInfo& itvhi, const AnchorSequencer& as)
	  -> float
	{
		set_pmod(itvhi, as);

		interval_end();
		return pmod;
	}

	/* ok technically not necessary since we don't ever do anything before these
	 * values are updated, however, supposing we do add anything like shortcut
	 * case handling prior to calculating these values, better this is already
	 * in place */
	void interval_end()
	{
		diff = 0;
		a = 0;
		b = 0;
	}
};
