#pragma once
#include "../IntervalHandInfo.h"
#include "../HD_Sequencers/GenericSequencing.h"

/// Hand-Dependent PatternMod detecting anchors in general.
/// cj specific mod
/// scales down chordjacks if they get long anchor vibro-y
struct WideRangeCJAnchorMod
{
	const CalcPatternMod _pmod = WideRangeCJAnchor;
	const std::string name = "WideRangeCJAnchorMod";

#pragma region params

	float min_mod = 0.5F;
	float max_mod = 1.F;

	float len_3_weight = 0.01F;
	float len_long_weight = -0.0034F;
	float min_required_taps = 7.F;
	float moving_window_intervals = 2.F;
	float long_anchor_pow_base = 1.025F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },

		{ "len_3_weight", &len_3_weight },
		{ "len_long_weight", &len_long_weight },
		{ "long_anchor_pow_base", &long_anchor_pow_base },

		{ "min_required_taps", &min_required_taps },
		{ "moving_window_intervals", &moving_window_intervals },
	};
#pragma endregion params and param map


	// set in setup
	float pmod = min_mod;

	void full_reset()
	{
		interval_end();
		pmod = neutral;
	}

	void setup()
	{

	}

	void set_pmod(const ItvHandInfo& itvhi, const AnchorSequencer& as)
	{
		// if the interval has basically no notes to care about
		// just bias to neutral
		// this should help really low density charts not get nerfed
		if (itvhi.get_taps_nowf() < min_required_taps) {
			pmod = fastsqrt(pmod);
			return;
		}

		const auto left = as.get_max_for_window_and_col(
		  col_left, static_cast<int>(moving_window_intervals));
		const auto right = as.get_max_for_window_and_col(
		  col_right, static_cast<int>(moving_window_intervals));

		// we are interested in whether or not a finger is anchoring hard
		// this is different from WideRangeAnchor (which is weighing the difference of fingers)

		const auto anchorest_value = left > right ? left : right;

		if (anchorest_value < 3) {
			// too short to care. the interval is full of probably not actual anchors
			pmod = fastsqrt(pmod);

		} else if (anchorest_value == 3) {
			// 3 has one way to scale the pmod
			pmod = fastsqrt(pmod) + len_3_weight;

		} else {
			// more than 3 has something more involved
			// this generally means the longer the anchor the less it is worth
			// if the weight is negative
			pmod = pmod + fastpow(long_anchor_pow_base, anchorest_value) * len_long_weight;
		}
	}

	auto operator()(const ItvHandInfo& itvhi, const AnchorSequencer& as)
	  -> float
	{
		set_pmod(itvhi, as);
		pmod = std::clamp(pmod, min_mod, max_mod);

		interval_end();
		return pmod;
	}

	void interval_end()
	{

	}
};
