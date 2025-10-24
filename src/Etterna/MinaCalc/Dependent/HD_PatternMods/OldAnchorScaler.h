#pragma once
#include "../MetaIntervalHandInfo.h"

/// Hand downscaler based on how many notes are on either finger
struct OldAnchorScalerMod
{
	const CalcPatternMod _pmod = OldAnchorScaler;
	const std::string name = "OldAnchorScalerMod";

#pragma region params

	float min_mod = 0.8F;
	float max_mod = 1.05F;

	float extra_divisor = 4.45F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		
		{ "extra_divisor", &extra_divisor },
	};
#pragma endregion params and param map

	float pmod = neutral;

#pragma region generic functions

	void full_reset()
	{
		pmod = neutral;
	}

#pragma endregion

	void set_pmod(const ItvHandInfo& itvhi)
	{

		if (itvhi.get_col_taps_nowi(col_left) == 0 ||
			itvhi.get_col_taps_nowi(col_right) == 0) {
			pmod = neutral;
		} else {
			const auto minval = std::min(itvhi.get_col_taps_nowf(col_left),
										 itvhi.get_col_taps_nowf(col_right));
			const auto maxval = std::max(itvhi.get_col_taps_nowf(col_left),
										 itvhi.get_col_taps_nowf(col_right));
			pmod = fastsqrt(1 - (minval / maxval / extra_divisor));
		}

		pmod = std::clamp(pmod, min_mod, max_mod);
	}

	auto operator()(const ItvHandInfo& itvhi) -> float
	{
		set_pmod(itvhi);
		return pmod;
	}

};
