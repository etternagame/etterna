#pragma once
#include "../MetaIntervalGenericHandInfo.h"

// The grand comment of what HSM should do.
/*
 * Should be able to take advantage of hand bias to evauluate a handswitch.
 * Should be able to consider a longjack a handswitching pattern.
 * Should be able to consider a trill with the middle lane involved a handswitch.
 * Should be able to consider bracket and chordstreams a handswitch depending on middle lane usage.
 */

struct HandSwitchMod
{
	const CalcPatternMod _pmod = HandSwitch;
	const std::string name = "HandSwitchMod";

#pragma region params
	float base = 0.15F;
	float min_mod = 0.15F;
	float max_mod = 1.68F;

	float decay = 0.02F;

	float prop_buffer = 0.15F;
	float prop_scaler = 0.57F;

	float influence_center = 0.78F;
	float influence_external = 0.70F;
	float influence_length = 0.001F;

	float length_cap = 1.0F;

	float encounter_weight = 0.35f;

	const std::vector<std::pair<std::string, float*>> _params {
		{ "base", &base },
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },

		{ "decay", &decay },

		{ "prop_buffer", &prop_buffer },
		{ "prop_scaler", &prop_scaler },

		{ "influence_center", &influence_center },
		{ "influence_external", &influence_external },
		{ "influence_length", &influence_length },

		{ "length_cap", &length_cap },

		{ "encounter_weight", &encounter_weight },
	};

#pragma endregion params and param map
	float pmod = min_mod;

	float _value = 0.F;
	int _encountered = 0;
	int _length = 0;

	int _eml = 0;

	int _nm = 0;
	int _om = 0;

	int _last = 0;

	float getValue()
	{
		float e = encounter_weight;
		if (e <= 0.0F)
			e = 1.0f;

		return _value / std::max(static_cast<float>((1 / e - 1) + _encountered) * encounter_weight, 1.F);
	}

	// aggressively shoot down value.
	void no()
	{
		_value *= (1 - decay);
	}

	void advance_sequencing(const float& ms_now, const unsigned& notes)
	{
		_eml--;

		// prevents patterning like this, as it's a different field of tech.
		// 11011
		// 00100
		if ((notes & 0b11) >= 0b11 && (notes & 0b00011) >= 0b00011)
		{
			_eml = 0;
			return;
		}
		if (notes == _last && notes != 0b00100)
		{
			no();
			_eml = 0;
			return;
		}
		if (notes & 0b00100)
		{
			// chord that has both hands and middle lane involved should not be counted as handswitch.
			if ((notes & 0b11) && (notes & 0b00011))
			{
				_eml = 0;
				return;
			}

			_eml = 2;
			_om++;

			if (_length == 0) _length++;
		}
		if (_eml > 0)
		{
			if (notes & 0b11011) _nm++;
		}

		if (_eml > 0 && _length > 0)
		{
			_length++;
		}
		_last = notes;
	}

	void full_reset()
	{
		_value = 0.F;
		_encountered = 0;

		_length = 0;

		_eml = 0;

		_om = 0;
		_nm = 0;

		_last = 0;
	}

	auto operator()(const metaItvGenericHandInfo& mitvghi)
	{
		float c = _om * influence_center;
		float x = _nm * influence_external;
		float m = (1 + std::min(influence_length * _length, length_cap)) * (c + x) / std::max(_length, 1);

		_value = _value * (1 - decay) + m;
		_encountered++;

		if (_eml <= 0)
		{
			_length = 0;
			_om = 0;
			_nm = 0;
		}

		pmod = prop_buffer + getValue() * prop_scaler;
		pmod = std::clamp(base + pmod, min_mod, max_mod);

		return pmod;
	}
};
