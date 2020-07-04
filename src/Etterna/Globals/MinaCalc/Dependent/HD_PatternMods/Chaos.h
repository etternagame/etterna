#pragma once
#include <string>
#include <array>
#include <vector>

#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include "Etterna/Globals/MinaCalc/CalcWindow.h"

// slightly different implementation of the old chaos mod, basically picks up
// polyishness and tries to detect awkward transitions

struct ChaosMod
{
	const CalcPatternMod _pmod = Chaos;
	const std::string name = "ChaosMod";

#pragma region params

	float min_mod = 0.95F;
	float max_mod = 1.05F;
	float base = -0.1F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },
	};
#pragma endregion params and param map

	// don't allow this to be a modifiable param
	const int window = 6;

	CalcMovingWindow<float> _u;
	CalcMovingWindow<float> _wot;

	float pmod = neutral;

#pragma region generic functions

	void full_reset()
	{
		_u.zero();
		_wot.zero();
		pmod = neutral;
	}

#pragma endregion

	void advance_sequencing(const CalcMovingWindow<float>& ms_any)
	{
		// most recent value
		float a = ms_any.get_now();

		// previous value
		float b = ms_any.get_last();

		if (a == 0.F || b == 0.F || a == b) {
			_u(1.F);
			_wot(_u.get_mean_of_window(window));
			return;
		}

		float prop = div_high_by_low(a, b);
		int mop = static_cast<int>(prop);
		float flop = prop - static_cast<float>(mop);

		if (flop == 0.F) {
			flop = 1.F;
		} else if (flop >= 0.5F) {
			flop = abs(flop - 1.F) + 1.F;

		} else if (flop < 0.5F) {
			flop += 1.F;
		}

		_u(flop);
		_wot(_u.get_mean_of_window(window));
	}

	auto operator()(const int& total_taps) -> float
	{

		if (total_taps == 0) {
			return neutral;
		}

		pmod = base + _wot.get_mean_of_window(max_moving_window_size);
		pmod = CalcClamp(pmod, min_mod, max_mod);
		return pmod;
	}
};
