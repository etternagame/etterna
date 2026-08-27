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
	float min_mod = 0.5F;
	float max_mod = 1.5F;
	float mod_base = 0.6F;

	float decay = 1.0F;

	float prop_buffer = 0.0F;
	float prop_scaler = 0.67F;

	float influence_center = 0.5F;
	float influence_external = 0.45F;
	float influence_length = 0.25F;

	float length_cap = 1.0F;

	const std::vector<std::pair<std::string, float*>> _params {
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_base", &mod_base },

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
	float last_mod = min_mod;

	float _value = 0.F;
	int _encountered = 0;
	int _length = 0;

	int _eml = 0;

	int _nm = 0;
	int _om = 0;

	int _last = 0;

	// basically do the same thing other pmods do
	void reset_and_decay()
	{
		pmod = std::clamp(last_mod - decay, min_mod, max_mod);
		last_mod = pmod;

		_length = 0;
		_nm = 0;
		_om = 0;
	}

	void advance_sequencing(const float& ms_now, const unsigned& notes)
	{
		_eml--;

		// prevents patterning like this, as it's a different field of tech.
		// 11011
		// 00100

		if ((notes & 0b11) >= 0b11 && (notes & 0b11000) >= 0b11000)
		{
			_eml = 0;
			return;
		}
		if (notes == _last && notes != 0b00100)
		{
			_eml = 0;
			return;
		}
		if (notes & 0b00100)
		{
			// chord that has both hands and middle lane involved should not be counted as handswitch.
			if ((notes & 0b11) && (notes & 0b11000))
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
		last_mod = min_mod;

		_value = 0;
		_encountered = 0;

		_length = 0;

		_eml = 0;

		_om = 0;
		_nm = 0;

		_last = 0;
	}

	auto operator()(const metaItvGenericHandInfo& mitvghi)
	{
		if (mitvghi.total_taps == 0)
		{
			return neutral;
		}

		if (_eml <= 0)
		{
			reset_and_decay();
			return pmod;
		}

		float c = _om * influence_center;
		float x = _nm * influence_external;
		float m = (1 + std::clamp(influence_length * _length, 0.F, length_cap)) * (c + x) / std::max(_length, 1);

		_value = prop_scaler * m + prop_buffer;
		_value = std::max(_value, .0F);

		pmod = std::clamp(mod_base + _value, min_mod, max_mod);
		last_mod = pmod;

		return pmod;
	}
};
