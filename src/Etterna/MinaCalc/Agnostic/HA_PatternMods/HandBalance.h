#pragma once
#include "../../PatternModHelpers.h"

/// Hand-Agnostic PatternMod weighing the balance of notes across hands.
struct HandBalanceMod
{
	const CalcPatternMod _pmod = HandBalance;
	const std::string name = "HandBalance";

#pragma region params

	float min_mod = 0.5F;
	float max_mod = 1.F;

	float power_scaling = 0.5F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },

		{ "power_scaling", &power_scaling },
	};
#pragma endregion params and param map

	float pmod = neutral;

	float left_hand = 0.F;
	float right_hand = 0.F;

	void full_reset()
	{
		pmod = neutral;
		interval_end();
	}

	void advance_sequencing(const int& notes, Calc& calc) {
		left_hand += std::popcount(notes & calc.hand_col_masks.at(0));
		right_hand += std::popcount(notes & calc.hand_col_masks.at(1));
	}

	void interval_end() {
		left_hand = 0;
		right_hand = 0;
	}

	auto operator()() -> float
	{
		if (left_hand == 0 && right_hand == 0) {
			return neutral;
		}

		const auto diff =
		  fabsf(left_hand - right_hand) / ((left_hand + right_hand) / 2.F);

		const auto prop = fastpow(1 - diff, power_scaling);

		pmod = std::clamp(prop, min_mod, max_mod);

		interval_end();
		return pmod;
	}
};
