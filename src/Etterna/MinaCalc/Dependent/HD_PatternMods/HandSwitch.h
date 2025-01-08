#pragma once
#include "../MetaIntervalGenericHandInfo.h"

// 241 should trigger

struct HandSwitchMod
{
	const CalcPatternMod _pmod = HandSwitch;
	const std::string name = "HandSwitchMod";

#pragma region params
	float base = 0.0F;
	float min_mod = 0.F;
	float max_mod = 2.F;

	float prop_multi = 1.5F;
	float threshold = 0.625F;

	const std::vector<std::pair<std::string, float*>> _params {
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },
		{ "prop_multi", &prop_multi },
		{ "threshold", &threshold },
	};

#pragma endregion params and param map
	float pmod = min_mod;

	float _yep = 0;

	int _spike_amount = 0;
	float _spike_average = 0.F;

	int _mh, _oh, _ml = 0;
	int _hmi = 0;

	void advance_sequencing(const float& ms_now, const unsigned& notes)
	{
		_hmi--;

		// using a xor with a not statement to block off any chords with the middle lane included.
		if (!(notes ^ 0b00100))
		{
			_hmi = 2;
		}

		if (_hmi > 0)
		{
			if (notes & 0b001) _ml++;
			if (notes & 0b110) _mh++;
			if (notes & 0b00011) _oh++;
		}
		else if (_ml >= 1)
		{
			float mf, of;
			float total = _mh + _oh;

			// this should not reward flams
			if (total >= 2) {
				mf = _mh / total;
				of = _oh / total;

				float max = std::max(mf, of);
				if (max >= threshold)
				{
					float yep = std::max(_mh, _oh) / _ml;

					if (yep > 1.F)
					{
						yep = fastsqrt(yep);
					}
					yep *= prop_multi;

					_yep += yep;
					_spike_average += _yep;
				}
			}

			_mh = _oh = _ml = 0;
		}
	}

	void full_reset()
	{
		_yep = 0.F;

		_spike_amount = 0;
		_spike_average = 0.F;

		_mh = _oh = _ml = 0;
		_hmi = 0;
	}

	auto operator()(const metaItvGenericHandInfo& mitvghi)
	{
		_yep *= 0.707;
		pmod = (_spike_average / std::max(static_cast<float>(_spike_amount), 1.F));
		_spike_amount++;

		pmod = std::clamp(base + pmod, min_mod, max_mod);

		return pmod;
	}
};
