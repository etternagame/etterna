#pragma once
#include "../../PatternModHelpers.h"

/// Hand-Agnostic PatternMod detecting Handstream.
/// Looks for jacks, jumptrills, and hands (3-chords)
struct GBracketingMod
{
	const CalcPatternMod _pmod = GBracketing;
	// const std::vector<CalcPatternMod> _dbg = { HSS, HSJ };
	const std::string name = "GenericBracketingMod";
	const int min_tap_size = jump;

#pragma region params

	float min_mod = 0.6F;
	float max_mod = 1.1F;
	float mod_base = 0.4F;
	float prop_buffer = 1.F;

	float total_prop_min = min_mod;
	float total_prop_max = max_mod;

	// was ~32/7, is higher now to push up light hs (maybe overkill tho)
	float total_prop_scaler = 5.571F;
	float total_prop_base = 0.4F;

	float split_hand_pool = 1.6F;
	float split_hand_min = 0.89F;
	float split_hand_max = 1.F;
	float split_hand_scaler = 1.F;

	float jack_pool = 1.35F;
	float jack_min = 0.5F;
	float jack_max = 1.F;
	float jack_scaler = 1.F;

	float decay_factor = 0.05F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_base", &mod_base },
		{ "prop_buffer", &prop_buffer },

		{ "total_prop_scaler", &total_prop_scaler },
		{ "total_prop_min", &total_prop_min },
		{ "total_prop_max", &total_prop_max },
		{ "total_prop_base", &total_prop_base },

		{ "decay_factor", &decay_factor },
	};
#pragma endregion params and param map

	float total_prop = 0.F;
	float jumptrill_prop = 0.F;
	float jack_prop = 0.F;
	float last_mod = min_mod;
	float pmod = min_mod;
	float t_taps = 0.F;
	float bracket_taps = 0.F;

	void full_reset()
	{
		last_mod = min_mod;
	}

	void decay_mod()
	{
		pmod = std::clamp(last_mod - decay_factor, min_mod, max_mod);
		last_mod = pmod;
	}

	auto operator()(const metaItvGenericHandInfo& mitvghi) -> float
	{
		// empty interval, don't decay mod or update last_mod
		if (mitvghi.total_taps == 0) {
			return neutral;
		}

		// definitely no brackets, decay
		if (mitvghi.taps_bracketing == 0) {
			decay_mod();
			return pmod;
		}

		t_taps = static_cast<float>(mitvghi.total_taps);
		bracket_taps = static_cast<float>(mitvghi.taps_bracketing);

		total_prop =
		  total_prop_base + ((bracket_taps + prop_buffer) /
							 (t_taps - prop_buffer) * total_prop_scaler);
		total_prop =
		  std::clamp(fastsqrt(total_prop), total_prop_min, total_prop_max);

		// limits
		pmod = std::clamp(total_prop, min_mod, max_mod);

		// for decay
		last_mod = pmod;

		return pmod;
	}
};
