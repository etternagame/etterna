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
	float base = 0.F;
	float min_mod = 0.F;
	float max_mod = 2.F;

	float decay = 1.0F;

	float prop_buffer = 0.F;
	float prop_scaler = 0.1F;

	float influence_center = 1.F;
	float influence_external = 0.5F;
	float influence_length = 0.125F;

	float length_cap = 1.F;

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
	};

#pragma endregion params and param map
	float pmod = min_mod;

	float _value = 0.F;
	int _encountered = 0;
	int _length = 0;

	int _eml = 0;

	int _nm = 0;
	int _om = 0;

	void advance_sequencing(const float& ms_now, const unsigned& notes)
	{
		_eml--;
		if (notes & 0b00100)
		{
			if ((notes & 0b11) && (notes & 0b00011))
			{
				return;
			}

			_eml = 2;
		}

		if (_eml > 0)
		{
			if (notes & 0b001) _om++;
			if (notes & 0b11011) _nm++;
			_length++;
		}
		else
		{
			if (_length >= 2)
			{
				// only real ones know what the variable names mean.

				float c = _om * influence_center;
				float x = _nm * influence_external;
				float m = std::min(influence_length * _length, length_cap) * (c + x) / _length;

				_value = _value * (1 - decay) + m;
				_encountered++;

				_length = 0;
				_om = 0;
				_nm = 0;
			}
		}
	}

	void full_reset()
	{
		_value = .0F;
		_encountered = 0;

		_length = 0;

		_eml = 0;

		_om = 0;
		_nm = 0;
	}

	auto operator()(const metaItvGenericHandInfo& mitvghi)
	{
		pmod = prop_buffer + _value / std::max(_encountered, 1);
		pmod *= prop_scaler;

		pmod = std::clamp(base + pmod, min_mod, max_mod);

		return pmod;
	}
};
