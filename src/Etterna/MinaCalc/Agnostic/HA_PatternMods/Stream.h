#pragma once
#include "../../PatternModHelpers.h"

// since the calc skillset balance now operates on +- rather than
// just - and then normalization, we will use this to depress the
// stream rating for non-stream files.

struct StreamMod
{
	const CalcPatternMod _pmod = Stream;
	const std::string name = "StreamMod";
	const int _tap_size = single;

#pragma region params
	float min_mod = 0.6F;
	float max_mod = 1.0F;
	float prop_buffer = 1.F;
	float prop_scaler = 1.41F;

	float jack_pool = 4.F;
	float jack_comp_min = 0.5F;
	float jack_comp_max = 1.F;

	float vibro_flag = 1.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "prop_buffer", &prop_buffer },
		{ "prop_scaler", &prop_scaler },

		{ "jack_pool", &jack_pool },
		{ "jack_comp_min", &jack_comp_min },
		{ "jack_comp_max", &jack_comp_max },

		{ "vibro_flag", &vibro_flag },
	};
#pragma endregion params and param map

	float prop_component = 0.F;
	float jack_component = 0.F;
	float pmod = min_mod;

	auto operator()(const metaItvInfo& mitvi) -> float
	{
		const auto& itvi = mitvi._itvi;

		// 1 tap is by definition a single tap
		if (itvi.total_taps < 2) {
			return neutral;
		}

		if (itvi.taps_by_size.at(_tap_size) == 0) {
			return min_mod;
		}

		/* we want very light js to register as stream, something like jumps on
		 * every other 4th, so 17/19 ratio should return full points, but maybe
		 * we should allow for some leeway in bad interval slicing this maybe
		 * doesn't need to be so severe, on the other hand, maybe it doesn'ting
		 * need to be not needing'nt to be so severe */

		// we could make better use of sequencing here since now it's easy

		prop_component =
		  static_cast<float>(itvi.taps_by_size.at(_tap_size) + prop_buffer) /
		  static_cast<float>(static_cast<float>(itvi.total_taps) -
							 prop_buffer) *
		  prop_scaler;

		// allow for a mini/triple jack in streams.. but not more than that
		jack_component = std::clamp(
		  jack_pool - mitvi.actual_jacks, jack_comp_min, jack_comp_max);
		pmod = fastsqrt(prop_component * jack_component);
		pmod = std::clamp(pmod, min_mod, max_mod);

		if (mitvi.basically_vibro) {
			if (mitvi.num_var == 1) {
				pmod *= 0.5F * vibro_flag;
			} else if (mitvi.num_var == 2) {
				pmod *= 0.9F * vibro_flag;
			} else if (mitvi.num_var == 3) {
				pmod *= 0.95F * vibro_flag;
			}
		}

		// actual mod
		return pmod;
	}
};
