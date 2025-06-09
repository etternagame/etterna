#pragma once
#include "../MetaIntervalGenericHandInfo.h"

struct HandSwitchMod
{
	const CalcPatternMod _pmod = HandSwitch;
	const std::string name = "HandSwitchMod";

#pragma region params
	float base = 0.F;
	float min_mod = 0.F;
	float max_mod = 2.F;

	float prop_multi = 0.01F;

	const std::vector<std::pair<std::string, float*>> _params {
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },
		{ "prop_buffer", &prop_multi },
	};

#pragma endregion params and param map
	float pmod = min_mod;

	float _value = .0F;
	int _length = 0;
	int _encountered = 0;

	int _eml = 0;

	int _nm = 0;
	int _om = 0;

	void advance_sequencing(const float& ms_now, const unsigned& notes)
	{
		_eml--;
		if (notes & 0b00100)
		{
			_eml = 2;
		}

		if (_eml > 0)
		{
			if (notes & 0b001) _om++;
			if (notes & 0b11011) _nm++;
		}
		else
		{
		}
	}

	void full_reset()
	{
		_value = .0F;
		_encountered = 0;
		_length = 0;
		_eml = 0;
	}

	auto operator()(const metaItvGenericHandInfo& mitvghi)
	{
		pmod = std::clamp(base + _value / std::max(_encountered, 1), min_mod, max_mod);

		return pmod;
	}
};
