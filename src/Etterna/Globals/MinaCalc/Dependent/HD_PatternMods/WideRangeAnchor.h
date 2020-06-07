#pragma once
#include <string>
#include <array>
#include <vector>

#include "HD_GenericSequencing.h"
#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include "IntervalHandInfo.h"

using std::pair;
using std::vector;

static const CalcPatternMod _pmod = WideRangeAnchor;
static const std::string name = "WideRangeAnchorMod";

// general mod, should maybe make cj specific one
struct WideRangeAnchorMod
{
#pragma region params

	float window_param = 4.F;

	float min_mod = 1.F;
	float max_mod = 1.075F;
	float base = 1.F;

	float diff_min = 4.F;
	float diff_max = 12.F;
	float scaler = 0.1F;

	const vector<pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "diff_min", &diff_min },
		{ "diff_max", &diff_max },
		{ "scaler", &scaler },
	};
#pragma endregion params and param map

	int window = 0;
	int a = 0;
	int b = 0;
	int diff = 0;
	float divisor = diff_max - diff_min;
	float pmod = min_mod;

	inline void full_reset() { pmod = neutral; }

	inline void setup()
	{
		// setup should be run after loading params from disk
		window =
		  CalcClamp(static_cast<int>(window_param), 1, max_moving_window_size);
		divisor = diff_max - diff_min;
	}

	inline auto operator()(const ItvHandInfo& itvhi,
						   const AnchorSequencer& as) -> float
	{
		// nothing here
		if (itvhi.get_taps_nowi() == 0) {
			return neutral;
		}

		// set max mod if either is 0
		if (itvhi.get_col_taps_nowi(col_left) == 0 ||
			itvhi.get_col_taps_nowi(col_right) == 0) {
			return max_mod;
		}

		// now we need these
		a = as.get_max_for_window_and_col(col_left, window);
		b = as.get_max_for_window_and_col(col_right, window);

		// will be set for use after we return from here
		diff = diff_high_by_low(a, b);

		// difference won't matter
		if (diff <= diff_min) {
			return neutral;
		}

		// would max anyway
		if (diff > diff_max) {
			return max_mod;
		}

		pmod =
		  base + (scaler * ((static_cast<float>(diff) - diff_min) / divisor));
		pmod = CalcClamp(pmod, min_mod, max_mod);

		return pmod;
	}
};
