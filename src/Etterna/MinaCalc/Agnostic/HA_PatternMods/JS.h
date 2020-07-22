#pragma once
#include "../../PatternModHelpers.h"

struct JSMod
{
	const CalcPatternMod _pmod = JS;
	// const vector<CalcPatternMod> _dbg = { JSS, JSJ };
	const std::string name = "JSMod";
	const int _tap_size = jump;

#pragma region params
	float min_mod = 0.6F;
	float max_mod = 1.1F;
	float mod_base = 0.F;
	float prop_buffer = 1.F;

	float total_prop_min = min_mod;
	float total_prop_max = max_mod;
	float total_prop_scaler = 2.714F; // ~19/7

	float split_hand_pool = 1.5F;
	float split_hand_min = 0.9F;
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

		{ "split_hand_pool", &split_hand_pool },
		{ "split_hand_min", &split_hand_min },
		{ "split_hand_max", &split_hand_max },
		{ "split_hand_scaler", &split_hand_scaler },

		{ "jack_pool", &jack_pool },
		{ "jack_min", &jack_min },
		{ "jack_max", &jack_max },
		{ "jack_scaler", &jack_scaler },

		{ "decay_factor", &decay_factor },
	};
#pragma endregion params and param map

	float total_prop = 0.F;
	float jumptrill_prop = 0.F;
	float jack_prop = 0.F;
	float last_mod = min_mod;
	float pmod = min_mod;
	float t_taps = 0.F;

	// inline void set_dbg(vector<float> doot[], const int& i)
	//{
	//		doot[JSS][i] = jumptrill_prop;
	//		doot[JSJ][i] = jack_prop;
	//}

	void decay_mod()
	{
		pmod = std::clamp(last_mod - decay_factor, min_mod, max_mod);
		last_mod = pmod;
	}

	auto operator()(const metaItvInfo& mitvi) -> float
	{
		const auto& itvi = mitvi._itvi;

		// empty interval, don't decay js mod or update last_mod
		if (itvi.total_taps == 0) {
			return neutral;
		}

		// at least 1 tap but no jumps
		if (itvi.taps_by_size.at(_tap_size) == 0) {
			decay_mod();
			return pmod;
		}

		/* end case optimizations */

		t_taps = static_cast<float>(itvi.total_taps);

		// creepy banana
		total_prop =
		  static_cast<float>(itvi.taps_by_size.at(_tap_size) + prop_buffer) /
		  (t_taps - prop_buffer) * total_prop_scaler;
		total_prop =
		  std::clamp(fastsqrt(total_prop), total_prop_min, total_prop_max);

		// punish lots splithand jumptrills
		// uhh this might also catch oh jumptrills can't remember
		jumptrill_prop = std::clamp(
		  split_hand_pool - (static_cast<float>(mitvi.not_js) / t_taps),
		  split_hand_min,
		  split_hand_max);

		// downscale by jack density rather than upscale like cj
		// theoretically the ohjump downscaler should handle
		// this but handling it here gives us more flexbility
		// with the ohjump mod
		jack_prop = std::clamp(
		  jack_pool - (static_cast<float>(mitvi.actual_jacks) / t_taps),
		  jack_min,
		  jack_max);

		pmod =
		  std::clamp(total_prop * jumptrill_prop * jack_prop, min_mod, max_mod);

		if (mitvi.dunk_it) {
			pmod *= 0.99F;
		}

		// set last mod, we're using it to create a decaying mod that won't
		// result in extreme spikiness if files alternate between js and
		// hs/stream
		last_mod = pmod;

		return pmod;
	}
};
