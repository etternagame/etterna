#pragma once
#include "../MetaIntervalHandInfo.h"

/// Hand downscaler based on how many one hand jumps there are
struct OldOHJScalerMod
{
	const CalcPatternMod _pmod = OldOHJScaler;
	const std::string name = "OldOHJScalerMod";

#pragma region params

	float min_mod = 0.6F;
	float max_mod = 1.1F;

	float extra_divisor = 1.F;
	float power_scale = 0.24F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },

		{ "extra_divisor", &extra_divisor },
		{ "power_scale", &power_scale },
	};
#pragma endregion params and param map

	float pmod = neutral;

#pragma region generic functions

	void full_reset() { pmod = neutral; }

#pragma endregion

	void set_pmod(const ItvHandInfo& itvhi)
	{

		// nothing here
		if (itvhi.get_taps_nowi() == 0) {
			pmod = neutral;
		} else {
			// taps counts up twice for ohj
			const auto taps = itvhi.get_taps_nowf();
			// one ohj makes this also worth 2
			const auto jumptaps = itvhi.get_col_taps_nowf(col_ohjump);
			// so, if there is just a single ohj in the interval
			// taps == jumptaps

			pmod = fastpow(1 - (jumptaps / taps / extra_divisor), power_scale);
		}

		
		pmod = std::clamp(pmod, min_mod, max_mod);
	}

	auto operator()(const ItvHandInfo& itvhi) -> float
	{
		set_pmod(itvhi);
		return pmod;
	}
};
