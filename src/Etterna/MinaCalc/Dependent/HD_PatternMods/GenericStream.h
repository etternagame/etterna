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

	auto operator()(const metaItvGenericHandInfo& mitvghi) -> float
	{
		// it needs more taps to bracket
		if (mitvghi.total_taps < 2) {
			return neutral;
		}

		// it's all chords
		if (mitvghi.taps_by_size.at(_tap_size) == 0) {
			return min_mod;
		}

		prop_component =
		  static_cast<float>(mitvghi.taps_by_size.at(_tap_size) + prop_buffer) /
		  static_cast<float>(static_cast<float>(mitvghi.total_taps) -
							 prop_buffer) *
		  prop_scaler;

		pmod = fastsqrt(prop_component);

		pmod = std::clamp(base + pmod, min_mod, max_mod);

		// actual mod
		return pmod;
	}
};
