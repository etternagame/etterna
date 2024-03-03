#pragma once
#include "../../PatternModHelpers.h"

/// Hand-Agnostic PatternMod detecting Stream.
/// Looks for single taps out of all taps in the interval.
/// Begins to dampen in value if too many jacks or chords are present
struct GStreamMod
{
	const CalcPatternMod _pmod = GStream;
	const std::string name = "GenericStreamMod";
	const int _tap_size = single;

#pragma region params
	float base = 0.F;
	float min_mod = 0.6F;
	float max_mod = 1.0F;
	float prop_buffer = 1.F;
	float prop_scaler = 1.41F;

	float jack_pool = 4.F;
	float jack_comp_min = 0.5F;
	float jack_comp_max = 1.F;

	float vibro_flag = 1.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "base", &base },
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

	void setup() {

	}

	void advance_sequencing(const float& ms_now, const unsigned& notes) {

	}

	void full_reset() {

	}

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

		prop_component =
		  static_cast<float>(itvi.taps_by_size.at(_tap_size) + prop_buffer) /
		  static_cast<float>(static_cast<float>(itvi.total_taps) -
							 prop_buffer) *
		  prop_scaler;

		// allow for a mini/triple jack in streams.. but not more than that
		jack_component = std::clamp(
		  jack_pool - mitvi.actual_jacks, jack_comp_min, jack_comp_max);
		pmod = fastsqrt(prop_component * jack_component);

		pmod = std::clamp(base + pmod, min_mod, max_mod);

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
