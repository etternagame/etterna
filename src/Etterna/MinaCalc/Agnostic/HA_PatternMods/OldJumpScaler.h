#pragma once
#include "../../PatternModHelpers.h"

/// Agnostic downscaler based on how many jumps are in the interval
struct OldJumpScalerMod
{
	const CalcPatternMod _pmod = OldJumpScaler;
	const std::string name = "OldJumpScalerMod";

#pragma region params

	float min_mod = 0.6F;
	float max_mod = 1.1F;

	float extra_divisor = 6.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },

		{ "extra_divisor", &extra_divisor },
	};
#pragma endregion params and param map

	float pmod = neutral;

#pragma region generic functions

	void full_reset() {
		pmod = neutral;
	}

#pragma endregion

	void set_pmod(const metaItvInfo& mitvi)
	{
		const auto& itvhi = mitvi._itvi;

		if (itvhi.total_taps == 0) {
			pmod = neutral;
		} else {

			// goes up by N for how many notes per row
			const auto taps = static_cast<float>(itvhi.total_taps);
			// goes up by 2 for amount of jumps
			const auto jumps = itvhi.taps_by_size.at(jump);

			pmod = fastsqrt(fastsqrt(1 - (jumps / taps / extra_divisor)));
		}
		pmod = std::clamp(pmod, min_mod, max_mod);
	}

	auto operator()(const metaItvInfo& mitvi) -> float
	{
		set_pmod(mitvi);
		return pmod;
	}
};
