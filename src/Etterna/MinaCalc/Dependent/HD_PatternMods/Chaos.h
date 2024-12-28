#pragma once
#include "../IntervalHandInfo.h"

/// slightly different implementation of the old chaos mod, basically picks up
/// polyishness and tries to detect awkward transitions
/// In other words, detects chaotic timing between continuous notes.
struct ChaosMod
{
	const CalcPatternMod _pmod = Chaos;
	const std::string name = "ChaosMod";

#pragma region params

	float min_mod = 0.88F;
	float max_mod = 1.07F;
	float base = -0.088F;

	const std::vector<std::pair<std::string, float*>> _params{
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
		const float a = ms_any.get_now();

		// previous value
		const float b = ms_any.get_last();

		if (any_ms_is_zero(a) || any_ms_is_zero(b) || any_ms_is_close(a, b)) {
			_u(1.F);
			_wot(_u.get_mean_of_window(window));
			return;
		}

		const float prop = div_high_by_low(a, b);
		const int mop = static_cast<int>(prop);
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
		pmod = std::clamp(pmod, min_mod, max_mod);
		return pmod;
	}
};
