#pragma once
#include "../../PatternModHelpers.h"

/// Hand-Agnostic PatternMod describing chord density.
/// Forms a value based on counts of chords of different sizes
/// relative to the number of notes in the interval
struct CJDensityMod
{
	const CalcPatternMod _pmod = CJDensity;
	const std::string name = "CJDensityMod";
	const int _tap_size = quad;

#pragma region params

	float min_mod = 0.98F;
	float max_mod = 1.F;
	float base = 0.F;

	float single_scaler = 1.F;
	float jump_scaler = 1.25F;
	float hand_scaler = 0.9F;
	float quad_scaler = 1.15F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "single_scaler", &single_scaler },
		{ "jump_scaler", &jump_scaler },
		{ "hand_scaler", &hand_scaler },
		{ "quad_scaler", &quad_scaler },
	};
#pragma endregion params and param map

	float pmod = neutral;

	auto operator()(const metaItvInfo& mitvi) -> float
	{
		const auto& itvi = mitvi._itvi;
		if (itvi.total_taps == 0) {
			return neutral;
		}

		const auto& t_taps = static_cast<float>(itvi.total_taps);
		auto a0 =
		  static_cast<float>(static_cast<float>(itvi.taps_by_size[single]) *
							 single_scaler) /
		  t_taps;
		auto a1 = static_cast<float>(
					static_cast<float>(itvi.taps_by_size[jump]) * jump_scaler) /
				  t_taps;
		auto a2 = static_cast<float>(
					static_cast<float>(itvi.taps_by_size[hand]) * hand_scaler) /
				  t_taps;
		auto a3 = static_cast<float>(
					static_cast<float>(itvi.taps_by_size[quad]) * quad_scaler) /
				  t_taps;

		auto aaa = a0 + a1 + a2 + a3;

		pmod = std::clamp(base + fastsqrt(aaa), min_mod, max_mod);

		return pmod;
	}
};
