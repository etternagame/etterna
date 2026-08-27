#pragma once
#include "../MetaIntervalHandInfo.h"

/// Hand downscaler based on how trilly the hand is
struct OldRollScalerMod
{
	const CalcPatternMod _pmod = OldRollScaler;
	const std::string name = "OldRollScalerMod";

#pragma region params

	float min_mod = 0.9F;
	float max_mod = 1.075F;

	float use_cv_threshold = 0.6F;
	float min_cv = 0.15F;
	float cv_base = 0.85F;
	float power_scale = 6.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },

		{ "use_cv_threshold", &use_cv_threshold },
		{ "min_cv", &min_cv },
		{ "cv_base", &cv_base },
		{ "power_scale", &power_scale },
	};
#pragma endregion params and param map

	float pmod = neutral;

	std::vector<float> all_ms{};
	bool left_empty = true;
	bool right_empty = true;

#pragma region generic functions

	void full_reset() {
		pmod = neutral;
		interval_end();
	}

	void interval_end() {
		all_ms.clear();
		left_empty = true;
		right_empty = true;
	}

#pragma endregion

	void advance_sequencing(const col_type& ct, const float& any_ms) {
		switch (ct) {
			case col_left:
				left_empty = false;
				all_ms.push_back(any_ms);
				break;
			case col_right:
				right_empty = false;
				all_ms.push_back(any_ms);
				break;
			case col_ohjump:
				all_ms.push_back(any_ms);
				all_ms.push_back(any_ms);
				left_empty = false;
				right_empty = false;
				break;
			default:
				break;
		}
	}

	void set_pmod(const ItvHandInfo& itvhi)
	{

		if (left_empty || right_empty || all_ms.size() == 1) {
			pmod = neutral;
		} else {

			const auto mmm = mean(all_ms);

			for (auto& x : all_ms) {
				x = mmm / x < use_cv_threshold ? mmm : x;
			}

			const auto dacv = cv(all_ms);
			if (dacv >= min_cv) {
				pmod = fastsqrt(fastsqrt(cv_base + dacv));
			} else {
				pmod = fastpow(cv_base + dacv, power_scale);
			}
		}
		pmod = std::clamp(pmod, min_mod, max_mod);
	}

	auto operator()(const ItvHandInfo& itvhi) -> float
	{
		set_pmod(itvhi);

		// lazy. reset this pmod when we query it
		interval_end();
		return pmod;
	}
};
