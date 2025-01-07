#pragma once
#include "../MetaIntervalGenericHandInfo.h"

// 241 should trigger

struct HandSwitchMod
{
	const CalcPatternMod _pmod = HandSwitch;
	const std::string name = "HandSwitchMod";

#pragma region params
	float base = 0.1F;
	float min_mod = 0.25F;
	float max_mod = 1.F;

	const std::vector<std::pair<std::string, float*>> _params {
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },
	};

#pragma endregion params and param map
	float pmod = min_mod;

	int _lc, _md, _rc = 0;
	int yep = 0;

	void advance_sequencing(const float& ms_now, const unsigned& notes) {
		if (notes & 0b00011)
		{
			_lc++;
		}
	}

	void full_reset()
	{
		yep = 0;
	}

	auto operator()(const metaItvGenericHandInfo& mitvghi)
	{
		pmod = fastsqrt(yep / 10.0F);

		return pmod;
	}
};
